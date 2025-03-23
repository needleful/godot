#include "dialog_parser.h"

#include "core/os/file_access.h"
#include "core/string_builder.h"

RegEx r_comment("^\\s*//");
RegEx r_whitespace_start("^(\\s+)");
RegEx r_label("^\\s*:(.+)");
RegEx r_narrate("^\\s*\\*\\s*");
RegEx r_reply("^\\s*>\\s*");
RegEx r_special_menu("^\\s*==>\\s*");
RegEx r_context_reply("^\\s*\\?([a-zA-Z0-9_/\\-]*)>");
RegEx r_speaker("^\\s*([^\\-]+)\\s*--\\s*");
RegEx r_whitespace("\\s+");
RegEx r_special_replace("\\$([\\w\\d_]+)");

struct ParseInfo {
	HashMap<String, String> special_functions;
	ParseInfo() {
		special_functions["+stat"] = "Global.add_stat";
		special_functions["stat?"] = "Global.stat";
		special_functions["$"]     = "set_var";
		special_functions["++"]    = "inc_var";
		special_functions["?"]     = "ternary";
		special_functions["exit+"] = "exit_with_effect";
		special_functions["!!!"]   = "alert_new_dialog";
	}
};

static ParseInfo info;

Error DialogParser::parse_dialog(const String &p_path) {
	line_number = 0;
	FileAccessRef f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(!f, ERR_CANT_OPEN, vformat("Couldn't open Dialog file '%s', it may not exist or not be readable.", p_path));
	current_file = p_path;
	Ref<DialogSequence> seq;

	Vector<String> labels;
	String indent = "";
	int current_dialog = -1;
	int current_level = 0;
	Ref<DialogItem> prev;

	Error errcode = OK;

	while(true) {
		String line = f->get_line().strip_edges();
		line_number++;

		if(r_comment.search(line) || line.strip_edges() == "") {
			continue;
		}

		Ref<RegExMatch> label_search = r_label.search(line);
		if(label_search) {
			labels.push_back(label_search->get_string(1));
			continue;
		}

		WhiteSpace wspe = _extract_whitespace(line, indent);

		Ref<DialogItem> wd;
		seq->dialog[line_number] = wd;

		ExtractedLine line_exp;
		Error experr = _extract_expressions(line, line_exp);
		if(experr != OK) {
			ERR_PRINT("Error reading expression at "+current_file+": "+itos(line_number));
			errcode = experr;
		}
		else {
			line = line_exp.line;
			wd->set_conditions(line_exp.conditions);
		}

		TypeLine tl = _extract_type(line);
		line = tl.line;
		wd->set_type(tl.type);
		wd->set_speaker(tl.context);

		if(wd->get_type() == DialogItem::MESSAGE) {
			SpeakerLine sl = _extract_speaker(line);
			wd->set_speaker(sl.speaker);
			line = sl.line;
		}

		wd->set_text(line.strip_edges());

		indent = wspe.indent;
		int level_change = wspe.indent_level - current_level;
		if(level_change > 1) {
			ERR_PRINT("Extra indentation at "+current_file+": "+itos(line_number));
			level_change = 1;
		}
		// Apply parent/next relations
		if(current_dialog != -1) {
			if(level_change == 0) {	
				prev->set_next(line_number);
				wd->set_parent(prev->get_parent());
			}
			else if(level_change > 0) {
				prev->set_child(line_number);
				wd->set_parent(current_dialog);
			}
			else {
				int lv = level_change;
				Ref<DialogItem> previous = prev;
				while(lv < 0) {
					if(previous->get_parent() == -1){
						ERR_PRINT("Error at: "+current_file+": "+ itos(line_number) + "; Indented block with no parent.");
						return ERR_BUG;
					}
					previous = seq->dialog[previous->get_parent()];
					lv += 1;
				}
				previous->set_next(line_number);
				wd->set_parent(previous->get_parent());
			}
		}
		if ( wd->get_speaker() == "" 
			&& seq->dialog.has(wd->get_parent())
			&& seq->dialog[wd->get_parent()]->get_type() != DialogItem::CONTEXT_REPLY
			&& seq->dialog[wd->get_parent()]->get_speaker() != StringName("")
		){
			wd->set_speaker(seq->dialog[wd->get_parent()]->get_speaker());
		}
		current_dialog = line_number;
		current_level += level_change;
		prev = wd;
		for(int l = 0; l < labels.size(); l++) {
			String label = labels[l];
			seq->labels[label] = line_number;
		}
		labels.clear();
	}
	result = seq;
	return errcode;
}

DialogParser::WhiteSpace DialogParser::_extract_whitespace(const String &p_line, const String &p_indent) {
	WhiteSpace wsp;
	String space = "";
	
	Ref<RegExMatch> whitespace = r_whitespace_start.search(p_line);
	String indent2 = p_indent;
	if(whitespace) {
		space = whitespace->get_string(1);
		if (p_indent == "") {
			wsp.indent = space;
			indent2 = space;
		}
	}

	if(space != "" && (space.length() % indent2.length() != 0)) {
		WARN_PRINT("\tInvalid indentation: {"+space+"} at: "+ current_file+ ":"+ itos(line_number));
	}

	if(space.length()) {
		wsp.indent_level = space.length() / indent2.length();
	}
	else {
		wsp.indent_level = 0;
	}
	return wsp;
}

Error DialogParser::_extract_expressions(const String &p_line, DialogParser::ExtractedLine &r_exp) {
	if(p_line.find("{") == -1 || p_line.find("[") == -1) {
		r_exp.line = p_line;
		return OK;
	}
	Vector<String> exp_strings;
	bool interp_expression = false;
	String l = p_line;
	StringBuilder line_no_expressions;

	while(l != "") {
		switch(l[0]) {
		case '[':
		case '{': {
			String end_char = l[0] == '['
				? "]"
				: "}";
			int end = l.find(end_char);
			ERR_FAIL_COND_V_MSG(end < 0, ERR_PARSE_ERROR, "Missing end character for expression: "+ end_char);
			
			String ex_parsed;
			{
				String exp_raw = l.substr(1, end-1);
				Error perr = l[0] == '['
				? _exp_sq_convert(exp_raw, ex_parsed)
				: _exp_replace_vars(exp_raw, ex_parsed);
				ERR_FAIL_COND_V(perr != OK, perr);
			}
			
			if (interp_expression) {
				line_no_expressions += "#{" + ex_parsed + "}";
			}
			else {
				exp_strings.push_back(ex_parsed);
			}

			interp_expression = false;
			break;
		}
		case '#': {
			interp_expression = true;
			l = l.substr(1);
			break;
		}
		default: {
			interp_expression = false;
			line_no_expressions += String(&l[0], 1);
			l = l.substr(1);
		}
		}
	}
	r_exp.line = line_no_expressions;
	r_exp.conditions = exp_strings;
	return OK;
}


DialogParser::TypeLine DialogParser::_extract_type(const String &p_line){
	Ref<RegExMatch> match;
	#define With_Match(Regex, T) \
		match = Regex.search(p_line); \
		if(match) { \
			return TypeLine( \
				DialogItem::T, \
				p_line.replace(match->get_string(0), ""), \
				match->get_strings().size() > 1 ? match->get_string(1) : "" \
			); \
		}

		With_Match(r_narrate, NARRATION);
		With_Match(r_special_menu, SPECIAL_MENU_ENTRY);
		With_Match(r_reply, REPLY);
		With_Match(r_context_reply, CONTEXT_REPLY);

	#undef With_Match

	return TypeLine(DialogItem::MESSAGE, p_line, "");
}

DialogParser::SpeakerLine DialogParser::_extract_speaker(const String &p_line){
	Ref<RegExMatch> m = r_speaker.search(p_line);
	if (m) {
		return SpeakerLine(
			StringName(m->get_string(1).strip_edges()),
			p_line.replace(m->get_string(0), "")
		);
	}
	else {
		return SpeakerLine(StringName(""), p_line);
	}
}

Error DialogParser::_exp_sq_convert(const String &p_sqexp, String &r_parsed){
	String func_str = "";
	String args_str = "";
	int idx = p_sqexp.find(":");

	if(idx >= 0) {
		func_str = p_sqexp.substr(0, idx);
		args_str = p_sqexp.substr(idx+1);
	}
	else {
		func_str = p_sqexp.strip_edges();
	}

	StringBuilder slot;
	bool negated = false;
	Vector<String> func_list = func_str.split(" ", false);
	for(int i = 0; i < func_list.size(); i++) {
		String f = func_list[i];
		if(info.special_functions.has(f)) {
			f = info.special_functions[f];
		}
		else if(f.begins_with("$")) {
			Error p_err = _exp_replace_vars(f, f);
			ERR_FAIL_COND_V(p_err != OK, p_err);
		}
		else if(f == "!") {
			negated = true;
		}
		if(slot.num_strings_appended() && !negated) {
			slot += ".";
		}
		slot += f.strip_edges();
		if(negated){
			negated = false;
		}
	} 

	StringBuilder message;
	if(args_str != "") {
		Vector<String> arg_list = args_str.split("|");
		for(int i = 0; i < arg_list.size(); i++) {
			if(message.num_strings_appended()) {
				message += ", ";
			}
			String a = arg_list[i];
			if(a.begins_with("+")) {
				a = a.substr(1);
				message += "[";
				int items = 0;
				Vector<String> sub_list = a.split(" ", false);
				for (int j = 0; j < sub_list.size(); j++) {
					String s2;
					Error subErr = _exp_sq_arg(sub_list[j], s2);
					ERR_FAIL_COND_V(subErr != OK, subErr);

					if(items) {
						message += ",";
					}
					message += s2;
					items ++;
				}
				message += "]";
			}
		}
	}
	r_parsed = slot + "(" + message + ")";
	return OK;
}

Error DialogParser::_exp_sq_arg(const String &p_sqarg, String &r_parsed) {
	String s = p_sqarg.strip_edges();

	if(s.begins_with("#")) {
		return _exp_replace_vars(s.substr(1), r_parsed);
	}
	String replaced;
	Error errx = _exp_replace_vars(s, replaced);
	ERR_FAIL_COND_V(errx != OK, errx);

	if (replaced != s) {
		r_parsed = replaced;
	}
	else {
		r_parsed = "\"" + s.replace("\"", "\\\"") + "\"";
	}
	return OK;
}

Error DialogParser::_exp_replace_vars(const String &p_cexp, String &r_parsed){
	String replaced = p_cexp;

	Array results = r_special_replace.search_all(p_cexp); 
	for(int i = 0; i < results.size(); i++) {
		Ref<RegExMatch> m = (Ref<RegExMatch>)results[i];
		String m1 = m->get_string(0);
		String m2 = "variables['"+m->get_string(1)+"']";
		replaced = replaced.replace(m1, m2);
	}
	r_parsed = replaced;
	return OK;
}

DialogParser::DialogParser(){}
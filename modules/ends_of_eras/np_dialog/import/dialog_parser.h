#ifndef NP_DIALOG_PARSER_H
#define NP_DIALOG_PARSER_H

#include "modules/ends_of_eras/np_dialog/dialog_sequence.h"
#include "modules/regex/regex.h"

struct DialogParser {
	Error parse_dialog(const String &p_path);
	Ref<DialogSequence> result;
	DialogParser();
private:
	String current_file;
	unsigned line_number;
	RegEx
		r_comment, r_whitespace_start, r_label, r_narrate, r_reply, 
		r_special_menu, r_context_reply, r_speaker, r_whitespace;
	struct WhiteSpace {
		String indent;
		unsigned indent_level;
	};
	WhiteSpace _extract_whitespace(const String &line, const String &indent);

	struct ExtractedLine {
		String line;
		Vector<String> conditions;
	};
	Error _extract_expressions(const String &line, ExtractedLine &exp);

	struct TypeLine {
		String line;
		DialogItem::Type type;
		String context;
		TypeLine(DialogItem::Type p_type, String p_line, String p_context)
		: type(p_type), line(p_line), context(p_context) {}; 
	};
	TypeLine _extract_type(const String &p_line);

	struct SpeakerLine {
		StringName speaker;
		String line;
		SpeakerLine(StringName p_speaker, String p_line)
		: speaker(p_speaker), line(p_line) {}; 
	};
	SpeakerLine _extract_speaker(const String &p_line);

	Error _exp_sq_convert(const String &p_sqexp, String &r_parsed);
	Error _exp_sq_arg(const String &p_sqarg, String &r_parsed);
	Error _exp_replace_vars(const String &p_cexp, String &r_parsed);
};

#endif // NP_DIALOG_PARSER_H
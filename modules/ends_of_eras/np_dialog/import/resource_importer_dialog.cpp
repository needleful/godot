
#include "core/io/resource_saver.h"

#include "dialog_parser.h"
#include "resource_importer_dialog.h"

String ResourceImporterDialog::get_importer_name() const {
	return "np_dialog";
}
String ResourceImporterDialog::get_visible_name() const {
	return "needleful's Powerful Dialog System";
}
void ResourceImporterDialog::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("dialog");
}
String ResourceImporterDialog::get_save_extension() const {
	return "tres";
}
String ResourceImporterDialog::get_resource_type() const {
	return "DialogSequence";
}

int ResourceImporterDialog::get_preset_count() const {
	return 0;
}
String ResourceImporterDialog::get_preset_name(int p_idx) const {
	return "";
}

void ResourceImporterDialog::get_import_options(List<ImportOption> *r_options, int p_preset) const {
}

bool ResourceImporterDialog::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	return true;
}

Error ResourceImporterDialog::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	DialogParser parser;
	Error e = parser.parse_dialog(p_source_file);

	ERR_FAIL_COND_V(e != OK, e);
	
	String out_path = p_save_path + ".tres";
	e = ResourceSaver::save(out_path, parser.result);
	ERR_FAIL_COND_V_MSG(e != OK, e, "Issue when saving dialog file");
	return OK;
}

ResourceImporterDialog::ResourceImporterDialog() {}

static void ResourceImporterDialog::_bind_methods() {
	
}
#include "core/class_db.h"

#include "modules/ends_of_eras/np_dialog/np_dialog.h"

static void register_dialog_types() {
	ClassDB::register_class<ResourceImporterDialog>();
	ClassDB::register_class<DialogSequence>();
}

void register_ends_of_eras_types() {
	register_dialog_types();
}

void unregister_ends_of_eras_types() {}
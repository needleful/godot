// register_types.cpp
// Pattented needleful yeehaw!!!

#include "register_types.h"

#include "cims_creator.h"
#include "core/class_db.h"

void register_cims_creator_types() {
	ClassDB::register_class<CimsStructure>();
}

void unregister_cims_creator_types() {
	return;
}

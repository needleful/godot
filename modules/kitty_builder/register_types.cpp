
#include "player.h"
#include "register_types.h"


#include <core/class_db.h>

void register_kitty_builder_types() {
	ClassDB::register_class<Leggy>();
}

void unregister_kitty_builder_types() { }

#include "register_types.h"
#include "core/class_db.h"
#include "multi_mesh_system.h"

void register_np_multi_mesh_types() {
	ClassDB::register_class<MultiMeshComponent>();
	ClassDB::register_class<MultiMeshSystem>();
}

void unregister_np_multi_mesh_types() {}
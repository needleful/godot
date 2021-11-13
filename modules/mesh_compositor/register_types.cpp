
#include "register_types.h"

#include "core/class_db.h"
#include "mesh_compositor.h"


void register_mesh_compositor_types() {
	ClassDB::register_class<MC_Component>();
	ClassDB::register_class<MeshCompositor>();
}
void unregister_mesh_compositor_types() {}
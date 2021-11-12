
#include "mesh_compositor.h"

MC_Component::MC_Component() {}

MeshCompositor::MeshCompositor(){}

void MeshCompositor::_enter_tree() {
	for(size_t i = 0; i < colored_meshes.size(); i++) {
		Mesh* m = colored_meshes[i];
		add_multimesh(m, true);
	}
}


#include "mesh_compositor.h"

MC_Component::MC_Component() {}

void MC_Component::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
}

Ref<Mesh> MC_Component::get_mesh() const {
	return mesh;
}

void MC_Component::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MC_Component::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &MC_Component::get_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
}

/////  MESH COMPOSITOR  /////

MeshCompositor::MeshCompositor()
: dynamic(true), render_layers(0x1) {
}


Array MeshCompositor::get_colored_meshes() {
	return colored_meshes;
}

void MeshCompositor::set_colored_meshes(const Array &p_meshes) {
	colored_meshes = p_meshes;
}

int MeshCompositor::get_render_layers() const {
	return render_layers;
}

void MeshCompositor::set_render_layers(int p_layers) {
	render_layers = p_layers;
}

bool MeshCompositor::is_dynamic() const {
	return dynamic;
}

void MeshCompositor::set_dynamic(bool p_dynamic) {
	dynamic = p_dynamic;
}

void MeshCompositor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_colored_meshes", "meshes"), &MeshCompositor::set_colored_meshes);
	ClassDB::bind_method(D_METHOD("get_colored_meshes"), &MeshCompositor::get_colored_meshes);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "colored_meshes", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_colored_meshes", "get_colored_meshes");

	ClassDB::bind_method(D_METHOD("set_render_layers", "layers"), &MeshCompositor::set_render_layers);
	ClassDB::bind_method(D_METHOD("get_render_layers"), &MeshCompositor::get_render_layers);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_layers", PROPERTY_HINT_LAYERS_3D_RENDER), "set_render_layers", "get_render_layers");

	ClassDB::bind_method(D_METHOD("set_dynamic", "dynamic"), &MeshCompositor::set_dynamic);
	ClassDB::bind_method(D_METHOD("is_dynamic"), &MeshCompositor::is_dynamic);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dynamic"), "set_dynamic", "is_dynamic");
}

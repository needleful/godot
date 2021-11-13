
#include "mesh_compositor.h"

#include "core/math/math_funcs.h"

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

void MeshCompositor::initialize() {
	inv_transform = get_global_transform().affine_inverse();
	find_components(this);
	configure_meshes();
}

void MeshCompositor::find_components(const Node *p_node) {
	for(int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		MC_Component *mc = Object::cast_to<MC_Component>(c);
		if(mc) {
			if(!meshes.has(mc->get_mesh()->get_path())) {
				add_multimesh(mc->get_mesh(), false);
			}
			add_component(mc);
		}
		if (c->get_child_count() > 0) {
			find_components(c);
		}
	}
}

void MeshCompositor::add_multimesh(const Ref<Mesh> p_mesh, bool p_colored) {
	MultiMeshInstance* mmi = memnew(MultiMeshInstance);
	Ref<MultiMesh> multimesh = Ref<MultiMesh>(memnew(MultiMesh));
	mmi->set_multimesh(multimesh);
	multimesh->set_mesh(p_mesh);
	if(p_colored) {
		multimesh->set_color_format(MultiMesh::COLOR_8BIT);
	}
	else {
		multimesh->set_color_format(MultiMesh::COLOR_NONE);
	}
	multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
	meshes[p_mesh->get_path()] = mmi;
	components[p_mesh->get_path()] = Vector<MC_Component*>();
	add_child(mmi);
}

void MeshCompositor::add_component(MC_Component *p_comp) {
	components[p_comp->get_mesh()->get_path()].push_back(p_comp);
}

void MeshCompositor::configure_meshes() {
	for(auto *elem = meshes.front(); elem; elem = elem->next()) {
		Vector<MC_Component*> mc = components[elem->key()];
		Ref<MultiMesh> multimesh = elem->value()->get_multimesh();
		if(multimesh->get_color_format() != MultiMesh::COLOR_NONE) {
			for(int i = 0; i < mc.size(); i++) {
				multimesh->set_instance_color(i, Color(Math::randf(), Math::randf(), Math::randf()));
			}
		}
	}
	update_transforms();
}

void MeshCompositor::update_transforms() {
	for(auto *elem = meshes.front(); elem; elem = elem->next()) {
		Vector<MC_Component*> comps = components[elem->key()];
		Ref<MultiMesh> multimesh = elem->value()->get_multimesh();

		for(int i = 0; i < comps.size(); i++) {
			multimesh->set_instance_transform(i, inv_transform*comps[i]->get_global_transform());
		}
	}
}

void MeshCompositor::clear() {
	meshes.clear();
	components.clear();
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

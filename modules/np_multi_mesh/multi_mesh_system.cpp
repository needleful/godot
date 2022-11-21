// by needleful

#include "multi_mesh_system.h"

#include "core/print_string.h"
#include "scene/3d/mesh_instance.h"
#include "scene/resources/material.h"

MultiMeshComponent::MultiMeshComponent() :
		index(-1) {}

void MultiMeshComponent::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		MultiMeshSystem *system = _find_system();
		if (system) {
			system->_add_component(this);
		}
	} else if (p_what == NOTIFICATION_READY) {
		if (!_find_system()) {
			// fall back to MeshInstance
			MeshInstance *meshi = memnew(MeshInstance);
			meshi->set_mesh(mesh);
			meshi->set_name("__fallback_mesh");
			add_child(meshi);
		}
	} else if (p_what == NOTIFICATION_TRANSFORM_CHANGED) {
		if (index < 0) {
			return;
		}
		MultiMeshSystem *system = _find_system();
		if (system) {
			system->_update_component_transform(this);
		}
	} else if (p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		MultiMeshSystem *system = _find_system();
		if (system) {
			if (is_visible_in_tree())
				system->_add_component(this);
			else
				system->_remove_component(this);
		}
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		if (index < 0) {
			return;
		}
		MultiMeshSystem *system = _find_system();
		if (system) {
			system->_remove_component(this);
		}
	}
}

MultiMeshSystem *MultiMeshComponent::_find_system() {
	Node *parent = get_parent();
	ERR_FAIL_COND_V_MSG(!parent, nullptr, "node is not inside tree");
	while (parent) {
		MultiMeshSystem *system = Object::cast_to<MultiMeshSystem>(parent);
		if (system) {
			return system;
		}
		parent = parent->get_parent();
	}
	return nullptr;
}

void MultiMeshComponent::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
}

Ref<Mesh> MultiMeshComponent::get_mesh() const {
	return mesh;
}

void MultiMeshComponent::set_material_override(const Ref<Material> &p_material) {
	material_override = p_material;
}

Ref<Material> MultiMeshComponent::get_material_override() const {
	return material_override;
}

void MultiMeshComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MultiMeshComponent::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &MultiMeshComponent::get_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");

	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &MultiMeshComponent::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &MultiMeshComponent::get_material_override);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material_override", "get_material_override");
}

// MultiMeshSystem

MultiMeshSystem::MultiMeshSystem() {}

void MultiMeshSystem::_add_component(MultiMeshComponent *p_component) {
	ERR_FAIL_NULL(p_component);
	ERR_FAIL_COND_MSG(!p_component->get_mesh().is_valid(), "Component has no mesh: " + get_path());
	uint32_t mesh_id = p_component->get_mesh()->get_rid().get_id();
	if (!meshes.has(mesh_id)) {
		MultiMeshInstance *mmi = memnew(MultiMeshInstance);

		Ref<MultiMesh> mm = Ref<MultiMesh>(memnew(MultiMesh));
		mm->set_visible_instance_count(0);
		mm->set_mesh(p_component->get_mesh());
		mm->set_color_format(MultiMesh::COLOR_NONE);
		mm->set_transform_format(MultiMesh::TRANSFORM_3D);
		mmi->set_multimesh(mm);
		if (p_component->get_material_override().is_valid()) {
			mmi->set_material_override(p_component->get_material_override());
		}

		meshes[mesh_id] = mmi;
		components[mesh_id] = Vector<MultiMeshComponent *>();
		// TODO: add logic for adding once the node is ready.
	}

	components[mesh_id].push_back(p_component);
	p_component->index = components[mesh_id].size() - 1;

	if (meshes[mesh_id]->is_inside_tree()) {
		Ref<MultiMesh> mesh = meshes[mesh_id]->get_multimesh();
		int vis = mesh->get_visible_instance_count();
		mesh->set_visible_instance_count(vis + 1);
		mesh->set_instance_count(components[mesh_id].size());
		mesh->set_instance_transform(p_component->index,
				get_global_transform().affine_inverse() * p_component->get_global_transform());
	}
}

void MultiMeshSystem::_update_component_transform(MultiMeshComponent *p_component) {
	ERR_FAIL_NULL(p_component);
	ERR_FAIL_COND_MSG(!p_component->get_mesh().is_valid(), "Component has no mesh: " + get_path());
	ERR_FAIL_COND_MSG(p_component->index < 0, "Invalid component index: " + p_component->get_path());

	uint32_t mesh_id = p_component->get_mesh()->get_rid().get_id();
	ERR_FAIL_COND_MSG(!meshes.has(mesh_id), "Mesh was not added for component: " + p_component->get_path());
	ERR_FAIL_COND_MSG(p_component->index >= meshes[mesh_id]->get_multimesh()->get_instance_count(), "Invalid component index: " + p_component->get_path());
	meshes[mesh_id]->get_multimesh()->set_instance_transform(p_component->index, get_global_transform().affine_inverse() * p_component->get_global_transform());
}

void MultiMeshSystem::_remove_component(MultiMeshComponent *p_component) {
	ERR_FAIL_NULL(p_component);
	ERR_FAIL_COND_MSG(!p_component->get_mesh().is_valid(), "Component has no mesh: " + get_path());

	uint32_t mesh_id = p_component->get_mesh()->get_rid().get_id();

	ERR_FAIL_COND_MSG(!meshes.has(mesh_id), "Mesh was not added for component: " + p_component->get_path());
	ERR_FAIL_COND_MSG(p_component->index < 0, "Invalid component index: " + p_component->get_path());
	if (!meshes[mesh_id]->is_inside_tree()) {
		// irrelevant
		return;
	}
	Vector<MultiMeshComponent *> &comps = components[mesh_id];
	Ref<MultiMesh> multimesh = meshes[mesh_id]->get_multimesh();

	int end = multimesh->get_visible_instance_count() - 1;
	if (p_component->index < end) {
		multimesh->set_instance_transform(p_component->index, multimesh->get_instance_transform(end));
		comps.set(p_component->index, comps[end]);
		comps[end]->index = p_component->index;
	}

	comps.resize(end);
	multimesh->set_visible_instance_count(comps.size());

	p_component->index = -1;
}

void MultiMeshSystem::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		// Create the multimeshes
		const uint32_t *mesh_id = NULL;
		Transform inv_transform = get_global_transform().affine_inverse();
		while ((mesh_id = meshes.next(mesh_id))) {
			MultiMeshInstance *m = meshes[*mesh_id];
			Vector<MultiMeshComponent *> &comps = components[*mesh_id];
			int visible_count = 0;

			m->get_multimesh()->set_instance_count(comps.size());
			for (int i = 0; i < comps.size(); i++) {
				m->get_multimesh()->set_instance_transform(i, inv_transform * comps[i]->get_global_transform());
				comps[i]->set_notify_transform(true);
			}
			m->get_multimesh()->set_visible_instance_count(m->get_multimesh()->get_instance_count());
			m->set_name("__mmi_" + m->get_multimesh()->get_mesh()->get_name());
			add_child(m);
		}
	}
}

void MultiMeshSystem::_bind_methods() {}
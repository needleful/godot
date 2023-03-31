// by needleful

#ifndef NP_MULTIMESH_SYSTEM_H
#define NP_MULTIMESH_SYSTEM_H

#include "core/typedefs.h"
#include "scene/3d/multimesh_instance.h"

class MultiMeshSystem;

class MultiMeshComponent : public Spatial {
	GDCLASS(MultiMeshComponent, Spatial);
	friend class MultiMeshSystem;

protected:
	Ref<Mesh> mesh;
	Ref<Material> material_override;
	int index;
	MultiMeshSystem *system;
	MultiMeshSystem *_find_system();
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const;

	MultiMeshComponent();
};

class MultiMeshSystem : public Spatial {
	GDCLASS(MultiMeshSystem, Spatial);
	friend class MultiMeshComponent;

protected:
	// The uint32_t here is the internal id of the mesh resource or something.
	HashMap<uint32_t, MultiMeshInstance *> meshes;
	HashMap<uint32_t, Vector<MultiMeshComponent *>> components;
	bool ready;

	void _add_component(MultiMeshComponent *p_component);
	void _update_component_transform(MultiMeshComponent *p_component);
	void _remove_component(MultiMeshComponent *p_component);
	void _notification(int p_what);
	static void _bind_methods();

public:
	MultiMeshSystem();
};

#endif //NP_MULTIMESH_SYSTEM_H
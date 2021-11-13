
#ifndef M_COMPOSITOR_H
#define M_COMPOSITOR_H

#include "scene/3d/multimesh_instance.h"

class MC_Component: public Spatial {
	GDCLASS(MC_Component, Spatial);
protected:
	Ref<Mesh> mesh;

	static void _bind_methods();
public:
	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;
	MC_Component();
};

class MeshCompositor: public Spatial {
	GDCLASS(MeshCompositor, Spatial);
protected:
	Array colored_meshes;
	Transform inv_transform;
	Map<String, MultiMeshInstance*> meshes;
	Map<String, Vector<MC_Component*>> components;
	int render_layers;
	bool dynamic;
	static void _bind_methods();
public:
	MeshCompositor();

	Array get_colored_meshes();
	void set_colored_meshes(const Array &p_meshes);

	int get_render_layers() const;
	void set_render_layers(int p_layers);

	bool is_dynamic() const;
	void set_dynamic(bool p_dynamic);

	void initialize();

	void find_components(const Node *node);

	void configure_meshes();

	void add_multimesh(const Ref<Mesh> p_mesh, bool p_colored);

	void add_component(MC_Component *p_comp);
	
	void update_transforms();

	void clear();

};

// M_COMPOSITOR_H
#endif
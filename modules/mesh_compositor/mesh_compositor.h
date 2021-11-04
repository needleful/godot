
#ifndef M_COMPOSITOR_H
#define M_COMPOSITOR_H

#include "scene/3d/spatial.h"

class MC_Component: public Spatial {
	GDCLASS(MC_Component, Spatial);
	Ref<Mesh> mesh;

public:
	void set_mesh(Ref<Mesh>);
	Ref<Mesh> get_mesh();

	MC_Component();
	~MC_Component();
}

class MeshCompositor: public Spatial {
	GDCLASS(MeshCompositor, Spatial);

private:
	bool dynamic;
	Ref<Material> colored_material;
	Vector<Mesh*> colored_meshes;

	Dictionary meshes;
	Dictionary components;

	Transform inv_transform;

	void _add_multimesh(Ref<Mesh> mesh, bool colored);
	void _add_component(MC_Component);
	void _configure_meshes();

public:

	void clear();
	void initialize();
	void update_transforms();

	MeshCompositor();
	~MeshCompositor();

}


// M_COMPOSITOR_H
#endif
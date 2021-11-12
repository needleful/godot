
#ifndef M_COMPOSITOR_H
#define M_COMPOSITOR_H

#include "scene/3d/spatial.h"

class MC_Component: public Spatial {
	GDCLASS(MC_Component, Spatial);
	static void _bind_methods();
	MC_Component();
};

class MeshCompositor: public Spatial {
	GDCLASS(MeshCompositor, Spatial);
protected:
	static void _bind_methods();
	MeshCompositor();

};


// M_COMPOSITOR_H
#endif
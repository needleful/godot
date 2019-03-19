// cims_creator.h
// cowboy

#ifndef CIMS_CREATOR_H
#define CIMS_CREATOR_H

#include "scene/3d/spatial.h"

class CimsStructure : public Spatial {
	GDCLASS(CimsStructure, Spatial);

protected:
	static void _bind_methods();

public:
	int cowboy_yehow();
};

#endif

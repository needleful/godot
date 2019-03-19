// player.h
// for kitty!!!

#ifndef KITTY_PLAYER_H
#define KITTY_PLAYER_H

#include <scene/3d/mesh_instance.h>
#include <scene/3d/ray_cast.h>

class Leggy : public RayCast {
	GDCLASS(Leggy, RayCast);
	float alignment;
	
protected:
	static void _bind_methods();

public:
	void set_alignment(real_t p_alignment);
	float get_alignment() const;
	Vector3 get_snap_point();
	Leggy(real_t a = 0.5);
};

#endif

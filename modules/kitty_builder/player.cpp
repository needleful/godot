#include "player.h"

Leggy::Leggy(real_t a) :
		alignment(a) {}

void Leggy::set_alignment(real_t p_alignment) {
	this->alignment = p_alignment;
#ifdef DEBUG_ENABLED
	emit_signal("alignment_set");
#endif
}

real_t Leggy::get_alignment() const {
	return alignment;
}

void Leggy::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_snap_point"), &Leggy::get_snap_point);
	ClassDB::bind_method(D_METHOD("get_alignment"), &Leggy::get_alignment);
	ClassDB::bind_method(D_METHOD("set_alignment"), &Leggy::set_alignment);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "alignment", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"), "set_alignment", "get_alignment");
	ADD_SIGNAL(MethodInfo("alignment_set"));
}

Vector3 Leggy::get_snap_point() {
	Basis b = get_global_transform().basis;
	Vector3 cast_to = get_cast_to();
	Vector3 c = (cast_to.x * b.get_axis(0)) + (cast_to.y * b.get_axis(1)) + (cast_to.z * b.get_axis(2));

	return get_global_transform().origin + (c * alignment);
}

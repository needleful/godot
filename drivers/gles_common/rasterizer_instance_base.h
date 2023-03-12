
#ifndef RASTERIZER_INSTANCE_BASE
#define RASTERIZER_INSTANCE_BASE

#include "core/math/camera_matrix.h"
#include "core/math/transform_interpolator.h"
#include "core/self_list.h"
#include "servers/visual_server.h"

struct RasterizerInstanceBase : RID_Data {
	VS::InstanceType base_type;
	RID base;

	RID skeleton;
	RID material_override;
	RID material_overlay;

	// This is the main transform to be drawn with ..
	// This will either be the interpolated transform (when using fixed timestep interpolation)
	// or the ONLY transform (when not using FTI).
	Transform transform;

	// for interpolation we store the current transform (this physics tick)
	// and the transform in the previous tick
	Transform transform_curr;
	Transform transform_prev;

	int depth_layer;
	uint32_t layer_mask;

	//RID sampled_light;

	Vector<RID> materials;
	Vector<RID> light_instances;
	Vector<RID> reflection_probe_instances;

	PoolVector<float> blend_values;

	VS::ShadowCastingSetting cast_shadows;

	//fit in 32 bits
	bool mirror : 1;
	bool receive_shadows : 1;
	bool visible : 1;
	bool redraw_if_visible : 1;

	bool on_interpolate_list : 1;
	bool on_interpolate_transform_list : 1;
	bool interpolated : 1;
	TransformInterpolator::Method interpolation_method : 3;

	// For fixed timestep interpolation.
	// Note 32 bits is plenty for checksum, no need for real_t
	float transform_checksum_curr;
	float transform_checksum_prev;

	float depth; //used for sorting

	SelfList<RasterizerInstanceBase> dependency_item;

	virtual void base_removed() = 0;
	virtual void base_changed(bool p_aabb, bool p_materials) = 0;
	RasterizerInstanceBase() :
			dependency_item(this) {
		base_type = VS::INSTANCE_NONE;
		cast_shadows = VS::SHADOW_CASTING_SETTING_ON;
		receive_shadows = true;
		visible = true;
		depth_layer = 0;
		layer_mask = 1;
		redraw_if_visible = false;
		on_interpolate_list = false;
		on_interpolate_transform_list = false;
		interpolated = true;
		interpolation_method = TransformInterpolator::INTERP_LERP;
		transform_checksum_curr = 0.0;
		transform_checksum_prev = 0.0;
	}
};

#endif

#ifndef RASTERIZER_INSTANCE_BASE
#define RASTERIZER_INSTANCE_BASE

#include "core/list.h"
#include "core/math/bvh.h"
#include "core/math/camera_matrix.h"
#include "core/math/transform_interpolator.h"
#include "core/self_list.h"
#include "core/set.h"
#include "servers/visual/portals/portal_renderer.h"
#include "servers/visual/portals/portal_types.h"
#include "servers/visual_server.h"

// common interface for all spatial partitioning schemes
// this is a bit excessive boilerplatewise but can be removed if we decide to stick with one method

// note this is actually the BVH id +1, so that visual server can test against zero
// for validity to maintain compatibility with octree (where 0 indicates invalid)
typedef uint32_t SpatialPartitionID;

struct RasterizerInstance;

class SpatialPartitioningScene {
public:
	virtual SpatialPartitionID create(RasterizerInstance *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t pairable_mask = 1) = 0;
	virtual void erase(SpatialPartitionID p_handle) = 0;
	virtual void move(SpatialPartitionID p_handle, const AABB &p_aabb) = 0;
	virtual void activate(SpatialPartitionID p_handle, const AABB &p_aabb) {}
	virtual void deactivate(SpatialPartitionID p_handle) {}
	virtual void force_collision_check(SpatialPartitionID p_handle) {}
	virtual void update() {}
	virtual void update_collisions() {}
	virtual void set_pairable(RasterizerInstance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) = 0;
	virtual int cull_convex(const Vector<Plane> &p_convex, RasterizerInstance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF) = 0;
	virtual int cull_aabb(const AABB &p_aabb, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF) = 0;
	virtual int cull_segment(const Vector3 &p_from, const Vector3 &p_to, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF) = 0;

	typedef void *(*PairCallback)(void *, uint32_t, RasterizerInstance *, int, uint32_t, RasterizerInstance *, int);
	typedef void (*UnpairCallback)(void *, uint32_t, RasterizerInstance *, int, uint32_t, RasterizerInstance *, int, void *);

	virtual void set_pair_callback(PairCallback p_callback, void *p_userdata) = 0;
	virtual void set_unpair_callback(UnpairCallback p_callback, void *p_userdata) = 0;

	// bvh specific
	virtual void params_set_node_expansion(real_t p_value) {}
	virtual void params_set_pairing_expansion(real_t p_value) {}

	// octree specific
	virtual void set_balance(float p_balance) {}

	~SpatialPartitioningScene() {}
};

class SpatialPartitioningScene_Octree : public SpatialPartitioningScene {
	Octree_CL<RasterizerInstance, true> _octree;

public:
	virtual SpatialPartitionID create(RasterizerInstance *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t pairable_mask = 1);
	virtual void erase(SpatialPartitionID p_handle);
	virtual void move(SpatialPartitionID p_handle, const AABB &p_aabb);
	virtual void set_pairable(RasterizerInstance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask);
	virtual int cull_convex(const Vector<Plane> &p_convex, RasterizerInstance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF);
	virtual int cull_aabb(const AABB &p_aabb, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
	virtual int cull_segment(const Vector3 &p_from, const Vector3 &p_to, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
	virtual void set_pair_callback(PairCallback p_callback, void *p_userdata);
	virtual void set_unpair_callback(UnpairCallback p_callback, void *p_userdata);
	virtual void set_balance(float p_balance);
};

class SpatialPartitioningScene_BVH : public SpatialPartitioningScene {
	template <class T>
	class UserPairTestFunction {
	public:
		static bool user_pair_check(const T *p_a, const T *p_b) {
			// return false if no collision, decided by masks etc
			return true;
		}
	};

	template <class T>
	class UserCullTestFunction {
		// write this logic once for use in all routines
		// double check this as a possible source of bugs in future.
		static bool _cull_pairing_mask_test_hit(uint32_t p_maskA, uint32_t p_typeA, uint32_t p_maskB, uint32_t p_typeB) {
			// double check this as a possible source of bugs in future.
			bool A_match_B = p_maskA & p_typeB;

			if (!A_match_B) {
				bool B_match_A = p_maskB & p_typeA;
				if (!B_match_A) {
					return false;
				}
			}

			return true;
		}

	public:
		static bool user_cull_check(const T *p_a, const T *p_b) {
			DEV_ASSERT(p_a);
			DEV_ASSERT(p_b);

			uint32_t a_mask = p_a->bvh_pairable_mask;
			uint32_t a_type = p_a->bvh_pairable_type;
			uint32_t b_mask = p_b->bvh_pairable_mask;
			uint32_t b_type = p_b->bvh_pairable_type;

			if (!_cull_pairing_mask_test_hit(a_mask, a_type, b_mask, b_type)) {
				return false;
			}

			return true;
		}
	};

private:
	// Note that SpatialPartitionIDs are +1 based when stored in visual server, to enable 0 to indicate invalid ID.
	BVH_Manager<RasterizerInstance, 2, true, 256, UserPairTestFunction<RasterizerInstance>, UserCullTestFunction<RasterizerInstance>> _bvh;
	RasterizerInstance *_dummy_cull_object;

public:
	SpatialPartitioningScene_BVH();
	~SpatialPartitioningScene_BVH();
	virtual SpatialPartitionID create(RasterizerInstance *p_userdata, const AABB &p_aabb = AABB(), int p_subindex = 0, bool p_pairable = false, uint32_t p_pairable_type = 0, uint32_t p_pairable_mask = 1);
	virtual void erase(SpatialPartitionID p_handle);
	virtual void move(SpatialPartitionID p_handle, const AABB &p_aabb);
	void activate(SpatialPartitionID p_handle, const AABB &p_aabb);
	void deactivate(SpatialPartitionID p_handle);
	void force_collision_check(SpatialPartitionID p_handle);
	void update();
	void update_collisions();
	virtual void set_pairable(RasterizerInstance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask);
	virtual int cull_convex(const Vector<Plane> &p_convex, RasterizerInstance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF);
	virtual int cull_aabb(const AABB &p_aabb, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
	virtual int cull_segment(const Vector3 &p_from, const Vector3 &p_to, RasterizerInstance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
	virtual void set_pair_callback(PairCallback p_callback, void *p_userdata);
	virtual void set_unpair_callback(UnpairCallback p_callback, void *p_userdata);

	virtual void params_set_node_expansion(real_t p_value) { _bvh.params_set_node_expansion(p_value); }
	virtual void params_set_pairing_expansion(real_t p_value) { _bvh.params_set_pairing_expansion(p_value); }
};

struct RasterizerScenario : RID_Data {
	VS::ScenarioDebugMode debug;
	RID self;

	SpatialPartitioningScene *sps;
	PortalRenderer _portal_renderer;

	List<RasterizerInstance *> directional_lights;
	RID environment;
	RID fallback_environment;
	RID reflection_probe_shadow_atlas;
	RID reflection_atlas;

	SelfList<RasterizerInstance>::List instances;

	RasterizerScenario();
	~RasterizerScenario() { memdelete(sps); }
};

struct GeometryInstanceData {
	List<RasterizerInstance *> lighting;
	List<RasterizerInstance *> reflection_probes;

	bool lighting_dirty;
	bool can_cast_shadows;
	bool material_is_animated;
	bool reflection_dirty;

	GeometryInstanceData() {
		lighting_dirty = true;
		reflection_dirty = true;
		can_cast_shadows = true;
		material_is_animated = true;
	}
};

struct InstanceReflectionProbeData {
	struct PairInfo {
		List<RasterizerInstance *>::Element *L; //reflection iterator in geometry
		RasterizerInstance *geometry;
	};

	RasterizerInstance *owner;
	SelfList<InstanceReflectionProbeData> update_list;
	List<PairInfo> geometries;

	RID instance;

	int32_t previous_room_id_hint;
	int render_step;
	bool reflection_dirty;

	InstanceReflectionProbeData() :
			update_list(this) {
		reflection_dirty = true;
		render_step = -1;
		previous_room_id_hint = -1;
	}
};

struct InstanceLightData {
	struct PairInfo {
		List<RasterizerInstance *>::Element *L; //light iterator in geometry
		RasterizerInstance *geometry;
	};

	List<PairInfo> geometries;
	List<RasterizerInstance *>::Element *D; // directional light in scenario

	RID instance;
	uint64_t last_version;
	int32_t previous_room_id_hint;
	bool shadow_dirty;

	InstanceLightData() {
		shadow_dirty = true;
		D = nullptr;
		last_version = 0;
		previous_room_id_hint = -1;
	}
};

struct RasterizerInstance : RID_Data {
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
	bool baked_light : 1; //this flag is only to know if it actually did use baked light
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

	SelfList<RasterizerInstance> dependency_item;

	RasterizerInstance *lightmap_capture;
	RID lightmap;
	Vector<Color> lightmap_capture_data; //in a array (12 values) to avoid wasting space if unused. Alpha is unused, but needed to send to shader
	int lightmap_slice;
	Rect2 lightmap_uv_rect;
	RID self;
	//scenario stuff
	SpatialPartitionID spatial_partition_id;

	// rooms & portals
	OcclusionHandle occlusion_handle; // handle of instance in occlusion system (or 0)
	VisualServer::InstancePortalMode portal_mode;

	RasterizerScenario *scenario;
	SelfList<RasterizerInstance> scenario_item;

	//aabb stuff
	bool update_aabb;
	bool update_materials;

	SelfList<RasterizerInstance> update_item;

	AABB aabb;
	AABB transformed_aabb;
	AABB *custom_aabb; // <Zylann> would using aabb directly with a bool be better?
	float sorting_offset;
	bool use_aabb_center;
	float extra_margin;
	uint32_t object_id;

	float lod_begin;
	float lod_end;
	float lod_begin_hysteresis;
	float lod_end_hysteresis;
	RID lod_instance;

	// These are used for the user cull testing function
	// in the BVH, this is precached rather than recalculated each time.
	uint32_t bvh_pairable_mask;
	uint32_t bvh_pairable_type;

	uint64_t last_render_pass;
	uint64_t last_frame_pass;

	uint64_t version; // changes to this, and changes to base increase version

	union BaseData {
		GeometryInstanceData geometry;
		InstanceReflectionProbeData reflection;
		InstanceLightData light;
		BaseData() {}
		~BaseData() {}
	} data;

	void base_removed();

	void base_changed(bool p_aabb, bool p_materials);

	RasterizerInstance() :
			dependency_item(this),
			scenario_item(this),
			update_item(this) {
		base_type = VS::INSTANCE_NONE;
		cast_shadows = VS::SHADOW_CASTING_SETTING_ON;
		receive_shadows = true;
		visible = true;
		depth_layer = 0;
		layer_mask = 1;
		baked_light = false;
		redraw_if_visible = false;
		lightmap_capture = nullptr;
		lightmap_slice = -1;
		lightmap_uv_rect = Rect2(0, 0, 1, 1);
		on_interpolate_list = false;
		on_interpolate_transform_list = false;
		interpolated = true;
		interpolation_method = TransformInterpolator::INTERP_LERP;
		transform_checksum_curr = 0.0;
		transform_checksum_prev = 0.0;

		spatial_partition_id = 0;
		scenario = nullptr;

		update_aabb = false;
		update_materials = false;

		extra_margin = 0;

		object_id = 0;
		visible = true;

		occlusion_handle = 0;
		portal_mode = VisualServer::InstancePortalMode::INSTANCE_PORTAL_MODE_STATIC;

		lod_begin = 0;
		lod_end = 0;
		lod_begin_hysteresis = 0;
		lod_end_hysteresis = 0;

		bvh_pairable_mask = 0;
		bvh_pairable_type = 0;

		last_render_pass = 0;
		last_frame_pass = 0;
		version = 1;

		custom_aabb = nullptr;
		sorting_offset = 0.0f;
		use_aabb_center = true;
	}

	~RasterizerInstance() {
		if (custom_aabb) {
			memdelete(custom_aabb);
		}
	}
};

#endif
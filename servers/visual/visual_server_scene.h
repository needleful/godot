/*************************************************************************/
/*  visual_server_scene.h                                                */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef VISUAL_SERVER_SCENE_H
#define VISUAL_SERVER_SCENE_H

#include "servers/visual/rasterizer.h"

#include "core/math/bvh.h"
#include "core/math/geometry.h"
#include "core/math/octree.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"
#include "core/safe_refcount.h"
#include "core/self_list.h"
#include "drivers/gles_common/rasterizer_instance_base.h"
#include "portals/portal_renderer.h"
#include "servers/arvr/arvr_interface.h"

class VisualServerScene {
public:
	enum {

		MAX_INSTANCE_CULL = 65536,
		MAX_LIGHTS_CULLED = 4096,
		MAX_REFLECTION_PROBES_CULLED = 4096,
		MAX_ROOM_CULL = 32,
		MAX_EXTERIOR_PORTALS = 128,
	};

	uint64_t render_pass;
	static VisualServerScene *singleton;

	/* EVENT QUEUING */

	void tick();
	void pre_draw(bool p_will_draw);

	/* CAMERA API */

	struct Camera : public RID_Data {
		enum Type {
			PERSPECTIVE,
			ORTHOGONAL,
			FRUSTUM
		};
		Type type;
		float fov;
		float znear, zfar;
		float size;
		Vector2 offset;
		uint32_t visible_layers;
		RID env;

		// transform_prev is only used when using fixed timestep interpolation
		Transform transform;
		Transform transform_prev;

		bool interpolated : 1;
		bool on_interpolate_transform_list : 1;

		bool vaspect : 1;
		TransformInterpolator::Method interpolation_method : 3;

		int32_t previous_room_id_hint;

		Transform get_transform_interpolated() const;

		Camera() {
			visible_layers = 0xFFFFFFFF;
			fov = 70;
			type = PERSPECTIVE;
			znear = 0.05;
			zfar = 100;
			size = 1.0;
			offset = Vector2();
			vaspect = false;
			previous_room_id_hint = -1;
			interpolated = true;
			on_interpolate_transform_list = false;
			interpolation_method = TransformInterpolator::INTERP_LERP;
		}
	};

	mutable RID_Owner<Camera> camera_owner;

	virtual RID camera_create();
	virtual void camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	virtual void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	virtual void camera_set_frustum(RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	virtual void camera_set_transform(RID p_camera, const Transform &p_transform);
	virtual void camera_set_interpolated(RID p_camera, bool p_interpolated);
	virtual void camera_reset_physics_interpolation(RID p_camera);
	virtual void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	virtual void camera_set_environment(RID p_camera, RID p_env);
	virtual void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	mutable RID_Owner<RasterizerScenario> scenario_owner;

	static void *_instance_pair(void *p_self, SpatialPartitionID, RasterizerInstance *p_A, int, SpatialPartitionID, RasterizerInstance *p_B, int);
	static void _instance_unpair(void *p_self, SpatialPartitionID, RasterizerInstance *p_A, int, SpatialPartitionID, RasterizerInstance *p_B, int, void *);

	virtual RID scenario_create();

	virtual void scenario_set_debug(RID p_scenario, VS::ScenarioDebugMode p_debug_mode);
	virtual void scenario_set_environment(RID p_scenario, RID p_environment);
	virtual void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	virtual void scenario_set_reflection_atlas_size(RID p_scenario, int p_size, int p_subdiv);

	/* INSTANCING API */
	SelfList<RasterizerInstance>::List _instance_update_list;

	// fixed timestep interpolation
	virtual void set_physics_interpolation_enabled(bool p_enabled);

	struct InterpolationData {
		void notify_free_camera(RID p_rid, Camera &r_camera);
		void notify_free_instance(RID p_rid, RasterizerInstance &r_instance);
		LocalVector<RID> instance_interpolate_update_list;
		LocalVector<RID> instance_transform_update_lists[2];
		LocalVector<RID> *instance_transform_update_list_curr = &instance_transform_update_lists[0];
		LocalVector<RID> *instance_transform_update_list_prev = &instance_transform_update_lists[1];
		LocalVector<RID> instance_teleport_list;

		LocalVector<RID> camera_transform_update_lists[2];
		LocalVector<RID> *camera_transform_update_list_curr = &camera_transform_update_lists[0];
		LocalVector<RID> *camera_transform_update_list_prev = &camera_transform_update_lists[1];
		LocalVector<RID> camera_teleport_list;

		bool interpolation_enabled = false;
	} _interpolation_data;

	void _instance_queue_update(RasterizerInstance *p_instance, bool p_update_aabb, bool p_update_materials = false);

	int instance_cull_count;
	RasterizerInstance *instance_cull_result[MAX_INSTANCE_CULL];
	RasterizerInstance *instance_shadow_cull_result[MAX_INSTANCE_CULL]; //used for generating shadowmaps
	RasterizerInstance *light_cull_result[MAX_LIGHTS_CULLED];
	RID light_instance_cull_result[MAX_LIGHTS_CULLED];
	int light_cull_count;
	int directional_light_count;
	SelfList<InstanceReflectionProbeData>::List reflection_probe_render_list;
	RID reflection_probe_instance_cull_result[MAX_REFLECTION_PROBES_CULLED];
	int reflection_probe_cull_count;

	RID_Owner<RasterizerInstance> instance_owner;

	virtual RID instance_create();

	virtual void instance_set_base(RID p_instance, RID p_base);
	virtual void instance_set_scenario(RID p_instance, RID p_scenario);
	virtual void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	virtual void instance_set_pivot_data(RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	virtual void instance_set_transform(RID p_instance, const Transform &p_transform);
	virtual void instance_set_interpolated(RID p_instance, bool p_interpolated);
	virtual void instance_reset_physics_interpolation(RID p_instance);
	virtual void instance_attach_object_instance_id(RID p_instance, ObjectID p_id);
	virtual void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	virtual void instance_set_surface_material(RID p_instance, int p_surface, RID p_material);
	virtual void instance_set_visible(RID p_instance, bool p_visible);

	virtual void instance_set_custom_aabb(RID p_instance, AABB p_aabb);

	virtual void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	virtual void instance_set_exterior(RID p_instance, bool p_enabled);

	virtual void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);

	// Portals
	virtual void instance_set_portal_mode(RID p_instance, VisualServer::InstancePortalMode p_mode);
	bool _instance_get_transformed_aabb(RID p_instance, AABB &r_aabb);
	bool _instance_get_transformed_aabb_for_occlusion(VSInstance *p_instance, AABB &r_aabb) const {
		r_aabb = ((RasterizerInstance *)p_instance)->transformed_aabb;
		return ((RasterizerInstance *)p_instance)->portal_mode != VisualServer::INSTANCE_PORTAL_MODE_GLOBAL;
	}
	void *_instance_get_from_rid(RID p_instance);
	bool _instance_cull_check(VSInstance *p_instance, uint32_t p_cull_mask) const {
		uint32_t pairable_type = 1 << ((RasterizerInstance *)p_instance)->base_type;
		return pairable_type & p_cull_mask;
	}
	ObjectID _instance_get_object_ID(VSInstance *p_instance) const {
		if (p_instance) {
			return ((RasterizerInstance *)p_instance)->object_id;
		}
		return 0;
	}

private:
	void _instance_create_occlusion_rep(RasterizerInstance *p_instance);
	void _instance_destroy_occlusion_rep(RasterizerInstance *p_instance);

public:
	struct Ghost : RID_Data {
		// all interactions with actual ghosts are indirect, as the ghost is part of the scenario
		RasterizerScenario *scenario = nullptr;
		uint32_t object_id = 0;
		RGhostHandle rghost_handle = 0; // handle in occlusion system (or 0)
		AABB aabb;
		virtual ~Ghost() {
			if (scenario) {
				if (rghost_handle) {
					scenario->_portal_renderer.rghost_destroy(rghost_handle);
					rghost_handle = 0;
				}
				scenario = nullptr;
			}
		}
	};
	RID_Owner<Ghost> ghost_owner;

	virtual RID ghost_create();
	virtual void ghost_set_scenario(RID p_ghost, RID p_scenario, ObjectID p_id, const AABB &p_aabb);
	virtual void ghost_update(RID p_ghost, const AABB &p_aabb);

private:
	void _ghost_create_occlusion_rep(Ghost *p_ghost);
	void _ghost_destroy_occlusion_rep(Ghost *p_ghost);

public:
	/* PORTALS API */

	struct Portal : RID_Data {
		// all interactions with actual portals are indirect, as the portal is part of the scenario
		uint32_t scenario_portal_id = 0;
		RasterizerScenario *scenario = nullptr;
		virtual ~Portal() {
			if (scenario) {
				scenario->_portal_renderer.portal_destroy(scenario_portal_id);
				scenario = nullptr;
				scenario_portal_id = 0;
			}
		}
	};
	RID_Owner<Portal> portal_owner;

	virtual RID portal_create();
	virtual void portal_set_scenario(RID p_portal, RID p_scenario);
	virtual void portal_set_geometry(RID p_portal, const Vector<Vector3> &p_points, real_t p_margin);
	virtual void portal_link(RID p_portal, RID p_room_from, RID p_room_to, bool p_two_way);
	virtual void portal_set_active(RID p_portal, bool p_active);

	/* ROOMGROUPS API */

	struct RoomGroup : RID_Data {
		// all interactions with actual roomgroups are indirect, as the roomgroup is part of the scenario
		uint32_t scenario_roomgroup_id = 0;
		RasterizerScenario *scenario = nullptr;
		virtual ~RoomGroup() {
			if (scenario) {
				scenario->_portal_renderer.roomgroup_destroy(scenario_roomgroup_id);
				scenario = nullptr;
				scenario_roomgroup_id = 0;
			}
		}
	};
	RID_Owner<RoomGroup> roomgroup_owner;

	virtual RID roomgroup_create();
	virtual void roomgroup_prepare(RID p_roomgroup, ObjectID p_roomgroup_object_id);
	virtual void roomgroup_set_scenario(RID p_roomgroup, RID p_scenario);
	virtual void roomgroup_add_room(RID p_roomgroup, RID p_room);

	/* OCCLUDERS API */

	struct OccluderInstance : RID_Data {
		uint32_t scenario_occluder_id = 0;
		RasterizerScenario *scenario = nullptr;
		virtual ~OccluderInstance() {
			if (scenario) {
				scenario->_portal_renderer.occluder_instance_destroy(scenario_occluder_id);
				scenario = nullptr;
				scenario_occluder_id = 0;
			}
		}
	};
	RID_Owner<OccluderInstance> occluder_instance_owner;

	struct OccluderResource : RID_Data {
		uint32_t occluder_resource_id = 0;
		void destroy(PortalResources &r_portal_resources) {
			r_portal_resources.occluder_resource_destroy(occluder_resource_id);
			occluder_resource_id = 0;
		}
		virtual ~OccluderResource() {
			DEV_ASSERT(occluder_resource_id == 0);
		}
	};
	RID_Owner<OccluderResource> occluder_resource_owner;

	virtual RID occluder_instance_create();
	virtual void occluder_instance_set_scenario(RID p_occluder_instance, RID p_scenario);
	virtual void occluder_instance_link_resource(RID p_occluder_instance, RID p_occluder_resource);
	virtual void occluder_instance_set_transform(RID p_occluder_instance, const Transform &p_xform);
	virtual void occluder_instance_set_active(RID p_occluder_instance, bool p_active);

	virtual RID occluder_resource_create();
	virtual void occluder_resource_prepare(RID p_occluder_resource, VisualServer::OccluderType p_type);
	virtual void occluder_resource_spheres_update(RID p_occluder_resource, const Vector<Plane> &p_spheres);
	virtual void occluder_resource_mesh_update(RID p_occluder_resource, const Geometry::OccluderMeshData &p_mesh_data);
	virtual void set_use_occlusion_culling(bool p_enable);

	// editor only .. slow
	virtual Geometry::MeshData occlusion_debug_get_current_polys(RID p_scenario) const;
	const PortalResources &get_portal_resources() const {
		return _portal_resources;
	}
	PortalResources &get_portal_resources() {
		return _portal_resources;
	}

	/* ROOMS API */

	struct Room : RID_Data {
		// all interactions with actual rooms are indirect, as the room is part of the scenario
		uint32_t scenario_room_id = 0;
		RasterizerScenario *scenario = nullptr;
		virtual ~Room() {
			if (scenario) {
				scenario->_portal_renderer.room_destroy(scenario_room_id);
				scenario = nullptr;
				scenario_room_id = 0;
			}
		}
	};
	RID_Owner<Room> room_owner;

	virtual RID room_create();
	virtual void room_set_scenario(RID p_room, RID p_scenario);
	virtual void room_add_instance(RID p_room, RID p_instance, const AABB &p_aabb, const Vector<Vector3> &p_object_pts);
	virtual void room_add_ghost(RID p_room, ObjectID p_object_id, const AABB &p_aabb);
	virtual void room_set_bound(RID p_room, ObjectID p_room_object_id, const Vector<Plane> &p_convex, const AABB &p_aabb, const Vector<Vector3> &p_verts);
	virtual void room_prepare(RID p_room, int32_t p_priority);
	virtual void rooms_and_portals_clear(RID p_scenario);
	virtual void rooms_unload(RID p_scenario, String p_reason);
	virtual void rooms_finalize(RID p_scenario, bool p_generate_pvs, bool p_cull_using_pvs, bool p_use_secondary_pvs, bool p_use_signals, String p_pvs_filename, bool p_use_simple_pvs, bool p_log_pvs_generation);
	virtual void rooms_override_camera(RID p_scenario, bool p_override, const Vector3 &p_point, const Vector<Plane> *p_convex);
	virtual void rooms_set_active(RID p_scenario, bool p_active);
	virtual void rooms_set_params(RID p_scenario, int p_portal_depth_limit, real_t p_roaming_expansion_margin);
	virtual void rooms_set_debug_feature(RID p_scenario, VisualServer::RoomsDebugFeature p_feature, bool p_active);
	virtual void rooms_update_gameplay_monitor(RID p_scenario, const Vector<Vector3> &p_camera_positions);

	// don't use this in a game
	virtual bool rooms_is_loaded(RID p_scenario) const;

	virtual void callbacks_register(VisualServerCallbacks *p_callbacks);
	VisualServerCallbacks *get_callbacks() const {
		return _visual_server_callbacks;
	}

	// don't use these in a game!
	virtual Vector<ObjectID> instances_cull_aabb(const AABB &p_aabb, RID p_scenario = RID()) const;
	virtual Vector<ObjectID> instances_cull_ray(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario = RID()) const;
	virtual Vector<ObjectID> instances_cull_convex(const Vector<Plane> &p_convex, RID p_scenario = RID()) const;

	// internal (uses portals when available)
	int _cull_convex_from_point(RasterizerScenario *p_scenario, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Vector<Plane> &p_convex, RasterizerInstance **p_result_array, int p_result_max, int32_t &r_previous_room_id_hint, uint32_t p_mask = 0xFFFFFFFF);
	void _rooms_instance_update(RasterizerInstance *p_instance, const AABB &p_aabb);

	virtual void instance_geometry_set_flag(RID p_instance, VS::InstanceFlags p_flags, bool p_enabled);
	virtual void instance_geometry_set_cast_shadows_setting(RID p_instance, VS::ShadowCastingSetting p_shadow_casting_setting);
	virtual void instance_geometry_set_material_override(RID p_instance, RID p_material);
	virtual void instance_geometry_set_material_overlay(RID p_instance, RID p_material);

	virtual void instance_geometry_set_draw_range(RID p_instance, float p_min, float p_max, float p_min_margin, float p_max_margin);
	virtual void instance_geometry_set_as_instance_lod(RID p_instance, RID p_as_lod_of_instance);

	_FORCE_INLINE_ void _update_instance(RasterizerInstance *p_instance);
	_FORCE_INLINE_ void _update_instance_aabb(RasterizerInstance *p_instance);
	_FORCE_INLINE_ void _update_dirty_instance(RasterizerInstance *p_instance);

	_FORCE_INLINE_ bool _light_instance_update_shadow(RasterizerInstance *p_instance, const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, RID p_shadow_atlas, RasterizerScenario *p_scenario, uint32_t p_visible_layers = 0xFFFFFF);

	void _prepare_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, RID p_force_environment, uint32_t p_visible_layers, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int32_t &r_previous_room_id_hint);
	void _render_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, const int p_eye, bool p_cam_orthogonal, RID p_force_environment, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int p_reflection_probe_pass);
	void render_empty_scene(RID p_scenario, RID p_shadow_atlas);

	void render_camera(RID p_camera, RID p_scenario, Size2 p_viewport_size, RID p_shadow_atlas);
	void render_camera(Ref<ARVRInterface> &p_interface, ARVRInterface::Eyes p_eye, RID p_camera, RID p_scenario, Size2 p_viewport_size, RID p_shadow_atlas);
	void update_dirty_instances();

	// interpolation
	void update_interpolation_tick(bool p_process = true);
	void update_interpolation_frame(bool p_process = true);

	bool _render_reflection_probe_step(RasterizerInstance *p_instance, int p_step);
	void render_probes();

	bool free(RID p_rid);

private:
	bool _use_bvh;
	VisualServerCallbacks *_visual_server_callbacks;
	PortalResources _portal_resources;

public:
	VisualServerScene();
	virtual ~VisualServerScene();
};

#endif // VISUAL_SERVER_SCENE_H

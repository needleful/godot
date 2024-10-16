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

class VisualServerScene {
public:
	enum {

		MAX_INSTANCE_CULL = 65536,
		MAX_LIGHTS_CULLED = 4096,
		MAX_REFLECTION_PROBES_CULLED = 4096,
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

	RID camera_create();
	void camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	void camera_set_frustum(RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	void camera_set_transform(RID p_camera, const Transform &p_transform);
	void camera_set_interpolated(RID p_camera, bool p_interpolated);
	void camera_reset_physics_interpolation(RID p_camera);
	void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	void camera_set_environment(RID p_camera, RID p_env);
	void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	mutable RID_Owner<RasterizerScenario> scenario_owner;

	static void *_instance_pair(void *p_self, SpatialPartitionID, RasterizerInstance *p_A, int, SpatialPartitionID, RasterizerInstance *p_B, int);
	static void _instance_unpair(void *p_self, SpatialPartitionID, RasterizerInstance *p_A, int, SpatialPartitionID, RasterizerInstance *p_B, int, void *);

	RID scenario_create();

	void scenario_set_debug(RID p_scenario, VS::ScenarioDebugMode p_debug_mode);
	void scenario_set_environment(RID p_scenario, RID p_environment);
	void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	void scenario_set_reflection_atlas_size(RID p_scenario, int p_size, int p_subdiv);

	/* INSTANCING API */

	SelfList<RasterizerInstance>::List _instance_update_list;

	// fixed timestep interpolation
	void set_physics_interpolation_enabled(bool p_enabled);

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

	RID instance_create();

	void instance_set_base(RID p_instance, RID p_base);
	void instance_set_scenario(RID p_instance, RID p_scenario);
	void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	void instance_set_pivot_data(RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	void instance_set_transform(RID p_instance, const Transform &p_transform);
	void instance_set_interpolated(RID p_instance, bool p_interpolated);
	void instance_reset_physics_interpolation(RID p_instance);
	void instance_attach_object_instance_id(RID p_instance, ObjectID p_id);
	void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	void instance_set_surface_material(RID p_instance, int p_surface, RID p_material);
	void instance_set_visible(RID p_instance, bool p_visible);
	void instance_set_custom_aabb(RID p_instance, AABB p_aabb);

	void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	void instance_set_exterior(RID p_instance, bool p_enabled);

	void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);

	bool _instance_get_transformed_aabb(RID p_instance, AABB &r_aabb);
	void *_instance_get_from_rid(RID p_instance);

public:
	/* OCCLUDERS API */

	struct OccluderInstance : RID_Data {
		uint32_t scenario_occluder_id = 0;
		RasterizerScenario *scenario = nullptr;
		virtual ~OccluderInstance() {
			if (scenario) {
				scenario = nullptr;
				scenario_occluder_id = 0;
			}
		}
	};
	RID_Owner<OccluderInstance> occluder_instance_owner;

	struct OccluderResource : RID_Data {
		uint32_t occluder_resource_id = 0;
		virtual ~OccluderResource() {
			DEV_ASSERT(occluder_resource_id == 0);
		}
	};
	RID_Owner<OccluderResource> occluder_resource_owner;

	RID occluder_instance_create();
	void occluder_instance_set_scenario(RID p_occluder_instance, RID p_scenario);
	void occluder_instance_link_resource(RID p_occluder_instance, RID p_occluder_resource);
	void occluder_instance_set_transform(RID p_occluder_instance, const Transform &p_xform);
	void occluder_instance_set_active(RID p_occluder_instance, bool p_active);

	RID occluder_resource_create();
	void occluder_resource_prepare(RID p_occluder_resource, VisualServer::OccluderType p_type);
	void occluder_resource_spheres_update(RID p_occluder_resource, const Vector<Plane> &p_spheres);
	void occluder_resource_mesh_update(RID p_occluder_resource, const Geometry::OccluderMeshData &p_mesh_data);
	void set_use_occlusion_culling(bool p_enable);

	// editor only .. slow
	Geometry::MeshData occlusion_debug_get_current_polys(RID p_scenario) const;

	void callbacks_register(VisualServerCallbacks *p_callbacks);
	VisualServerCallbacks *get_callbacks() const {
		return _visual_server_callbacks;
	}

	// don't use these in a game!
	Vector<ObjectID> instances_cull_aabb(const AABB &p_aabb, RID p_scenario = RID()) const;
	Vector<ObjectID> instances_cull_ray(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario = RID()) const;
	Vector<ObjectID> instances_cull_convex(const Vector<Plane> &p_convex, RID p_scenario = RID()) const;

	// internal (uses portals when available)
	int _cull_convex_from_point(RasterizerScenario *p_scenario, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, const Vector<Plane> &p_convex, RasterizerInstance **p_result_array, int p_result_max, int32_t &r_previous_room_id_hint, uint32_t p_mask = 0xFFFFFFFF);
	void _rooms_instance_update(RasterizerInstance *p_instance, const AABB &p_aabb);

	void instance_geometry_set_flag(RID p_instance, VS::InstanceFlags p_flags, bool p_enabled);
	void instance_geometry_set_cast_shadows_setting(RID p_instance, VS::ShadowCastingSetting p_shadow_casting_setting);
	void instance_geometry_set_material_override(RID p_instance, RID p_material);
	void instance_geometry_set_material_overlay(RID p_instance, RID p_material);

	void instance_geometry_set_draw_range(RID p_instance, float p_min, float p_max, float p_min_margin, float p_max_margin);
	void instance_geometry_set_as_instance_lod(RID p_instance, RID p_as_lod_of_instance);

	_FORCE_INLINE_ void _update_instance(RasterizerInstance *p_instance);
	_FORCE_INLINE_ void _update_instance_aabb(RasterizerInstance *p_instance);
	_FORCE_INLINE_ void _update_dirty_instance(RasterizerInstance *p_instance);

	_FORCE_INLINE_ bool _light_instance_update_shadow(RasterizerInstance *p_instance, const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, RID p_shadow_atlas, RasterizerScenario *p_scenario, uint32_t p_visible_layers = 0xFFFFFF);

	void _prepare_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, RID p_force_environment, uint32_t p_visible_layers, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int32_t &r_previous_room_id_hint);
	void _render_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, const int p_eye, bool p_cam_orthogonal, RID p_force_environment, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int p_reflection_probe_pass);
	void render_empty_scene(RID p_scenario, RID p_shadow_atlas);

	void render_camera(RID p_camera, RID p_scenario, Size2 p_viewport_size, RID p_shadow_atlas);
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

public:
	VisualServerScene();
	~VisualServerScene();
};

#endif // VISUAL_SERVER_SCENE_H

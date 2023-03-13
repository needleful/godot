/*************************************************************************/
/*  visual_server.h                                                      */
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

#ifndef VISUAL_SERVER_H
#define VISUAL_SERVER_H

#include "core/command_queue_mt.h"
#include "core/image.h"
#include "core/math/bsp_tree.h"
#include "core/math/geometry.h"
#include "core/math/octree.h"
#include "core/math/transform_2d.h"
#include "core/object.h"
#include "core/os/thread.h"
#include "core/rid.h"
#include "core/safe_refcount.h"
#include "core/variant.h"

class VisualServerCallbacks;

class VisualServer : public Object {
	GDCLASS(VisualServer, Object);

	static VisualServer *singleton;

	int mm_policy;
	bool render_loop_enabled = true;
#ifdef DEBUG_ENABLED
	bool force_shader_fallbacks = false;
#endif

	void _camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	void _canvas_item_add_style_box(RID p_item, const Rect2 &p_rect, const Rect2 &p_source, RID p_texture, const Vector<float> &p_margins, const Color &p_modulate = Color(1, 1, 1));
	Array _get_array_from_surface(uint32_t p_format, PoolVector<uint8_t> p_vertex_data, int p_vertex_len, PoolVector<uint8_t> p_index_data, int p_index_len) const;

protected:
	RID _make_test_cube();
	void _free_internal_rids();
	RID test_texture;
	RID white_texture;
	RID test_material;

	Error _surface_set_data(Array p_arrays, uint32_t p_format, uint32_t *p_offsets, uint32_t *p_stride, PoolVector<uint8_t> &r_vertex_array, int p_vertex_array_len, PoolVector<uint8_t> &r_index_array, int p_index_array_len, AABB &r_aabb, Vector<AABB> &r_bone_aabb);

	static VisualServer *(*create_func)();
	static void _bind_methods();

public:
	static VisualServer *get_singleton();
	static VisualServer *create();
	static Vector2 norm_to_oct(const Vector3 v);
	static Vector2 tangent_to_oct(const Vector3 v, const float sign, const bool high_precision);
	static Vector3 oct_to_norm(const Vector2 v);
	static Vector3 oct_to_tangent(const Vector2 v, float *out_sign);

	enum {

		NO_INDEX_ARRAY = -1,
		ARRAY_WEIGHTS_SIZE = 4,
		CANVAS_ITEM_Z_MIN = -4096,
		CANVAS_ITEM_Z_MAX = 4096,
		MAX_GLOW_LEVELS = 7,

		MAX_CURSORS = 8,
	};

	/* TEXTURE API */

	enum TextureFlags : unsigned int { // unsigned to stop sanitizer complaining about bit operations on ints
		TEXTURE_FLAG_MIPMAPS = 1, /// Enable automatic mipmap generation - when available
		TEXTURE_FLAG_REPEAT = 2, /// Repeat texture (Tiling), otherwise Clamping
		TEXTURE_FLAG_FILTER = 4, /// Create texture with linear (or available) filter
		TEXTURE_FLAG_ANISOTROPIC_FILTER = 8,
		TEXTURE_FLAG_CONVERT_TO_LINEAR = 16,
		TEXTURE_FLAG_MIRRORED_REPEAT = 32, /// Repeat texture, with alternate sections mirrored
		TEXTURE_FLAG_USED_FOR_STREAMING = 2048,
		TEXTURE_FLAGS_DEFAULT = TEXTURE_FLAG_REPEAT | TEXTURE_FLAG_MIPMAPS | TEXTURE_FLAG_FILTER
	};

	enum TextureType {
		TEXTURE_TYPE_2D,
		TEXTURE_TYPE_EXTERNAL,
		TEXTURE_TYPE_CUBEMAP,
		TEXTURE_TYPE_2D_ARRAY,
		TEXTURE_TYPE_3D,
	};

	enum CubeMapSide {

		CUBEMAP_LEFT,
		CUBEMAP_RIGHT,
		CUBEMAP_BOTTOM,
		CUBEMAP_TOP,
		CUBEMAP_FRONT,
		CUBEMAP_BACK
	};

	RID texture_create();
	RID texture_create_from_image(const Ref<Image> &p_image, uint32_t p_flags = TEXTURE_FLAGS_DEFAULT); // helper
	void texture_allocate(RID p_texture,
			int p_width,
			int p_height,
			int p_depth_3d,
			Image::Format p_format,
			TextureType p_type,
			uint32_t p_flags = TEXTURE_FLAGS_DEFAULT);

	void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer = 0);
	void texture_set_data_partial(RID p_texture,
			const Ref<Image> &p_image,
			int src_x, int src_y,
			int src_w, int src_h,
			int dst_x, int dst_y,
			int p_dst_mip,
			int p_layer = 0);

	Ref<Image> texture_get_data(RID p_texture, int p_layer = 0) const;
	void texture_set_flags(RID p_texture, uint32_t p_flags);
	uint32_t texture_get_flags(RID p_texture) const;
	Image::Format texture_get_format(RID p_texture) const;
	TextureType texture_get_type(RID p_texture) const;
	uint32_t texture_get_texid(RID p_texture) const;
	uint32_t texture_get_width(RID p_texture) const;
	uint32_t texture_get_height(RID p_texture) const;
	uint32_t texture_get_depth(RID p_texture) const;
	void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d);
	void texture_bind(RID p_texture, uint32_t p_texture_no);

	void texture_set_path(RID p_texture, const String &p_path);
	String texture_get_path(RID p_texture) const;

	void texture_set_shrink_all_x2_on_set_data(bool p_enable);

	typedef void (*TextureDetectCallback)(void *);

	void texture_set_detect_3d_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata);
	void texture_set_detect_srgb_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata);
	void texture_set_detect_normal_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata);

	struct TextureInfo {
		RID texture;
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		Image::Format format;
		int bytes;
		String path;
	};

	void texture_debug_usage(List<TextureInfo> *r_info);
	Array _texture_debug_usage_bind();

	void textures_keep_original(bool p_enable);

	void texture_set_proxy(RID p_proxy, RID p_base);
	void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	/* SKY API */

	RID sky_create();
	void sky_set_texture(RID p_sky, RID p_cube_map, int p_radiance_size);

	/* SHADER API */

	enum ShaderMode {

		SHADER_SPATIAL,
		SHADER_CANVAS_ITEM,
		SHADER_PARTICLES,
		SHADER_MAX
	};

	RID shader_create();

	void shader_set_code(RID p_shader, const String &p_code);
	String shader_get_code(RID p_shader) const;
	void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const;
	Array _shader_get_param_list_bind(RID p_shader) const;

	void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture);
	RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const;

	void shader_add_custom_define(RID p_shader, const String &p_define);
	void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const;
	void shader_remove_custom_define(RID p_shader, const String &p_define);

	void set_shader_async_hidden_forbidden(bool p_forbidden);

	/* COMMON MATERIAL API */

	enum {
		MATERIAL_RENDER_PRIORITY_MIN = -128,
		MATERIAL_RENDER_PRIORITY_MAX = 127,

	};
	RID material_create();

	void material_set_shader(RID p_shader_material, RID p_shader);
	RID material_get_shader(RID p_shader_material) const;

	void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value);
	Variant material_get_param(RID p_material, const StringName &p_param) const;
	Variant material_get_param_default(RID p_material, const StringName &p_param) const;

	void material_set_render_priority(RID p_material, int priority);

	void material_set_line_width(RID p_material, float p_width);
	void material_set_next_pass(RID p_material, RID p_next_material);

	/* MESH API */

	enum ArrayType {

		ARRAY_VERTEX = 0,
		ARRAY_NORMAL = 1,
		ARRAY_TANGENT = 2,
		ARRAY_COLOR = 3,
		ARRAY_TEX_UV = 4,
		ARRAY_TEX_UV2 = 5,
		ARRAY_BONES = 6,
		ARRAY_WEIGHTS = 7,
		ARRAY_INDEX = 8,
		ARRAY_MAX = 9
	};

	enum ArrayFormat {
		/* ARRAY FORMAT FLAGS */
		ARRAY_FORMAT_VERTEX = 1 << ARRAY_VERTEX, // mandatory
		ARRAY_FORMAT_NORMAL = 1 << ARRAY_NORMAL,
		ARRAY_FORMAT_TANGENT = 1 << ARRAY_TANGENT,
		ARRAY_FORMAT_COLOR = 1 << ARRAY_COLOR,
		ARRAY_FORMAT_TEX_UV = 1 << ARRAY_TEX_UV,
		ARRAY_FORMAT_TEX_UV2 = 1 << ARRAY_TEX_UV2,
		ARRAY_FORMAT_BONES = 1 << ARRAY_BONES,
		ARRAY_FORMAT_WEIGHTS = 1 << ARRAY_WEIGHTS,
		ARRAY_FORMAT_INDEX = 1 << ARRAY_INDEX,

		ARRAY_COMPRESS_BASE = (ARRAY_INDEX + 1),
		ARRAY_COMPRESS_VERTEX = 1 << (ARRAY_VERTEX + ARRAY_COMPRESS_BASE), // mandatory
		ARRAY_COMPRESS_NORMAL = 1 << (ARRAY_NORMAL + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TANGENT = 1 << (ARRAY_TANGENT + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_COLOR = 1 << (ARRAY_COLOR + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TEX_UV = 1 << (ARRAY_TEX_UV + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TEX_UV2 = 1 << (ARRAY_TEX_UV2 + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_BONES = 1 << (ARRAY_BONES + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_WEIGHTS = 1 << (ARRAY_WEIGHTS + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_INDEX = 1 << (ARRAY_INDEX + ARRAY_COMPRESS_BASE),

		ARRAY_FLAG_USE_2D_VERTICES = ARRAY_COMPRESS_INDEX << 1,
		ARRAY_FLAG_USE_16_BIT_BONES = ARRAY_COMPRESS_INDEX << 2,
		ARRAY_FLAG_USE_DYNAMIC_UPDATE = ARRAY_COMPRESS_INDEX << 3,
		ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION = ARRAY_COMPRESS_INDEX << 4,

		ARRAY_COMPRESS_DEFAULT = ARRAY_COMPRESS_NORMAL | ARRAY_COMPRESS_TANGENT | ARRAY_COMPRESS_COLOR | ARRAY_COMPRESS_TEX_UV | ARRAY_COMPRESS_TEX_UV2 | ARRAY_COMPRESS_WEIGHTS | ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION

	};

	enum PrimitiveType {
		PRIMITIVE_POINTS = 0,
		PRIMITIVE_LINES = 1,
		PRIMITIVE_LINE_STRIP = 2,
		PRIMITIVE_LINE_LOOP = 3,
		PRIMITIVE_TRIANGLES = 4,
		PRIMITIVE_TRIANGLE_STRIP = 5,
		PRIMITIVE_TRIANGLE_FAN = 6,
		PRIMITIVE_MAX = 7,
	};

	RID mesh_create();

	uint32_t mesh_surface_get_format_offset(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const;
	uint32_t mesh_surface_get_format_stride(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const;
	/// Returns stride
	void mesh_surface_make_offsets_from_format(uint32_t p_format, int p_vertex_len, int p_index_len, uint32_t *r_offsets, uint32_t *r_strides) const;
	void mesh_add_surface_from_arrays(RID p_mesh, PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes = Array(), uint32_t p_compress_format = ARRAY_COMPRESS_DEFAULT);
	void mesh_add_surface(RID p_mesh, uint32_t p_format, PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>());

	void mesh_set_blend_shape_count(RID p_mesh, int p_amount);
	int mesh_get_blend_shape_count(RID p_mesh) const;

	enum BlendShapeMode {
		BLEND_SHAPE_MODE_NORMALIZED,
		BLEND_SHAPE_MODE_RELATIVE,
	};

	void mesh_set_blend_shape_mode(RID p_mesh, BlendShapeMode p_mode);
	BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const;

	void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data);

	void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	RID mesh_surface_get_material(RID p_mesh, int p_surface) const;

	int mesh_surface_get_array_len(RID p_mesh, int p_surface) const;
	int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const;

	PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const;
	PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const;

	Array mesh_surface_get_arrays(RID p_mesh, int p_surface) const;
	Array mesh_surface_get_blend_shape_arrays(RID p_mesh, int p_surface) const;

	uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const;
	PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const;

	AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const;
	Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const;
	Vector<AABB> mesh_surface_get_skeleton_aabb(RID p_mesh, int p_surface) const;
	Array _mesh_surface_get_skeleton_aabb_bind(RID p_mesh, int p_surface) const;

	void mesh_remove_surface(RID p_mesh, int p_index);
	int mesh_get_surface_count(RID p_mesh) const;

	void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb);
	AABB mesh_get_custom_aabb(RID p_mesh) const;

	void mesh_clear(RID p_mesh);

	/* MULTIMESH API */

	RID multimesh_create();

	enum MultimeshTransformFormat {
		MULTIMESH_TRANSFORM_2D,
		MULTIMESH_TRANSFORM_3D,
	};

	enum MultimeshColorFormat {
		MULTIMESH_COLOR_NONE,
		MULTIMESH_COLOR_8BIT,
		MULTIMESH_COLOR_FLOAT,
		MULTIMESH_COLOR_MAX,
	};

	enum MultimeshCustomDataFormat {
		MULTIMESH_CUSTOM_DATA_NONE,
		MULTIMESH_CUSTOM_DATA_8BIT,
		MULTIMESH_CUSTOM_DATA_FLOAT,
		MULTIMESH_CUSTOM_DATA_MAX,
	};

	enum MultimeshPhysicsInterpolationQuality {
		MULTIMESH_INTERP_QUALITY_FAST,
		MULTIMESH_INTERP_QUALITY_HIGH,
	};

	void multimesh_allocate(RID p_multimesh, int p_instances, MultimeshTransformFormat p_transform_format, MultimeshColorFormat p_color_format, MultimeshCustomDataFormat p_data_format = MULTIMESH_CUSTOM_DATA_NONE);
	int multimesh_get_instance_count(RID p_multimesh) const;

	void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	void multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform);
	void multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform);
	void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color);
	void multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color);

	RID multimesh_get_mesh(RID p_multimesh) const;
	AABB multimesh_get_aabb(RID p_multimesh) const;

	Transform multimesh_instance_get_transform(RID p_multimesh, int p_index) const;
	Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const;
	Color multimesh_instance_get_color(RID p_multimesh, int p_index) const;
	Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const;

	void multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array);

	// Interpolation
	void multimesh_set_as_bulk_array_interpolated(RID p_multimesh, const PoolVector<float> &p_array, const PoolVector<float> &p_array_prev);
	void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
	void multimesh_set_physics_interpolation_quality(RID p_multimesh, MultimeshPhysicsInterpolationQuality p_quality);
	void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);

	void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	int multimesh_get_visible_instances(RID p_multimesh) const;

	/* IMMEDIATE API */

	RID immediate_create();
	void immediate_begin(RID p_immediate, PrimitiveType p_rimitive, RID p_texture = RID());
	void immediate_vertex(RID p_immediate, const Vector3 &p_vertex);
	void immediate_vertex_2d(RID p_immediate, const Vector2 &p_vertex);
	void immediate_normal(RID p_immediate, const Vector3 &p_normal);
	void immediate_tangent(RID p_immediate, const Plane &p_tangent);
	void immediate_color(RID p_immediate, const Color &p_color);
	void immediate_uv(RID p_immediate, const Vector2 &tex_uv);
	void immediate_uv2(RID p_immediate, const Vector2 &tex_uv);
	void immediate_end(RID p_immediate);
	void immediate_clear(RID p_immediate);
	void immediate_set_material(RID p_immediate, RID p_material);
	RID immediate_get_material(RID p_immediate) const;

	/* SKELETON API */

	RID skeleton_create();
	void skeleton_allocate(RID p_skeleton, int p_bones, bool p_2d_skeleton = false);
	int skeleton_get_bone_count(RID p_skeleton) const;
	void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform &p_transform);
	Transform skeleton_bone_get_transform(RID p_skeleton, int p_bone) const;
	void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D &p_transform);
	Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone) const;
	void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D &p_base_transform);

	/* Light API */

	enum LightType {
		LIGHT_DIRECTIONAL,
		LIGHT_OMNI,
		LIGHT_SPOT
	};

	enum LightParam {

		LIGHT_PARAM_ENERGY,
		LIGHT_PARAM_INDIRECT_ENERGY,
		LIGHT_PARAM_SIZE,
		LIGHT_PARAM_SPECULAR,
		LIGHT_PARAM_RANGE,
		LIGHT_PARAM_ATTENUATION,
		LIGHT_PARAM_SPOT_ANGLE,
		LIGHT_PARAM_SPOT_ATTENUATION,
		LIGHT_PARAM_CONTACT_SHADOW_SIZE,
		LIGHT_PARAM_SHADOW_MAX_DISTANCE,
		LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
		LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
		LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
		LIGHT_PARAM_SHADOW_NORMAL_BIAS,
		LIGHT_PARAM_SHADOW_BIAS,
		LIGHT_PARAM_SHADOW_BIAS_SPLIT_SCALE,
		LIGHT_PARAM_MAX
	};

	RID directional_light_create();
	RID omni_light_create();
	RID spot_light_create();

	void light_set_color(RID p_light, const Color &p_color);
	void light_set_param(RID p_light, LightParam p_param, float p_value);
	void light_set_shadow(RID p_light, bool p_enabled);
	void light_set_shadow_color(RID p_light, const Color &p_color);
	void light_set_projector(RID p_light, RID p_texture);
	void light_set_negative(RID p_light, bool p_enable);
	void light_set_cull_mask(RID p_light, uint32_t p_mask);
	void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled);

	// omni light
	enum LightOmniShadowMode {
		LIGHT_OMNI_SHADOW_DUAL_PARABOLOID,
		LIGHT_OMNI_SHADOW_CUBE,
	};

	void light_omni_set_shadow_mode(RID p_light, LightOmniShadowMode p_mode);

	// omni light
	enum LightOmniShadowDetail {
		LIGHT_OMNI_SHADOW_DETAIL_VERTICAL,
		LIGHT_OMNI_SHADOW_DETAIL_HORIZONTAL
	};

	void light_omni_set_shadow_detail(RID p_light, LightOmniShadowDetail p_detail);

	// directional light
	enum LightDirectionalShadowMode {
		LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL,
		LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
		LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS
	};

	void light_directional_set_shadow_mode(RID p_light, LightDirectionalShadowMode p_mode);
	void light_directional_set_blend_splits(RID p_light, bool p_enable);

	enum LightDirectionalShadowDepthRangeMode {
		LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_STABLE,
		LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_OPTIMIZED,

	};

	void light_directional_set_shadow_depth_range_mode(RID p_light, LightDirectionalShadowDepthRangeMode p_range_mode);

	/* PROBE API */

	RID reflection_probe_create();

	enum ReflectionProbeUpdateMode {
		REFLECTION_PROBE_UPDATE_ONCE,
		REFLECTION_PROBE_UPDATE_ALWAYS,
	};

	void reflection_probe_set_update_mode(RID p_probe, ReflectionProbeUpdateMode p_mode);
	void reflection_probe_set_intensity(RID p_probe, float p_intensity);
	void reflection_probe_set_interior_ambient(RID p_probe, const Color &p_color);
	void reflection_probe_set_interior_ambient_energy(RID p_probe, float p_energy);
	void reflection_probe_set_interior_ambient_probe_contribution(RID p_probe, float p_contrib);
	void reflection_probe_set_max_distance(RID p_probe, float p_distance);
	void reflection_probe_set_extents(RID p_probe, const Vector3 &p_extents);
	void reflection_probe_set_origin_offset(RID p_probe, const Vector3 &p_offset);
	void reflection_probe_set_as_interior(RID p_probe, bool p_enable);
	void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable);
	void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable);
	void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers);
	void reflection_probe_set_resolution(RID p_probe, int p_resolution);

	/* PARTICLES API */

	RID particles_create();

	void particles_set_emitting(RID p_particles, bool p_emitting);
	bool particles_get_emitting(RID p_particles);
	void particles_set_amount(RID p_particles, int p_amount);
	void particles_set_lifetime(RID p_particles, float p_lifetime);
	void particles_set_one_shot(RID p_particles, bool p_one_shot);
	void particles_set_pre_process_time(RID p_particles, float p_time);
	void particles_set_explosiveness_ratio(RID p_particles, float p_ratio);
	void particles_set_randomness_ratio(RID p_particles, float p_ratio);
	void particles_set_custom_aabb(RID p_particles, const AABB &p_aabb);
	void particles_set_speed_scale(RID p_particles, float p_scale);
	void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	void particles_set_process_material(RID p_particles, RID p_material);
	void particles_set_fixed_fps(RID p_particles, int p_fps);
	void particles_set_fractional_delta(RID p_particles, bool p_enable);
	bool particles_is_inactive(RID p_particles);
	void particles_request_process(RID p_particles);
	void particles_restart(RID p_particles);

	enum ParticlesDrawOrder {
		PARTICLES_DRAW_ORDER_INDEX,
		PARTICLES_DRAW_ORDER_LIFETIME,
		PARTICLES_DRAW_ORDER_VIEW_DEPTH,
	};

	void particles_set_draw_order(RID p_particles, ParticlesDrawOrder p_order);

	void particles_set_draw_passes(RID p_particles, int p_count);
	void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	AABB particles_get_current_aabb(RID p_particles);

	void particles_set_emission_transform(RID p_particles, const Transform &p_transform); //this is only used for 2D, in 3D it's automatic

	/* CAMERA API */

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

	/*
	enum ParticlesCollisionMode {
		PARTICLES_COLLISION_NONE,
		PARTICLES_COLLISION_TEXTURE,
		PARTICLES_COLLISION_CUBEMAP,
	};

	void particles_set_collision(RID p_particles,ParticlesCollisionMode p_mode,const Transform&, p_xform,const RID p_depth_tex,const RID p_normal_tex)=0;
*/
	/* VIEWPORT TARGET API */

	RID viewport_create();

	void viewport_set_use_arvr(RID p_viewport, bool p_use_arvr);
	void viewport_set_size(RID p_viewport, int p_width, int p_height);
	void viewport_set_active(RID p_viewport, bool p_active);
	void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);

	void viewport_attach_to_screen(RID p_viewport, const Rect2 &p_rect = Rect2(), int p_screen = 0);
	void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);
	void viewport_detach(RID p_viewport);

	enum ViewportUpdateMode {
		VIEWPORT_UPDATE_DISABLED,
		VIEWPORT_UPDATE_ONCE, //then goes to disabled, must be manually updated
		VIEWPORT_UPDATE_WHEN_VISIBLE, // default
		VIEWPORT_UPDATE_ALWAYS
	};

	void viewport_set_update_mode(RID p_viewport, ViewportUpdateMode p_mode);
	void viewport_set_vflip(RID p_viewport, bool p_enable);

	enum ViewportClearMode {

		VIEWPORT_CLEAR_ALWAYS,
		VIEWPORT_CLEAR_NEVER,
		VIEWPORT_CLEAR_ONLY_NEXT_FRAME
	};

	void viewport_set_clear_mode(RID p_viewport, ViewportClearMode p_clear_mode);

	RID viewport_get_texture(RID p_viewport) const;

	void viewport_set_hide_scenario(RID p_viewport, bool p_hide);
	void viewport_set_hide_canvas(RID p_viewport, bool p_hide);
	void viewport_set_disable_environment(RID p_viewport, bool p_disable);
	void viewport_set_disable_3d(RID p_viewport, bool p_disable);
	void viewport_set_keep_3d_linear(RID p_viewport, bool p_disable);

	void viewport_attach_camera(RID p_viewport, RID p_camera);
	void viewport_set_scenario(RID p_viewport, RID p_scenario);
	void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	void viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D &p_offset);
	void viewport_set_transparent_background(RID p_viewport, bool p_enabled);

	void viewport_set_global_canvas_transform(RID p_viewport, const Transform2D &p_transform);
	void viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	void viewport_set_shadow_atlas_size(RID p_viewport, int p_size);
	void viewport_set_shadow_atlas_quadrant_subdivision(RID p_viewport, int p_quadrant, int p_subdiv);

	enum ViewportMSAA {
		VIEWPORT_MSAA_DISABLED,
		VIEWPORT_MSAA_2X,
		VIEWPORT_MSAA_4X,
		VIEWPORT_MSAA_8X,
		VIEWPORT_MSAA_16X,
		VIEWPORT_MSAA_EXT_2X,
		VIEWPORT_MSAA_EXT_4X,
	};

	void viewport_set_msaa(RID p_viewport, ViewportMSAA p_msaa);
	void viewport_set_use_fxaa(RID p_viewport, bool p_fxaa);
	void viewport_set_use_debanding(RID p_viewport, bool p_debanding);
	void viewport_set_sharpen_intensity(RID p_viewport, float p_intensity);

	enum ViewportUsage {
		VIEWPORT_USAGE_2D,
		VIEWPORT_USAGE_2D_NO_SAMPLING,
		VIEWPORT_USAGE_3D,
		VIEWPORT_USAGE_3D_NO_EFFECTS,
	};

	void viewport_set_hdr(RID p_viewport, bool p_enabled);
	void viewport_set_use_32_bpc_depth(RID p_viewport, bool p_enabled);
	void viewport_set_usage(RID p_viewport, ViewportUsage p_usage);

	enum ViewportRenderInfo {

		VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME,
		VIEWPORT_RENDER_INFO_VERTICES_IN_FRAME,
		VIEWPORT_RENDER_INFO_MATERIAL_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_SHADER_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_SURFACE_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME,
		VIEWPORT_RENDER_INFO_2D_ITEMS_IN_FRAME,
		VIEWPORT_RENDER_INFO_2D_DRAW_CALLS_IN_FRAME,
		VIEWPORT_RENDER_INFO_MAX
	};

	int viewport_get_render_info(RID p_viewport, ViewportRenderInfo p_info);

	enum ViewportDebugDraw {
		VIEWPORT_DEBUG_DRAW_DISABLED,
		VIEWPORT_DEBUG_DRAW_UNSHADED,
		VIEWPORT_DEBUG_DRAW_OVERDRAW,
		VIEWPORT_DEBUG_DRAW_WIREFRAME,
	};

	void viewport_set_debug_draw(RID p_viewport, ViewportDebugDraw p_draw);

	/* ENVIRONMENT API */

	RID environment_create();

	enum EnvironmentBG {

		ENV_BG_CLEAR_COLOR,
		ENV_BG_COLOR,
		ENV_BG_SKY,
		ENV_BG_COLOR_SKY,
		ENV_BG_CANVAS,
		ENV_BG_KEEP,
		ENV_BG_CAMERA_FEED,
		ENV_BG_MAX
	};

	void environment_set_background(RID p_env, EnvironmentBG p_bg);
	void environment_set_sky(RID p_env, RID p_sky);
	void environment_set_sky_custom_fov(RID p_env, float p_scale);
	void environment_set_sky_orientation(RID p_env, const Basis &p_orientation);
	void environment_set_bg_color(RID p_env, const Color &p_color);
	void environment_set_bg_energy(RID p_env, float p_energy);
	void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	void environment_set_ambient_light(RID p_env, const Color &p_color, float p_energy = 1.0, float p_sky_contribution = 0.0);
	void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);

	//set default SSAO options
	//set default SSR options
	//set default SSSSS options

	enum EnvironmentDOFBlurQuality {
		ENV_DOF_BLUR_QUALITY_LOW,
		ENV_DOF_BLUR_QUALITY_MEDIUM,
		ENV_DOF_BLUR_QUALITY_HIGH,
	};

	void environment_set_dof_blur_near(RID p_env, bool p_enable, float p_distance, float p_transition, float p_far_amount, EnvironmentDOFBlurQuality p_quality);
	void environment_set_dof_blur_far(RID p_env, bool p_enable, float p_distance, float p_transition, float p_far_amount, EnvironmentDOFBlurQuality p_quality);

	enum EnvironmentGlowBlendMode {
		GLOW_BLEND_MODE_ADDITIVE,
		GLOW_BLEND_MODE_SCREEN,
		GLOW_BLEND_MODE_SOFTLIGHT,
		GLOW_BLEND_MODE_REPLACE,
	};
	void environment_set_glow(RID p_env, bool p_enable, int p_level_flags, float p_intensity, float p_strength, float p_bloom_threshold, EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold, float p_hdr_bleed_scale, float p_hdr_luminance_cap, bool p_bicubic_upscale, bool p_high_quality);

	enum EnvironmentToneMapper {
		ENV_TONE_MAPPER_LINEAR,
		ENV_TONE_MAPPER_REINHARD,
		ENV_TONE_MAPPER_FILMIC,
		ENV_TONE_MAPPER_ACES,
		ENV_TONE_MAPPER_ACES_FITTED
	};

	void environment_set_tonemap(RID p_env, EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white, bool p_auto_exposure, float p_min_luminance, float p_max_luminance, float p_auto_exp_speed, float p_auto_exp_grey);
	void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast, float p_saturation, RID p_ramp);

	void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_in, float p_fade_out, float p_depth_tolerance, bool p_roughness);

	enum EnvironmentSSAOQuality {
		ENV_SSAO_QUALITY_LOW,
		ENV_SSAO_QUALITY_MEDIUM,
		ENV_SSAO_QUALITY_HIGH,
	};

	enum EnvironmentSSAOBlur {
		ENV_SSAO_BLUR_DISABLED,
		ENV_SSAO_BLUR_1x1,
		ENV_SSAO_BLUR_2x2,
		ENV_SSAO_BLUR_3x3,
	};

	void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_radius2, float p_intensity2, float p_bias, float p_light_affect, float p_ao_channel_affect, const Color &p_color, EnvironmentSSAOQuality p_quality, EnvironmentSSAOBlur p_blur, float p_bilateral_sharpness);

	void environment_set_fog(RID p_env, bool p_enable, const Color &p_color, const Color &p_sun_color, float p_sun_amount);
	void environment_set_fog_depth(RID p_env, bool p_enable, float p_depth_begin, float p_depth_end, float p_depth_curve, bool p_transmit, float p_transmit_curve);
	void environment_set_fog_height(RID p_env, bool p_enable, float p_min_height, float p_max_height, float p_height_curve);

	/* INTERPOLATION API */

	void set_physics_interpolation_enabled(bool p_enabled);

	/* SCENARIO API */

	RID scenario_create();

	enum ScenarioDebugMode {
		SCENARIO_DEBUG_DISABLED,
		SCENARIO_DEBUG_WIREFRAME,
		SCENARIO_DEBUG_OVERDRAW,
		SCENARIO_DEBUG_SHADELESS,

	};

	void scenario_set_debug(RID p_scenario, ScenarioDebugMode p_debug_mode);
	void scenario_set_environment(RID p_scenario, RID p_environment);
	void scenario_set_reflection_atlas_size(RID p_scenario, int p_size, int p_subdiv);
	void scenario_set_fallback_environment(RID p_scenario, RID p_environment);

	/* INSTANCING API */

	enum InstanceType {

		INSTANCE_NONE,
		INSTANCE_MESH,
		INSTANCE_MULTIMESH,
		INSTANCE_IMMEDIATE,
		INSTANCE_PARTICLES,
		INSTANCE_LIGHT,
		INSTANCE_REFLECTION_PROBE,
		INSTANCE_MAX,

		INSTANCE_GEOMETRY_MASK = (1 << INSTANCE_MESH) | (1 << INSTANCE_MULTIMESH) | (1 << INSTANCE_IMMEDIATE) | (1 << INSTANCE_PARTICLES)
	};

	RID instance_create2(RID p_base, RID p_scenario);

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

	void instance_set_custom_aabb(RID p_instance, AABB aabb);

	void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	void instance_set_exterior(RID p_instance, bool p_enabled);

	void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);

	/* PORTALS API */

	enum InstancePortalMode {
		INSTANCE_PORTAL_MODE_STATIC, // not moving within a room
		INSTANCE_PORTAL_MODE_DYNAMIC, //  moving within room
		INSTANCE_PORTAL_MODE_ROAMING, // moving between rooms
		INSTANCE_PORTAL_MODE_GLOBAL, // frustum culled only
		INSTANCE_PORTAL_MODE_IGNORE, // don't show at all - e.g. manual bounds, hidden portals
	};

	void instance_set_portal_mode(RID p_instance, InstancePortalMode p_mode);

	RID ghost_create();
	void ghost_set_scenario(RID p_ghost, RID p_scenario, ObjectID p_id, const AABB &p_aabb);
	void ghost_update(RID p_ghost, const AABB &p_aabb);

	RID portal_create();
	void portal_set_scenario(RID p_portal, RID p_scenario);
	void portal_set_geometry(RID p_portal, const Vector<Vector3> &p_points, real_t p_margin);
	void portal_link(RID p_portal, RID p_room_from, RID p_room_to, bool p_two_way);
	void portal_set_active(RID p_portal, bool p_active);

	/* ROOMGROUPS API */

	RID roomgroup_create();
	void roomgroup_prepare(RID p_roomgroup, ObjectID p_roomgroup_object_id);
	void roomgroup_set_scenario(RID p_roomgroup, RID p_scenario);
	void roomgroup_add_room(RID p_roomgroup, RID p_room);

	/* OCCLUDERS API */

	enum OccluderType {
		OCCLUDER_TYPE_UNDEFINED,
		OCCLUDER_TYPE_SPHERE,
		OCCLUDER_TYPE_MESH,
		OCCLUDER_TYPE_NUM_TYPES,
	};

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
	Geometry::MeshData occlusion_debug_get_current_polys(RID p_scenario) const;

	/* ROOMS API */

	enum RoomsDebugFeature {
		ROOMS_DEBUG_SPRAWL,
	};

	RID room_create();
	void room_set_scenario(RID p_room, RID p_scenario);
	void room_add_instance(RID p_room, RID p_instance, const AABB &p_aabb, const Vector<Vector3> &p_object_pts);
	void room_add_ghost(RID p_room, ObjectID p_object_id, const AABB &p_aabb);
	void room_set_bound(RID p_room, ObjectID p_room_object_id, const Vector<Plane> &p_convex, const AABB &p_aabb, const Vector<Vector3> &p_verts);
	void room_prepare(RID p_room, int32_t p_priority);
	void rooms_and_portals_clear(RID p_scenario);
	void rooms_unload(RID p_scenario, String p_reason);
	void rooms_finalize(RID p_scenario, bool p_generate_pvs, bool p_cull_using_pvs, bool p_use_secondary_pvs, bool p_use_signals, String p_pvs_filename, bool p_use_simple_pvs, bool p_log_pvs_generation);
	void rooms_override_camera(RID p_scenario, bool p_override, const Vector3 &p_point, const Vector<Plane> *p_convex);
	void rooms_set_active(RID p_scenario, bool p_active);
	void rooms_set_params(RID p_scenario, int p_portal_depth_limit, real_t p_roaming_expansion_margin);
	void rooms_set_debug_feature(RID p_scenario, RoomsDebugFeature p_feature, bool p_active);
	void rooms_update_gameplay_monitor(RID p_scenario, const Vector<Vector3> &p_camera_positions);

	// don't use this in a game!
	bool rooms_is_loaded(RID p_scenario) const;

	// callbacks are used to send messages back from the visual server to scene tree in thread friendly manner
	void callbacks_register(VisualServerCallbacks *p_callbacks);

	// don't use these in a game!
	Vector<ObjectID> instances_cull_aabb(const AABB &p_aabb, RID p_scenario = RID()) const;
	Vector<ObjectID> instances_cull_ray(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario = RID()) const;
	Vector<ObjectID> instances_cull_convex(const Vector<Plane> &p_convex, RID p_scenario = RID()) const;

	Array _instances_cull_aabb_bind(const AABB &p_aabb, RID p_scenario = RID()) const;
	Array _instances_cull_ray_bind(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario = RID()) const;
	Array _instances_cull_convex_bind(const Array &p_convex, RID p_scenario = RID()) const;

	enum InstanceFlags {
		INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE,
		INSTANCE_FLAG_MAX
	};

	enum ShadowCastingSetting {
		SHADOW_CASTING_SETTING_OFF,
		SHADOW_CASTING_SETTING_ON,
		SHADOW_CASTING_SETTING_DOUBLE_SIDED,
		SHADOW_CASTING_SETTING_SHADOWS_ONLY,
	};

	void instance_geometry_set_flag(RID p_instance, InstanceFlags p_flags, bool p_enabled);
	void instance_geometry_set_cast_shadows_setting(RID p_instance, ShadowCastingSetting p_shadow_casting_setting);
	void instance_geometry_set_material_override(RID p_instance, RID p_material);
	void instance_geometry_set_material_overlay(RID p_instance, RID p_material);

	void instance_geometry_set_draw_range(RID p_instance, float p_min, float p_max, float p_min_margin, float p_max_margin);
	void instance_geometry_set_as_instance_lod(RID p_instance, RID p_as_lod_of_instance);

	/* CANVAS (2D) */

	RID canvas_create();
	void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2 &p_mirroring);
	void canvas_set_modulate(RID p_canvas, const Color &p_color);
	void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);

	void canvas_set_disable_scale(bool p_disable);

	RID canvas_item_create();
	void canvas_item_set_parent(RID p_item, RID p_parent);

	void canvas_item_set_visible(RID p_item, bool p_visible);
	void canvas_item_set_light_mask(RID p_item, int p_mask);

	void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	void canvas_item_set_transform(RID p_item, const Transform2D &p_transform);
	void canvas_item_set_clip(RID p_item, bool p_clip);
	void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	void canvas_item_set_custom_rect(RID p_item, bool p_custom_rect, const Rect2 &p_rect = Rect2());
	void canvas_item_set_modulate(RID p_item, const Color &p_color);
	void canvas_item_set_self_modulate(RID p_item, const Color &p_color);

	void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);

	enum NinePatchAxisMode {
		NINE_PATCH_STRETCH,
		NINE_PATCH_TILE,
		NINE_PATCH_TILE_FIT,
	};

	void canvas_item_add_line(RID p_item, const Point2 &p_from, const Point2 &p_to, const Color &p_color, float p_width = 1.0, bool p_antialiased = false);
	void canvas_item_add_polyline(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, float p_width = 1.0, bool p_antialiased = false);
	void canvas_item_add_multiline(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, float p_width = 1.0, bool p_antialiased = false);
	void canvas_item_add_rect(RID p_item, const Rect2 &p_rect, const Color &p_color);
	void canvas_item_add_circle(RID p_item, const Point2 &p_pos, float p_radius, const Color &p_color);
	void canvas_item_add_texture_rect(RID p_item, const Rect2 &p_rect, RID p_texture, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID());
	void canvas_item_add_texture_rect_region(RID p_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false);
	void canvas_item_add_nine_patch(RID p_item, const Rect2 &p_rect, const Rect2 &p_source, RID p_texture, const Vector2 &p_topleft, const Vector2 &p_bottomright, NinePatchAxisMode p_x_axis_mode = NINE_PATCH_STRETCH, NinePatchAxisMode p_y_axis_mode = NINE_PATCH_STRETCH, bool p_draw_center = true, const Color &p_modulate = Color(1, 1, 1), RID p_normal_map = RID());
	void canvas_item_add_primitive(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs, RID p_texture, float p_width = 1.0, RID p_normal_map = RID());
	void canvas_item_add_polygon(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), RID p_texture = RID(), RID p_normal_map = RID(), bool p_antialiased = false);
	void canvas_item_add_triangle_array(RID p_item, const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), const Vector<int> &p_bones = Vector<int>(), const Vector<float> &p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1, RID p_normal_map = RID(), bool p_antialiased = false, bool p_antialiasing_use_indices = false);
	void canvas_item_add_mesh(RID p_item, const RID &p_mesh, const Transform2D &p_transform = Transform2D(), const Color &p_modulate = Color(1, 1, 1), RID p_texture = RID(), RID p_normal_map = RID());
	void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID(), RID p_normal_map = RID());
	void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture, RID p_normal_map);
	void canvas_item_add_set_transform(RID p_item, const Transform2D &p_transform);
	void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	void canvas_item_set_z_index(RID p_item, int p_z);
	void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2 &p_rect);

	void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	void canvas_item_clear(RID p_item);
	void canvas_item_set_draw_index(RID p_item, int p_index);

	void canvas_item_set_material(RID p_item, RID p_material);

	void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	RID canvas_light_create();
	void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	void canvas_light_set_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_scale(RID p_light, float p_scale);
	void canvas_light_set_transform(RID p_light, const Transform2D &p_transform);
	void canvas_light_set_texture(RID p_light, RID p_texture);
	void canvas_light_set_texture_offset(RID p_light, const Vector2 &p_offset);
	void canvas_light_set_color(RID p_light, const Color &p_color);
	void canvas_light_set_height(RID p_light, float p_height);
	void canvas_light_set_energy(RID p_light, float p_energy);
	void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);

	enum CanvasLightMode {
		CANVAS_LIGHT_MODE_ADD,
		CANVAS_LIGHT_MODE_SUB,
		CANVAS_LIGHT_MODE_MIX,
		CANVAS_LIGHT_MODE_MASK,
	};

	void canvas_light_set_mode(RID p_light, CanvasLightMode p_mode);

	enum CanvasLightShadowFilter {
		CANVAS_LIGHT_FILTER_NONE,
		CANVAS_LIGHT_FILTER_PCF3,
		CANVAS_LIGHT_FILTER_PCF5,
		CANVAS_LIGHT_FILTER_PCF7,
		CANVAS_LIGHT_FILTER_PCF9,
		CANVAS_LIGHT_FILTER_PCF13,
	};

	void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_shadow_buffer_size(RID p_light, int p_size);
	void canvas_light_set_shadow_gradient_length(RID p_light, float p_length);
	void canvas_light_set_shadow_filter(RID p_light, CanvasLightShadowFilter p_filter);
	void canvas_light_set_shadow_color(RID p_light, const Color &p_color);
	void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	RID canvas_light_occluder_create();
	void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D &p_xform);
	void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	RID canvas_occluder_polygon_create();
	void canvas_occluder_polygon_set_shape(RID p_occluder_polygon, const PoolVector<Vector2> &p_shape, bool p_closed);
	void canvas_occluder_polygon_set_shape_as_lines(RID p_occluder_polygon, const PoolVector<Vector2> &p_shape);

	enum CanvasOccluderPolygonCullMode {
		CANVAS_OCCLUDER_POLYGON_CULL_DISABLED,
		CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE,
		CANVAS_OCCLUDER_POLYGON_CULL_COUNTER_CLOCKWISE,
	};
	void canvas_occluder_polygon_set_cull_mode(RID p_occluder_polygon, CanvasOccluderPolygonCullMode p_mode);

	/* BLACK BARS */

	void black_bars_set_margins(int p_left, int p_top, int p_right, int p_bottom);
	void black_bars_set_images(RID p_left, RID p_top, RID p_right, RID p_bottom);

	/* FREE */

	void free(RID p_rid); ///< free RIDs associated with the visual server

	void request_frame_drawn_callback(Object *p_where, const StringName &p_method, const Variant &p_userdata);

	/* EVENT QUEUING */

	enum ChangedPriority {
		CHANGED_PRIORITY_ANY = 0,
		CHANGED_PRIORITY_LOW,
		CHANGED_PRIORITY_HIGH,
	};

	void draw(bool p_swap_buffers = true, double frame_step = 0.0);
	void sync();
	bool has_changed(ChangedPriority p_priority = CHANGED_PRIORITY_ANY) const;
	void init();
	void finish();
	void tick();
	void pre_draw(bool p_will_draw);

	/* STATUS INFORMATION */

	enum RenderInfo {

		INFO_OBJECTS_IN_FRAME,
		INFO_VERTICES_IN_FRAME,
		INFO_MATERIAL_CHANGES_IN_FRAME,
		INFO_SHADER_CHANGES_IN_FRAME,
		INFO_SHADER_COMPILES_IN_FRAME,
		INFO_SURFACE_CHANGES_IN_FRAME,
		INFO_DRAW_CALLS_IN_FRAME,
		INFO_2D_ITEMS_IN_FRAME,
		INFO_2D_DRAW_CALLS_IN_FRAME,
		INFO_USAGE_VIDEO_MEM_TOTAL,
		INFO_VIDEO_MEM_USED,
		INFO_TEXTURE_MEM_USED,
		INFO_VERTEX_MEM_USED,
	};

	uint64_t get_render_info(RenderInfo p_info);
	String get_video_adapter_name() const;
	String get_video_adapter_vendor() const;

	/* Materials for 2D on 3D */

	/* TESTING */

	RID get_test_cube();

	RID get_test_texture();
	RID get_white_texture();

	RID make_sphere_mesh(int p_lats, int p_lons, float p_radius);

	void mesh_add_surface_from_mesh_data(RID p_mesh, const Geometry::MeshData &p_mesh_data);
	void mesh_add_surface_from_planes(RID p_mesh, const PoolVector<Plane> &p_planes);

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true);
	void set_default_clear_color(const Color &p_color);
	void set_shader_time_scale(float p_scale);

	enum Features {
		FEATURE_SHADERS,
		FEATURE_MULTITHREADED,
	};

	bool has_feature(Features p_feature) const;

	bool has_os_feature(const String &p_feature) const;

	void set_debug_generate_wireframes(bool p_generate);

	void call_set_use_vsync(bool p_enable);

	bool is_low_end() const;

	bool is_render_loop_enabled() const;
	void set_render_loop_enabled(bool p_enabled);

#ifdef DEBUG_ENABLED
	bool is_force_shader_fallbacks_enabled() const;
	void set_force_shader_fallbacks_enabled(bool p_enabled);
#endif

	VisualServer();
	~VisualServer();

private:
	enum {

		MAX_INSTANCE_CULL = 8192,
		MAX_INSTANCE_LIGHTS = 4,
		LIGHT_CACHE_DIRTY = -1,
		MAX_LIGHTS_CULLED = 256,
		MAX_ROOM_CULL = 32,
		MAX_EXTERIOR_PORTALS = 128,
		MAX_LIGHT_SAMPLERS = 256,
		INSTANCE_ROOMLESS_MASK = (1 << 20)

	};

	// low and high priority
	static int changes[2];
	RID test_cube;

	int black_margin[4];
	RID black_image[4];

	struct FrameDrawnCallbacks {
		ObjectID object;
		StringName method;
		Variant param;
	};

	List<FrameDrawnCallbacks> frame_drawn_callbacks;

	void _draw_margins();

	// This function is NOT dead code.
	// It is specifically for debugging redraws to help identify problems with
	// undesired constant editor updating.
	// The function will be called in DEV builds (and thus does not require a recompile),
	// allowing you to place a breakpoint either at the first line or the semicolon.
	// You can then look at the callstack to find the cause of the redraw.
	static void _changes_changed(int p_priority) {
		if (p_priority) {
			;
		}
	}

public:
	// if editor is redrawing when it shouldn't, use a DEV build and put a breakpoint in _changes_changed()
	_FORCE_INLINE_ static void redraw_request(bool p_high_priority = true) {
		int priority = p_high_priority ? 1 : 0;
		changes[priority] += 1;
#ifdef DEV_ENABLED
		_changes_changed(priority);
#endif
	}

#ifdef DEV_ENABLED
#define DISPLAY_CHANGED \
	changes[1] += 1;    \
	_changes_changed(1);
#else
#define DISPLAY_CHANGED \
	changes[1] += 1;
#endif
};

// make variant understand the enums
VARIANT_ENUM_CAST(VisualServer::CubeMapSide);
VARIANT_ENUM_CAST(VisualServer::TextureFlags);
VARIANT_ENUM_CAST(VisualServer::ShaderMode);
VARIANT_ENUM_CAST(VisualServer::ArrayType);
VARIANT_ENUM_CAST(VisualServer::ArrayFormat);
VARIANT_ENUM_CAST(VisualServer::PrimitiveType);
VARIANT_ENUM_CAST(VisualServer::BlendShapeMode);
VARIANT_ENUM_CAST(VisualServer::LightType);
VARIANT_ENUM_CAST(VisualServer::LightParam);
VARIANT_ENUM_CAST(VisualServer::ViewportUpdateMode);
VARIANT_ENUM_CAST(VisualServer::ViewportClearMode);
VARIANT_ENUM_CAST(VisualServer::ViewportMSAA);
VARIANT_ENUM_CAST(VisualServer::ViewportUsage);
VARIANT_ENUM_CAST(VisualServer::ViewportRenderInfo);
VARIANT_ENUM_CAST(VisualServer::ViewportDebugDraw);
VARIANT_ENUM_CAST(VisualServer::ScenarioDebugMode);
VARIANT_ENUM_CAST(VisualServer::InstanceType);
VARIANT_ENUM_CAST(VisualServer::InstancePortalMode);
VARIANT_ENUM_CAST(VisualServer::NinePatchAxisMode);
VARIANT_ENUM_CAST(VisualServer::CanvasLightMode);
VARIANT_ENUM_CAST(VisualServer::CanvasLightShadowFilter);
VARIANT_ENUM_CAST(VisualServer::CanvasOccluderPolygonCullMode);
VARIANT_ENUM_CAST(VisualServer::RenderInfo);
VARIANT_ENUM_CAST(VisualServer::Features);
VARIANT_ENUM_CAST(VisualServer::MultimeshTransformFormat);
VARIANT_ENUM_CAST(VisualServer::MultimeshColorFormat);
VARIANT_ENUM_CAST(VisualServer::MultimeshCustomDataFormat);
VARIANT_ENUM_CAST(VisualServer::MultimeshPhysicsInterpolationQuality);
VARIANT_ENUM_CAST(VisualServer::LightOmniShadowMode);
VARIANT_ENUM_CAST(VisualServer::LightOmniShadowDetail);
VARIANT_ENUM_CAST(VisualServer::LightDirectionalShadowMode);
VARIANT_ENUM_CAST(VisualServer::LightDirectionalShadowDepthRangeMode);
VARIANT_ENUM_CAST(VisualServer::ReflectionProbeUpdateMode);
VARIANT_ENUM_CAST(VisualServer::ParticlesDrawOrder);
VARIANT_ENUM_CAST(VisualServer::EnvironmentBG);
VARIANT_ENUM_CAST(VisualServer::EnvironmentDOFBlurQuality);
VARIANT_ENUM_CAST(VisualServer::EnvironmentGlowBlendMode);
VARIANT_ENUM_CAST(VisualServer::EnvironmentToneMapper);
VARIANT_ENUM_CAST(VisualServer::EnvironmentSSAOQuality);
VARIANT_ENUM_CAST(VisualServer::EnvironmentSSAOBlur);
VARIANT_ENUM_CAST(VisualServer::InstanceFlags);
VARIANT_ENUM_CAST(VisualServer::ShadowCastingSetting);
VARIANT_ENUM_CAST(VisualServer::TextureType);
VARIANT_ENUM_CAST(VisualServer::ChangedPriority);

//typedef VisualServer VS; // makes it easier to use
#define VS VisualServer

#endif // VISUAL_SERVER_H

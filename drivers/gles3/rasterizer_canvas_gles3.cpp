/*************************************************************************/
/*  rasterizer_canvas_gles3.cpp                                          */
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

#include "rasterizer_canvas_gles3.h"

#include "core/os/os.h"
#include "core/project_settings.h"

#include "drivers/gles_common/rasterizer_array.h"
#include "drivers/gles_common/rasterizer_asserts.h"
#include "drivers/gles_common/rasterizer_storage_common.h"

#include "rasterizer_scene_gles3.h"
#include "servers/visual/visual_server_raster.h"

static const GLenum gl_primitive[] = {
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN
};

#ifndef GLES_OVER_GL
#define glClearDepth glClearDepthf
#endif

static _FORCE_INLINE_ void store_transform2d(const Transform2D &p_mtx, float *p_array) {
	p_array[0] = p_mtx.elements[0][0];
	p_array[1] = p_mtx.elements[0][1];
	p_array[2] = 0;
	p_array[3] = 0;
	p_array[4] = p_mtx.elements[1][0];
	p_array[5] = p_mtx.elements[1][1];
	p_array[6] = 0;
	p_array[7] = 0;
	p_array[8] = 0;
	p_array[9] = 0;
	p_array[10] = 1;
	p_array[11] = 0;
	p_array[12] = p_mtx.elements[2][0];
	p_array[13] = p_mtx.elements[2][1];
	p_array[14] = 0;
	p_array[15] = 1;
}

static _FORCE_INLINE_ void store_transform(const Transform &p_mtx, float *p_array) {
	p_array[0] = p_mtx.basis.elements[0][0];
	p_array[1] = p_mtx.basis.elements[1][0];
	p_array[2] = p_mtx.basis.elements[2][0];
	p_array[3] = 0;
	p_array[4] = p_mtx.basis.elements[0][1];
	p_array[5] = p_mtx.basis.elements[1][1];
	p_array[6] = p_mtx.basis.elements[2][1];
	p_array[7] = 0;
	p_array[8] = p_mtx.basis.elements[0][2];
	p_array[9] = p_mtx.basis.elements[1][2];
	p_array[10] = p_mtx.basis.elements[2][2];
	p_array[11] = 0;
	p_array[12] = p_mtx.origin.x;
	p_array[13] = p_mtx.origin.y;
	p_array[14] = p_mtx.origin.z;
	p_array[15] = 1;
}

static _FORCE_INLINE_ void store_camera(const CameraMatrix &p_mtx, float *p_array) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			p_array[i * 4 + j] = p_mtx.matrix[i][j];
		}
	}
}

RID RasterizerCanvasGLES3::light_internal_create() {
	LightInternal *li = memnew(LightInternal);

	glGenBuffers(1, &li->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, li->ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightInternal::UBOData), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return light_internal_owner.make_rid(li);
}

void RasterizerCanvasGLES3::light_internal_update(RID p_rid, Light *p_light) {
	LightInternal *li = light_internal_owner.getornull(p_rid);
	ERR_FAIL_COND(!li);

	store_transform2d(p_light->light_shader_xform, li->ubo_data.light_matrix);
	store_transform2d(p_light->xform_cache.affine_inverse(), li->ubo_data.local_matrix);
	store_camera(p_light->shadow_matrix_cache, li->ubo_data.shadow_matrix);

	for (int i = 0; i < 4; i++) {
		li->ubo_data.color[i] = p_light->color[i] * p_light->energy;
		li->ubo_data.shadow_color[i] = p_light->shadow_color[i];
	}

	li->ubo_data.light_pos[0] = p_light->light_shader_pos.x;
	li->ubo_data.light_pos[1] = p_light->light_shader_pos.y;
	li->ubo_data.shadowpixel_size = (1.0 / p_light->shadow_buffer_size) * (1.0 + p_light->shadow_smooth);
	li->ubo_data.light_outside_alpha = p_light->mode == VS::CANVAS_LIGHT_MODE_MASK ? 1.0 : 0.0;
	li->ubo_data.light_height = p_light->height;
	if (p_light->radius_cache == 0) {
		li->ubo_data.shadow_gradient = 0;
	} else {
		li->ubo_data.shadow_gradient = p_light->shadow_gradient_length / (p_light->radius_cache * 1.1);
	}

	li->ubo_data.shadow_distance_mult = (p_light->radius_cache * 1.1);

	glBindBuffer(GL_UNIFORM_BUFFER, li->ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightInternal::UBOData), &li->ubo_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RasterizerCanvasGLES3::light_internal_free(RID p_rid) {
	LightInternal *li = light_internal_owner.getornull(p_rid);
	ERR_FAIL_COND(!li);

	glDeleteBuffers(1, &li->ubo);
	light_internal_owner.free(p_rid);
	memdelete(li);
}

void RasterizerCanvasGLES3::canvas_begin() {
	batch_canvas_begin();
	if (storage->frame.current_rt && storage->frame.clear_request) {
		// a clear request may be pending, so do it
		bool transparent = storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT];

		glClearColor(storage->frame.clear_request_color.r,
				storage->frame.clear_request_color.g,
				storage->frame.clear_request_color.b,
				transparent ? storage->frame.clear_request_color.a : 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		storage->frame.clear_request = false;
		glColorMask(1, 1, 1, transparent ? 1 : 0);
	}

	reset_canvas();

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_TEXTURE_RECT, true);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_LIGHTING, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SHADOWS, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_NEAREST, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF3, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF5, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF7, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF9, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF13, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_DISTANCE_FIELD, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_NINEPATCH, false);

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_LIGHT_ANGLE, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_MODULATE, false);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_LARGE_VERTEX, false);

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SKELETON, false);

	state.canvas_shader.set_custom_shader(0);
	state.canvas_shader.bind();
	state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, Color(1, 1, 1, 1));
	state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, Transform2D());
	state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, Transform2D());
	if (storage->frame.current_rt) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
	} else {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
	}

	//state.canvas_shader.set_uniform(CanvasShaderGLES3::PROJECTION_MATRIX,state.vp);
	//state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX,Transform());
	//state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX,Transform());

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, state.canvas_item_ubo);
	glBindVertexArray(data.canvas_quad_array);
	state.using_texture_rect = true;
	state.using_ninepatch = false;

	state.using_light_angle = false;
	state.using_modulate = false;
	state.using_large_vertex = false;

	state.using_skeleton = false;
}

void RasterizerCanvasGLES3::canvas_end() {
	batch_canvas_end();
	glBindVertexArray(0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
	glColorMask(1, 1, 1, 1);

	glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);

	state.using_texture_rect = false;
	state.using_ninepatch = false;
	state.using_light_angle = false;
}

RasterizerStorageGLES3::Texture *RasterizerCanvasGLES3::_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map, bool p_force) {
	RasterizerStorageGLES3::Texture *tex_return = nullptr;

	if (p_texture == state.current_tex && !p_force) {
		tex_return = state.current_tex_ptr;
	} else if (p_texture.is_valid()) {
		RasterizerStorageGLES3::Texture *texture = storage->texture_owner.getornull(p_texture);

		if (!texture) {
			state.current_tex = RID();
			state.current_tex_ptr = nullptr;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

		} else {
			if (texture->redraw_if_visible) { //check before proxy, because this is usually used with proxies
				VisualServerRaster::redraw_request(false);
			}

			texture = texture->get_ptr();

			if (texture->render_target) {
				texture->render_target->used_in_frame = true;
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->tex_id);
			state.current_tex = p_texture;
			state.current_tex_ptr = texture;

			tex_return = texture;
		}

	} else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
		state.current_tex = RID();
		state.current_tex_ptr = nullptr;
	}

	if (p_normal_map == state.current_normal && !p_force) {
		//do none
		state.canvas_shader.set_uniform(CanvasShaderGLES3::USE_DEFAULT_NORMAL, state.current_normal.is_valid());

	} else if (p_normal_map.is_valid()) {
		RasterizerStorageGLES3::Texture *normal_map = storage->texture_owner.getornull(p_normal_map);

		if (!normal_map) {
			state.current_normal = RID();
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
			state.canvas_shader.set_uniform(CanvasShaderGLES3::USE_DEFAULT_NORMAL, false);

		} else {
			if (normal_map->redraw_if_visible) { //check before proxy, because this is usually used with proxies
				VisualServerRaster::redraw_request(false);
			}

			normal_map = normal_map->get_ptr();

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, normal_map->tex_id);
			state.current_normal = p_normal_map;
			state.canvas_shader.set_uniform(CanvasShaderGLES3::USE_DEFAULT_NORMAL, true);
		}

	} else {
		state.current_normal = RID();
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
		state.canvas_shader.set_uniform(CanvasShaderGLES3::USE_DEFAULT_NORMAL, false);
	}

	return tex_return;
}

void RasterizerCanvasGLES3::_set_texture_rect_mode(bool p_enable, bool p_ninepatch, bool p_light_angle, bool p_modulate, bool p_large_vertex) {
	// this state check could be done individually
	if (state.using_texture_rect == p_enable && state.using_ninepatch == p_ninepatch && state.using_light_angle == p_light_angle && state.using_modulate == p_modulate && state.using_large_vertex == p_large_vertex) {
		return;
	}

	if (p_enable) {
		glBindVertexArray(data.canvas_quad_array);

	} else {
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_NINEPATCH, p_ninepatch && p_enable);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_TEXTURE_RECT, p_enable);

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_LIGHT_ANGLE, p_light_angle);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_MODULATE, p_modulate);
	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_ATTRIB_LARGE_VERTEX, p_large_vertex);

	state.canvas_shader.bind();
	state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, state.canvas_item_modulate);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, state.extra_matrix);
	if (state.using_skeleton) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM, state.skeleton_transform);
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM_INVERSE, state.skeleton_transform_inverse);
	}
	if (storage->frame.current_rt) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
	} else {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
	}

	state.using_texture_rect = p_enable;
	state.using_ninepatch = p_ninepatch;

	state.using_light_angle = p_light_angle;
	state.using_modulate = p_modulate;
	state.using_large_vertex = p_large_vertex;
}

void RasterizerCanvasGLES3::_draw_polygon(const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor, const int *p_bones, const float *p_weights) {
	glBindVertexArray(data.polygon_buffer_pointer_array);
	glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);

	uint32_t buffer_ofs = 0;
	uint32_t buffer_ofs_after = buffer_ofs + (sizeof(Vector2) * p_vertex_count);
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(buffer_ofs_after > data.polygon_buffer_size);
#endif

	storage->buffer_orphan_and_upload(data.polygon_buffer_size, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_vertices, GL_ARRAY_BUFFER, _buffer_upload_usage_flag);

	glEnableVertexAttribArray(VS::ARRAY_VERTEX);
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
	buffer_ofs = buffer_ofs_after;

	//color
	if (p_singlecolor) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		Color m = *p_colors;
		glVertexAttrib4f(VS::ARRAY_COLOR, m.r, m.g, m.b, m.a);
	} else if (!p_colors) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
	} else {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Color) * p_vertex_count, p_colors, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, false, sizeof(Color), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;
	}

	if (p_uvs) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_uvs, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

	if (p_bones && p_weights) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(int) * 4 * p_vertex_count, p_bones, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_BONES);
		//glVertexAttribPointer(VS::ARRAY_BONES, 4, GL_UNSIGNED_INT, false, sizeof(int) * 4, ((uint8_t *)0) + buffer_ofs);
		glVertexAttribIPointer(VS::ARRAY_BONES, 4, GL_UNSIGNED_INT, sizeof(int) * 4, CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(float) * 4 * p_vertex_count, p_weights, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_WEIGHTS);
		glVertexAttribPointer(VS::ARRAY_WEIGHTS, 4, GL_FLOAT, false, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

	} else if (state.using_skeleton) {
		glVertexAttribI4ui(VS::ARRAY_BONES, 0, 0, 0, 0);
		glVertexAttrib4f(VS::ARRAY_WEIGHTS, 0, 0, 0, 0);
	}

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND((sizeof(int) * p_index_count) > data.polygon_index_buffer_size);
#endif

	//bind the indices buffer.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.polygon_index_buffer);
	storage->buffer_orphan_and_upload(data.polygon_index_buffer_size, 0, sizeof(int) * p_index_count, p_indices, GL_ELEMENT_ARRAY_BUFFER, _buffer_upload_usage_flag);

	//draw the triangles.
	glDrawElements(GL_TRIANGLES, p_index_count, GL_UNSIGNED_INT, nullptr);

	storage->info.render._2d_draw_call_count++;

	if (p_bones && p_weights) {
		//not used so often, so disable when used
		glDisableVertexAttribArray(VS::ARRAY_BONES);
		glDisableVertexAttribArray(VS::ARRAY_WEIGHTS);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::_draw_generic(GLuint p_primitive, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor) {
	glBindVertexArray(data.polygon_buffer_pointer_array);
	glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);

	//vertex
	uint32_t buffer_ofs = 0;
	uint32_t buffer_ofs_after = buffer_ofs + (sizeof(Vector2) * p_vertex_count);
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(buffer_ofs_after > data.polygon_buffer_size);
#endif
	storage->buffer_orphan_and_upload(data.polygon_buffer_size, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_vertices, GL_ARRAY_BUFFER, _buffer_upload_usage_flag);

	glEnableVertexAttribArray(VS::ARRAY_VERTEX);
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
	buffer_ofs = buffer_ofs_after;

	//color
	if (p_singlecolor) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		Color m = *p_colors;
		glVertexAttrib4f(VS::ARRAY_COLOR, m.r, m.g, m.b, m.a);
	} else if (!p_colors) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
	} else {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Color) * p_vertex_count, p_colors, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, false, sizeof(Color), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;
	}

	if (p_uvs) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_uvs, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

	glDrawArrays(p_primitive, 0, p_vertex_count);

	storage->info.render._2d_draw_call_count++;

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::_draw_generic_indices(GLuint p_primitive, const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor) {
	glBindVertexArray(data.polygon_buffer_pointer_array);
	glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);

	//vertex
	uint32_t buffer_ofs = 0;
	uint32_t buffer_ofs_after = buffer_ofs + (sizeof(Vector2) * p_vertex_count);
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(buffer_ofs_after > data.polygon_buffer_size);
#endif
	storage->buffer_orphan_and_upload(data.polygon_buffer_size, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_vertices, GL_ARRAY_BUFFER, _buffer_upload_usage_flag);

	glEnableVertexAttribArray(VS::ARRAY_VERTEX);
	glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
	buffer_ofs = buffer_ofs_after;

	//color
	if (p_singlecolor) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		Color m = *p_colors;
		glVertexAttrib4f(VS::ARRAY_COLOR, m.r, m.g, m.b, m.a);
	} else if (!p_colors) {
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
	} else {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Color) * p_vertex_count, p_colors, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, false, sizeof(Color), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;
	}

	if (p_uvs) {
		RAST_FAIL_COND(!storage->safe_buffer_sub_data(data.polygon_buffer_size, GL_ARRAY_BUFFER, buffer_ofs, sizeof(Vector2) * p_vertex_count, p_uvs, buffer_ofs_after));
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, false, sizeof(Vector2), CAST_INT_TO_UCHAR_PTR(buffer_ofs));
		buffer_ofs = buffer_ofs_after;

	} else {
		glDisableVertexAttribArray(VS::ARRAY_TEX_UV);
	}

#ifdef RASTERIZER_EXTRA_CHECKS
	// very slow, do not enable in normal use
	for (int n = 0; n < p_index_count; n++) {
		RAST_DEV_DEBUG_ASSERT(p_indices[n] < p_vertex_count);
	}
#endif

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND((sizeof(int) * p_index_count) > data.polygon_index_buffer_size);
#endif

	//bind the indices buffer.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.polygon_index_buffer);
	storage->buffer_orphan_and_upload(data.polygon_index_buffer_size, 0, sizeof(int) * p_index_count, p_indices, GL_ELEMENT_ARRAY_BUFFER, _buffer_upload_usage_flag);

	//draw the triangles.
	glDrawElements(p_primitive, p_index_count, GL_UNSIGNED_INT, nullptr);

	storage->info.render._2d_draw_call_count++;

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::_draw_gui_primitive(int p_points, const Vector2 *p_vertices, const Color *p_colors, const Vector2 *p_uvs, const float *p_light_angles) {
	static const GLenum prim[5] = { GL_POINTS, GL_POINTS, GL_LINES, GL_TRIANGLES, GL_TRIANGLE_FAN };

	//#define GLES_USE_PRIMITIVE_BUFFER

	int version = 0;
	int color_ofs = 0;
	int uv_ofs = 0;
	int light_angle_ofs = 0;
	int stride = 2;

	if (p_colors) { //color
		version |= 1;
		color_ofs = stride;
		stride += 4;
	}

	if (p_uvs) { //uv
		version |= 2;
		uv_ofs = stride;
		stride += 2;
	}

	if (p_light_angles) { //light_angles
		version |= 4;
		light_angle_ofs = stride;
		stride += 1;
	}

	DEV_ASSERT(p_points <= 4);
	float b[(2 + 2 + 4 + 1) * 4];

	for (int i = 0; i < p_points; i++) {
		b[stride * i + 0] = p_vertices[i].x;
		b[stride * i + 1] = p_vertices[i].y;
	}

	if (p_colors) {
		for (int i = 0; i < p_points; i++) {
			b[stride * i + color_ofs + 0] = p_colors[i].r;
			b[stride * i + color_ofs + 1] = p_colors[i].g;
			b[stride * i + color_ofs + 2] = p_colors[i].b;
			b[stride * i + color_ofs + 3] = p_colors[i].a;
		}
	}

	if (p_uvs) {
		for (int i = 0; i < p_points; i++) {
			b[stride * i + uv_ofs + 0] = p_uvs[i].x;
			b[stride * i + uv_ofs + 1] = p_uvs[i].y;
		}
	}

	if (p_light_angles) {
		for (int i = 0; i < p_points; i++) {
			b[stride * i + light_angle_ofs] = p_light_angles[i];
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);
	storage->buffer_orphan_and_upload(data.polygon_buffer_size, 0, p_points * stride * sizeof(float), &b[0], GL_ARRAY_BUFFER, _buffer_upload_usage_flag);

	glBindVertexArray(data.polygon_buffer_quad_arrays[version]);
	glDrawArrays(prim[p_points], 0, p_points);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	storage->info.render._2d_draw_call_count++;
}

void RasterizerCanvasGLES3::render_rect_nvidia_workaround(const Item::CommandRect *p_rect, const RasterizerStorageGLES3::Texture *p_texture) {
	if (p_texture) {
		bool send_light_angles = false;

		// only need to use light angles when normal mapping
		// otherwise we can use the default shader
		if (state.current_normal != RID()) {
			send_light_angles = true;
		}

		// we don't want to use texture rect, and we want to send light angles if we are using normal mapping
		_set_texture_rect_mode(false, false, send_light_angles);

		bool untile = false;

		if (p_rect->flags & CANVAS_RECT_TILE && !(p_texture->flags & VS::TEXTURE_FLAG_REPEAT)) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			untile = true;
		}

		Size2 texpixel_size(1.0 / p_texture->width, 1.0 / p_texture->height);

		state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, p_rect->flags & CANVAS_RECT_CLIP_UV);

		Vector2 points[4] = {
			p_rect->rect.position,
			p_rect->rect.position + Vector2(p_rect->rect.size.x, 0.0),
			p_rect->rect.position + p_rect->rect.size,
			p_rect->rect.position + Vector2(0.0, p_rect->rect.size.y),
		};

		if (p_rect->rect.size.x < 0) {
			SWAP(points[0], points[1]);
			SWAP(points[2], points[3]);
		}
		if (p_rect->rect.size.y < 0) {
			SWAP(points[0], points[3]);
			SWAP(points[1], points[2]);
		}
		Rect2 src_rect = (p_rect->flags & CANVAS_RECT_REGION) ? Rect2(p_rect->source.position * texpixel_size, p_rect->source.size * texpixel_size) : Rect2(0, 0, 1, 1);

		Vector2 uvs[4] = {
			src_rect.position,
			src_rect.position + Vector2(src_rect.size.x, 0.0),
			src_rect.position + src_rect.size,
			src_rect.position + Vector2(0.0, src_rect.size.y),
		};

		// for encoding in light angle
		bool flip_h = false;
		bool flip_v = false;

		if (p_rect->flags & CANVAS_RECT_TRANSPOSE) {
			SWAP(uvs[1], uvs[3]);
		}

		if (p_rect->flags & CANVAS_RECT_FLIP_H) {
			SWAP(uvs[0], uvs[1]);
			SWAP(uvs[2], uvs[3]);
			flip_h = true;
			flip_v = !flip_v;
		}
		if (p_rect->flags & CANVAS_RECT_FLIP_V) {
			SWAP(uvs[0], uvs[3]);
			SWAP(uvs[1], uvs[2]);
			flip_v = !flip_v;
		}

		if (send_light_angles) {
			// for single rects, there is no need to fully utilize the light angle,
			// we only need it to encode flips (horz and vert). But the shader can be reused with
			// batching in which case the angle encodes the transform as well as
			// the flips.
			// Note transpose is NYI. I don't think it worked either with the non-nvidia method.

			// if horizontal flip, angle is 180
			float angle = 0.0f;
			if (flip_h) {
				angle = Math_PI;
			}

			// add 1 (to take care of zero floating point error with sign)
			angle += 1.0f;

			// flip if necessary
			if (flip_v) {
				angle *= -1.0f;
			}

			// light angle must be sent for each vert, instead as a single uniform in the uniform draw method
			// this has the benefit of enabling batching with light angles.
			float light_angles[4] = { angle, angle, angle, angle };

			_draw_gui_primitive(4, points, nullptr, uvs, light_angles);
		} else {
			_draw_gui_primitive(4, points, nullptr, uvs);
		}

		if (untile) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

	} else {
		_set_texture_rect_mode(false);

		state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);

		Vector2 points[4] = {
			p_rect->rect.position,
			p_rect->rect.position + Vector2(p_rect->rect.size.x, 0.0),
			p_rect->rect.position + p_rect->rect.size,
			p_rect->rect.position + Vector2(0.0, p_rect->rect.size.y),
		};

		_draw_gui_primitive(4, points, nullptr, nullptr);
	}
}

void RasterizerCanvasGLES3::_copy_texscreen(const Rect2 &p_rect) {
	ERR_FAIL_COND_MSG(storage->frame.current_rt->effects.mip_maps[0].sizes.size() == 0, "Can't use screen texture copying in a render target configured without copy buffers. To resolve this, change the viewport's Usage property to \"2D\" or \"3D\" instead of \"2D Without Sampling\" or \"3D Without Effects\" respectively.");

	glDisable(GL_BLEND);

	state.canvas_texscreen_used = true;
	//blur diffuse into effect mipmaps using separatable convolution
	//storage->shaders.copy.set_conditional(CopyShaderGLES3::GAUSSIAN_HORIZONTAL,true);

	Vector2 wh(storage->frame.current_rt->width, storage->frame.current_rt->height);

	Color blur_section(p_rect.position.x / wh.x, p_rect.position.y / wh.y, p_rect.size.x / wh.x, p_rect.size.y / wh.y);

	if (p_rect != Rect2()) {
		scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::USE_BLUR_SECTION, true);
		storage->shaders.copy.set_conditional(CopyShaderGLES3::USE_COPY_SECTION, true);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->effects.mip_maps[0].sizes[0].fbo);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->color);

	storage->shaders.copy.bind();
	storage->shaders.copy.set_uniform(CopyShaderGLES3::COPY_SECTION, blur_section);

	scene_render->_copy_screen();

	for (int i = 0; i < storage->frame.current_rt->effects.mip_maps[1].sizes.size(); i++) {
		int vp_w = storage->frame.current_rt->effects.mip_maps[1].sizes[i].width;
		int vp_h = storage->frame.current_rt->effects.mip_maps[1].sizes[i].height;
		glViewport(0, 0, vp_w, vp_h);
		//horizontal pass
		scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::GAUSSIAN_HORIZONTAL, true);
		scene_render->state.effect_blur_shader.bind();
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::PIXEL_SIZE, Vector2(1.0 / vp_w, 1.0 / vp_h));
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::LOD, float(i));
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::BLUR_SECTION, blur_section);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->effects.mip_maps[0].color); //previous level, since mipmaps[0] starts one level bigger
		glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->effects.mip_maps[1].sizes[i].fbo);

		scene_render->_copy_screen();

		scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::GAUSSIAN_HORIZONTAL, false);

		//vertical pass
		scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::GAUSSIAN_VERTICAL, true);
		scene_render->state.effect_blur_shader.bind();
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::PIXEL_SIZE, Vector2(1.0 / vp_w, 1.0 / vp_h));
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::LOD, float(i));
		scene_render->state.effect_blur_shader.set_uniform(EffectBlurShaderGLES3::BLUR_SECTION, blur_section);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->effects.mip_maps[1].color);
		glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->effects.mip_maps[0].sizes[i + 1].fbo); //next level, since mipmaps[0] starts one level bigger

		scene_render->_copy_screen();

		scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::GAUSSIAN_VERTICAL, false);
	}

	scene_render->state.effect_blur_shader.set_conditional(EffectBlurShaderGLES3::USE_BLUR_SECTION, false);
	storage->shaders.copy.set_conditional(CopyShaderGLES3::USE_COPY_SECTION, false);

	glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->fbo); //back to front
	glViewport(0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height);

	// back to canvas, force rebind
	state.using_texture_rect = true;
	_set_texture_rect_mode(false);

	_bind_canvas_texture(state.current_tex, state.current_normal, true);

	glEnable(GL_BLEND);
}

void RasterizerCanvasGLES3::canvas_debug_viewport_shadows(Light *p_lights_with_shadow) {
	Light *light = p_lights_with_shadow;

	canvas_begin(); //reset
	glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
	int h = 10;
	int w = storage->frame.current_rt->width;
	int ofs = h;
	glDisable(GL_BLEND);

	while (light) {
		if (light->shadow_buffer.is_valid()) {
			RasterizerStorageGLES3::CanvasLightShadow *sb = storage->canvas_light_shadow_owner.get(light->shadow_buffer);
			if (sb) {
				glBindTexture(GL_TEXTURE_2D, sb->distance);
				draw_generic_textured_rect(Rect2(h, ofs, w - h * 2, h), Rect2(0, 0, 1, 1));
				ofs += h * 2;
			}
		}

		light = light->shadows_next_ptr;
	}

	canvas_end();
}

void RasterizerCanvasGLES3::canvas_light_shadow_buffer_update(RID p_buffer, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders, CameraMatrix *p_xform_cache) {
	RasterizerStorageGLES3::CanvasLightShadow *cls = storage->canvas_light_shadow_owner.get(p_buffer);
	ERR_FAIL_COND(!cls);

	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);

	glBindFramebuffer(GL_FRAMEBUFFER, cls->fbo);

	state.canvas_shadow_shader.bind();

	glViewport(0, 0, cls->size, cls->height);
	glClearDepth(1.0f);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	VS::CanvasOccluderPolygonCullMode cull = VS::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;

	for (int i = 0; i < 4; i++) {
		//make sure it remains orthogonal, makes easy to read angle later

		Transform light;
		light.origin[0] = p_light_xform[2][0];
		light.origin[1] = p_light_xform[2][1];
		light.basis[0][0] = p_light_xform[0][0];
		light.basis[0][1] = p_light_xform[1][0];
		light.basis[1][0] = p_light_xform[0][1];
		light.basis[1][1] = p_light_xform[1][1];

		//light.basis.scale(Vector3(to_light.elements[0].length(),to_light.elements[1].length(),1));

		//p_near=1;
		CameraMatrix projection;
		{
			real_t fov = 90;
			real_t nearp = p_near;
			real_t farp = p_far;
			real_t aspect = 1.0;

			real_t ymax = nearp * Math::tan(Math::deg2rad(fov * 0.5));
			real_t ymin = -ymax;
			real_t xmin = ymin * aspect;
			real_t xmax = ymax * aspect;

			projection.set_frustum(xmin, xmax, ymin, ymax, nearp, farp);
		}

		Vector3 cam_target = Basis(Vector3(0, 0, Math_PI * 2 * (i / 4.0))).xform(Vector3(0, 1, 0));
		projection = projection * CameraMatrix(Transform().looking_at(cam_target, Vector3(0, 0, -1)).affine_inverse());

		state.canvas_shadow_shader.set_uniform(CanvasShadowShaderGLES3::PROJECTION_MATRIX, projection);
		state.canvas_shadow_shader.set_uniform(CanvasShadowShaderGLES3::LIGHT_MATRIX, light);
		state.canvas_shadow_shader.set_uniform(CanvasShadowShaderGLES3::DISTANCE_NORM, 1.0 / p_far);

		if (i == 0) {
			*p_xform_cache = projection;
		}

		glViewport(0, (cls->height / 4) * i, cls->size, cls->height / 4);

		LightOccluderInstance *instance = p_occluders;

		while (instance) {
			RasterizerStorageGLES3::CanvasOccluder *cc = storage->canvas_occluder_owner.getornull(instance->polygon_buffer);
			if (!cc || cc->len == 0 || !(p_light_mask & instance->light_mask)) {
				instance = instance->next;
				continue;
			}

			state.canvas_shadow_shader.set_uniform(CanvasShadowShaderGLES3::WORLD_MATRIX, instance->xform_cache);

			VS::CanvasOccluderPolygonCullMode transformed_cull_cache = instance->cull_cache;

			if (transformed_cull_cache != VS::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED &&
					(p_light_xform.basis_determinant() * instance->xform_cache.basis_determinant()) < 0) {
				transformed_cull_cache = (transformed_cull_cache == VS::CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE)
						? VS::CANVAS_OCCLUDER_POLYGON_CULL_COUNTER_CLOCKWISE
						: VS::CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE;
			}

			if (cull != transformed_cull_cache) {
				cull = transformed_cull_cache;
				switch (cull) {
					case VS::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED: {
						glDisable(GL_CULL_FACE);

					} break;
					case VS::CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE: {
						glEnable(GL_CULL_FACE);
						glCullFace(GL_FRONT);
					} break;
					case VS::CANVAS_OCCLUDER_POLYGON_CULL_COUNTER_CLOCKWISE: {
						glEnable(GL_CULL_FACE);
						glCullFace(GL_BACK);

					} break;
				}
			}

			glBindVertexArray(cc->array_id);
			glDrawElements(GL_TRIANGLES, cc->len * 3, GL_UNSIGNED_SHORT, nullptr);

			instance = instance->next;
		}
	}

	glBindVertexArray(0);
}
void RasterizerCanvasGLES3::reset_canvas() {
	if (storage->frame.current_rt) {
		glBindFramebuffer(GL_FRAMEBUFFER, storage->frame.current_rt->fbo);
		glColorMask(1, 1, 1, 1); //don't touch alpha
	}

	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DITHER);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	//glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	//glLineWidth(1.0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//use for reading from screen
	if (storage->frame.current_rt && !storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_NO_SAMPLING]) {
		glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
		glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->effects.mip_maps[0].color);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);

	Transform canvas_transform;

	if (storage->frame.current_rt) {
		float csy = 1.0;
		if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
			csy = -1.0;
		}
		canvas_transform.translate(-(storage->frame.current_rt->width / 2.0f), -(storage->frame.current_rt->height / 2.0f), 0.0f);
		canvas_transform.scale(Vector3(2.0f / storage->frame.current_rt->width, csy * -2.0f / storage->frame.current_rt->height, 1.0f));
	} else {
		Vector2 ssize = OS::get_singleton()->get_window_size();
		canvas_transform.translate(-(ssize.width / 2.0f), -(ssize.height / 2.0f), 0.0f);
		canvas_transform.scale(Vector3(2.0f / ssize.width, -2.0f / ssize.height, 1.0f));
	}

	state.vp = canvas_transform;

	store_transform(canvas_transform, state.canvas_item_ubo_data.projection_matrix);
	state.canvas_item_ubo_data.time = storage->frame.time[0];

	glBindBuffer(GL_UNIFORM_BUFFER, state.canvas_item_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CanvasItemUBO), &state.canvas_item_ubo_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	state.canvas_texscreen_used = false;
}

void RasterizerCanvasGLES3::draw_generic_textured_rect(const Rect2 &p_rect, const Rect2 &p_src) {
	state.canvas_shader.set_uniform(CanvasShaderGLES3::DST_RECT, Color(p_rect.position.x, p_rect.position.y, p_rect.size.x, p_rect.size.y));
	state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(p_src.position.x, p_src.position.y, p_src.size.x, p_src.size.y));
	state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void RasterizerCanvasGLES3::draw_lens_distortion_rect(const Rect2 &p_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample) {
	Vector2 half_size;
	if (storage->frame.current_rt) {
		half_size = Vector2(storage->frame.current_rt->width, storage->frame.current_rt->height);
	} else {
		half_size = OS::get_singleton()->get_window_size();
	}
	half_size *= 0.5;
	Vector2 offset((p_rect.position.x - half_size.x) / half_size.x, (p_rect.position.y - half_size.y) / half_size.y);
	Vector2 scale(p_rect.size.x / half_size.x, p_rect.size.y / half_size.y);

	float aspect_ratio = p_rect.size.x / p_rect.size.y;

	// setup our lens shader
	state.lens_shader.bind();
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::OFFSET, offset);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::SCALE, scale);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::K1, p_k1);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::K2, p_k2);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::EYE_CENTER, p_eye_center);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::UPSCALE, p_oversample);
	state.lens_shader.set_uniform(LensDistortedShaderGLES3::ASPECT_RATIO, aspect_ratio);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, state.canvas_item_ubo);
	glBindVertexArray(data.canvas_quad_array);

	// and draw
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindVertexArray(0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
}

void RasterizerCanvasGLES3::draw_window_margins(int *black_margin, RID *black_image) {
	Vector2 window_size = OS::get_singleton()->get_window_size();
	int window_h = window_size.height;
	int window_w = window_size.width;

	glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES3::system_fbo);
	glViewport(0, 0, window_size.width, window_size.height);
	canvas_begin();

	if (black_image[MARGIN_LEFT].is_valid()) {
		_bind_canvas_texture(black_image[MARGIN_LEFT], RID(), true);
		Size2 sz(storage->texture_get_width(black_image[MARGIN_LEFT]), storage->texture_get_height(black_image[MARGIN_LEFT]));

		draw_generic_textured_rect(Rect2(0, 0, black_margin[MARGIN_LEFT], window_h),
				Rect2(0, 0, (float)black_margin[MARGIN_LEFT] / sz.x, (float)(window_h) / sz.y));
	} else if (black_margin[MARGIN_LEFT]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);

		draw_generic_textured_rect(Rect2(0, 0, black_margin[MARGIN_LEFT], window_h), Rect2(0, 0, 1, 1));
	}

	if (black_image[MARGIN_RIGHT].is_valid()) {
		_bind_canvas_texture(black_image[MARGIN_RIGHT], RID(), true);
		Size2 sz(storage->texture_get_width(black_image[MARGIN_RIGHT]), storage->texture_get_height(black_image[MARGIN_RIGHT]));
		draw_generic_textured_rect(Rect2(window_w - black_margin[MARGIN_RIGHT], 0, black_margin[MARGIN_RIGHT], window_h),
				Rect2(0, 0, (float)black_margin[MARGIN_RIGHT] / sz.x, (float)window_h / sz.y));
	} else if (black_margin[MARGIN_RIGHT]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);

		draw_generic_textured_rect(Rect2(window_w - black_margin[MARGIN_RIGHT], 0, black_margin[MARGIN_RIGHT], window_h), Rect2(0, 0, 1, 1));
	}

	if (black_image[MARGIN_TOP].is_valid()) {
		_bind_canvas_texture(black_image[MARGIN_TOP], RID(), true);

		Size2 sz(storage->texture_get_width(black_image[MARGIN_TOP]), storage->texture_get_height(black_image[MARGIN_TOP]));
		draw_generic_textured_rect(Rect2(0, 0, window_w, black_margin[MARGIN_TOP]),
				Rect2(0, 0, (float)window_w / sz.x, (float)black_margin[MARGIN_TOP] / sz.y));

	} else if (black_margin[MARGIN_TOP]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);

		draw_generic_textured_rect(Rect2(0, 0, window_w, black_margin[MARGIN_TOP]), Rect2(0, 0, 1, 1));
	}

	if (black_image[MARGIN_BOTTOM].is_valid()) {
		_bind_canvas_texture(black_image[MARGIN_BOTTOM], RID(), true);

		Size2 sz(storage->texture_get_width(black_image[MARGIN_BOTTOM]), storage->texture_get_height(black_image[MARGIN_BOTTOM]));
		draw_generic_textured_rect(Rect2(0, window_h - black_margin[MARGIN_BOTTOM], window_w, black_margin[MARGIN_BOTTOM]),
				Rect2(0, 0, (float)window_w / sz.x, (float)black_margin[MARGIN_BOTTOM] / sz.y));

	} else if (black_margin[MARGIN_BOTTOM]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);

		draw_generic_textured_rect(Rect2(0, window_h - black_margin[MARGIN_BOTTOM], window_w, black_margin[MARGIN_BOTTOM]), Rect2(0, 0, 1, 1));
	}
}

void RasterizerCanvasGLES3::finalize() {
	glDeleteBuffers(1, &data.canvas_quad_vertices);
	glDeleteVertexArrays(1, &data.canvas_quad_array);

	glDeleteBuffers(1, &data.canvas_quad_vertices);
	glDeleteVertexArrays(1, &data.canvas_quad_array);

	glDeleteVertexArrays(1, &data.polygon_buffer_pointer_array);
}

void RasterizerCanvasGLES3::canvas_render_items_begin(const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform) {
	batch_canvas_render_items_begin(p_modulate, p_light, p_base_transform);
}

void RasterizerCanvasGLES3::canvas_render_items_end() {
	batch_canvas_render_items_end();
}

void RasterizerCanvasGLES3::canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform) {
	batch_canvas_render_items(p_item_list, p_z, p_modulate, p_light, p_base_transform);
}

void RasterizerCanvasGLES3::gl_enable_scissor(int p_x, int p_y, int p_width, int p_height) const {
	glEnable(GL_SCISSOR_TEST);
	glScissor(p_x, p_y, p_width, p_height);
}

void RasterizerCanvasGLES3::gl_disable_scissor() const {
	glDisable(GL_SCISSOR_TEST);
}

// Legacy non-batched implementation for regression testing.
// Should be removed after testing phase to avoid duplicate codepaths.
void RasterizerCanvasGLES3::_legacy_canvas_render_item(Item *p_ci, RenderItemState &r_ris) {
	storage->info.render._2d_item_count++;

	if (r_ris.prev_distance_field != p_ci->distance_field) {
		state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_DISTANCE_FIELD, p_ci->distance_field);
		r_ris.prev_distance_field = p_ci->distance_field;
		r_ris.rebind_shader = true;
	}

	if (r_ris.current_clip != p_ci->final_clip_owner) {
		r_ris.current_clip = p_ci->final_clip_owner;

		//setup clip
		if (r_ris.current_clip) {
			glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
				y = r_ris.current_clip->final_clip_rect.position.y;
			}

			glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.x, r_ris.current_clip->final_clip_rect.size.y);

		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	if (p_ci->copy_back_buffer) {
		if (p_ci->copy_back_buffer->full) {
			_copy_texscreen(Rect2());
		} else {
			_copy_texscreen(p_ci->copy_back_buffer->rect);
		}
	}

	RasterizerStorageGLES3::Skeleton *skeleton = nullptr;

	{
		//skeleton handling
		if (p_ci->skeleton.is_valid() && storage->skeleton_owner.owns(p_ci->skeleton)) {
			skeleton = storage->skeleton_owner.get(p_ci->skeleton);
			if (!skeleton->use_2d) {
				skeleton = nullptr;
			} else {
				state.skeleton_transform = r_ris.item_group_base_transform * skeleton->base_transform_2d;
				state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
			}
		}

		bool use_skeleton = skeleton != nullptr;
		if (r_ris.prev_use_skeleton != use_skeleton) {
			r_ris.rebind_shader = true;
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SKELETON, use_skeleton);
			r_ris.prev_use_skeleton = use_skeleton;
		}

		if (skeleton) {
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
			glBindTexture(GL_TEXTURE_2D, skeleton->texture);
			state.using_skeleton = true;
		} else {
			state.using_skeleton = false;
		}
	}

	//begin rect
	Item *material_owner = p_ci->material_owner ? p_ci->material_owner : p_ci;

	RID material = material_owner->material;

	if (material != r_ris.canvas_last_material || r_ris.rebind_shader) {
		RasterizerStorageGLES3::Material *material_ptr = storage->material_owner.getornull(material);
		RasterizerStorageGLES3::Shader *shader_ptr = nullptr;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = nullptr; //do not use non canvasitem shader
			}
		}

		if (shader_ptr) {
			if (shader_ptr->canvas_item.uses_screen_texture && !state.canvas_texscreen_used) {
				//copy if not copied before
				_copy_texscreen(Rect2());

				// blend mode will have been enabled so make sure we disable it again later on
				r_ris.last_blend_mode = r_ris.last_blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED ? r_ris.last_blend_mode : -1;
			}

			if (shader_ptr != r_ris.shader_cache || r_ris.rebind_shader) {
				if (shader_ptr->canvas_item.uses_time) {
					VisualServerRaster::redraw_request(false);
				}

				state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
				state.canvas_shader.bind();
			}

			if (material_ptr->ubo_id) {
				glBindBufferBase(GL_UNIFORM_BUFFER, 2, material_ptr->ubo_id);
			}

			int tc = material_ptr->textures.size();
			RID *textures = material_ptr->textures.ptrw();
			ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

			for (int i = 0; i < tc; i++) {
				glActiveTexture(GL_TEXTURE2 + i);

				RasterizerStorageGLES3::Texture *t = storage->texture_owner.getornull(textures[i]);
				if (!t) {
					switch (texture_hints[i]) {
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_TRANSPARENT: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.transparent_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
						} break;
						default: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
						} break;
					}

					//check hints

					continue;
				}

				if (t->redraw_if_visible) { //check before proxy, because this is usually used with proxies
					VisualServerRaster::redraw_request();
				}

				t = t->get_ptr();

				if (storage->config.srgb_decode_supported && t->using_srgb) {
					//no srgb in 2D
					glTexParameteri(t->target, _TEXTURE_SRGB_DECODE_EXT, _SKIP_DECODE_EXT);
					t->using_srgb = false;
				}

				glBindTexture(t->target, t->tex_id);
			}

		} else {
			state.canvas_shader.set_custom_shader(0);
			state.canvas_shader.bind();
		}

		r_ris.shader_cache = shader_ptr;

		r_ris.canvas_last_material = material;
		r_ris.rebind_shader = false;
	}

	int blend_mode = r_ris.shader_cache ? r_ris.shader_cache->canvas_item.blend_mode : RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX;
	if (blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED && (!storage->frame.current_rt || !storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])) {
		blend_mode = RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX;
	}
	bool unshaded = r_ris.shader_cache && (r_ris.shader_cache->canvas_item.light_mode == RasterizerStorageGLES3::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA));
	bool reclip = false;

	if (r_ris.last_blend_mode != blend_mode) {
		if (r_ris.last_blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED) {
			// re-enable it
			glEnable(GL_BLEND);
		} else if (blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED) {
			// disable it
			glDisable(GL_BLEND);
		}

		switch (blend_mode) {
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED: {
				// nothing to do here

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_ADD: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_SUB: {
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}
			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MUL: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
				} else {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
		}

		r_ris.last_blend_mode = blend_mode;
	}

	state.canvas_item_modulate = unshaded ? p_ci->final_modulate : Color(p_ci->final_modulate.r * r_ris.item_group_modulate.r, p_ci->final_modulate.g * r_ris.item_group_modulate.g, p_ci->final_modulate.b * r_ris.item_group_modulate.b, p_ci->final_modulate.a * r_ris.item_group_modulate.a);

	state.final_transform = p_ci->final_transform;
	state.extra_matrix = Transform2D();

	if (state.using_skeleton) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM, state.skeleton_transform);
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM_INVERSE, state.skeleton_transform_inverse);
	}

	state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, state.canvas_item_modulate);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, state.extra_matrix);
	if (storage->frame.current_rt) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
	} else {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
	}
	if (unshaded || (state.canvas_item_modulate.a > 0.001 && (!r_ris.shader_cache || r_ris.shader_cache->canvas_item.light_mode != RasterizerStorageGLES3::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !p_ci->light_masked)) {
		_legacy_canvas_item_render_commands(p_ci, r_ris.current_clip, reclip, nullptr);
	}

	if ((blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX || blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA) && r_ris.item_group_light && !unshaded) {
		Light *light = r_ris.item_group_light;
		bool light_used = false;
		VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;
		state.canvas_item_modulate = p_ci->final_modulate; // remove the canvas modulate

		while (light) {
			if (p_ci->light_mask & light->item_mask && r_ris.item_group_z >= light->z_min && r_ris.item_group_z <= light->z_max && p_ci->global_rect_cache.intersects_transformed(light->xform_cache, light->rect_cache)) {
				//intersects this light

				if (!light_used || mode != light->mode) {
					mode = light->mode;

					switch (mode) {
						case VS::CANVAS_LIGHT_MODE_ADD: {
							glBlendEquation(GL_FUNC_ADD);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);

						} break;
						case VS::CANVAS_LIGHT_MODE_SUB: {
							glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						} break;
						case VS::CANVAS_LIGHT_MODE_MIX:
						case VS::CANVAS_LIGHT_MODE_MASK: {
							glBlendEquation(GL_FUNC_ADD);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

						} break;
					}
				}

				if (!light_used) {
					state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_LIGHTING, true);
					light_used = true;
				}

				bool has_shadow = light->shadow_buffer.is_valid() && p_ci->light_mask & light->item_shadow_mask;

				state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SHADOWS, has_shadow);
				if (has_shadow) {
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_USE_GRADIENT, light->shadow_gradient_length > 0);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_NEAREST, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_NONE);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF3, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF3);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF5, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF5);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF7, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF7);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF9, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF9);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF13, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF13);
				}

				bool light_rebind = state.canvas_shader.bind();

				if (light_rebind) {
					state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, state.canvas_item_modulate);
					state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);
					state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, Transform2D());
					if (storage->frame.current_rt) {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
					} else {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
					}
					if (state.using_skeleton) {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM, state.skeleton_transform);
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM_INVERSE, state.skeleton_transform_inverse);
					}
				}

				glBindBufferBase(GL_UNIFORM_BUFFER, 1, static_cast<LightInternal *>(light_internal_owner.get(light->light_internal))->ubo);

				if (has_shadow) {
					RasterizerStorageGLES3::CanvasLightShadow *cls = storage->canvas_light_shadow_owner.get(light->shadow_buffer);
					glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
					glBindTexture(GL_TEXTURE_2D, cls->distance);

					/*canvas_shader.set_uniform(CanvasShaderGLES3::SHADOW_MATRIX,light->shadow_matrix_cache);
					canvas_shader.set_uniform(CanvasShaderGLES3::SHADOW_ESM_MULTIPLIER,light->shadow_esm_mult);
					canvas_shader.set_uniform(CanvasShaderGLES3::LIGHT_SHADOW_COLOR,light->shadow_color);*/
				}

				glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
				RasterizerStorageGLES3::Texture *t = storage->texture_owner.getornull(light->texture);
				if (!t) {
					glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
				} else {
					t = t->get_ptr();

					glBindTexture(t->target, t->tex_id);
				}

				glActiveTexture(GL_TEXTURE0);
				_legacy_canvas_item_render_commands(p_ci, r_ris.current_clip, reclip, nullptr); //redraw using light
			}

			light = light->next_ptr;
		}

		if (light_used) {
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_LIGHTING, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SHADOWS, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_NEAREST, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF3, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF5, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF7, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF9, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF13, false);

			state.canvas_shader.bind();

			r_ris.last_blend_mode = -1;

			/*
			//this is set again, so it should not be needed anyway?
			state.canvas_item_modulate = unshaded ? ci->final_modulate : Color(
						ci->final_modulate.r * p_modulate.r,
						ci->final_modulate.g * p_modulate.g,
						ci->final_modulate.b * p_modulate.b,
						ci->final_modulate.a * p_modulate.a );


			state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX,state.final_transform);
			state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX,Transform2D());
			state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE,state.canvas_item_modulate);

			glBlendEquation(GL_FUNC_ADD);

			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			} else {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}

			//@TODO RESET canvas_blend_mode
			*/
		}
	}

	if (reclip) {
		glEnable(GL_SCISSOR_TEST);
		int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);
		if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
			y = r_ris.current_clip->final_clip_rect.position.y;
		}
		glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
	}
}

void RasterizerCanvasGLES3::render_batches(Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material) {
	//	bdata.reset_flush();
	//	return;

	int num_batches = bdata.batches.size();

	for (int batch_num = 0; batch_num < num_batches; batch_num++) {
		const Batch &batch = bdata.batches[batch_num];

		switch (batch.type) {
			case RasterizerStorageCommon::BT_RECT: {
				_batch_render_generic(batch, p_material);
			} break;
			case RasterizerStorageCommon::BT_POLY: {
				_batch_render_generic(batch, p_material);
			} break;
			case RasterizerStorageCommon::BT_LINE: {
				_batch_render_lines(batch, p_material, false);
			} break;
			case RasterizerStorageCommon::BT_LINE_AA: {
				_batch_render_lines(batch, p_material, true);
			} break;
			default: {
				int end_command = batch.first_command + batch.num_commands;

				DEV_ASSERT(batch.item);
				RasterizerCanvas::Item::Command *const *commands = batch.item->commands.ptr();

				for (int i = batch.first_command; i < end_command; i++) {
					Item::Command *c = commands[i];

					switch (c->type) {
						case Item::Command::TYPE_LINE: {
							Item::CommandLine *line = static_cast<Item::CommandLine *>(c);
							_set_texture_rect_mode(false);

							_bind_canvas_texture(RID(), RID());

							glVertexAttrib4f(VS::ARRAY_COLOR, line->color.r, line->color.g, line->color.b, line->color.a);

							if (line->width <= 1) {
								Vector2 verts[2] = {
									Vector2(line->from.x, line->from.y),
									Vector2(line->to.x, line->to.y)
								};

#ifdef GLES_OVER_GL
								if (line->antialiased) {
									glEnable(GL_LINE_SMOOTH);
								}
#endif
								//glLineWidth(line->width);
								_draw_gui_primitive(2, verts, nullptr, nullptr);

#ifdef GLES_OVER_GL
								if (line->antialiased) {
									glDisable(GL_LINE_SMOOTH);
								}
#endif
							} else {
								//thicker line

								Vector2 t = (line->from - line->to).normalized().tangent() * line->width * 0.5;

								Vector2 verts[4] = {
									line->from - t,
									line->from + t,
									line->to + t,
									line->to - t,
								};

								//glLineWidth(line->width);
								_draw_gui_primitive(4, verts, nullptr, nullptr);
#ifdef GLES_OVER_GL
								if (line->antialiased) {
									glEnable(GL_LINE_SMOOTH);
									for (int j = 0; j < 4; j++) {
										Vector2 vertsl[2] = {
											verts[j],
											verts[(j + 1) % 4],
										};
										_draw_gui_primitive(2, vertsl, nullptr, nullptr);
									}
									glDisable(GL_LINE_SMOOTH);
								}
#endif
							}
						} break;
						case Item::Command::TYPE_POLYLINE: {
							Item::CommandPolyLine *pline = static_cast<Item::CommandPolyLine *>(c);
							_set_texture_rect_mode(false);

							_bind_canvas_texture(RID(), RID());

							if (pline->triangles.size()) {
#ifdef RASTERIZER_EXTRA_CHECKS
								if (pline->triangle_colors.ptr() && (pline->triangle_colors.size() != 1)) {
									RAST_DEV_DEBUG_ASSERT(pline->triangle_colors.size() == pline->triangles.size());
								}
#endif
								_draw_generic(GL_TRIANGLE_STRIP, pline->triangles.size(), pline->triangles.ptr(), nullptr, pline->triangle_colors.ptr(), pline->triangle_colors.size() == 1);
#ifdef GLES_OVER_GL
								glEnable(GL_LINE_SMOOTH);
								if (pline->multiline) {
									//needs to be different
								} else {
									_draw_generic(GL_LINE_LOOP, pline->lines.size(), pline->lines.ptr(), nullptr, pline->line_colors.ptr(), pline->line_colors.size() == 1);
								}
								glDisable(GL_LINE_SMOOTH);
#endif
							} else {
#ifdef GLES_OVER_GL
								if (pline->antialiased) {
									glEnable(GL_LINE_SMOOTH);
								}
#endif

								if (pline->multiline) {
									int todo = pline->lines.size() / 2;
									int max_per_call = data.polygon_buffer_size / (sizeof(real_t) * 4);
									int offset = 0;

									while (todo) {
										int to_draw = MIN(max_per_call, todo);
										_draw_generic(GL_LINES, to_draw * 2, &pline->lines.ptr()[offset], nullptr, pline->line_colors.size() == 1 ? pline->line_colors.ptr() : &pline->line_colors.ptr()[offset], pline->line_colors.size() == 1);
										todo -= to_draw;
										offset += to_draw * 2;
									}

								} else {
									_draw_generic(GL_LINE_STRIP, pline->lines.size(), pline->lines.ptr(), nullptr, pline->line_colors.ptr(), pline->line_colors.size() == 1);
								}

#ifdef GLES_OVER_GL
								if (pline->antialiased) {
									glDisable(GL_LINE_SMOOTH);
								}
#endif
							}

						} break;
						case Item::Command::TYPE_RECT: {
							Item::CommandRect *rect = static_cast<Item::CommandRect *>(c);

							//set color
							glVertexAttrib4f(VS::ARRAY_COLOR, rect->modulate.r, rect->modulate.g, rect->modulate.b, rect->modulate.a);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(rect->texture, rect->normal_map);

							if (use_nvidia_rect_workaround) {
								render_rect_nvidia_workaround(rect, texture);
							} else {
								_set_texture_rect_mode(true);

								if (texture) {
									bool untile = false;

									if (rect->flags & CANVAS_RECT_TILE && !(texture->flags & VS::TEXTURE_FLAG_REPEAT)) {
										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
										untile = true;
									}

									Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
									Rect2 src_rect = (rect->flags & CANVAS_RECT_REGION) ? Rect2(rect->source.position * texpixel_size, rect->source.size * texpixel_size) : Rect2(0, 0, 1, 1);
									Rect2 dst_rect = Rect2(rect->rect.position, rect->rect.size);

									if (dst_rect.size.width < 0) {
										dst_rect.position.x += dst_rect.size.width;
										dst_rect.size.width *= -1;
									}
									if (dst_rect.size.height < 0) {
										dst_rect.position.y += dst_rect.size.height;
										dst_rect.size.height *= -1;
									}

									if (rect->flags & CANVAS_RECT_FLIP_H) {
										src_rect.size.x *= -1;
									}

									if (rect->flags & CANVAS_RECT_FLIP_V) {
										src_rect.size.y *= -1;
									}

									if (rect->flags & CANVAS_RECT_TRANSPOSE) {
										dst_rect.size.x *= -1; // Encoding in the dst_rect.z uniform
									}

									state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);

									state.canvas_shader.set_uniform(CanvasShaderGLES3::DST_RECT, Color(dst_rect.position.x, dst_rect.position.y, dst_rect.size.x, dst_rect.size.y));
									state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(src_rect.position.x, src_rect.position.y, src_rect.size.x, src_rect.size.y));
									state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, rect->flags & CANVAS_RECT_CLIP_UV);

									glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									storage->info.render._2d_draw_call_count++;

									if (untile) {
										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
										glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
									}

								} else {
									Rect2 dst_rect = Rect2(rect->rect.position, rect->rect.size);

									if (dst_rect.size.width < 0) {
										dst_rect.position.x += dst_rect.size.width;
										dst_rect.size.width *= -1;
									}
									if (dst_rect.size.height < 0) {
										dst_rect.position.y += dst_rect.size.height;
										dst_rect.size.height *= -1;
									}

									state.canvas_shader.set_uniform(CanvasShaderGLES3::DST_RECT, Color(dst_rect.position.x, dst_rect.position.y, dst_rect.size.x, dst_rect.size.y));
									state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(0, 0, 1, 1));
									state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);
									glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
									storage->info.render._2d_draw_call_count++;
								}
							} // if not use nvidia workaround
						} break;
						case Item::Command::TYPE_NINEPATCH: {
							Item::CommandNinePatch *np = static_cast<Item::CommandNinePatch *>(c);

							_set_texture_rect_mode(true, true);

							glVertexAttrib4f(VS::ARRAY_COLOR, np->color.r, np->color.g, np->color.b, np->color.a);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(np->texture, np->normal_map);

							Size2 texpixel_size;

							if (!texture) {
								texpixel_size = Size2(1, 1);

								state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(0, 0, 1, 1));

							} else {
								if (np->source != Rect2()) {
									texpixel_size = Size2(1.0 / np->source.size.width, 1.0 / np->source.size.height);
									state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(np->source.position.x / texture->width, np->source.position.y / texture->height, np->source.size.x / texture->width, np->source.size.y / texture->height));
								} else {
									texpixel_size = Size2(1.0 / texture->width, 1.0 / texture->height);
									state.canvas_shader.set_uniform(CanvasShaderGLES3::SRC_RECT, Color(0, 0, 1, 1));
								}
							}

							state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);
							state.canvas_shader.set_uniform(CanvasShaderGLES3::NP_REPEAT_H, int(np->axis_x));
							state.canvas_shader.set_uniform(CanvasShaderGLES3::NP_REPEAT_V, int(np->axis_y));
							state.canvas_shader.set_uniform(CanvasShaderGLES3::NP_DRAW_CENTER, np->draw_center);
							state.canvas_shader.set_uniform(CanvasShaderGLES3::NP_MARGINS, Color(np->margin[MARGIN_LEFT], np->margin[MARGIN_TOP], np->margin[MARGIN_RIGHT], np->margin[MARGIN_BOTTOM]));
							state.canvas_shader.set_uniform(CanvasShaderGLES3::DST_RECT, Color(np->rect.position.x, np->rect.position.y, np->rect.size.x, np->rect.size.y));

							glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

							storage->info.render._2d_draw_call_count++;
						} break;
						case Item::Command::TYPE_PRIMITIVE: {
							Item::CommandPrimitive *primitive = static_cast<Item::CommandPrimitive *>(c);
							_set_texture_rect_mode(false);

							ERR_CONTINUE(primitive->points.size() < 1);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(primitive->texture, primitive->normal_map);

							if (texture) {
								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							}

							// we need a temporary because this must be nulled out
							// if only a single color specified
							const Color *colors = primitive->colors.ptr();
							if (primitive->colors.size() == 1 && primitive->points.size() > 1) {
								Color col = primitive->colors[0];
								glVertexAttrib4f(VS::ARRAY_COLOR, col.r, col.g, col.b, col.a);
								colors = nullptr;

							} else if (primitive->colors.empty()) {
								glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1);
							}
#ifdef RASTERIZER_EXTRA_CHECKS
							else {
								RAST_DEV_DEBUG_ASSERT(primitive->colors.size() == primitive->points.size());
							}

							if (primitive->uvs.ptr()) {
								RAST_DEV_DEBUG_ASSERT(primitive->uvs.size() == primitive->points.size());
							}
#endif

							_draw_gui_primitive(primitive->points.size(), primitive->points.ptr(), colors, primitive->uvs.ptr());

						} break;
						case Item::Command::TYPE_POLYGON: {
							Item::CommandPolygon *polygon = static_cast<Item::CommandPolygon *>(c);
							_set_texture_rect_mode(false);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(polygon->texture, polygon->normal_map);

							if (texture) {
								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							}

							_draw_polygon(polygon->indices.ptr(), polygon->count, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1, polygon->bones.ptr(), polygon->weights.ptr());
#ifdef GLES_OVER_GL
							if (polygon->antialiased) {
								glEnable(GL_LINE_SMOOTH);
								if (polygon->antialiasing_use_indices) {
									_draw_generic_indices(GL_LINE_STRIP, polygon->indices.ptr(), polygon->count, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1);
								} else {
									_draw_generic(GL_LINE_LOOP, polygon->points.size(), polygon->points.ptr(), polygon->uvs.ptr(), polygon->colors.ptr(), polygon->colors.size() == 1);
								}
								glDisable(GL_LINE_SMOOTH);
							}
#endif

						} break;
						case Item::Command::TYPE_MESH: {
							Item::CommandMesh *mesh = static_cast<Item::CommandMesh *>(c);
							_set_texture_rect_mode(false);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(mesh->texture, mesh->normal_map);

							if (texture) {
								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							}

							state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform * mesh->transform);

							RasterizerStorageGLES3::Mesh *mesh_data = storage->mesh_owner.getornull(mesh->mesh);
							if (mesh_data) {
								for (int j = 0; j < mesh_data->surfaces.size(); j++) {
									RasterizerStorageGLES3::Surface *s = mesh_data->surfaces[j];
									// materials are ignored in 2D meshes, could be added but many things (ie, lighting mode, reading from screen, etc) would break as they are not meant be set up at this point of drawing
									glBindVertexArray(s->array_id);

									glVertexAttrib4f(VS::ARRAY_COLOR, mesh->modulate.r, mesh->modulate.g, mesh->modulate.b, mesh->modulate.a);

									if (s->index_array_len) {
										glDrawElements(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, nullptr);
									} else {
										glDrawArrays(gl_primitive[s->primitive], 0, s->array_len);
									}
									storage->info.render._2d_draw_call_count++;

									glBindVertexArray(0);
								}
							}
							state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);

						} break;
						case Item::Command::TYPE_MULTIMESH: {
							Item::CommandMultiMesh *mmesh = static_cast<Item::CommandMultiMesh *>(c);

							RasterizerStorageGLES3::MultiMesh *multi_mesh = storage->multimesh_owner.getornull(mmesh->multimesh);

							if (!multi_mesh) {
								break;
							}

							RasterizerStorageGLES3::Mesh *mesh_data = storage->mesh_owner.getornull(multi_mesh->mesh);

							if (!mesh_data) {
								break;
							}

							int amount = MIN(multi_mesh->size, multi_mesh->visible_instances);

							if (amount == -1) {
								amount = multi_mesh->size;
							}

							if (!amount) {
								break;
							}

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(mmesh->texture, mmesh->normal_map);

							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCE_CUSTOM, multi_mesh->custom_data_format != VS::MULTIMESH_CUSTOM_DATA_NONE);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCING, true);
							//reset shader and force rebind
							state.using_texture_rect = true;
							_set_texture_rect_mode(false);

							if (texture) {
								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							}

							glVertexAttrib4f(VS::ARRAY_COLOR, 1.0, 1.0, 1.0, 1.0);

							for (int j = 0; j < mesh_data->surfaces.size(); j++) {
								RasterizerStorageGLES3::Surface *s = mesh_data->surfaces[j];
								// materials are ignored in 2D meshes, could be added but many things (ie, lighting mode, reading from screen, etc) would break as they are not meant be set up at this point of drawing
								glBindVertexArray(s->instancing_array_id);

								glBindBuffer(GL_ARRAY_BUFFER, multi_mesh->buffer); //modify the buffer

								int stride = (multi_mesh->xform_floats + multi_mesh->color_floats + multi_mesh->custom_data_floats) * 4;
								glEnableVertexAttribArray(8);
								glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(0));
								glVertexAttribDivisor(8, 1);
								glEnableVertexAttribArray(9);
								glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(4 * 4));
								glVertexAttribDivisor(9, 1);

								int color_ofs;

								if (multi_mesh->transform_format == VS::MULTIMESH_TRANSFORM_3D) {
									glEnableVertexAttribArray(10);
									glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(8 * 4));
									glVertexAttribDivisor(10, 1);
									color_ofs = 12 * 4;
								} else {
									glDisableVertexAttribArray(10);
									glVertexAttrib4f(10, 0, 0, 1, 0);
									color_ofs = 8 * 4;
								}

								int custom_data_ofs = color_ofs;

								switch (multi_mesh->color_format) {
									case VS::MULTIMESH_COLOR_MAX:
									case VS::MULTIMESH_COLOR_NONE: {
										glDisableVertexAttribArray(11);
										glVertexAttrib4f(11, 1, 1, 1, 1);
									} break;
									case VS::MULTIMESH_COLOR_8BIT: {
										glEnableVertexAttribArray(11);
										glVertexAttribPointer(11, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, CAST_INT_TO_UCHAR_PTR(color_ofs));
										glVertexAttribDivisor(11, 1);
										custom_data_ofs += 4;

									} break;
									case VS::MULTIMESH_COLOR_FLOAT: {
										glEnableVertexAttribArray(11);
										glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(color_ofs));
										glVertexAttribDivisor(11, 1);
										custom_data_ofs += 4 * 4;
									} break;
								}

								switch (multi_mesh->custom_data_format) {
									case VS::MULTIMESH_CUSTOM_DATA_MAX:
									case VS::MULTIMESH_CUSTOM_DATA_NONE: {
										glDisableVertexAttribArray(12);
										glVertexAttrib4f(12, 1, 1, 1, 1);
									} break;
									case VS::MULTIMESH_CUSTOM_DATA_8BIT: {
										glEnableVertexAttribArray(12);
										glVertexAttribPointer(12, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, CAST_INT_TO_UCHAR_PTR(custom_data_ofs));
										glVertexAttribDivisor(12, 1);

									} break;
									case VS::MULTIMESH_CUSTOM_DATA_FLOAT: {
										glEnableVertexAttribArray(12);
										glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(custom_data_ofs));
										glVertexAttribDivisor(12, 1);
									} break;
								}

								if (s->index_array_len) {
									glDrawElementsInstanced(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, nullptr, amount);
								} else {
									glDrawArraysInstanced(gl_primitive[s->primitive], 0, s->array_len, amount);
								}
								storage->info.render._2d_draw_call_count++;

								glBindVertexArray(0);
							}

							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCE_CUSTOM, false);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCING, false);
							state.using_texture_rect = true;
							_set_texture_rect_mode(false);

						} break;
						case Item::Command::TYPE_PARTICLES: {
							Item::CommandParticles *particles_cmd = static_cast<Item::CommandParticles *>(c);

							RasterizerStorageGLES3::Particles *particles = storage->particles_owner.getornull(particles_cmd->particles);
							if (!particles) {
								break;
							}

							if (particles->inactive && !particles->emitting) {
								break;
							}

							glVertexAttrib4f(VS::ARRAY_COLOR, 1, 1, 1, 1); //not used, so keep white

							VisualServerRaster::redraw_request(false);

							storage->particles_request_process(particles_cmd->particles);
							//enable instancing

							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCE_CUSTOM, true);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_PARTICLES, true);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCING, true);
							//reset shader and force rebind
							state.using_texture_rect = true;
							_set_texture_rect_mode(false);

							RasterizerStorageGLES3::Texture *texture = _bind_canvas_texture(particles_cmd->texture, particles_cmd->normal_map);

							if (texture) {
								Size2 texpixel_size(1.0 / texture->width, 1.0 / texture->height);
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, texpixel_size);
							} else {
								state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, Vector2(1.0, 1.0));
							}

							if (!particles->use_local_coords) {
								Transform2D inv_xf;
								inv_xf.set_axis(0, Vector2(particles->emission_transform.basis.get_axis(0).x, particles->emission_transform.basis.get_axis(0).y));
								inv_xf.set_axis(1, Vector2(particles->emission_transform.basis.get_axis(1).x, particles->emission_transform.basis.get_axis(1).y));
								inv_xf.set_origin(Vector2(particles->emission_transform.get_origin().x, particles->emission_transform.get_origin().y));
								inv_xf.affine_invert();

								state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform * inv_xf);
							}

							glBindVertexArray(data.particle_quad_array); //use particle quad array
							glBindBuffer(GL_ARRAY_BUFFER, particles->particle_buffers[0]); //bind particle buffer

							int stride = sizeof(float) * 4 * 6;

							int amount = particles->amount;

							if (particles->draw_order != ParticlesData::DRAW_ORDER_LIFETIME) {
								glEnableVertexAttribArray(8); //xform x
								glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 3));
								glVertexAttribDivisor(8, 1);
								glEnableVertexAttribArray(9); //xform y
								glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 4));
								glVertexAttribDivisor(9, 1);
								glEnableVertexAttribArray(10); //xform z
								glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 5));
								glVertexAttribDivisor(10, 1);
								glEnableVertexAttribArray(11); //color
								glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
								glVertexAttribDivisor(11, 1);
								glEnableVertexAttribArray(12); //custom
								glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 2));
								glVertexAttribDivisor(12, 1);

								glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, amount);
								storage->info.render._2d_draw_call_count++;
							} else {
								//split
								int split = int(Math::ceil(particles->phase * particles->amount));

								if (amount - split > 0) {
									glEnableVertexAttribArray(8); //xform x
									glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(stride * split + sizeof(float) * 4 * 3));
									glVertexAttribDivisor(8, 1);
									glEnableVertexAttribArray(9); //xform y
									glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(stride * split + sizeof(float) * 4 * 4));
									glVertexAttribDivisor(9, 1);
									glEnableVertexAttribArray(10); //xform z
									glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(stride * split + sizeof(float) * 4 * 5));
									glVertexAttribDivisor(10, 1);
									glEnableVertexAttribArray(11); //color
									glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(stride * split + 0));
									glVertexAttribDivisor(11, 1);
									glEnableVertexAttribArray(12); //custom
									glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(stride * split + sizeof(float) * 4 * 2));
									glVertexAttribDivisor(12, 1);

									glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, amount - split);
									storage->info.render._2d_draw_call_count++;
								}

								if (split > 0) {
									glEnableVertexAttribArray(8); //xform x
									glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 3));
									glVertexAttribDivisor(8, 1);
									glEnableVertexAttribArray(9); //xform y
									glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 4));
									glVertexAttribDivisor(9, 1);
									glEnableVertexAttribArray(10); //xform z
									glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 5));
									glVertexAttribDivisor(10, 1);
									glEnableVertexAttribArray(11); //color
									glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
									glVertexAttribDivisor(11, 1);
									glEnableVertexAttribArray(12); //custom
									glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(sizeof(float) * 4 * 2));
									glVertexAttribDivisor(12, 1);

									glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, split);
									storage->info.render._2d_draw_call_count++;
								}
							}

							glBindVertexArray(0);

							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCE_CUSTOM, false);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_PARTICLES, false);
							state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_INSTANCING, false);
							state.using_texture_rect = true;
							_set_texture_rect_mode(false);

						} break;
						case Item::Command::TYPE_CIRCLE: {
							_set_texture_rect_mode(false);

							Item::CommandCircle *circle = static_cast<Item::CommandCircle *>(c);
							static const int numpoints = 32;
							Vector2 points[numpoints + 1];
							points[numpoints] = circle->pos;
							int indices[numpoints * 3];

							for (int j = 0; j < numpoints; j++) {
								points[j] = circle->pos + Vector2(Math::sin(j * Math_PI * 2.0 / numpoints), Math::cos(j * Math_PI * 2.0 / numpoints)) * circle->radius;
								indices[j * 3 + 0] = j;
								indices[j * 3 + 1] = (j + 1) % numpoints;
								indices[j * 3 + 2] = numpoints;
							}

							_bind_canvas_texture(RID(), RID());
							_draw_polygon(indices, numpoints * 3, numpoints + 1, points, nullptr, &circle->color, true, nullptr, nullptr);

							//_draw_polygon(numpoints*3,indices,points,NULL,&circle->color,RID(),true);
							//canvas_draw_circle(circle->indices.size(),circle->indices.ptr(),circle->points.ptr(),circle->uvs.ptr(),circle->colors.ptr(),circle->texture,circle->colors.size()==1);
						} break;
						case Item::Command::TYPE_TRANSFORM: {
							Item::CommandTransform *transform = static_cast<Item::CommandTransform *>(c);
							state.extra_matrix = transform->xform;
							state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, state.extra_matrix);

						} break;
						case Item::Command::TYPE_CLIP_IGNORE: {
							Item::CommandClipIgnore *ci = static_cast<Item::CommandClipIgnore *>(c);
							if (p_current_clip) {
								if (ci->ignore != r_reclip) {
									if (ci->ignore) {
										glDisable(GL_SCISSOR_TEST);
										r_reclip = true;
									} else {
										glEnable(GL_SCISSOR_TEST);
										//glScissor(viewport.x+current_clip->final_clip_rect.pos.x,viewport.y+ (viewport.height-(current_clip->final_clip_rect.pos.y+current_clip->final_clip_rect.size.height)),
										//current_clip->final_clip_rect.size.width,current_clip->final_clip_rect.size.height);
										int y = storage->frame.current_rt->height - (p_current_clip->final_clip_rect.position.y + p_current_clip->final_clip_rect.size.y);
										if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
											y = p_current_clip->final_clip_rect.position.y;
										}

										glScissor(p_current_clip->final_clip_rect.position.x, y, p_current_clip->final_clip_rect.size.x, p_current_clip->final_clip_rect.size.y);

										r_reclip = false;
									}
								}
							}

						} break;

						default: {
							// FIXME: Proper error handling if relevant
							//print_line("other");
						} break;
					}
				}

			} // default
			break;
		}
	}
}

void RasterizerCanvasGLES3::render_joined_item(const BItemJoined &p_bij, RenderItemState &r_ris) {
	storage->info.render._2d_item_count++;

	// all the joined items will share the same state with the first item
	Item *p_ci = bdata.item_refs[p_bij.first_item_ref].item;

	if (r_ris.prev_distance_field != p_ci->distance_field) {
		state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_DISTANCE_FIELD, p_ci->distance_field);
		r_ris.prev_distance_field = p_ci->distance_field;
		r_ris.rebind_shader = true;
	}

	if (r_ris.current_clip != p_ci->final_clip_owner) {
		r_ris.current_clip = p_ci->final_clip_owner;

		//setup clip
		if (r_ris.current_clip) {
			glEnable(GL_SCISSOR_TEST);
			int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);
			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
				y = r_ris.current_clip->final_clip_rect.position.y;
			}

			glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.x, r_ris.current_clip->final_clip_rect.size.y);

		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}

	if (p_ci->copy_back_buffer) {
		if (p_ci->copy_back_buffer->full) {
			_copy_texscreen(Rect2());
		} else {
			_copy_texscreen(p_ci->copy_back_buffer->rect);
		}
	}

	if (!bdata.settings_use_batching || !bdata.settings_use_software_skinning) {
		RasterizerStorageGLES3::Skeleton *skeleton = nullptr;

		//skeleton handling
		if (p_ci->skeleton.is_valid() && storage->skeleton_owner.owns(p_ci->skeleton)) {
			skeleton = storage->skeleton_owner.get(p_ci->skeleton);
			if (!skeleton->use_2d) {
				skeleton = nullptr;
			} else {
				state.skeleton_transform = r_ris.item_group_base_transform * skeleton->base_transform_2d;
				state.skeleton_transform_inverse = state.skeleton_transform.affine_inverse();
			}
		}

		bool use_skeleton = skeleton != nullptr;
		if (r_ris.prev_use_skeleton != use_skeleton) {
			r_ris.rebind_shader = true;
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SKELETON, use_skeleton);
			r_ris.prev_use_skeleton = use_skeleton;
		}

		if (skeleton) {
			glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
			glBindTexture(GL_TEXTURE_2D, skeleton->texture);
			state.using_skeleton = true;
		} else {
			state.using_skeleton = false;
		}

	} // if not using batching

	//begin rect
	Item *material_owner = p_ci->material_owner ? p_ci->material_owner : p_ci;

	RID material = material_owner->material;

	if (material != r_ris.canvas_last_material || r_ris.rebind_shader) {
		RasterizerStorageGLES3::Material *material_ptr = storage->material_owner.getornull(material);
		RasterizerStorageGLES3::Shader *shader_ptr = nullptr;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = nullptr; //do not use non canvasitem shader
			}
		}

		if (shader_ptr) {
			if (shader_ptr->canvas_item.uses_screen_texture && !state.canvas_texscreen_used) {
				//copy if not copied before
				_copy_texscreen(Rect2());

				// blend mode will have been enabled so make sure we disable it again later on
				r_ris.last_blend_mode = r_ris.last_blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED ? r_ris.last_blend_mode : -1;
			}

			if (shader_ptr != r_ris.shader_cache || r_ris.rebind_shader) {
				if (shader_ptr->canvas_item.uses_time) {
					VisualServerRaster::redraw_request(false);
				}

				state.canvas_shader.set_custom_shader(shader_ptr->custom_code_id);
				state.canvas_shader.bind();
			}

			if (material_ptr->ubo_id) {
				glBindBufferBase(GL_UNIFORM_BUFFER, 2, material_ptr->ubo_id);
			}

			int tc = material_ptr->textures.size();
			RID *textures = material_ptr->textures.ptrw();
			ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = shader_ptr->texture_hints.ptrw();

			for (int i = 0; i < tc; i++) {
				glActiveTexture(GL_TEXTURE2 + i);

				RasterizerStorageGLES3::Texture *t = storage->texture_owner.getornull(textures[i]);
				if (!t) {
					switch (texture_hints[i]) {
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
						case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_TRANSPARENT: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.transparent_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
						} break;
						case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
						} break;
						default: {
							glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
						} break;
					}

					//check hints

					continue;
				}

				if (t->redraw_if_visible) { //check before proxy, because this is usually used with proxies
					VisualServerRaster::redraw_request(false);
				}

				t = t->get_ptr();

				if (storage->config.srgb_decode_supported && t->using_srgb) {
					//no srgb in 2D
					glTexParameteri(t->target, _TEXTURE_SRGB_DECODE_EXT, _SKIP_DECODE_EXT);
					t->using_srgb = false;
				}

				glBindTexture(t->target, t->tex_id);
			}

		} else {
			state.canvas_shader.set_custom_shader(0);
			state.canvas_shader.bind();
		}

		r_ris.shader_cache = shader_ptr;

		r_ris.canvas_last_material = material;
		r_ris.rebind_shader = false;
	}

	int blend_mode = r_ris.shader_cache ? r_ris.shader_cache->canvas_item.blend_mode : RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX;
	if (blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED && (!storage->frame.current_rt || !storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT])) {
		blend_mode = RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX;
	}
	bool unshaded = r_ris.shader_cache && (r_ris.shader_cache->canvas_item.light_mode == RasterizerStorageGLES3::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA));
	bool reclip = false;

	if (r_ris.last_blend_mode != blend_mode) {
		if (r_ris.last_blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED) {
			// re-enable it
			glEnable(GL_BLEND);
		} else if (blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED) {
			// disable it
			glDisable(GL_BLEND);
		}

		switch (blend_mode) {
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_DISABLED: {
				// nothing to do here

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_ADD: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_SUB: {
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);
				}
			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MUL: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
				} else {
					glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
				}

			} break;
			case RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA: {
				glBlendEquation(GL_FUNC_ADD);
				if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				} else {
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
				}

			} break;
		}

		r_ris.last_blend_mode = blend_mode;
	}

	//state.canvas_item_modulate = unshaded ? p_ci->final_modulate : Color(p_ci->final_modulate.r * r_ris.item_group_modulate.r, p_ci->final_modulate.g * r_ris.item_group_modulate.g, p_ci->final_modulate.b * r_ris.item_group_modulate.b, p_ci->final_modulate.a * r_ris.item_group_modulate.a);

	//	state.final_transform = p_ci->final_transform;
	//	state.extra_matrix = Transform2D();

	// using software transform?
	// (i.e. don't send the transform matrix, send identity, and either use baked verts,
	// or large fvf where the transform is done in the shader from transform stored in the fvf.)
	if (!p_bij.is_single_item()) {
		state.final_transform = Transform2D();
		// final_modulate will be baked per item ref so the final_modulate can be an identity color
		state.canvas_item_modulate = Color(1, 1, 1, 1);
	} else {
		state.final_transform = p_ci->final_transform;
		// could use the stored version of final_modulate in item ref? Test which is faster NYI
		state.canvas_item_modulate = unshaded ? p_ci->final_modulate : (p_ci->final_modulate * r_ris.item_group_modulate);
	}
	state.extra_matrix = Transform2D();

	if (state.using_skeleton) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM, state.skeleton_transform);
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM_INVERSE, state.skeleton_transform_inverse);
	}

	state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, state.canvas_item_modulate);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, state.extra_matrix);
	if (storage->frame.current_rt) {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
	} else {
		state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
	}
	if (unshaded || (state.canvas_item_modulate.a > 0.001 && (!r_ris.shader_cache || r_ris.shader_cache->canvas_item.light_mode != RasterizerStorageGLES3::Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY) && !p_ci->light_masked)) {
		RasterizerStorageGLES3::Material *material_ptr = nullptr;
		render_joined_item_commands(p_bij, nullptr, reclip, material_ptr, false, r_ris);
	}

	if ((blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX || blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA) && r_ris.item_group_light && !unshaded) {
		Light *light = r_ris.item_group_light;
		bool light_used = false;
		VS::CanvasLightMode mode = VS::CANVAS_LIGHT_MODE_ADD;

		// we leave this set to 1, 1, 1, 1 if using software because the colors are baked into the vertices
		if (p_bij.is_single_item()) {
			state.canvas_item_modulate = p_ci->final_modulate; // remove the canvas modulate
		}

		while (light) {
			// use the bounding rect of the joined items, NOT only the bounding rect of the first item.
			// note this is a cost of batching, the light culling will be less effective

			// note that the r_ris.item_group_z will be out of date because we are using deferred rendering till canvas_render_items_end()
			// so we have to test z against the stored value in the joined item
			if (p_ci->light_mask & light->item_mask && p_bij.z_index >= light->z_min && p_bij.z_index <= light->z_max && p_bij.bounding_rect.intersects_transformed(light->xform_cache, light->rect_cache)) {
				//intersects this light

				if (!light_used || mode != light->mode) {
					mode = light->mode;

					switch (mode) {
						case VS::CANVAS_LIGHT_MODE_ADD: {
							glBlendEquation(GL_FUNC_ADD);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);

						} break;
						case VS::CANVAS_LIGHT_MODE_SUB: {
							glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE);
						} break;
						case VS::CANVAS_LIGHT_MODE_MIX:
						case VS::CANVAS_LIGHT_MODE_MASK: {
							glBlendEquation(GL_FUNC_ADD);
							glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

						} break;
					}
				}

				if (!light_used) {
					state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_LIGHTING, true);
					light_used = true;
				}

				bool has_shadow = light->shadow_buffer.is_valid() && p_ci->light_mask & light->item_shadow_mask;

				state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SHADOWS, has_shadow);
				if (has_shadow) {
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_USE_GRADIENT, light->shadow_gradient_length > 0);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_NEAREST, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_NONE);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF3, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF3);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF5, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF5);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF7, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF7);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF9, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF9);
					state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF13, light->shadow_filter == VS::CANVAS_LIGHT_FILTER_PCF13);
				}

				bool light_rebind = state.canvas_shader.bind();

				if (light_rebind) {
					state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE, state.canvas_item_modulate);
					state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX, state.final_transform);
					state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX, Transform2D());
					if (storage->frame.current_rt) {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
					} else {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SCREEN_PIXEL_SIZE, Vector2(1.0, 1.0));
					}
					if (state.using_skeleton) {
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM, state.skeleton_transform);
						state.canvas_shader.set_uniform(CanvasShaderGLES3::SKELETON_TRANSFORM_INVERSE, state.skeleton_transform_inverse);
					}
				}

				glBindBufferBase(GL_UNIFORM_BUFFER, 1, static_cast<LightInternal *>(light_internal_owner.get(light->light_internal))->ubo);

				if (has_shadow) {
					RasterizerStorageGLES3::CanvasLightShadow *cls = storage->canvas_light_shadow_owner.get(light->shadow_buffer);
					glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 2);
					glBindTexture(GL_TEXTURE_2D, cls->distance);

					/*canvas_shader.set_uniform(CanvasShaderGLES3::SHADOW_MATRIX,light->shadow_matrix_cache);
					canvas_shader.set_uniform(CanvasShaderGLES3::SHADOW_ESM_MULTIPLIER,light->shadow_esm_mult);
					canvas_shader.set_uniform(CanvasShaderGLES3::LIGHT_SHADOW_COLOR,light->shadow_color);*/
				}

				glActiveTexture(GL_TEXTURE0 + storage->config.max_texture_image_units - 1);
				RasterizerStorageGLES3::Texture *t = storage->texture_owner.getornull(light->texture);
				if (!t) {
					glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
				} else {
					t = t->get_ptr();

					glBindTexture(t->target, t->tex_id);
				}

				glActiveTexture(GL_TEXTURE0);

				// redraw using light.
				// if there is no clip item, we can consider scissoring to the intersection area between the light and the item
				// this can greatly reduce fill rate ..
				// at the cost of glScissor commands, so is optional
				if (!bdata.settings_scissor_lights || r_ris.current_clip) {
					render_joined_item_commands(p_bij, nullptr, reclip, nullptr, true, r_ris);
				} else {
					bool scissor = _light_scissor_begin(p_bij.bounding_rect, light->xform_cache, light->rect_cache);
					render_joined_item_commands(p_bij, nullptr, reclip, nullptr, true, r_ris);
					if (scissor) {
						glDisable(GL_SCISSOR_TEST);
					}
				}
			}

			light = light->next_ptr;
		}

		if (light_used) {
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_LIGHTING, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SHADOWS, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_NEAREST, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF3, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF5, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF7, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF9, false);
			state.canvas_shader.set_conditional(CanvasShaderGLES3::SHADOW_FILTER_PCF13, false);

			state.canvas_shader.bind();

			r_ris.last_blend_mode = -1;

			/*
			//this is set again, so it should not be needed anyway?
			state.canvas_item_modulate = unshaded ? ci->final_modulate : Color(
						ci->final_modulate.r * p_modulate.r,
						ci->final_modulate.g * p_modulate.g,
						ci->final_modulate.b * p_modulate.b,
						ci->final_modulate.a * p_modulate.a );


			state.canvas_shader.set_uniform(CanvasShaderGLES3::MODELVIEW_MATRIX,state.final_transform);
			state.canvas_shader.set_uniform(CanvasShaderGLES3::EXTRA_MATRIX,Transform2D());
			state.canvas_shader.set_uniform(CanvasShaderGLES3::FINAL_MODULATE,state.canvas_item_modulate);

			glBlendEquation(GL_FUNC_ADD);

			if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			} else {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}

			//@TODO RESET canvas_blend_mode
			*/
		}
	}

	if (reclip) {
		glEnable(GL_SCISSOR_TEST);
		int y = storage->frame.current_rt->height - (r_ris.current_clip->final_clip_rect.position.y + r_ris.current_clip->final_clip_rect.size.y);
		if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
			y = r_ris.current_clip->final_clip_rect.position.y;
		}
		glScissor(r_ris.current_clip->final_clip_rect.position.x, y, r_ris.current_clip->final_clip_rect.size.width, r_ris.current_clip->final_clip_rect.size.height);
	}
}

// This function is a dry run of the state changes when drawing the item.
// It should duplicate the logic in _canvas_render_item,
// to decide whether items are similar enough to join
// i.e. no state differences between the 2 items.
bool RasterizerCanvasGLES3::try_join_item(Item *p_ci, RenderItemState &r_ris, bool &r_batch_break) {
	// if we set max join items to zero we can effectively prevent any joining, so
	// none of the other logic needs to run. Good for testing regression bugs, and
	// could conceivably be faster in some games.
	if (!bdata.settings_max_join_item_commands) {
		return false;
	}

	// if there are any state changes we change join to false
	// we also set r_batch_break to true if we don't want this item joined to the next
	// (e.g. an item that must not be joined at all)
	r_batch_break = false;
	bool join = true;

	// light_masked objects we just don't currently support for joining
	// (this could possibly be improved at a later date)
	if (p_ci->light_masked) {
		join = false;
		r_batch_break = true;
	}

	// we will now allow joining even if final modulate is different
	// we will instead bake the final modulate into the vertex colors
	//	if (p_ci->final_modulate != r_ris.final_modulate) {
	//		join = false;
	//		r_ris.final_modulate = p_ci->final_modulate;
	//	}

	if (r_ris.current_clip != p_ci->final_clip_owner) {
		r_ris.current_clip = p_ci->final_clip_owner;
		join = false;
	}

	// TODO: copy back buffer

	if (p_ci->copy_back_buffer) {
		join = false;
	}

	RasterizerStorageGLES3::Skeleton *skeleton = nullptr;

	{
		//skeleton handling
		if (p_ci->skeleton.is_valid() && storage->skeleton_owner.owns(p_ci->skeleton)) {
			skeleton = storage->skeleton_owner.get(p_ci->skeleton);
			if (!skeleton->use_2d) {
				skeleton = nullptr;
			}
		}

		bool skeleton_prevent_join = false;

		bool use_skeleton = skeleton != nullptr;
		if (r_ris.prev_use_skeleton != use_skeleton) {
			if (!bdata.settings_use_software_skinning) {
				r_ris.rebind_shader = true;
			}

			r_ris.prev_use_skeleton = use_skeleton;
			//			join = false;
			skeleton_prevent_join = true;
		}

		if (skeleton) {
			//			join = false;
			skeleton_prevent_join = true;
			state.using_skeleton = true;
		} else {
			state.using_skeleton = false;
		}

		if (skeleton_prevent_join) {
			if (!bdata.settings_use_software_skinning) {
				join = false;
			}
		}
	}

	Item *material_owner = p_ci->material_owner ? p_ci->material_owner : p_ci;

	RID material = material_owner->material;
	RasterizerStorageGLES3::Material *material_ptr = storage->material_owner.getornull(material);

	if (material != r_ris.canvas_last_material || r_ris.rebind_shader) {
		join = false;
		RasterizerStorageGLES3::Shader *shader_ptr = nullptr;

		if (material_ptr) {
			shader_ptr = material_ptr->shader;

			// special case, if the user has made an error in the shader code
			if (shader_ptr && !shader_ptr->valid) {
				join = false;
				r_batch_break = true;
			}

			if (shader_ptr && shader_ptr->mode != VS::SHADER_CANVAS_ITEM) {
				shader_ptr = nullptr; // not a canvas item shader, don't use.
			}
		}

		if (shader_ptr) {
			if (shader_ptr->canvas_item.uses_screen_texture) {
				if (!state.canvas_texscreen_used) {
					join = false;
				}
			}
		}

		r_ris.shader_cache = shader_ptr;

		r_ris.canvas_last_material = material;

		r_ris.rebind_shader = false;
	}

	int blend_mode = r_ris.shader_cache ? r_ris.shader_cache->canvas_item.blend_mode : RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX;
	bool unshaded = r_ris.shader_cache && (r_ris.shader_cache->canvas_item.light_mode == RasterizerStorageGLES3::Shader::CanvasItem::LIGHT_MODE_UNSHADED || (blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX && blend_mode != RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA));
	bool reclip = false;

	// we are precalculating the final_modulate ahead of time because we need this for baking of final modulate into vertex colors
	// (only in software transform mode)
	// This maybe inefficient storing it...
	r_ris.final_modulate = unshaded ? p_ci->final_modulate : (p_ci->final_modulate * r_ris.item_group_modulate);

	if (r_ris.last_blend_mode != blend_mode) {
		join = false;
		r_ris.last_blend_mode = blend_mode;
	}

	// does the shader contain BUILTINs which should break the batching?
	bdata.joined_item_batch_flags = 0;
	if (r_ris.shader_cache) {
		unsigned int and_flags = r_ris.shader_cache->canvas_item.batch_flags & (RasterizerStorageCommon::PREVENT_COLOR_BAKING | RasterizerStorageCommon::PREVENT_VERTEX_BAKING | RasterizerStorageCommon::PREVENT_ITEM_JOINING);
		if (and_flags) {
			// special case for preventing item joining altogether
			if (and_flags & RasterizerStorageCommon::PREVENT_ITEM_JOINING) {
				join = false;
				//r_batch_break = true; // don't think we need a batch break

				// save the flags so that they don't need to be recalculated in the 2nd pass
				bdata.joined_item_batch_flags |= r_ris.shader_cache->canvas_item.batch_flags;
			} else {
				bool use_larger_fvfs = true;

				if (and_flags == RasterizerStorageCommon::PREVENT_COLOR_BAKING) {
					// in some circumstances, if the modulate is identity, we still allow baking because reading modulate / color
					// will still be okay to do in the shader with no ill effects
					if (r_ris.final_modulate == Color(1, 1, 1, 1)) {
						use_larger_fvfs = false;
					}
				}

				// new .. always use large FVF
				if (use_larger_fvfs) {
					if (and_flags == RasterizerStorageCommon::PREVENT_COLOR_BAKING) {
						bdata.joined_item_batch_flags |= RasterizerStorageCommon::USE_MODULATE_FVF;
					} else {
						// we need to save on the joined item that it should use large fvf.
						// This info will then be used in filling and rendering
						bdata.joined_item_batch_flags |= RasterizerStorageCommon::USE_LARGE_FVF;
					}

					bdata.joined_item_batch_flags |= r_ris.shader_cache->canvas_item.batch_flags;
				}
			} // if not prevent item joining
		}
	}

	if ((blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_MIX || blend_mode == RasterizerStorageGLES3::Shader::CanvasItem::BLEND_MODE_PMALPHA) && r_ris.item_group_light && !unshaded) {
		// we cannot join lit items easily.
		// it is possible, but not if they overlap, because
		// a + light_blend + b + light_blend IS NOT THE SAME AS
		// a + b + light_blend

		bool light_allow_join = true;

		// this is a quick getout if we have turned off light joining
		if ((bdata.settings_light_max_join_items == 0) || r_ris.light_region.too_many_lights) {
			light_allow_join = false;
		} else {
			// do light joining...

			// first calculate the light bitfield
			uint64_t light_bitfield = 0;
			uint64_t shadow_bitfield = 0;
			Light *light = r_ris.item_group_light;

			int light_count = -1;
			while (light) {
				light_count++;
				uint64_t light_bit = 1ULL << light_count;

				// note that as a cost of batching, the light culling will be less effective
				if (p_ci->light_mask & light->item_mask && r_ris.item_group_z >= light->z_min && r_ris.item_group_z <= light->z_max) {
					// Note that with the above test, it is possible to also include a bound check.
					// Tests so far have indicated better performance without it, but there may be reason to change this at a later stage,
					// so I leave the line here for reference:
					// && p_ci->global_rect_cache.intersects_transformed(light->xform_cache, light->rect_cache)) {

					light_bitfield |= light_bit;

					bool has_shadow = light->shadow_buffer.is_valid() && p_ci->light_mask & light->item_shadow_mask;

					if (has_shadow) {
						shadow_bitfield |= light_bit;
					}
				}

				light = light->next_ptr;
			}

			// now compare to previous
			if ((r_ris.light_region.light_bitfield != light_bitfield) || (r_ris.light_region.shadow_bitfield != shadow_bitfield)) {
				light_allow_join = false;

				r_ris.light_region.light_bitfield = light_bitfield;
				r_ris.light_region.shadow_bitfield = shadow_bitfield;
			} else {
				// only do these checks if necessary
				if (join && (!r_batch_break)) {
					// we still can't join, even if the lights are exactly the same, if there is overlap between the previous and this item
					if (r_ris.joined_item && light_bitfield) {
						if ((int)r_ris.joined_item->num_item_refs <= bdata.settings_light_max_join_items) {
							for (uint32_t r = 0; r < r_ris.joined_item->num_item_refs; r++) {
								Item *pRefItem = bdata.item_refs[r_ris.joined_item->first_item_ref + r].item;
								if (p_ci->global_rect_cache.intersects(pRefItem->global_rect_cache)) {
									light_allow_join = false;
									break;
								}
							}

#ifdef DEBUG_ENABLED
							if (light_allow_join) {
								bdata.stats_light_items_joined++;
							}
#endif

						} // if below max join items
						else {
							// just don't allow joining if above overlap check max items
							light_allow_join = false;
						}
					}

				} // if not batch broken already (no point in doing expensive overlap tests if not needed)
			} // if bitfields don't match
		} // if do light joining

		if (!light_allow_join) {
			// can't join
			join = false;
			// we also dont want to allow joining this item with the next item, because the next item could have no lights!
			r_batch_break = true;
		}

	} else {
		// if the last item had lights, we should not join it to this one (which has no lights)
		if (r_ris.light_region.light_bitfield || r_ris.light_region.shadow_bitfield) {
			join = false;

			// setting these to zero ensures that any following item with lights will, by definition,
			// be affected by a different set of lights, and thus prevent a join
			r_ris.light_region.light_bitfield = 0;
			r_ris.light_region.shadow_bitfield = 0;
		}
	}

	if (reclip) {
		join = false;
	}

	// non rects will break the batching anyway, we don't want to record item changes, detect this
	if (!r_batch_break && _detect_item_batch_break(r_ris, p_ci, r_batch_break)) {
		join = false;
		r_batch_break = true;
	}

	return join;
}

void RasterizerCanvasGLES3::canvas_render_items_implementation(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform) {
	// parameters are easier to pass around in a structure
	RenderItemState ris;
	ris.item_group_z = p_z;
	ris.item_group_modulate = p_modulate;
	ris.item_group_light = p_light;
	ris.item_group_base_transform = p_base_transform;
	ris.prev_distance_field = false;

	glBindBuffer(GL_UNIFORM_BUFFER, state.canvas_item_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CanvasItemUBO), &state.canvas_item_ubo_data, _buffer_upload_usage_flag);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	state.current_tex = RID();
	state.current_tex_ptr = nullptr;
	state.current_normal = RID();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	//	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SKELETON, false);
	//	state.current_tex = RID();
	//	state.current_tex_ptr = NULL;
	//	state.current_normal = RID();
	//	state.canvas_texscreen_used = false;
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);

	if (bdata.settings_use_batching) {
		for (int j = 0; j < bdata.items_joined.size(); j++) {
			render_joined_item(bdata.items_joined[j], ris);
		}
	} else {
		while (p_item_list) {
			Item *ci = p_item_list;
			_legacy_canvas_render_item(ci, ris);
			p_item_list = p_item_list->next;
		}
	}

	if (ris.current_clip) {
		glDisable(GL_SCISSOR_TEST);
	}

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_SKELETON, false);
}

void RasterizerCanvasGLES3::_batch_upload_buffers() {
	// noop?
	if (!bdata.vertices.size()) {
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, bdata.gl_vertex_buffer);

	// usage flag is a project setting
	GLenum buffer_usage_flag = GL_DYNAMIC_DRAW;
	if (bdata.buffer_mode_batch_upload_flag_stream) {
		buffer_usage_flag = GL_STREAM_DRAW;
	}

	// orphan the old (for now)
	if (bdata.buffer_mode_batch_upload_send_null) {
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, buffer_usage_flag); // GL_DYNAMIC_DRAW);
	}

	switch (bdata.fvf) {
		case RasterizerStorageCommon::FVF_UNBATCHED: // should not happen
			break;
		case RasterizerStorageCommon::FVF_REGULAR: // no change
			glBufferData(GL_ARRAY_BUFFER, sizeof(BatchVertex) * bdata.vertices.size(), bdata.vertices.get_data(), buffer_usage_flag);
			break;
		case RasterizerStorageCommon::FVF_COLOR:
			glBufferData(GL_ARRAY_BUFFER, sizeof(BatchVertexColored) * bdata.unit_vertices.size(), bdata.unit_vertices.get_unit(0), buffer_usage_flag);
			break;
		case RasterizerStorageCommon::FVF_LIGHT_ANGLE:
			glBufferData(GL_ARRAY_BUFFER, sizeof(BatchVertexLightAngled) * bdata.unit_vertices.size(), bdata.unit_vertices.get_unit(0), buffer_usage_flag);
			break;
		case RasterizerStorageCommon::FVF_MODULATED:
			glBufferData(GL_ARRAY_BUFFER, sizeof(BatchVertexModulated) * bdata.unit_vertices.size(), bdata.unit_vertices.get_unit(0), buffer_usage_flag);
			break;
		case RasterizerStorageCommon::FVF_LARGE:
			glBufferData(GL_ARRAY_BUFFER, sizeof(BatchVertexLarge) * bdata.unit_vertices.size(), bdata.unit_vertices.get_unit(0), buffer_usage_flag);
			break;
	}

	// might not be necessary
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerCanvasGLES3::_batch_render_lines(const Batch &p_batch, RasterizerStorageGLES3::Material *p_material, bool p_anti_alias) {
	_set_texture_rect_mode(false);

	_bind_canvas_texture(RID(), RID());

	glBindVertexArray(batch_gl_data.batch_vertex_array[0]);

	glDisableVertexAttribArray(VS::ARRAY_COLOR);
	glVertexAttrib4fv(VS::ARRAY_COLOR, (float *)&p_batch.color);

	int64_t offset = p_batch.first_vert; // 6 inds per quad at 2 bytes each

	int num_elements = p_batch.num_commands * 2;

#ifdef GLES_OVER_GL
	if (p_anti_alias) {
		glEnable(GL_LINE_SMOOTH);
	}
#endif

	glDrawArrays(GL_LINES, offset, num_elements);

	storage->info.render._2d_draw_call_count++;

	glBindVertexArray(0);

#ifdef GLES_OVER_GL
	if (p_anti_alias) {
		glDisable(GL_LINE_SMOOTH);
	}
#endif
}

void RasterizerCanvasGLES3::_batch_render_prepare() {
	//const bool &colored_verts = bdata.use_colored_vertices;
	const bool &use_light_angles = bdata.use_light_angles;
	const bool &use_modulate = bdata.use_modulate;
	const bool &use_large_verts = bdata.use_large_verts;

	_set_texture_rect_mode(false, false, use_light_angles, use_modulate, use_large_verts);

	//	state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, p_rect->flags & CANVAS_RECT_CLIP_UV);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);

	switch (bdata.fvf) {
		case RasterizerStorageCommon::FVF_UNBATCHED: // should not happen
			return;
			break;
		case RasterizerStorageCommon::FVF_REGULAR: // no change
			glBindVertexArray(batch_gl_data.batch_vertex_array[0]);
			break;
		case RasterizerStorageCommon::FVF_COLOR:
			glBindVertexArray(batch_gl_data.batch_vertex_array[1]);
			break;
		case RasterizerStorageCommon::FVF_LIGHT_ANGLE:
			glBindVertexArray(batch_gl_data.batch_vertex_array[2]);
			break;
		case RasterizerStorageCommon::FVF_MODULATED:
			glBindVertexArray(batch_gl_data.batch_vertex_array[3]);
			break;
		case RasterizerStorageCommon::FVF_LARGE:
			glBindVertexArray(batch_gl_data.batch_vertex_array[4]);
			break;
	}
}

void RasterizerCanvasGLES3::_batch_render_generic(const Batch &p_batch, RasterizerStorageGLES3::Material *p_material) {
	ERR_FAIL_COND(p_batch.num_commands <= 0);

	const bool &use_light_angles = bdata.use_light_angles;
	const bool &use_modulate = bdata.use_modulate;
	const bool &use_large_verts = bdata.use_large_verts;
	const bool &colored_verts = bdata.use_colored_vertices | use_light_angles | use_modulate | use_large_verts;

	_set_texture_rect_mode(false, false, use_light_angles, use_modulate, use_large_verts);

	//	state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, p_rect->flags & CANVAS_RECT_CLIP_UV);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::CLIP_RECT_UV, false);

	switch (bdata.fvf) {
		case RasterizerStorageCommon::FVF_UNBATCHED: // should not happen
			return;
			break;
		case RasterizerStorageCommon::FVF_REGULAR: // no change
			glBindVertexArray(batch_gl_data.batch_vertex_array[0]);
			break;
		case RasterizerStorageCommon::FVF_COLOR:
			glBindVertexArray(batch_gl_data.batch_vertex_array[1]);
			break;
		case RasterizerStorageCommon::FVF_LIGHT_ANGLE:
			glBindVertexArray(batch_gl_data.batch_vertex_array[2]);
			break;
		case RasterizerStorageCommon::FVF_MODULATED:
			glBindVertexArray(batch_gl_data.batch_vertex_array[3]);
			break;
		case RasterizerStorageCommon::FVF_LARGE:
			glBindVertexArray(batch_gl_data.batch_vertex_array[4]);
			break;
	}

	// batch tex
	const BatchTex &tex = bdata.batch_textures[p_batch.batch_texture_id];

	_bind_canvas_texture(tex.RID_texture, tex.RID_normal);

	if (!colored_verts) {
		// may not need this disable
		glDisableVertexAttribArray(VS::ARRAY_COLOR);
		glVertexAttrib4fv(VS::ARRAY_COLOR, p_batch.color.get_data());
	}

	// We only want to set the GL wrapping mode if the texture is not already tiled (i.e. set in Import).
	// This  is an optimization left over from the legacy renderer.
	// If we DID set tiling in the API, and reverted to clamped, then the next draw using this texture
	// may use clamped mode incorrectly.
	bool tex_is_already_tiled = tex.flags & VS::TEXTURE_FLAG_REPEAT;

	switch (tex.tile_mode) {
		case BatchTex::TILE_NORMAL: {
			// if the texture is imported as tiled, no need to set GL state, as it will already be bound with repeat
			if (!tex_is_already_tiled) {
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
		} break;
		default: {
		} break;
	}

	// we need to convert explicitly from pod Vec2 to Vector2 ...
	// could use a cast but this might be unsafe in future
	Vector2 tps;
	tex.tex_pixel_size.to(tps);
	state.canvas_shader.set_uniform(CanvasShaderGLES3::COLOR_TEXPIXEL_SIZE, tps);

	switch (p_batch.type) {
		default: {
			// prevent compiler warning
		} break;
		case RasterizerStorageCommon::BT_RECT: {
			int64_t offset = p_batch.first_vert * 3; // 6 inds per quad at 2 bytes each

			int num_elements = p_batch.num_commands * 6;
			glDrawElements(GL_TRIANGLES, num_elements, GL_UNSIGNED_SHORT, (void *)offset);
		} break;
		case RasterizerStorageCommon::BT_POLY: {
			int64_t offset = p_batch.first_vert;

			int num_elements = p_batch.num_commands;
			glDrawArrays(GL_TRIANGLES, offset, num_elements);
		} break;
	}

	storage->info.render._2d_draw_call_count++;

	glBindVertexArray(0);

	switch (tex.tile_mode) {
		case BatchTex::TILE_NORMAL: {
			// if the texture is imported as tiled, no need to revert GL state
			if (!tex_is_already_tiled) {
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		} break;
		default: {
		} break;
	}
}

void RasterizerCanvasGLES3::initialize() {
	RasterizerGLES3::gl_check_errors();
	int flag_stream_mode = GLOBAL_GET("rendering/2d/opengl/legacy_stream");
	switch (flag_stream_mode) {
		default: {
			_buffer_upload_usage_flag = GL_STREAM_DRAW;
		} break;
		case 1: {
			_buffer_upload_usage_flag = GL_DYNAMIC_DRAW;
		} break;
		case 2: {
			_buffer_upload_usage_flag = GL_STREAM_DRAW;
		} break;
	}

	{
		//quad buffers

		glGenBuffers(1, &data.canvas_quad_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, data.canvas_quad_vertices);
		{
			const float qv[8] = {
				0, 0,
				0, 1,
				1, 1,
				1, 0
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, qv, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		glGenVertexArrays(1, &data.canvas_quad_array);
		glBindVertexArray(data.canvas_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data.canvas_quad_vertices);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	}
	{
		//particle quad buffers

		glGenBuffers(1, &data.particle_quad_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, data.particle_quad_vertices);
		{
			//quad of size 1, with pivot on the center for particles, then regular UVS. Color is general plus fetched from particle
			const float qv[16] = {
				-0.5, -0.5,
				0.0, 0.0,
				-0.5, 0.5,
				0.0, 1.0,
				0.5, 0.5,
				1.0, 1.0,
				0.5, -0.5,
				1.0, 0.0
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, qv, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

		glGenVertexArrays(1, &data.particle_quad_array);
		glBindVertexArray(data.particle_quad_array);
		glBindBuffer(GL_ARRAY_BUFFER, data.particle_quad_vertices);
		glEnableVertexAttribArray(VS::ARRAY_VERTEX);
		glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(8));
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	}
	{
		uint32_t poly_size = GLOBAL_DEF_RST("rendering/limits/buffers/canvas_polygon_buffer_size_kb", 128);
		ProjectSettings::get_singleton()->set_custom_property_info("rendering/limits/buffers/canvas_polygon_buffer_size_kb", PropertyInfo(Variant::INT, "rendering/limits/buffers/canvas_polygon_buffer_size_kb", PROPERTY_HINT_RANGE, "0,256,1,or_greater"));
		poly_size = MAX(poly_size, 2); // minimum 2k, may still see anomalies in editor
		poly_size *= 1024; //kb
		glGenBuffers(1, &data.polygon_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);
		glBufferData(GL_ARRAY_BUFFER, poly_size, nullptr, GL_DYNAMIC_DRAW); //allocate max size
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		data.polygon_buffer_size = poly_size;

		//quad arrays
		for (int i = 0; i < Data::NUM_QUAD_ARRAY_VARIATIONS; i++) {
			glGenVertexArrays(1, &data.polygon_buffer_quad_arrays[i]);
			glBindVertexArray(data.polygon_buffer_quad_arrays[i]);
			glBindBuffer(GL_ARRAY_BUFFER, data.polygon_buffer);

			int uv_ofs = 0;
			int color_ofs = 0;
			int light_angle_ofs = 0;
			int stride = 2 * 4;

			if (i & 1) { //color
				color_ofs = stride;
				stride += 4 * 4;
			}

			if (i & 2) { //uv
				uv_ofs = stride;
				stride += 2 * 4;
			}

			if (i & 4) { //light_angle
				light_angle_ofs = stride;
				stride += 1 * 4;
			}

			glEnableVertexAttribArray(VS::ARRAY_VERTEX);
			glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, stride, nullptr);

			if (i & 1) {
				glEnableVertexAttribArray(VS::ARRAY_COLOR);
				glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(color_ofs));
			}

			if (i & 2) {
				glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
				glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(uv_ofs));
			}

			if (i & 4) {
				// reusing tangent for light_angle
				glEnableVertexAttribArray(VS::ARRAY_TANGENT);
				glVertexAttribPointer(VS::ARRAY_TANGENT, 1, GL_FLOAT, GL_FALSE, stride, CAST_INT_TO_UCHAR_PTR(light_angle_ofs));
			}

			glBindVertexArray(0);
		}

		glGenVertexArrays(1, &data.polygon_buffer_pointer_array);

		uint32_t index_size = GLOBAL_DEF_RST("rendering/limits/buffers/canvas_polygon_index_buffer_size_kb", 128);
		ProjectSettings::get_singleton()->set_custom_property_info("rendering/limits/buffers/canvas_polygon_index_buffer_size_kb", PropertyInfo(Variant::INT, "rendering/limits/buffers/canvas_polygon_index_buffer_size_kb", PROPERTY_HINT_RANGE, "0,256,1,or_greater"));
		index_size = MAX(index_size, 2);
		index_size *= 1024; //kb
		glGenBuffers(1, &data.polygon_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.polygon_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, nullptr, GL_DYNAMIC_DRAW); //allocate max size
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		data.polygon_index_buffer_size = index_size;
	}

	store_transform(Transform(), state.canvas_item_ubo_data.projection_matrix);

	glGenBuffers(1, &state.canvas_item_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, state.canvas_item_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CanvasItemUBO), &state.canvas_item_ubo_data, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	state.canvas_shader.init();
	state.canvas_shader.set_base_material_tex_index(2);
	state.canvas_shadow_shader.init();
	state.lens_shader.init();

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_RGBA_SHADOWS, storage->config.use_rgba_2d_shadows);
	state.canvas_shadow_shader.set_conditional(CanvasShadowShaderGLES3::USE_RGBA_SHADOWS, storage->config.use_rgba_2d_shadows);

	state.canvas_shader.set_conditional(CanvasShaderGLES3::USE_PIXEL_SNAP, GLOBAL_DEF("rendering/2d/snapping/use_gpu_pixel_snap", false));

	batch_initialize();

	// just reserve some space (may not be needed as we are orphaning, but hey ho)
	glGenBuffers(1, &bdata.gl_vertex_buffer);

	if (bdata.vertex_buffer_size_bytes) {
		glBindBuffer(GL_ARRAY_BUFFER, bdata.gl_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, bdata.vertex_buffer_size_bytes, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// pre fill index buffer, the indices never need to change so can be static
		glGenBuffers(1, &bdata.gl_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bdata.gl_index_buffer);

		Vector<uint16_t> indices;
		indices.resize(bdata.index_buffer_size_units);

		for (unsigned int q = 0; q < bdata.max_quads; q++) {
			int i_pos = q * 6; //  6 inds per quad
			int q_pos = q * 4; // 4 verts per quad
			indices.set(i_pos, q_pos);
			indices.set(i_pos + 1, q_pos + 1);
			indices.set(i_pos + 2, q_pos + 2);
			indices.set(i_pos + 3, q_pos);
			indices.set(i_pos + 4, q_pos + 2);
			indices.set(i_pos + 5, q_pos + 3);
		}

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bdata.index_buffer_size_bytes, &indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	} // only if there is a vertex buffer (batching is on)

	// vertex array objects
	for (int vao = 0; vao < 5; vao++) {
		int sizeof_vert;
		switch (vao) {
			case 0:
				sizeof_vert = sizeof(BatchVertex);
				break;
			case 1:
				sizeof_vert = sizeof(BatchVertexColored);
				break;
			case 2:
				sizeof_vert = sizeof(BatchVertexLightAngled);
				break;
			case 3:
				sizeof_vert = sizeof(BatchVertexModulated);
				break;
			case 4:
				sizeof_vert = sizeof(BatchVertexLarge);
				break;
		}

		glGenVertexArrays(1, &batch_gl_data.batch_vertex_array[vao]);
		glBindVertexArray(batch_gl_data.batch_vertex_array[vao]);
		glBindBuffer(GL_ARRAY_BUFFER, bdata.gl_vertex_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bdata.gl_index_buffer);

		uint64_t pointer = 0;
		glEnableVertexAttribArray(VS::ARRAY_VERTEX);
		glVertexAttribPointer(VS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof_vert, (const void *)pointer);

		// always send UVs, even within a texture specified because a shader can still use UVs
		glEnableVertexAttribArray(VS::ARRAY_TEX_UV);
		glVertexAttribPointer(VS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (2 * 4)));

		// optional attributes
		bool a_color = false;
		bool a_light_angle = false;
		bool a_modulate = false;
		bool a_large = false;

		switch (vao) {
			case 0:
				break;
			case 1: {
				a_color = true;
			} break;
			case 2: {
				a_color = true;
				a_light_angle = true;
			} break;
			case 3: {
				a_color = true;
				a_light_angle = true;
				a_modulate = true;
			} break;
			case 4: {
				a_color = true;
				a_light_angle = true;
				a_modulate = true;
				a_large = true;
			} break;
		}

		if (a_color) {
			glEnableVertexAttribArray(VS::ARRAY_COLOR);
			glVertexAttribPointer(VS::ARRAY_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (4 * 4)));
		}
		if (a_light_angle) {
			glEnableVertexAttribArray(VS::ARRAY_TANGENT);
			glVertexAttribPointer(VS::ARRAY_TANGENT, 1, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (8 * 4)));
		}
		if (a_modulate) {
			glEnableVertexAttribArray(VS::ARRAY_TEX_UV2);
			glVertexAttribPointer(VS::ARRAY_TEX_UV2, 4, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (9 * 4)));
		}
		if (a_large) {
			glEnableVertexAttribArray(VS::ARRAY_BONES);
			glVertexAttribPointer(VS::ARRAY_BONES, 2, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (13 * 4)));
			glEnableVertexAttribArray(VS::ARRAY_WEIGHTS);
			glVertexAttribPointer(VS::ARRAY_WEIGHTS, 4, GL_FLOAT, GL_FALSE, sizeof_vert, CAST_INT_TO_UCHAR_PTR(pointer + (15 * 4)));
		}

		glBindVertexArray(0);
	} // for vao

	// deal with ninepatch mode option
	if (bdata.settings_ninepatch_mode == 1) {
		state.canvas_shader.add_custom_define("#define USE_NINEPATCH_SCALING\n");
	}

	RasterizerGLES3::gl_check_errors();
}

RasterizerCanvasGLES3::RasterizerCanvasGLES3() {
	batch_constructor();
}

// BATCHING

void RasterizerCanvasGLES3::batch_canvas_render_items_begin(const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform) {
	// if we are debugging, flash each frame between batching renderer and old version to compare for regressions
	if (bdata.settings_flash_batching) {
		if ((Engine::get_singleton()->get_frames_drawn() % 2) == 0) {
			bdata.settings_use_batching = true;
		} else {
			bdata.settings_use_batching = false;
		}
	}

	if (!bdata.settings_use_batching) {
		return;
	}

	// this only needs to be done when screen size changes, but this should be
	// infrequent enough
	_calculate_scissor_threshold_area();

	// set up render item state for all the z_indexes (this is common to all z_indexes)
	_render_item_state.reset();
	_render_item_state.item_group_modulate = p_modulate;
	_render_item_state.item_group_light = p_light;
	_render_item_state.item_group_base_transform = p_base_transform;
	_render_item_state.light_region.reset();

	// batch break must be preserved over the different z indices,
	// to prevent joining to an item on a previous index if not allowed
	_render_item_state.join_batch_break = false;

	// whether to join across z indices depends on whether there are z ranged lights.
	// joined z_index items can be wrongly classified with z ranged lights.
	bdata.join_across_z_indices = true;

	int light_count = 0;
	while (p_light) {
		light_count++;

		if ((p_light->z_min != VS::CANVAS_ITEM_Z_MIN) || (p_light->z_max != VS::CANVAS_ITEM_Z_MAX)) {
			// prevent joining across z indices. This would have caused visual regressions
			bdata.join_across_z_indices = false;
		}

		p_light = p_light->next_ptr;
	}

	// can't use the light region bitfield if there are too many lights
	// hopefully most games won't blow this limit..
	// if they do they will work but it won't batch join items just in case
	if (light_count > 64) {
		_render_item_state.light_region.too_many_lights = true;
	}
}

void RasterizerCanvasGLES3::batch_canvas_render_items(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform) {
	// stage 1 : join similar items, so that their state changes are not repeated,
	// and commands from joined items can be batched together
	if (bdata.settings_use_batching) {
		record_items(p_item_list, p_z);
		return;
	}

	// only legacy renders at this stage, batched renderer doesn't render until canvas_render_items_end()
	canvas_render_items_implementation(p_item_list, p_z, p_modulate, p_light, p_base_transform);
}

void RasterizerCanvasGLES3::batch_canvas_render_items_end() {
	if (!bdata.settings_use_batching) {
		return;
	}

	join_sorted_items();

#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	if (bdata.diagnose_frame) {
		bdata.frame_string += "items\n";
	}
#endif

	// batching render is deferred until after going through all the z_indices, joining all the items
	canvas_render_items_implementation(nullptr, 0, _render_item_state.item_group_modulate,
			_render_item_state.item_group_light,
			_render_item_state.item_group_base_transform);

	bdata.items_joined.reset();
	bdata.item_refs.reset();
	bdata.sort_items.reset();
}

void RasterizerCanvasGLES3::batch_canvas_begin() {
	// diagnose_frame?
	bdata.frame_string = ""; // just in case, always set this as we don't want a string leak in release...
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	if (bdata.settings_diagnose_frame) {
		bdata.diagnose_frame = false;

		uint32_t tick = OS::get_singleton()->get_ticks_msec();
		uint64_t frame = Engine::get_singleton()->get_frames_drawn();

		if (tick >= bdata.next_diagnose_tick) {
			bdata.next_diagnose_tick = tick + 10000;

			// the plus one is prevent starting diagnosis half way through frame
			bdata.diagnose_frame_number = frame + 1;
		}

		if (frame == bdata.diagnose_frame_number) {
			bdata.diagnose_frame = true;
			bdata.reset_stats();
		}

		if (bdata.diagnose_frame) {
			bdata.frame_string = "canvas_begin FRAME " + itos(frame) + "\n";
		}
	}
#endif
}

void RasterizerCanvasGLES3::batch_canvas_end() {
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	if (bdata.diagnose_frame) {
		bdata.frame_string += "canvas_end\n";
		if (bdata.stats_items_sorted) {
			bdata.frame_string += "\titems reordered: " + itos(bdata.stats_items_sorted) + "\n";
		}
		if (bdata.stats_light_items_joined) {
			bdata.frame_string += "\tlight items joined: " + itos(bdata.stats_light_items_joined) + "\n";
		}

		print_line(bdata.frame_string);
	}
#endif
}

// Default batches will not occur in software transform only items
// EXCEPT IN THE CASE OF SINGLE RECTS (and this may well not occur, check the logic in prefill_join_item TYPE_RECT)
// but can occur where transform commands have been sent during hardware batch
void RasterizerCanvasGLES3::_prefill_default_batch(FillState &r_fill_state, int p_command_num, const RasterizerCanvas::Item &p_item) {
	if (r_fill_state.curr_batch->type == RasterizerStorageCommon::BT_DEFAULT) {
		// don't need to flush an extra transform command?
		if (!r_fill_state.transform_extra_command_number_p1) {
			// another default command, just add to the existing batch
			r_fill_state.curr_batch->num_commands++;

			// Note this is getting hit, needs investigation as to whether this is a bug or a false flag
			// DEV_CHECK_ONCE(r_fill_state.curr_batch->num_commands <= p_command_num);
		} else {
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
			if (r_fill_state.transform_extra_command_number_p1 != p_command_num) {
				WARN_PRINT_ONCE("_prefill_default_batch : transform_extra_command_number_p1 != p_command_num");
			}
#endif
			// if the first member of the batch is a transform we have to be careful
			if (!r_fill_state.curr_batch->num_commands) {
				// there can be leading useless extra transforms (sometimes happens with debug collision polys)
				// we need to rejig the first_command for the first useful transform
				r_fill_state.curr_batch->first_command += r_fill_state.transform_extra_command_number_p1 - 1;
			}

			// we do have a pending extra transform command to flush
			// either the extra transform is in the prior command, or not, in which case we need 2 batches
			r_fill_state.curr_batch->num_commands += 2;

			r_fill_state.transform_extra_command_number_p1 = 0; // mark as sent
			r_fill_state.extra_matrix_sent = true;

			// the original mode should always be hardware transform ..
			// test this assumption
			//CRASH_COND(r_fill_state.orig_transform_mode != TM_NONE);
			r_fill_state.transform_mode = r_fill_state.orig_transform_mode;

			// do we need to restore anything else?
		}
	} else {
		// end of previous different type batch, so start new default batch

		// first consider whether there is a dirty extra matrix to send
		if (r_fill_state.transform_extra_command_number_p1) {
			// get which command the extra is in, and blank all the records as it no longer is stored CPU side
			int extra_command = r_fill_state.transform_extra_command_number_p1 - 1; // plus 1 based
			r_fill_state.transform_extra_command_number_p1 = 0;
			r_fill_state.extra_matrix_sent = true;

			// send the extra to the GPU in a batch
			r_fill_state.curr_batch = _batch_request_new();
			r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_DEFAULT;
			r_fill_state.curr_batch->first_command = extra_command;
			r_fill_state.curr_batch->num_commands = 1;
			r_fill_state.curr_batch->item = &p_item;

			// revert to the original transform mode
			// e.g. go back to NONE if we were in hardware transform mode
			r_fill_state.transform_mode = r_fill_state.orig_transform_mode;

			// reset the original transform if we are going back to software mode,
			// because the extra is now done on the GPU...
			// (any subsequent extras are sent directly to the GPU, no deferring)
			if (r_fill_state.orig_transform_mode != TM_NONE) {
				r_fill_state.transform_combined = p_item.final_transform;
			}

			// can possibly combine batch with the next one in some cases
			// this is more efficient than having an extra batch especially for the extra
			if ((extra_command + 1) == p_command_num) {
				r_fill_state.curr_batch->num_commands = 2;
				return;
			}
		}

		// start default batch
		r_fill_state.curr_batch = _batch_request_new();
		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_DEFAULT;
		r_fill_state.curr_batch->first_command = p_command_num;
		r_fill_state.curr_batch->num_commands = 1;
		r_fill_state.curr_batch->item = &p_item;
	}
}

int RasterizerCanvasGLES3::_batch_find_or_create_tex(const RID &p_texture, const RID &p_normal, bool p_tile, int p_previous_match) {
	// optimization .. in 99% cases the last matched value will be the same, so no need to traverse the list
	if (p_previous_match > 0) // if it is zero, it will get hit first in the linear search anyway
	{
		const BatchTex &batch_texture = bdata.batch_textures[p_previous_match];

		// note for future reference, if RID implementation changes, this could become more expensive
		if ((batch_texture.RID_texture == p_texture) && (batch_texture.RID_normal == p_normal)) {
			// tiling mode must also match
			bool tiles = batch_texture.tile_mode != BatchTex::TILE_OFF;

			if (tiles == p_tile) {
				// match!
				return p_previous_match;
			}
		}
	}

	// not the previous match .. we will do a linear search ... slower, but should happen
	// not very often except with non-batchable runs, which are going to be slow anyway
	// n.b. could possibly be replaced later by a fast hash table
	for (int n = 0; n < bdata.batch_textures.size(); n++) {
		const BatchTex &batch_texture = bdata.batch_textures[n];
		if ((batch_texture.RID_texture == p_texture) && (batch_texture.RID_normal == p_normal)) {
			// tiling mode must also match
			bool tiles = batch_texture.tile_mode != BatchTex::TILE_OFF;

			if (tiles == p_tile) {
				// match!
				return n;
			}
		}
	}

	// pushing back from local variable .. not ideal but has to use a Vector because non pod
	// due to RIDs
	BatchTex new_batch_tex;
	new_batch_tex.RID_texture = p_texture;
	new_batch_tex.RID_normal = p_normal;

	// get the texture
	typename RasterizerStorageGLES3::Texture *texture = _get_canvas_texture(p_texture);

	if (texture) {
		// special case, there can be textures with no width or height
		int w = texture->width;
		int h = texture->height;

		if (!w || !h) {
			w = 1;
			h = 1;
		}

		new_batch_tex.tex_pixel_size.x = 1.0 / w;
		new_batch_tex.tex_pixel_size.y = 1.0 / h;
		new_batch_tex.flags = texture->flags;
	} else {
		// maybe doesn't need doing...
		new_batch_tex.tex_pixel_size.x = 1.0f;
		new_batch_tex.tex_pixel_size.y = 1.0f;
		new_batch_tex.flags = 0;
	}

	if (p_tile) {
		if (texture) {
			// default
			new_batch_tex.tile_mode = BatchTex::TILE_NORMAL;

			// no hardware support for non power of 2 tiling
			if (!storage->config.support_npot_repeat_mipmap) {
				if (next_power_of_2(texture->alloc_width) != (unsigned int)texture->alloc_width && next_power_of_2(texture->alloc_height) != (unsigned int)texture->alloc_height) {
					new_batch_tex.tile_mode = BatchTex::TILE_FORCE_REPEAT;
				}
			}
		} else {
			// this should not happen?
			new_batch_tex.tile_mode = BatchTex::TILE_OFF;
		}
	} else {
		new_batch_tex.tile_mode = BatchTex::TILE_OFF;
	}

	// push back
	bdata.batch_textures.push_back(new_batch_tex);

	return bdata.batch_textures.size() - 1;
}

void RasterizerCanvasGLES3::batch_constructor() {
	bdata.settings_use_batching = false;

#ifdef GLES_OVER_GL
	use_nvidia_rect_workaround = GLOBAL_GET("rendering/2d/options/use_nvidia_rect_flicker_workaround");
#else
	// Not needed (a priori) on GLES devices
	use_nvidia_rect_workaround = false;
#endif
}

void RasterizerCanvasGLES3::batch_initialize() {
	bdata.settings_use_batching = GLOBAL_GET("rendering/batching/options/use_batching");
	bdata.settings_max_join_item_commands = GLOBAL_GET("rendering/batching/parameters/max_join_item_commands");
	bdata.settings_colored_vertex_format_threshold = GLOBAL_GET("rendering/batching/parameters/colored_vertex_format_threshold");
	bdata.settings_item_reordering_lookahead = GLOBAL_GET("rendering/batching/parameters/item_reordering_lookahead");
	bdata.settings_light_max_join_items = GLOBAL_GET("rendering/batching/lights/max_join_items");
	bdata.settings_use_single_rect_fallback = GLOBAL_GET("rendering/batching/options/single_rect_fallback");
	bdata.settings_use_software_skinning = GLOBAL_GET("rendering/2d/options/use_software_skinning");
	bdata.settings_ninepatch_mode = GLOBAL_GET("rendering/2d/options/ninepatch_mode");

	// allow user to override the api usage techniques using project settings
	int send_null_mode = GLOBAL_GET("rendering/2d/opengl/batching_send_null");
	switch (send_null_mode) {
		default: {
			bdata.buffer_mode_batch_upload_send_null = true;
		} break;
		case 1: {
			bdata.buffer_mode_batch_upload_send_null = false;
		} break;
		case 2: {
			bdata.buffer_mode_batch_upload_send_null = true;
		} break;
	}

	int stream_mode = GLOBAL_GET("rendering/2d/opengl/batching_stream");
	switch (stream_mode) {
		default: {
			bdata.buffer_mode_batch_upload_flag_stream = false;
		} break;
		case 1: {
			bdata.buffer_mode_batch_upload_flag_stream = false;
		} break;
		case 2: {
			bdata.buffer_mode_batch_upload_flag_stream = true;
		} break;
	}

	// alternatively only enable uv contract if pixel snap in use,
	// but with this enable bool, it should not be necessary
	bdata.settings_uv_contract = GLOBAL_GET("rendering/batching/precision/uv_contract");
	bdata.settings_uv_contract_amount = (float)GLOBAL_GET("rendering/batching/precision/uv_contract_amount") / 1000000.0f;

	// we can use the threshold to determine whether to turn scissoring off or on
	bdata.settings_scissor_threshold = GLOBAL_GET("rendering/batching/lights/scissor_area_threshold");
	if (bdata.settings_scissor_threshold > 0.999f) {
		bdata.settings_scissor_lights = false;
	} else {
		bdata.settings_scissor_lights = true;

		// apply power of 4 relationship for the area, as most of the important changes
		// will be happening at low values of scissor threshold
		bdata.settings_scissor_threshold *= bdata.settings_scissor_threshold;
		bdata.settings_scissor_threshold *= bdata.settings_scissor_threshold;
	}

	// The sweet spot on my desktop for cache is actually smaller than the max, and this
	// is the default. This saves memory too so we will use it for now, needs testing to see whether this varies according
	// to device / platform.
	bdata.settings_batch_buffer_num_verts = GLOBAL_GET("rendering/batching/parameters/batch_buffer_size");

	// override the use_batching setting in the editor
	// (note that if the editor can't start, you can't change the use_batching project setting!)
	if (Engine::get_singleton()->is_editor_hint()) {
		bool use_in_editor = GLOBAL_GET("rendering/batching/options/use_batching_in_editor");
		bdata.settings_use_batching = use_in_editor;

		// fix some settings in the editor, as the performance not worth the risk
		bdata.settings_use_single_rect_fallback = false;
	}

	// if we are using batching, we will purposefully disable the nvidia workaround.
	// This is because the only reason to use the single rect fallback is the approx 2x speed
	// of the uniform drawing technique. If we used nvidia workaround, speed would be
	// approx equal to the batcher drawing technique (indexed primitive + VB).
	if (bdata.settings_use_batching) {
		use_nvidia_rect_workaround = false;
	}

	// For debugging, if flash is set in project settings, it will flash on alternate frames
	// between the non-batched renderer and the batched renderer,
	// in order to find regressions.
	// This should not be used except during development.
	// make a note of the original choice in case we are flashing on and off the batching
	bdata.settings_use_batching_original_choice = bdata.settings_use_batching;
	bdata.settings_flash_batching = GLOBAL_GET("rendering/batching/debug/flash_batching");
	if (!bdata.settings_use_batching) {
		// no flash when batching turned off
		bdata.settings_flash_batching = false;
	}

	// frame diagnosis. print out the batches every nth frame
	bdata.settings_diagnose_frame = false;
	if (!Engine::get_singleton()->is_editor_hint() && bdata.settings_use_batching) {
		//	{
		bdata.settings_diagnose_frame = GLOBAL_GET("rendering/batching/debug/diagnose_frame");
	}

	// the maximum num quads in a batch is limited by GLES2. We can have only 16 bit indices,
	// which means we can address a vertex buffer of max size 65535. 4 vertices are needed per quad.

	// Note this determines the memory use by the vertex buffer vector. max quads (65536/4)-1
	// but can be reduced to save memory if really required (will result in more batches though)
	const int max_possible_quads = (65536 / 4) - 1;
	const int min_possible_quads = 8; // some reasonable small value

	// value from project settings
	int max_quads = bdata.settings_batch_buffer_num_verts / 4;

	// sanity checks
	max_quads = CLAMP(max_quads, min_possible_quads, max_possible_quads);
	bdata.settings_max_join_item_commands = CLAMP(bdata.settings_max_join_item_commands, 0, 65535);
	bdata.settings_colored_vertex_format_threshold = CLAMP(bdata.settings_colored_vertex_format_threshold, 0.0f, 1.0f);
	bdata.settings_scissor_threshold = CLAMP(bdata.settings_scissor_threshold, 0.0f, 1.0f);
	bdata.settings_light_max_join_items = CLAMP(bdata.settings_light_max_join_items, 0, 65535);
	bdata.settings_item_reordering_lookahead = CLAMP(bdata.settings_item_reordering_lookahead, 0, 65535);

	// For debug purposes, output a string with the batching options.
	if (bdata.settings_use_batching) {
		String batching_options_string = "OpenGL ES 2D Batching: ON\n";
		batching_options_string += "Batching Options:\n";
		batching_options_string += "\tmax_join_item_commands " + itos(bdata.settings_max_join_item_commands) + "\n";
		batching_options_string += "\tcolored_vertex_format_threshold " + String(Variant(bdata.settings_colored_vertex_format_threshold)) + "\n";
		batching_options_string += "\tbatch_buffer_size " + itos(bdata.settings_batch_buffer_num_verts) + "\n";
		batching_options_string += "\tlight_scissor_area_threshold " + String(Variant(bdata.settings_scissor_threshold)) + "\n";
		batching_options_string += "\titem_reordering_lookahead " + itos(bdata.settings_item_reordering_lookahead) + "\n";
		batching_options_string += "\tlight_max_join_items " + itos(bdata.settings_light_max_join_items) + "\n";
		batching_options_string += "\tsingle_rect_fallback " + String(Variant(bdata.settings_use_single_rect_fallback)) + "\n";
		batching_options_string += "\tdebug_flash " + String(Variant(bdata.settings_flash_batching)) + "\n";
		batching_options_string += "\tdiagnose_frame " + String(Variant(bdata.settings_diagnose_frame));
		print_verbose(batching_options_string);
	}

	// special case, for colored vertex format threshold.
	// as the comparison is >=, we want to be able to totally turn on or off
	// conversion to colored vertex format at the extremes, so we will force
	// 1.0 to be just above 1.0
	if (bdata.settings_colored_vertex_format_threshold > 0.995f) {
		bdata.settings_colored_vertex_format_threshold = 1.01f;
	}

	// save memory when batching off
	if (!bdata.settings_use_batching) {
		max_quads = 0;
	}

	uint32_t sizeof_batch_vert = sizeof(BatchVertex);

	bdata.max_quads = max_quads;

	// 4 verts per quad
	bdata.vertex_buffer_size_units = max_quads * 4;

	// the index buffer can be longer than 65535, only the indices need to be within this range
	bdata.index_buffer_size_units = max_quads * 6;

	const int max_verts = bdata.vertex_buffer_size_units;

	// this comes out at approx 64K for non-colored vertex buffer, and 128K for colored vertex buffer
	bdata.vertex_buffer_size_bytes = max_verts * sizeof_batch_vert;
	bdata.index_buffer_size_bytes = bdata.index_buffer_size_units * 2; // 16 bit inds

	// create equal number of normal and (max) unit sized verts (as the normal may need to be translated to a larger FVF)
	bdata.vertices.create(max_verts); // 512k
	bdata.unit_vertices.create(max_verts, sizeof(BatchVertexLarge));

	// extra data per vert needed for larger FVFs
	bdata.light_angles.create(max_verts);
	bdata.vertex_colors.create(max_verts);
	bdata.vertex_modulates.create(max_verts);
	bdata.vertex_transforms.create(max_verts);

	// num batches will be auto increased dynamically if required
	bdata.batches.create(1024);
	bdata.batches_temp.create(bdata.batches.max_size());

	// batch textures can also be increased dynamically
	bdata.batch_textures.create(32);
}

bool RasterizerCanvasGLES3::_light_scissor_begin(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect) const {
	float area_item = p_item_rect.size.x * p_item_rect.size.y; // double check these are always positive

	// quick reject .. the area of pixels saved can never be more than the area of the item
	if (area_item < bdata.scissor_threshold_area) {
		return false;
	}

	Rect2 cliprect;
	if (!_light_find_intersection(p_item_rect, p_light_xform, p_light_rect, cliprect)) {
		// should not really occur .. but just in case
		cliprect = Rect2(0, 0, 0, 0);
	} else {
		// some conditions not to scissor
		// determine the area (fill rate) that will be saved
		float area_cliprect = cliprect.size.x * cliprect.size.y;
		float area_saved = area_item - area_cliprect;

		// if area saved is too small, don't scissor
		if (area_saved < bdata.scissor_threshold_area) {
			return false;
		}
	}

	int rh = storage->frame.current_rt->height;

	// using the exact size was leading to off by one errors,
	// possibly due to pixel snap. For this reason we will boost
	// the scissor area by 1 pixel, this will take care of any rounding
	// issues, and shouldn't significantly negatively impact performance.
	int y = rh - (cliprect.position.y + cliprect.size.y);
	y += 1; // off by 1 boost before flipping

	if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
		y = cliprect.position.y;
	}

	gl_enable_scissor(cliprect.position.x - 1, y, cliprect.size.width + 2, cliprect.size.height + 2);

	return true;
}

bool RasterizerCanvasGLES3::_light_find_intersection(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect, Rect2 &r_cliprect) const {
	// transform light to world space (note this is done in the earlier intersection test, so could
	// be made more efficient)
	Vector2 pts[4] = {
		p_light_xform.xform(p_light_rect.position),
		p_light_xform.xform(Vector2(p_light_rect.position.x + p_light_rect.size.x, p_light_rect.position.y)),
		p_light_xform.xform(Vector2(p_light_rect.position.x, p_light_rect.position.y + p_light_rect.size.y)),
		p_light_xform.xform(Vector2(p_light_rect.position.x + p_light_rect.size.x, p_light_rect.position.y + p_light_rect.size.y)),
	};

	// calculate the light bound rect in world space
	Rect2 lrect(pts[0].x, pts[0].y, 0, 0);
	for (int n = 1; n < 4; n++) {
		lrect.expand_to(pts[n]);
	}

	// intersection between the 2 rects
	// they should probably always intersect, because of earlier check, but just in case...
	if (!p_item_rect.intersects(lrect)) {
		return false;
	}

	// note this does almost the same as Rect2.clip but slightly more efficient for our use case
	r_cliprect.position.x = MAX(p_item_rect.position.x, lrect.position.x);
	r_cliprect.position.y = MAX(p_item_rect.position.y, lrect.position.y);

	Point2 item_rect_end = p_item_rect.position + p_item_rect.size;
	Point2 lrect_end = lrect.position + lrect.size;

	r_cliprect.size.x = MIN(item_rect_end.x, lrect_end.x) - r_cliprect.position.x;
	r_cliprect.size.y = MIN(item_rect_end.y, lrect_end.y) - r_cliprect.position.y;

	return true;
}

void RasterizerCanvasGLES3::_calculate_scissor_threshold_area() {
	if (!bdata.settings_scissor_lights) {
		return;
	}

	// scissor area threshold is 0.0 to 1.0 in the settings for ease of use.
	// we need to translate to an absolute area to determine quickly whether
	// to scissor.
	if (bdata.settings_scissor_threshold < 0.0001f) {
		bdata.scissor_threshold_area = -1.0f; // will always pass
	} else {
		// in pixels
		int w = storage->frame.current_rt->width;
		int h = storage->frame.current_rt->height;

		int screen_area = w * h;

		bdata.scissor_threshold_area = bdata.settings_scissor_threshold * screen_area;
	}
}

bool RasterizerCanvasGLES3::_prefill_line(RasterizerCanvas::Item::CommandLine *p_line, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// we have separate batch types for non and anti aliased lines.
	// You can't batch the different types together.
	RasterizerStorageCommon::BatchType line_batch_type = RasterizerStorageCommon::BT_LINE;
	uint32_t line_batch_flags = RasterizerStorageCommon::BTF_LINE;
#ifdef GLES_OVER_GL
	if (p_line->antialiased) {
		line_batch_type = RasterizerStorageCommon::BT_LINE_AA;
		line_batch_flags = RasterizerStorageCommon::BTF_LINE_AA;
	}
#endif

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != line_batch_type) {
		if (r_fill_state.sequence_batch_type_flags & (~line_batch_flags)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= line_batch_flags;

		change_batch = true;
	}

	// get the baked line color
	Color col = p_line->color;

	if (multiply_final_modulate) {
		col *= r_fill_state.final_modulate;
	}

	BatchColor bcol;
	bcol.set(col);

	// if the color has changed we need a new batch
	// (only single color line batches supported so far)
	if (!change_batch && r_fill_state.curr_batch->color != bcol) {
		change_batch = true;
	}

	// not sure if needed
	r_fill_state.batch_tex_id = -1;

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	// request multiple vertices at a time, this is more efficient
	BatchVertex *bvs = bdata.vertices.request(2);
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	if (change_batch) {
		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = line_batch_type;
		r_fill_state.curr_batch->color = bcol;
		// cast is to stop sanitizer benign warning .. watch though in case destination type changes
		r_fill_state.curr_batch->batch_texture_id = (uint16_t)-1;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = 1;
		//r_fill_state.curr_batch->first_quad = bdata.total_quads;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands++;
	}

	// fill the geometry
	Vector2 from = p_line->from;
	Vector2 to = p_line->to;

	const bool use_large_verts = bdata.use_large_verts;

	if ((r_fill_state.transform_mode != TM_NONE) && (!use_large_verts)) {
		_software_transform_vertex(from, r_fill_state.transform_combined);
		_software_transform_vertex(to, r_fill_state.transform_combined);
	}

	bvs[0].pos.set(from);
	bvs[0].uv.set(0, 0); // may not be necessary
	bvs[1].pos.set(to);
	bvs[1].uv.set(0, 0);

	bdata.total_verts += 2;

	return false;
}

//unsigned int _ninepatch_apply_tiling_modes(RasterizerCanvas::Item::CommandNinePatch *p_np, Rect2 &r_source) {
//	unsigned int rect_flags = 0;

//	switch (p_np->axis_x) {
//		default:
//			break;
//		case VisualServer::NINE_PATCH_TILE: {
//			r_source.size.x = p_np->rect.size.x;
//			rect_flags = RasterizerCanvas::CANVAS_RECT_TILE;
//		} break;
//		case VisualServer::NINE_PATCH_TILE_FIT: {
//			// prevent divide by zero (may never happen)
//			if (r_source.size.x) {
//				int units = p_np->rect.size.x / r_source.size.x;
//				if (!units)
//					units++;
//				r_source.size.x = r_source.size.x * units;
//				rect_flags = RasterizerCanvas::CANVAS_RECT_TILE;
//			}
//		} break;
//	}

//	switch (p_np->axis_y) {
//		default:
//			break;
//		case VisualServer::NINE_PATCH_TILE: {
//			r_source.size.y = p_np->rect.size.y;
//			rect_flags = RasterizerCanvas::CANVAS_RECT_TILE;
//		} break;
//		case VisualServer::NINE_PATCH_TILE_FIT: {
//			// prevent divide by zero (may never happen)
//			if (r_source.size.y) {
//				int units = p_np->rect.size.y / r_source.size.y;
//				if (!units)
//					units++;
//				r_source.size.y = r_source.size.y * units;
//				rect_flags = RasterizerCanvas::CANVAS_RECT_TILE;
//			}
//		} break;
//	}

//	return rect_flags;
//}

template <bool SEND_LIGHT_ANGLES>
bool RasterizerCanvasGLES3::_prefill_ninepatch(RasterizerCanvas::Item::CommandNinePatch *p_np, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	typename RasterizerStorageGLES3::Texture *tex = _get_canvas_texture(p_np->texture);

	if (!tex) {
		// FIXME: Handle textureless ninepatch gracefully
		WARN_PRINT("NinePatch without texture not supported yet, skipping.");
		return false;
	}

	if (tex->width == 0 || tex->height == 0) {
		WARN_PRINT("Cannot set empty texture to NinePatch.");
		return false;
	}

	// cope with ninepatch of zero area. These cannot be created by the user interface or gdscript, but can
	// be created programmatically from c++, e.g. by the Godot UI for sliders. We will just not draw these.
	if ((p_np->rect.size.x * p_np->rect.size.y) <= 0.0f) {
		return false;
	}

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != RasterizerStorageCommon::BT_RECT) {
		// don't allow joining to a different sequence type
		if (r_fill_state.sequence_batch_type_flags & (~RasterizerStorageCommon::BTF_RECT)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
	}

	// first check there are enough verts for this to complete successfully
	if (bdata.vertices.size() + (4 * 9) > bdata.vertices.max_size()) {
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	// create a temporary rect so we can reuse the rect routine
	RasterizerCanvas::Item::CommandRect trect;

	trect.texture = p_np->texture;
	trect.normal_map = p_np->normal_map;
	trect.modulate = p_np->color;
	trect.flags = RasterizerCanvas::CANVAS_RECT_REGION;

	//Size2 texpixel_size(1.0f / tex->width, 1.0f / tex->height);

	Rect2 source = p_np->source;
	if (source.size.x == 0 && source.size.y == 0) {
		source.size.x = tex->width;
		source.size.y = tex->height;
	}

	float screen_scale = 1.0f;

	// optional crazy ninepatch scaling mode
	if ((bdata.settings_ninepatch_mode == 1) && (source.size.x != 0) && (source.size.y != 0)) {
		screen_scale = MIN(p_np->rect.size.x / source.size.x, p_np->rect.size.y / source.size.y);
		screen_scale = MIN(1.0, screen_scale);
	}

	// deal with nine patch texture wrapping modes
	// this is switched off because it may not be possible with batching
	// trect.flags |= _ninepatch_apply_tiling_modes(p_np, source);

	// translate to rects
	Rect2 &rt = trect.rect;
	Rect2 &src = trect.source;

	float tex_margin_left = p_np->margin[MARGIN_LEFT];
	float tex_margin_right = p_np->margin[MARGIN_RIGHT];
	float tex_margin_top = p_np->margin[MARGIN_TOP];
	float tex_margin_bottom = p_np->margin[MARGIN_BOTTOM];

	float x[4];
	x[0] = p_np->rect.position.x;
	x[1] = x[0] + (p_np->margin[MARGIN_LEFT] * screen_scale);
	x[3] = x[0] + (p_np->rect.size.x);
	x[2] = x[3] - (p_np->margin[MARGIN_RIGHT] * screen_scale);

	float y[4];
	y[0] = p_np->rect.position.y;
	y[1] = y[0] + (p_np->margin[MARGIN_TOP] * screen_scale);
	y[3] = y[0] + (p_np->rect.size.y);
	y[2] = y[3] - (p_np->margin[MARGIN_BOTTOM] * screen_scale);

	float u[4];
	u[0] = source.position.x;
	u[1] = u[0] + tex_margin_left;
	u[3] = u[0] + source.size.x;
	u[2] = u[3] - tex_margin_right;

	float v[4];
	v[0] = source.position.y;
	v[1] = v[0] + tex_margin_top;
	v[3] = v[0] + source.size.y;
	v[2] = v[3] - tex_margin_bottom;

	// Some protection for the use of ninepatches with rect size smaller than margin size.
	// Note these cannot be produced by the UI, only programmatically, and the results
	// are somewhat undefined, because the margins overlap.
	// Ninepatch get_minimum_size()	forces minimum size to be the sum of the margins.
	// So this should occur very rarely if ever. Consider commenting these 4 lines out for higher speed
	// in ninepatches.
	x[1] = MIN(x[1], x[3]);
	x[2] = MIN(x[2], x[3]);
	y[1] = MIN(y[1], y[3]);
	y[2] = MIN(y[2], y[3]);

	// temporarily override to prevent single rect fallback
	bool single_rect_fallback = bdata.settings_use_single_rect_fallback;
	bdata.settings_use_single_rect_fallback = false;

	// each line of the ninepatch
	for (int line = 0; line < 3; line++) {
		rt.position = Vector2(x[0], y[line]);
		rt.size = Vector2(x[1] - x[0], y[line + 1] - y[line]);
		src.position = Vector2(u[0], v[line]);
		src.size = Vector2(u[1] - u[0], v[line + 1] - v[line]);
		_prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);

		if ((line == 1) && (!p_np->draw_center)) {
			;
		} else {
			rt.position.x = x[1];
			rt.size.x = x[2] - x[1];
			src.position.x = u[1];
			src.size.x = u[2] - u[1];
			_prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);
		}

		rt.position.x = x[2];
		rt.size.x = x[3] - x[2];
		src.position.x = u[2];
		src.size.x = u[3] - u[2];
		_prefill_rect<SEND_LIGHT_ANGLES>(&trect, r_fill_state, r_command_start, command_num, command_count, nullptr, p_item, multiply_final_modulate);
	}

	// restore single rect fallback
	bdata.settings_use_single_rect_fallback = single_rect_fallback;
	return false;
}

template <bool SEND_LIGHT_ANGLES>
bool RasterizerCanvasGLES3::_prefill_polygon(RasterizerCanvas::Item::CommandPolygon *p_poly, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != RasterizerStorageCommon::BT_POLY) {
		// don't allow joining to a different sequence type
		if (r_fill_state.sequence_batch_type_flags & (~RasterizerStorageCommon::BTF_POLY)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= RasterizerStorageCommon::BTF_POLY;

		change_batch = true;
	}

	int num_inds = p_poly->indices.size();

	// nothing to draw?
	if (!num_inds || !p_poly->points.size()) {
		return false;
	}

	// we aren't using indices, so will transform verts more than once .. less efficient.
	// could be done with a temporary vertex buffer
	BatchVertex *bvs = bdata.vertices.request(num_inds);
	if (!bvs) {
		// run out of space in the vertex buffer
		// check for special case where the batching buffer is simply not big enough to fit this primitive.
		if (!bdata.vertices.size()) {
			// can't draw, ignore the primitive, otherwise we would enter an infinite loop
			WARN_PRINT_ONCE("poly has too many indices to draw, increase batch buffer size");
			return false;
		}

		// .. finish this function and draw what we have so far
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	BatchColor *vertex_colors = bdata.vertex_colors.request(num_inds);
	RAST_DEBUG_ASSERT(vertex_colors);

	// are we using large FVF?
	////////////////////////////////////
	const bool use_large_verts = bdata.use_large_verts;
	const bool use_modulate = bdata.use_modulate;

	BatchColor *vertex_modulates = nullptr;
	if (use_modulate) {
		vertex_modulates = bdata.vertex_modulates.request(num_inds);
		RAST_DEBUG_ASSERT(vertex_modulates);
		// precalc the vertex modulate (will be shared by all verts)
		// we store the modulate as an attribute in the fvf rather than a uniform
		vertex_modulates[0].set(r_fill_state.final_modulate);
	}

	BatchTransform *pBT = nullptr;
	if (use_large_verts) {
		pBT = bdata.vertex_transforms.request(num_inds);
		RAST_DEBUG_ASSERT(pBT);
		// precalc the batch transform (will be shared by all verts)
		// we store the transform as an attribute in the fvf rather than a uniform
		const Transform2D &tr = r_fill_state.transform_combined;

		pBT[0].translate.set(tr.elements[2]);
		pBT[0].basis[0].set(tr.elements[0][0], tr.elements[0][1]);
		pBT[0].basis[1].set(tr.elements[1][0], tr.elements[1][1]);
	}
	////////////////////////////////////

	// the modulate is always baked
	Color modulate;
	if (multiply_final_modulate) {
		modulate = r_fill_state.final_modulate;
	} else {
		modulate = Color(1, 1, 1, 1);
	}

	int old_batch_tex_id = r_fill_state.batch_tex_id;
	r_fill_state.batch_tex_id = _batch_find_or_create_tex(p_poly->texture, p_poly->normal_map, false, old_batch_tex_id);

	// conditions for creating a new batch
	if (old_batch_tex_id != r_fill_state.batch_tex_id) {
		change_batch = true;
	}

	// N.B. polygons don't have color thus don't need a batch change with color
	// This code is left as reference in case of problems.
	//	if (!r_fill_state.curr_batch->color.equals(modulate)) {
	//		change_batch = true;
	//		bdata.total_color_changes++;
	//	}

	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
		const BatchTex &batchtex = bdata.batch_textures[r_fill_state.batch_tex_id];
		batchtex.tex_pixel_size.to(r_fill_state.texpixel_size);

		if (bdata.settings_uv_contract) {
			r_fill_state.contract_uvs = (batchtex.flags & VS::TEXTURE_FLAG_FILTER) == 0;
		}

		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_POLY;

		// modulate unused except for debugging?
		r_fill_state.curr_batch->color.set(modulate);
		r_fill_state.curr_batch->batch_texture_id = r_fill_state.batch_tex_id;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = num_inds;
		//		r_fill_state.curr_batch->num_elements = num_inds;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands += num_inds;
	}

	// PRECALCULATE THE COLORS (as there may be less colors than there are indices
	// in either hardware or software paths)
	BatchColor vcol;
	int num_verts = p_poly->points.size();

	// in special cases, only 1 color is specified by convention, so we want to preset this
	// to use in all verts.
	if (p_poly->colors.size()) {
		vcol.set(p_poly->colors[0] * modulate);
	} else {
		// color is undefined, use modulate color straight
		vcol.set(modulate);
	}

	BatchColor *precalced_colors = (BatchColor *)alloca(num_verts * sizeof(BatchColor));

	// two stage, super efficient setup of precalculated colors
	int num_colors_specified = p_poly->colors.size();

	for (int n = 0; n < num_colors_specified; n++) {
		vcol.set(p_poly->colors[n] * modulate);
		precalced_colors[n] = vcol;
	}
	for (int n = num_colors_specified; n < num_verts; n++) {
		precalced_colors[n] = vcol;
	}

	if (!_software_skin_poly(p_poly, p_item, bvs, vertex_colors, r_fill_state, precalced_colors)) {
		bool software_transform = (r_fill_state.transform_mode != TM_NONE) && (!use_large_verts);

		for (int n = 0; n < num_inds; n++) {
			int ind = p_poly->indices[n];

			DEV_CHECK_ONCE(ind < p_poly->points.size());

			// recover at runtime from invalid polys (the editor may send invalid polys)
			if ((unsigned int)ind >= (unsigned int)num_verts) {
				// will recover as long as there is at least one vertex.
				// if there are no verts, we will have quick rejected earlier in this function
				ind = 0;
			}

			// this could be moved outside the loop
			if (software_transform) {
				Vector2 pos = p_poly->points[ind];
				_software_transform_vertex(pos, r_fill_state.transform_combined);
				bvs[n].pos.set(pos.x, pos.y);
			} else {
				const Point2 &pos = p_poly->points[ind];
				bvs[n].pos.set(pos.x, pos.y);
			}

			if (ind < p_poly->uvs.size()) {
				const Point2 &uv = p_poly->uvs[ind];
				bvs[n].uv.set(uv.x, uv.y);
			} else {
				bvs[n].uv.set(0.0f, 0.0f);
			}

			vertex_colors[n] = precalced_colors[ind];

			if (use_modulate) {
				vertex_modulates[n] = vertex_modulates[0];
			}

			if (use_large_verts) {
				// reuse precalced transform (same for each vertex within polygon)
				pBT[n] = pBT[0];
			}
		}
	} // if not software skinning
	else {
		// software skinning extra passes
		if (use_modulate) {
			for (int n = 0; n < num_inds; n++) {
				vertex_modulates[n] = vertex_modulates[0];
			}
		}
		// not sure if this will produce garbage if software skinning is changing vertex pos
		// in the shader, but is included for completeness
		if (use_large_verts) {
			for (int n = 0; n < num_inds; n++) {
				pBT[n] = pBT[0];
			}
		}
	}

	// increment total vert count
	bdata.total_verts += num_inds;

	return false;
}

bool RasterizerCanvasGLES3::_software_skin_poly(RasterizerCanvas::Item::CommandPolygon *p_poly, RasterizerCanvas::Item *p_item, BatchVertex *bvs, BatchColor *vertex_colors, const FillState &p_fill_state, const BatchColor *p_precalced_colors) {
	//	alternatively could check state.using_skeleton
	if (p_item->skeleton == RID()) {
		return false;
	}

	int num_inds = p_poly->indices.size();
	int num_verts = p_poly->points.size();

	RID skeleton = p_item->skeleton;
	int bone_count = RasterizerStorage::base_singleton->skeleton_get_bone_count(skeleton);

	// we want a temporary buffer of positions to transform
	Vector2 *pTemps = (Vector2 *)alloca(num_verts * sizeof(Vector2));
	memset((void *)pTemps, 0, num_verts * sizeof(Vector2));

	// only the inverse appears to be needed
	const Transform2D &skel_trans_inv = p_fill_state.skeleton_base_inverse_xform;
	// we can't get this from the state, because more than one skeleton item may have been joined together..
	// we need to handle the base skeleton on a per item basis as the joined item is rendered.
	// const Transform2D &skel_trans = state.skeleton_transform;
	// const Transform2D &skel_trans_inv = state.skeleton_transform_inverse;

	// get the bone transforms.
	// this is not ideal because we don't know in advance which bones are needed
	// for any particular poly, but depends how cheap the skeleton_bone_get_transform_2d call is
	Transform2D *bone_transforms = (Transform2D *)alloca(bone_count * sizeof(Transform2D));
	for (int b = 0; b < bone_count; b++) {
		bone_transforms[b] = RasterizerStorage::base_singleton->skeleton_bone_get_transform_2d(skeleton, b);
	}

	if (num_verts && (p_poly->bones.size() == num_verts * 4) && (p_poly->weights.size() == p_poly->bones.size())) {
		// instead of using the p_item->xform we use the final transform,
		// because we want the poly transform RELATIVE to the base skeleton.
		Transform2D item_transform = skel_trans_inv * p_item->final_transform;

		Transform2D item_transform_inv = item_transform.affine_inverse();

		for (int n = 0; n < num_verts; n++) {
			const Vector2 &src_pos = p_poly->points[n];
			Vector2 &dst_pos = pTemps[n];

			// there can be an offset on the polygon at rigging time, this has to be accounted for
			// note it may be possible that this could be concatenated with the bone transforms to save extra transforms - not sure yet
			Vector2 src_pos_back_transformed = item_transform.xform(src_pos);

			float total_weight = 0.0f;

			for (int k = 0; k < 4; k++) {
				int bone_id = p_poly->bones[n * 4 + k];
				float weight = p_poly->weights[n * 4 + k];
				if (weight == 0.0f) {
					continue;
				}

				total_weight += weight;

				DEV_CHECK_ONCE(bone_id < bone_count);
				const Transform2D &bone_tr = bone_transforms[bone_id];

				Vector2 pos = bone_tr.xform(src_pos_back_transformed);

				dst_pos += pos * weight;
			}

			// this is some unexplained weirdness with verts with no weights,
			// but it seemed to work for the example project ... watch for regressions
			if (total_weight < 0.01f) {
				dst_pos = src_pos;
			} else {
				dst_pos /= total_weight;

				// retransform back from the poly offset space
				dst_pos = item_transform_inv.xform(dst_pos);
			}
		}

	} // if bone format matches
	else {
		// not rigged properly, just copy the verts directly
		for (int n = 0; n < num_verts; n++) {
			const Vector2 &src_pos = p_poly->points[n];
			Vector2 &dst_pos = pTemps[n];

			dst_pos = src_pos;
		}
	}

	// software transform with combined matrix?
	if (p_fill_state.transform_mode != TM_NONE) {
		for (int n = 0; n < num_verts; n++) {
			Vector2 &dst_pos = pTemps[n];
			_software_transform_vertex(dst_pos, p_fill_state.transform_combined);
		}
	}

	// output to the batch verts
	for (int n = 0; n < num_inds; n++) {
		int ind = p_poly->indices[n];

		DEV_CHECK_ONCE(ind < num_verts);

		// recover at runtime from invalid polys (the editor may send invalid polys)
		if ((unsigned int)ind >= (unsigned int)num_verts) {
			// will recover as long as there is at least one vertex.
			// if there are no verts, we will have quick rejected earlier in this function
			ind = 0;
		}

		const Point2 &pos = pTemps[ind];
		bvs[n].pos.set(pos.x, pos.y);

		if (ind < p_poly->uvs.size()) {
			const Point2 &uv = p_poly->uvs[ind];
			bvs[n].uv.set(uv.x, uv.y);
		} else {
			bvs[n].uv.set(0.0f, 0.0f);
		}

		vertex_colors[n] = p_precalced_colors[ind];
	}

	return true;
}

template <bool SEND_LIGHT_ANGLES>
bool RasterizerCanvasGLES3::_prefill_rect(RasterizerCanvas::Item::CommandRect *rect, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item::Command *const *commands, RasterizerCanvas::Item *p_item, bool multiply_final_modulate) {
	bool change_batch = false;

	// conditions for creating a new batch
	if (r_fill_state.curr_batch->type != RasterizerStorageCommon::BT_RECT) {
		// don't allow joining to a different sequence type
		if (r_fill_state.sequence_batch_type_flags & (~RasterizerStorageCommon::BTF_RECT)) {
			// don't allow joining to a different sequence type
			r_command_start = command_num;
			return true;
		}
		r_fill_state.sequence_batch_type_flags |= RasterizerStorageCommon::BTF_RECT;

		change_batch = true;

		// check for special case if there is only a single or small number of rects,
		// in which case we will use the legacy default rect renderer
		// because it is faster for single rects

		// we only want to do this if not a joined item with more than 1 item,
		// because joined items with more than 1, the command * will be incorrect
		// NOTE - this is assuming that use_hardware_transform means that it is a non-joined item!!
		// If that assumption is incorrect this will go horribly wrong.
		if (bdata.settings_use_single_rect_fallback && r_fill_state.is_single_item) {
			bool is_single_rect = false;
			int command_num_next = command_num + 1;
			if (command_num_next < command_count) {
				RasterizerCanvas::Item::Command *command_next = commands[command_num_next];
				if ((command_next->type != RasterizerCanvas::Item::Command::TYPE_RECT) && (command_next->type != RasterizerCanvas::Item::Command::TYPE_TRANSFORM)) {
					is_single_rect = true;
				}
			} else {
				is_single_rect = true;
			}
			// if it is a rect on its own, do exactly the same as the default routine
			if (is_single_rect) {
				_prefill_default_batch(r_fill_state, command_num, *p_item);
				return false;
			}
		} // if use hardware transform
	}

	// try to create vertices BEFORE creating a batch,
	// because if the vertex buffer is full, we need to finish this
	// function, draw what we have so far, and then start a new set of batches

	// request FOUR vertices at a time, this is more efficient
	BatchVertex *bvs = bdata.vertices.request(4);
	if (!bvs) {
		// run out of space in the vertex buffer .. finish this function and draw what we have so far
		// return where we got to
		r_command_start = command_num;
		return true;
	}

	// are we using large FVF?
	const bool use_large_verts = bdata.use_large_verts;
	const bool use_modulate = bdata.use_modulate;

	Color col = rect->modulate;

	// use_modulate and use_large_verts should have been checked in the calling prefill_item function.
	// we don't want to apply the modulate on the CPU if it is stored in the vertex format, it will
	// be applied in the shader
	if (multiply_final_modulate) {
		col *= r_fill_state.final_modulate;
	}

	// instead of doing all the texture preparation for EVERY rect,
	// we build a list of texture combinations and do this once off.
	// This means we have a potentially rather slow step to identify which texture combo
	// using the RIDs.
	int old_batch_tex_id = r_fill_state.batch_tex_id;
	r_fill_state.batch_tex_id = _batch_find_or_create_tex(rect->texture, rect->normal_map, rect->flags & RasterizerCanvas::CANVAS_RECT_TILE, old_batch_tex_id);

	//r_fill_state.use_light_angles = send_light_angles;
	if (SEND_LIGHT_ANGLES) {
		bdata.use_light_angles = true;
	}

	// conditions for creating a new batch
	if (old_batch_tex_id != r_fill_state.batch_tex_id) {
		change_batch = true;
	}

	// we need to treat color change separately because we need to count these
	// to decide whether to switch on the fly to colored vertices.
	if (!change_batch && !r_fill_state.curr_batch->color.equals(col)) {
		change_batch = true;
		bdata.total_color_changes++;
	}

	if (change_batch) {
		// put the tex pixel size  in a local (less verbose and can be a register)
		const BatchTex &batchtex = bdata.batch_textures[r_fill_state.batch_tex_id];
		batchtex.tex_pixel_size.to(r_fill_state.texpixel_size);

		if (bdata.settings_uv_contract) {
			r_fill_state.contract_uvs = (batchtex.flags & VS::TEXTURE_FLAG_FILTER) == 0;
		}

		// need to preserve texpixel_size between items
		//r_fill_state.texpixel_size = r_fill_state.texpixel_size;

		// open new batch (this should never fail, it dynamically grows)
		r_fill_state.curr_batch = _batch_request_new(false);

		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_RECT;
		r_fill_state.curr_batch->color.set(col);
		r_fill_state.curr_batch->batch_texture_id = r_fill_state.batch_tex_id;
		r_fill_state.curr_batch->first_command = command_num;
		r_fill_state.curr_batch->num_commands = 1;
		//r_fill_state.curr_batch->first_quad = bdata.total_quads;
		r_fill_state.curr_batch->first_vert = bdata.total_verts;
	} else {
		// we could alternatively do the count when closing a batch .. perhaps more efficient
		r_fill_state.curr_batch->num_commands++;
	}

	// fill the quad geometry
	Vector2 mins = rect->rect.position;

	if (r_fill_state.transform_mode == TM_TRANSLATE) {
		if (!use_large_verts) {
			_software_transform_vertex(mins, r_fill_state.transform_combined);
		}
	}

	Vector2 maxs = mins + rect->rect.size;

	// just aliases
	BatchVertex *bA = &bvs[0];
	BatchVertex *bB = &bvs[1];
	BatchVertex *bC = &bvs[2];
	BatchVertex *bD = &bvs[3];

	bA->pos.x = mins.x;
	bA->pos.y = mins.y;

	bB->pos.x = maxs.x;
	bB->pos.y = mins.y;

	bC->pos.x = maxs.x;
	bC->pos.y = maxs.y;

	bD->pos.x = mins.x;
	bD->pos.y = maxs.y;

	// possibility of applying flips here for normal mapping .. but they don't seem to be used
	if (rect->rect.size.x < 0) {
		SWAP(bA->pos, bB->pos);
		SWAP(bC->pos, bD->pos);
	}
	if (rect->rect.size.y < 0) {
		SWAP(bA->pos, bD->pos);
		SWAP(bB->pos, bC->pos);
	}

	if (r_fill_state.transform_mode == TM_ALL) {
		if (!use_large_verts) {
			_software_transform_vertex(bA->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bB->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bC->pos, r_fill_state.transform_combined);
			_software_transform_vertex(bD->pos, r_fill_state.transform_combined);
		}
	}

	// uvs
	Vector2 src_min;
	Vector2 src_max;
	if (rect->flags & RasterizerCanvas::CANVAS_RECT_REGION) {
		src_min = rect->source.position;
		src_max = src_min + rect->source.size;

		src_min *= r_fill_state.texpixel_size;
		src_max *= r_fill_state.texpixel_size;

		const float uv_epsilon = bdata.settings_uv_contract_amount;

		// nudge offset for the maximum to prevent precision error on GPU reading into line outside the source rect
		// this is very difficult to get right.
		if (r_fill_state.contract_uvs) {
			src_min.x += uv_epsilon;
			src_min.y += uv_epsilon;
			src_max.x -= uv_epsilon;
			src_max.y -= uv_epsilon;
		}
	} else {
		src_min = Vector2(0, 0);
		src_max = Vector2(1, 1);
	}

	// 10% faster calculating the max first
	Vector2 uvs[4] = {
		src_min,
		Vector2(src_max.x, src_min.y),
		src_max,
		Vector2(src_min.x, src_max.y),
	};

	// for encoding in light angle
	// flips should be optimized out when not being used for light angle.
	bool flip_h = false;
	bool flip_v = false;

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_TRANSPOSE) {
		SWAP(uvs[1], uvs[3]);
	}

	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_H) {
		SWAP(uvs[0], uvs[1]);
		SWAP(uvs[2], uvs[3]);
		flip_h = !flip_h;
		flip_v = !flip_v;
	}
	if (rect->flags & RasterizerCanvas::CANVAS_RECT_FLIP_V) {
		SWAP(uvs[0], uvs[3]);
		SWAP(uvs[1], uvs[2]);
		flip_v = !flip_v;
	}

	bA->uv.set(uvs[0]);
	bB->uv.set(uvs[1]);
	bC->uv.set(uvs[2]);
	bD->uv.set(uvs[3]);

	// modulate
	if (use_modulate) {
		// store the final modulate separately from the rect modulate
		BatchColor *pBC = bdata.vertex_modulates.request(4);
		RAST_DEBUG_ASSERT(pBC);
		pBC[0].set(r_fill_state.final_modulate);
		pBC[1] = pBC[0];
		pBC[2] = pBC[0];
		pBC[3] = pBC[0];
	}

	if (use_large_verts) {
		// store the transform separately
		BatchTransform *pBT = bdata.vertex_transforms.request(4);
		RAST_DEBUG_ASSERT(pBT);

		const Transform2D &tr = r_fill_state.transform_combined;

		pBT[0].translate.set(tr.elements[2]);

		pBT[0].basis[0].set(tr.elements[0][0], tr.elements[0][1]);
		pBT[0].basis[1].set(tr.elements[1][0], tr.elements[1][1]);

		pBT[1] = pBT[0];
		pBT[2] = pBT[0];
		pBT[3] = pBT[0];
	}

	if (SEND_LIGHT_ANGLES) {
		// we can either keep the light angles in sync with the verts when writing,
		// or sync them up during translation. We are syncing in translation.
		// N.B. There may be batches that don't require light_angles between batches that do.
		float *angles = bdata.light_angles.request(4);
		RAST_DEBUG_ASSERT(angles);

		float angle = 0.0f;
		const float TWO_PI = Math_PI * 2;

		if (r_fill_state.transform_mode != TM_NONE) {
			const Transform2D &tr = r_fill_state.transform_combined;

			// apply to an x axis
			// the x axis and y axis can be taken directly from the transform (no need to xform identity vectors)
			Vector2 x_axis(tr.elements[0][0], tr.elements[0][1]);

			// have to do a y axis to check for scaling flips
			// this is hassle and extra slowness. We could only allow flips via the flags.
			Vector2 y_axis(tr.elements[1][0], tr.elements[1][1]);

			// has the x / y axis flipped due to scaling?
			float cross = x_axis.cross(y_axis);
			if (cross < 0.0f) {
				flip_v = !flip_v;
			}

			// passing an angle is smaller than a vector, it can be reconstructed in the shader
			angle = x_axis.angle();

			// we don't want negative angles, as negative is used to encode flips.
			// This moves range from -PI to PI to 0 to TWO_PI
			if (angle < 0.0f) {
				angle += TWO_PI;
			}

		} // if transform needed

		// if horizontal flip, angle is shifted by 180 degrees
		if (flip_h) {
			angle += Math_PI;

			// mod to get back to 0 to TWO_PI range
			angle = fmodf(angle, TWO_PI);
		}

		// add 1 (to take care of zero floating point error with sign)
		angle += 1.0f;

		// flip if necessary to indicate a vertical flip in the shader
		if (flip_v) {
			angle *= -1.0f;
		}

		// light angle must be sent for each vert, instead as a single uniform in the uniform draw method
		// this has the benefit of enabling batching with light angles.
		for (int n = 0; n < 4; n++) {
			angles[n] = angle;
		}
	}

	// increment quad count
	bdata.total_quads++;
	bdata.total_verts += 4;

	return false;
}

// This function may be called MULTIPLE TIMES for each item, so needs to record how far it has got
bool RasterizerCanvasGLES3::prefill_joined_item(FillState &r_fill_state, int &r_command_start, RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename RasterizerStorageGLES3::Material *p_material) {
	// we will prefill batches and vertices ready for sending in one go to the vertex buffer
	int command_count = p_item->commands.size();
	RasterizerCanvas::Item::Command *const *commands = p_item->commands.ptr();

	// whether to multiply final modulate on the CPU, or pass it in the FVF and apply in the shader
	bool multiply_final_modulate = true;

	if (r_fill_state.is_single_item || bdata.use_modulate || bdata.use_large_verts) {
		multiply_final_modulate = false;
	}

	// start batch is a dummy batch (tex id -1) .. could be made more efficient
	if (!r_fill_state.curr_batch) {
		// allocate dummy batch on the stack, it should always get replaced
		// note that the rest of the structure is uninitialized, this should not matter
		// if the type is checked before anything else.
		r_fill_state.curr_batch = (Batch *)alloca(sizeof(Batch));
		r_fill_state.curr_batch->type = RasterizerStorageCommon::BT_DUMMY;

		// this is assumed to be the case
		//CRASH_COND (r_fill_state.transform_extra_command_number_p1);
	}

	// we need to return which command we got up to, so
	// store this outside the loop
	int command_num;

	// do as many commands as possible until the vertex buffer will be full up
	for (command_num = r_command_start; command_num < command_count; command_num++) {
		RasterizerCanvas::Item::Command *command = commands[command_num];

		switch (command->type) {
			default: {
				_prefill_default_batch(r_fill_state, command_num, *p_item);
			} break;
			case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
				// if the extra matrix has been sent already,
				// break this extra matrix software path (as we don't want to unset it on the GPU etc)
				if (r_fill_state.extra_matrix_sent) {
					_prefill_default_batch(r_fill_state, command_num, *p_item);

					// keep track of the combined matrix on the CPU in parallel, in case we use large vertex format
					RasterizerCanvas::Item::CommandTransform *transform = static_cast<RasterizerCanvas::Item::CommandTransform *>(command);
					const Transform2D &extra_matrix = transform->xform;
					r_fill_state.transform_combined = p_item->final_transform * extra_matrix;
				} else {
					// Extra matrix fast path.
					// Instead of sending the command immediately, we store the modified transform (in combined)
					// for software transform, and only flush this transform command if we NEED to (i.e. we want to
					// render some default commands)
					RasterizerCanvas::Item::CommandTransform *transform = static_cast<RasterizerCanvas::Item::CommandTransform *>(command);
					const Transform2D &extra_matrix = transform->xform;

					if (r_fill_state.is_single_item && !r_fill_state.use_attrib_transform) {
						// if we are using hardware transform mode, we have already sent the final transform,
						// so we only want to software transform the extra matrix
						r_fill_state.transform_combined = extra_matrix;
					} else {
						r_fill_state.transform_combined = p_item->final_transform * extra_matrix;
					}
					// after a transform command, always use some form of software transform (either the combined final + extra, or just the extra)
					// until we flush this dirty extra matrix because we need to render default commands.
					r_fill_state.transform_mode = _find_transform_mode(r_fill_state.transform_combined);

					// make a note of which command the dirty extra matrix is store in, so we can send it later
					// if necessary
					r_fill_state.transform_extra_command_number_p1 = command_num + 1; // plus 1 so we can test against zero
				}
			} break;
			case RasterizerCanvas::Item::Command::TYPE_RECT: {
				RasterizerCanvas::Item::CommandRect *rect = static_cast<RasterizerCanvas::Item::CommandRect *>(command);

				// unoptimized - could this be done once per batch / batch texture?
				bool send_light_angles = rect->normal_map != RID();

				bool buffer_full = false;

				// the template params must be explicit for compilation,
				// this forces building the multiple versions of the function.
				if (send_light_angles) {
					buffer_full = _prefill_rect<true>(rect, r_fill_state, r_command_start, command_num, command_count, commands, p_item, multiply_final_modulate);
				} else {
					buffer_full = _prefill_rect<false>(rect, r_fill_state, r_command_start, command_num, command_count, commands, p_item, multiply_final_modulate);
				}

				if (buffer_full) {
					return true;
				}

			} break;
			case RasterizerCanvas::Item::Command::TYPE_NINEPATCH: {
				RasterizerCanvas::Item::CommandNinePatch *np = static_cast<RasterizerCanvas::Item::CommandNinePatch *>(command);

				if ((np->axis_x != VisualServer::NINE_PATCH_STRETCH) || (np->axis_y != VisualServer::NINE_PATCH_STRETCH)) {
					// not accelerated
					_prefill_default_batch(r_fill_state, command_num, *p_item);
					continue;
				}

				// unoptimized - could this be done once per batch / batch texture?
				bool send_light_angles = np->normal_map != RID();

				bool buffer_full = false;

				if (send_light_angles) {
					buffer_full = _prefill_ninepatch<true>(np, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);
				} else {
					buffer_full = _prefill_ninepatch<false>(np, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);
				}

				if (buffer_full) {
					return true;
				}

			} break;

			case RasterizerCanvas::Item::Command::TYPE_LINE: {
				RasterizerCanvas::Item::CommandLine *line = static_cast<RasterizerCanvas::Item::CommandLine *>(command);

				if (line->width <= 1) {
					bool buffer_full = _prefill_line(line, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);

					if (buffer_full) {
						return true;
					}
				} else {
					// not accelerated
					_prefill_default_batch(r_fill_state, command_num, *p_item);
				}
			} break;

			case RasterizerCanvas::Item::Command::TYPE_POLYGON: {
				RasterizerCanvas::Item::CommandPolygon *polygon = static_cast<RasterizerCanvas::Item::CommandPolygon *>(command);
#ifdef GLES_OVER_GL
				// anti aliasing not accelerated .. it is problematic because it requires a 2nd line drawn around the outside of each
				// poly, which would require either a second list of indices or a second list of vertices for this step
				bool use_legacy_path = false;

				if (polygon->antialiased) {
					// anti aliasing is also not supported for software skinned meshes.
					// we can't easily revert, so we force software skinned meshes to run through
					// batching path with no AA.
					use_legacy_path = !bdata.settings_use_software_skinning || p_item->skeleton == RID();
				}

				if (use_legacy_path) {
					// not accelerated
					_prefill_default_batch(r_fill_state, command_num, *p_item);
				} else {
#endif
					// not using software skinning?
					if (!bdata.settings_use_software_skinning && state.using_skeleton) {
						// not accelerated
						_prefill_default_batch(r_fill_state, command_num, *p_item);
					} else {
						// unoptimized - could this be done once per batch / batch texture?
						bool send_light_angles = polygon->normal_map != RID();

						bool buffer_full = false;

						if (send_light_angles) {
							// polygon with light angles is not yet implemented
							// for batching .. this means software skinned with light angles won't work
							_prefill_default_batch(r_fill_state, command_num, *p_item);
						} else {
							buffer_full = _prefill_polygon<false>(polygon, r_fill_state, r_command_start, command_num, command_count, p_item, multiply_final_modulate);
						}

						if (buffer_full) {
							return true;
						}
					} // if not using hardware skinning path
#ifdef GLES_OVER_GL
				} // if not anti-aliased poly
#endif

			} break;
		}
	}

	// VERY IMPORTANT to return where we got to, because this func may be called multiple
	// times per item.
	// Don't miss out on this step by calling return earlier in the function without setting r_command_start.
	r_command_start = command_num;

	return false;
}

void RasterizerCanvasGLES3::flush_render_batches(RasterizerCanvas::Item *p_first_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename RasterizerStorageGLES3::Material *p_material, uint32_t p_sequence_batch_type_flags) {
	// some heuristic to decide whether to use colored verts.
	// feel free to tweak this.
	// this could use hysteresis, to prevent jumping between methods
	// .. however probably not necessary
	bdata.use_colored_vertices = false;

	RasterizerStorageCommon::FVF backup_fvf = bdata.fvf;

	// the batch type in this flush can override the fvf from the joined item.
	// The joined item uses the material to determine fvf, assuming a rect...
	// however with custom drawing, lines or polys may be drawn.
	// lines contain no color (this is stored in the batch), and polys contain vertex and color only.
	if (p_sequence_batch_type_flags & (RasterizerStorageCommon::BTF_LINE | RasterizerStorageCommon::BTF_LINE_AA)) {
		// do nothing, use the default regular FVF
		bdata.fvf = RasterizerStorageCommon::FVF_REGULAR;
	} else {
		// switch from regular to colored?
		if (bdata.fvf == RasterizerStorageCommon::FVF_REGULAR) {
			// only check whether to convert if there are quads (prevent divide by zero)
			// and we haven't decided to prevent color baking (due to e.g. MODULATE
			// being used in a shader)
			if (bdata.total_quads && !(bdata.joined_item_batch_flags & RasterizerStorageCommon::PREVENT_COLOR_BAKING)) {
				// minus 1 to prevent single primitives (ratio 1.0) always being converted to colored..
				// in that case it is slightly cheaper to just have the color as part of the batch
				float ratio = (float)(bdata.total_color_changes - 1) / (float)bdata.total_quads;

				// use bigger than or equal so that 0.0 threshold can force always using colored verts
				if (ratio >= bdata.settings_colored_vertex_format_threshold) {
					bdata.use_colored_vertices = true;
					bdata.fvf = RasterizerStorageCommon::FVF_COLOR;
				}
			}

			// if we used vertex colors
			if (bdata.vertex_colors.size()) {
				bdata.use_colored_vertices = true;
				bdata.fvf = RasterizerStorageCommon::FVF_COLOR;
			}

			// needs light angles?
			if (bdata.use_light_angles) {
				bdata.fvf = RasterizerStorageCommon::FVF_LIGHT_ANGLE;
			}
		}

		backup_fvf = bdata.fvf;
	} // if everything else except lines

	// translate if required to larger FVFs
	switch (bdata.fvf) {
		case RasterizerStorageCommon::FVF_UNBATCHED: // should not happen
			break;
		case RasterizerStorageCommon::FVF_REGULAR: // no change
			break;
		case RasterizerStorageCommon::FVF_COLOR: {
			// special case, where vertex colors are used (polys)
			if (!bdata.vertex_colors.size()) {
				_translate_batches_to_larger_FVF<BatchVertexColored, false, false, false>(p_sequence_batch_type_flags);
			} else {
				// normal, reduce number of batches by baking batch colors
				_translate_batches_to_vertex_colored_FVF();
			}
		} break;
		case RasterizerStorageCommon::FVF_LIGHT_ANGLE:
			_translate_batches_to_larger_FVF<BatchVertexLightAngled, true, false, false>(p_sequence_batch_type_flags);
			break;
		case RasterizerStorageCommon::FVF_MODULATED:
			_translate_batches_to_larger_FVF<BatchVertexModulated, true, true, false>(p_sequence_batch_type_flags);
			break;
		case RasterizerStorageCommon::FVF_LARGE:
			_translate_batches_to_larger_FVF<BatchVertexLarge, true, true, true>(p_sequence_batch_type_flags);
			break;
	}

	// send buffers to opengl
	_batch_upload_buffers();

	render_batches(p_current_clip, r_reclip, p_material);

	// if we overrode the fvf for lines, set it back to the joined item fvf
	bdata.fvf = backup_fvf;

	// overwrite source buffers with garbage if error checking
#ifdef RASTERIZER_EXTRA_CHECKS
	_debug_write_garbage();
#endif
}

void RasterizerCanvasGLES3::render_joined_item_commands(const BItemJoined &p_bij, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename RasterizerStorageGLES3::Material *p_material, bool p_lit, const RenderItemState &p_ris) {
	RasterizerCanvas::Item *item = nullptr;
	RasterizerCanvas::Item *first_item = bdata.item_refs[p_bij.first_item_ref].item;

	// fill_state and bdata have once off setup per joined item, and a smaller reset on flush
	FillState fill_state;
	fill_state.reset_joined_item(p_bij.is_single_item(), p_bij.use_attrib_transform());

	bdata.reset_joined_item();

	// should this joined item be using large FVF?
	if (p_bij.flags & RasterizerStorageCommon::USE_MODULATE_FVF) {
		bdata.use_modulate = true;
		bdata.fvf = RasterizerStorageCommon::FVF_MODULATED;
	}
	if (p_bij.flags & RasterizerStorageCommon::USE_LARGE_FVF) {
		bdata.use_modulate = true;
		bdata.use_large_verts = true;
		bdata.fvf = RasterizerStorageCommon::FVF_LARGE;
	}

	// make sure the jointed item flags state is up to date, as it is read indirectly in
	// a couple of places from the state rather than from the joined item.
	// we could alternatively make sure to only read directly from the joined item
	// during the render, but it is probably more bug future proof to make sure both
	// are up to date.
	bdata.joined_item_batch_flags = p_bij.flags;

	// in the special case of custom shaders that read from VERTEX (i.e. vertex position)
	// we want to disable software transform of extra matrix
	if (bdata.joined_item_batch_flags & RasterizerStorageCommon::PREVENT_VERTEX_BAKING) {
		fill_state.extra_matrix_sent = true;
	}

	for (unsigned int i = 0; i < p_bij.num_item_refs; i++) {
		const BItemRef &ref = bdata.item_refs[p_bij.first_item_ref + i];
		item = ref.item;

		if (!p_lit) {
			// if not lit we use the complex calculated final modulate
			fill_state.final_modulate = ref.final_modulate;
		} else {
			// if lit we ignore canvas modulate and just use the item modulate
			fill_state.final_modulate = item->final_modulate;
		}

		int command_count = item->commands.size();
		int command_start = 0;

		// ONCE OFF fill state setup, that will be retained over multiple calls to
		// prefill_joined_item()
		fill_state.transform_combined = item->final_transform;

		// calculate skeleton base inverse transform if required for software skinning
		// put in the fill state as this is readily accessible from the software skinner
		if (item->skeleton.is_valid() && bdata.settings_use_software_skinning && storage->skeleton_owner.owns(item->skeleton)) {
			typename RasterizerStorageGLES3::Skeleton *skeleton = nullptr;
			skeleton = storage->skeleton_owner.get(item->skeleton);

			if (skeleton->use_2d) {
				// with software skinning we still need to know the skeleton inverse transform, the other two aren't needed
				// but are left in for simplicity here
				Transform2D skeleton_transform = p_ris.item_group_base_transform * skeleton->base_transform_2d;
				fill_state.skeleton_base_inverse_xform = skeleton_transform.affine_inverse();
			}
		}

		// decide the initial transform mode, and make a backup
		// in orig_transform_mode in case we need to switch back
		if (fill_state.use_software_transform) {
			fill_state.transform_mode = _find_transform_mode(fill_state.transform_combined);
		} else {
			fill_state.transform_mode = TM_NONE;
		}
		fill_state.orig_transform_mode = fill_state.transform_mode;

		// keep track of when we added an extra matrix
		// so we can defer sending until we see a default command
		fill_state.transform_extra_command_number_p1 = 0;

		while (command_start < command_count) {
			// fill as many batches as possible (until all done, or the vertex buffer is full)
			bool bFull = prefill_joined_item(fill_state, command_start, item, p_current_clip, r_reclip, p_material);

			if (bFull) {
				// always pass first item (commands for default are always first item)
				flush_render_batches(first_item, p_current_clip, r_reclip, p_material, fill_state.sequence_batch_type_flags);

				// zero all the batch data ready for a new run
				bdata.reset_flush();

				// don't zero all the fill state, some may need to be preserved
				fill_state.reset_flush();
			}
		}
	}

	// flush if any left
	flush_render_batches(first_item, p_current_clip, r_reclip, p_material, fill_state.sequence_batch_type_flags);

	// zero all the batch data ready for a new run
	bdata.reset_flush();
}

void RasterizerCanvasGLES3::_legacy_canvas_item_render_commands(RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, typename RasterizerStorageGLES3::Material *p_material) {
	int command_count = p_item->commands.size();

	// legacy .. just create one massive batch and render everything as before
	bdata.batches.reset();
	Batch *batch = _batch_request_new();
	batch->type = RasterizerStorageCommon::BT_DEFAULT;
	batch->num_commands = command_count;
	batch->item = p_item;

	render_batches(p_current_clip, r_reclip, p_material);
	bdata.reset_flush();
}

void RasterizerCanvasGLES3::record_items(RasterizerCanvas::Item *p_item_list, int p_z) {
	while (p_item_list) {
		BSortItem *s = bdata.sort_items.request_with_grow();

		s->item = p_item_list;
		s->z_index = p_z;

		p_item_list = p_item_list->next;
	}
}

void RasterizerCanvasGLES3::join_sorted_items() {
	sort_items();

	int z = VS::CANVAS_ITEM_Z_MIN;
	_render_item_state.item_group_z = z;

	for (int s = 0; s < bdata.sort_items.size(); s++) {
		const BSortItem &si = bdata.sort_items[s];
		RasterizerCanvas::Item *ci = si.item;

		// change z?
		if (si.z_index != z) {
			z = si.z_index;

			// may not be required
			_render_item_state.item_group_z = z;

			// if z ranged lights are present, sometimes we have to disable joining over z_indices.
			// we do this here.
			// Note this restriction may be able to be relaxed with light bitfields, investigate!
			if (!bdata.join_across_z_indices) {
				_render_item_state.join_batch_break = true;
			}
		}

		bool join;

		if (_render_item_state.join_batch_break) {
			// always start a new batch for this item
			join = false;

			// could be another batch break (i.e. prevent NEXT item from joining this)
			// so we still need to run try_join_item
			// even though we know join is false.
			// also we need to run try_join_item for every item because it keeps the state up to date,
			// if we didn't run it the state would be out of date.
			try_join_item(ci, _render_item_state, _render_item_state.join_batch_break);
		} else {
			join = try_join_item(ci, _render_item_state, _render_item_state.join_batch_break);
		}

		// assume the first item will always return no join
		if (!join) {
			_render_item_state.joined_item = bdata.items_joined.request_with_grow();
			_render_item_state.joined_item->first_item_ref = bdata.item_refs.size();
			_render_item_state.joined_item->num_item_refs = 1;
			_render_item_state.joined_item->bounding_rect = ci->global_rect_cache;
			_render_item_state.joined_item->z_index = z;
			_render_item_state.joined_item->flags = bdata.joined_item_batch_flags;

			// we need some logic to prevent joining items that have vastly different batch types
			_render_item_state.joined_item_batch_type_flags_prev = _render_item_state.joined_item_batch_type_flags_curr;

			// add the reference
			BItemRef *r = bdata.item_refs.request_with_grow();
			r->item = ci;
			// we are storing final_modulate in advance per item reference
			// for baking into vertex colors.
			// this may not be ideal... as we are increasing the size of item reference,
			// but it is stupidly complex to calculate later, which would probably be slower.
			r->final_modulate = _render_item_state.final_modulate;
		} else {
			DEV_ASSERT(_render_item_state.joined_item != nullptr);
			_render_item_state.joined_item->num_item_refs += 1;
			_render_item_state.joined_item->bounding_rect = _render_item_state.joined_item->bounding_rect.merge(ci->global_rect_cache);

			BItemRef *r = bdata.item_refs.request_with_grow();
			r->item = ci;
			r->final_modulate = _render_item_state.final_modulate;

			// joined item references may introduce new flags
			_render_item_state.joined_item->flags |= bdata.joined_item_batch_flags;
		}

	} // for s through sort items
}

void RasterizerCanvasGLES3::sort_items() {
	// turned off?
	if (!bdata.settings_item_reordering_lookahead) {
		return;
	}

	for (int s = 0; s < bdata.sort_items.size() - 2; s++) {
		if (sort_items_from(s)) {
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
			bdata.stats_items_sorted++;
#endif
		}
	}
}

bool RasterizerCanvasGLES3::_sort_items_match(const BSortItem &p_a, const BSortItem &p_b) const {
	const RasterizerCanvas::Item *a = p_a.item;
	const RasterizerCanvas::Item *b = p_b.item;

	if (b->commands.size() != 1) {
		return false;
	}

	// tested outside function
	//	if (a->commands.size() != 1)
	//		return false;

	const RasterizerCanvas::Item::Command &cb = *b->commands[0];
	if (cb.type != RasterizerCanvas::Item::Command::TYPE_RECT) {
		return false;
	}

	const RasterizerCanvas::Item::Command &ca = *a->commands[0];
	// tested outside function
	//	if (ca.type != Item::Command::TYPE_RECT)
	//		return false;

	const RasterizerCanvas::Item::CommandRect *rect_a = static_cast<const RasterizerCanvas::Item::CommandRect *>(&ca);
	const RasterizerCanvas::Item::CommandRect *rect_b = static_cast<const RasterizerCanvas::Item::CommandRect *>(&cb);

	if (rect_a->texture != rect_b->texture) {
		return false;
	}

	/* ALTERNATIVE APPROACH NOT LIMITED TO RECTS
const RasterizerCanvas::Item::Command &ca = *a->commands[0];
const RasterizerCanvas::Item::Command &cb = *b->commands[0];

if (ca.type != cb.type)
	return false;

// do textures match?
switch (ca.type)
{
default:
	break;
case RasterizerCanvas::Item::Command::TYPE_RECT:
	{
		const RasterizerCanvas::Item::CommandRect *comm_a = static_cast<const RasterizerCanvas::Item::CommandRect *>(&ca);
		const RasterizerCanvas::Item::CommandRect *comm_b = static_cast<const RasterizerCanvas::Item::CommandRect *>(&cb);
		if (comm_a->texture != comm_b->texture)
			return false;
	}
	break;
case RasterizerCanvas::Item::Command::TYPE_POLYGON:
	{
		const RasterizerCanvas::Item::CommandPolygon *comm_a = static_cast<const RasterizerCanvas::Item::CommandPolygon *>(&ca);
		const RasterizerCanvas::Item::CommandPolygon *comm_b = static_cast<const RasterizerCanvas::Item::CommandPolygon *>(&cb);
		if (comm_a->texture != comm_b->texture)
			return false;
	}
	break;
}
*/

	return true;
}

bool RasterizerCanvasGLES3::sort_items_from(int p_start) {
#if defined(TOOLS_ENABLED) && defined(DEBUG_ENABLED)
	ERR_FAIL_COND_V((p_start + 1) >= bdata.sort_items.size(), false);
#endif

	const BSortItem &start = bdata.sort_items[p_start];
	int start_z = start.z_index;

	// check start is the right type for sorting
	if (start.item->commands.size() != 1) {
		return false;
	}
	const RasterizerCanvas::Item::Command &command_start = *start.item->commands[0];
	if (command_start.type != RasterizerCanvas::Item::Command::TYPE_RECT) {
		return false;
	}

	BSortItem &second = bdata.sort_items[p_start + 1];
	if (second.z_index != start_z) {
		// no sorting across z indices (for now)
		return false;
	}

	// if the neighbours are already a good match
	if (_sort_items_match(start, second)) // order is crucial, start first
	{
		return false;
	}

	// local cached aabb
	Rect2 second_AABB = second.item->global_rect_cache;

	// if the start and 2nd items overlap, can do no more
	if (start.item->global_rect_cache.intersects(second_AABB)) {
		return false;
	}

	// disallow sorting over copy back buffer
	if (second.item->copy_back_buffer) {
		return false;
	}

	// which neighbour to test
	int test_last = 2 + bdata.settings_item_reordering_lookahead;
	for (int test = 2; test < test_last; test++) {
		int test_sort_item_id = p_start + test;

		// if we've got to the end of the list, can't sort any more, give up
		if (test_sort_item_id >= bdata.sort_items.size()) {
			return false;
		}

		BSortItem *test_sort_item = &bdata.sort_items[test_sort_item_id];

		// across z indices?
		if (test_sort_item->z_index != start_z) {
			return false;
		}

		RasterizerCanvas::Item *test_item = test_sort_item->item;

		// if the test item overlaps the second item, we can't swap, AT ALL
		// because swapping an item OVER this one would cause artefacts
		if (second_AABB.intersects(test_item->global_rect_cache)) {
			return false;
		}

		// do they match?
		if (!_sort_items_match(start, *test_sort_item)) // order is crucial, start first
		{
			continue;
		}

		// we can only swap if there are no AABB overlaps with sandwiched neighbours
		bool ok = true;

		// start from 2, no need to check 1 as the second has already been checked against this item
		// in the intersection test above
		for (int sn = 2; sn < test; sn++) {
			BSortItem *sandwich_neighbour = &bdata.sort_items[p_start + sn];
			if (test_item->global_rect_cache.intersects(sandwich_neighbour->item->global_rect_cache)) {
				ok = false;
				break;
			}
		}
		if (!ok) {
			continue;
		}

		// it is ok to exchange them!
		BSortItem temp;
		temp.assign(second);
		second.assign(*test_sort_item);
		test_sort_item->assign(temp);

		return true;
	} // for test

	return false;
}

void RasterizerCanvasGLES3::_software_transform_vertex(BatchVector2 &r_v, const Transform2D &p_tr) const {
	Vector2 vc(r_v.x, r_v.y);
	vc = p_tr.xform(vc);
	r_v.set(vc);
}

void RasterizerCanvasGLES3::_software_transform_vertex(Vector2 &r_v, const Transform2D &p_tr) const {
	r_v = p_tr.xform(r_v);
}

void RasterizerCanvasGLES3::_translate_batches_to_vertex_colored_FVF() {
	// zeros the size and sets up how big each unit is
	bdata.unit_vertices.prepare(sizeof(BatchVertexColored));

	const BatchColor *source_vertex_colors = &bdata.vertex_colors[0];
	DEV_ASSERT(bdata.vertex_colors.size() == bdata.vertices.size());

	int num_verts = bdata.vertices.size();

	for (int n = 0; n < num_verts; n++) {
		const BatchVertex &bv = bdata.vertices[n];

		BatchVertexColored *cv = (BatchVertexColored *)bdata.unit_vertices.request();

		cv->pos = bv.pos;
		cv->uv = bv.uv;
		cv->col = *source_vertex_colors++;
	}
}

// Translation always involved adding color to the FVF, which enables
// joining of batches that have different colors.
// There is a trade off. Non colored verts are smaller so work faster, but
// there comes a point where it is better to just use colored verts to avoid lots of
// batches.
// In addition this can optionally add light angles to the FVF, necessary for normal mapping.

template <class BATCH_VERTEX_TYPE, bool INCLUDE_LIGHT_ANGLES, bool INCLUDE_MODULATE, bool INCLUDE_LARGE>
void RasterizerCanvasGLES3::_translate_batches_to_larger_FVF(uint32_t p_sequence_batch_type_flags) {
	bool include_poly_color = false;

	// we ONLY want to include the color verts in translation when using polys,
	// as rects do not write vertex colors, only colors per batch.
	if (p_sequence_batch_type_flags & RasterizerStorageCommon::BTF_POLY) {
		include_poly_color = INCLUDE_LIGHT_ANGLES | INCLUDE_MODULATE | INCLUDE_LARGE;
	}

	// zeros the size and sets up how big each unit is
	bdata.unit_vertices.prepare(sizeof(BATCH_VERTEX_TYPE));
	bdata.batches_temp.reset();

	// As the vertices_colored and batches_temp are 'mirrors' of the non-colored version,
	// the sizes should be equal, and allocations should never fail. Hence the use of debug
	// asserts to check program flow, these should not occur at runtime unless the allocation
	// code has been altered.
	DEV_ASSERT(bdata.unit_vertices.max_size() == bdata.vertices.max_size());
	DEV_ASSERT(bdata.batches_temp.max_size() == bdata.batches.max_size());

	Color curr_col(-1.0f, -1.0f, -1.0f, -1.0f);

	Batch *dest_batch = nullptr;

	const BatchColor *source_vertex_colors = &bdata.vertex_colors[0];
	const float *source_light_angles = &bdata.light_angles[0];
	const BatchColor *source_vertex_modulates = &bdata.vertex_modulates[0];
	const BatchTransform *source_vertex_transforms = &bdata.vertex_transforms[0];

	// translate the batches into vertex colored batches
	for (int n = 0; n < bdata.batches.size(); n++) {
		const Batch &source_batch = bdata.batches[n];

		// does source batch use light angles?
		const BatchTex &btex = bdata.batch_textures[source_batch.batch_texture_id];
		bool source_batch_uses_light_angles = btex.RID_normal != RID();

		bool needs_new_batch = true;

		if (dest_batch) {
			if (dest_batch->type == source_batch.type) {
				if (source_batch.type == RasterizerStorageCommon::BT_RECT) {
					if (dest_batch->batch_texture_id == source_batch.batch_texture_id) {
						// add to previous batch
						dest_batch->num_commands += source_batch.num_commands;
						needs_new_batch = false;

						// create the colored verts (only if not default)
						int first_vert = source_batch.first_vert;
						int num_verts = source_batch.get_num_verts();
						int end_vert = first_vert + num_verts;

						for (int v = first_vert; v < end_vert; v++) {
							RAST_DEV_DEBUG_ASSERT(bdata.vertices.size());
							const BatchVertex &bv = bdata.vertices[v];
							BATCH_VERTEX_TYPE *cv = (BATCH_VERTEX_TYPE *)bdata.unit_vertices.request();
							RAST_DEBUG_ASSERT(cv);
							cv->pos = bv.pos;
							cv->uv = bv.uv;
							cv->col = source_batch.color;

							if (INCLUDE_LIGHT_ANGLES) {
								RAST_DEV_DEBUG_ASSERT(bdata.light_angles.size());
								// this is required to allow compilation with non light angle vertex.
								// it should be compiled out.
								BatchVertexLightAngled *lv = (BatchVertexLightAngled *)cv;
								if (source_batch_uses_light_angles) {
									lv->light_angle = *source_light_angles++;
								} else {
									lv->light_angle = 0.0f; // dummy, unused in vertex shader (could possibly be left uninitialized, but probably bad idea)
								}
							} // if including light angles

							if (INCLUDE_MODULATE) {
								RAST_DEV_DEBUG_ASSERT(bdata.vertex_modulates.size());
								BatchVertexModulated *mv = (BatchVertexModulated *)cv;
								mv->modulate = *source_vertex_modulates++;
							} // including modulate

							if (INCLUDE_LARGE) {
								RAST_DEV_DEBUG_ASSERT(bdata.vertex_transforms.size());
								BatchVertexLarge *lv = (BatchVertexLarge *)cv;
								lv->transform = *source_vertex_transforms++;
							} // if including large
						}
					} // textures match
				} else {
					// default
					// we can still join, but only under special circumstances
					// does this ever happen? not sure at this stage, but left for future expansion
					uint32_t source_last_command = source_batch.first_command + source_batch.num_commands;
					if (source_last_command == dest_batch->first_command) {
						dest_batch->num_commands += source_batch.num_commands;
						needs_new_batch = false;
					} // if the commands line up exactly
				}
			} // if both batches are the same type

		} // if dest batch is valid

		if (needs_new_batch) {
			dest_batch = bdata.batches_temp.request();
			RAST_DEBUG_ASSERT(dest_batch);

			*dest_batch = source_batch;

			// create the colored verts (only if not default)
			if (source_batch.type != RasterizerStorageCommon::BT_DEFAULT) {
				int first_vert = source_batch.first_vert;
				int num_verts = source_batch.get_num_verts();
				int end_vert = first_vert + num_verts;

				for (int v = first_vert; v < end_vert; v++) {
					RAST_DEV_DEBUG_ASSERT(bdata.vertices.size());
					const BatchVertex &bv = bdata.vertices[v];
					BATCH_VERTEX_TYPE *cv = (BATCH_VERTEX_TYPE *)bdata.unit_vertices.request();
					RAST_DEBUG_ASSERT(cv);
					cv->pos = bv.pos;
					cv->uv = bv.uv;

					// polys are special, they can have per vertex colors
					if (!include_poly_color) {
						cv->col = source_batch.color;
					} else {
						RAST_DEV_DEBUG_ASSERT(bdata.vertex_colors.size());
						cv->col = *source_vertex_colors++;
					}

					if (INCLUDE_LIGHT_ANGLES) {
						RAST_DEV_DEBUG_ASSERT(bdata.light_angles.size());
						// this is required to allow compilation with non light angle vertex.
						// it should be compiled out.
						BatchVertexLightAngled *lv = (BatchVertexLightAngled *)cv;
						if (source_batch_uses_light_angles) {
							lv->light_angle = *source_light_angles++;
						} else {
							lv->light_angle = 0.0f; // dummy, unused in vertex shader (could possibly be left uninitialized, but probably bad idea)
						}
					} // if using light angles

					if (INCLUDE_MODULATE) {
						RAST_DEV_DEBUG_ASSERT(bdata.vertex_modulates.size());
						BatchVertexModulated *mv = (BatchVertexModulated *)cv;
						mv->modulate = *source_vertex_modulates++;
					} // including modulate

					if (INCLUDE_LARGE) {
						RAST_DEV_DEBUG_ASSERT(bdata.vertex_transforms.size());
						BatchVertexLarge *lv = (BatchVertexLarge *)cv;
						lv->transform = *source_vertex_transforms++;
					} // if including large
				}
			}
		}
	}

	// copy the temporary batches to the master batch list (this could be avoided but it makes the code cleaner)
	bdata.batches.copy_from(bdata.batches_temp);
}

bool RasterizerCanvasGLES3::_disallow_item_join_if_batch_types_too_different(RenderItemState &r_ris, uint32_t btf_allowed) {
	r_ris.joined_item_batch_type_flags_curr |= btf_allowed;

	bool disallow = false;

	if (r_ris.joined_item_batch_type_flags_prev & (~btf_allowed)) {
		disallow = true;
	}

	return disallow;
}

bool RasterizerCanvasGLES3::_detect_item_batch_break(RenderItemState &r_ris, RasterizerCanvas::Item *p_ci, bool &r_batch_break) {
	int command_count = p_ci->commands.size();

	// Any item that contains commands that are default
	// (i.e. not handled by software transform and the batching renderer) should not be joined.

	// ALSO batched types that differ in what the vertex format is needed to be should not be
	// joined.

	// In order to work this out, it does a lookahead through the commands,
	// which could potentially be very expensive. As such it makes sense to put a limit on this
	// to some small number, which will catch nearly all cases which need joining,
	// but not be overly expensive in the case of items with large numbers of commands.

	// It is hard to know what this number should be, empirically,
	// and this has not been fully investigated. It works to join single sprite items when set to 1 or above.
	// Note that there is a cost to increasing this because it has to look in advance through
	// the commands.
	// On the other hand joining items where possible will usually be better up to a certain
	// number where the cost of software transform is higher than separate drawcalls with hardware
	// transform.

	// if there are more than this number of commands in the item, we
	// don't allow joining (separate state changes, and hardware transform)
	// This is set to quite a conservative (low) number until investigated properly.
	// const int MAX_JOIN_ITEM_COMMANDS = 16;

	r_ris.joined_item_batch_type_flags_curr = 0;

	if (command_count > bdata.settings_max_join_item_commands) {
		return true;
	} else {
		RasterizerCanvas::Item::Command *const *commands = p_ci->commands.ptr();

		// run through the commands looking for one that could prevent joining
		for (int command_num = 0; command_num < command_count; command_num++) {
			RasterizerCanvas::Item::Command *command = commands[command_num];
			RAST_DEBUG_ASSERT(command);

			switch (command->type) {
				default: {
					//r_batch_break = true;
					return true;
				} break;
				case RasterizerCanvas::Item::Command::TYPE_LINE: {
					// special case, only batches certain lines
					RasterizerCanvas::Item::CommandLine *line = static_cast<RasterizerCanvas::Item::CommandLine *>(command);

					if (line->width > 1) {
						//r_batch_break = true;
						return true;
					}

					if (_disallow_item_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_LINE | RasterizerStorageCommon::BTF_LINE_AA)) {
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_POLYGON: {
					// only allow polygons to join if they aren't skeleton
					RasterizerCanvas::Item::CommandPolygon *poly = static_cast<RasterizerCanvas::Item::CommandPolygon *>(command);

#ifdef GLES_OVER_GL
					// anti aliasing not accelerated
					if (poly->antialiased) {
						return true;
					}
#endif

					// light angles not yet implemented, treat as default
					if (poly->normal_map != RID()) {
						return true;
					}

					if (!bdata.settings_use_software_skinning && poly->bones.size()) {
						return true;
					}

					if (_disallow_item_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_POLY)) {
						//r_batch_break = true;
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_RECT: {
					if (_disallow_item_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_RECT)) {
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_NINEPATCH: {
					// do not handle tiled ninepatches, these can't be batched and need to use legacy method
					RasterizerCanvas::Item::CommandNinePatch *np = static_cast<RasterizerCanvas::Item::CommandNinePatch *>(command);
					if ((np->axis_x != VisualServer::NINE_PATCH_STRETCH) || (np->axis_y != VisualServer::NINE_PATCH_STRETCH)) {
						return true;
					}

					if (_disallow_item_join_if_batch_types_too_different(r_ris, RasterizerStorageCommon::BTF_RECT)) {
						return true;
					}
				} break;
				case RasterizerCanvas::Item::Command::TYPE_TRANSFORM: {
					// compatible with all types
				} break;
			} // switch

		} // for through commands

	} // else

	// special case, back buffer copy, so don't join
	if (p_ci->copy_back_buffer) {
		return true;
	}

	return false;
}
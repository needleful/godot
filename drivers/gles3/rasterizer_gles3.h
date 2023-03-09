/*************************************************************************/
/*  rasterizer_gles3.h                                                   */
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

#ifndef RASTERIZER_GLES3_H
#define RASTERIZER_GLES3_H

#include "rasterizer_canvas_gles3.h"
#include "rasterizer_scene_gles3.h"
#include "rasterizer_storage_gles3.h"

#define Rasterizer RasterizerGLES3

class RasterizerGLES3 {
protected:
	static Rasterizer *(*_create_func)();

public:
	static Rasterizer *create();
	static Rasterizer *_create_current();

	RasterizerStorageGLES3 *storage;
	RasterizerCanvasGLES3 *canvas;
	RasterizerSceneGLES3 *scene;

	double time_total;
	float time_scale;

public:
	RasterizerStorage *get_storage();
	RasterizerCanvas *get_canvas();
	RasterizerScene *get_scene();

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true);
	void set_shader_time_scale(float p_scale);

	void initialize();
	void begin_frame(double frame_step);
	void set_current_render_target(RID p_render_target);
	void restore_render_target(bool p_3d_was_drawn);
	void clear_render_target(const Color &p_color);
	void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0);
	void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample);
	void end_frame(bool p_swap_buffers);
	void finalize();

	static Error is_viable();
	static void make_current();
	static void register_config();

	bool is_low_end() const { return false; }

	static bool gl_check_errors();

	RasterizerGLES3();
	~RasterizerGLES3();
};

#endif // RASTERIZER_GLES3_H

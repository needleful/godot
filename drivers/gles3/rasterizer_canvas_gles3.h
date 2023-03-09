/*************************************************************************/
/*  rasterizer_canvas_gles3.h                                            */
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

#ifndef RASTERIZER_CANVAS_GLES3_H
#define RASTERIZER_CANVAS_GLES3_H

#include "rasterizer_storage_gles3.h"

#include "servers/visual_server.h"

#include "drivers/gles_common/rasterizer_array.h"
#include "drivers/gles_common/rasterizer_asserts.h"
#include "drivers/gles_common/rasterizer_storage_common.h"

#include "shaders/canvas.glsl.gen.h"
#include "shaders/canvas_shadow.glsl.gen.h"
#include "shaders/lens_distorted.glsl.gen.h"

class RasterizerSceneGLES3;

#define RasterizerCanvas RasterizerCanvasGLES3

class RasterizerCanvasGLES3 {
public:
	enum CanvasRectFlags {

		CANVAS_RECT_REGION = 1,
		CANVAS_RECT_TILE = 2,
		CANVAS_RECT_FLIP_H = 4,
		CANVAS_RECT_FLIP_V = 8,
		CANVAS_RECT_TRANSPOSE = 16,
		CANVAS_RECT_CLIP_UV = 32
	};

	struct Light : public RID_Data {
		bool enabled;
		Color color;
		Transform2D xform;
		float height;
		float energy;
		float scale;
		int z_min;
		int z_max;
		int layer_min;
		int layer_max;
		int item_mask;
		int item_shadow_mask;
		VS::CanvasLightMode mode;
		RID texture;
		Vector2 texture_offset;
		RID canvas;
		RID shadow_buffer;
		int shadow_buffer_size;
		float shadow_gradient_length;
		VS::CanvasLightShadowFilter shadow_filter;
		Color shadow_color;
		float shadow_smooth;

		void *texture_cache; // implementation dependent
		Rect2 rect_cache;
		Transform2D xform_cache;
		float radius_cache; //used for shadow far plane
		CameraMatrix shadow_matrix_cache;

		Transform2D light_shader_xform;
		Vector2 light_shader_pos;

		Light *shadows_next_ptr;
		Light *filter_next_ptr;
		Light *next_ptr;
		Light *mask_next_ptr;

		RID light_internal;

		Light() {
			enabled = true;
			color = Color(1, 1, 1);
			shadow_color = Color(0, 0, 0, 0);
			height = 0;
			z_min = -1024;
			z_max = 1024;
			layer_min = 0;
			layer_max = 0;
			item_mask = 1;
			scale = 1.0;
			energy = 1.0;
			item_shadow_mask = 1;
			mode = VS::CANVAS_LIGHT_MODE_ADD;
			texture_cache = nullptr;
			next_ptr = nullptr;
			mask_next_ptr = nullptr;
			filter_next_ptr = nullptr;
			shadow_buffer_size = 2048;
			shadow_gradient_length = 0;
			shadow_filter = VS::CANVAS_LIGHT_FILTER_NONE;
			shadow_smooth = 0.0;
		}
	};
	struct Item : public RID_Data {
		struct Command {
			enum Type {

				TYPE_LINE,
				TYPE_POLYLINE,
				TYPE_RECT,
				TYPE_NINEPATCH,
				TYPE_PRIMITIVE,
				TYPE_POLYGON,
				TYPE_MESH,
				TYPE_MULTIMESH,
				TYPE_PARTICLES,
				TYPE_CIRCLE,
				TYPE_TRANSFORM,
				TYPE_CLIP_IGNORE,
			};

			Type type;
			virtual ~Command() {}
		};

		struct CommandLine : public Command {
			Point2 from, to;
			Color color;
			float width;
			bool antialiased;
			CommandLine() { type = TYPE_LINE; }
		};
		struct CommandPolyLine : public Command {
			bool antialiased;
			bool multiline;
			Vector<Point2> triangles;
			Vector<Color> triangle_colors;
			Vector<Point2> lines;
			Vector<Color> line_colors;
			CommandPolyLine() {
				type = TYPE_POLYLINE;
				antialiased = false;
				multiline = false;
			}
		};

		struct CommandRect : public Command {
			Rect2 rect;
			RID texture;
			RID normal_map;
			Color modulate;
			Rect2 source;
			uint8_t flags;

			CommandRect() {
				flags = 0;
				type = TYPE_RECT;
			}
		};

		struct CommandNinePatch : public Command {
			Rect2 rect;
			Rect2 source;
			RID texture;
			RID normal_map;
			float margin[4];
			bool draw_center;
			Color color;
			VS::NinePatchAxisMode axis_x;
			VS::NinePatchAxisMode axis_y;
			CommandNinePatch() {
				draw_center = true;
				type = TYPE_NINEPATCH;
			}
		};

		struct CommandPrimitive : public Command {
			Vector<Point2> points;
			Vector<Point2> uvs;
			Vector<Color> colors;
			RID texture;
			RID normal_map;
			float width;

			CommandPrimitive() {
				type = TYPE_PRIMITIVE;
				width = 1;
			}
		};

		struct CommandPolygon : public Command {
			Vector<int> indices;
			Vector<Point2> points;
			Vector<Point2> uvs;
			Vector<Color> colors;
			Vector<int> bones;
			Vector<float> weights;
			RID texture;
			RID normal_map;
			int count;
			bool antialiased;
			bool antialiasing_use_indices;

			CommandPolygon() {
				type = TYPE_POLYGON;
				count = 0;
			}
		};

		struct CommandMesh : public Command {
			RID mesh;
			RID texture;
			RID normal_map;
			Transform2D transform;
			Color modulate;
			CommandMesh() { type = TYPE_MESH; }
		};

		struct CommandMultiMesh : public Command {
			RID multimesh;
			RID texture;
			RID normal_map;
			CommandMultiMesh() { type = TYPE_MULTIMESH; }
		};

		struct CommandParticles : public Command {
			RID particles;
			RID texture;
			RID normal_map;
			CommandParticles() { type = TYPE_PARTICLES; }
		};

		struct CommandCircle : public Command {
			Point2 pos;
			float radius;
			Color color;
			CommandCircle() { type = TYPE_CIRCLE; }
		};

		struct CommandTransform : public Command {
			Transform2D xform;
			CommandTransform() { type = TYPE_TRANSFORM; }
		};

		struct CommandClipIgnore : public Command {
			bool ignore;
			CommandClipIgnore() {
				type = TYPE_CLIP_IGNORE;
				ignore = false;
			}
		};

		struct ViewportRender {
			VisualServer *owner;
			void *udata;
			Rect2 rect;
		};

		Transform2D xform;
		bool clip : 1;
		bool visible : 1;
		bool behind : 1;
		bool update_when_visible : 1;
		bool distance_field : 1;
		bool light_masked : 1;
		mutable bool custom_rect : 1;
		mutable bool rect_dirty : 1;

		Vector<Command *> commands;
		mutable Rect2 rect;
		RID material;
		RID skeleton;

		//VS::MaterialBlendMode blend_mode;
		int32_t light_mask;
		mutable uint32_t skeleton_revision;

		Item *next;

		struct CopyBackBuffer {
			Rect2 rect;
			Rect2 screen_rect;
			bool full;
		};
		CopyBackBuffer *copy_back_buffer;

		Color final_modulate;
		Transform2D final_transform;
		Rect2 final_clip_rect;
		Item *final_clip_owner;
		Item *material_owner;
		ViewportRender *vp_render;

		Rect2 global_rect_cache;

		const Rect2 &get_rect() const {
			if (custom_rect) {
				return rect;
			}
			if (!rect_dirty && !update_when_visible) {
				if (skeleton == RID()) {
					return rect;
				} else {
					// special case for skeletons
					uint32_t rev = RasterizerStorageGLES3::base_singleton->skeleton_get_revision(skeleton);
					if (rev == skeleton_revision) {
						// no change to the skeleton since we last calculated the bounding rect
						return rect;
					} else {
						// We need to recalculate.
						// Mark as done for next time.
						skeleton_revision = rev;
					}
				}
			}

			//must update rect
			int s = commands.size();
			if (s == 0) {
				rect = Rect2();
				rect_dirty = false;
				return rect;
			}

			Transform2D xf;
			bool found_xform = false;
			bool first = true;

			const Item::Command *const *cmd = &commands[0];

			for (int i = 0; i < s; i++) {
				const Item::Command *c = cmd[i];
				Rect2 r;

				switch (c->type) {
					case Item::Command::TYPE_LINE: {
						const Item::CommandLine *line = static_cast<const Item::CommandLine *>(c);
						r.position = line->from;
						r.expand_to(line->to);
					} break;
					case Item::Command::TYPE_POLYLINE: {
						const Item::CommandPolyLine *pline = static_cast<const Item::CommandPolyLine *>(c);
						if (pline->triangles.size()) {
							for (int j = 0; j < pline->triangles.size(); j++) {
								if (j == 0) {
									r.position = pline->triangles[j];
								} else {
									r.expand_to(pline->triangles[j]);
								}
							}
						} else {
							for (int j = 0; j < pline->lines.size(); j++) {
								if (j == 0) {
									r.position = pline->lines[j];
								} else {
									r.expand_to(pline->lines[j]);
								}
							}
						}

					} break;
					case Item::Command::TYPE_RECT: {
						const Item::CommandRect *crect = static_cast<const Item::CommandRect *>(c);
						r = crect->rect;

					} break;
					case Item::Command::TYPE_NINEPATCH: {
						const Item::CommandNinePatch *style = static_cast<const Item::CommandNinePatch *>(c);
						r = style->rect;
					} break;
					case Item::Command::TYPE_PRIMITIVE: {
						const Item::CommandPrimitive *primitive = static_cast<const Item::CommandPrimitive *>(c);
						r.position = primitive->points[0];
						for (int j = 1; j < primitive->points.size(); j++) {
							r.expand_to(primitive->points[j]);
						}
					} break;
					case Item::Command::TYPE_POLYGON: {
						const Item::CommandPolygon *polygon = static_cast<const Item::CommandPolygon *>(c);
						int l = polygon->points.size();
						const Point2 *pp = &polygon->points[0];
						r.position = pp[0];
						for (int j = 1; j < l; j++) {
							r.expand_to(pp[j]);
						}

						if (skeleton != RID()) {
							// calculate bone AABBs
							int bone_count = RasterizerStorageGLES3::base_singleton->skeleton_get_bone_count(skeleton);

							Vector<Rect2> bone_aabbs;
							bone_aabbs.resize(bone_count);
							Rect2 *bptr = bone_aabbs.ptrw();

							for (int j = 0; j < bone_count; j++) {
								bptr[j].size = Vector2(-1, -1); //negative means unused
							}
							if (l && polygon->bones.size() == l * 4 && polygon->weights.size() == polygon->bones.size()) {
								for (int j = 0; j < l; j++) {
									Point2 p = pp[j];
									for (int k = 0; k < 4; k++) {
										int idx = polygon->bones[j * 4 + k];
										float w = polygon->weights[j * 4 + k];
										if (w == 0) {
											continue;
										}

										if (bptr[idx].size.x < 0) {
											//first
											bptr[idx] = Rect2(p, Vector2(0.00001, 0.00001));
										} else {
											bptr[idx].expand_to(p);
										}
									}
								}

								Rect2 aabb;
								bool first_bone = true;
								for (int j = 0; j < bone_count; j++) {
									Transform2D mtx = RasterizerStorageGLES3::base_singleton->skeleton_bone_get_transform_2d(skeleton, j);
									Rect2 baabb = mtx.xform(bone_aabbs[j]);

									if (first_bone) {
										aabb = baabb;
										first_bone = false;
									} else {
										aabb = aabb.merge(baabb);
									}
								}

								r = r.merge(aabb);
							}
						}

					} break;
					case Item::Command::TYPE_MESH: {
						const Item::CommandMesh *mesh = static_cast<const Item::CommandMesh *>(c);
						AABB aabb = RasterizerStorageGLES3::base_singleton->mesh_get_aabb(mesh->mesh, RID());

						r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

					} break;
					case Item::Command::TYPE_MULTIMESH: {
						const Item::CommandMultiMesh *multimesh = static_cast<const Item::CommandMultiMesh *>(c);
						AABB aabb = RasterizerStorageGLES3::base_singleton->multimesh_get_aabb(multimesh->multimesh);

						r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

					} break;
					case Item::Command::TYPE_PARTICLES: {
						const Item::CommandParticles *particles_cmd = static_cast<const Item::CommandParticles *>(c);
						if (particles_cmd->particles.is_valid()) {
							AABB aabb = RasterizerStorageGLES3::base_singleton->particles_get_aabb(particles_cmd->particles);
							r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
						}

					} break;
					case Item::Command::TYPE_CIRCLE: {
						const Item::CommandCircle *circle = static_cast<const Item::CommandCircle *>(c);
						r.position = Point2(-circle->radius, -circle->radius) + circle->pos;
						r.size = Point2(circle->radius * 2.0, circle->radius * 2.0);
					} break;
					case Item::Command::TYPE_TRANSFORM: {
						const Item::CommandTransform *transform = static_cast<const Item::CommandTransform *>(c);
						xf = transform->xform;
						found_xform = true;
						continue;
					} break;

					case Item::Command::TYPE_CLIP_IGNORE: {
					} break;
				}

				if (found_xform) {
					r = xf.xform(r);
				}

				if (first) {
					rect = r;
					first = false;
				} else {
					rect = rect.merge(r);
				}
			}

			rect_dirty = false;
			return rect;
		}

		void clear() {
			for (int i = 0; i < commands.size(); i++) {
				memdelete(commands[i]);
			}
			commands.clear();
			clip = false;
			rect_dirty = true;
			final_clip_owner = nullptr;
			material_owner = nullptr;
			light_masked = false;
		}
		Item() {
			light_mask = 1;
			skeleton_revision = 0;
			vp_render = nullptr;
			next = nullptr;
			final_clip_owner = nullptr;
			clip = false;
			final_modulate = Color(1, 1, 1, 1);
			visible = true;
			rect_dirty = true;
			custom_rect = false;
			behind = false;
			material_owner = nullptr;
			copy_back_buffer = nullptr;
			distance_field = false;
			light_masked = false;
			update_when_visible = false;
		}
		virtual ~Item() {
			clear();
			if (copy_back_buffer) {
				memdelete(copy_back_buffer);
			}
		}
	};

	struct LightOccluderInstance : public RID_Data {
		bool enabled;
		RID canvas;
		RID polygon;
		RID polygon_buffer;
		Rect2 aabb_cache;
		Transform2D xform;
		Transform2D xform_cache;
		int light_mask;
		VS::CanvasOccluderPolygonCullMode cull_cache;

		LightOccluderInstance *next;

		LightOccluderInstance() {
			enabled = true;
			next = nullptr;
			light_mask = 1;
			cull_cache = VS::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
		}
	};

	struct CanvasItemUBO {
		float projection_matrix[16];
		float time;
		uint8_t padding[12];
	};

	RasterizerSceneGLES3 *scene_render;

	struct Data {
		enum { NUM_QUAD_ARRAY_VARIATIONS = 8 };

		GLuint canvas_quad_vertices;
		GLuint canvas_quad_array;

		GLuint polygon_buffer;
		GLuint polygon_buffer_quad_arrays[NUM_QUAD_ARRAY_VARIATIONS];
		GLuint polygon_buffer_pointer_array;
		GLuint polygon_index_buffer;

		GLuint particle_quad_vertices;
		GLuint particle_quad_array;

		uint32_t polygon_buffer_size;
		uint32_t polygon_index_buffer_size;

	} data;

	struct State {
		CanvasItemUBO canvas_item_ubo_data;
		GLuint canvas_item_ubo;
		bool canvas_texscreen_used;
		CanvasShaderGLES3 canvas_shader;
		CanvasShadowShaderGLES3 canvas_shadow_shader;
		LensDistortedShaderGLES3 lens_shader;

		bool using_texture_rect;
		bool using_ninepatch;

		bool using_light_angle;
		bool using_modulate;
		bool using_large_vertex;

		RID current_tex;
		RID current_normal;
		RasterizerStorageGLES3::Texture *current_tex_ptr;

		Transform vp;

		Color canvas_item_modulate;
		Transform2D extra_matrix;
		Transform2D final_transform;
		bool using_skeleton;
		Transform2D skeleton_transform;
		Transform2D skeleton_transform_inverse;

	} state;

	// BATCHING

	// used to determine whether we use hardware transform (none)
	// software transform all verts, or software transform just a translate
	// (no rotate or scale)
	enum TransformMode {
		TM_NONE,
		TM_ALL,
		TM_TRANSLATE,
	};

	// pod versions of vector and color and RID, need to be 32 bit for vertex format
	struct BatchVector2 {
		float x, y;
		void set(float xx, float yy) {
			x = xx;
			y = yy;
		}
		void set(const Vector2 &p_o) {
			x = p_o.x;
			y = p_o.y;
		}
		void to(Vector2 &r_o) const {
			r_o.x = x;
			r_o.y = y;
		}
	};

	struct BatchColor {
		float r, g, b, a;
		void set_white() {
			r = 1.0f;
			g = 1.0f;
			b = 1.0f;
			a = 1.0f;
		}
		void set(const Color &p_c) {
			r = p_c.r;
			g = p_c.g;
			b = p_c.b;
			a = p_c.a;
		}
		void set(float rr, float gg, float bb, float aa) {
			r = rr;
			g = gg;
			b = bb;
			a = aa;
		}
		bool operator==(const BatchColor &p_c) const {
			return (r == p_c.r) && (g == p_c.g) && (b == p_c.b) && (a == p_c.a);
		}
		bool operator!=(const BatchColor &p_c) const { return (*this == p_c) == false; }
		bool equals(const Color &p_c) const {
			return (r == p_c.r) && (g == p_c.g) && (b == p_c.b) && (a == p_c.a);
		}
		const float *get_data() const { return &r; }
		String to_string() const {
			String sz = "{";
			const float *data = get_data();
			for (int c = 0; c < 4; c++) {
				float f = data[c];
				int val = ((f * 255.0f) + 0.5f);
				sz += String(Variant(val)) + " ";
			}
			sz += "}";
			return sz;
		}
	};

	// simplest FVF - local or baked position
	struct BatchVertex {
		// must be 32 bit pod
		BatchVector2 pos;
		BatchVector2 uv;
	};

	// simple FVF but also incorporating baked color
	struct BatchVertexColored : public BatchVertex {
		// must be 32 bit pod
		BatchColor col;
	};

	// if we are using normal mapping, we need light angles to be sent
	struct BatchVertexLightAngled : public BatchVertexColored {
		// must be pod
		float light_angle;
	};

	// CUSTOM SHADER vertex formats. These are larger but will probably
	// be needed with custom shaders in order to have the data accessible in the shader.

	// if we are using COLOR in vertex shader but not position (VERTEX)
	struct BatchVertexModulated : public BatchVertexLightAngled {
		BatchColor modulate;
	};

	struct BatchTransform {
		BatchVector2 translate;
		BatchVector2 basis[2];
	};

	// last resort, specially for custom shader, we put everything possible into a huge FVF
	// not very efficient, but better than no batching at all.
	struct BatchVertexLarge : public BatchVertexModulated {
		// must be pod
		BatchTransform transform;
	};

	// Batch should be as small as possible, and ideally nicely aligned (is 32 bytes at the moment)
	struct Batch {
		RasterizerStorageCommon::BatchType type; // should be 16 bit
		uint16_t batch_texture_id;

		// also item reference number
		uint32_t first_command;

		// in the case of DEFAULT, this is num commands.
		// with rects, is number of command and rects.
		// with lines, is number of lines
		// with polys, is number of indices (actual rendered verts)
		uint32_t num_commands;

		// first vertex of this batch in the vertex lists
		uint32_t first_vert;

		// we can keep the batch structure small because we either need to store
		// the color if a handled batch, or the parent item if a default batch, so
		// we can reference the correct originating command
		union {
			BatchColor color;

			// for default batches we will store the parent item
			const RasterizerCanvas::Item *item;
		};

		uint32_t get_num_verts() const {
			switch (type) {
				default: {
				} break;
				case RasterizerStorageCommon::BT_RECT: {
					return num_commands * 4;
				} break;
				case RasterizerStorageCommon::BT_LINE: {
					return num_commands * 2;
				} break;
				case RasterizerStorageCommon::BT_LINE_AA: {
					return num_commands * 2;
				} break;
				case RasterizerStorageCommon::BT_POLY: {
					return num_commands;
				} break;
			}

			// error condition
			WARN_PRINT_ONCE("reading num_verts from incorrect batch type");
			return 0;
		}
	};

	struct BatchTex {
		enum TileMode : uint32_t {
			TILE_OFF,
			TILE_NORMAL,
			TILE_FORCE_REPEAT,
		};
		RID RID_texture;
		RID RID_normal;
		TileMode tile_mode;
		BatchVector2 tex_pixel_size;
		uint32_t flags;
	};

	// items in a list to be sorted prior to joining
	struct BSortItem {
		// have a function to keep as pod, rather than operator
		void assign(const BSortItem &o) {
			item = o.item;
			z_index = o.z_index;
		}
		RasterizerCanvas::Item *item;
		int z_index;
	};

	// batch item may represent 1 or more items
	struct BItemJoined {
		uint32_t first_item_ref;
		uint32_t num_item_refs;

		Rect2 bounding_rect;

		// note the z_index  may only be correct for the first of the joined item references
		// this has implications for light culling with z ranged lights.
		int16_t z_index;

		// these are defined in RasterizerStorageCommon::BatchFlags
		uint16_t flags;

		// we are always splitting items with lots of commands,
		// and items with unhandled primitives (default)
		bool is_single_item() const { return (num_item_refs == 1); }
		bool use_attrib_transform() const { return flags & RasterizerStorageCommon::USE_LARGE_FVF; }
	};

	struct BItemRef {
		RasterizerCanvas::Item *item;
		Color final_modulate;
	};

	struct BLightRegion {
		void reset() {
			light_bitfield = 0;
			shadow_bitfield = 0;
			too_many_lights = false;
		}
		uint64_t light_bitfield;
		uint64_t shadow_bitfield;
		bool too_many_lights; // we can only do light region optimization if there are 64 or less lights
	};

	struct BatchData {
		BatchData() {
			reset_flush();
			reset_joined_item();

			gl_vertex_buffer = 0;
			gl_index_buffer = 0;
			max_quads = 0;
			vertex_buffer_size_units = 0;
			vertex_buffer_size_bytes = 0;
			index_buffer_size_units = 0;
			index_buffer_size_bytes = 0;

			use_colored_vertices = false;

			settings_use_batching = false;
			settings_max_join_item_commands = 0;
			settings_colored_vertex_format_threshold = 0.0f;
			settings_batch_buffer_num_verts = 0;
			scissor_threshold_area = 0.0f;
			joined_item_batch_flags = 0;
			diagnose_frame = false;
			next_diagnose_tick = 10000;
			diagnose_frame_number = 9999999999; // some high number
			join_across_z_indices = true;
			settings_item_reordering_lookahead = 0;

			settings_use_batching_original_choice = false;
			settings_flash_batching = false;
			settings_diagnose_frame = false;
			settings_scissor_lights = false;
			settings_scissor_threshold = -1.0f;
			settings_use_single_rect_fallback = false;
			settings_use_software_skinning = true;
			settings_ninepatch_mode = 0; // default
			settings_light_max_join_items = 16;

			settings_uv_contract = false;
			settings_uv_contract_amount = 0.0f;

			buffer_mode_batch_upload_send_null = true;
			buffer_mode_batch_upload_flag_stream = false;

			stats_items_sorted = 0;
			stats_light_items_joined = 0;
		}

		// called for each joined item
		void reset_joined_item() {
			// noop but left in as a stub
		}

		// called after each flush
		void reset_flush() {
			batches.reset();
			batch_textures.reset();

			vertices.reset();
			light_angles.reset();
			vertex_colors.reset();
			vertex_modulates.reset();
			vertex_transforms.reset();

			total_quads = 0;
			total_verts = 0;
			total_color_changes = 0;

			use_light_angles = false;
			use_modulate = false;
			use_large_verts = false;
			fvf = RasterizerStorageCommon::FVF_REGULAR;
		}

		unsigned int gl_vertex_buffer;
		unsigned int gl_index_buffer;

		uint32_t max_quads;
		uint32_t vertex_buffer_size_units;
		uint32_t vertex_buffer_size_bytes;
		uint32_t index_buffer_size_units;
		uint32_t index_buffer_size_bytes;

		// small vertex FVF type - pos and UV.
		// This will always be written to initially, but can be translated
		// to larger FVFs if necessary.
		RasterizerArray<BatchVertex> vertices;

		// extra data which can be stored during prefilling, for later translation to larger FVFs
		RasterizerArray<float> light_angles;
		RasterizerArray<BatchColor> vertex_colors; // these aren't usually used, but are for polys
		RasterizerArray<BatchColor> vertex_modulates;
		RasterizerArray<BatchTransform> vertex_transforms;

		// instead of having a different buffer for each vertex FVF type
		// we have a special array big enough for the biggest FVF
		// which can have a changeable unit size, and reuse it.
		RasterizerUnitArray unit_vertices;

		RasterizerArray<Batch> batches;
		RasterizerArray<Batch> batches_temp; // used for translating to colored vertex batches
		RasterizerArray_non_pod<BatchTex> batch_textures; // the only reason this is non-POD is because of RIDs

		// SHOULD THESE BE IN FILLSTATE?
		// flexible vertex format.
		// all verts have pos and UV.
		// some have color, some light angles etc.
		RasterizerStorageCommon::FVF fvf;
		bool use_colored_vertices;
		bool use_light_angles;
		bool use_modulate;
		bool use_large_verts;

		// if the shader is using MODULATE, we prevent baking color so the final_modulate can
		// be read in the shader.
		// if the shader is reading VERTEX, we prevent baking vertex positions with extra matrices etc
		// to prevent the read position being incorrect.
		// These flags are defined in RasterizerStorageCommon::BatchFlags
		uint32_t joined_item_batch_flags;

		RasterizerArray<BItemJoined> items_joined;
		RasterizerArray<BItemRef> item_refs;

		// items are sorted prior to joining
		RasterizerArray<BSortItem> sort_items;

		// counts
		int total_quads;
		int total_verts;

		// we keep a record of how many color changes caused new batches
		// if the colors are causing an excessive number of batches, we switch
		// to alternate batching method and add color to the vertex format.
		int total_color_changes;

		// measured in pixels, recalculated each frame
		float scissor_threshold_area;

		// diagnose this frame, every nTh frame when settings_diagnose_frame is on
		bool diagnose_frame;
		String frame_string;
		uint32_t next_diagnose_tick;
		uint64_t diagnose_frame_number;

		// whether to join items across z_indices - this can interfere with z ranged lights,
		// so has to be disabled in some circumstances
		bool join_across_z_indices;

		// global settings
		bool settings_use_batching; // the current use_batching (affected by flash)
		bool settings_use_batching_original_choice; // the choice entered in project settings
		bool settings_flash_batching; // for regression testing, flash between non-batched and batched renderer
		bool settings_diagnose_frame; // print out batches to help optimize / regression test
		int settings_max_join_item_commands;
		float settings_colored_vertex_format_threshold;
		int settings_batch_buffer_num_verts;
		bool settings_scissor_lights;
		float settings_scissor_threshold; // 0.0 to 1.0
		int settings_item_reordering_lookahead;
		bool settings_use_single_rect_fallback;
		bool settings_use_software_skinning;
		int settings_light_max_join_items;
		int settings_ninepatch_mode;

		// buffer orphaning modes
		bool buffer_mode_batch_upload_send_null;
		bool buffer_mode_batch_upload_flag_stream;

		// uv contraction
		bool settings_uv_contract;
		float settings_uv_contract_amount;

		// only done on diagnose frame
		void reset_stats() {
			stats_items_sorted = 0;
			stats_light_items_joined = 0;
		}

		// frame stats (just for monitoring and debugging)
		int stats_items_sorted;
		int stats_light_items_joined;
	} bdata;

	struct FillState {
		void reset_flush() {
			// don't reset members that need to be preserved after flushing
			// half way through a list of commands
			curr_batch = nullptr;
			batch_tex_id = -1;
			texpixel_size = Vector2(1, 1);
			contract_uvs = false;

			sequence_batch_type_flags = 0;
		}

		void reset_joined_item(bool p_is_single_item, bool p_use_attrib_transform) {
			reset_flush();
			is_single_item = p_is_single_item;
			use_attrib_transform = p_use_attrib_transform;
			use_software_transform = !is_single_item && !use_attrib_transform;

			extra_matrix_sent = false;
		}

		// for batching multiple types, we don't allow mixing RECTs / LINEs etc.
		// using flags allows quicker rejection of sequences with different batch types
		uint32_t sequence_batch_type_flags;

		Batch *curr_batch;
		int batch_tex_id;

		bool is_single_item;
		bool use_attrib_transform;
		bool use_software_transform;

		bool contract_uvs;
		Vector2 texpixel_size;
		Color final_modulate;
		TransformMode transform_mode;
		TransformMode orig_transform_mode;

		// support for extra matrices
		bool extra_matrix_sent; // whether sent on this item (in which case software transform can't be used untl end of item)
		int transform_extra_command_number_p1; // plus one to allow fast checking against zero
		Transform2D transform_combined; // final * extra
		Transform2D skeleton_base_inverse_xform; // used in software skinning
	};

	// used during try_join
	struct RenderItemState {
		RenderItemState() { reset(); }
		void reset() {
			current_clip = nullptr;
			shader_cache = nullptr;
			rebind_shader = true;
			prev_use_skeleton = false;
			last_blend_mode = -1;
			canvas_last_material = RID();
			item_group_z = 0;
			item_group_light = nullptr;
			final_modulate = Color(-1.0, -1.0, -1.0, -1.0); // just something unlikely

			joined_item_batch_type_flags_curr = 0;
			joined_item_batch_type_flags_prev = 0;

			joined_item = nullptr;
		}

		RasterizerCanvas::Item *current_clip;
		RasterizerStorageGLES3::Shader *shader_cache;
		bool rebind_shader;
		bool prev_use_skeleton;
		bool prev_distance_field;
		int last_blend_mode;
		RID canvas_last_material;
		Color final_modulate;

		// used for joining items only
		BItemJoined *joined_item;
		bool join_batch_break;
		BLightRegion light_region;

		// we need some logic to prevent joining items that have vastly different batch types
		// these are defined in RasterizerStorageCommon::BatchTypeFlags
		uint32_t joined_item_batch_type_flags_curr;
		uint32_t joined_item_batch_type_flags_prev;

		// 'item group' is data over a single call to canvas_render_items
		int item_group_z;
		Color item_group_modulate;
		RasterizerCanvas::Light *item_group_light;
		Transform2D item_group_base_transform;
	} _render_item_state;

	// End batching

	RasterizerStorageGLES3 *storage;
	bool use_nvidia_rect_workaround;

	// allow user to choose api usage
	GLenum _buffer_upload_usage_flag;

	struct LightInternal : public RID_Data {
		struct UBOData {
			float light_matrix[16];
			float local_matrix[16];
			float shadow_matrix[16];
			float color[4];
			float shadow_color[4];
			float light_pos[2];
			float shadowpixel_size;
			float shadow_gradient;
			float light_height;
			float light_outside_alpha;
			float shadow_distance_mult;
			uint8_t padding[4];
		} ubo_data;

		GLuint ubo;
	};

	RID_Owner<LightInternal> light_internal_owner;

private:
	struct BatchGLData {
		// for batching
		GLuint batch_vertex_array[5];
	} batch_gl_data;

public:
	RID light_internal_create();
	void light_internal_update(RID p_rid, Light *p_light);
	void light_internal_free(RID p_rid);

	void canvas_render_items_begin(const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void canvas_render_items_end();
	void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void canvas_begin();
	void canvas_end();

	void batch_canvas_render_items_begin(const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform);
	void batch_canvas_render_items_end();
	void batch_canvas_render_items(RasterizerCanvas::Item *p_item_list, int p_z, const Color &p_modulate, RasterizerCanvas::Light *p_light, const Transform2D &p_base_transform);

	void _set_texture_rect_mode(bool p_enable, bool p_ninepatch = false, bool p_light_angle = false, bool p_modulate = false, bool p_large_vertex = false);
	RasterizerStorageGLES3::Texture *_bind_canvas_texture(const RID &p_texture, const RID &p_normal_map, bool p_force = false);

	void _draw_gui_primitive(int p_points, const Vector2 *p_vertices, const Color *p_colors, const Vector2 *p_uvs, const float *p_light_angles = nullptr);
	void _draw_polygon(const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor, const int *p_bones, const float *p_weights);
	void _draw_generic(GLuint p_primitive, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor);
	void _draw_generic_indices(GLuint p_primitive, const int *p_indices, int p_index_count, int p_vertex_count, const Vector2 *p_vertices, const Vector2 *p_uvs, const Color *p_colors, bool p_singlecolor);

	void _copy_texscreen(const Rect2 &p_rect);

	void canvas_debug_viewport_shadows(Light *p_lights_with_shadow);

	void canvas_light_shadow_buffer_update(RID p_buffer, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders, CameraMatrix *p_xform_cache);

	void reset_canvas();

	void draw_generic_textured_rect(const Rect2 &p_rect, const Rect2 &p_src);
	void draw_lens_distortion_rect(const Rect2 &p_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample);
	void render_rect_nvidia_workaround(const Item::CommandRect *p_rect, const RasterizerStorageGLES3::Texture *p_texture);

	void finalize();

	void draw_window_margins(int *black_margin, RID *black_image);

private:
	// legacy codepath .. to remove after testing
	void _legacy_canvas_render_item(Item *p_ci, RenderItemState &r_ris);

	// high level batch funcs
	void canvas_render_items_implementation(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_base_transform);
	void render_joined_item(const BItemJoined &p_bij, RenderItemState &r_ris);
	bool try_join_item(Item *p_ci, RenderItemState &r_ris, bool &r_batch_break);
	void render_batches(Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material);

	// low level batch funcs
	void _batch_upload_buffers();
	void _batch_render_prepare();
	void _batch_render_generic(const Batch &p_batch, RasterizerStorageGLES3::Material *p_material);
	void _batch_render_lines(const Batch &p_batch, RasterizerStorageGLES3::Material *p_material, bool p_anti_alias);

	// funcs used from rasterizer_canvas_batcher template
	void gl_enable_scissor(int p_x, int p_y, int p_width, int p_height) const;
	void gl_disable_scissor() const;

public:
	void initialize();
	RasterizerCanvasGLES3();

	// BATCHING

protected:
	// main functions called from the rasterizer canvas
	void batch_constructor();
	void batch_initialize();

	void batch_canvas_begin();
	void batch_canvas_end();

	// recording and sorting items from the initial pass
	void record_items(RasterizerCanvas::Item *p_item_list, int p_z);
	void join_sorted_items();
	void sort_items();
	bool _sort_items_match(const BSortItem &p_a, const BSortItem &p_b) const;
	bool sort_items_from(int p_start);

	// joining logic
	bool _disallow_item_join_if_batch_types_too_different(RenderItemState &r_ris, uint32_t btf_allowed);
	bool _detect_item_batch_break(RenderItemState &r_ris, RasterizerCanvas::Item *p_ci, bool &r_batch_break);

	// drives the loop filling batches and flushing
	void render_joined_item_commands(const BItemJoined &p_bij, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material, bool p_lit, const RenderItemState &p_ris);

	// flush once full or end of joined item
	void flush_render_batches(RasterizerCanvas::Item *p_first_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material, uint32_t p_sequence_batch_type_flags);

	// a single joined item can contain multiple itemrefs, and thus create lots of batches
	bool prefill_joined_item(FillState &r_fill_state, int &r_command_start, RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material);

	// prefilling different types of batch

	// default batch is an 'unhandled' legacy type batch that will be drawn with the legacy path,
	// all other batches are accelerated.
	void _prefill_default_batch(FillState &r_fill_state, int p_command_num, const RasterizerCanvas::Item &p_item);

	// accelerated batches
	bool _prefill_line(RasterizerCanvas::Item::CommandLine *p_line, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate);
	template <bool SEND_LIGHT_ANGLES>
	bool _prefill_ninepatch(RasterizerCanvas::Item::CommandNinePatch *p_np, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate);
	template <bool SEND_LIGHT_ANGLES>
	bool _prefill_polygon(RasterizerCanvas::Item::CommandPolygon *p_poly, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item *p_item, bool multiply_final_modulate);
	template <bool SEND_LIGHT_ANGLES>
	bool _prefill_rect(RasterizerCanvas::Item::CommandRect *rect, FillState &r_fill_state, int &r_command_start, int command_num, int command_count, RasterizerCanvas::Item::Command *const *commands, RasterizerCanvas::Item *p_item, bool multiply_final_modulate);

	// dealing with textures
	int _batch_find_or_create_tex(const RID &p_texture, const RID &p_normal, bool p_tile, int p_previous_match);

protected:
	// legacy support for non batched mode
	void _legacy_canvas_item_render_commands(RasterizerCanvas::Item *p_item, RasterizerCanvas::Item *p_current_clip, bool &r_reclip, RasterizerStorageGLES3::Material *p_material);

	// light scissoring
	bool _light_scissor_begin(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect) const;
	bool _light_find_intersection(const Rect2 &p_item_rect, const Transform2D &p_light_xform, const Rect2 &p_light_rect, Rect2 &r_cliprect) const;
	void _calculate_scissor_threshold_area();

	// translating vertex formats prior to rendering
	void _translate_batches_to_vertex_colored_FVF();
	template <class BATCH_VERTEX_TYPE, bool INCLUDE_LIGHT_ANGLES, bool INCLUDE_MODULATE, bool INCLUDE_LARGE>
	void _translate_batches_to_larger_FVF(uint32_t p_sequence_batch_type_flags);

protected:
	// accessory funcs
	void _software_transform_vertex(BatchVector2 &r_v, const Transform2D &p_tr) const;
	void _software_transform_vertex(Vector2 &r_v, const Transform2D &p_tr) const;

	TransformMode _find_transform_mode(const Transform2D &p_tr) const {
		// decided whether to do translate only for software transform
		if ((p_tr.elements[0].x == 1.0f) &&
				(p_tr.elements[0].y == 0.0f) &&
				(p_tr.elements[1].x == 0.0f) &&
				(p_tr.elements[1].y == 1.0f)) {
			return TM_TRANSLATE;
		}

		return TM_ALL;
	}

	bool _software_skin_poly(RasterizerCanvas::Item::CommandPolygon *p_poly, RasterizerCanvas::Item *p_item, BatchVertex *bvs, BatchColor *vertex_colors, const FillState &p_fill_state, const BatchColor *p_precalced_colors);

	RasterizerStorageGLES3::Texture *_get_canvas_texture(const RID &p_texture) const {
		if (p_texture.is_valid()) {
			RasterizerStorageGLES3::Texture *texture = storage->texture_owner.getornull(p_texture);

			if (texture) {
				// could be a proxy texture (e.g. animated)
				if (texture->proxy) {
					// take care to prevent infinite loop
					int count = 0;
					while (texture->proxy) {
						texture = texture->proxy;
						count++;
						ERR_FAIL_COND_V_MSG(count == 16, nullptr, "Texture proxy infinite loop detected.");
					}
				}

				return texture->get_ptr();
			}
		}

		return nullptr;
	}

public:
	Batch *_batch_request_new(bool p_blank = true) {
		Batch *batch = bdata.batches.request();
		if (!batch) {
			// grow the batches
			bdata.batches.grow();

			// and the temporary batches (used for color verts)
			bdata.batches_temp.reset();
			bdata.batches_temp.grow();

			// this should always succeed after growing
			batch = bdata.batches.request();
			RAST_DEBUG_ASSERT(batch);
		}

		if (p_blank) {
			memset(batch, 0, sizeof(Batch));
		} else {
			batch->item = nullptr;
		}

		return batch;
	}

	BatchVertex *_batch_vertex_request_new() {
		return bdata.vertices.request();
	}
};

#endif // RASTERIZER_CANVAS_GLES3_H

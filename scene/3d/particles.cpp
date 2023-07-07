/*************************************************************************/
/*  particles.cpp                                                        */
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

#include "particles.h"

#include "core/os/os.h"
#include "scene/resources/particles_material.h"

#include "servers/visual_server.h"

AABB Particles::get_aabb() const {
	return AABB();
}
PoolVector<Face3> Particles::get_faces(uint32_t p_usage_flags) const {
	return PoolVector<Face3>();
}

void Particles::set_emitting(bool p_emitting) {
	emitting = p_emitting;
	VS::get_singleton()->particles_set_emitting(particles, emitting);

	if (emitting && data.one_shot) {
		set_process_internal(true);
	} else if (!emitting) {
		set_process_internal(false);
	}
}

void Particles::_mark_dirty() {
	dirty = true;
	set_process_internal(true);
}

void Particles::_update_dirty() {
	if (!emitting || !data.one_shot) {
		set_process_internal(false);
	}

	if (dirty) {
		VS::get_singleton()->particles_set(particles, data);
	}

	dirty = false;
}

void Particles::set_amount(int p_amount) {
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	data.amount = p_amount;
	_mark_dirty();
}

void Particles::set_lifetime(float p_lifetime) {
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	data.lifetime = p_lifetime;
	_mark_dirty();
}

void Particles::set_one_shot(bool p_one_shot) {
	data.one_shot = p_one_shot;
	if (is_emitting()) {
		set_process_internal(true);
		if (!data.one_shot) {
			VisualServer::get_singleton()->particles_restart(particles);
		}
	}
	if (!dirty && !data.one_shot) {
		set_process_internal(false);
	}
	_mark_dirty();
}

void Particles::set_pre_process_time(float p_time) {
	data.pre_process_time = p_time;
	_mark_dirty();
}

void Particles::set_explosiveness_ratio(float p_ratio) {
	data.explosiveness_ratio = p_ratio;
	_mark_dirty();
}

void Particles::set_randomness_ratio(float p_ratio) {
	data.randomness_ratio = p_ratio;
	_mark_dirty();
}

void Particles::set_visibility_aabb(const AABB &p_aabb) {
	data.visibility_aabb = p_aabb;
	_mark_dirty();
	update_gizmo();
	_change_notify("visibility_aabb");
}

void Particles::set_use_local_coordinates(bool p_enable) {
	data.local_coords = p_enable;
	_mark_dirty();
}

void Particles::set_process_material(const Ref<Material> &p_material) {
	process_material = p_material;
	RID material_rid;
	if (process_material.is_valid()) {
		material_rid = process_material->get_rid();
		data.process_material = material_rid;
	} else {
		data.process_material = RID();
	}
	_mark_dirty();

	update_configuration_warning();
}

void Particles::set_speed_scale(float p_scale) {
	data.speed_scale = p_scale;
	_mark_dirty();
}

void Particles::set_draw_order(ParticlesData::DrawOrder p_order) {
	data.draw_order = p_order;
	_mark_dirty();
}

void Particles::set_draw_passes(int p_count) {
	ERR_FAIL_COND(p_count < 1);
	draw_passes.resize(p_count);
	for (int i = p_count; i < ParticlesData::MAX_DRAW_PASSES; i++) {
		data.draw_passes[i] = RID();
	}
	_mark_dirty();
	_change_notify();
}

void Particles::set_draw_pass_mesh(int p_pass, const Ref<Mesh> &p_mesh) {
	ERR_FAIL_INDEX(p_pass, draw_passes.size());

	draw_passes.write[p_pass] = p_mesh;

	RID mesh_rid;
	if (p_mesh.is_valid()) {
		mesh_rid = p_mesh->get_rid();
		data.draw_passes[p_pass] = mesh_rid;
	} else {
		data.draw_passes[p_pass] = RID();
	}

	_mark_dirty();
	update_configuration_warning();
}

void Particles::set_fixed_fps(int p_count) {
	data.fixed_fps = p_count;
	_mark_dirty();
}

void Particles::set_fractional_delta(bool p_enable) {
	data.fractional_delta = p_enable;
	_mark_dirty();
}

bool Particles::is_emitting() const {
	return VS::get_singleton()->particles_get_emitting(particles);
}
int Particles::get_amount() const {
	return data.amount;
}
float Particles::get_lifetime() const {
	return data.lifetime;
}
bool Particles::get_one_shot() const {
	return data.one_shot;
}

float Particles::get_pre_process_time() const {
	return data.pre_process_time;
}
float Particles::get_explosiveness_ratio() const {
	return data.explosiveness_ratio;
}
float Particles::get_randomness_ratio() const {
	return data.randomness_ratio;
}
AABB Particles::get_visibility_aabb() const {
	return data.visibility_aabb;
}
bool Particles::get_use_local_coordinates() const {
	return data.local_coords;
}
Ref<Material> Particles::get_process_material() const {
	return process_material;
}

float Particles::get_speed_scale() const {
	return data.speed_scale;
}

Ref<Mesh> Particles::get_draw_pass_mesh(int p_pass) const {
	ERR_FAIL_INDEX_V(p_pass, draw_passes.size(), Ref<Mesh>());

	return draw_passes[p_pass];
}

ParticlesData::DrawOrder Particles::get_draw_order() const {
	return data.draw_order;
}
int Particles::get_draw_passes() const {
	return draw_passes.size();
}

int Particles::get_fixed_fps() const {
	return data.fixed_fps;
}

bool Particles::get_fractional_delta() const {
	return data.fractional_delta;
}

String Particles::get_configuration_warning() const {
	String warnings = GeometryInstance::get_configuration_warning();

#ifdef OSX_ENABLED
	if (warnings != String()) {
		warnings += "\n\n";
	}

	warnings += "- " + TTR("On macOS, Particles rendering is much slower than CPUParticles due to transform feedback being implemented on the CPU instead of the GPU.\nConsider using CPUParticles instead when targeting macOS.\nYou can use the \"Convert to CPUParticles\" toolbar option for this purpose.");
#endif

	bool meshes_found = false;
	bool anim_material_found = false;

	for (int i = 0; i < draw_passes.size(); i++) {
		if (draw_passes[i].is_valid()) {
			meshes_found = true;
			for (int j = 0; j < draw_passes[i]->get_surface_count(); j++) {
				anim_material_found = Object::cast_to<ShaderMaterial>(draw_passes[i]->surface_get_material(j).ptr()) != nullptr;
				SpatialMaterial *spat = Object::cast_to<SpatialMaterial>(draw_passes[i]->surface_get_material(j).ptr());
				anim_material_found = anim_material_found || (spat && spat->get_billboard_mode() == SpatialMaterial::BILLBOARD_PARTICLES);
			}
			if (anim_material_found) {
				break;
			}
		}
	}

	anim_material_found = anim_material_found || Object::cast_to<ShaderMaterial>(get_material_override().ptr()) != nullptr;
	SpatialMaterial *spat = Object::cast_to<SpatialMaterial>(get_material_override().ptr());
	anim_material_found = anim_material_found || (spat && spat->get_billboard_mode() == SpatialMaterial::BILLBOARD_PARTICLES);

	if (!meshes_found) {
		if (warnings != String()) {
			warnings += "\n\n";
		}
		warnings += "- " + TTR("Nothing is visible because meshes have not been assigned to draw passes.");
	}

	if (process_material.is_null()) {
		if (warnings != String()) {
			warnings += "\n";
		}
		warnings += "- " + TTR("A material to process the particles is not assigned, so no behavior is imprinted.");
	} else {
		const ParticlesMaterial *process = Object::cast_to<ParticlesMaterial>(process_material.ptr());
		if (!anim_material_found && process &&
				(process->get_param(ParticlesMaterial::PARAM_ANIM_SPEED) != 0.0 || process->get_param(ParticlesMaterial::PARAM_ANIM_OFFSET) != 0.0 ||
						process->get_param_texture(ParticlesMaterial::PARAM_ANIM_SPEED).is_valid() || process->get_param_texture(ParticlesMaterial::PARAM_ANIM_OFFSET).is_valid())) {
			if (warnings != String()) {
				warnings += "\n";
			}
			warnings += "- " + TTR("Particles animation requires the usage of a SpatialMaterial whose Billboard Mode is set to \"Particle Billboard\".");
		}
	}

	return warnings;
}

void Particles::restart() {
	VisualServer::get_singleton()->particles_restart(particles);
	VisualServer::get_singleton()->particles_set_emitting(particles, true);
}

AABB Particles::capture_aabb() const {
	return VS::get_singleton()->particles_get_current_aabb(particles);
}

void Particles::_validate_property(PropertyInfo &property) const {
	if (property.name.begins_with("draw_pass_")) {
		int index = property.name.get_slicec('_', 2).to_int() - 1;
		if (index >= draw_passes.size()) {
			property.usage = 0;
			return;
		}
	}
}

void Particles::_notification(int p_what) {
	if (p_what == NOTIFICATION_PAUSED || p_what == NOTIFICATION_UNPAUSED) {
		if (can_process()) {
			VS::get_singleton()->particles_set_speed_scale(particles, data.speed_scale);
		} else {
			VS::get_singleton()->particles_set_speed_scale(particles, 0);
		}
	}

	// Use internal process when emitting and one_shot are on so that when
	// the shot ends the editor can properly update
	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		if (dirty) {
			_update_dirty();
		}
		if (data.one_shot && !is_emitting()) {
			_change_notify();
			set_process_internal(false);
		}
	}

	if (p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		// make sure particles are updated before rendering occurs if they were active before
		if (is_visible_in_tree() && !VS::get_singleton()->particles_is_inactive(particles)) {
			VS::get_singleton()->particles_request_process(particles);
		}
	}
}

void Particles::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_emitting", "emitting"), &Particles::set_emitting);
	ClassDB::bind_method(D_METHOD("set_amount", "amount"), &Particles::set_amount);
	ClassDB::bind_method(D_METHOD("set_lifetime", "secs"), &Particles::set_lifetime);
	ClassDB::bind_method(D_METHOD("set_one_shot", "enable"), &Particles::set_one_shot);
	ClassDB::bind_method(D_METHOD("set_pre_process_time", "secs"), &Particles::set_pre_process_time);
	ClassDB::bind_method(D_METHOD("set_explosiveness_ratio", "ratio"), &Particles::set_explosiveness_ratio);
	ClassDB::bind_method(D_METHOD("set_randomness_ratio", "ratio"), &Particles::set_randomness_ratio);
	ClassDB::bind_method(D_METHOD("set_visibility_aabb", "aabb"), &Particles::set_visibility_aabb);
	ClassDB::bind_method(D_METHOD("set_use_local_coordinates", "enable"), &Particles::set_use_local_coordinates);
	ClassDB::bind_method(D_METHOD("set_fixed_fps", "fps"), &Particles::set_fixed_fps);
	ClassDB::bind_method(D_METHOD("set_fractional_delta", "enable"), &Particles::set_fractional_delta);
	ClassDB::bind_method(D_METHOD("set_process_material", "material"), &Particles::set_process_material);
	ClassDB::bind_method(D_METHOD("set_speed_scale", "scale"), &Particles::set_speed_scale);

	ClassDB::bind_method(D_METHOD("is_emitting"), &Particles::is_emitting);
	ClassDB::bind_method(D_METHOD("get_amount"), &Particles::get_amount);
	ClassDB::bind_method(D_METHOD("get_lifetime"), &Particles::get_lifetime);
	ClassDB::bind_method(D_METHOD("get_one_shot"), &Particles::get_one_shot);
	ClassDB::bind_method(D_METHOD("get_pre_process_time"), &Particles::get_pre_process_time);
	ClassDB::bind_method(D_METHOD("get_explosiveness_ratio"), &Particles::get_explosiveness_ratio);
	ClassDB::bind_method(D_METHOD("get_randomness_ratio"), &Particles::get_randomness_ratio);
	ClassDB::bind_method(D_METHOD("get_visibility_aabb"), &Particles::get_visibility_aabb);
	ClassDB::bind_method(D_METHOD("get_use_local_coordinates"), &Particles::get_use_local_coordinates);
	ClassDB::bind_method(D_METHOD("get_fixed_fps"), &Particles::get_fixed_fps);
	ClassDB::bind_method(D_METHOD("get_fractional_delta"), &Particles::get_fractional_delta);
	ClassDB::bind_method(D_METHOD("get_process_material"), &Particles::get_process_material);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &Particles::get_speed_scale);

	ClassDB::bind_method(D_METHOD("set_draw_order", "order"), &Particles::set_draw_order);

	ClassDB::bind_method(D_METHOD("get_draw_order"), &Particles::get_draw_order);

	ClassDB::bind_method(D_METHOD("set_draw_passes", "passes"), &Particles::set_draw_passes);
	ClassDB::bind_method(D_METHOD("set_draw_pass_mesh", "pass", "mesh"), &Particles::set_draw_pass_mesh);

	ClassDB::bind_method(D_METHOD("get_draw_passes"), &Particles::get_draw_passes);
	ClassDB::bind_method(D_METHOD("get_draw_pass_mesh", "pass"), &Particles::get_draw_pass_mesh);

	ClassDB::bind_method(D_METHOD("restart"), &Particles::restart);
	ClassDB::bind_method(D_METHOD("capture_aabb"), &Particles::capture_aabb);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emitting"), "set_emitting", "is_emitting");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "amount", PROPERTY_HINT_EXP_RANGE, "1,1000000,1"), "set_amount", "get_amount");
	ADD_GROUP("Time", "");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "lifetime", PROPERTY_HINT_EXP_RANGE, "0.01,600.0,0.01,or_greater"), "set_lifetime", "get_lifetime");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "one_shot"), "set_one_shot", "get_one_shot");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "preprocess", PROPERTY_HINT_EXP_RANGE, "0.00,600.0,0.01"), "set_pre_process_time", "get_pre_process_time");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "speed_scale", PROPERTY_HINT_RANGE, "0,64,0.01"), "set_speed_scale", "get_speed_scale");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "explosiveness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_explosiveness_ratio", "get_explosiveness_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "randomness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_randomness_ratio", "get_randomness_ratio");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fixed_fps", PROPERTY_HINT_RANGE, "0,1000,1"), "set_fixed_fps", "get_fixed_fps");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fract_delta"), "set_fractional_delta", "get_fractional_delta");
	ADD_GROUP("Drawing", "");
	ADD_PROPERTY(PropertyInfo(Variant::AABB, "visibility_aabb"), "set_visibility_aabb", "get_visibility_aabb");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "local_coords"), "set_use_local_coordinates", "get_use_local_coordinates");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "draw_order", PROPERTY_HINT_ENUM, "Index,Lifetime,View Depth"), "set_draw_order", "get_draw_order");
	ADD_GROUP("Process Material", "");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "process_material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial,ParticlesMaterial"), "set_process_material", "get_process_material");
	ADD_GROUP("Draw Passes", "draw_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "draw_passes", PROPERTY_HINT_RANGE, "0," + itos(ParticlesData::MAX_DRAW_PASSES) + ",1"), "set_draw_passes", "get_draw_passes");
	for (int i = 0; i < ParticlesData::MAX_DRAW_PASSES; i++) {
		ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "draw_pass_" + itos(i + 1), PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_draw_pass_mesh", "get_draw_pass_mesh", i);
	}
}

Particles::Particles() {
	particles = RID_PRIME(VS::get_singleton()->particles_create());
	set_base(particles);
	set_emitting(false);
	set_one_shot(false);
	set_amount(8);
	set_lifetime(1);
	set_fixed_fps(0);
	set_fractional_delta(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_visibility_aabb(AABB(Vector3(-4, -4, -4), Vector3(8, 8, 8)));
	set_use_local_coordinates(true);
	set_draw_passes(1);
	set_draw_order(ParticlesData::DRAW_ORDER_INDEX);
	set_speed_scale(1);
	dirty = false;
}

Particles::~Particles() {
	if (particles.is_valid()) {
		VS::get_singleton()->free(particles);
	}
}

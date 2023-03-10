/*************************************************************************/
/*  input.h                                                              */
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

#ifndef INPUT_H
#define INPUT_H

#include "core/object.h"
#include "core/os/main_loop.h"
#include "core/os/thread_safe.h"

class Input : public Object {
	GDCLASS(Input, Object);
	_THREAD_SAFE_CLASS_

public:
#undef CursorShape
	enum CursorShape {
		CURSOR_ARROW,
		CURSOR_IBEAM,
		CURSOR_POINTING_HAND,
		CURSOR_CROSS,
		CURSOR_WAIT,
		CURSOR_BUSY,
		CURSOR_DRAG,
		CURSOR_CAN_DROP,
		CURSOR_FORBIDDEN,
		CURSOR_VSIZE,
		CURSOR_HSIZE,
		CURSOR_BDIAGSIZE,
		CURSOR_FDIAGSIZE,
		CURSOR_MOVE,
		CURSOR_VSPLIT,
		CURSOR_HSPLIT,
		CURSOR_HELP,
		CURSOR_MAX
	};
	enum MouseMode {
		MOUSE_MODE_VISIBLE,
		MOUSE_MODE_HIDDEN,
		MOUSE_MODE_CAPTURED,
		MOUSE_MODE_CONFINED,
		MOUSE_MODE_CONFINED_HIDDEN,
	};

	struct Action {
		uint64_t physics_frame;
		uint64_t idle_frame;
		bool pressed;
		bool exact;
		float strength;
		float raw_strength;
	};

	struct SpeedTrack {
		uint64_t last_tick;
		Vector2 speed;
		Vector2 accum;
		float accum_t;
		float min_ref_frame;
		float max_ref_frame;

		void update(const Vector2 &p_delta_p);
		void reset();
		SpeedTrack();
	};

	struct Joypad {
		StringName name;
		StringName uid;
		bool connected;
		bool last_buttons[JOY_BUTTON_MAX + 12]; //apparently SDL specifies 35 possible buttons on android
		float last_axis[JOY_AXIS_MAX];
		int last_hat;
		int mapping;
		int hat_current;

		Joypad() {
			for (int i = 0; i < JOY_AXIS_MAX; i++) {
				last_axis[i] = 0.0f;
			}
			for (int i = 0; i < JOY_BUTTON_MAX + 12; i++) {
				last_buttons[i] = false;
			}
			connected = false;
			last_hat = HAT_MASK_CENTER;
			mapping = -1;
			hat_current = 0;
		}
	};

	enum HatMask {
		HAT_MASK_CENTER = 0,
		HAT_MASK_UP = 1,
		HAT_MASK_RIGHT = 2,
		HAT_MASK_DOWN = 4,
		HAT_MASK_LEFT = 8,
	};

	enum HatDir {
		HAT_UP,
		HAT_RIGHT,
		HAT_DOWN,
		HAT_LEFT,
		HAT_MAX,
	};

	enum {
		JOYPADS_MAX = 16,
	};

	enum JoyType {
		TYPE_BUTTON,
		TYPE_AXIS,
		TYPE_HAT,
		TYPE_MAX,
	};

	enum JoyAxisRange {
		NEGATIVE_HALF_AXIS = -1,
		FULL_AXIS = 0,
		POSITIVE_HALF_AXIS = 1
	};

	struct JoyEvent {
		int type;
		int index;
		float value;
	};

	struct JoyBinding {
		JoyType inputType;
		union {
			int button;

			struct {
				int axis;
				JoyAxisRange range;
				bool invert;
			} axis;

			struct {
				int hat;
				HatMask hat_mask;
			} hat;

		} input;

		JoyType outputType;
		union {
			JoystickList button;

			struct {
				JoystickList axis;
				JoyAxisRange range;
			} axis;

		} output;
	};

	struct JoyDeviceMapping {
		String uid;
		String name;
		Vector<JoyBinding> bindings;
	};
	struct VibrationInfo {
		float weak_magnitude;
		float strong_magnitude;
		float duration; // Duration in seconds
		uint64_t timestamp;
	};

	int mouse_button_mask;

	Set<int> physical_keys_pressed;
	Set<int> keys_pressed;
	Set<int> joy_buttons_pressed;
	Map<int, float> _joy_axis;
	//Map<StringName,int> custom_action_press;
	Vector3 gravity;
	Vector3 accelerometer;
	Vector3 magnetometer;
	Vector3 gyroscope;
	Vector2 mouse_pos;
	MainLoop *main_loop;

	Map<StringName, Action> action_state;

	bool emulate_touch_from_mouse;
	bool emulate_mouse_from_touch;

	int mouse_from_touch_index;

	SpeedTrack mouse_speed_track;
	Map<int, SpeedTrack> touch_speed_track;
	Map<int, Joypad> joy_names;
	int fallback_mapping;

	CursorShape default_shape;

private:
	Vector<JoyDeviceMapping> map_db;

	JoyEvent _get_mapped_button_event(const JoyDeviceMapping &mapping, int p_button);
	JoyEvent _get_mapped_axis_event(const JoyDeviceMapping &mapping, int p_axis, float p_value);
	void _get_mapped_hat_events(const JoyDeviceMapping &mapping, int p_hat, JoyEvent r_events[HAT_MAX]);
	JoystickList _get_output_button(String output);
	JoystickList _get_output_axis(String output);
	void _button_event(int p_device, int p_index, bool p_pressed);
	void _axis_event(int p_device, int p_axis, float p_value);

	void _parse_input_event_impl(const Ref<InputEvent> &p_event, bool p_is_emulated);

	List<Ref<InputEvent>> buffered_events;
	bool use_input_buffering;
	bool use_accumulated_input;

protected:
	Map<int, VibrationInfo> joy_vibration;
	static Input *singleton;

protected:
	static void _bind_methods();

public:
	void set_mouse_mode(MouseMode p_mode);
	MouseMode get_mouse_mode() const;

	static Input *get_singleton();

	bool is_key_pressed(int p_scancode) const;
	bool is_physical_key_pressed(int p_scancode) const;
	bool is_mouse_button_pressed(int p_button) const;
	bool is_joy_button_pressed(int p_device, int p_button) const;
	bool is_action_pressed(const StringName &p_action, bool p_exact = false) const;
	bool is_action_just_pressed(const StringName &p_action, bool p_exact = false) const;
	bool is_action_just_released(const StringName &p_action, bool p_exact = false) const;
	float get_action_strength(const StringName &p_action, bool p_exact = false) const;
	float get_action_raw_strength(const StringName &p_action, bool p_exact = false) const;

	float get_axis(const StringName &p_negative_action, const StringName &p_positive_action) const;
	Vector2 get_vector(const StringName &p_negative_x, const StringName &p_positive_x, const StringName &p_negative_y, const StringName &p_positive_y, float p_deadzone = -1.0f) const;

	float get_joy_axis(int p_device, int p_axis) const;
	String get_joy_name(int p_idx);
	Array get_connected_joypads();
	void joy_connection_changed(int p_idx, bool p_connected, String p_name, String p_guid = "");
	void add_joy_mapping(String p_mapping, bool p_update_existing = false);
	void remove_joy_mapping(String p_guid);
	bool is_joy_known(int p_device);
	String get_joy_guid(int p_device) const;
	Vector2 get_joy_vibration_strength(int p_device);
	float get_joy_vibration_duration(int p_device);
	uint64_t get_joy_vibration_timestamp(int p_device);
	void start_joy_vibration(int p_device, float p_weak_magnitude, float p_strong_magnitude, float p_duration = 0);
	void stop_joy_vibration(int p_device);
	void vibrate_handheld(int p_duration_ms = 500);

	Point2 get_mouse_position() const;
	Point2 get_last_mouse_speed();
	int get_mouse_button_mask() const;

	void warp_mouse_position(const Vector2 &p_to);
	Point2i warp_mouse_motion(const Ref<InputEventMouseMotion> &p_motion, const Rect2 &p_rect);

	Vector3 get_gravity() const;
	Vector3 get_accelerometer() const;
	Vector3 get_magnetometer() const;
	Vector3 get_gyroscope() const;
	void set_gravity(const Vector3 &p_gravity);
	void set_accelerometer(const Vector3 &p_accel);
	void set_magnetometer(const Vector3 &p_magnetometer);
	void set_gyroscope(const Vector3 &p_gyroscope);

	void get_argument_options(const StringName &p_function, int p_idx, List<String> *r_options) const;

	bool is_emulating_touch_from_mouse() const;
	bool is_emulating_mouse_from_touch() const;

	CursorShape get_default_cursor_shape() const;
	void set_default_cursor_shape(CursorShape p_shape);
	CursorShape get_current_cursor_shape() const;
	void set_custom_mouse_cursor(const RES &p_cursor, CursorShape p_shape = CURSOR_ARROW, const Vector2 &p_hotspot = Vector2());

	String get_joy_button_string(int p_button);
	String get_joy_axis_string(int p_axis);
	int get_joy_button_index_from_string(String p_button);
	int get_joy_axis_index_from_string(String p_axis);

	void parse_input_event(const Ref<InputEvent> &p_event);
	void flush_buffered_events();
	bool is_using_input_buffering();
	void set_use_input_buffering(bool p_enable);
	bool is_using_accumulated_input();
	void set_use_accumulated_input(bool p_enable);

	void release_pressed_events();

	void set_joy_axis(int p_device, int p_axis, float p_value);
	void set_main_loop(MainLoop *p_main_loop);
	void set_mouse_position(const Point2 &p_posf);

	void action_press(const StringName &p_action, float p_strength = 1.f);
	void action_release(const StringName &p_action);

	void iteration(float p_step);

	void set_emulate_touch_from_mouse(bool p_emulate);
	void ensure_touch_mouse_raised();

	void set_emulate_mouse_from_touch(bool p_emulate);
	void parse_mapping(String p_mapping);
	void joy_button(int p_device, int p_button, bool p_pressed);
	void joy_axis(int p_device, int p_axis, float p_value);
	void joy_hat(int p_device, int p_val);

	int get_unused_joy_id();

	bool is_joy_mapped(int p_device);
	String get_joy_guid_remapped(int p_device) const;
	void set_fallback_mapping(String p_guid);

	Input();
};

VARIANT_ENUM_CAST(Input::MouseMode);
VARIANT_ENUM_CAST(Input::CursorShape);

#endif // INPUT_H

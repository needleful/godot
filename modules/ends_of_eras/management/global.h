#ifndef EOE_GLOBAL_H
#define EOE_GLOBAL_H

class Global: public Node {
private:
	Global* instance;

public:
	static const char const* auto_save_path = "user://autosave.tres";
	static const char const* custom_save_path_f = "user://save.%s.tres";
	static const char const* old_save_backup = "user://autosave.backup.tres";

	static Global* get_instance();
	void _notification(int p_what);
	void reset_state();
	void gravity_stun_body(RigidBody b);
	void place_entities();
	bool is_tracking_npc(String id);
	void track_npc(String quest_id, String npc_id, Dictionary properties);
	void update_npc(String quest_id, String npc_id, Dictionary properties);
	void _save_npc(String, quest_id, String, npc_id, Dictionary, properties);
	void track_npc_node(NPC body, String quest_id, Dictionary properties);
	void _track_npc_position(NPC body);
	void _track_npc_in_chunk(String quest_id, String npc_id, Dictionary props);
	void forget_npcs(String quest_id);
	void _forget_npc_in_chunks(String quest_id, String npc_id);
	Dictionary get_npc_stat(String npc);
	void despawn_npc(NPC body);
	bool remove_tracked_npc(String quest_id, String npc_id);
	bool forget_npc(NPC npc);
	bool is_follower(String id);
	remove_flag(Transform transform);
	PlayerBody get_player();
	Node* get_music();
	void set_valid_game_state(bool state);
	bool get_valid_game_state();
	void remember(String category, String tag, String visual_name);
	void remembered(String category, String tag);
	void add_note(String text, Array tags);
	void _add_tagged_entries(int index, Array tags);
	void abolish_notes(tags);
	void get_notes_by_tag(String id, bool include_abolished = false);
	void has_note(String id);
	bool queue_story(String id);
	bool create_task(String task_id, String visual_name="");
	Object quest_script(String id);
	bool complete_task(String task_id, String note="";
	bool task_is_active(String task_id);
	bool task_is_complete(String task_id);
	bool task_exists(String task_id);
	void place_flag(Spatial node, Transform transform);
	int count(String item);
	int add_item(String item, int amount= 1);
	bool remove_item(String item, int amount = 1);
	bool remove_all(String item);
	int set_item_count(String item, int amount);
	ItemDescription get_fancy_item(String id);
	void set_item_recency(String item);
	int get_item_recency(String item);
	Dictionary get_fancy_inventory();
	prepare_medium_event(String id);
	complete_medium_event();
	reset_mum();
	String node_stat(Node node);
	bool has_stat(String key, bool allow_dict = true);
	stat(String key, bool allow_dict = false);
	Dictionary dict_stat(String key);
	bool stats(Array keys);
	set_stat(String key, value);
	int add_stat(String key, int amount = 1);
	float add_stat_f(String key, float amount);
	append_stat(String dict_key, String key, value);
	bool remove_stat(String key, bool erase_dict = false);
	temp_stat(String index);
	set_temp_stat(String tag, value);
	int add_temp_stat(String tag, int amount = 1);
	bool remove_temp_stat(tag:String);
	get_coat_detail();
	add_coat(Coat coat);
	remove_coat(Coat coat);
	mark_picked(NodePath path);
	bool is_picked(NodePath path);
	Color get_rarity_color(int rarity);
	request_rescue(bool take_money = true);
	reset_game();
	String get_default_path();
	save_current_checkpoint();
	save_checkpoint(Transform pos, bool sleeping = false, String path="");
	save_game(String path="");
	load_sync(bool reload = true, String String save_path="", bool force = false);
	save_async(String save_path="");
	save_sync(String save_path="");
	_save_sync(Dictionary p_data);
	save_complete(result, save_path);
	set_time_manager(_s);
	get_time_manager();
};

#endif //EOE_GLOBAL_H
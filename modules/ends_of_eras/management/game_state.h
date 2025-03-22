#ifndef EOE_GAMESTATE_H
#define EOE_GAMESTATE_H

class GameState: public Resource {
public:
	GameState version;
	Dictionary stats;
	Dictionary inventory;
	Transform checkpoint_position;
	Array var all_coats;
	Array var picked_items;
	Array var flags;

	Array journal;
	Array active_tasks;
	Array completed_tasks;

	void set_stat(String key, Variant value);

	Variant get_stat(String key, bool allow_dict = false);

	int add_stat(String key, int val);

	bool erase_stat(String key, bool erase_dict = false);

	bool has_stat(String key, bool allow_dict);
	
	GameState(GameVersion p_version);
};

#endif //EOE_GAMESTATE_H
#ifndef NP_DIALOG_SEQUENCE_H
#define NP_DIALOG_SEQUENCE_H

#include "core/resource.h"

class DialogItem : public Resource {
public:
	enum Type {
		MESSAGE,
		REPLY,
		NARRATION,
		CONTEXT_REPLY,
		SPECIAL_MENU_ENTRY
	};
private:
	Vector<String> conditions;
	String text;
	StringName speaker;
	int next, child, parent;
	Type type;
public:
	int get_next() const;
	void set_next(int);

	int get_child() const;
	void set_child(int);

	int get_parent() const;
	void set_parent(int);

	Type get_type() const;
	void set_type(Type);

	String get_text() const;
	void set_text(String);

	StringName get_speaker() const;
	void set_speaker(StringName);

	Vector<String> get_conditions() const;
	void set_conditions(Vector<String>);

	DialogItem();

	static void _bind_methods();
};

class DialogSequence: public Resource {
	DialogItem _next_at_or_up(DialogItem d);
	bool m_went_up;
public:
	Map<int, Ref<DialogItem>> dialog;
	Map<String, int> labels;

	bool went_up() const;

	Ref<DialogItem> find_index(int i) const;
	Ref<DialogItem> find_label(String l) const;
	int index_of(Ref<DialogItem> &d) const;
	bool has_index(int i) const;
	bool has_label(String l) const;
	Ref<DialogItem> next(Ref<DialogItem> d) const;
	Ref<DialogItem> child(Ref<DialogItem> d) const;
	Ref<DialogItem> parent(Ref<DialogItem> d) const;
	Ref<DialogItem> canonical_next(Ref<DialogItem> d);
	Ref<DialogItem> failed_next(Ref<DialogItem> d);

	DialogSequence();

	static void _bind_methods();
};

#endif //NP_DIALOG_SEQUENCE_H
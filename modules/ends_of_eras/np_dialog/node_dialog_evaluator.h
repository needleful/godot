
#ifndef NP_DIALOG_EVALUATOR_H
#define NP_DIALOG_EVALUATOR_H

class DialogRuntime : public Object {
	GDCLASS(DialogRuntime, Object);

	bool otherwise := false;
	enum ResultType {
		VALUE,
		SKIP,
		END,
		NOSKIP,
		SWAP
	};
	struct Result {
		ResultType type;
		Variant value;
	};

	// Persistent state of this dialog block
	enum BlockState {
		Unvisited,
		Visited,
		Completed
	};

	// Immediate state of a block
	enum LocalBlockState {
		NotApplicable,
		Optional,
		Quest,
		Completed,
		Visited
	};

	struct Block {
		String name;
		Dictionary
	};
};


#endif //NP_DIALOG_EVALUATOR_H
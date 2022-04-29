struct Button_State {
	bool is_down;
	bool changed;
};

enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_ESC,
	BUTTON_F1,
	BUTTON_F2,
	BUTTON_COUNT, // Should be the last item
};

struct Input {
	Button_State buttons[BUTTON_COUNT];
};
/* XPAD data */

struct xpad_data
{
	int timestamp;
	short stick_left_x;
	short stick_left_y;
	short stick_right_x;
	short stick_right_y;
	short trig_left;
	short trig_right;
	char pad; /* 1 up 2 down 4 left 8 right */
	char state; /* 1 start 2 back 4 stick_left 8 stick_right */
	unsigned char keys[6]; /* A B X Y Black White */
	
};

#define XPAD_PAD_UP 1
#define XPAD_PAD_DOWN 2
#define XPAD_PAD_LEFT 4
#define XPAD_PAD_RIGHT 8

#define XPAD_STATE_START 1
#define XPAD_STATE_BACK 2
#define XPAD_STATE_LEFT 4
#define XPAD_STATE_RIGHT 8


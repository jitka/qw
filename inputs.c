#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "input_functions.h"
#include "settings.h"
#include "render.h" //fuj document_t
#include "window.h" //zmeneni kurzoru

enum {
	BASIC, //ceka
	NUMBERS, //ceka na cisla 
	DISTANCE_1, //ceka na kliknuti
	DISTANCE_2,
	POSITION,
        MOVE,	
} state=BASIC;
int number;
int number_is_negativ;
int click_x; //kde bylo naposledy zmacknuto prave tlacitka mysi
int click_y;

extern document_t *document;

void wait_distance(){ 	
	state = DISTANCE_1;
	gdk_window_set_cursor(window,cursor_mesure);
}
void wait_position(){ 	
	state = POSITION;
	gdk_window_set_cursor(window,cursor_mesure);
}


static struct {
	unsigned int mode_mask;
	guint pressed_char;
	void (*function)(int);
} number_chars[] = { //pozor zmacknuti modifikacni klavesy zastavi brani cisla
	{ 0, GDK_g, key_jump },
	{ 0, GDK_Page_Down, key_jump_down },
	{ 0, GDK_Page_Up, key_jump_up },
        { 0, GDK_o, key_this_page_has_number },
	{ 0, GDK_c, key_set_columns},
	{ 0, GDK_r, key_set_rows},
	{ 0, GDK_F5, key_presentation_mode },
};

static struct {
	unsigned int modifier;
	guint pressed_char;
	void (*function)(void);
} command_chars[] = {
	//pohyb
	{ 0, GDK_Home, key_home },
	{ 0, GDK_End, key_end },
	{ 0, GDK_space, key_next_screan },
	{ 0, GDK_BackSpace, key_prev_screan },
	{ 0, GDK_Page_Down, key_next_screan },
	{ 0, GDK_Page_Up, key_prev_screan },
	{ 0, GDK_Down, key_next_row },
	{ 0, GDK_Up, key_prev_row },
	{ 0, GDK_Right, key_next_page },
	{ 0, GDK_Left, key_prev_page },
	//otaceni
	{ GDK_SHIFT_MASK, GDK_Left, key_rotate_page_left },
	{ GDK_SHIFT_MASK, GDK_Right, key_rotate_page_right },
	{ GDK_MOD1_MASK, GDK_Left, key_rotate_document_left },
	{ GDK_MOD1_MASK, GDK_Right, key_rotate_document_right },
	//okynkove
	{ 0, GDK_q, key_quit },
	{ 0, GDK_F11, key_fullscreen },
	{ 0, GDK_f, key_fullscreen },
	{ GDK_CONTROL_MASK, GDK_r, key_reload },
	{ GDK_SHIFT_MASK, GDK_C, key_crop},
	//zobrazeni
	{ GDK_CONTROL_MASK|GDK_MOD1_MASK, GDK_space, key_center },
	{ GDK_CONTROL_MASK, GDK_Right, key_move_right },
	{ GDK_CONTROL_MASK, GDK_Left, key_move_left },
	{ GDK_CONTROL_MASK, GDK_Down, key_move_down },
	{ GDK_CONTROL_MASK, GDK_Up, key_move_up },
	{ GDK_CONTROL_MASK|GDK_SHIFT_MASK, GDK_plus, key_zoom_in },
	{ GDK_CONTROL_MASK, GDK_plus, key_zoom_in },
	{ GDK_CONTROL_MASK, GDK_KP_Add, key_zoom_in },
	{ GDK_CONTROL_MASK, GDK_minus, key_zoom_out },
	{ GDK_CONTROL_MASK, GDK_KP_Subtract, key_zoom_out },
	//klikaci
	{ GDK_SHIFT_MASK, GDK_M, wait_distance },
	{ 0, GDK_m, wait_position },
};

void handle_key(guint keyval, guint modifier){
	modifier &= 				//vezmu jen ty prepinace co jsou zajimave
		GDK_SHIFT_MASK |
//		GDK_LOCK_MASK |
		GDK_CONTROL_MASK |
		GDK_MOD1_MASK | //alt_l
//		GDK_MOD2_MASK | //num lock
		GDK_MOD3_MASK | //ctrl_r
//		GDK_MOD4_MASK |
		GDK_MOD5_MASK; //alt_r
//		GDK_BUTTON1_MASK |
//		GDK_BUTTON2_MASK |
//		GDK_BUTTON3_MASK |
//		GDK_BUTTON4_MASK |
//		GDK_BUTTON5_MASK |
//		GDK_SUPER_MASK |
//		GDK_HYPER_MASK |
//		GDK_META_MASK |
//		GDK_RELEASE_MASK;

//	printf ("keyval %x prepinace %d state %d\n",keyval,modifier,state);

	if (mode == PRESENTATION)
		key_page_mode();
	switch(state){
		case NUMBERS:
			if ((modifier & (~GDK_SHIFT_MASK)) != 0) //pokud byl zmacknut prepinac mimo shift
				break;
			if ((keyval >= GDK_0) && (keyval <= GDK_9)){
				number = number*10 + keyval - GDK_0;
				return;
			} else if ((keyval >= GDK_KP_0) && (keyval <= GDK_KP_9)){
				number = number*10 + keyval - GDK_KP_0;
				return;
			} else {
				if (number_is_negativ)
					number *= -1;
				for (unsigned int i=0; i < sizeof(number_chars) / sizeof(number_chars[0]); i++)
					if (number_chars[i].pressed_char == keyval &&
							(command_chars[i].modifier == modifier) ) {
						number_chars[i].function(number);
					}
				state = BASIC;
			}
			break;
		case BASIC:

			// kdyby zacaly chodit cisla
			if ((modifier & (~GDK_SHIFT_MASK)) == 0){
				state = NUMBERS;
				number = 0;
				number_is_negativ = 0;
				if (keyval == GDK_plus || keyval == GDK_KP_Add){
				} else if (keyval == GDK_minus || keyval == GDK_KP_Subtract){
					number_is_negativ = 42;
				} else if ((keyval >= GDK_0) && (keyval <= GDK_9)){
					number = keyval - GDK_0;
				} else if ((keyval >= GDK_KP_0) && (keyval <= GDK_KP_9)){
					number = keyval - GDK_KP_0;
				} else { //nezacaly chodit cisla...
					state = BASIC;
				}
			}

			for (unsigned int i=0; i < sizeof(command_chars)/sizeof(command_chars[0]); i++)
				if ( command_chars[i].pressed_char == keyval &&
						(command_chars[i].modifier == modifier) ) {
					command_chars[i].function();
//					break;
				}		
			break;
		case MOVE:
			break;
		default:
			if (key_cancel_waiting){
				state = BASIC;
				gdk_window_set_cursor(window,cursor_basic);
			}
			break;
	}
}

void handle_click(int x, int y){
	switch(state){
	case DISTANCE_1:
		state=DISTANCE_2;
		click_x=x;
		click_y=y;
		break;
	case DISTANCE_2:
		state=BASIC;
		gdk_window_set_cursor(window,cursor_basic);
		click_distance(click_x,click_y,x,y);
		break;
	case POSITION:
		state=BASIC;
		gdk_window_set_cursor(window,cursor_basic);
		click_position(x,y);
		break;
	default:
		state=MOVE;
		gdk_window_set_cursor(window,cursor_move);
		click_x=x;
		click_y=y;
		break;
	}
}

void handle_release(){
	if (state != MOVE) return;
	state = BASIC;
	gdk_window_set_cursor(window,cursor_basic);
}

void handle_motion(int x, int y, int button){
	if (state == DISTANCE_1 || state == DISTANCE_2 || state == POSITION)
		return;
	if (button & GDK_BUTTON3_MASK)
		move(x-click_x,y-click_y);
	if (button & GDK_BUTTON1_MASK)
		move(0,y-click_y);
	click_x=x;
	click_y=y;
}

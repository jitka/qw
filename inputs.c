#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "input_functions.h"
#include "settings.h"

enum {
	BASIC,
	NUMBERS,
	DISTANCE_1,
	DISTANCE_2,
	POSITION
} state=BASIC;
int number;
int click_x;
int click_y;

void wait_distance();
void wait_position();


typedef struct command_char {
	guint pressed_char;
	void (*function)(void);
} command_char;

typedef struct number_char {
	guint pressed_char;
	void (*function)(int);
} number_char;


//pocislove pismenka
static number_char number_chars[] = {
	{ GDK_p, key_jump },
	{ GDK_P, key_jump },
	{ GDK_Page_Down, key_jump_down },
	{ GDK_Page_Up, key_jump_up },
        { GDK_o, key_this_page_has_number },
	{ GDK_c, key_set_columns},
	{ GDK_l, key_set_rows},
	{ GDK_e, key_presentation_mode },
};

//jednopismenka pozor tohle se pusti opravdu _vzdy_
static command_char basic_command_chars[] = {
	{ GDK_space, key_down }, 	// dolu o 1 stranku
	{ GDK_BackSpace, key_down },	// nahoru o 1 stranku
	{ GDK_Home, key_home },
	{ GDK_End, key_end },
	{ GDK_a, key_rotate },
	{ GDK_A, key_rotate_document },
	{ GDK_q, key_quit },
	{ GDK_F11, key_fullscreen },
	{ GDK_f, key_fullscreen },
	{ GDK_r, key_reload },
	//klikaci pismenka
	{ GDK_d, wait_distance },
	{ GDK_D, wait_position },
};
static command_char page_command_chars[] = {
	{ GDK_Right, key_down }, 		// dolu o 1 stranku
	{ GDK_Left, key_up },			// nahoru o 1 stranku
	{ GDK_Down, key_row_down },		// dolu o 1 radu
	{ GDK_Up, key_row_up },			// nahoru o radu
	{ GDK_Page_Down, key_screan_down },	// dolu o tabulku
	{ GDK_Page_Up, key_screan_up },		// nahoru o tabulku
	{ GDK_z, key_zoom_mode },
	{ GDK_u, key_crop},
};
static command_char zoom_command_chars[] = {
	{ GDK_Escape, key_page_mode },
};

void wait_distance(){ 	state = DISTANCE_1;}
void wait_position(){ 	state = POSITION;}

void aplicate_function(guint keyval, command_char *arr, int count){
	for (int i=0; i < count; i++)
		if (arr[i].pressed_char == keyval) {
			arr[i].function();
			break;
		}
}

void handling_key(guint keyval){
	switch(mode){
		case START:
			break;
		case PRESENTATION:
			key_page_mode();
		case PAGE:
			switch(state){
				case BASIC:
					aplicate_function(keyval,page_command_chars,sizeof(page_command_chars)/sizeof(page_command_chars[0]));
					if ((keyval >= GDK_0) && (keyval <= GDK_9)){
						state = NUMBERS;
						number = keyval - GDK_0;
					}	
					break;
				case NUMBERS:
					if ((keyval >= GDK_0) && (keyval <= GDK_9)){
						number = number*10 + keyval - GDK_0;
						break;
					}
					for (unsigned int i=0; i < sizeof(number_chars) / sizeof(number_chars[0]); i++)
						if (number_chars[i].pressed_char == keyval) {
							number_chars[i].function(number);
							break;
						}
					state = BASIC;

					break;
				default:
					if (key_cancel_waiting)
						state = BASIC;
					break;
			}
			break;
		case ZOOM:
			aplicate_function(keyval,zoom_command_chars,sizeof(zoom_command_chars)/sizeof(zoom_command_chars[0]));
			break;
	}
	aplicate_function(keyval,basic_command_chars,sizeof(basic_command_chars)/sizeof(basic_command_chars[0]));
}

void handling_click(int x, int y){
	switch(mode){
		case PAGE:
			switch(state){
				case BASIC: case NUMBERS:
					break;
				case DISTANCE_1:
					click_x=x;
					click_y=y;
					state=DISTANCE_2;
					break;
				case DISTANCE_2:
					click_distance(click_x,click_y,x,y);
					state=BASIC;
					break;
				case POSITION:
					click_position(x,y);
					state=BASIC;
					break;
			}
		default:
			break;
	}
}

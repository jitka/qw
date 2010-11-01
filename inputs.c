#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "input_functions.h"
#include "settings.h"
//todo: prelozit PREPINAC

enum {
	BASIC, //ceka
	NUMBERS, //ceka na cisla 
	DISTANCE_1, //ceka na kliknuti
	DISTANCE_2,
	POSITION, 
	PREPINAC, //je aktualne zmackle ctrl
} state=BASIC;
int number;
int number_is_negativ;
int click_x;
int click_y;

void wait_distance(){ 	state = DISTANCE_1;}
void wait_position(){ 	state = POSITION;}

static struct {
	unsigned int mode_mask;
	guint pressed_char;
	void (*function)(int);
} number_chars[] = {
	//vzdy
	{ ~0, GDK_p, key_jump },
	{ ~0, GDK_P, key_jump },
	{ ~0, GDK_Page_Down, key_jump_down },
	{ ~0, GDK_Page_Up, key_jump_up },
        { ~0, GDK_o, key_this_page_has_number },
	//tabulka
	{ PAGE|PRESENTATION, GDK_c, key_set_columns},
	{ PAGE|PRESENTATION, GDK_l, key_set_rows},
	{ PAGE|PRESENTATION, GDK_e, key_presentation_mode },
};

static struct {
	unsigned int mode_mask;
	guint pressed_char;
	void (*function)(void);
} command_chars[] = {
	//vzdy
	{ ~0, GDK_space, key_down }, 		// dolu o 1 stranku
	{ ~0, GDK_BackSpace, key_down },	// nahoru o 1 stranku
	{ ~0, GDK_Home, key_home },
	{ ~0, GDK_End, key_end },
	{ ~0, GDK_a, key_rotate },
	{ ~0, GDK_A, key_rotate_document },
	{ ~0, GDK_q, key_quit },
	{ ~0, GDK_F11, key_fullscreen },
	{ ~0, GDK_f, key_fullscreen },
	{ ~0, GDK_r, key_reload },
	{ ~0, GDK_c, key_center },
	//klikaci pismenka budou vzdy
	{ PAGE, GDK_d, wait_distance },
	{ PAGE, GDK_D, wait_position },
	//tabulka
//	{ PAGE|PRESENTATION, GDK_Right, key_down }, 		// dolu o 1 stranku
//	{ PAGE|PRESENTATION, GDK_Left, key_up },		// nahoru o 1 stranku
//	{ PAGE|PRESENTATION, GDK_Down, key_row_down },		// dolu o 1 radu
//	{ PAGE|PRESENTATION, GDK_Up, key_row_up },		// nahoru o radu
	{ PAGE|PRESENTATION, GDK_Page_Down, key_screan_down },	// dolu o tabulku
	{ PAGE|PRESENTATION, GDK_Page_Up, key_screan_up },	// nahoru o tabulku
	{ PAGE|PRESENTATION, GDK_z, key_zoom_mode },
	{ PAGE|PRESENTATION, GDK_u, key_crop},
	//zoom
	{ ZOOM, GDK_Escape, key_page_mode },
	{ ZOOM, GDK_plus, key_zoom_in },
	{ ZOOM, GDK_KP_Add, key_zoom_in },
	{ ZOOM, GDK_minus, key_zoom_out },
	{ ZOOM, GDK_KP_Subtract, key_zoom_out },
	{ ~0, GDK_Right, key_zoom_right },	
	{ ~0, GDK_Left, key_zoom_left },		
	{ ~0, GDK_Down, key_zoom_down },		
	{ ~0, GDK_Up, key_zoom_up },		
};

void handle_key(guint keyval){
	printf ("keyval %x\n",keyval);
	if (mode == PRESENTATION)
		key_page_mode();
	switch(state){
		case NUMBERS:
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
							(command_chars[i].mode_mask & mode) ) {
						number_chars[i].function(number);
					}
				state = BASIC;
			}
			break;
		case BASIC:

			// kdyby zacaly chodit cisla
			state = NUMBERS;
			number = 0;
			number_is_negativ = 0;
			if (keyval == GDK_plus || keyval == GDK_KP_Add){
			printf("tu plus%d\n",number_is_negativ);
			} else if (keyval == GDK_minus || keyval == GDK_KP_Subtract){
				number_is_negativ = 42;
			printf("tu minus%d\n",number_is_negativ);
			} else if ((keyval >= GDK_0) && (keyval <= GDK_9)){
				number = keyval - GDK_0;
			} else if ((keyval >= GDK_KP_0) && (keyval <= GDK_KP_9)){
				number = keyval - GDK_KP_0;
			} else { //nezacaly chodit cisla...
				state = BASIC;
			}
//			printf(" %d\n",number_is_negativ);

			for (unsigned int i=0; i < sizeof(command_chars)/sizeof(command_chars[0]); i++)
				if ( command_chars[i].pressed_char == keyval &&
						(command_chars[i].mode_mask & mode) ) {
					command_chars[i].function();
//					break;
				}		
			break;
		default:
			if (key_cancel_waiting)
				state = BASIC;
			break;
	}
}

void handle_click(int x, int y){
	switch(state){
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
		default: 
			break;
	}
}

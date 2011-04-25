#include <gdk/gdkkeysyms.h>
#include "input_functions.h"
#include "settings.h"
/* zatim nemam konfigurak
 * par veci lze nastavit zde
 * vsechny rozumne hodnoty by meli fungovat
 * muzete prirazit k pismenum vic funkci i naopak
 * hodne stesti pri experimentovani
 */

int key_cancel_waiting = TRUE; //pri mereni
int keep_scale = TRUE; //zachovává přiblížení a polohu při posouvání stran
int margin = 5; //sirka mezery v pixelech
int start_window_w = 400; 
int start_window_h = 500;
int minimum_width = 10; //nejmenší možná velikost stránky (pokud dáte menší neručím za chyby)
int minimum_height = 10;
int cache_size = 2000000; //v poctu pixelu
double zoom_speed = 1.5;
int move_delta = 10; //o kolikatinu stranky posouvaji sipky
int presentation_in_fullscreen = FALSE;
int start_maximalized = FALSE;
int start_fullscreen = FALSE;
int start_columns = 1;
int start_rows = 1;
char *start_comand = "C"; //jsou zavolany po prvnim expose
int parallel_window = TRUE; //jestli jsou vytvorene vsechny okna naraz

number_chars_t number_chars[] = {
	{ 0, GDK_g, key_jump },
	{ 0, GDK_Page_Down, key_jump_down },
	{ 0, GDK_Page_Up, key_jump_up },
        { 0, GDK_o, key_this_page_has_number },
	{ 0, GDK_c, key_set_columns},
	{ 0, GDK_r, key_set_rows},
	{ 0, GDK_F5, key_presentation_mode },
};

command_chars_t command_chars[] = {
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
	{ 0, GDK_C, key_crop},
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

int number_chars_count = sizeof(number_chars) / sizeof(*number_chars);
int command_chars_count = sizeof(command_chars) / sizeof(*command_chars);



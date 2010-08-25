#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "main.h"

/* kouzelna krabicka na zpracovani klaves
 * stavy: 
 * 1 - zakladni
 * 	prima jednoznakove rozkazy, muze prejit do rezimu 2,3
 * 2 - cisliska
 * 	pricita cislo jak ho uzivatel zada ukonci se neciselnym znakem
 * 	podle toho se cislo vyuzije a smazne
 * 3 - po znaku se ceka neco delsiho, mozna nekdy bude hledani
 * 4-6 - ceka na kliknuti (zrusi to stisknuti klavesy?)
 *
 */

int state=1;
int number;
int click_x;
int click_y;

void wait_distance();
void wait_position();


struct command_char {
	guint pressed_char;
	void (*function)(void);
};
struct number_char {
	guint pressed_char;
	void (*function)(int);
};
typedef struct command_char click_char;


//pocislove pismenka
static struct number_char number_chars[] = {
	{ GDK_p, key_jump },
	{ GDK_P, key_jump },
	{ GDK_Page_Down, key_jump_down },
	{ GDK_Page_Up, key_jump_up }
};

//jednopismenka
static struct command_char command_chars[] = {
	{ GDK_Page_Down, key_down }, 	// dolu o stranu/radu
	{ GDK_Right, key_down }, 	// dolu o stranu/stranku
	{ GDK_Down, key_down },		// dolu o stranu/kousek
	{ GDK_Page_Up, key_up },	// nahoru o stranu/radu
	{ GDK_Left, key_up },		// nahoru o stranu/stranku
	{ GDK_Up, key_up },		// nahoru o stranu/kousek
	{ GDK_Home, key_home },
	{ GDK_End, key_end },
	//klikaci pismenka
	{ GDK_d, wait_distance },
	{ GDK_D, wait_position },
};


void wait_distance(){ 	state = 4;}
void wait_position(){ 	state = 6;}

void handling_key(guint keyval){
	switch(state){
		case 1:
//			printf("key press %d\n",keyval);
			for (unsigned int i=0; i < sizeof(command_chars) / sizeof(command_chars[0]); i++)
				if (command_chars[i].pressed_char == keyval) {
					command_chars[i].function();
					break;
				}
			if ((keyval >= GDK_0) && (keyval <= GDK_9)){
				state = 2;
				number = keyval - GDK_0;
			}	
 			break;
		case 2:
			if ((keyval >= GDK_0) && (keyval <= GDK_9)){
				number = number*10 + keyval - GDK_0;
				break;
			}
			for (unsigned int i=0; i < sizeof(number_chars) / sizeof(number_chars[0]); i++)
				if (number_chars[i].pressed_char == keyval) {
					number_chars[i].function(number);
					break;
				}
			state = 1;
		
			break;
	}
}

void handling_click(int x, int y){
	switch(state){
		case 4:
			click_x=x;
			click_y=y;
			state=5;
			break;
		case 5:
			click_distance(click_x,click_y,x,y);
			state=1;
			break;
		case 6:
			click_position(x,y);
			state=1;
			break;
	}
}

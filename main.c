#include <stdio.h>
#include <stdlib.h>
#include <string.h> //open_file
#include <sys/stat.h> //hlidani zmen v souboru
#include <unistd.h> //cwd
#include <getopt.h> //parametry
#include <libintl.h> //preklady
#include <math.h> //sqrt
#include <gdk-pixbuf/gdk-pixbuf.h> //kvuli ikonce
//#include <gdk/gdk.h> //okynka,poppler.h pixbuffer.h
//#include <glib-2.0/glib.h>
#include "poppler.h" //open_file
#include "render.h" //render,expose
#include "inputs.h" //vstup -> funkce
#include "settings.h"
#define _(X) gettext(X)

//global
view_mode_t mode = START;
int current_page = 0;
static int page_number_shift = -1; //lide pocitaji od 1
static guint timer_id;

//settings
int key_cancel_waiting = TRUE;
int keep_scale = FALSE;
int margin = 5; //sirka mezery v pixelech
int start_window_width = 400;
int start_window_height = 500;
int maximum_displayed = 1000;
int minimum_width = 10;
int minimum_height = 10;
int cache_size = 2000000; //v poctu pixelu
double zoom_speed = 1.5;
int zoom_shift = 10; //o kolik posouvaji sipky
int presentation_in_fullscreen = FALSE;
int start_maximalized = FALSE;
int start_fullscreen = FALSE;

extern document_t *document;

//file
static char * file_path;
static time_t modification_time;

//window
GMainLoop *mainloop;
GdkWindow *window;
GdkGC *gdkGC;
int is_fullscreen = FALSE;
int window_width;
int window_height;


/////////////////////////////////////////////////////////////////
	
void open_file(char *path){
	if (path == NULL){
		fprintf(stderr,_("Nebylo zadáno jmeno souboru.\n"));
		exit(1);
	}
	//absoulutni adresa
	char pwd[1024];
	if (!getcwd(pwd, sizeof(pwd))) {
		fprintf(stderr,_("au getcwd \n"));
		exit(1);
	}
	char abs_path[strlen("file://")+strlen(path)+1+strlen(pwd)+1];
	strcpy(abs_path,"file://");	
	if (*path != '/'){
		strcat(abs_path,pwd);	
		strcat(abs_path,"/");	
	}
	strcat(abs_path,path);
	//otevreni
	char *err;
	err = pdf_init(abs_path);
	if (err != NULL){
		fprintf(stderr,_("Chyba načtení: %s\n"),err);
		exit(1);
	}				
	struct stat s;
	if (stat(file_path, &s) != -1){
		modification_time = s.st_mtime;
	}
}

void change_page(int new){
	if (new <0 || new >= document->number_pages)
		return;
	current_page=new;
	if (!keep_scale)
		document->scale = UNKNOWN;
	render(document);
	expose();
}

void key_up(){ 		change_page(current_page-1);}
void key_down(){ 	change_page(current_page+1);}
void key_row_up(){ 	change_page(current_page-document->columns);}
void key_row_down(){ 	change_page(current_page+document->columns);}
void key_screan_up(){ 	change_page(current_page-document->columns*document->rows);}
void key_screan_down(){ change_page(current_page+document->columns*document->rows);}
void key_home(){ 	change_page(0);}
void key_end(){ 	change_page(document->number_pages-1);}
void key_jump(int num_page){ 	change_page(num_page+page_number_shift);}
void key_jump_up(int diff){ 	change_page(current_page - diff);}
void key_jump_down(int diff){ 	change_page(current_page + diff);}
void key_this_page_has_number(int printed_number){ 	page_number_shift = -printed_number+current_page;}


void key_zoom_in(){ 	change_scale(document->scale*zoom_speed);}
void key_zoom_out(){ 	change_scale(document->scale/zoom_speed);}

void key_zoom_right(){ 	document->zoom_shift_w += zoom_shift; render(document); expose();}
void key_zoom_left(){ 	document->zoom_shift_w -= zoom_shift; render(document); expose();}
void key_zoom_down(){ 	document->zoom_shift_h += zoom_shift; render(document); expose();}
void key_zoom_up(){ 	document->zoom_shift_h -= zoom_shift; render(document); expose();}

void key_quit(){
	gdk_event_put( gdk_event_new(GDK_DELETE));
}

void key_fullscreen(){
	if (is_fullscreen){
		if (presentation_in_fullscreen && mode==PRESENTATION)
			return;
		gdk_window_unfullscreen(window);
		is_fullscreen = 0;
	} else {
		gdk_window_fullscreen(window);
		is_fullscreen = 1;
	}
}

void key_rotate(){ 
	document->pages[current_page].rotation = (document->pages[current_page].rotation + 90) % 360;
	render(document); expose();
}

void key_rotate_document(){
	document->rotation = (document->rotation+90)%360; 
	render(document); expose();
}

void key_reload(){
	document_t *new;
	open_file(file_path);
	new = document_create_databse();
	render_set_max_columns(new);
	render(new);
	document_delete_database(document);
	document=new;
}

void key_set_columns(int c){
	if (c * document->rows >= maximum_displayed
			|| c > document->max_columns)
		return;
	document->columns = c; 
	render(document); expose(); 
}
void key_set_rows(int r){
	if (r * document->columns >= maximum_displayed
			|| r > document->max_rows)
		return;
	document->rows = r; 
	render(document); expose(); 
}
void key_page_mode(){
	if (mode == PRESENTATION)
		 g_source_remove (timer_id); //odstrani vlakno casovace	
	if (!keep_scale) //v zoomu
		document->scale = UNKNOWN;
	mode=PAGE; 
	render(document); expose();
}
void key_zoom_mode(){//,posunuti	
	if (mode == PRESENTATION)
		 g_source_remove (timer_id);	
	mode=ZOOM; 
	render(document); expose();
}
static gboolean timeout_callback (gpointer data){
	data = NULL; //at mi to nehazi warningy
	key_screan_down(); 
	return TRUE; //volat se znova
}
void key_presentation_mode(int time){ 
	if (presentation_in_fullscreen && !is_fullscreen)
		key_fullscreen();
	timer_id = gdk_threads_add_timeout(
			time*1000,
			timeout_callback,
			NULL
	);
	mode=PRESENTATION;
}

void click_distance(int first_x, int first_y, int second_x, int second_y){
	int page_1,page_2;
	int x1,x2,y1,y2;
	int height,width;
	render_get_relative_position(first_x,first_y,&page_1,&x1,&y1,&height,&width);
	render_get_relative_position(second_x,second_y,&page_2,&x2,&y2,&height,&width);
	if (page_1 != page_2 || page_1 == -1 || page_2 == -1){
		printf(_("Priste klikejte do stejneho obrazku.\n"));
		return;
	}
	printf(_("strana %d (cislovano od jedne) %d (logicke cislovani)\n"
			"%lf cm x %lf cm     %d x %d pikelu\n"
			//"kliknuti 1: %d,%d kliknuti 2: %d,%d\n"
			"vzdalenot %lf cm %lf pixelu\n\n"),
			page_1+1,
			page_1-page_number_shift,
			document->pages[page_1].width*0.035278,
			document->pages[page_1].height*0.035278,
			width,
			height,
			//x1,y1,x2,y2,
			sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) / width * document->pages[page_1].width*0.035278,
			sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2))
			);
}
void click_position(int click_x, int click_y){
	int page;
	int x,y;
	int height,width;
	render_get_relative_position(click_x,click_y,&page,&x,&y,&height,&width);
	if (page == -1){
		printf(_("Priste klikejte do obrazku.\n"));
		return;
	}
	printf(_("strana %d (cislovano od jedne) %d (logicke cislovani)\n"
			"%lf cm x %lf cm     %d x %d pikelu\n"
			"kliknuti: %lf,%lf cm %d,%d pixelu \n\n"),
			page+1,
			page-page_number_shift,
			document->pages[page].width*0.035278,
			document->pages[page].height*0.035278,
			width,
			height,
			(double) x / width * document->pages[page].width*0.035278,
			(double) y / height * document->pages[page].height*0.035278,
			x,y
			);
}

///////////////////////////////////////////////////////////////////////////////////

static void event_func(GdkEvent *ev, gpointer data) {
	data = NULL; //nemam rada warningy
	switch(ev->type) {
		case GDK_KEY_PRESS:
			handle_key(ev->key.keyval);
			break;
		case GDK_BUTTON_PRESS:
			if (ev->button.button == 1)
				handle_click((int)ev->button.x,(int)ev->button.y);
			break;
		case GDK_FOCUS_CHANGE:
			{
				struct stat s;
				if (stat(file_path, &s) == -1)
					break; //stat neprosel
				if (modification_time != s.st_mtime && mode != PRESENTATION)
					key_reload();
				break;
			}
		case GDK_CONFIGURE: //zmena pozici ci velikosti-zavola exspose
			{
				int w_width, w_height;
				gdk_drawable_get_size(window,&w_width,&w_height);
				if (window_width != w_width || window_height != w_height){
					//zmenila se velkost okna
					window_width = w_width;
					window_height = w_height;
					render_set_max_columns(document);
					render(document);		
					expose();
				}
				break;
			}
		case GDK_EXPOSE:
			//todo: rekne mi kolik jich je ve fronte-> zabijeni zbytecnych rendrovani
			switch (mode) {
				case START:
					//todo: jeste by se tu dal vlozit nejaky hezky obrazek
					mode = PAGE;
					key_reload();
					break;
				default:
					expose();
					break;
			}
			break;
		case GDK_DELETE:
			g_main_loop_quit(mainloop);
			break;
		default:
			break;
	}
}

int main(int argc, char * argv[]) {

	g_type_init();
	//zpracovani parametru
	const char *short_options = "hi:p:";
	const struct option long_options[] = {
		{ "help",    0, NULL, 'h' },
		{ "page",   1, NULL, 'p' },
		{ NULL,      0, NULL,  0  }
	};
	int next_option;
	
	do {
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);
		switch(next_option){
			case 'h': 
				printf(_("Nápověda qw: \n"
					" -h 	-help 	nápověda\n"
					" -p 	-page 	počáteční stránka\n"
					" \n"
					"Ovladání:\n"
					" fungují běžné pohybové klávesy\n"
					" 2p posune na stranu dvě\n"
					" 3 + PageDown posune o dvě strany dál\n"
					" 42o nastavi cislo aktualni stranky na 42 a ostatni podobne precisluje\n"
					" r znovu nacte soubor\n"
					" a otoci vsechny stranky o 90 stupňů\n"
					" A otočí pouze aktuální stránku o 90 stupňů\n"
					" f nebo F11 přepne na fullscrean\n"
					" q ukončí program\n"
					" \n"
					"program má tři mody:\n"
					"   tabulka:\n"
					" 	je základní stav\n"
					" 	2c nastavi dve strany vedle sebe\n"
					" 	3l nastavi tri radky\n"
					" 	d a dvě kliknutí změří vzdálenost\n"
					" 	D a kliknutí napíše pozici\n"
					" 	c ořeže okno na použitelnou velikost\n"
					" 	z přepne do zoom modu\n"
					" 	1e spusti mod prezentace s 1s intervalem\n"
					"   prezentace:\n"
					" 	vypada jako tabulka samy se posouvaji stranky\n"
					" 	libovolnou klavesou jde vyskočit\n"
					"   zoom:\n"
					" 	+- přiblužuje\n"
					" 	šipečky posouvaji stránku\n"
					));
				return 0;
			case 'p':
				current_page = atoi(optarg) - 1; //lide cisluji od 1
				break;
			case '?': 
				fprintf(stderr,"Špatné volby. Po nápovědu pište -h nebo -help.\n");
				return 1;
			case -1 :
				break;
		}
	} while (next_option != -1);
	file_path = argv[optind]; //vim ze tohle obecne nefunguje, ale tady to staci

	open_file(file_path); //ověří, jestli soubor je skutečně pdf

	//vytvoreni okna
	gdk_init(NULL,NULL);

	GdkVisual *visual = gdk_visual_get_system();
	GdkColormap *colormap = gdk_colormap_new(visual,TRUE);

	GdkWindowAttr attr = {
		NULL, //gchar *title; //nefunguje
		0x3FFFFE, //gint event_mask; all events mask
		100,0, //gint x, y;
		start_window_width,start_window_height,
	 	GDK_INPUT_OUTPUT, //GdkWindowClass wclass;
		visual, //GdkVisual *visual;
		colormap, //GdkColormap *colormap;
		GDK_WINDOW_TOPLEVEL, //GdkWindowType window_type;
		GDK_X_CURSOR, //GdkCursor *cursor;
		NULL, //gchar *wmclass_name;
		NULL, //gchar *wmclass_class;
		TRUE, //gboolean override_redirect; ???
		GDK_WINDOW_TYPE_HINT_NORMAL, //GdkWindowTypeHint type_hint;
	};

	window = gdk_window_new(
			NULL, //GdkWindow *parent,
			&attr, //GdkWindowAttr *attributes,
			0 //gint attributes_mask ????
	);
	gdkGC  = gdk_gc_new(window);

	window_width = start_window_width;
	window_height = start_window_height;

	//ikona
	GdkPixbuf * icon_pixbuf = gdk_pixbuf_new_from_file("icon.png",NULL);
	GList* icons = g_list_append(NULL,icon_pixbuf);	
	gdk_window_set_icon_list(window,icons);

	//titulek
	gdk_window_set_title(window,"huiii");

	if (start_maximalized)
		gdk_window_maximize(window);
	if (start_fullscreen)
		gdk_window_fullscreen(window);
	//spusteni programu
	gdk_window_show(window);
	gdk_event_handler_set(event_func, NULL, NULL);
	mainloop = g_main_loop_new(g_main_context_default(), FALSE);	
	g_main_loop_run(mainloop);
	gdk_window_destroy(window); 

	return 0;
}

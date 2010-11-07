#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //hlidani zmen v souboru
#include <getopt.h> //parametry
#include <libintl.h> //preklady
#include <math.h> //sqrt
#include <fcntl.h> //presmerovani chyboveho vystupu
#include <gdk-pixbuf/gdk-pixbuf.h> //kvuli ikonce
#include <gdk/gdkkeysyms.h> //prepinace
//#include <gdk/gdk.h> //okynka,poppler.h pixbuffer.h
//#include <glib-2.0/glib.h>
#include "backend.h" //open_file
#include "render.h" //render,expose
#include "inputs.h" //vstup -> funkce
#include "settings.h"
#define _(X) gettext(X)
#define IN_CENTIMETRES *2159/61200
//spectre, poppler meri v 72tiny palce / pspoint

//global
view_mode_t mode = START;
file_type_t file_type;
int current_page = 0;
int need_render = FALSE;
static int page_number_shift = -1; //lide pocitaji od 1
static guint timer_id;

//settings
int key_cancel_waiting = TRUE;
int keep_scale = FALSE;
int margin = 5; //sirka mezery v pixelech
int start_window_width = 400;
int start_window_height = 500;
int minimum_width = 10;
int minimum_height = 10;
int cache_size = 2000000; //v poctu pixelu
double zoom_speed = 1.5;
int move_shift = 10; //o kolik posouvaji sipky
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
		printf(_("Nebylo zadáno jmeno souboru.\n"));
		exit(1);
	}
	if (!doc_init(path)){
		printf(_("Load error\n"));
		exit(1);
	}
	//cas posledni zmeny	
	struct stat s;
	if (stat(path, &s) != -1){
		modification_time = s.st_mtime;
	}
}


void key_center(){
	document->center_h = window_height/2;
	document->center_w = window_width/2;
	compute_space_center(document);
	render(document);
}

void change_page(int new){
	if (new <0 || new >= document->number_pages)
		return;
	current_page=new;
	if (!keep_scale)
		key_center();
	else {
		render(document);
		expose();
	}
}

void key_prev_page(){ 		change_page(current_page-1);}
void key_next_page(){ 	change_page(current_page+1);}
void key_prev_row(){ 	change_page(current_page-document->columns);}
void key_next_row(){ 	change_page(current_page+document->columns);}
void key_prev_screan(){ 	change_page(current_page-document->columns*document->rows);}
void key_next_screan(){ change_page(current_page+document->columns*document->rows);}
void key_home(){ 	change_page(0);}
void key_end(){ 	change_page(document->number_pages-1);}
void key_jump(int num_page){ 	change_page(num_page+page_number_shift);}
void key_jump_up(int diff){ 	change_page(current_page - diff);}
void key_jump_down(int diff){ 	change_page(current_page + diff);}
void key_this_page_has_number(int printed_number){ 	page_number_shift = -printed_number+current_page;}

void key_move_right(){ 	document->center_w += move_shift; render(document); expose();}
void key_move_left(){ 	document->center_w -= move_shift; render(document); expose();}
void key_move_down(){ 	document->center_h += move_shift; render(document); expose();}
void key_move_up(){ 	document->center_h -= move_shift; render(document); expose();}
void move(int x, int y){ document->center_w +=x; document->center_h += y; render(document); expose();}

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

void key_rotate_document_right(){
	document->rotation = (document->rotation+90)%360; 
	render(document); expose();
}
void key_rotate_document_left(){
	document->rotation = (document->rotation-90)%360; 
	render(document); expose();
}
void key_rotate_page_right(){
	document->pages[current_page].rotation = (document->pages[current_page].rotation + 90) % 360;
	render(document); expose();
}
void key_rotate_page_left(){
	document->pages[current_page].rotation = (document->pages[current_page].rotation - 90) % 360;
	render(document); expose();
}

void key_reload(){
	if (mode == PRESENTATION)
	 	g_source_remove (timer_id); //odstrani vlakno casovace	
	document_t *new;
	open_file(file_path);
	new = document_create_databse();
	compute_space_center(new);
	render(new);
	document_delete_database(document);
	document=new;
}

void key_set_columns(int c){
	if (c > document->max_columns || c < 1)
		return;
	document->columns = c;
	key_center();
}
void key_set_rows(int r){
	if (r > document->max_rows || r < 1)
		return;
	document->rows = r; 
	key_center();
}
void key_page_mode(){
	if (mode == PRESENTATION)
		 g_source_remove (timer_id); //odstrani vlakno casovace	
	mode=PAGE; 
	key_center();
}
static gboolean timeout_callback (gpointer data){
	data = NULL; //at mi to nehazi warningy
	GdkEvent *tmp = gdk_event_new(GDK_CLIENT_EVENT);
	//gdkclient je vazne moc moc divne
	//nejak tam dopisu o co se jedna
	gdk_event_put(tmp);
	return TRUE; //volat se znova
}
void key_presentation_mode(int time){ 
	if (presentation_in_fullscreen && !is_fullscreen)
		key_fullscreen();
	key_center();
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
			document->pages[page_1].width IN_CENTIMETRES,
			document->pages[page_1].height IN_CENTIMETRES,
			width,
			height,
			//x1,y1,x2,y2,
			sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) / width*document->pages[page_1].width IN_CENTIMETRES,
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
			document->pages[page].width IN_CENTIMETRES,
			document->pages[page].height IN_CENTIMETRES,
			width,
			height,
			(double) x / width * document->pages[page].width IN_CENTIMETRES,
			(double) y / height * document->pages[page].height IN_CENTIMETRES,
			x,y
			);
}

///////////////////////////////////////////////////////////////////////////////////

static void event_func(GdkEvent *ev, gpointer data) {
	data = NULL; //nemam rada warningy
//	printf("%d\n",ev->type);
	switch(ev->type) {
		case GDK_KEY_PRESS:
			handle_key(ev->key.keyval,ev->key.state);
			break;
		case GDK_BUTTON_PRESS:
			if (ev->button.button == 1 || ev->button.button == 3)
				handle_click((int)ev->button.x,(int)ev->button.y);
			break;
		case GDK_MOTION_NOTIFY:
			if (ev->motion.state & GDK_BUTTON1_MASK){
				handle_motion((int)ev->motion.x,(int)ev->motion.y);
				gdk_event_request_motions(&ev->motion);
			}
			if (ev->motion.state & GDK_BUTTON3_MASK){
				handle_motion2((int)ev->motion.x,(int)ev->motion.y);
				gdk_event_request_motions(&ev->motion);
			}
			break;
		case GDK_CLIENT_EVENT:
			//presentation
			key_next_screan(); 
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
					key_center(); //tam je i render
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
//					printf("expose: %dx%d\n",ev->expose.area.width,ev->expose.area.height);
					expose();
					break;
			}
			break;
		case GDK_NOTHING:
//			printf("nothing\n");
			if (need_render){
//				render_displayed_pixbuf(document);
//				clean_window(document);
//				printf("rendering\n");
//				need_render = FALSE;
			} //tady by se dalo vykrestlovat dopredu
			break;
		case GDK_DELETE:
			g_main_loop_quit(mainloop);
			break;
		default:
			break;
	}
}

int main(int argc, char * argv[]) {


//	close(2); 
//	open("/dev/null", O_RDWR);
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
//	printf("%d\n",file_type);
//	double w,h;
//	doc_page_get_size(0,&w,&h);
//	printf("stran %d, %lfx%lfcm\n",doc_get_number_pages(),w IN_CENTIMETRES,h IN_CENTIMETRES);
	//vytvoreni okna
	gdk_init(NULL,NULL);

	GdkVisual *visual = gdk_visual_get_system();
	GdkColormap *colormap = gdk_colormap_new(visual,TRUE);

	GdkWindowAttr attr = {
		NULL, //gchar *title; //nefunguje
		GDK_ALL_EVENTS_MASK, //gint event_mask;
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

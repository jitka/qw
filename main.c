#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //hlidani zmen v souboru
#include <getopt.h> //parametry
#include <libintl.h> //preklady
#include <math.h> //sqrt
#include <fcntl.h> //presmerovani chyboveho vystupu
#include <unistd.h> //dup2 na zahozeni chyb popplera
#include <errno.h>
//#include <gdk-pixbuf/gdk-pixbuf.h> //kvuli ikonce
//#include <gdk/gdkkeysyms.h> //prepinace
//#include <gdk/gdk.h> //okynka,poppler.h pixbuffer.h
//#include <glib-2.0/glib.h>
#include "backend.h" //open_file
#include "render.h" //render,expose
#include "inputs.h" //vstup -> funkce
#include "settings.h"
#include "window.h"
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

extern document_t *document;

//file
static char * file_path;
static time_t modification_time;

//window
GMainLoop *mainloop;
GdkWindow *window;
GdkGC *gdkGC;
int is_fullscreen = FALSE;
int window_w;
int window_h;
GdkCursor* cursor_basic;
GdkCursor* cursor_move;
GdkCursor* cursor_mesure;

/////////////////////////////////////////////////////////////////
	
void open_file(char *path){
	if (path == NULL){
		printf(_("No file given.\n"));
		exit(1);
	}

	//zahodim chybovy vystup at nema poppler kam zvanit
	int dn = open("/dev/null",O_WRONLY);
	int err2 = dup(2);
	dup2(dn,2);
	close(dn);

	if (!doc_init(path)){
		printf(_("Couldn't open file %s\n"),path);
		exit(1);
	}

	//nahodim chybovy vystup
	dup2(err2,2);
	close(err2);


	//cas posledni zmeny	
	struct stat s;
	if (stat(path, &s) != -1){
		modification_time = s.st_mtime;
	}
}


void key_center(){
	compute_space_center(document);
	if (document->rows == 0)
		document->ulc_h = 0;
	else
		document->ulc_h = window_h/2 - document->table_h/2;
	document->ulc_w = window_w/2 - document->table_w/2;
	render(document);
	expose();
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
void key_prev_screan(){ 	
	if (document->rows > 0)
		change_page(current_page-document->columns*document->rows);
	else{
		document->ulc_h += window_h;
		render(document);
		expose();
	}
}
void key_next_screan(){ 
	if (document->rows > 0)
		change_page(current_page+document->columns*document->rows);
	else{
		document->ulc_h -= window_h;
		render(document);
		expose();
	}
}
void key_home(){ 	change_page(0);}
void key_end(){ 	change_page(document->number_pages-1);}
void key_jump(int num_page){ 	change_page(num_page+page_number_shift);}
void key_jump_up(int diff){ 	change_page(current_page - diff);}
void key_jump_down(int diff){ 	change_page(current_page + diff);}
void key_this_page_has_number(int printed_number){ 	page_number_shift = -printed_number+current_page;}

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

void check_position(){
	if ( document->table_w < window_w ){ //male
		document->ulc_w = (window_w - document->table_w)/2;
	} else if ( document->ulc_w > 0 ) { //moc vlevo
		document->ulc_w = 0;
	} else if ( document->table_w + document->ulc_w < window_w ) //moc vpravo
		document->ulc_w += window_w - (document->table_w + document->ulc_w); 
	if ( document->rows > 0 ){
		if ( document->table_h < window_h ){ //male
			document->ulc_h = (window_h - document->table_h)/2;
		} else if ( document->ulc_h > 0 ) { //moc nahore
			document->ulc_h = 0;
		} else if ( document->table_h + document->ulc_h < window_h ) //moc dole
			document->ulc_h += window_h - (document->table_h + document->ulc_h); 
	}
}

void move(int x, int y){
	document->ulc_w += x; 
	document->ulc_h += y; 
	check_position();
	render(document); expose();
}
void key_move_right(){ 	move(window_w/move_delta,0);}
void key_move_left(){	move(-window_w/move_delta,0);}
void key_move_down(){	move(0,-window_w/move_delta);}
void key_move_up(){	move(0,window_w/move_delta);}

void change_scale(double scale){
	document->ulc_h += document->table_h/2;
	document->ulc_w += document->table_w/2;
	document->space_h *= scale;
	document->space_w = floor(document->space_h*document->aspect);
	document->table_h = document->rows * (document->space_h + margin) - margin;
	document->table_w = document->columns * (document->space_w + margin) - margin;
	document->ulc_h -= document->table_h/2;
	document->ulc_w -= document->table_w/2;
	check_position();
	render(document);
	expose();

}


void key_zoom_in(){ 	
	change_scale(zoom_speed);
}
void key_zoom_out(){
	if (document->space_h / zoom_speed > minimum_height &&
			document->space_w / zoom_speed > minimum_width)
		change_scale(1/zoom_speed);
}



void rotate(int rotation, int all){
	if (all)
		document->pages[current_page].rotation = rotation;
	else
		document->rotation = rotation;
	key_center();
}
void key_rotate_document_right(){
	rotate((document->rotation+90)%360,FALSE); 
}
void key_rotate_document_left(){
	rotate((document->rotation-90)%360,FALSE); 
}
void key_rotate_page_right(){
	rotate((document->pages[current_page].rotation+90)%360,TRUE);
}
void key_rotate_page_left(){
	rotate((document->pages[current_page].rotation-90)%360,TRUE);
}

void key_reload(){
	if (mode == PRESENTATION)
	 	g_source_remove (timer_id); //odstrani vlakno casovace	
	document_t *new;
	open_file(file_path);
	new = document_create_databse();
	compute_space_center(new);
	document_delete_database(document);
	document=new;
	key_center();
//	render(document);
//	expose();
}

void key_set_columns(int c){
	if (c > document->max_columns || c < 1)
		return;
	document->columns = c;
	key_center();
}
void key_set_rows(int r){
	if (r > document->max_rows || r < 0)
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
		printf(_("You must click to same picture\n"));
		return;
	}
	printf(_("page %d (numbered from 1) %d (user numbered)\n"
			"	%lf cm x %lf cm     %d x %d pixels\n"
			//"kliknuti 1: %d,%d kliknuti 2: %d,%d\n"
			//"kliknuti 1: %d,%d kliknuti 2: %d,%d\n"
			"distance %lf cm %lf pixels\n\n"),
			page_1+1,
			page_1-page_number_shift,
			document->pages[page_1].width IN_CENTIMETRES,
			document->pages[page_1].height IN_CENTIMETRES,
			width,
			height,
			//x1,y1,x2,y2,
			//first_x, first_y, second_x, second_y,
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
		printf(_("You must click to picture.\n"));
		return;
	}
	printf(_("page %d (numbered from 1) %d (user numbered)\n"
			"	%lf cm x %lf cm     %d x %d pixel\n"
			"clicked: %lf,%lf cm %d,%d pixel \n\n"),
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
		case GDK_BUTTON_RELEASE:
				handle_release();
			break;
		case GDK_MOTION_NOTIFY:
			if (ev->motion.state & (GDK_BUTTON1_MASK|GDK_BUTTON3_MASK)){
				handle_motion((int)ev->motion.x,(int)ev->motion.y,ev->motion.state);
				gdk_event_request_motions(&ev->motion);
			}
			break;
		case GDK_SCROLL:
			switch (ev->scroll.direction){
				case GDK_SCROLL_UP:
					key_move_up();
					break;	
				case GDK_SCROLL_DOWN:
					key_move_down();
					break;
				case GDK_SCROLL_LEFT:
					key_move_left();
					break;
				case GDK_SCROLL_RIGHT:
					key_move_right();
					break;
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
				if (window_w != w_width || window_h != w_height){
					//zmenila se velkost okna
					window_w = w_width;
					window_h = w_height;
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
					for (int i = 0; start_comand[i] != 0; i++)
						handle_key(start_comand[i],0);
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
				printf(_(
					"Usage: hui [OPINION]... [FILE] \n"
					"View pdf document.\n"
					"\n"
					"  -p 	--page[=NUMBER] start at page NUMBER\n"
					"  -h   --help 		display this help and exit\n"
					"\n"
//					"TODO\n"
//					"zmena cislovani\n"
//					"zapnuti na pozadi\n"
//					"velikost cache\n"
//					"nepouzivat informace z minula\n"
//					"info o soubru\n"
					"\n"
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

	for (; optind <argc; optind++){
		pid_t cpid = fork();
		if (cpid == -1) {
			perror("fork\n");
			exit(EXIT_FAILURE);
		}

		if (cpid == 0) {    //dite
			file_path = argv[optind]; //vim ze tohle obecne nefunguje, ale tady to staci

			open_file(file_path); //ověří, jestli soubor je skutečně pdf
			//	printf("%d\n",file_type);
			//	double w,h;
			//	doc_page_get_size(0,&w,&h);
			//	printf("stran %d, %lfx%lfcm\n",doc_get_number_pages(),w IN_CENTIMETRES,h IN_CENTIMETRES);
			//vytvoreni okna
			gdk_init(NULL,NULL);


			GdkDisplay *display = gdk_display_get_default();
			cursor_basic = gdk_cursor_new_for_display(display,GDK_LEFT_PTR);
			cursor_mesure = gdk_cursor_new_for_display(display,GDK_CROSSHAIR);
			cursor_move = gdk_cursor_new_for_display(display,GDK_FLEUR);

			//	GdkVisual *visual = gdk_visual_get_system();
			//	GdkColormap *colormap = gdk_colormap_new(visual,TRUE);

			GdkWindowAttr attr = {
				"huiii", //gchar *title; //nefunguje
				GDK_ALL_EVENTS_MASK, //gint event_mask;
				100,0, //gint x, y;
				start_window_w,start_window_h,
				GDK_INPUT_OUTPUT, //GdkWindowClass wclass;
				NULL, //GdkVisual *visual;
				NULL, //GdkColormap *colormap;
				GDK_WINDOW_TOPLEVEL, //GdkWindowType window_type;
				cursor_basic, //GdkCursor *cursor;
				NULL, //gchar *wmclass_name;
				NULL, //gchar *wmclass_class;
				TRUE, //gboolean override_redirect; ???
				GDK_WINDOW_TYPE_HINT_NORMAL, //GdkWindowTypeHint type_hint;
			};

			window = gdk_window_new(
					NULL, //GdkWindow *parent,
					&attr, //GdkWindowAttr *attributes,
					GDK_WA_TITLE|GDK_WA_X|GDK_WA_Y|GDK_WA_CURSOR //gint attributes_mask
					);

			gdkGC  = gdk_gc_new(window);

			window_w = start_window_w;
			window_h = start_window_h;

			//ikona
			/*	GdkPixbuf * icon_pixbuf = gdk_pixbuf_new_from_file("icon.png",NULL);
				GList* icons = g_list_append(NULL,icon_pixbuf);	
				gdk_window_set_icon_list(window,icons);
				*/


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
		} else {            //rodic
			//TODO toto je jen jedna varianta
			waitpid(cpid,NULL,0);
		}
	}

	return 0;
}

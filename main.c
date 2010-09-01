#include <stdio.h>
#include <stdlib.h>
#include <string.h> //open_file
#include <sys/stat.h> //hlidani zmen v souboru
#include <unistd.h> //cwd
#include <getopt.h> //parametry
#include <libintl.h> //preklady
//#include <gdk/gdk.h> //okynka,poppler.h pixbuffer.h
//#include <glib-2.0/glib.h>
#include "poppler.h" //open_file
#include "render.h" //render,expose
#include "inputs.h" //vstup -> funkce
#include "settings.h"
#define _(X) gettext(X)

//global
view_mode_t mode = START;
int is_fullscreen = FALSE;
int current_page = 0;
int page_number_shift = -1; //lide pocitaji od 1
//tady budou rezimy a veskere veci
//co se reloudem nemeni

//settings
int key_cancel_waiting = TRUE;
int margin = 20; //sirka mezery v pixelech
int start_window_width = 400;
int start_window_height = 500;
int start_window_maximalise = FALSE;
int start_window_fullscrean = FALSE;

//render veci co po reloudu zustavaji
extern document_t *document;
//extern document_t new_document;

//file
char * file_path;
time_t modification_time;

//window
GMainLoop *mainloop;
GdkWindow *window;
GdkGC *gdkGC;


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
	int old = current_page;
	current_page=new;
	render(document);
	pixbuf_free(&document->pixbufs,old);
	expose();
}

void key_up(){ 		change_page(current_page-1);}
void key_down(){ 	change_page(current_page+1);}
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
		gdk_window_unfullscreen(window);
		is_fullscreen = 0;
	} else {
		gdk_window_fullscreen(window);
		is_fullscreen = 1;
	}
}

void key_rotate(){ 
	document->pages[current_page].rotation = (document->pages[current_page].rotation + 90) % 360;
	render(document);
	expose();
}

void key_rotate_document(){
	document->rotation = (document->rotation+90)%360; 
	render(document);
	expose();
}

void key_reload(){
	document_t *new;
	open_file(file_path);
	new = document_create_databse();
	render(new);
	document_delete_database(document);
	document=new;
}

void key_set_columns(int c){	document->columns = c; render(document); expose(); }
void key_set_rows(int r){	document->rows = r; render(document); expose(); }
void key_page_mode(){	mode=PAGE; render(document); expose();}
void key_zoom_mode(){	mode=ZOOM; render(document); expose();}

void click_distance(int first_x, int first_y, int second_x, int second_y){
	printf(_("hui %f %f %d %d - %d %d\n"),document->pages[current_page].width,document->pages[current_page].height,first_x,first_y,second_x,second_y);
}
void click_position(int x, int y){
	printf(_("hui %f %f %d %d\n"),document->pages[current_page].width,document->pages[current_page].height,x,y);
}

///////////////////////////////////////////////////////////////////////////////////

static void event_func(GdkEvent *ev, gpointer data) {
	switch(ev->type) {
		case GDK_KEY_PRESS:
			handling_key(ev->key.keyval);
			break;
		case GDK_SCROLL:
//			printf("SC\n");
			break;
		case GDK_BUTTON_PRESS:
			if (ev->button.button == 1)
				handling_click((int)ev->button.x,(int)ev->button.x);
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
			render(document);		
			expose();
			break;
		case GDK_EXPOSE:
			//rekne mi kolik jich je ve fronte-> zabijeni zbytecnych rendrovani
			switch (mode) {
				case START:
					//jeste by se tu dal vlozit nejaky hezky obrazek
					mode = PAGE;
					key_reload();
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

	g_type_init()
		;
	//zpracovani parametru
	const char *short_options = "hi:p:";
	const struct option long_options[] = {
		{ "help",    0, NULL, 'h' },
		{ "info",   1, NULL, 'i' },
		{ "page",   1, NULL, 'p' },
		{ NULL,      0, NULL,  0  }
	};
	int next_option;
	
	do {
		next_option = getopt_long(argc, argv, short_options, long_options, NULL);
		switch(next_option){
			case 'h': 
				printf(_("Nápověda qw: \n"
					" šipečky posunuji, okynko se da zvětšit\n"
					" -h 	-help 	nápověda\n"
					" -p 	-page 	počáteční stránka\n"
					" -i 	-info 	info dokumentu\n"));
				return 0;
			case 'i':
				open_file(optarg);
//				printf("Stran: %d\n",pdf_num_pages);
//printf("Strana: %d sirka: %f cm vyska: %f cm\n",current_page,document->pages[current_page].width*0.035278,document->pages[current_page].height*0.035278);
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

	//ikona
	GdkPixmap* icon_pixmap = gdk_pixmap_create_from_xpm(window, NULL, NULL, "icon.xpm");
	GdkPixbuf * icon_pixbuf = gdk_pixbuf_get_from_drawable( NULL, icon_pixmap, colormap, 0,0, 0,0, 32,32);
	GList* icons = g_list_append(NULL,icon_pixbuf);	
	gdk_window_set_icon_list(window,icons);

	//titulek
	gdk_window_set_title(window,"huiii");

		//gdk_window_fullscreen(window);
	gdk_window_show(window);
	gdk_event_handler_set(event_func, NULL, NULL);
	mainloop = g_main_loop_new(g_main_context_default(), FALSE);	
	g_main_loop_run(mainloop);
	gdk_window_destroy(window); 

	return 0;
}

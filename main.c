#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> //parametry
#include <gdk/gdk.h> //okynka
#include <glib.h> 
#include "poppler.h"  //ovladani c++ knihovny
#include "inputs.h" //vstup -> funkce

int current_page = 0;
extern int pdf_num_pages;
int document_rotation = 0;
extern pdf_page pdf_page_1;
char * file_name;

GMainLoop *mainloop;
GdkWindow *root_window;
int is_fullscreen = FALSE;
GdkGC *gdkGC;

void render_page(){
	gint w_w,w_h;
	gdk_drawable_get_size(root_window,&w_w,&w_h);
	int p_w,p_h;
	double scale;
	int rotation = (pdf_page_1.rotation+document_rotation) % 360;

	if ((rotation)%180 == 0){
		if (w_w*pdf_page_1.height < pdf_page_1.width*w_h){
			//šířka je stejná
			p_w = w_w;
			p_h = ceil(w_w*pdf_page_1.height/pdf_page_1.width);
			scale = w_w/pdf_page_1.width;
			pdf_page_1.shift_width = 0;
			pdf_page_1.shift_height = (w_h-p_h)/2;
		} else {
			//výška je stejná
			p_w = ceil(w_h*(pdf_page_1.width/pdf_page_1.height));
			p_h = w_h;
			scale = w_h/pdf_page_1.height;
			pdf_page_1.shift_width = (w_w-p_w)/2;
			pdf_page_1.shift_height = 0;
		}
	} else {
		if (w_w*pdf_page_1.width < pdf_page_1.height*w_h){
			//šířka je stejná
			p_w = w_w;
			p_h = ceil(w_w*pdf_page_1.width/pdf_page_1.height);
			scale = w_w/pdf_page_1.height;
			pdf_page_1.shift_width = 0;
			pdf_page_1.shift_height = (w_h-p_h)/2;
		} else {
			//výška je stejná
			p_w = ceil(w_h*(pdf_page_1.height/pdf_page_1.width));
			p_h = w_h;
			scale = w_h/pdf_page_1.width;
			pdf_page_1.shift_width = (w_w-p_w)/2;
			pdf_page_1.shift_height = 0;
		}
	}
	if ( (pdf_page_1.pixbuf_width != p_w) 
			|| (pdf_page_1.pixbuf_height != p_h) 
			|| (pdf_page_1.pixbuf_rotation != rotation)){
		pdf_page_1.pixbuf_width = p_w;
		pdf_page_1.pixbuf_height = p_h;
		pdf_page_1.pixbuf_rotation = rotation;

		pdf_render_page_to_pixbuf(
				current_page, //int num_page
				pdf_page_1.pixbuf_width,pdf_page_1.pixbuf_height, //int width, int height
				scale, //double scale
				rotation); //int rotation);
	}
	gdk_window_invalidate_rect(root_window,NULL,FALSE); //prekresleni
}

 
void open_file(char *path){
	if (path == NULL){
		fprintf(stderr,"Nebylo zadáno jméno souboru.\n");
		exit(1);
	}
	//absoulutni adresa
	char *pwd = getenv("PWD");
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
		fprintf(stderr,"Chyba načtení: %s\n",err);
		exit(1);
	}
	
	if ((current_page < 0) || (current_page > pdf_num_pages))
		current_page = 0;
}

/////////////////////////////////////////////////////////////////
	
void change_page(int new){
	if (new <0 || new >= pdf_num_pages)
		return;
//	int old = current_page;
	pdf_page_init(new);
	current_page=new;
	render_page();
//	pdf_page_free(old);
}

void key_up(){ 		change_page(current_page-1);}
void key_down(){ 	change_page(current_page+1);}
void key_home(){ 	change_page(0);}
void key_end(){ 	change_page(pdf_num_pages-1);}
void key_jump(int num_page){ 	change_page(num_page);}
void key_jump_up(int diff){ 	change_page(current_page - diff);}
void key_jump_down(int diff){ 	change_page(current_page + diff);}

void key_quit(){
	gdk_event_put( gdk_event_new(GDK_DELETE));
}

void key_fullscreen(){
	printf("%d\n",is_fullscreen);
	if (is_fullscreen){
		gdk_window_unfullscreen(root_window);
		is_fullscreen = 0;
	} else {
		gdk_window_fullscreen(root_window);
		is_fullscreen = 1;
	}
}

void key_rotate(){ 
	pdf_page_1.rotation = (pdf_page_1.rotation + 90) % 360;
	render_page();
}

void key_rotate_document(){
	document_rotation = (document_rotation+90)%360; 
	render_page();
}

void key_reload(){
	open_file(file_name);
	pdf_page_init(current_page);
	document_rotation = 0;
	pdf_page_1.rotation = 0;
	pdf_page_1.pixbuf_height = 0;
	render_page();
}

void click_distance(int first_x, int first_y, int second_x, int second_y){
	printf("hui %f %f %d %d - %d %d\n",pdf_page_1.width,pdf_page_1.height,first_x,first_y,second_x,second_y);
}
void click_position(int x, int y){
	printf("hui %f %f %d %d\n",pdf_page_1.width,pdf_page_1.height,x,y);
}

///////////////////////////////////////////////////////////////////////////////////

static void event_func(GdkEvent *ev, gpointer data) {
	switch(ev->type) {
		case GDK_KEY_PRESS:
			handling_key(ev->key.keyval);
			break;
		case GDK_BUTTON_PRESS:
			if (ev->button.button == 1)
				handling_click((int)ev->button.x,(int)ev->button.x);
			break;
		case GDK_DELETE:
			g_main_loop_quit(mainloop);
			break;
		case GDK_CONFIGURE: //zmena pozici ci velikosti-zavola exspose
			render_page();		
			break;
		case GDK_EXPOSE:
			//printf("expose\n"); //doublebuffering!!!!!
			gdk_window_clear(root_window); //smaže starý obrázek
			gdk_pixbuf_render_to_drawable(
					pdf_page_1.pixbuf, //GdkPixbuf *pixbuf,
					root_window,//GdkDrawable *drawable,
					gdkGC, //GdkGC *gc,
					0,0, //vykreslit cely pixbuf
					pdf_page_1.shift_width,pdf_page_1.shift_height, // kreslit do leveho horniho rohu okna
					pdf_page_1.pixbuf_width,pdf_page_1.pixbuf_height, //rozmery
					GDK_RGB_DITHER_NONE, //fujvec nechci
					0,0);
			break;
	}
}

int main(int argc, char * argv[]) {

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
				printf("Nápověda qw: \n"
					" šipečky posunuji, okynko se da zvětšit\n"
					" -h 	-help 	nápověda\n"
					" -p 	-page 	počáteční stránka\n"
					" -i 	-info 	info dokumentu\n");
				return 0;
			case 'i':
				open_file(optarg);
				printf("Stran: %d\n",pdf_num_pages);
				pdf_page_init(current_page);
				printf("Strana: %d sirka: %f cm vyska: %f cm\n",current_page,pdf_page_1.width*0.035278,pdf_page_1.height*0.035278);
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

	file_name = argv[optind]; //vim ze tohle obecne nefunguje, ale tady to staci
	open_file(file_name);

	pdf_page_init(current_page);

	//vytvoreni okna
	gdk_init(NULL,NULL);

	GdkVisual *visual = gdk_visual_get_system();
	GdkColormap *colormap = gdk_colormap_new(visual,TRUE);

	GdkWindowAttr attr = {
		"huiii", //gchar *title;
		0x3FFFFE, //gint event_mask; all events mask
		100,0, //gint x, y;
		ceil(pdf_page_1.width),ceil(pdf_page_1.height),
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

	root_window = gdk_window_new(
			NULL, //GdkWindow *parent,
			&attr, //GdkWindowAttr *attributes,
			0 //gint attributes_mask ????
	);
	gdkGC  = gdk_gc_new(root_window);

		//toto tady provizorne
	render_page();

	gdk_window_show(root_window);
	gdk_event_handler_set(event_func, NULL, NULL);
	mainloop = g_main_loop_new(g_main_context_default(), FALSE);
	g_main_loop_run(mainloop);
	gdk_window_destroy(root_window); 

	return 0;
}
	

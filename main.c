#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> //parametry
#include <X11/Xlib.h> //okynka
#include <gdk/gdk.h>
#include <glib.h> 
#include "poppler.h"  //ovladani c++ knihovny

int current_page = 0;
extern int pdf_num_pages;
extern pdf_page pdf_page_1;
GMainLoop *mainloop;
GdkGC *gdkGC;

void change_page(GdkWindow *window, int new){
//	int old = current_page;
	pdf_page_init(new);
	current_page=new;
	render_page(window);
//	pdf_page_free(old);
}

void render_page(GdkWindow *window){
	gint w_w,w_h;
	gdk_drawable_get_size(window,&w_w,&w_h);
	int p_w,p_h;
	double scale;

	if (w_w*pdf_page_1.height < pdf_page_1.width*w_h){
		//šířka je stejná
		p_w = w_w;
		p_h = ceil(w_w*pdf_page_1.height/pdf_page_1.width);
		scale = w_w/pdf_page_1.width/w_w;
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

	if ( (pdf_page_1.width != p_w) || (pdf_page_1.height != p_h) ){
		pdf_page_1.pixbuf_width = p_w;
		pdf_page_1.pixbuf_height = p_h;

		pdf_render_page_to_pixbuf(
				current_page, //int num_page
				pdf_page_1.pixbuf_width,pdf_page_1.pixbuf_height, //int width, int height
				scale, //double scale
				0); //int rotation);
		gdk_window_invalidate_rect(window,NULL,FALSE); //prekresleni
	}
}

static void event_func(GdkEvent *ev, gpointer data) {
	switch(ev->type) {
		case GDK_KEY_PRESS:
			printf("key press [%s]-%d\n", ev->key.string,ev->key.keyval);
			switch(ev->key.keyval){
				case 65362: case 65365:
					if (current_page > 0)
						change_page(ev->any.window,current_page-1);
					break;
				case 65364: case 65366:
					if (current_page < pdf_num_pages-1)
						change_page(ev->any.window,current_page+1);
					break;
			}
			break;
		case GDK_DELETE:
			g_main_loop_quit(mainloop);
			break;
		case GDK_CONFIGURE: //zmena pozici ci velikosti-zavola exspose
			render_page(ev->any.window);		
			break;
		case GDK_EXPOSE:
			//printf("expose\n"); doublebuffering!!!!!
			gdk_window_clear(ev->any.window); //smaže starý obrázek
			gdk_pixbuf_render_to_drawable(
					pdf_page_1.pixbuf, //GdkPixbuf *pixbuf,
					ev->any.window,//GdkDrawable *drawable,
					gdkGC, //GdkGC *gc,
					0,0, //vykreslit cely pixbuf
					pdf_page_1.shift_width,pdf_page_1.shift_height, // kreslit do leveho horniho rohu okna
					pdf_page_1.pixbuf_width,pdf_page_1.pixbuf_height, //rozmery
					GDK_RGB_DITHER_NONE, //fujvec nechci
					0,0);
			break;
	}
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

	open_file(argv[optind]); //optind je prvni argument, kteri neni volba

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

	GdkWindow *root_window = gdk_window_new(
			NULL, //GdkWindow *parent,
			&attr, //GdkWindowAttr *attributes,
			0 //gint attributes_mask ????
	);
	gdkGC  = gdk_gc_new(root_window);

		//toto tady provizorne
	render_page(root_window);

	gdk_window_show(root_window);
	gdk_event_handler_set(event_func, NULL, NULL);
	mainloop = g_main_loop_new(g_main_context_default(), FALSE);
	g_main_loop_run(mainloop);
	gdk_window_destroy(root_window); 

	return 0;
}
	

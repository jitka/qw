#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h> //parametry
#include <X11/Xlib.h> //okynka
#include <gdk/gdk.h>
#include <cairo/cairo.h> //kreslici knihovna
#include <cairo/cairo-xlib.h> //kreslici knihovna
#include "poppler.h"  //ovladani c++ knihovny

extern int pdf_num_pages;
int start_page = 1;
extern double pdf_page_width;
extern double pdf_page_height;
gint window_width=840;
gint window_height=1200;

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

	start_page--; //lidi cisluji od 1
	if ((start_page < 1) || (start_page > pdf_num_pages))
		start_page = 0;
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
					" -h 	-help 	nápověda\n"
					" -p 	-page 	počáteční stránka\n"
					" -i 	-info 	info dokumentu\n");
				return 0;
			case 'i':
				open_file(optarg);
				printf("Stran: %d\n",pdf_num_pages);
				pdf_page_init(start_page);
				printf("Strana: %d sirka: %f cm vyska: %f cm\n",start_page,pdf_page_width*0.035278,pdf_page_height*0.035278);
				return 0;
			case 'p':
				start_page = atoi(optarg);	
				break;
			case '?': 
				fprintf(stderr,"Špatné volby. Po nápovědu pište -h nebo -help.\n");
				return 1;
			case -1 :
				break;
		}
	} while (next_option != -1);

	open_file(argv[optind]); //optind je prvni argument, kteri neni volba

	pdf_page_init(start_page);
	window_width = ceil(pdf_page_width);
	window_height = ceil(pdf_page_height);


	//vytvoreni okna
	gdk_init(NULL,NULL);

	GdkDisplay *display = gdk_display_open(NULL);
	GdkVisual *visual = gdk_visual_get_system();
	GdkColormap *colormap = gdk_colormap_new(visual,TRUE);

	GdkWindowAttr attr = {
		"huiii", //gchar *title;
		0x3FFFFE, //gint event_mask; all events mask
		0,0, //gint x, y;
		window_width, window_height,
	 	GDK_INPUT_OUTPUT, //GdkWindowClass wclass;
		visual, //GdkVisual *visual;
		colormap, //GdkColormap *colormap;
		GDK_WINDOW_ROOT, //GdkWindowType window_type;
		GDK_X_CURSOR, //GdkCursor *cursor;
		NULL, //gchar *wmclass_name;
		NULL, //gchar *wmclass_class;
		TRUE, //gboolean override_redirect; ???
		GDK_WINDOW_TYPE_HINT_NORMAL, //GdkWindowTypeHint type_hint;
	};

	GdkWindow *root_window =  gdk_window_new(
			NULL, //GdkWindow *parent,
			&attr, //GdkWindowAttr *attributes,
			0x3FFFFE //gint attributes_mask ????
	);

	gdk_window_show(root_window);


	//hlavni smycka
	int alive = 1;
	while (alive) {
/*		XNextEvent(display, &event);
		switch (event.type) {
			case Expose:
				cairo_set_source_rgb(cr, 1, 1, 1);
				cairo_paint(cr);
				pdf_render_page(cr, start_page);
				if (event.xexpose.count > 0)
					break;
//				XDrawLine(display, window, gc, 10, 20, 150, 80);
//				XFlush(display);
				break;

			case ClientMessage:
				if (event.xclient.data.l[0] == delete_window)
					alive = 0;
				break;
			case KeyPress:
				printf("klavesa\n");
		}
*/	alive = 0;
	}

	gdk_display_close(display);
	return 0;
}
	

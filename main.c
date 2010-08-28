#include <stdio.h>
#include <stdlib.h>
#include <string.h> //open_file
#include <sys/stat.h> //hlidani zmen v souboru
#include <unistd.h> //cwd
#include <getopt.h> //parametry
#include <gdk/gdk.h> //okynka
#include "poppler.h" //open_file
#include "render.h" //render,expose
#include "inputs.h" //vstup -> funkce

//global
enum {
	START,
	PAGE,
	PRESENTATION
} mode = START;
int is_fullscreen = FALSE;
//tady budou rezimy a veskere veci
//co se reloudem nemeni

//settings



//render veci co po reloudu zustavaji
//extern int pdf_num_pages;
extern struct document document;
extern struct document new_document;
extern int current_page;
extern pdf_page pdf_page_1;

//file
char * file_path;
time_t modification_time;

//window
GMainLoop *mainloop;
GdkWindow *root_window;
GdkGC *gdkGC;

 
/////////////////////////////////////////////////////////////////
	
void open_file(char *path){
	if (path == NULL){
		fprintf(stderr,"Nebylo zadáno jméno souboru.\n");
		exit(1);
	}
	//absoulutni adresa
	char pwd[1024];
	if (!getcwd(pwd, sizeof(pwd))) {
		printf("au getcwd \n");
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
		fprintf(stderr,"Chyba načtení: %s\n",err);
		exit(1);
	}				
	struct stat s;
	if (stat(file_path, &s) != -1){
		modification_time = s.st_mtime;
	}

	
	if ((current_page < 0) || (current_page > pdf_num_pages))
		current_page = 0;
}

void key_up(){ 		change_page(root_window,current_page-1);}
void key_down(){ 	change_page(root_window,current_page+1);}
void key_home(){ 	change_page(root_window,0);}
void key_end(){ 	change_page(root_window,pdf_num_pages-1);}
void key_jump(int num_page){ 	change_page(root_window,num_page);}
void key_jump_up(int diff){ 	change_page(root_window,current_page - diff);}
void key_jump_down(int diff){ 	change_page(root_window,current_page + diff);}

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
//	current_database.page[current_page].rotation = (pdf_page_1.rotation + 90) % 360;
	render_page(&document,root_window);
}

void key_rotate_document(){
	document.rotation = (document.rotation+90)%360; 
	render_page(&document,root_window);
}

void key_reload(){
// 	close_file(file_path); //je potreba?
	open_file(file_path);
	document_create_databse(&new_document);
	render_page(&new_document,root_window);
	document_replace_database(&document,&new_document);
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
			render_page(&document,root_window);		
			break;
		case GDK_EXPOSE:
			//rekne mi kolik jich je ve fronte-> zabijeni zbytecnych rendrovani
			switch (mode) {
				case START:
					//jeste by se tu dal vlozit nejaky hezky obrazek
					key_reload();
					mode = PAGE;
					break;
				case PAGE:
					expose(root_window,gdkGC);
					break;
			}
			break;
		case GDK_DELETE:
			g_main_loop_quit(mainloop);
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
				printf("Nápověda qw: \n"
					" šipečky posunuji, okynko se da zvětšit\n"
					" -h 	-help 	nápověda\n"
					" -p 	-page 	počáteční stránka\n"
					" -i 	-info 	info dokumentu\n");
				return 0;
			case 'i':
				open_file(optarg);
				printf("Stran: %d\n",pdf_num_pages);
//				printf("Strana: %d sirka: %f cm vyska: %f cm\n",current_page,pdf_page_1.width*0.035278,pdf_page_1.height*0.035278);
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
		"huiii", //gchar *title;
		0x3FFFFE, //gint event_mask; all events mask
		100,0, //gint x, y;
		545,692,
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

	gdk_window_show(root_window);
	gdk_event_handler_set(event_func, NULL, NULL);
	mainloop = g_main_loop_new(g_main_context_default(), FALSE);
	
	
	g_main_loop_run(mainloop);
	gdk_window_destroy(root_window); 

	return 0;
}

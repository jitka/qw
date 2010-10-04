#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include "window.h"
#include "backend.h"
#include "render.h"
#include "settings.h"
#define min(A,B) ( (A) < (B) ? (A) : (B) )

extern int current_page;
document_t *document;

document_t * document_create_databse(){
	document_t * doc = calloc(1, sizeof(document_t));
	doc->rotation = 0;
	doc->number_pages = doc_get_number_pages();
	doc->columns = 1;
	doc->rows = 1;
	doc->scale = UNKNOWN;
	doc->table_h = 0;
	doc->table_w = 0;
	doc->center_h = window_height/2;
	doc->center_w = window_width/2;
	pixbuf_create_database(&doc->cache);
	pixbuf_create_database(&doc->displayed);
	doc->pages = calloc(doc->number_pages, sizeof(page_t));

	if (current_page < 0) current_page = 0;
	if (current_page >= doc->number_pages) current_page = doc->number_pages -1;
	return doc;
}

void render_set_max_columns(document_t *doc){
	doc->max_columns = (window_width + margin) / (minimum_width + margin);
	doc->max_rows = (window_height + margin) / (minimum_height + margin);
}

void document_delete_database(document_t *old){
	if (old == NULL) return;
	pixbuf_delete_database(&old->cache);
	pixbuf_delete_database(&old->displayed);
	free(old->pages);
	free(old);
}


void render_get_size(document_t * doc, int number_page, double *width, double *height){
	//da spravne rezmery vcetne rotace
	int rotation = (doc->pages[number_page].rotation+doc->rotation) % 360;
	switch (doc->pages[number_page].state){
		case UNINITIALIZED:
			doc_page_get_size(
					number_page,
					&doc->pages[number_page].width,
					&doc->pages[number_page].height);
			doc->pages[number_page].state = INITIALIZED;
		case INITIALIZED: 
			if (rotation%180 == 0){
				*width = doc->pages[number_page].width;
				*height = doc->pages[number_page].height;
			} else {
				*width = doc->pages[number_page].height;
				*height = doc->pages[number_page].width;
			}
	}
	
}

void render_page(document_t * doc, int page_number, int space_width, int space_height, int space_shift_w, int space_shift_h){
	//vyrendruje stranku doprosted krabicky
	int black_w,black_h;
	double page_width,page_height;
	render_get_size(doc,page_number,&page_width,&page_height);
	
	pixbuf_item *tmp = calloc(1,sizeof(pixbuf_item));

	tmp->page_number = page_number;
	tmp->rotation = (doc->pages[page_number].rotation+doc->rotation) % 360;
	if (space_width*page_height < page_width*space_height){
		//šířka /width je stejná
		tmp->width = space_width;
		tmp->height = ceil(space_width*page_height/page_width);
		black_w = space_width;
		black_h = (space_height - tmp->height + 1)/2;
		tmp->shift_width = space_shift_w;
		tmp->shift_height = space_shift_h + black_h;
		tmp->scale = space_width/page_width;
	} else {
		//výška/height je stejná
		tmp->width = ceil(space_height*(page_width/page_height));
		tmp->height = space_height;
		black_w = (space_width-tmp->width+1)/2;
		black_h = space_height;
		tmp->shift_width = space_shift_w + black_w;
		tmp->shift_height = space_shift_h;
		tmp->scale = space_height/page_height;
	}

	pixbuf_render(&doc->cache,tmp);
	pixbuf_insert_into_database(&doc->displayed,tmp);

	if (mode == ZOOM)
		doc->scale = tmp->scale;

	//cisteni
	gdk_window_clear_area_e(
			window,
			space_shift_w,space_shift_h,
			black_w,black_h);
	gdk_window_clear_area_e(
			window,
			space_shift_w + space_width - black_w,
			space_shift_h + space_height - black_h,
			black_w,black_h);
}

void compute_space(document_t *doc,int *space_width, int *space_height){
	//spočítá velkosti prostoru na jednu stranu tak aby se vešlo vše do okna a poměr stran
	//vyhovoval medánu z poměrů všech stran
	int num_displayed = min(doc->columns * doc->rows, doc->number_pages-current_page);

	//median pomeru stran
	double aspects[num_displayed];
	for (int i=0; i<num_displayed; i++){
		double a,b;
		render_get_size(doc,current_page+i,&a,&b);
		aspects[i] = a/b;
	}
	int unsorted = TRUE;
	while (unsorted){ //tenhle bublesort by mel byt na prumernem pdf prumerne linearni
		unsorted = FALSE;
		for (int i=0; i<num_displayed-1; i++)
			if (aspects[i]>aspects[i+1]){
				double tmp = aspects[i];
				aspects[i] = aspects[i+1];
				aspects[i+1] = tmp;
				unsorted = TRUE;
			}
	}
	double aspect = aspects[num_displayed/2];

	//velikost okynka na vykresleni jedne stranky
	if ( (double) ((window_width-(doc->columns-1)*margin) //pouzitlna sirka
		/doc->columns) //na jeden ramecek
	   / (double) ((window_height-(doc->rows-1)*margin)
	   	/doc->rows)
	   > aspect){
		//vyska je stejna
		*space_height = ( window_height - (doc->rows-1)*margin ) / doc->rows;
		*space_width = floor(*space_height*aspect);
	} else {
		//sirka je stejna
		*space_width = ( window_width-(doc->columns-1)*margin ) / doc->columns;
		*space_height = floor(*space_width/aspect);
	}

}

void render_mode_page(document_t *doc){
	/* podle tabulky rozdelim
	 * na casti (space) a do tech vlozim stranky
	 */	
	int space_width,space_height;
	compute_space(doc,&space_width,&space_height);
	int ulc_shift_width = (window_width - space_width*doc->columns - margin*(doc->columns-1) +1) / 2;
	int ulc_shift_height = (window_height - space_height*doc->rows - margin*(doc->rows-1) +1) / 2;
	//vykresleni jednotlivych ramecku		
	for (int j=0; j < doc->rows; j++){
		for (int i=0; i < doc->columns; i++){
			if ((current_page+i+j*doc->columns < doc->number_pages) &&
					(current_page+i+j*doc->columns >= 0)){
				render_page(
						doc,//document_t * doc,
						current_page+i+j*doc->columns,//int page_number,	
						space_width,space_height,//int space_width, int space_height
						ulc_shift_width + i * (space_width + margin),
						ulc_shift_height + j * (space_height + margin));//int space_shift_w, int space_shift_h)
			} else {
				//mazat
				gdk_window_clear_area_e(
						window,
						ulc_shift_width + i * (space_width + margin),
						ulc_shift_height + j * (space_height + margin),
						space_width,space_height);
			}
		}
	}
	//mazani okraju + mezer/margin
	gdk_window_clear_area_e(window,0,0,window_width,ulc_shift_height);
	gdk_window_clear_area_e(window,0,window_height-ulc_shift_height,window_width,ulc_shift_height);
	gdk_window_clear_area_e(window,0,0,ulc_shift_width,window_height);
	gdk_window_clear_area_e(window,window_width-ulc_shift_width,0,ulc_shift_width,window_height);
	for (int i=0; i < doc->rows-1; i++){
			gdk_window_clear_area_e(
				window,
				0, ulc_shift_height + (i+1) * space_height + i * margin,
				window_width, margin);
			}
	for (int i=0; i < doc->columns-1; i++){
			gdk_window_clear_area_e(
				window,
				ulc_shift_width + (i+1) * space_width + i * margin, 0,
				margin,window_height);
			}

}

void render_mode_zoom(document_t *doc){
	if (doc->scale == UNKNOWN){
		render_page(doc, current_page, window_width, window_height, 0, 0);
		doc->center_w = window_width/2;
		doc->center_h = window_height/2;
	} else {
		gdk_window_clear(window);
		pixbuf_item *tmp = calloc(1,sizeof(pixbuf_item));
		tmp->page_number = current_page;
		double width,height;
		render_get_size(doc,current_page,&width,&height);
		tmp->width=(int)floor(width*doc->scale),
		tmp->height=(int)floor(height*doc->scale),
		tmp->shift_width = document->center_w;
		tmp->shift_height = document->center_h;
		tmp->scale = doc->scale;
		tmp->rotation = (doc->pages[current_page].rotation+doc->rotation) % 360;
		pixbuf_render(&doc->cache,tmp);
		pixbuf_insert_into_database(&doc->displayed,tmp);
	}
}

void render(document_t *doc){
	pixbuf_delete_displayed(&doc->cache,&doc->displayed);
	switch(mode){
		case START:
			break;
		case PAGE: case PRESENTATION:
			render_mode_page(doc);
			break;
		case ZOOM:
			render_mode_zoom(doc);
			break;
	}
	
}

void key_crop(){
	//oreze stranku na velikost zobrazene tabulky
  	if (is_fullscreen)
		return;
	int space_width,space_height;
	compute_space(document,&space_width,&space_height);
	gdk_window_resize(window,
			document->columns*space_width + (document->columns-1)*margin,
			document->rows*space_height + (document->rows-1)*margin);
}


void render_get_relative_position(
		int pointer_x, int pointer_y,
		int *page,
		int *relative_x, int *relative_y,
		int *space_height, int *space_width){

	gint  compare(gconstpointer a, gconstpointer b){
		const pixbuf_item *item = a;
		return !(item->shift_width < pointer_x &&
			item->shift_height < pointer_y &&
			item->width + item->shift_width > pointer_x &&
			item->height + item->shift_height > pointer_y);
		b = NULL;
	}
	GList *pointer = g_list_find_custom(document->displayed.glist,NULL,compare);
	if (pointer == NULL){
		*page = -1;
	} else {
		pixbuf_item *item = pointer->data;
		*page = item->page_number;
		*space_height = item->height;
		*space_width = item->width;
		*relative_x = pointer_x - item->shift_width;
		*relative_y = pointer_y - item->shift_height;
	}
}

void expose(){
	void render(gpointer data, gpointer user_data){
		pixbuf_item *item = data;
		user_data = NULL;
		gdk_pixbuf_render_to_drawable(
				item->pixbuf,
				window,//GdkDrawable *drawable,
				gdkGC, //GdkGC *gc,
				0,0, //vykreslit cely pixbuf
				item->shift_width,
				item->shift_height, //posunuti
				item->width, //rozmery
				item->height,
				GDK_RGB_DITHER_NONE, //fujvec nechci
				0,0);		
	}
	g_list_foreach(document->displayed.glist,render,NULL);
}

void change_scale(double scale){
	double width,height;
	render_get_size(document,current_page,&width,&height);
	if (
			(floor(width*scale) > minimum_width) &&
			(floor(height*scale) > minimum_height) &&
			(floor(width*scale) * floor(height*scale) < cache_size) ){
		document->center_w += (int)( floor(width*document->scale) - floor(width*scale))/2;
		document->center_h += (int)( floor(height*document->scale) - floor(height*scale))/2;
		document->scale=scale;
		render(document);
		expose();
	}
}

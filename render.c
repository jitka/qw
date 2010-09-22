#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include "window.h"
#include "poppler.h"
#include "render.h"
#include "settings.h"
#define min(A,B) ( (A) < (B) ? (A) : (B) )

extern int current_page;
document_t *document;

document_t * document_create_databse(){
	document_t * doc = calloc(1, sizeof(document_t));
	doc->rotation = 0;
	doc->number_pages = pdf_get_number_pages();
	doc->columns = 1;
	doc->rows = 1;
	doc->scale = UNKNOWN;
	doc->zoom_shift_h=0;
	doc->zoom_shift_w=0;
	pixbuf_create_database(&doc->pixbufs_cache);
	doc->pages = calloc(doc->number_pages, sizeof(struct pdf_page));
	doc->pixbufs_displayed = calloc(maximum_displayed, sizeof(pixbuf_item));
	doc->pixbufs_displayed_length = 0;

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
	pixbuf_delete_displayed(&old->pixbufs_cache,old->pixbufs_displayed,old->pixbufs_displayed_length);
	pixbuf_delete_database(&old->pixbufs_cache);
	free(old->pixbufs_displayed);
	free(old->pages);
	free(old);
}


void render_get_size(document_t * doc, int number_page, double *width, double *height){
	//da spravne rezmery vcetne rotace
	int rotation = (doc->pages[number_page].rotation+doc->rotation) % 360;
	switch (doc->pages[number_page].state){
		case DONT_INIT:
			pdf_page_get_size(
					number_page,
					&doc->pages[number_page].width,
					&doc->pages[number_page].height);
			doc->pages[number_page].state = INITED;
		case INITED: 
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
	
	pixbuf_item *tmp = &(doc->pixbufs_displayed[doc->pixbufs_displayed_length]);

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

	pixbuf_render(&doc->pixbufs_cache,tmp);
	doc->pixbufs_displayed_length++;

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
		doc->zoom_shift_w = doc->pixbufs_displayed[0].shift_width;
		doc->zoom_shift_h = doc->pixbufs_displayed[0].shift_height;
	} else {
		gdk_window_clear(window);
		pixbuf_item *tmp = &(doc->pixbufs_displayed[0]);
		tmp->page_number = current_page;
		double width,height;
		render_get_size(doc,current_page,&width,&height);
		tmp->width=(int)floor(width*doc->scale),
		tmp->height=(int)floor(height*doc->scale),
		tmp->shift_width = document->zoom_shift_w;
		tmp->shift_height = document->zoom_shift_h;
		tmp->scale = doc->scale;
		tmp->rotation = (doc->pages[current_page].rotation+doc->rotation) % 360;
		pixbuf_render(&doc->pixbufs_cache,tmp);
		doc->pixbufs_displayed_length = 1;
	}
}

void render(document_t *doc){
	pixbuf_delete_displayed(&doc->pixbufs_cache,doc->pixbufs_displayed,doc->pixbufs_displayed_length);
	doc->pixbufs_displayed_length = 0;
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
		int clicked_x, int clicked_y,
		int *page,
		int *relative_x, int *relative_y,
		int *space_height, int *space_width){
	//pouziva se pro měření
	*page = -1;
	for(int i=0; i<document->pixbufs_displayed_length; i++){
		pixbuf_item *item = &(document->pixbufs_displayed[i]);
		if (	
				item->shift_width < clicked_x &&
				item->shift_height < clicked_y &&
				item->width + item->shift_width > clicked_x &&
				item->height + item->shift_height > clicked_y
		   ){ // pokud se kliklo do strány v item
			*page = item->page_number;
			*space_height = item->height;
			*space_width = item->width;
			*relative_x = clicked_x - item->shift_width;
			*relative_y = clicked_y - item->shift_height;
			break;
		}
	}
}

void expose(){
	for(int i=0; i<document->pixbufs_displayed_length; i++){
		pixbuf_item *item = &(document->pixbufs_displayed[i]);
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
}

void change_scale(double scale){
	double width,height;
	render_get_size(document,current_page,&width,&height);
	if (
			(floor(width*scale) > minimum_width) &&
			(floor(height*scale) > minimum_height) &&
			(floor(width*scale) * floor(height*scale) < max_size_of_cache) ){
		document->zoom_shift_w += (int)( floor(width*document->scale) - floor(width*scale))/2;
		document->zoom_shift_h += (int)( floor(height*document->scale) - floor(height*scale))/2;
		document->scale=scale;
		render(document);
		expose();
	}
}

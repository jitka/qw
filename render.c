#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include "window.h"
#include "poppler.h"
#include "render.h"
#include "settings.h"
#define min(A,B) ( (A) < (B) ? (A) : (B) )
//doladit vetsi tabulky
extern int current_page;
document_t *document;

document_t * document_create_databse(){
	document_t * doc = calloc(1, sizeof(document_t));
	doc->rotation = 0;
	doc->number_pages = pdf_get_number_pages();
	doc->columns = 1;
	doc->rows = 1;
	pixbuf_create_database(&doc->pixbufs, doc->number_pages);
	doc->pages = calloc(doc->number_pages, sizeof(struct pdf_page));

	if (current_page < 0) current_page = 0;
	if (current_page >= doc->number_pages) current_page = doc->number_pages -1;
	return doc;
}

void document_delete_database(document_t *old){
	if (old == NULL) return;
	pixbuf_delete_database(&old->pixbufs);
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

void render_page(document_t * doc, int number_page, int space_width, int space_height, int space_shift_w, int space_shift_h){
	//vyrendruje stranku doprosted krabicky
	int pixbuf_width,pixbuf_height;
	int black_w,black_h;
	double scale;
	int rotation = (doc->pages[number_page].rotation+doc->rotation) % 360;
	double page_width,page_height;
	render_get_size(doc,number_page,&page_width,&page_height);

			if (space_width*page_height < page_width*space_height){
				//šířka /width je stejná
				pixbuf_width = space_width;
				pixbuf_height = ceil(space_width*page_height/page_width);
				black_w = space_width;
				black_h = (space_height-pixbuf_height+1)/2;
				doc->pages[number_page].shift_width = space_shift_w;
				doc->pages[number_page].shift_height = space_shift_h + black_h;
				scale = space_width/page_width;
			} else {
				//výška/height je stejná
				pixbuf_width = ceil(space_height*(page_width/page_height));
				pixbuf_height = space_height;
				black_w = (space_width-pixbuf_width+1)/2;
				black_h = space_height;
				doc->pages[number_page].shift_width = space_shift_w + black_w;
				doc->pages[number_page].shift_height = space_shift_h;
				scale = space_height/page_height;
			}

	pixbuf_render(&doc->pixbufs,number_page,pixbuf_width,pixbuf_height,scale,rotation);

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

void render(document_t *doc){
	/* zjistim velikost okna, podle tabulky/rezimu rozdelim
	 * na casti (space) a do tech vlozim stranky
	 */
	int num_displayed = min(doc->columns * doc->rows, doc->number_pages-current_page);
	//median pomeru stran
	double aspects[num_displayed];
	for (int i=0; i<num_displayed; i++){
		double a,b;
		render_get_size(doc,current_page+i,&a,&b);
		aspects[i] = a/b;
	}
	int unsorted = TRUE;
	while (unsorted){ //tenhle bublesort by mel byt na normalnim pdf linearni
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

	//velikost okynka na jednu stranku
	gint window_width,window_height;
	gdk_drawable_get_size(window,&window_width,&window_height);
	//printf("win%d %d\n",window_width,window_height);
	//printf("co,ro %d %d\n",doc->columns,doc->rows);
	int space_width,space_height;
	if ( (double) ((window_width-(doc->columns-1)*margin) //pouzitlna sirka
		/doc->columns) //na jeden ramecek
	   / (double) ((window_height-(doc->rows-1)*margin)
	   	/doc->rows)
	   > aspect){
		//vyska je stejna
		space_height = ( window_height - (doc->rows-1)*margin ) / doc->rows;
		space_width = floor(space_height*aspect);
	} else {
		//sirka je stejna
		space_width = ( window_width-(doc->columns-1)*margin ) / doc->columns;
		space_height = floor(space_width/aspect);
	}
	//printf("space %lf %d %d\n",aspect,space_width,space_height);
	int ulc_shift_width = (window_width - space_width*doc->columns - margin*(doc->columns-1) +1) / 2;
	int ulc_shift_height = (window_height - space_height*doc->rows - margin*(doc->rows-1) +1) / 2;
	//printf("ulc %d %d\n",ulc_shift_width,ulc_shift_height);
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
	//vykresleni jednotlivych ramecku		
	for (int j=0; j < doc->rows; j++){
		for (int i=0; i < doc->columns; i++){
			if ((current_page+i+j*doc->columns < doc->number_pages) &&
					(current_page+i+j*doc->columns >= 0)){
				render_page(
						doc,//document_t * doc,
						current_page+i+j*doc->columns,//int number_page,	
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
}

void expose(){
	for (int j=0; j < document->rows; j++)
		for (int i=0; i < document->columns; i++){
			if ((current_page+i+j*document->columns < document->number_pages) &&
					(current_page+i+j*document->columns >= 0))
				gdk_pixbuf_render_to_drawable(
						document->pixbufs.page[current_page+i+j*document->columns].pixbuf,
						window,//GdkDrawable *drawable,
						gdkGC, //GdkGC *gc,
						0,0, //vykreslit cely pixbuf
						document->pages[current_page+i+j*document->columns].shift_width,
						document->pages[current_page+i+j*document->columns].shift_height, //posunuti
						document->pixbufs.page[current_page+i+j*document->columns].width, //rozmery
						document->pixbufs.page[current_page+i+j*document->columns].height,
						GDK_RGB_DITHER_NONE, //fujvec nechci
						0,0);
		}
}

	

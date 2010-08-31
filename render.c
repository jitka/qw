#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include "window.h"
#include "poppler.h"
#include "render.h"

extern int current_page;
document_t document;
document_t new_document;

void document_create_databse(document_t * doc){
	doc->rotation = 0;
	doc->number_pages = pdf_get_number_pages();
	doc->columns = 3;
	doc->rows = 2;
	pixbuf_create_database(&doc->pixbufs, doc->number_pages);
	doc->pages = calloc(doc->number_pages, sizeof(struct pdf_page));

	if (current_page < 0) current_page = 0;
	if (current_page >= doc->number_pages) current_page = doc->number_pages -1;
}

void document_replace_database(document_t *old_db, struct document_t *new_db){
	pixbuf_delete_database(&old_db->pixbufs);
	free(old_db->pages);
	memcpy(old_db,new_db,sizeof(document_t));
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
			root_window,
			space_shift_w,space_shift_h,
			black_w,black_h);
	gdk_window_clear_area_e(
			root_window,
			space_shift_w + space_width - black_w,
			space_shift_h + space_height - black_h,
			black_w,black_h);
}

void render(document_t *doc){
	/* zjistim velikost okna, podle tabulky/rezimu rozdelim
	 * na casti (space) a do tech vlozim stranky
	 */
	int num_displayed = doc->columns * doc->rows;

	double aspects[num_displayed];
	for (int i=0; i<num_displayed; i++){
		double a,b;
		render_get_size(doc,current_page+i,&a,&b);
		aspects[i] = a/b;
	}
	int unsorted = TRUE;
	while (unsorted){
		unsorted = FALSE;
		for (int i=0; i<num_displayed-1; i++)
			if (aspects[i]>aspects[i+1]){
				double tmp = aspects[i];
				aspects[i] = aspects[i+1];
				aspects[i+1] = tmp;
				unsorted = TRUE;
			}
	}
	int aspect = aspects[num_displayed/2];

	gint window_width,window_height;
	gdk_drawable_get_size(root_window,&window_width,&window_height);
	int width,height;

	if ( window_width*doc->columns / (double) window_height*doc->rows < aspect){
		//vyska je stejna
		width = ceil(window_height/aspect);
		height = window_height;
	} else {
		//sirka je stejna
		width = window_width;
		height = ceil(window_width*aspect);
	}
	printf("%d %d\n",width,height);
	for (int j=0; j < doc->rows; j++){
		for (int i=0; i < doc->columns; i++){
			if ((current_page+i+j*doc->columns < doc->number_pages) &&
					(current_page+i+j*doc->columns >= 0))
				render_page(
						doc,//document_t * doc,
						current_page+i+j*doc->columns,//int number_page,	
						window_width/(doc->columns),window_height/doc->rows,//int space_width, int space_height
						i*window_width/doc->columns,j*window_height/doc->rows);//int space_shift_w, int space_shift_h)
		}
	}
}

void expose(){
//mazat!!!	
	for (int j=0; j < document.rows; j++)
		for (int i=0; i < document.columns; i++){
			if ((current_page+i+j*document.columns < document.number_pages) &&
					(current_page+i+j*document.columns >= 0))
				gdk_pixbuf_render_to_drawable(
						document.pixbufs.page[current_page+i+j*document.columns].pixbuf,
						root_window,//GdkDrawable *drawable,
						gdkGC, //GdkGC *gc,
						0,0, //vykreslit cely pixbuf
						document.pages[current_page+i+j*document.columns].shift_width,
						document.pages[current_page+i+j*document.columns].shift_height, //posunuti
						document.pixbufs.page[current_page+i+j*document.columns].width, //rozmery
						document.pixbufs.page[current_page+i+j*document.columns].height,
						GDK_RGB_DITHER_NONE, //fujvec nechci
						0,0);
		}
}

	

#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include "window.h"
#include "poppler.h"
#include "render.h"

extern int current_page;
document_t document;
document_t new_document;

void document_create_databse(struct document_t * doc){
	doc->rotation = 0;
	doc->number_pages = pdf_get_number_pages();
	doc->columns = 1;
	doc->rows = 1;
	pixbuf_create_database(&doc->pixbufs, doc->number_pages);
	doc->pages = calloc(doc->number_pages, sizeof(struct pdf_page));

	if (current_page < 0) current_page = 0;
	if (current_page >= doc->number_pages) current_page = doc->number_pages -1;
}

void document_replace_database(struct document_t *old_db, struct document_t *new_db){
	pixbuf_delete_database(&old_db->pixbufs);
	free(old_db->pages);
	memcpy(old_db,new_db,sizeof(struct document_t));
}


void render_page(struct document_t * doc, int number_page, int space_width, int space_height, int space_shift_w, int space_shift_h){
	gint w_w,w_h;
	gdk_drawable_get_size(root_window,&w_w,&w_h);
	int p_w,p_h;
	double scale;
	int rotation = (doc->pages[current_page].rotation+doc->rotation) % 360;
	double page_width,page_height;
	switch (doc->pages[current_page].state){
		case DONT_INIT:
			pdf_page_get_size(
					current_page,
					&doc->pages[current_page].width,
					&doc->pages[current_page].height); //to vse bude v rendrovani
			doc->pages[current_page].state = INITED;
		case INITED: 
			if (rotation%180 == 0){
				page_width = doc->pages[number_page].width;
				page_height = doc->pages[number_page].height;
			} else {
				page_width = doc->pages[number_page].height;
				page_height = doc->pages[number_page].width;
			}

			// zapamatovat si stare rozmery
			//if ((rotation)%180 == 0){
				if (w_w*page_height < page_width*w_h){
					//šířka je stejná
					p_w = w_w;
					p_h = ceil(w_w*page_height/page_width);
					scale = w_w/page_width;
					doc->pages[current_page].shift_width = 0;
					doc->pages[current_page].shift_height = (w_h-p_h+1)/2;
				} else {
					//výška je stejná
					p_w = ceil(w_h*(page_width/page_height));
					p_h = w_h;
					scale = w_h/page_height;
					doc->pages[current_page].shift_width = (w_w-p_w+1)/2;
					doc->pages[current_page].shift_height = 0;
				}
/*			} else {
				if (w_w*doc->pages[current_page].width < doc->pages[current_page].height*w_h){
					//šířka je stejná
					p_w = w_w;
					p_h = ceil(w_w*doc->pages[current_page].width/doc->pages[current_page].height);
					scale = w_w/doc->pages[current_page].height;
					doc->pages[current_page].shift_width = 0;
					doc->pages[current_page].shift_height = (w_h-p_h+1)/2;
				} else {
					//výška je stejná
					p_w = ceil(w_h*(doc->pages[current_page].height/doc->pages[current_page].width));
					p_h = w_h;
					scale = w_h/doc->pages[current_page].width;
					doc->pages[current_page].shift_width = (w_w-p_w+1)/2;
					doc->pages[current_page].shift_height = 0;
				}
			}
*/	}

	pixbuf_render(&doc->pixbufs,current_page,p_w,p_h,scale,rotation);

	int w,h;
	if (doc->pages[current_page].shift_width == 0){
		if (doc->pages[current_page].shift_height == 0){
			w = 0; h = 0;
		} else {
			w = w_w; h = doc->pages[current_page].shift_height;
		}
	} else {
		w = doc->pages[current_page].shift_width; h = w_h;
	}
	gdk_window_clear_area_e(
			root_window,
			0,0,
			w,h);
	gdk_window_clear_area_e(
			root_window,
			w_w-w,w_h-h,
			w,h);
}

void render(struct document_t *doc){
	gint w_w,w_h;
	gdk_drawable_get_size(root_window,&w_w,&w_h);
	render_page(
			doc,//struct document_t * doc,
		        current_page,//int number_page,	
			w_w,w_h,//int space_width, int space_height
			0,0);//int space_shift_w, int space_shift_h){
}

void change_page(int new){
	if (new <0 || new >= document.number_pages)
		return;
	int old = current_page;
	current_page=new;
	render(&document);
	pixbuf_free(&document.pixbufs,old);
}

void expose(){
	gdk_pixbuf_render_to_drawable(
			document.pixbufs.page[current_page].pixbuf,
			root_window,//GdkDrawable *drawable,
			gdkGC, //GdkGC *gc,
			0,0, //vykreslit cely pixbuf
			document.pages[current_page].shift_width,
			document.pages[current_page].shift_height, //posunuti
			document.pixbufs.page[current_page].width, //rozmery
			document.pixbufs.page[current_page].height,
			GDK_RGB_DITHER_NONE, //fujvec nechci
			0,0);
}
	

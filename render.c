#include <math.h>
#include "pixbuffer.h"
#include "poppler.h"
#include "render.h"

pdf_page pdf_page_1;
int current_page = 0;
int pdf_num_pages;

struct document document;
struct document new_document;

//pixbuf_database current_database;
//pixbuf_database new_database;

void document_create_databse(struct document * document){
	pixbuf_create_database(&document->pixbufs, pdf_num_pages);
	document->rotation = 0;
//	document_rotation = 0;
	pdf_page_1.rotation = 0; //fuj
	pdf_page_init(current_page); //to vse bude v rendrovani


}

void document_replace_database(struct document *old_db, struct document *new_db){
	
//	pixbuf_replace_database(&current_database,&new_database);
	pixbuf_replace_database(&old_db->pixbufs,&new_db->pixbufs);
}

void render_page(struct document * document,GdkWindow * root_window){
	gint w_w,w_h;
	gdk_drawable_get_size(root_window,&w_w,&w_h);
	int p_w,p_h;
	double scale;
	//int rotation = (database->page[current_page].rotation+document_rotation) % 360;
	int rotation = (pdf_page_1.rotation+document->rotation) % 360;
// zapamatovat si stare rozmery
//	pdf_page_init(current_page); //to vse bude v rendrovani
	if ((rotation)%180 == 0){
		if (w_w*pdf_page_1.height < pdf_page_1.width*w_h){
			//šířka je stejná
			p_w = w_w;
			p_h = ceil(w_w*pdf_page_1.height/pdf_page_1.width);
			scale = w_w/pdf_page_1.width;
			pdf_page_1.shift_width = 0;
			pdf_page_1.shift_height = (w_h-p_h+1)/2;
		} else {
			//výška je stejná
			p_w = ceil(w_h*(pdf_page_1.width/pdf_page_1.height));
			p_h = w_h;
			scale = w_h/pdf_page_1.height;
			pdf_page_1.shift_width = (w_w-p_w+1)/2;
			pdf_page_1.shift_height = 0;
		}
	} else {
		if (w_w*pdf_page_1.width < pdf_page_1.height*w_h){
			//šířka je stejná
			p_w = w_w;
			p_h = ceil(w_w*pdf_page_1.width/pdf_page_1.height);
			scale = w_w/pdf_page_1.height;
			pdf_page_1.shift_width = 0;
			pdf_page_1.shift_height = (w_h-p_h+1)/2;
		} else {
			//výška je stejná
			p_w = ceil(w_h*(pdf_page_1.height/pdf_page_1.width));
			p_h = w_h;
			scale = w_h/pdf_page_1.width;
			pdf_page_1.shift_width = (w_w-p_w+1)/2;
			pdf_page_1.shift_height = 0;
		}
	}

	pixbuf_render(&document->pixbufs,current_page,p_w,p_h,scale,rotation);

	int w,h;
	if (pdf_page_1.shift_width == 0){
		if (pdf_page_1.shift_height == 0){
			w = 0; h = 0;
		} else {
			w = w_w; h = pdf_page_1.shift_height;
		}
	} else {
		w = pdf_page_1.shift_width; h = w_h;
	}
	gdk_window_clear_area_e(
			root_window,
			0,0,
			w,h);
	gdk_window_clear_area_e(
			root_window,
			w_w-w,w_h-h,
			w,h);
gdk_window_invalidate_rect(root_window,NULL,FALSE); //prekresleni
}

void change_page(GdkWindow * root_window, int new){
	if (new <0 || new >= pdf_num_pages)
		return;
	int old = current_page;
	//!!
	pdf_page_1.rotation=0;
	pdf_page_init(new);
	current_page=new;
	render_page(&document,root_window);
	pixbuf_free(&document.pixbufs,old);
}

void expose(GdkWindow * root_window,GdkGC *gdkGC){
	gdk_pixbuf_render_to_drawable(
			document.pixbufs.page[current_page].pixbuf,
			root_window,//GdkDrawable *drawable,
			gdkGC, //GdkGC *gc,
			0,0, //vykreslit cely pixbuf
			pdf_page_1.shift_width,pdf_page_1.shift_height, //posunuti
			document.pixbufs.page[current_page].width, //rozmery
			document.pixbufs.page[current_page].height,
			GDK_RGB_DITHER_NONE, //fujvec nechci
			0,0);
}
	

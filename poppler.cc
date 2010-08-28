//#include <stdio.h>
//#include <glib-2.0/glib.h>
#include <poppler/glib/poppler-document.h>

extern "C" {
	#include "pixbuffer.h"
	#include "render.h"
	#include "poppler.h"
}

extern int pdf_num_pages;

PopplerDocument * doc;
PopplerPage * page;
GError * err;
char *pdf_err;

char * pdf_init( char* filePath) {

	err = (GError *) 0;
	doc = poppler_document_new_from_file(filePath,0,&err); //0->nema heslo
		//vrati-li NULL -> ma chybu
	if (err != 0)
		return err->message;
	pdf_num_pages = poppler_document_get_n_pages(doc);
//	pixbuf_create_database(&current_database, pdf_num_pages);

	return NULL;
}

void pdf_page_init(int n){
	page = poppler_document_get_page (doc, n);
	poppler_page_get_size(page, &pdf_page_1.width, &pdf_page_1.height);
}

void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf,int num_page, int width, int height, double scale, int rotation) {
	pdf_page_init(num_page);
	*pixbuf = gdk_pixbuf_new(
		GDK_COLORSPACE_RGB,
		FALSE,	//gboolean has_alpha,
		8, //bits_per_sample,
		width,height); //int width,height
	poppler_page_render_to_pixbuf(
		page,
		0,0,
		width,height, //int src_width, int src_height,
		scale, //scale hustota
		rotation, //ve stupních
		*pixbuf);

}

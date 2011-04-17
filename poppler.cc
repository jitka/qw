//#include <stdio.h>
//#include <glib-2.0/glib.h>
#include <poppler/glib/poppler-document.h>

extern "C" {
//	#include "pixbuffer.h"
	#include "render.h"
	#include "poppler.h"
	#include "settings.h"
}


static PopplerDocument * doc;

int pdf_init( char* filePath) {
	if (mode != START) g_object_unref(doc);

	doc = poppler_document_new_from_file(filePath,NULL,NULL); //0->nema heslo
	//vrati-li NULL -> ma chybu
	if (doc == 0){
		return 0;
	}
	return 1;
}

int pdf_get_number_pages(){
	return poppler_document_get_n_pages(doc);
}

void pdf_page_get_size(int n, double *width, double *height){
	PopplerPage *page = poppler_document_get_page (doc, n);
	poppler_page_get_size(page, width, height);
	g_object_unref(page);
}

void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf,int num_page, int width, int height, double scale, int rotation) {
	PopplerPage *page = poppler_document_get_page (doc, num_page);
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
	g_object_unref(page);
}

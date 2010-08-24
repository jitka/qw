/*
 * Tohle predevala c++ obekty na hezci funkce
 *
 */

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <poppler/glib/poppler-document.h>

extern "C" {
#include "poppler.h"
}

int pdf_num_pages;
pdf_page pdf_page_1;

PopplerDocument * doc;
PopplerPage * page;
GError * err;
char *pdf_err;

char * pdf_init( char* filePath) {
	g_type_init();

	err = (GError *) 0;
	doc = poppler_document_new_from_file(filePath,0,&err); //0->nema heslo
	if (err != 0)
		return err->message;
	pdf_num_pages = poppler_document_get_n_pages(doc);
	return NULL;
}

void pdf_page_init(int n){
	page = poppler_document_get_page (doc, n);
	poppler_page_get_size(page, &pdf_page_1.width, &pdf_page_1.height);
	pdf_page_1.pixbuf_width = 0;
	pdf_page_1.pixbuf_height = 0;
}

void pdf_render_page_to_pixbuf(int num_page, int width, int height, double scale, int rotation) {
	pdf_page_1.pixbuf_width = width;
	pdf_page_1.pixbuf_height = height;	
	pdf_page_1.pixbuf = gdk_pixbuf_new(
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
		pdf_page_1.pixbuf);

}

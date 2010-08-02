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

PopplerDocument * doc;
PopplerPage * page;
GError * err;
int pdf_num_pages;
char *pdf_err;
double pdf_page_width;
double pdf_page_height;

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
	poppler_page_get_size(page, &pdf_page_width, &pdf_page_height);
}

void pdf_render_page( cairo_t* cairo, int n) {
	poppler_page_render (page, cairo);

}

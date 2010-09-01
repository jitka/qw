#include "pixbuffer.h"

typedef struct pdf_page{
	double width;
	double height;
	int rotation;
	int shift_width;
	int shift_height;
	enum {DONT_INIT=0,INITED} state;
} pdf_page;

typedef struct document_t{
	int number_pages;
	int rotation;
	pixbuf_database pixbufs;
	pdf_page * pages;
	int columns;
	int rows;
} document_t;

document_t * document_create_databse();
void document_delete_database(document_t *old);
void render(struct document_t * document);
void expose();

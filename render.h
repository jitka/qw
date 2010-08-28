#include "pixbuffer.h"

typedef struct pdf_page{
	double width;
	double height;
	int rotation;
	int shift_width;
	int shift_height;
	enum {DONT_INIT=0,DONT_DISPLAYED,DISPLAYED} state;
} pdf_page;

typedef struct document_t{
	int number_pages;
	int rotation;
	pixbuf_database pixbufs;
	pdf_page * pages;
	int columns;
	int rows;
} document_t;

extern document_t document;
extern document_t new_document;

extern int pdf_num_pages;
//extern pdf_page pdf_page_1;

//extern int document_rotation;
extern int current_page;

void document_create_databse(struct document_t * document);
void document_replace_database(struct document_t *old_db, struct document_t *new_db);
void render_page(struct document_t * document,GdkWindow * root_window);
void change_page(GdkWindow * widnow, int new_page);
void expose(GdkWindow * root_window,GdkGC *gdkGC);

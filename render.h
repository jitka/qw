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

void document_create_databse(struct document_t * document);
void document_replace_database(struct document_t *old_db, struct document_t *new_db);
void render(struct document_t * document);
//void change_page(int new_page);
void expose();

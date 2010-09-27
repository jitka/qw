#ifndef _render_h_
#define _render_h_
#include "pixbuffer.h"
#include <glib.h> //glist

typedef struct page_t{
	double width;
	double height;
	int rotation;
	enum {UNINITIALIZED=0,INITIALIZED} state;
} page_t;

typedef struct document_t{
	int number_pages;
	int rotation;
	pixbuf_database cache;
	pixbuf_database displayed;
	page_t * pages;
	int columns;
	int rows;
	int max_columns;
	int max_rows;
	double scale;
	int zoom_shift_h;
	int zoom_shift_w;
} document_t;

document_t * document_create_databse();
void document_delete_database(document_t *old);
void render_set_max_columns(document_t *doc);
void render(struct document_t * document);
void expose();
void change_scale(double scale);
void render_get_relative_position(
		int clicked_x, int clicked_y,
		int *page,
		int *relative_x, int *relative_y,
		int *space_height, int *space_width);
#endif

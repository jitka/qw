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
	pixbuf_database cache;
	pixbuf_database displayed;
	page_t * pages;
	int rotation;
	int columns;
	int rows;
	int max_columns;
	int max_rows;
//	double scale; //docasne
	//zobrazeny kus
	int current_h; //kde zacina aktualni stranka (pokud je 0 radku)
	int center_h;
	int center_w;
	int space_h;
	int space_w;
	int table_h; //space_h +margin 
	int table_w;
} document_t;

document_t * document_create_databse();
void document_delete_database(document_t *old);
void render_set_max_columns(document_t *doc);
void compute_space_center(document_t *doc);
void render(document_t * doc);
void expose();
//void change_scale(double scale);
void render_get_relative_position(
		int clicked_x, int clicked_y,
		int *page,
		int *relative_x, int *relative_y,
		int *space_height, int *space_width);
#endif

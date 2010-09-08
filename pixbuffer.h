#ifndef _pixbuffer_h_
#define _pixbuffer_h_
#include <gdk/gdk.h> //pixbuffery
#include <glib.h> //GList

typedef struct pixbuf_item{
	GdkPixbuf *pixbuf;
	int page_number;
	int width;
	int height;
	int shift_width;
	int shift_height;
	double scale;
	int rotation;
} pixbuf_item;

typedef struct pixbuf_database{
	GList *cache;
	int cache_size; 
} pixbuf_database;


void pixbuf_create_database(pixbuf_database * database);
void pixbuf_delete_database(pixbuf_database * old);
void pixbuf_delete_displayed(pixbuf_database *cache, pixbuf_item *arr, int length);
void pixbuf_render(pixbuf_database *cache, pixbuf_item *db);

#endif

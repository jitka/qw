#ifndef _pixbuffer_h_
#define _pixbuffer_h_
#include <gdk/gdk.h> //pixbuffery

struct pixbuf_pixbuf{
	GdkPixbuf *pixbuf;
	int width;
	int height;
	int rotation;
	enum {DONT_EXIST=0,USED,CACHED} state;
};

typedef struct pixbuf_database{
	int number_pages;
	struct pixbuf_pixbuf * page; //pole, pro kazdou stronku jeden
	int * cached; // shrobazdiste predpocitanych
	int cached_length;
} pixbuf_database;


void pixbuf_create_database(pixbuf_database * database, int number_pages);
void pixbuf_replace_database(pixbuf_database * old_db, pixbuf_database * new_db);
void pixbuf_render(pixbuf_database * database, int page, int width, int height, double scale, int rotation);
void pixbuf_free(pixbuf_database * database, int number_page);

#endif

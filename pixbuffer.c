#include <stdlib.h> //calloc
#include "pixbuffer.h"
#include "poppler.h"

int max_cached = 30;

/*void really_free(pixbuf_item *s){
	g_object_unref(s->pixbuf);
}*/

void pixbuf_create_database(pixbuf_database * database, int number_pages){
	database->number_pages = number_pages;
	database->page = calloc(number_pages, sizeof(pixbuf_item));
	database->cached = calloc(max_cached, sizeof(int));
	database->cached_length = 0;
}

void pixbuf_delete_database(pixbuf_database * old_db){
//	for (int i=0; i < old_db->number_pages; i++)
//		really_free(&old_db->page[i]);
	free(old_db->page);
	free(old_db->cached);
}

void pixbuf_delete_displayed(pixbuf_item *arr, int length){
	//predat do cache, nulovat citatel
	for(int i=0; i<length; i++)
		g_object_unref(arr[i].pixbuf);
}

void pixbuf_render(pixbuf_database *cache,pixbuf_item *db){
//	kouknout do cache, vyndat/rendrovat
	pdf_render_page_to_pixbuf(
			&db->pixbuf,
			db->page_number,
			db->width, db->height,
			db->scale,
			db->rotation);
}


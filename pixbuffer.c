#include <stdlib.h> //free
#include <string.h> //memcpy
#include "pixbuffer.h"
#include "poppler.h"
#include "settings.h"

void pixbuf_create_database(pixbuf_database * database){
	database->glist = NULL;
	database->size = 0;
}

void pixbuf_free(gpointer data, gpointer size){
	pixbuf_item *item = data;
	*((int *) size) -= item->width * item->height;
	g_object_unref(item->pixbuf);
	free(item);
}
void pixbuf_delete_database(pixbuf_database *db){
	g_list_foreach(db->glist,pixbuf_free,&db->size);
	g_list_free(db->glist);
	db->glist = NULL;
}

void pixbuf_delete_displayed(pixbuf_database *cache, pixbuf_item *arr, int length){
	for(int i=0; i<length; i++){
		pixbuf_item *tmp = malloc(sizeof(pixbuf_item));
		memcpy(tmp,&arr[i],sizeof(pixbuf_item)); //kopiruje malou strukturu s ukazatelem na pixbuf
		cache->glist = g_list_append(cache->glist,tmp);
		cache->size += tmp->width * tmp->height;
	}
	while (cache->size > max_size_of_cache){
		//smaze první polozku
		GList *first = g_list_first(cache->glist);
		cache->glist = g_list_remove_link(cache->glist,first);
		pixbuf_free(first->data,&cache->size);
		g_list_free_1(first);
	}
}

int compare(gconstpointer a, gconstpointer b){
	const pixbuf_item *x = a;
	const pixbuf_item *y = b;
	if ( 		(x->page_number == y->page_number) &&
	 		(x->width == y->width) &&
	 		(x->height == y->height) &&
	 		(x->rotation == y->rotation)
	   )
		return 0;
	else 	return 1;

}
void pixbuf_render(pixbuf_database *cache,pixbuf_item *it){
//	kouknout do cache, vyndat/rendrovat
	GList *tmp = g_list_find_custom(cache->glist,it,compare);
	if (tmp != NULL){
		cache->glist = g_list_remove_link(cache->glist,tmp);
		memcpy(it,tmp->data,sizeof(pixbuf_item));
	} else {
		pdf_render_page_to_pixbuf(
			&it->pixbuf,
			it->page_number,
			it->width, it->height,
			it->scale,
			it->rotation);
	}
}


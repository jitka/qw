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
	//	pixbuf_delete_database(cache);
	//	cache=NULL;
	//	g list neni tak hezky jak se zda tohle cele fuj jen maze prvni prvek
		GList *fuj_fuj = g_list_first(cache->glist);
		pixbuf_item *first = fuj_fuj->data;
//		cache->cache_size -= first->width * first->height;
		pixbuf_free(first,&cache->size);
		cache->glist = g_list_remove_link(cache->glist,fuj_fuj);
		g_list_free_1(fuj_fuj);
	}
}

void pixbuf_render(pixbuf_database *cache,pixbuf_item *it){
//	kouknout do cache, vyndat/rendrovat
	pdf_render_page_to_pixbuf(
			&it->pixbuf,
			it->page_number,
			it->width, it->height,
			it->scale,
			it->rotation);
}


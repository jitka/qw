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
	//if (item->pixbuf != NULL)
		g_object_unref(item->pixbuf);
	free(item);
}
void pixbuf_delete_database(pixbuf_database *db){
	g_list_foreach(db->glist,pixbuf_free,&db->size);
	g_list_free(db->glist);
	db->glist = NULL;
}
void pixbuf_free_from_database(pixbuf_database *db,GList *old){
	db->glist = g_list_remove_link(db->glist,old);
	pixbuf_free(old->data,&db->size);
	g_list_free_1(old);	
}
void pixbuf_insert_to_database(pixbuf_database *db, pixbuf_item *item){	
	db->glist = g_list_append(db->glist,item);
	db->size += item->width * item->height;
}

void pixbuf_delete_displayed(pixbuf_database *cache, pixbuf_database *displayed){
	cache->glist = g_list_concat(cache->glist,displayed->glist);
	cache->size += displayed->size;
	pixbuf_create_database(displayed);
	while (cache->size > max_size_of_cache){
		GList *first = g_list_first(cache->glist);
		pixbuf_free_from_database(cache,first);
	}
}


void pixbuf_render(pixbuf_database *cache,pixbuf_item *it){
	int compare(gconstpointer a, gconstpointer b){
		const pixbuf_item *x = a;
		const pixbuf_item *y = b;
		return !((x->page_number == y->page_number) &&
			 (x->width == y->width) &&
			 (x->height == y->height) &&
			 (x->rotation == y->rotation) );
	}
	GList *tmp = g_list_find_custom(cache->glist,it,compare);
	if (tmp != NULL){ // pokud byla v cachy vynda se
		cache->glist = g_list_remove_link(cache->glist,tmp);
		memcpy(it,tmp->data,sizeof(pixbuf_item));
	} else { // jinak se vykresli
		pdf_render_page_to_pixbuf(
			&it->pixbuf,
			it->page_number,
			it->width, it->height,
			it->scale,
			it->rotation);
	}
}


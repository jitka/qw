#include <stdlib.h> //calloc
#include <string.h> //memcpy
//#include <gdk-pixbuf/gdk-pixbuf.h>
#include "pixbuffer.h"
#include "poppler.h"

void really_free(struct pixbuf_pixbuf s){
//	g_free(s.pixbuf);
//	gdk_pixbuf_finalize(s.pixbuf);
//	g_object_unref(s.pixbuf);
//	   gdk_pixbuf_unref                    (s.pixbuf);
	s.width = 0;
	s.height = 0;
}

void pixbuf_create_database(pixbuf_database * database, int number_pages){
	database->number_pages = number_pages;
	database->page = calloc(number_pages, sizeof(struct pixbuf_pixbuf));
	database->cached = calloc(number_pages, sizeof(int));
	database->cached_length = 0;
}

void pixbuf_replace_database(pixbuf_database * old_db, pixbuf_database * new_db){
	//kvuli reloudovani
	for (int i=0; i < old_db->number_pages; i++)
		really_free(old_db->page[i]);
	free(old_db->page);
	free(old_db->cached);
	memcpy(old_db,new_db,sizeof(pixbuf_database));
}

void pixbuf_render(pixbuf_database * database, int page, int width, int height, double scale, int rotation){
/*	je tam?
	nastav pouzito
	rendruj
	vloz 
*/	
	if ( (database->page[page].width != width) 
			|| (database->page[page].height != height) 
			|| (database->page[page].rotation != rotation)){
		database->page[page].width = width;
		database->page[page].height = height;
		database->page[page].rotation = rotation;

		pdf_render_page_to_pixbuf(
				&database->page[page].pixbuf,
				page, //int num_page
				database->page[page].width,database->page[page].height, //int width, int height
				scale, //double scale
				rotation); //int rotation);
	}
}

void pixbuf_free(pixbuf_database * database, int number_page){
	database->page[number_page].is_used = FALSE;
	really_free(database->page[number_page]);
	/*bit v databazi
	vloz do zasobniky
	je-li tam moc smaz duplicity
	nestaci-li maz odzadu
*/
}

#include <math.h> //ceil
#include <stdlib.h> //calloc
#include <string.h> //memcpy
#include <gdk/gdk.h> //rectange
#include "window.h"
#include "backend.h"
#include "render.h"
#include "settings.h"
#define min(A,B) ( (A) < (B) ? (A) : (B) )

extern int current_page;
extern int need_render;
document_t *document;

document_t * document_create_databse(){
	document_t * doc = calloc(1, sizeof(document_t));
	doc->number_pages = doc_get_number_pages();
	pixbuf_create_database(&doc->cache);
	pixbuf_create_database(&doc->displayed);
	doc->pages = calloc(doc->number_pages, sizeof(page_t));
	doc->rotation = 0;
	doc->columns = 1;
	doc->rows = 1;
	render_set_max_columns(doc);
	doc->table_h = 0;
	doc->table_w = 0;
	doc->space_h = 0;
	doc->space_w = 0;
	doc->top_h = 0;
	doc->center_w = window_width/2;

	if (current_page < 0) current_page = 0;
	if (current_page >= doc->number_pages) current_page = doc->number_pages -1;
	return doc;
}

void render_set_max_columns(document_t *doc){
	doc->max_columns = (window_width + margin) / (minimum_width + margin);
	doc->max_rows = (window_height + margin) / (minimum_height + margin);
}

void document_delete_database(document_t *old){
	if (old == NULL) return;
	pixbuf_delete_database(&old->cache);
	pixbuf_delete_database(&old->displayed);
	free(old->pages);
	free(old);
}


void render_get_size(document_t * doc, int number_page, double *width, double *height){
	//da spravne rezmery vcetne rotace
	int rotation = (doc->pages[number_page].rotation+doc->rotation) % 360;
	switch (doc->pages[number_page].state){
		case UNINITIALIZED:
			doc_page_get_size(
					number_page,
					&doc->pages[number_page].width,
					&doc->pages[number_page].height);
			doc->pages[number_page].state = INITIALIZED;
		case INITIALIZED: 
			if (rotation%180 == 0){
				*width = doc->pages[number_page].width;
				*height = doc->pages[number_page].height;
			} else {
				*width = doc->pages[number_page].height;
				*height = doc->pages[number_page].width;
			}
	}
	
}

void render_page(document_t * doc, int page_number, int space_shift_w, int space_shift_h){
	//vymysli presnou pozitci a detaily doprosted krabicky
	//todo: co takhle zachovavat pomery mezi velikostmi ruznych stran?
	pixbuf_item *tmp = calloc(1,sizeof(pixbuf_item));
	
	if ((page_number < doc->number_pages) && (page_number >= 0)){
		double page_width,page_height;
		render_get_size(doc,page_number,&page_width,&page_height);

		tmp->page_number = page_number;
		tmp->rotation = (doc->pages[page_number].rotation+doc->rotation) % 360;
		tmp->space_shift_w = space_shift_w;
		tmp->space_shift_h = space_shift_h;
		if (doc->space_w*page_height < page_width*doc->space_h){
			//šířka /width je stejná
			tmp->width = doc->space_w;
			tmp->height = ceil(doc->space_w*page_height/page_width);
			tmp->shift_w = 0;
			tmp->shift_h = (doc->space_h - tmp->height) / 2;
			tmp->scale = doc->space_w/page_width;
		} else {
			//výška/height je stejná
			tmp->width = ceil(doc->space_h*(page_width/page_height));
			tmp->height = doc->space_h;
			tmp->shift_w = (doc->space_w - tmp->width) / 2;
			tmp->shift_h = 0;
			tmp->scale = doc->space_h/page_height;
		}
	} else{
		tmp->page_number = -1;
		tmp->space_shift_w = space_shift_w;
		tmp->space_shift_h = space_shift_h;
		tmp->width = doc->space_w;
		tmp->height = doc->space_h;
	}
	pixbuf_insert_into_database(&doc->displayed,tmp);
}
double median_aspect(document_t *doc, int num_displayed){

	//median pomeru stran
	double aspects[num_displayed];
	for (int i=0; i<num_displayed; i++){
		double a,b;
		render_get_size(doc,current_page+i,&a,&b);
		aspects[i] = a/b;
	}
	int unsorted = TRUE;
	while (unsorted){ //tenhle bublesort by mel byt na prumernem pdf prumerne linearni
		unsorted = FALSE;
		for (int i=0; i<num_displayed-1; i++)
			if (aspects[i]>aspects[i+1]){
				double tmp = aspects[i];
				aspects[i] = aspects[i+1];
				aspects[i+1] = tmp;
				unsorted = TRUE;
			}
	}
	return aspects[num_displayed/2];
}
	

void compute_space_center(document_t *doc){
	//spočítá velkosti prostoru na jednu stranku tak aby se vešlo vše do okna a poměr stran
	//vyhovoval medánu z poměrů všech stran
	if (doc->rows > 0){
		int num_displayed = min(doc->columns * doc->rows, doc->number_pages-current_page);

		double aspect = median_aspect(doc,num_displayed);

		//velikost okynka na vykresleni jedne stranky
		if ( (double) ((window_width-(doc->columns-1)*margin) //pouzitlna sirka
					/doc->columns) //na jeden ramecek
				/ (double) ((window_height-(doc->rows-1)*margin)
					/doc->rows)
				> aspect){
			//vyska je stejna
			doc->space_h = ( window_height - (doc->rows-1)*margin ) / doc->rows;
			doc->space_w = floor(doc->space_h*aspect);
		} else {
			//sirka je stejna
			doc->space_w = ( window_width-(doc->columns-1)*margin ) / doc->columns;
			doc->space_h = floor(doc->space_w/aspect);
		}
	} else {
		//n je pocetet sloupcu
		//vezmu vysku z prvnich n pokud je to min nez vyska stranky
		//pridam dalsich n a prepocitam vysku
		double aspect;
		int i=0;
		doc->space_w = ( window_width-(doc->columns-1)*margin ) / doc->columns;
		do {
			i++;
			aspect = median_aspect(doc,min(doc->columns*i, doc->number_pages-current_page));
			doc->space_h = floor(doc->space_w/aspect);
		} while ((doc->space_h+margin)*i-margin < window_height); //celkova hlouba je mensi nez vyska stranky
		//printf("%d\n",doc->space_h);
	}
	doc->table_w = doc->space_w*doc->columns + margin*(doc->columns-1);
	doc->table_h = doc->space_h*doc->rows + margin*(doc->rows-1);
}

void render_displayed_pixbuf(document_t *doc){
	void render(gpointer data, gpointer user_data){
		pixbuf_item *item = data;
		pixbuf_render(&doc->cache,item);
		user_data = NULL;
	}
	g_list_foreach(doc->displayed.glist,render,NULL);
}

void render(document_t *doc){
	/* podle tabulky rozdelim
	 * na casti (space) a do tech vlozim stranky
	 */	
	pixbuf_delete_displayed(&doc->cache,&doc->displayed);
	
	if (doc->rows > 0){
		for (int j=0; j < doc->rows; j++)
			for (int i=0; i < doc->columns; i++)
				render_page(
						doc,//document_t * doc,
						current_page + j*doc->columns + i,//int page_number,	
						i * (doc->space_w + margin),
						j * (doc->space_h + margin));//int space_shift_w, int space_shift_h)
	} else {
		//vykleslit to co pujde videt
		int row=0;
		while (doc->top_h + (doc->space_h+margin)*row-margin > 0) //top_h je dole
			row--;
		while (doc->top_h + (doc->space_h+margin)*(row+1)-margin < 0) //neni videt je nahore
			row++;
		while (doc->top_h + (doc->space_h+margin)*row-margin < window_height){ //je videt
			for (int i=0; i<doc->columns; i++)
				render_page(
						doc,//document_t * doc,
						current_page + row*doc->columns + i,//int page_number,	
						i * (doc->space_w + margin),
						row * (doc->space_h + margin));//int space_shift_w, int space_shift_h)
			row++;
		}
	}
	//need_render = TRUE;	
	render_displayed_pixbuf(doc);
}


void key_crop(){
	//oreze stranku na velikost zobrazene tabulky
  	if (is_fullscreen)
		return;
	gdk_window_resize(window,
			document->columns*document->space_w + (document->columns-1)*margin,
			document->rows*document->space_h + (document->rows-1)*margin);
}


void render_get_relative_position(
		int pointer_x, int pointer_y,
		int *page,
		int *relative_x, int *relative_y,
		int *space_height, int *space_width){
	//pouziva se pri mereni
	//da souradnice kliknuti vzhledem k jedne strance
	
	gint  compare(gconstpointer a, gconstpointer b){
		const pixbuf_item *item = a;
		int shift_w = item->space_shift_w + item->shift_w;
		int shift_h = item->space_shift_h + item->shift_h;
		return !(shift_w < pointer_x && 
			shift_h < pointer_y &&
			item->width + shift_w > pointer_x &&
			item->height + shift_h > pointer_y);
		b = NULL;
	}
	//prevedu na relativni vuci ulc tabulky
	pointer_x -= document->center_w - document->table_w/2;
	pointer_y -= document->top_h;
	//najdu stranku do ktere se kliklo
	GList *pointer = g_list_find_custom(document->displayed.glist,NULL,compare);
	if (pointer == NULL){
		*page = -1;
	} else {
		//zjistim relativni souradnice
		pixbuf_item *item = pointer->data;
		*page = item->page_number;
		*space_height = item->height;
		*space_width = item->width;
		*relative_x = pointer_x - (item->space_shift_w + item->shift_w);
		*relative_y = pointer_y - (item->space_shift_h + item->shift_h);
	}
}

void expose(){
	void render(gpointer data, gpointer user_data){
		pixbuf_item *item = data;
		user_data = NULL;
		gdk_pixbuf_render_to_drawable(
				item->pixbuf,
				window,//GdkDrawable *drawable,
				gdkGC, //GdkGC *gc,
				0,0, //vykreslit cely pixbuf
				document->center_w - document->table_w/2 
					+ item->space_shift_w + item->shift_w,
				document->top_h 
					+ item->space_shift_h + item->shift_h,
				item->width, //rozmery
				item->height,
				GDK_RGB_DITHER_NONE, //fujvec nechci
				0,0);
	}
	GdkRectangle rec = {0,0,window_width,window_height};
	GdkRegion *  reg = gdk_region_rectangle(&rec);
	gdk_window_begin_paint_region(window,reg);
	g_list_foreach(document->displayed.glist,render,NULL);
	gdk_window_end_paint(window);
	gdk_region_destroy(reg);
}

void change_scale(double scale){
	document->space_h *= scale;
	document->space_w *= scale;
	document->table_h = document->rows * (document->space_h + margin) - margin;
	document->table_w = document->columns * (document->space_w + margin) - margin;
	render(document);
	expose();

}


void key_zoom_in(){ 	
	change_scale(zoom_speed);
}
void key_zoom_out(){
	if (document->space_h / zoom_speed > minimum_height &&
			document->space_w / zoom_speed > minimum_width)
		change_scale(1/zoom_speed);

}



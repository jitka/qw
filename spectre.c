#include <stdio.h>
#include <gdk/gdk.h>
#include <libspectre/spectre-document.h>

static SpectreDocument *doc = NULL;

char * ps_init(char *file_name){
	if (doc != NULL)
		spectre_document_free(doc);
	doc = spectre_document_new();
	spectre_document_load(doc,file_name);
//	printf("%d\n",spectre_document_status(doc));
//	printf("%s\n",spectre_document_get_creator(doc));
	if (spectre_document_status(doc) == SPECTRE_STATUS_SUCCESS)
		return NULL;
	return ("spectre load error");
}

int ps_get_number_pages(){
	return (int) spectre_document_get_n_pages(doc);
}


void ps_page_get_size(double *width, double *height){
	int w,h;
	spectre_document_get_page_size(doc,&w,&h);
	*width = (double) w;
	*height = (double) h;
}

void ps_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation){
	SpectrePage *page = spectre_document_get_page(doc, num_page);
	spectre_page_free(page);
}

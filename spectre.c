#include <stdio.h>
#include <gdk/gdk.h>
#include <libspectre/spectre-document.h>

static SpectreDocument *doc = NULL;

int ps_init(char *file_name){
	if (doc != NULL)
		spectre_document_free(doc);
	doc = spectre_document_new();
	spectre_document_load(doc,file_name);
//	printf("%d\n",spectre_document_status(doc));
//	printf("%s\n",spectre_document_get_creator(doc));
	if (spectre_document_status(doc) == SPECTRE_STATUS_SUCCESS)
		return 0;
	return 1;
}

int ps_get_number_pages(){
	return (int) spectre_document_get_n_pages(doc);
}


void ps_page_get_size(int num_page, double *width, double *height){
	SpectrePage *page = spectre_document_get_page(doc, num_page);
	int w,h;
	spectre_page_get_size(page,&w,&h);
	*width = (double) w;
	*height = (double) h;
	spectre_page_free(page);
}

/*void ps_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation){
	SpectrePage *page = spectre_document_get_page(doc, num_page);
	SpectreRenderContext *rc = spectre_render_context_new ();
	unsigned char * data;
	int row_length;
	spectre_page_render(page, rc, &data, &row_length);
	spectre_render_context_free (rc);
	spectre_page_free(page);


	cairo_surface_t *surface = cairo_image_surface_create_for_data(
			data,
			CAIRO_FORMAT_RGB24,
			200, 200, 
			row_length);
	GdkPixmap* pixmap = gdk_pixmap_new(NULL,
			200,200,
			4);
	cairo_t * cairo = gdk_cairo_create(pixmap);

	
}*/

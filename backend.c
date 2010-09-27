#include "backend.h"
#include "poppler.h"
#include "settings.h"

char * doc_init(char *fileName){
	if (file_type == PDF)
		return pdf_init(fileName);
	else
		return "skoncil svet";
}
void doc_page_get_size(int n, double *width, double *height){
	if (file_type == PDF)
		pdf_page_get_size(n,width,height);
}
int doc_get_number_pages(){
	if (file_type == PDF)
		return pdf_get_number_pages();
	else
		return 42;
}
void doc_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation){
	if (file_type == PDF)
		pdf_render_page_to_pixbuf(pixbuf,num_page,width,height,scale,rotation);
}

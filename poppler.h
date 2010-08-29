#include <gdk/gdk.h>
char * pdf_init(char *fileName);
void pdf_page_get_size(int n, double *width, double *height); //taky nepouzitav
int pdf_get_number_pages(); //používá jen! create_document_database()
void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

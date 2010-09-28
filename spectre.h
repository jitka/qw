//#include <gdk/gdk.h>
char * ps_init(char *fileName);
void ps_page_get_size(int num_page,double *width, double *height); //taky nepouzitav
int ps_get_number_pages(); //používá jen! create_document_database()
void ps_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

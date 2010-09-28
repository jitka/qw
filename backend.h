#include <gdk/gdk.h>
int doc_init(char *path);
void doc_close();
void doc_page_get_size(int n, double *width, double *height); //moc nepouzivat
int doc_get_number_pages(); //taky
void doc_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

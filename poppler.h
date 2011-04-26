#include <gdk/gdk.h>
int pdf_init(char *fileName);
void pdf_page_get_size(int n, double *width, double *height);
int pdf_get_number_pages();
void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

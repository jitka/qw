char * pdf_init(char *fileName);
void pdf_page_init(int n);
void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

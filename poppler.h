extern int pdf_num_pages;
extern double pdf_page_width;
extern double pdf_page_height;

char * pdf_init(char *fileName);
void pdf_page_init(int n);
void pdf_render_page(cairo_t *cairo, int n);

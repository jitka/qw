typedef struct pdf_page{
	double width;
	double height;
	int rotation;
	GdkPixbuf *pixbuf;
	int pixbuf_width;
	int pixbuf_height;
	int pixbuf_rotation;
	int shift_width;
	int shift_height;
} pdf_page;

extern int pdf_num_pages;
extern pdf_page pdf_page_1;

char * pdf_init(char *fileName);
void pdf_page_init(int n);
void pdf_render_page_to_pixbuf(int num_page, int width, int height, double scale, int rotation);


typedef struct pdf_page{
	double width;
	double height;
	GdkPixbuf *pixbuf;
} pdf_page;

extern int pdf_num_pages;
extern pdf_page pdf_page_1;

char * pdf_init(char *fileName);
void pdf_page_init(int n);
void pdf_render_page(int n);

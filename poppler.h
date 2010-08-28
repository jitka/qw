char * pdf_init(char *fileName);
int pdf_get_number_pages(); //používá jen! create_document_database()
void pdf_page_init(int n);
void pdf_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation);

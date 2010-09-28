#include <stdlib.h>
#include <unistd.h> //cwd
#include <string.h> //open_file
#include "backend.h"
#include "poppler.h"
#include "spectre.h"
#include "settings.h"

int doc_init(char *path){
	//pdf
	char pwd[1024];
	if (!getcwd(pwd, sizeof(pwd)))
		return 0;
	char abs_path[strlen("file://")+strlen(path)+1+strlen(pwd)+1];
	if (*path == '/')
		sprintf(abs_path,"file://%s",path);
	else
		sprintf(abs_path,"file://%s/%s",pwd,path);
	if (pdf_init(abs_path) == NULL){
		file_type = PDF;
		return 1;
	}
	//ps
	if (ps_init(path) == NULL){
		file_type = PS;
		return 1;
	}
	return 0;
}
void doc_page_get_size(int n, double *width, double *height){
	if (file_type == PDF)
		pdf_page_get_size(n,width,height);
	if (file_type == PS)
		ps_page_get_size(n,width,height);
}
int doc_get_number_pages(){
	if (file_type == PDF)
		return pdf_get_number_pages();
	if (file_type == PS)
		return ps_get_number_pages();
	return 42;
}
void doc_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation){
	if (file_type == PDF)
		pdf_render_page_to_pixbuf(pixbuf,num_page,width,height,scale,rotation);
//	if (file_type == PS)
//		ps_render_page_to_pixbuf(pixbuf,num_page,width,height,scale,rotation);
}

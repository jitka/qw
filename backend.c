#include <stdlib.h>
#include <unistd.h> //cwd
#include <string.h> //open_file
#include "backend.h"
#include "poppler.h"
#include "spectre.h"
#include "settings.h"

int doc_init(char *path){
	//pdf
//	close(2); open("/dev/null", O_RDWR);
	char pwd[1024];
	if (!getcwd(pwd, sizeof(pwd)))
		return 1;
	char abs_path[strlen("file://")+strlen(path)+1+strlen(pwd)+1];
	if (*path == '/')
		sprintf(abs_path,"file://%s",path);
	else
		sprintf(abs_path,"file://%s/%s",pwd,path);
	if (pdf_init(abs_path)){
		file_type = PDF;
		//printf("pdf\n");
		return 1;
	}
	//ps
//	printf("nepdf\n");
	if (!ps_init(path)){
		file_type = PS;
//		printf("ps\n");
		return 0; //protoze to zatim neumim
	}
	printf("nic\n");
//	return 0;

}
void doc_page_get_size(int n, double *width, double *height){
	if (file_type == PDF)
		pdf_page_get_size(n,width,height);
//	if (file_type == PS)
//		ps_page_get_size(n,width,height);
}
int doc_get_number_pages(){
	if (file_type == PDF)
		return pdf_get_number_pages();
//	if (file_type == PS)
//		return ps_get_number_pages();
	return 42;
}
void doc_render_page_to_pixbuf(GdkPixbuf **pixbuf, int num_page, int width, int height, double scale, int rotation){
	if (file_type == PDF)
		pdf_render_page_to_pixbuf(pixbuf,num_page,width,height,scale,rotation);
//	if (file_type == PS)
//		ps_render_page_to_pixbuf(pixbuf,num_page,width,height,scale,rotation);
}

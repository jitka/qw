#define UNKNOWN (-1)
typedef enum view_mode_t {
	START=1,
	PAGE=2,
	PRESENTATION=4,
} view_mode_t;
typedef enum file_type_t{
	PDF,
	PS,
} file_type_t;

extern view_mode_t mode;
extern file_type_t file_type;

extern int key_cancel_waiting;
extern int keep_scale;
extern int margin; //sirka mezery v pixelech
extern int start_window_w;
extern int start_window_h;
extern int minimum_width;
extern int minimum_height;
extern int cache_size;
extern double zoom_speed;
extern int zoom_shift;
extern int presentation_in_fullscreen;
extern int start_maximalized;
extern int start_fullscreen;
extern int start_columns;
extern int start_rows;

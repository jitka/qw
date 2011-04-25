#include <gdk/gdk.h>
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
extern int margin;
extern int start_window_w;
extern int start_window_h;
extern int minimum_width;
extern int minimum_height;
extern int cache_size;
extern double zoom_speed;
extern int zoom_shift;
extern int move_delta;
extern int presentation_in_fullscreen;
extern int start_maximalized;
extern int start_fullscreen;
extern int start_columns;
extern int start_rows;
extern char *start_comand;
extern int parallel_window;

typedef struct {
	unsigned int mode_mask;
	unsigned int pressed_char;
	void (*function)(int);
} number_chars_t;

typedef struct {
	unsigned int modifier;
	unsigned int pressed_char;
	void (*function)(void);
} command_chars_t;

extern command_chars_t command_chars[];
extern number_chars_t number_chars[];
extern int number_chars_count;
extern int command_chars_count;
	

#define UNKNOWN (-1)
typedef enum view_mode_t {
	START=1,
	PAGE=2,
	PRESENTATION=4,
	ZOOM=8,
} view_mode_t;
extern view_mode_t mode;

extern int key_cancel_waiting;
extern int keep_scale;
extern int margin; //sirka mezery v pixelech
extern int start_window_width;
extern int start_window_height;
extern int maximum_displayed;
extern int minimum_width;
extern int minimum_height;
extern int max_size_of_cache;
extern double zoom_speed;
extern int zoom_shift;
extern int presentation_in_fullscreen;
extern int start_maximalized;
extern int start_fullscreen;

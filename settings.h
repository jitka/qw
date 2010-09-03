typedef enum view_mode_t {
	START,
	PAGE,
	PRESENTATION,
	ZOOM,
} view_mode_t;
extern view_mode_t mode;

extern int key_cancel_waiting;
extern int margin; //sirka mezery v pixelech
extern int start_window_width;
extern int start_window_height;

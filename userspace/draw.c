#include <stdlib.h>

#include "lib/window.h"
#include "lib/graphics.h"
#include "lib/list.h"

/* XXX TOOLKIT FUNCTIONS */

gfx_context_t * ctx;

/* Active TTK window XXX */
static window_t * ttk_window = NULL;

/* TTK Window's objects XXX */
static list_t *   ttk_objects = NULL;

#define TTK_BUTTON_TYPE 0x00000001

/*
 * Core TTK GUI object
 */
typedef struct {
	uint32_t type; /* Object type indicator (for introspection) */
	int32_t  x;    /* Coordinates */
	int32_t  y;
	int32_t  width;  /* Sizes */
	int32_t  height;
	void (*render_func)(void *); /* (Internal) function to render the object */
	void (*click_callback)(void *, w_mouse_t *); /* Callback function for clicking */
} ttk_object;

/* TTK Button */
typedef struct {
	ttk_object _super;   /* Parent type (Object -> Button) */
	char * title;        /* Button text */
	uint32_t fill_color; /* Fill color */
} ttk_button;

void ttk_render_button(void * s) {
	ttk_object * self = (ttk_object *)s;
	/* Fill the button */
	for (uint16_t y = self->y + 1; y < self->y + self->height; y++) {
		draw_line(ctx, self->x, self->x + self->width, y, y, ((ttk_button *)self)->fill_color);
	}
	/* Then draw the border */
	uint32_t border_color = rgb(0,0,0);
	draw_line(ctx, self->x, self->x + self->width, self->y, self->y, border_color);
	draw_line(ctx, self->x, self->x,  self->y, self->y + self->height, border_color);
	draw_line(ctx, self->x + self->width, self->x + self->width, self->y, self->y + self->height, border_color);
	draw_line(ctx, self->x, self->x + self->width, self->y + self->height, self->y + self->height, border_color);
}

ttk_button * ttk_new_button(char * title, void (*callback)(void *, w_mouse_t *)) {
	ttk_button * out = malloc(sizeof(ttk_button));
	out->title = title;
	out->fill_color   = rgb(100,100,100);

	/* Standard */
	ttk_object * obj = (ttk_object *)out;
	obj->click_callback = callback;;
	obj->render_func = ttk_render_button;
	obj->type = TTK_BUTTON_TYPE;
	obj->x = 0;
	obj->y = 0;
	obj->width  = 20;
	obj->height = 20;

	list_insert(ttk_objects, obj);
	return out;
}

/*
 * Reposition a TTK object
 */
void ttk_position(ttk_object * obj, int x, int y, int width, int height) {
	obj->x = x;
	obj->y = y;
	obj->width = width;
	obj->height = height;
}

int ttk_within(ttk_object * obj, w_mouse_t * evt) {
	if (evt->new_x >= obj->x && evt->new_x < obj->x + obj->width &&
		evt->new_y >= obj->y && evt->new_y < obj->y + obj->height) {
		return 1;
	}
	return 0;
}

void ttk_check_click(w_mouse_t * evt) {
	if (evt->command == WE_MOUSECLICK) {
		foreach(node, ttk_objects) {
			ttk_object * obj = (ttk_object *)node->value;
			if (ttk_within(obj, evt)) {
				if (obj->click_callback) {
					obj->click_callback(obj, evt);
				}
			}
		}
	}
}

void ttk_render() {
	foreach(node, ttk_objects) {
		ttk_object * obj = (ttk_object *)node->value;
		if (obj->render_func) {
			obj->render_func(obj);
		}
	}
}

void setup_ttk(window_t * window) {
	ttk_window = window;
	ttk_objects = list_create();
	mouse_action_callback = ttk_check_click;
}

uint32_t drawing_color = 0;
uint16_t quit = 0;

static void set_color(void * button, w_mouse_t * event) {
	ttk_button * self = (ttk_button *)button;
	drawing_color = self->fill_color;
}

static void quit_app(void * button, w_mouse_t * event) {
	quit = 1;
	teardown_windowing();
	exit(0);
}

int main (int argc, char ** argv) {
	int left = 30;
	int top  = 30;

	int width  = 450;
	int height = 450;

	setup_windowing();

	/* Do something with a window */
	window_t * wina = window_create(left, top, width, height);
	ctx = init_graphics_window(wina);
	draw_fill(ctx, rgb(255,255,255));

	win_use_threaded_handler();

	setup_ttk(wina);
	ttk_button * button_blue = ttk_new_button("Blue", set_color);
	ttk_position((ttk_object *)button_blue, 3, 3, 100, 20);
	button_blue->fill_color = rgb(0,0,255);

	ttk_button * button_green = ttk_new_button("Grn", set_color);
	ttk_position((ttk_object *)button_green, 106, 3, 100, 20);
	button_green->fill_color = rgb(0,255,0);

	ttk_button * button_red = ttk_new_button("Red", set_color);
	ttk_position((ttk_object *)button_red, 209, 3, 100, 20);
	button_red->fill_color = rgb(255,0,0);

	ttk_button * button_quit = ttk_new_button("Quit", quit_app);
	ttk_position((ttk_object *)button_quit, width - 23, 3, 20, 20);

	drawing_color = rgb(255,0,0);

	ttk_render();

	while (!quit) {
		w_keyboard_t * kbd = poll_keyboard();
		if (kbd != NULL) {
			if (kbd->key == 'q') {
				break;
			}
			free(kbd);
		}
		w_mouse_t * mouse = poll_mouse();
		if (mouse != NULL) {
			if (mouse->command == WE_MOUSEMOVE && mouse->buttons & MOUSE_BUTTON_LEFT) {
				if (mouse->old_x >= 0 && mouse->new_x >= 0 && mouse->old_y >= 0 && mouse->new_y >= 0 &&
					mouse->old_x < width && mouse->new_x < width && mouse->old_y < width && mouse->new_y < width) {
					draw_line(ctx, mouse->old_x, mouse->new_x, mouse->old_y, mouse->new_y, drawing_color);
				}
			}
			free(mouse);
		}
		syscall_yield();
	}

	teardown_windowing();

	return 0;
}

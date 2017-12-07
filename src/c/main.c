#include <pebble.h>
#include "layout.h"
#include "logging.h"

static Window *s_window;
static Layout *s_layout;

static void prv_window_load(Window *window) {
    logf();
    s_layout = layout_create_with_resource(RESOURCE_ID_LAYOUT);
    layout_add_to_window(s_layout, window);
}

static void prv_window_unload(Window *window) {
    logf();
    layout_destroy(s_layout);
}

static void prv_init(void) {
    logf();
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });
    window_stack_push(s_window, true);
}

static void prv_deinit(void) {
    logf();
    window_destroy(s_window);
}

int main(void) {
    logf();
    prv_init();
     app_event_loop();
    prv_deinit();
}

#pragma once
#include <pebble.h>

typedef struct Layout Layout;

Layout *layout_create_with_resource(uint32_t resource_id);
void layout_destroy(Layout *this);
void layout_add_to_window(Layout *this, Window *window);

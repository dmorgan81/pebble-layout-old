#pragma once
#include <pebble.h>

typedef struct Layout Layout;

typedef void* (*TypeCreate)(GRect frame);
typedef void (*TypeDestroy)(void *this);
typedef Layer* (*TypeGetLayer)(void *this);

typedef struct TypeFuncs {
    TypeCreate create;
    TypeDestroy destroy;
    TypeGetLayer get_layer;
} TypeFuncs;

Layout *layout_create_with_resource(uint32_t resource_id);
void layout_destroy(Layout *this);
void layout_add_to_window(Layout *this, Window *window);
Layer *layout_find_by_id(Layout *this, char *id);
void layout_add_type(Layout* this, char *type, TypeFuncs type_funcs);

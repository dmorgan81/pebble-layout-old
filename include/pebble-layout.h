#pragma once
#include <pebble.h>
#include "json.h"

typedef struct Layout Layout;

typedef void* (*LayoutCreateFunc)(Layout *layout, Json *json, JsonToken *token);
typedef void (*LayoutDestroyFunc)(void *object);
typedef Layer* (*LayoutGetLayerFunc)(void *object);
typedef void (*LayoutSetFrameFunc)(void *object, GRect frame);

typedef struct {
    LayoutCreateFunc create;
    LayoutDestroyFunc destroy;
    LayoutGetLayerFunc get_layer;
    LayoutSetFrameFunc set_frame;
} LayoutFuncs;

typedef enum {
    StandardTypeText = 1,
    StandardTypeBitmap,
    StandardTypeEnd
} StandardType;

Layout *layout_create(void);
void layout_add_all_standard_types(Layout *this);
void layout_add_standard_type(Layout *this, StandardType type);
void layout_parse(Layout *this, uint32_t resource_id);
void layout_destroy(Layout *this);
Layer *layout_get_root_layer(Layout *this);
void *layout_find_by_id(Layout *this, char *id);
void layout_add_type(Layout *this, char *type, LayoutFuncs layout_funcs);
void layout_add_system_fonts(Layout *this);
void layout_add_font(Layout *this, char *name, uint32_t resource_id);
GFont layout_get_font(Layout* this, char *name);
void layout_add_resource_id(Layout *this, char *name, uint32_t resource_id);
uint32_t *layout_get_resource_id(Layout *this, char *name);

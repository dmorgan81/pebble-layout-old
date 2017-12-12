#pragma once
#include <pebble.h>
#include "jsmn.h"
#include "json.h"

typedef struct Layout Layout;

typedef void* (*LayoutCreateFunc)(Layout *layout, Json *json, jsmntok_t *token);
typedef void (*LayoutDestroyFunc)(void *object);
typedef Layer* (*LayoutGetLayerFunc)(void *object);
typedef void (*LayoutSetFrameFunc)(void *object, GRect frame);

typedef struct {
    LayoutCreateFunc create;
    LayoutDestroyFunc destroy;
    LayoutGetLayerFunc get_layer;
    LayoutSetFrameFunc set_frame;
} LayoutFuncs;

Layout *layout_create(void);
void layout_parse(Layout *this, uint32_t resource_id);
void layout_destroy(Layout *this);
Layer *layout_get_root_layer(Layout *this);
Layer *layout_find_by_id(Layout *this, char *id);
void layout_add_type(Layout *this, char *type, LayoutFuncs layout_funcs);

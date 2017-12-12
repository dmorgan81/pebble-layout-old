#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "stack.h"
#include "dict.h"
#include "jsmn.h"
#include "json.h"
#include "logging.h"
#include "layout.h"

struct Layout {
    Layer *root;
    Stack *layers;
    Dict *ids;
};

struct LayerData {
    GColor color;
};

static void prv_update_proc(Layer *layer, GContext *ctx) {
    logf();
    struct LayerData *data = layer_get_data(layer);
    if (!gcolor_equal(data->color, GColorClear)) {
        graphics_context_set_fill_color(ctx, data->color);
        graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
    }
}

static inline GRect json_grect(Json *json, jsmntok_t *tok) {
    logf();
    if (tok->type != JSMN_ARRAY) return GRectZero;

    int16_t values[4];
    for (uint i = 0; i < ARRAY_LENGTH(values); i++) {
        values[i] = json_next_int(json);
    }
    return GRect(values[0], values[1], values[2], values[3]);
}

static Layer *json_layer(Layout *layout, Json *json) {
    logf();
    jsmntok_t *tok = json_next(json);
    if (tok->type != JSMN_OBJECT) return NULL;

    Layer *layer = layer_create_with_data(GRectZero, sizeof(struct LayerData));
    struct LayerData *data = layer_get_data(layer);
    data->color = GColorClear;
    layer_set_update_proc(layer, prv_update_proc);
    stack_push(layout->layers, layer);

    int size = tok->size;
    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "frame")) {
            tok = json_next(json);
            GRect frame = json_grect(json, tok);
            layer_set_frame(layer, frame);
        } else if (json_eq(json, tok, "background")) {
            char *s = json_next_string(json);
            data->color = GColorFromHEX(strtoul(s + (s[0] == '#' ? 1 : 0), NULL, 16));
            free(s);
        } else if (json_eq(json, tok, "layers")) {
            tok = json_next(json);
            int nsize = tok->size;
            for (int j = 0; j < nsize; j++) {
                Layer *child = json_layer(layout, json);
                if (child != NULL) layer_add_child(layer, child);
            }
        } else if (json_eq(json, tok, "clips")) {
            layer_set_clips(layer, json_next_bool(json));
        } else if (json_eq(json, tok, "id")) {
            char *id = json_next_string(json);
            dict_put(layout->ids, id, layer);
        }
    }

    return layer;
}

Layout *layout_create_with_resource(uint32_t resource_id) {
    logf();
    Layout *this = malloc(sizeof(Layout));
    this->layers = stack_create();
    this->ids = dict_create();

    Json *json = json_create_with_resource(resource_id);
    if (!json_has_next(json)) goto cleanup;

    json_mark(json);
    jsmntok_t *token = json_next(json);
    if (token->type != JSMN_OBJECT) goto cleanup;
    json_reset(json);

    this->root = json_layer(this, json);
    GRect frame = layer_get_frame(this->root);
    if (grect_equal(&frame, &GRectZero)) {
        layer_set_frame(this->root, GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    }

cleanup:
    json_destroy(json);

    return this;
}

static bool prv_ids_destroy_callback(char *key, void *value, void *context) {
    logf();
    free(key);
    key = NULL;
    return true;
}

void layout_destroy(Layout *this) {
    logf();
    dict_foreach(this->ids, prv_ids_destroy_callback, NULL);
    dict_destroy(this->ids);
    this->ids = NULL;

    Layer *layer = NULL;
    while ((layer = stack_pop(this->layers)) != NULL) {
        layer_destroy(layer);
    }
    stack_destroy(this->layers);
    this->layers = NULL;
    this->root = NULL;

    free(this);
}

void layout_add_to_layer(Layout *this, Layer *layer) {
    logf();
    layer_add_child(layer, this->root);
}

Layer *layout_find_by_id(Layout *this, char *id) {
    logf();
    return (Layer *) dict_get(this->ids, id);
}

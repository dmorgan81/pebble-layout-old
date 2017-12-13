#include <pebble.h>
#include "stack.h"
#include "dict.h"
#include "jsmn.h"
#include "json.h"
#include "logging.h"
#include "pebble-layout.h"

struct Layout {
    Layer *root;
    Stack *layers;
    Dict *ids;
    Dict *types;
};

struct LayerData {
    LayoutFuncs *layout_funcs;
    void *object;
};

struct DefaultLayerData {
    GColor color;
};

static void prv_update_proc(Layer *layer, GContext *ctx) {
    logf();
    struct DefaultLayerData *data = layer_get_data(layer);
    if (!gcolor_equal(data->color, GColorClear)) {
        graphics_context_set_fill_color(ctx, data->color);
        graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
    }
}

static Layer *json_create_layer(Layout *layout, Json *json);

static void *prv_default_create(Layout *layout, Json *json, jsmntok_t *tok) {
    logf();
    Layer *layer = layer_create_with_data(GRectZero, sizeof(struct DefaultLayerData));
    struct DefaultLayerData *data = layer_get_data(layer);
    data->color = GColorClear;
    layer_set_update_proc(layer, prv_update_proc);

    int size = tok->size;
    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "background")) {
            data->color = json_next_gcolor(json);
        } else if (json_eq(json, tok, "layers")) {
            tok = json_next(json);
            int nsize = tok->size;
            for (int j = 0; j < nsize; j++) {
                Layer *child = json_create_layer(layout, json);
                if (child) layer_add_child(layer, child);
            }
        } else if (json_eq(json, tok, "clips")) {
            layer_set_clips(layer, json_next_bool(json));
        } else {
            json_skip_tree(json);
        }
    }

    return layer;
}

static void prv_default_destroy(void *object) {
    layer_destroy((Layer *) object);
}

static Layer *prv_default_get_layer(void *object) {
    return (Layer *) object;
}

static void prv_default_set_frame(void *object, GRect frame) {
    layer_set_frame((Layer *) object, frame);
}

static Layer *json_create_layer(Layout *layout, Json *json) {
    logf();
    jsmntok_t *tok = json_next(json);
    if (tok->type != JSMN_OBJECT) return NULL;

    jsmntok_t *orig = tok;
    LayoutFuncs *layout_funcs = NULL;
    int size = tok->size;
    int16_t index = json_get_index(json);
    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "type")) {
            char *s = json_next_string(json);
            layout_funcs = dict_get(layout->types, s);
            free(s);
            break;
        } else {
            json_skip_tree(json);
        }
    }
    json_set_index(json, index);

    if (layout_funcs == NULL) layout_funcs = dict_get(layout->types, "default");
    index = json_get_index(json);
    struct LayerData *data = malloc(sizeof(struct LayerData));
    data->layout_funcs = layout_funcs;
    data->object = layout_funcs->create(layout, json, orig);
    stack_push(layout->layers, data);
    json_set_index(json, index);

    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "id")) {
            char *id = json_next_string(json);
            dict_put(layout->ids, id, data->object);
        } else if (json_eq(json, tok, "frame")) {
            GRect frame = json_next_grect(json);
            layout_funcs->set_frame(data->object, frame);
        } else {
            json_skip_tree(json);
        }
    }

    return layout_funcs->get_layer(data->object);
}

Layout *layout_create(void) {
    Layout *this = malloc(sizeof(Layout));
    this->root = NULL;
    this->layers = stack_create();
    this->ids = dict_create();
    this->types = dict_create();

    layout_add_type(this, "default", (LayoutFuncs) {
        .create = prv_default_create,
        .destroy = prv_default_destroy,
        .get_layer = prv_default_get_layer,
        .set_frame = prv_default_set_frame
    });

    return this;
}

void layout_parse(Layout *this, uint32_t resource_id) {
    logf();
    Json *json = json_create_with_resource(resource_id);
    if (!json_has_next(json)) goto cleanup;

    int16_t index = json_get_index(json);
    jsmntok_t *token = json_next(json);
    if (token->type != JSMN_OBJECT) goto cleanup;
    json_set_index(json, index);

    this->root = json_create_layer(this, json);
    GRect frame = layer_get_frame(this->root);
    if (grect_equal(&frame, &GRectZero)) {
        layer_set_frame(this->root, GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    }

cleanup:
    json_destroy(json);
}

static bool prv_types_destroy_callback(char *key, void *value, void *context) {
    logf();
    free(value);
    value = NULL;
    return true;
}

static bool prv_ids_destroy_callback(char *key, void *value, void *context) {
    logf();
    free(key);
    key = NULL;
    return true;
}

void layout_destroy(Layout *this) {
    logf();
    dict_foreach(this->types, prv_types_destroy_callback, NULL);
    dict_destroy(this->types);
    this->types = NULL;

    dict_foreach(this->ids, prv_ids_destroy_callback, NULL);
    dict_destroy(this->ids);
    this->ids = NULL;

    struct LayerData *data = NULL;
    while ((data = stack_pop(this->layers)) != NULL) {
        data->layout_funcs->destroy(data->object);
        free(data);
    }
    stack_destroy(this->layers);
    this->layers = NULL;
    this->root = NULL;

    free(this);
}

Layer *layout_get_root_layer(Layout *this) {
    logf();
    return this->root;
}

Layer *layout_find_by_id(Layout *this, char *id) {
    logf();
    return (Layer *) dict_get(this->ids, id);
}

void layout_add_type(Layout *this, char *type, LayoutFuncs layout_funcs) {
    logf();
    LayoutFuncs *copy = malloc(sizeof(LayoutFuncs));
    memcpy(copy, &layout_funcs, sizeof(LayoutFuncs));
    dict_put(this->types, type, copy);
}

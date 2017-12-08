#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "stack.h"
#include "dict.h"
#include "jsmn.h"
#include "string.h"
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
    graphics_context_set_fill_color(ctx, data->color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

#ifdef DEBUG
static void json_log(const char *json, jsmntok_t *token) {
    logf();
    if (token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE) {
        char *s = strndup(json + token->start, token->end - token->start);
        if (token->type == JSMN_STRING) logd("\"%s\"", s); else logd("%s", s);
        free(s);
    } else if (token->type == JSMN_OBJECT) {
        logd("{ %d }", token->size);
    } else if (token->type == JSMN_ARRAY) {
        logd("[ %d", token->size);
        int size = token->size;
        for (int i = 0; i < size; i++) {
            token += 1;
            json_log(json, token);
        }
        logd("]");
    } else {
        logd("unknown");
    }
}
#else
#define json_log(json, token)
#endif

static inline bool json_eq(const char *json, jsmntok_t *token, const char *s) {
    logf();
    return (token->type == JSMN_STRING || token->type == JSMN_PRIMITIVE) && \
           (int) strlen(s) == token->end - token->start && \
           strncmp(json + token->start, s, token->end - token->start) == 0;
    
}

static inline GColor json_gcolor(const char *json, jsmntok_t *token) {
    logf();
    char *s = strndup(json + token->start, token->end - token->start);
    GColor color = GColorFromHEX(strtoul(s + (s[0] == '#' ? 1 : 0), NULL, 16));
    free(s);
    return color;
}

static inline int json_int(const char *json, jsmntok_t *token) {
    logf();
    char *s = strndup(json + token->start, token->end - token->start);
    int i = atoi(s);
    free(s);
    return i;
}

static inline GRect json_grect(const char *json, jsmntok_t **token) {
    logf();
    int16_t values[4];
    for (uint i = 0; i < ARRAY_LENGTH(values); i++) {
        *token += 1;
        values[i] = json_int(json, *token);
    }
    return GRect(values[0], values[1], values[2], values[3]);
}

static Layer *json_layer(const char *json, jsmntok_t **token, Layout *layout) {
    logf();
    Layer *layer = layer_create_with_data(GRectZero, sizeof(struct LayerData));
    struct LayerData *data = layer_get_data(layer);
    layer_set_update_proc(layer, prv_update_proc);
    stack_push(layout->layers, layer);

    int size = (*token)->size;
    for (int i = 0; i < size; i++) {
        *token += 1;
        if (json_eq(json, *token, "frame")) {
            *token += 1;
            GRect frame = json_grect(json, token);
            layer_set_frame(layer, frame);
        } else if (json_eq(json, *token, "background")) {
            *token += 1;
            data->color = json_gcolor(json, *token);            
        } else if (json_eq(json, *token, "layers")) {
            *token += 1;
            int nsize = (*token)->size;
            for (int j = 0; j < nsize; j++) {
                *token += 1;
                layer_add_child(layer, json_layer(json, token, layout));
            }
        } else if (json_eq(json, *token, "clips")) {
            *token += 1;
            layer_set_clips(layer, !json_eq(json, *token, "false"));
        } else if (json_eq(json, *token, "id")) {
            *token += 1;
            char *id = strndup(json + (*token)->start, (*token)->end - (*token)->start);
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

    ResHandle res_handle = resource_get_handle(resource_id);
    size_t res_size = resource_size(res_handle);
    char *json = malloc(sizeof(char) * res_size);
    resource_load(res_handle, (uint8_t *) json, res_size);

    jsmn_parser parser;
    jsmn_init(&parser);

    int count = jsmn_parse(&parser, json, strlen(json), NULL, 0);
    jsmn_init(&parser);
    jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * count);
    count = jsmn_parse(&parser, json, strlen(json), tokens, count);

    jsmntok_t *token = tokens;
    if (token->type != JSMN_OBJECT) goto cleanup;

    this->root = json_layer(json, &token, this);
    GRect frame = layer_get_frame(this->root);
    if (grect_equal(&frame, &GRectZero)) {
        layer_set_frame(this->root, GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    }

cleanup:
    free(tokens);
    free(json);

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

void layout_add_to_window(Layout *this, Window *window) {
    logf();
    layer_add_child(window_get_root_layer(window), this->root);
}

Layer *layout_find_by_id(Layout *this, char *id) {
    logf();
    return (Layer *) dict_get(this->ids, id);
}

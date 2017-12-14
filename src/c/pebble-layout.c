#include <pebble.h>
#include "stack.h"
#include "dict.h"
#include "json.h"
#include "standard-types.h"
#include "logging.h"
#include "pebble-layout.h"

struct Layout {
    Layer *root;
    Stack *layers;
    Dict *ids;
    Dict *types;
    Dict *fonts;
    Dict *resource_ids;
};

struct LayerData {
    LayoutFuncs *layout_funcs;
    void *object;
};

struct DefaultLayerData {
    GColor color;
};

struct FontInfo {
    GFont font;
    bool system;
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

static void *prv_default_create(Layout *layout, Json *json, JsonToken *tok) {
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
    logf();
    layer_destroy((Layer *) object);
}

static Layer *prv_default_get_layer(void *object) {
    logf();
    return (Layer *) object;
}

static void prv_default_set_frame(void *object, GRect frame) {
    logf();
    layer_set_frame((Layer *) object, frame);
}

static Layer *json_create_layer(Layout *layout, Json *json) {
    logf();
    JsonToken *tok = json_next(json);
    if (tok->type != JSON_OBJECT) return NULL;

    JsonToken *orig = tok;
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
    logf();
    Layout *this = malloc(sizeof(Layout));
    this->root = NULL;
    this->layers = stack_create();
    this->ids = dict_create();
    this->types = dict_create();
    this->fonts = dict_create();
    this->resource_ids = dict_create();

    layout_add_type(this, "default", (LayoutFuncs) {
        .create = prv_default_create,
        .destroy = prv_default_destroy,
        .get_layer = prv_default_get_layer,
        .set_frame = prv_default_set_frame
    });

    return this;
}

void layout_add_standard_types(Layout *this) {
    logf();
    standard_types_add(this);
}

void layout_parse(Layout *this, uint32_t resource_id) {
    logf();
    Json *json = json_create_with_resource(resource_id);
    if (!json_has_next(json)) goto cleanup;

    int16_t index = json_get_index(json);
    JsonToken *token = json_next(json);
    if (token->type != JSON_OBJECT) goto cleanup;
    json_set_index(json, index);

    this->root = json_create_layer(this, json);
    GRect frame = layer_get_frame(this->root);
    if (grect_equal(&frame, &GRectZero)) {
        layer_set_frame(this->root, GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    }

cleanup:
    json_destroy(json);
}

static bool prv_fonts_destroy_callback(char *key, void *value, void *context) {
    logf();
    FontInfo *font_info = (FontInfo *) value;
    if (!font_info->system) fonts_unload_custom_font(font_info->font);
    font_info->font = NULL;
    free(font_info);
    return true;
}

static bool prv_value_destroy_callback(char *key, void *value, void *context) {
    logf();
    free(value);
    return true;
}

static bool prv_key_destroy_callback(char *key, void *value, void *context) {
    logf();
    free(key);
    return true;
}

void layout_destroy(Layout *this) {
    logf();
    dict_foreach(this->resource_ids, prv_value_destroy_callback, NULL);
    dict_destroy(this->resource_ids);
    this->resource_ids = NULL;

    dict_foreach(this->fonts, prv_fonts_destroy_callback, NULL);
    dict_destroy(this->fonts);
    this->fonts = NULL;

    dict_foreach(this->types, prv_value_destroy_callback, NULL);
    dict_destroy(this->types);
    this->types = NULL;

    dict_foreach(this->ids, prv_key_destroy_callback, NULL);
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

void layout_add_font(Layout *this, char *name, GFont font) {
    logf();
    FontInfo *font_info = malloc(sizeof(FontInfo));
    font_info->font = font;
    font_info->system = false;
    dict_put(this->fonts, name, font_info);
}

GFont layout_get_font(Layout *this, char *name) {
    logf();
    FontInfo *font_info = dict_get(this->fonts, name);
    return font_info ? font_info->font : NULL;
}

void layout_add_resource_id(Layout *this, char *name, uint32_t resource_id) {
    logf();
    uint32_t *rid = malloc(sizeof(uint32_t));
    memcpy(rid, &resource_id, sizeof(uint32_t));
    dict_put(this->resource_ids, name, rid);
}

uint32_t *layout_get_resource_id(Layout *this, char *name) {
    logf();
    return dict_get(this->resource_ids, name);
}

static void prv_add_system_font(Layout *this, char *name, char *font_key) {
    logf();
    FontInfo *font_info = malloc(sizeof(FontInfo));
    font_info->font = fonts_get_system_font(font_key);
    font_info->system = true;
    dict_put(this->fonts, name, font_info);
}

void layout_add_system_fonts(Layout *this) {
    logf();
    prv_add_system_font(this, "GOTHIC_18_BOLD", FONT_KEY_GOTHIC_18_BOLD);
    prv_add_system_font(this, "GOTHIC_24", FONT_KEY_GOTHIC_24);
    prv_add_system_font(this, "GOTHIC_09", FONT_KEY_GOTHIC_09);
    prv_add_system_font(this, "GOTHIC_14", FONT_KEY_GOTHIC_14);
    prv_add_system_font(this, "GOTHIC_14_BOLD", FONT_KEY_GOTHIC_14_BOLD);
    prv_add_system_font(this, "GOTHIC_18", FONT_KEY_GOTHIC_18);
    prv_add_system_font(this, "GOTHIC_24_BOLD", FONT_KEY_GOTHIC_24_BOLD);
    prv_add_system_font(this, "GOTHIC_28", FONT_KEY_GOTHIC_28);
    prv_add_system_font(this, "GOTHIC_28_BOLD", FONT_KEY_GOTHIC_28_BOLD);
    prv_add_system_font(this, "BITHAM_30_BLACK", FONT_KEY_BITHAM_30_BLACK);
    prv_add_system_font(this, "BITHAM_42_BOLD", FONT_KEY_BITHAM_42_BOLD);
    prv_add_system_font(this, "BITHAM_42_LIGHT", FONT_KEY_BITHAM_42_LIGHT);
    prv_add_system_font(this, "BITHAM_42_MEDIUM_NUMBERS", FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
    prv_add_system_font(this, "BITHAM_34_MEDIUM_NUMBERS", FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
    prv_add_system_font(this, "BITHAM_34_LIGHT_SUBSET", FONT_KEY_BITHAM_34_LIGHT_SUBSET);
    prv_add_system_font(this, "BITHAM_18_LIGHT_SUBSET", FONT_KEY_BITHAM_18_LIGHT_SUBSET);
    prv_add_system_font(this, "ROBOTO_CONDENSED_21", FONT_KEY_ROBOTO_CONDENSED_21);
    prv_add_system_font(this, "ROBOTO_BOLD_SUBSET_49", FONT_KEY_ROBOTO_BOLD_SUBSET_49);
    prv_add_system_font(this, "DROID_SERIF_28_BOLD", FONT_KEY_DROID_SERIF_28_BOLD);
    prv_add_system_font(this, "LECO_20_BOLD_NUMBERS", FONT_KEY_LECO_20_BOLD_NUMBERS);
    prv_add_system_font(this, "LECO_26_BOLD_NUMBERS_AM_PM", FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
    prv_add_system_font(this, "LECO_32_BOLD_NUMBERS", FONT_KEY_LECO_32_BOLD_NUMBERS);
    prv_add_system_font(this, "LECO_36_BOLD_NUMBERS", FONT_KEY_LECO_36_BOLD_NUMBERS);
    prv_add_system_font(this, "LECO_38_BOLD_NUMBERS", FONT_KEY_LECO_38_BOLD_NUMBERS);
    prv_add_system_font(this, "LECO_42_NUMBERS", FONT_KEY_LECO_42_NUMBERS);
    prv_add_system_font(this, "LECO_28_LIGHT_NUMBERS", FONT_KEY_LECO_28_LIGHT_NUMBERS);
}

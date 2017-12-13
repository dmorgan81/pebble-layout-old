#include <pebble.h>
#include "pebble-layout.h"
#include "logging.h"
#include "standard-types.h"

static void *prv_text_create(Layout *this, Json *json, jsmntok_t *tok) {
    logf();
    TextLayer *layer = text_layer_create(GRectZero);
    text_layer_set_background_color(layer, GColorClear);

    int size = tok->size;
    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "text")) {
            char *s = json_next_string(json);
            text_layer_set_text(layer, s);
        } else if (json_eq(json, tok, "color")) {
            GColor color = json_next_gcolor(json);
            text_layer_set_text_color(layer, color);
        } else if (json_eq(json, tok, "background")) {
            GColor color = json_next_gcolor(json);
            text_layer_set_background_color(layer, color);
        } else if (json_eq(json, tok, "alignment")) {
            tok = json_next(json);
            GTextAlignment alignment = GTextAlignmentLeft;
            if (json_eq(json, tok, "right"))
                alignment = GTextAlignmentRight;
            else if (json_eq(json, tok, "center"))
                alignment = GTextAlignmentCenter;
            text_layer_set_text_alignment(layer, alignment);
        } else if (json_eq(json, tok, "overflow")) {
            tok = json_next(json);
            GTextOverflowMode overflow = GTextOverflowModeTrailingEllipsis;
            if (json_eq(json, tok, "wrap"))
                overflow = GTextOverflowModeWordWrap;
            else if (json_eq(json, tok, "fill"))
                overflow = GTextOverflowModeFill;
            text_layer_set_overflow_mode(layer, overflow);
        } else if (json_eq(json, tok, "font")) {
            char *s = json_next_string(json);
            GFont font = layout_get_font(this, s);
            free(s);
            if (font) text_layer_set_font(layer, font);
        } else {
            json_skip_tree(json);
        }
    }

    return layer;
}

static void prv_text_destroy(void *object) {
    logf();
    TextLayer *layer = (TextLayer *) object;
    char *s = (char *) text_layer_get_text(layer);
    if (s) free(s);
    text_layer_set_text(layer, NULL);
    text_layer_destroy(layer);
}

static Layer *prv_text_get_layer(void *object) {
    logf();
    return text_layer_get_layer((TextLayer *) object);
}

static void prv_text_set_frame(void *object, GRect frame) {
    logf();
    TextLayer *layer = (TextLayer *) object;
    layer_set_frame(text_layer_get_layer(layer), frame);
}

void standard_types_add(Layout *this) {
    logf();
    layout_add_type(this, "TextLayer", (LayoutFuncs) {
        .create = prv_text_create,
        .destroy = prv_text_destroy,
        .get_layer = prv_text_get_layer,
        .set_frame = prv_text_set_frame
    });
}

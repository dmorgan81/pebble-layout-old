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
            if (json_eq(json, tok, "GTextAlignmentRight"))
                alignment = GTextAlignmentRight;
            else if (json_eq(json, tok, "GTextAlignmentCenter"))
                alignment = GTextAlignmentCenter;
            text_layer_set_text_alignment(layer, alignment);
        } else if (json_eq(json, tok, "overflow")) {
            tok = json_next(json);
            GTextOverflowMode overflow = GTextOverflowModeTrailingEllipsis;
            if (json_eq(json, tok, "GTextOverflowModeWordWrap"))
                overflow = GTextOverflowModeWordWrap;
            else if (json_eq(json, tok, "GTextOverflowModeFill"))
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

static void *prv_bitmap_create(Layout *this, Json *json, jsmntok_t *tok) {
    logf();
    BitmapLayer *layer = bitmap_layer_create(GRectZero);

    int size = tok->size;
    for (int i = 0; i < size; i++) {
        tok = json_next(json);
        if (json_eq(json, tok, "bitmap")) {
            char *s = json_next_string(json);
            uint32_t *resource_id = layout_get_resource_id(this, s);
            free(s);
            if (resource_id) {
                GBitmap *bitmap = gbitmap_create_with_resource(*resource_id);
                bitmap_layer_set_bitmap(layer, bitmap);
            }
        } else if (json_eq(json, tok, "background")) {
            GColor color = json_next_gcolor(json);
            bitmap_layer_set_background_color(layer, color);
        } else if (json_eq(json, tok, "alignment")) {
            tok = json_next(json);
            GAlign alignment = GAlignCenter;
            if (json_eq(json, tok, "GAlignTopLeft"))
                alignment = GAlignTopLeft;
            else if (json_eq(json, tok, "GAlignTop"))
                alignment = GAlignTop;
            else if (json_eq(json, tok, "GAlignTopRight"))
                alignment = GAlignTopRight;
            else if (json_eq(json, tok, "GAlignLeft"))
                alignment = GAlignLeft;
            else if (json_eq(json, tok, "GAlignRight"))
                alignment = GAlignRight;
            else if (json_eq(json, tok, "GAlignBottomLeft"))
                alignment = GAlignBottomLeft;
            else if (json_eq(json, tok, "GAlignBottomRight"))
                alignment = GAlignBottomRight;
            bitmap_layer_set_alignment(layer, alignment);
        } else if (json_eq(json, tok, "compositing")) {
            tok = json_next(json);
            GCompOp compositing = GCompOpAssign;
            if (json_eq(json, tok, "GCompOpAssignInverted"))
                compositing = GCompOpAssignInverted;
            else if (json_eq(json, tok, "GCompOpOr"))
                compositing = GCompOpOr;
            else if (json_eq(json, tok, "GCompOpAnd"))
                compositing = GCompOpAnd;
            else if (json_eq(json, tok, "GCompOpClear"))
                compositing = GCompOpClear;
            else if (json_eq(json, tok, "GCompOpSet"))
                compositing = GCompOpSet;
            bitmap_layer_set_compositing_mode(layer, compositing);
        } else {
            json_skip_tree(json);
        }
    }

    return layer;
}

static void prv_bitmap_destroy(void *object) {
    logf();
    BitmapLayer *layer = (BitmapLayer *) object;
    GBitmap *b = (GBitmap *) bitmap_layer_get_bitmap(layer);
    if (b) gbitmap_destroy(b);
    bitmap_layer_set_bitmap(layer, NULL);
    bitmap_layer_destroy(layer);
}

static Layer *prv_bitmap_get_layer(void *object) {
    logf();
    return bitmap_layer_get_layer((BitmapLayer *) object);
}

static void prv_bitmap_set_frame(void *object, GRect frame) {
    logf();
    BitmapLayer *layer = (BitmapLayer *) object;
    layer_set_frame(bitmap_layer_get_layer(layer), frame);
}

void standard_types_add(Layout *this) {
    logf();
    layout_add_type(this, "TextLayer", (LayoutFuncs) {
        .create = prv_text_create,
        .destroy = prv_text_destroy,
        .get_layer = prv_text_get_layer,
        .set_frame = prv_text_set_frame
    });
    layout_add_type(this, "BitmapLayer", (LayoutFuncs) {
        .create = prv_bitmap_create,
        .destroy = prv_bitmap_destroy,
        .get_layer = prv_bitmap_get_layer,
        .set_frame = prv_bitmap_set_frame
    });
}

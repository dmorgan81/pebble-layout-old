#pragma once
#include <pebble.h>
#include "jsmn.h"

typedef struct Json Json;

Json *json_create_with_resource(uint32_t resource_id);
void json_destroy(Json *this);
bool json_has_next(Json *this);
jsmntok_t *json_next(Json *this);
char *json_next_string(Json *this);
int json_next_int(Json *this);
bool json_next_bool(Json *this);
GRect json_next_grect(Json *this);
GColor json_next_gcolor(Json *this);
void json_skip_tree(Json *this);
int16_t json_get_index(Json *this);
void json_set_index(Json *this, int16_t index);
bool json_eq(Json *this, jsmntok_t *tok, const char *s);

#pragma once
#include <pebble.h>

typedef struct Json Json;

typedef enum {
    JSON_UNDEFINED = 0,
    JSON_OBJECT = 1,
    JSON_ARRAY = 2,
    JSON_STRING = 3,
    JSON_PRIMITIVE = 4
} JsonType;

typedef struct {
    JsonType type;
    int start;
    int end;
    int len;
    int size;
} JsonToken;

Json *json_create_with_resource(uint32_t resource_id);
Json *json_create(char *s);
void json_destroy(Json *this);
bool json_has_next(Json *this);
JsonToken *json_next(Json *this);
char *json_next_string(Json *this);
int json_next_int(Json *this);
bool json_next_bool(Json *this);
GRect json_next_grect(Json *this);
GColor json_next_gcolor(Json *this);
void json_skip_tree(Json *this);
int16_t json_get_index(Json *this);
void json_set_index(Json *this, int16_t index);
bool json_eq(Json *this, JsonToken *tok, const char *s);

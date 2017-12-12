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
void json_mark(Json *this);
void json_reset(Json *this);
bool json_eq(Json *this, jsmntok_t *tok, const char *s);

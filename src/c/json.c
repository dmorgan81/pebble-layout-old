#include <pebble.h>
#include "jsmn.h"
#include "string.h"
#include "logging.h"
#include "json.h"

struct Json {
    char *buf;
    JsonToken *tokens;
    int16_t num_tokens;
    int16_t index;
};

Json *json_create_with_resource(uint32_t resource_id) {
    logf();
    Json *this = malloc(sizeof(Json));

    ResHandle res_handle = resource_get_handle(resource_id);
    size_t res_size = resource_size(res_handle);
    this->buf = malloc(sizeof(char) * (res_size + 1));
    resource_load(res_handle, (uint8_t *) this->buf, res_size);
    this->buf[res_size] = '\0';

    jsmn_parser parser;
    jsmn_init(&parser);

    int num_tokens = jsmn_parse(&parser, this->buf, strlen(this->buf), NULL, 0);
    jsmn_init(&parser);
    jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * num_tokens);
    this->num_tokens = jsmn_parse(&parser, this->buf, strlen(this->buf), tokens, num_tokens);
    this->index = 0;

    this->tokens = malloc(sizeof(JsonToken) * this->num_tokens);
    for (int i = 0; i < this->num_tokens; i++) {
        jsmntok_t *tok = &tokens[i];
        JsonToken *js_tok = &this->tokens[i];
        js_tok->type = tok->type;
        js_tok->start = tok->start;
        js_tok->end = tok->end;
        js_tok->len = tok->end - tok->start;
        js_tok->size = tok->size;
    }

    free(tokens);

    return this;
}

void json_destroy(Json *this) {
    logf();
    this->index = -1;
    this->num_tokens = -1;

    free(this->tokens);
    this->tokens = NULL;

    free(this->buf);
    this->buf = NULL;

    free(this);
}

bool json_has_next(Json *this) {
    logf();
    return this->tokens != NULL && this->num_tokens > -1 && this->index < this->num_tokens;
}

JsonToken *json_next(Json *this) {
    logf();
    return &this->tokens[this->index++];
}

char *json_next_string(Json *this) {
    logf();
    JsonToken *tok = json_next(this);
    return strndup(this->buf + tok->start, tok->len);
}

int json_next_int(Json *this) {
    logf();
    char *s = json_next_string(this);
    int i = atoi(s);
    free(s);
    return i;
}

bool json_next_bool(Json *this) {
    logf();
    JsonToken *tok = json_next(this);
    return tok->type == JSON_PRIMITIVE && \
           (int) strlen("true") == tok->len && \
           strncmp(this->buf + tok->start, "true", tok->len) == 0;
}

GRect json_next_grect(Json *this) {
    logf();
    JsonToken *tok = json_next(this);
    if (tok->type != JSON_ARRAY) return GRectZero;

    int16_t values[4];
    for (uint i = 0; i < ARRAY_LENGTH(values); i++) {
        values[i] = json_next_int(this);
    }
    return GRect(values[0], values[1], values[2], values[3]);
}

GColor json_next_gcolor(Json *this) {
    logf();
    char *s = json_next_string(this);
    GColor color = GColorFromHEX(strtoul(s + (s[0] == '#' ? 1 : 0), NULL, 16));
    free(s);
    return color;
}

void json_skip_tree(Json *this) {
    logf();
    JsonToken *tok = json_next(this);
    if (tok->type == JSON_ARRAY) {
        int size = tok->size;
        for (int i = 0; i < size; i++) json_skip_tree(this);
    } else if (tok->type == JSON_OBJECT) {
        int size = tok->size;
        for (int i = 0; i < size; i++) {
            tok = json_next(this);
            json_skip_tree(this);
        }
    }
}

int16_t json_get_index(Json *this) {
    logf();
    return this->index;
}

void json_set_index(Json *this, int16_t index) {
    logf();
    this->index = index;
}

bool json_eq(Json *this, JsonToken *tok, const char *s) {
    logf();
    return tok->type == JSON_STRING && \
           (int) strlen(s) == tok->len && \
           strncmp(this->buf + tok->start, s, tok->len) == 0;
}

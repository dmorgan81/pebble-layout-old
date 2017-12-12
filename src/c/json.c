#include <pebble.h>
#include "jsmn.h"
#include "string.h"
#include "logging.h"
#include "json.h"

struct Json {
    char *buf;
    jsmntok_t *tokens;
    int16_t num_tokens;
    int16_t index;
    int16_t mark;
};

Json *json_create_with_resource(uint32_t resource_id) {
    logf();
    Json *this = malloc(sizeof(Json));

    ResHandle res_handle = resource_get_handle(resource_id);
    size_t res_size = resource_size(res_handle);
    this->buf = malloc(sizeof(char) * res_size);
    resource_load(res_handle, (uint8_t *) this->buf, res_size);

    jsmn_parser parser;
    jsmn_init(&parser);

    int num_tokens = jsmn_parse(&parser, this->buf, strlen(this->buf), NULL, 0);
    jsmn_init(&parser);
    this->tokens = malloc(sizeof(jsmntok_t) * num_tokens);
    this->num_tokens = jsmn_parse(&parser, this->buf, strlen(this->buf), this->tokens, num_tokens);
    this->index = 0;
    this->mark = -1;

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

jsmntok_t *json_next(Json *this) {
    logf();
    return &this->tokens[this->index++];
}

char *json_next_string(Json *this) {
    logf();
    jsmntok_t *tok = json_next(this);
    return strndup(this->buf + tok->start, tok->end - tok->start);
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
    jsmntok_t *tok = json_next(this);
    return tok->type == JSMN_PRIMITIVE && \
           (int) strlen("true") == tok->end - tok->start && \
           strncmp(this->buf + tok->start, "true", tok->end - tok->start) == 0;
}

void json_mark(Json *this) {
    logf();
    this->mark = this->index;
}

void json_reset(Json *this) {
    logf();
    if (this->mark > -1) this->index = this->mark;
}

bool json_eq(Json *this, jsmntok_t *tok, const char *s) {
    logf();
    return tok->type == JSMN_STRING && \
           (int) strlen(s) == tok->end - tok->start && \
           strncmp(this->buf + tok->start, s, tok->end - tok->start) == 0;
}

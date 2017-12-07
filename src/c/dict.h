#pragma once

typedef struct Dict Dict;

typedef bool (*DictForEachCallback)(char *key, void *value, void *context);

Dict *dict_create(void);
void dict_destroy(Dict *this);
void dict_put(Dict *this, char *key, void *value);
bool dict_contains(Dict *this, char *key);
void *dict_get(Dict *this, char *key);
void dict_foreach(Dict *this, DictForEachCallback callback, void *context);

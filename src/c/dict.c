#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "string.h"
#include "logging.h"
#include "dict.h"

struct Dict {
    LinkedRoot *entries;
};

struct Entry {
    char *key;
    void *value;
};

Dict *dict_create(void) {
    logf();
    Dict *this = malloc(sizeof(Dict));
    this->entries = NULL;
    return this;
}

static bool prv_destroy_callback(void *object, void *context) {
    logf();
    free(object);
    object = NULL;
    return true;
}

void dict_destroy(Dict *this) {
    logf();
    if (this->entries) {
        linked_list_foreach(this->entries, prv_destroy_callback, NULL);
        linked_list_clear(this->entries);
        free(this->entries);
        this->entries = NULL;
    }
    free(this);
}

void dict_put(Dict *this, char *key, void *value) {
    logf();
    if (!this->entries) this->entries = linked_list_create_root();
    struct Entry *entry = malloc(sizeof(struct Entry));
    entry->key = key;
    entry->value = value;
    linked_list_append(this->entries, entry);
}

static bool prv_find_by_key_callback(void *object1, void *object2) {
    logf();
    char *key = (char *) object1;
    struct Entry *entry = (struct Entry *) object2;
    return strcmp(key, entry->key) == 0;
}

bool dict_contains(Dict *this, char *key) {
    logf();
    if (!this->entries) return false;
    return linked_list_find_compare(this->entries, key, prv_find_by_key_callback) >= 0;
}

void *dict_get(Dict *this, char *key) {
    logf();
    if (!this->entries) return NULL;
    int16_t index = linked_list_find_compare(this->entries, key, prv_find_by_key_callback);
    if (index < 0) return NULL;
    struct Entry *entry = (struct Entry *) linked_list_get(this->entries, index);
    return entry->value;
}

void *dict_remove(Dict *this, char *key) {
    logf();
    if (!this->entries) return NULL;
    int16_t index = linked_list_find_compare(this->entries, key, prv_find_by_key_callback);
    if (index < 0) return NULL;
    struct Entry *entry = (struct Entry *) linked_list_get(this->entries, index);
    void *value = entry->value;
    free(entry);

    if (linked_list_count(this->entries) == 0) {
        free(this->entries);
        this->entries = NULL;
    }

    return value;
}

struct ForEachData {
    DictForEachCallback callback;
    void *context;
};

static bool prv_foreach_callback(void *object, void *context) {
    logf();
    struct Entry *entry = (struct Entry *) object;
    struct ForEachData *data = (struct ForEachData *) context;
    return data->callback(entry->key, entry->value, data->context);
}

void dict_foreach(Dict *this, DictForEachCallback callback, void *context) {
    logf();
    if (!this->entries) return;
    struct ForEachData data = {
        .callback = callback,
        .context = context
    };
    linked_list_foreach(this->entries, prv_foreach_callback, &data);
}

#include <@smallstoneapps/linked-list/linked-list.h>
#include "logging.h"
#include "stack.h"

struct Stack {
    LinkedRoot *root;
};

Stack *stack_create(void) {
    logf();
    Stack *this = malloc(sizeof(Stack));
    this->root = NULL;
    return this;
}

void stack_destroy(Stack *this) {
    logf();
    if (this->root) {
        linked_list_clear(this->root);
        free(this->root);
    }
    this->root = NULL;
    free(this);
}

void stack_push(Stack *this, void *data) {
    logf();
    if (!this->root) this->root = linked_list_create_root();
    linked_list_prepend(this->root, data);
}

void *stack_pop(Stack *this) {
    logf();
    if (!this->root) return NULL;
    void *data = linked_list_get(this->root, 0);
    linked_list_remove(this->root, 0);
    if (linked_list_count(this->root) == 0) {
        free(this->root);
        this->root = NULL;
    }
    return data;
}

void *stack_peek(Stack *this) {
    logf();
    return (!this->root) ? NULL : linked_list_get(this->root, 0);
}

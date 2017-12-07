#pragma once

typedef struct Stack Stack;

Stack *stack_create(void);
void stack_destroy(Stack *this);
void stack_push(Stack *this, void *data);
void *stack_pop(Stack *this);
void *stack_peek(Stack *this);

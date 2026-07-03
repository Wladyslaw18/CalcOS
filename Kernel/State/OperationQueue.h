#ifndef OPERATION_QUEUE_H
#define OPERATION_QUEUE_H

#include "CalculatorState.h"

#define QUEUE_MAX_SIZE 16

typedef struct {
    OpType op;
    double operand;
} QueuedOperation;

// Initialize the static queue
void queue_init(void);

// Enqueue a new operation. Returns 1 if successful, 0 if queue is full.
int queue_enqueue(OpType op, double operand);

// Dequeue the next operation. Returns 1 if successful, 0 if queue is empty.
int queue_dequeue(QueuedOperation* out_op);

// Check if queue is empty
int queue_is_empty(void);

// Check if queue is full
int queue_is_full(void);

// Clear queue
void queue_clear(void);

#endif // OPERATION_QUEUE_H

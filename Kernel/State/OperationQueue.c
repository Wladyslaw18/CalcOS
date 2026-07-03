#include "OperationQueue.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

static QueuedOperation op_queue[QUEUE_MAX_SIZE];
static uint32_t head = 0;
static uint32_t tail = 0;
static uint32_t count = 0;

void queue_init(void) {
    head = 0;
    tail = 0;
    count = 0;
    fast_memset(op_queue, 0, sizeof(op_queue));
}

int queue_enqueue(OpType op, double operand) {
    if (count >= QUEUE_MAX_SIZE) {
        return 0;
    }
    op_queue[tail].op = op;
    op_queue[tail].operand = operand;
    tail = (tail + 1) % QUEUE_MAX_SIZE;
    count++;
    return 1;
}

int queue_dequeue(QueuedOperation* out_op) {
    if (count == 0 || !out_op) {
        return 0;
    }
    *out_op = op_queue[head];
    head = (head + 1) % QUEUE_MAX_SIZE;
    count--;
    return 1;
}

int queue_is_empty(void) {
    return count == 0;
}

int queue_is_full(void) {
    return count >= QUEUE_MAX_SIZE;
}

void queue_clear(void) {
    head = 0;
    tail = 0;
    count = 0;
    fast_memset(op_queue, 0, sizeof(op_queue));
}

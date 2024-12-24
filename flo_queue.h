#ifndef FLO_QUEUE_H
#define FLO_QUEUE_H

#include <threads.h>
#include <stdbool.h>

// C11 multithread safe queue
// it will assert / abort() on invalid usage

typedef struct flo_queue flo_queue_t;

// initializes queue with n_elements capacity of element_size
flo_queue_t *flo_queue_create(int n_elements, size_t element_size);
// initializes queue with n_elements capacity

// free resources of queue
void flo_queue_free(flo_queue_t *queue);

// pop element, block if empty
// return NULL if queue closed
void *flo_queue_pop_block(flo_queue_t *queue, void *result);

// push element, block if full
void flo_queue_push_block(flo_queue_t *queue, void *el);

// no more elements to be written
void flo_queue_close(flo_queue_t *queue);

// checks if there is smthg in the queue or not
bool flo_queue_empty(flo_queue_t *queue);

/* ------------------------------------------------------------------------- */
#ifdef FLO_QUEUE_IMPLEMENTATION

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct flo_queue
{
    mtx_t lock;
    cnd_t condition_empty; // not empty
    cnd_t condition_full;  // not full
    int n_elements;
    int element_count;
    int element_size;
    // ptr to element_size * n_elements sized field
    void *elements;
    bool closed;
} flo_queue_t;


// initializes queue with n_elements capacity
flo_queue_t *flo_queue_create(int n_elements, size_t element_size)
{
    assert(n_elements > 0);
    assert(element_size > 0);
    flo_queue_t *queue = (flo_queue_t *)calloc(1, sizeof(flo_queue_t));
    queue->elements = calloc(n_elements, element_size);
    queue->element_size = element_size;
    queue->element_count = 0;
    queue->n_elements = n_elements;
    mtx_init(&queue->lock, mtx_plain);
    cnd_init(&queue->condition_full);
    cnd_init(&queue->condition_empty);
    queue->closed = false;
    return queue;
}

// initializes queue with n_elements capacity
void flo_queue_free(flo_queue_t *queue)
{
    assert(queue);
    assert(queue->elements);
    mtx_destroy(&queue->lock);
    cnd_destroy(&queue->condition_full);
    cnd_destroy(&queue->condition_empty);
    free(queue->elements);
    free(queue);
}

static void *flo_queue_ptr_to_element(flo_queue_t *queue, int n)
{
    assert(queue);
    assert(n >= 0);
    assert(n < queue->n_elements);

    uint8_t *ptr = (uint8_t *)queue->elements;
    return ptr + (n * queue->element_size);
}

void flo_queue_close(flo_queue_t *queue)
{
    queue->closed = true;
    // not empty any more, make workers read
    cnd_broadcast(&queue->condition_empty);
}

// returns ptr to element (result)
void *flo_queue_pop_block(flo_queue_t *queue, void *result)
{
    assert(queue);
    if (thrd_success != mtx_lock(&queue->lock))
    {
        abort();
    }
    do
    {
        if (queue->element_count > 0)
        {
            void *ptr = flo_queue_ptr_to_element(queue, 0);
            memcpy(result, ptr, queue->element_size);
            if (queue->element_count > 1)
            {
                void *next = flo_queue_ptr_to_element(queue, 1);
                memcpy(ptr, next, queue->element_size * (queue->element_count - 1));
            }
            queue->element_count--;

            if (thrd_success != mtx_unlock(&queue->lock))
            {
                abort();
            }
            // not full any more
            cnd_signal(&queue->condition_full);
            return result;
        }
        else if (queue->closed)
        {

            if (thrd_success != mtx_unlock(&queue->lock))
            {
                abort();
            }
            return NULL;
        }
        // wait for not empty
        if (thrd_success != cnd_wait(&queue->condition_empty, &queue->lock))
        {
            abort();
        }
    } while (true);
}

// checks if there is smthg in the queue or not
bool flo_queue_empty(flo_queue_t *queue)
{
    assert(queue);
    if (thrd_success != mtx_lock(&queue->lock))
    {
        abort();
    }
    bool empty = (queue->element_count == 0);
    if (thrd_success != mtx_unlock(&queue->lock))
    {
        abort();
    }
    return empty;
}

void flo_queue_push_block(flo_queue_t *queue, void *el)
{
    assert(queue);
    assert(el);
    if (thrd_success != mtx_lock(&queue->lock))
    {
        abort();
    }
    do
    {
        if (queue->closed)
        {
            if (thrd_success != mtx_unlock(&queue->lock))
            {
                abort();
            }
            return;
        }
        if (queue->element_count < queue->n_elements)
        {
            void *ptr = flo_queue_ptr_to_element(queue, queue->element_count++);
            memcpy(ptr, el, queue->element_size);

            if (thrd_success != mtx_unlock(&queue->lock))
            {
                abort();
            }
            // not empty any more
            cnd_signal(&queue->condition_empty);
            return;
        }
        // wait for not full any more
        if (thrd_success != cnd_wait(&queue->condition_full, &queue->lock))
        {
            abort();
        }
    } while (true);
}

#endif
#endif

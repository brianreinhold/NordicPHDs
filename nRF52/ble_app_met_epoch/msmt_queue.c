// Utility function to initialize a queue
/*
Copyright (c) 2020 - 2024, Brian Reinhold

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the �Software�), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/


#include <stdlib.h>
#include <string.h>
#include "msmt_queue.h"
#ifndef _WIN32
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#else
#define NRF_LOG_DEBUG printf
#include <windows.h>
#include <stdio.h>
#endif

s_Queue* initializeQueue(int size)
{
    s_Queue* queue = NULL;
    queue = (s_Queue *)calloc(1, sizeof(s_Queue));

    if (queue != NULL)
    {
        queue->msmts = (void**)calloc(1, size * sizeof(void*));
        queue->maxsize = size;
        queue->front = 0;
        queue->rear = -1;
        queue->size = 0;
    }

    return queue;
}

int isFull(s_Queue *queue)
{
    return (size(queue) == queue->maxsize);
}

// Utility function to return the size of the queue
int size(s_Queue* queue) {
    return queue->size;
}

// Utility function to check if the queue is empty or not
int isEmpty(s_Queue* queue) {
    return !size(queue);
}

// Utility function to return the front element of the queue
void* front(s_Queue* queue)
{
    if (isEmpty(queue))
    {
        NRF_LOG_DEBUG("Underflow: Program Terminated\r\n");
        return NULL;

    }

    return queue->msmts[queue->front];
}

// Utility function to add an element `x` to the queue
void enqueue(s_Queue* queue, void* msmt, unsigned short length)
{
    if (size(queue) == queue->maxsize)
    {
        NRF_LOG_DEBUG("Overflow Program Terminated\r\n");
        return;
    }
    queue->rear = (queue->rear + 1) % queue->maxsize;    // circular queue
    if (queue->msmts[queue->rear] == NULL)
    {
        queue->msmts[queue->rear] = (void*)calloc(1, length);
        if (queue->msmts[queue->rear] == NULL)
        {
            //error
            return;
        }
    }
    memcpy(queue->msmts[queue->rear], msmt, length);
    queue->size++;

    NRF_LOG_DEBUG("front = %d, rear = %d\r\n", queue->front, queue->rear);
}

// Utility function to dequeue the front element
void dequeue(s_Queue* queue)
{
    if (isEmpty(queue))    // front == rear
    {
        NRF_LOG_DEBUG("Underflow: queue empty");
        return;
    }

    if (queue->msmts[queue->front] != NULL)
    {
        free(queue->msmts[queue->front]);
        queue->msmts[queue->front] = NULL;
    }

    queue->front = (queue->front + 1) % queue->maxsize;  // circular queue
    queue->size--;

    NRF_LOG_DEBUG("front = %d, rear = %d\r\n", queue->front, queue->rear);
}

void emptyQueue(s_Queue* queue)
{
    if (queue != NULL)
    {
        if (queue->msmts != NULL)
        {
            int i;
            for (i = 0; i < queue->maxsize; i++)
            {
                if (queue->msmts[i] != NULL)
                {
                    free(queue->msmts[i]);
                    queue->msmts[i] = NULL;
                }
            }
            queue->front = 0;
            queue->rear = -1;
            queue->size = 0;
        }
    }
}

void cleanUpQueue(s_Queue* queue)
{
    if (queue != NULL)
    {
        emptyQueue(queue);
        free(queue);
    }
}

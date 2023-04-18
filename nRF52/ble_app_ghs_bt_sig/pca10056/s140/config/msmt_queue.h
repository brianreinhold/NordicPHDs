/*
Copyright (c) 2020 - 2024, Brian Reinhold

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

// Utility function to initialize a queue
#ifndef MSMT_QUEUE_H__
#define MSMT_QUEUE_H__

// Data structure to represent a queue
typedef struct
{
    void** msmts;     // array to store queue elements
    int maxsize;    // maximum capacity of the queue
    int front;      // front points to the front element in the queue (if any)
    int rear;       // rear points to the last element in the queue
    int size;       // current capacity of the queue
}s_Queue;

s_Queue* initializeQueue(int size);

// Utility function to return the size of the queue
int size(s_Queue* queue);

// Utility function to check if the queue is empty or not
int isEmpty(s_Queue* queue);

int isFull(s_Queue *queue);

// Utility function to return the front element of the queue
void* front(s_Queue* queue);

// Utility function to add an element `x` to the queue
void enqueue(s_Queue* queue, void* msmt, unsigned short length);

// Utility function to dequeue the front element
void dequeue(s_Queue* queue);

void emptyQueue(s_Queue* queue);
void cleanUpQueue(s_Queue* queue);
#endif

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

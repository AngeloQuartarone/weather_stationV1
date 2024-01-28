#include "circularQueue.h"

// i dati vengono inseritoi con politica lifo, il dato più nuovo sta in prima posizione dell'array

circularQueue *initQueue(int size)
{
    circularQueue *q = (circularQueue *)calloc(1, sizeof(circularQueue));
    q->size = size + 1;
    q->arr = (void *)calloc(q->size, sizeof(void *));
    q->front = q->rear = 0;
    return q;
}

int isEmpty(circularQueue *q)
{
    if (q->rear == q->front)
    {
        return 1;
    }
    return 0;
}

int isFull(circularQueue *q)
{
    if ((q->rear + 1) % q->size == q->front)
    {
        return 1;
    }
    return 0;
}

int enqueue(circularQueue *q, float val)
{
    if (isFull(q))
    {
        q->front = (q->front + 1) % q->size;
    }
    else
    {
        q->rear = (q->rear + 1) % q->size;
    }
    q->arr[q->rear] = val;
    return 0;
}

int dequeue(circularQueue *q)
{
    if (isEmpty(q))
    {
        return -1;
    }
    else
    {
        q->front = (q->front + 1) % q->size;
    }
    return 0;
}

void deleteQueue(circularQueue *q)
{
    free(q->arr);
    free(q);
}

/*int main()
{
    circularQueue *q = initQueue(2);

    // Enqueue few elements
    if(enqueue(q, 12) == -1){
        printf("enqueue su coda piena\n");
    }
    if(enqueue(q, 15) == -1){
        printf("enqueue su coda piena\n");
    }
    if(enqueue(q, 17) == -1){
        printf("enqueue su coda piena\n");
    }
    if(dequeue(q) == -1){
        printf("dequeue su coda vuota\n");
    }
    if(dequeue(q) == -1){
        printf("dequeue su coda vuota\n");
    }
    if(dequeue(q) == -1){
        printf("dequeue su coda vuota\n");
    }

     if(enqueue(q, 45) == -1){
        printf("enqueue su coda piena\n");
    }
     if(enqueue(q, 22) == -1){
        printf("enqueue su coda piena\n");
    }
    if(enqueue(q, 55) == -1){
        printf("enqueue su coda piena\n");
    }

    if (isEmpty(q))
    {
        printf("Queue is empty\n");
    }
    if (isFull(q))
    {
        printf("Queue is full\n");
    }

    return 0;
}*/
//
// Created by Shinelon on 2024/8/13.
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

typedef void(*CallbackFunc)();

// 队列节点结构
typedef struct Node {
    CallbackFunc data;
    int64_t timeout;
    struct Node* next;
} Node;

// 队列结构
typedef struct Queue {
    Node* front;
    Node* rear;
    pthread_mutex_t lock;
    pthread_cond_t cond_non_empty;
} Queue;

// 初始化队列
void queue_init(Queue* q) {
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond_non_empty, NULL);
}

// 销毁队列
void queue_destroy(Queue* q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond_non_empty);
}

uint64_t GetMicroseconds() {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    // 获取计数器频率，单位是每秒多少计数
    QueryPerformanceFrequency(&frequency);

    // 获取当前计数器值
    QueryPerformanceCounter(&counter);

    // 计算微秒数
    return counter.QuadPart * 1000000 / frequency.QuadPart;
}

// 入队操作
void enqueue(Queue* q, CallbackFunc value, long int delay) {
    Node* new_node = malloc(sizeof(Node));
    new_node->data = value;
    new_node->timeout = GetMicroseconds() + delay;
    new_node->next = NULL;
    pthread_mutex_lock(&q->lock);
    if (q->rear) {
        q->rear->next = new_node;
    } else {
        q->front = new_node;
    }
    q->rear = new_node;
    pthread_cond_signal(&q->cond_non_empty);
    pthread_mutex_unlock(&q->lock);
}

// 出队操作
Node* dequeue(Queue* q) {
    pthread_mutex_lock(&q->lock);
    while (q->front == NULL) {
        pthread_cond_wait(&q->cond_non_empty, &q->lock);
    }
    Node* temp = q->front;
    q->front = temp->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    pthread_mutex_unlock(&q->lock);
    return temp;
}

//参数一表示 需要等待的时间 微秒为单位
int usSleep(int us)
{
    //储存计数的联合
    LARGE_INTEGER fre;
    //获取硬件支持的高精度计数器的频率
    if (QueryPerformanceFrequency(&fre))
    {
        LARGE_INTEGER run, priv, curr, res;
        run.QuadPart = fre.QuadPart * us / 1000000; //转换为微妙级
        //获取高精度计数器数值
        QueryPerformanceCounter(&priv);
        do
        {
            QueryPerformanceCounter(&curr);
        }
        while (curr.QuadPart - priv.QuadPart < run.QuadPart);
        curr.QuadPart -= priv.QuadPart;
        int nres = (curr.QuadPart * 1000000 / fre.QuadPart); //实际使用微秒时间
        return nres;
    }
    return -1; //
}

static pthread_t timer_thread;
static Queue* task_queue;

void* task_handler(void* arg)
{
    while (1) {
        Node* node = dequeue(task_queue);
        auto delay = GetMicroseconds() - node->timeout;
        if(delay > 0)
        {
            usSleep(delay);
        }
        node->data();
        free(node);
    }
}

void init_win_setitimer()
{
    if(!task_queue)
    {
        task_queue = malloc(sizeof(Queue));
        queue_init(task_queue);
        pthread_create(timer_thread, NULL, &task_handler, NULL);
    }
}

void win_setitimer(long int delay, void (*handler)())
{
    enqueue(task_queue, handler, delay);
}
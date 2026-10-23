// threadpool - a concurrent work queue

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef struct task_node {
    char task[1024];
    struct task_node *next;
} task_node_t;

typedef struct {
    task_node_t *head;
    task_node_t *tail;
    int done;
    int count;
    int base_workers;
    int active_workers;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} task_queue_t;

task_queue_t queue = {
    .head = NULL,
    .tail = NULL,
    .done = 0,
    .count = 0,
    .base_workers = 0,
    .active_workers = 0
};

unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

void process_task(const char *task) {
    usleep(100000);

    unsigned long hash = djb2_hash(task);

    printf("%lu %s\n", hash, task);
    fflush(stdout);
}

void push_task(task_queue_t *q, const char *task) {
    task_node_t *node = malloc(sizeof(task_node_t));

    strcpy(node->task, task);
    node->next = NULL;

    if (q->tail) {
        q->tail->next = node;
        q->tail = node;
    } else {
        q->head = node;
        q->tail = node;
    }

    q->count++;
}

int pop_task(task_queue_t *q, char *buf) {
    if (q->head == NULL) {
        return 0;
    }

    task_node_t *node = q->head;

    strcpy(buf, node->task);

    q->head = node->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }

    free(node);

    q->count--;

    return 1;
}

void *worker(void *arg) {
    (void)arg;

    while (1) {
        char task[1024];

        pthread_mutex_lock(&queue.mutex);

        while (queue.head == NULL && !queue.done) {
            if (queue.active_workers > queue.base_workers) {
                queue.active_workers--;
                pthread_mutex_unlock(&queue.mutex);
                return NULL;
            }

            pthread_cond_wait(&queue.cond, &queue.mutex);
        }

        if (queue.head == NULL && queue.done) {
            queue.active_workers--;
            pthread_mutex_unlock(&queue.mutex);
            break;
        }

        int got_task = pop_task(&queue, task);

        pthread_mutex_unlock(&queue.mutex);

        if (got_task) {
            process_task(task);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    char line[1024];

    if (argc < 2) {
        printf("Usage: %s <threads>\n", argv[0]);
        return 1;
    }

    int thread_count = atoi(argv[1]);

    if (thread_count < 1) {
        printf("Usage: %s <threads>\n", argv[0]);
        return 1;
    }

    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.cond, NULL);

    queue.base_workers = thread_count;
    queue.active_workers = thread_count;

    int thread_capacity = thread_count * 2;

    if (thread_capacity < 4) {
        thread_capacity = 4;
    }

    pthread_t *threads =
        malloc(sizeof(pthread_t) * thread_capacity);

    int total_threads = 0;

    for (int i = 0; i < thread_count; i++) {
        pthread_create(
            &threads[total_threads],
            NULL,
            worker,
            NULL
        );

        total_threads++;
    }

    while (fgets(line, sizeof(line), stdin) != NULL) {
        char *newline = strchr(line, '\n');

        if (newline != NULL) {
            *newline = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        pthread_mutex_lock(&queue.mutex);

        push_task(&queue, line);
        pthread_cond_signal(&queue.cond);

        if (queue.count > 2 * queue.base_workers) {

            if (total_threads == thread_capacity) {
                thread_capacity *= 2;

                threads = realloc(
                    threads,
                    sizeof(pthread_t) * thread_capacity
                );
            }

            queue.active_workers++;

            if (pthread_create(
                    &threads[total_threads],
                    NULL,
                    worker,
                    NULL
                ) == 0) {

                total_threads++;

            } else {
                queue.active_workers--;
            }
        }

        pthread_mutex_unlock(&queue.mutex);
    }

    pthread_mutex_lock(&queue.mutex);

    queue.done = 1;
    pthread_cond_broadcast(&queue.cond);

    pthread_mutex_unlock(&queue.mutex);

    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);

    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.cond);

    return 0;
}

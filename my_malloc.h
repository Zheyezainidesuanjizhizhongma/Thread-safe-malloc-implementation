#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>

#define metadata_size sizeof(metadata)

typedef struct metadata_tag{
    size_t allocSize;
    void * prev;
    void * next;
} metadata;

//metadata * head_metadata = NULL;
metadata * head_free_metadata_lock = NULL;
metadata * tail_free_metadata_lock = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread metadata * head_free_metadata_nolock = NULL;
__thread metadata * tail_free_metadata_nolock = NULL;

void *bf_malloc(size_t size, metadata ** head_free_metadata, metadata ** tail_free_metadata, int isLock);
void bf_free(void *ptr, metadata ** head_free_metadata, metadata ** tail_free_metadata);
void * ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
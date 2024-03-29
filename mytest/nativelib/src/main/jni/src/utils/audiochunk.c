#include "audiochunk.h"
#include "errno.h"
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static void _freeAudioChunk(AudioChunk **chunk);

AudioChunkQueue* createAudioChunkQueue(int maxCapacity)
{
    AudioChunkQueue *queue = malloc(sizeof(AudioChunkQueue));
    queue->first = NULL;
    queue->last = NULL;
    queue->quantity = 0;
    queue->capacity = maxCapacity;
    queue->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    
    sem_unlink("/full");
    sem_unlink("/empty");

    //有名信号量
    queue->full = sem_open("/full", O_CREAT, 0644, 0);
    queue->empty = sem_open("/empty", O_CREAT, 0644, maxCapacity);
    //释放使用sem_close,sem_unlink


    //无名信号量
    //创建使用sem_init, 释放使用sem_destroy

    return queue;
}

int insertAudioChunk(AudioChunk *chunk, AudioChunkQueue *queue)
{
    if (queue == NULL) {
        return 0;
    }
    
    AudioChunkList *newChunkList = malloc(sizeof(AudioChunkList));
    newChunkList->chunk = chunk;
    newChunkList->next = NULL;
    
    sem_wait(queue->empty);
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->quantity == 0) {
        queue->first = newChunkList;
    } else {
        queue->last->next = newChunkList;
    }
    queue->last = newChunkList;
    
    queue->quantity++;
    
    pthread_mutex_unlock(&queue->mutex);
    
    sem_post(queue->full);
    
    return 1;
}

int tryGetNextAudioChunk(AudioChunk **chunk, AudioChunkQueue *queue, int howManyBytes){
    if (queue == NULL) {
        return 0;
    }

    if(sem_trywait(queue->full) == EAGAIN){
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    AudioChunkList *first = queue->first;
    if (first->chunk->size <= howManyBytes) {
        *chunk = first->chunk;
        queue->first = queue->first->next;

        queue->quantity--;
        if (queue->quantity == 0) {
            queue->first = NULL;
            queue->last = NULL;
        }

        free(first);
        sem_post(queue->empty);

    } else {
        sem_post(queue->full);

        *chunk = malloc(sizeof(AudioChunk));
        (*chunk)->size = howManyBytes;
        (*chunk)->data = malloc(howManyBytes);
        memcpy((*chunk)->data, first->chunk->data, howManyBytes);

        first->chunk->size -= howManyBytes;
        uint8_t *remainingData = malloc(first->chunk->size);
        memcpy(remainingData, first->chunk->data + howManyBytes, first->chunk->size);
        free(first->chunk->data);
        first->chunk->data = remainingData;
    }

    pthread_mutex_unlock(&queue->mutex);

    return 1;
}


int getNextAudioChunk(AudioChunk **chunk, AudioChunkQueue *queue, int howManyBytes)
{
    if (queue == NULL) {
        return 0;
    }
    
    sem_wait(queue->full);
    pthread_mutex_lock(&queue->mutex);
    
    AudioChunkList *first = queue->first;
    if (first->chunk->size <= howManyBytes) {
        *chunk = first->chunk;
        queue->first = queue->first->next;
        
        queue->quantity--;
        if (queue->quantity == 0) {
            queue->first = NULL;
            queue->last = NULL;
        }

        free(first);
        sem_post(queue->empty);

    } else {
        sem_post(queue->full);
        
        *chunk = malloc(sizeof(AudioChunk));
        (*chunk)->size = howManyBytes;
        (*chunk)->data = malloc(howManyBytes);
        memcpy((*chunk)->data, first->chunk->data, howManyBytes);
        
        first->chunk->size -= howManyBytes;
        uint8_t *remainingData = malloc(first->chunk->size);
        memcpy(remainingData, first->chunk->data + howManyBytes, first->chunk->size);
        free(first->chunk->data);
        first->chunk->data = remainingData;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    return 1;
}

void flushAudioChunkQueue(AudioChunkQueue *queue, int shouldUnlockMutex)
{
    pthread_mutex_lock(&queue->mutex);
    
    AudioChunkList *chunkList = queue->first;
    while (chunkList != NULL) {
        AudioChunkList *next = chunkList->next;

        _freeAudioChunk(&chunkList->chunk);
        free(chunkList);

        chunkList = next;
        
        sem_wait(queue->full);
        sem_post(queue->empty);
    }
    
    queue->first = NULL;
    queue->last = NULL;
    queue->quantity = 0;
    
    if (shouldUnlockMutex) {
        pthread_mutex_unlock(&queue->mutex);
    }
}

void freeAudioChunkQueue(AudioChunkQueue **queue)
{
    flushAudioChunkQueue(*queue, 0);
    
    sem_unlink("/full");
    sem_unlink("/empty");
    
    sem_close((*queue)->full);
    sem_close((*queue)->empty);
    
    pthread_mutex_destroy(&(*queue)->mutex);
    
    free(*queue);
    *queue = NULL;
}

void _freeAudioChunk(AudioChunk **chunk)
{
    free((*chunk)->data);
    free(*chunk);
}

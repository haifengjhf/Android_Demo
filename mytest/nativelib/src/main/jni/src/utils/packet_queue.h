#ifndef packet_queue_h
#define packet_queue_h

#include "libavcodec/avcodec.h"
#include "pthread.h"

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int abort_request;
    int serial;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} PacketQueue;


int packet_queue_put(PacketQueue *q, AVPacket *pkt);

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

void packet_queue_init(PacketQueue *q);

void packet_queue_flush(PacketQueue *q);

void packet_queue_destroy(PacketQueue *q);

void packet_queue_abort(PacketQueue *q);

void packet_queue_start(PacketQueue *q);



#endif
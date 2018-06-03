#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdlib.h>

#define PACKET_SZ 1000

#define EOT       0
#define DATA      1
#define ACK       2
#define NO_RESULT 3
#define TIMEOUT 4

typedef uint8_t MsgMode;

//typedef enum {
//    EOT,
//    DATA,
//    ACK,
//    NO_RESULT,
//    TIMEOUT
//}MsgMode;

/*
 * This struct is used for sending/reading messages through pipes.
 */
typedef struct {
    // Total size of the message, after all the packets have been joined.
    uint32_t message_size;

    // Flag to indicate if there is another packet incoming.
    uint8_t more;

    // Message type.
    MsgMode mode;

    // The actual conentent of the Packet
    char content[PACKET_SZ];

    // Padding to round up to 1kB
    char padding[1024 - PACKET_SZ - 6];
} Packet;

int send_message(int fd, char *snd_buf, size_t message_size, MsgMode mode);
int read_message(int fd, char **rcv_buf, size_t *n, MsgMode *mode);
#endif


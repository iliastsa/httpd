#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>

#include "communication.h"
#include "utils.h"

/*
 * Reads a single Packet from the desired stream.
 *
 * Params:
 * - int fd : The file descriptor of the source we are reading from.
 * - Packet *packet : A pointer to the packet where the read data will be stored in.
 *
 * Returns: -  1 the read was completed sucessfully.
 *          -  0 if EOF was encountered before completing the read.
 *          - -1 if an error occured during the read.
 */
static
char read_packet(int fd, Packet *packet) {
    ssize_t total_read = 0;
    while (total_read < (ssize_t)sizeof(Packet)) {
        ssize_t status = read(fd, packet, sizeof(Packet) - total_read);

        if (status > 0)
            total_read += status;
        else if (status == 0)
            return 0;
        else {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return -1;
            }
        }
    }
    return 1;
}

/*
 * Writes a single Packet to the desired stream.
 *
 * Params:
 * - int fd         : The file descriptor of the destination.
 * - Packet *packet : A pointer to the packet where the read data will be stored in.
 *
 * Returns: -  1 the read was completed sucessfully.
 *          -  0 if no byte could be written for some reason.
 *          - -1 if an error occured during the read.
 */
static
char write_packet(int fd, Packet *packet) {
    ssize_t total_sent = 0;
    while (total_sent < (ssize_t)sizeof(Packet)) {
        ssize_t status = write(fd, packet, sizeof(Packet) - total_sent);

        if (status > 0)
            total_sent += status;
        else if (status == 0)
            return 0;
        else {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return -1;
            }
        }
    }
    return 1;
}

/*
 * Reads a stream of Packets and constructs a message from their payload.
 *
 * Params:
 * - int fd         : The file descriptor of the source we are reading from.
 * - char **rcv_buf : The buffer where the message will be stored in. 
 * - size_t *n      : The size of the message that was read.
 * - MsgMode *mode  : The mode of the message (see header file).
 *
 * Returns: -  1 the read was completed sucessfully.
 *          -  0 if EOF was encountered before completing the read.
 *          - -1 if an error occured during the read.
 */
int read_message(int fd, char **rcv_buf, size_t *n, MsgMode *mode){
    Packet packet;
    char     first_packet = 1;
    uint32_t message_size = 0;
    size_t   offset       = 0;
    char     *buf         = NULL;
    char     more         = 1;

    // While there are more packets coming for this message.
    while (more){
        ssize_t status = read_packet(fd, &packet);

        // Something went wrong...
        if (status < 0) {
            free(buf);
            return -1;
        }

        // EOF.
        if (status == 0) {
            free(buf);
            return 0;
        }

        if (first_packet) {
            message_size = packet.message_size;
            *mode        = packet.mode;
            
            // If the mode is special, do not allocate memory, just return.
            if (packet.mode == EOT       || 
                packet.mode == ACK       || 
                packet.mode == NO_RESULT || 
                packet.mode == TIMEOUT)
            {
                return 1;
            }

            buf = (char*)malloc(message_size);
            first_packet = 0;
        }
        // Calculate remaining message size.
        size_t remaining_msg_sz = message_size - offset;

        // Copy amount is the minimum between the size of each packet's payload and the remaining size.
        size_t copy_amount = remaining_msg_sz < PACKET_SZ ? remaining_msg_sz : PACKET_SZ;

        // Copy the payload into the buffer, and increase the offset accordingly.
        memcpy(buf + offset, packet.content, copy_amount);
        offset += copy_amount;

        // Set more flag accordingly.
        more = packet.more;
    }
    // Return new buffer and message size
    *rcv_buf = buf;
    *n       = message_size;

    return 1;
}

/*
 * Breaks up a buffer into Packet chunks and sends them to the desired destination.
 *
 * Params:
 * - int fd         : The file descriptor of the desination.
 * - char **snd_buf : The buffer where the message we are sending is stored in.
 * - size_t *n      : The size of the message that we are sending.
 * - MsgMode *mode  : The mode of the message (see header file).
 *
 * Returns: -   1 if the write was completed sucessfully.
 *          -  -1 if an error occured during the write.
 */
int send_message(int fd, char *snd_buf, size_t message_size, MsgMode mode){
    // Break up packets into chunks.
    long n_packets = ceil_division(message_size, PACKET_SZ);

    // If the mode is special, ignore snd_buf and size. Send message and return.
    if (mode == EOT || mode == ACK || mode == NO_RESULT || mode == TIMEOUT) {
        Packet packet;

        memset(&packet, 0, sizeof(Packet));

        packet.message_size = 0;
        packet.mode         = mode;
        packet.more         = 0;

        int status = write_packet(fd, &packet);
        if (status <= 0) {
            return -1;
        }

        return 1;
    }

    int offset = 0;
    for (long i = 0; i < n_packets; ++i){
        Packet packet;

        memset(&packet, 0, sizeof(Packet));

        packet.message_size = message_size;
        packet.mode         = mode;

        if (i == n_packets - 1)
            packet.more = 0;
        else
            packet.more = 1;

        size_t remaining_msg_sz = message_size - offset;

        size_t copy_amount = remaining_msg_sz < PACKET_SZ ? remaining_msg_sz : PACKET_SZ;

        memcpy(&packet.content, snd_buf + offset, copy_amount);
        offset += copy_amount;

        int status = write_packet(fd, &packet);
        if (status <= 0) {
            return -1;
        }
    }

    return 1;
}

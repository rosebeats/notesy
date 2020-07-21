#include <stdbool.h>

#include <gtk/gtk.h>
#include <zmq.h>
#include <protobuf-c/protobuf-c.h>

#include "protobuf/threading.pb-c.h"

static void *thread_context;
static void *thread_sock;

static gboolean process_zmq(GIOChannel *source,
                            GIOCondition condition,
                            gpointer data) {
    uint32_t status;
    size_t sizeof_status;

    sizeof_status = sizeof(status);
    while(1) {
        if(zmq_getsockopt(thread_sock, ZMQ_EVENTS, &status, &sizeof_status)) {
            perror("Error retrieving event status");
            exit(1);
        }
        if((status & ZMQ_POLLIN) == 0) {
            break;
        }
        
        // get message from zmq
        zmq_msg_t message;
        zmq_msg_init(&message);
        zmq_msg_recv(&message, thread_sock, 0);
        
        // get size and data from zmq
        size_t msg_size = zmq_msg_size(&message);
        uint8_t *raw = zmq_msg_data(&message);
        
        // unpack message as protobuf and print
        Notesy__Messaging__ServerMsg *contents = notesy__messaging__server_msg__unpack(NULL, msg_size, raw);
        printf("%s\n", contents->test);
        
        //release protobuf, then zmq
        notesy__messaging__server_msg__free_unpacked(contents, NULL);
        zmq_msg_close(&message);
        // message handling
        break;
    }
    return 1;
}

void *thread_comm_init() {
    int zmq_fd;
    size_t sizeof_zmq_fd;
    // create a zmq context and start a inproc socket
    thread_context = zmq_ctx_new();
    thread_sock = zmq_socket(thread_context, ZMQ_PAIR);
    zmq_bind(thread_sock, "inproc://test");
    
    // get the zmq file descriptor and listen with GTK
    sizeof_zmq_fd = sizeof(zmq_fd);
    if(zmq_getsockopt(thread_sock, ZMQ_FD, &zmq_fd, &sizeof_zmq_fd)) {
        perror("Failed to retrieve zmq file descriptor");
        exit(1);
    }
    GIOChannel* zmq_channel = g_io_channel_unix_new(zmq_fd);
    g_io_add_watch(zmq_channel, G_IO_IN|G_IO_ERR|G_IO_HUP, process_zmq, NULL);
    
    return thread_context;
}

void thread_comm_shutdown() {
    // create ClientMsg protobuf and set values
    Notesy__Messaging__ClientMsg contents = NOTESY__MESSAGING__CLIENT_MSG__INIT;
    contents.shutdown = true;
    
    // get the size of the protobuf
    size_t size = notesy__messaging__client_msg__get_packed_size(&contents);
    
    //initialize a zmq message
    zmq_msg_t message;
    zmq_msg_init_size(&message, size);
    
    // pack protobuf into message
    notesy__messaging__client_msg__pack(&contents, zmq_msg_data(&message));
    
    // send message
    zmq_msg_send(&message, thread_sock, 0);
}

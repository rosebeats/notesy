#include <pthread.h>
#include <gtk/gtk.h>
#include <notesy_core/core.h>
#include <zmq.h>
#include <protobuf-c/protobuf-c.h>
#include "protobuf/threading.pb-c.h"
#include <stdbool.h>

// cairo surface to store
static cairo_surface_t *surface = NULL;
static pthread_t compute_thread;
void *computation_conn;

static void clear_surface() {
    cairo_t *cr;
    
    // create a new context from the surface
    cr = cairo_create(surface);
    
    // paint it white
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    
    // clean up the context
    cairo_destroy(cr);
}

static void draw_brush(GtkWidget *widget, gdouble x, gdouble y) {
    cairo_t *cr;
    
    // create context
    cr = cairo_create(surface);
    
    // create a rectangle at the designated location
    cairo_rectangle(cr, x - 3, y - 3, 6, 6);
    cairo_fill(cr);
    
    // clean up the context
    cairo_destroy(cr);
    
    // redraw the area
    gtk_widget_queue_draw_area(widget, x - 3, y - 3, 6, 6);
}

static gboolean configure_editor(GtkWidget *widget,
                                 GdkEventConfigure *event,
                                 gpointer data) {
    cairo_t *cr;

    // destroy the surface
    if(surface) {
        cairo_surface_destroy(surface);
    }
    
    // create a buffer surface from the window
    surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
                                    CAIRO_CONTENT_COLOR,
                                    gtk_widget_get_allocated_width(widget),
                                    gtk_widget_get_allocated_height(widget));
    
    // set surface to white
    clear_surface();
    
    return TRUE;
}

static gboolean draw_editor(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    
    return FALSE;
}

static gboolean editor_button_press(GtkWidget *widget,
                                    GdkEventButton *event,
                                    gpointer data) {
    // sanity check
    if(surface == NULL) {
        return FALSE;
    }
    
    // start drawing
    if(event->button == GDK_BUTTON_PRIMARY) {
        draw_brush(widget, event->x, event->y);
    // clear canvas
    } else if(event->button == GDK_BUTTON_SECONDARY) {
        clear_surface();
        // redraw the widget since there has been a change
        gtk_widget_queue_draw(widget);
    }
}

static gboolean editor_motion(GtkWidget *widget,
                              GdkEventMotion *event,
                              gpointer data) {
    // sanity check
    if(surface == NULL) {
        return FALSE;
    }
    
    if(event->state & GDK_BUTTON1_MASK) {
        draw_brush(widget, event->x, event->y);
    }
    
    return TRUE;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *editor;

    // create main window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Notesy");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    
    // create the editor window and connect events
    editor = gtk_drawing_area_new();
    
    // drawing events
    g_signal_connect(editor, "configure-event",
                     G_CALLBACK(configure_editor), NULL);
    g_signal_connect(G_OBJECT(editor), "draw",
                     G_CALLBACK(draw_editor), NULL);
    
    // input
    g_signal_connect(editor, "motion-notify-event",
                     G_CALLBACK(editor_motion), NULL);
    g_signal_connect(editor, "button-press-event",
                     G_CALLBACK(editor_button_press), NULL);
    
    // put editor inside window
    gtk_container_add(GTK_CONTAINER(window), editor);
    
    // allow recieving of mouse and keyboard events
    gtk_widget_set_events(editor, gtk_widget_get_events(editor)
                                | GDK_BUTTON_PRESS_MASK
                                | GDK_POINTER_MOTION_MASK);
    
    gtk_widget_show_all(window);
}

static gboolean process_zmq(GIOChannel *source,
                            GIOCondition condition,
                            gpointer data) {
    uint32_t status;
    size_t sizeof_status = sizeof(status);
    
    while(1) {
        if(zmq_getsockopt(computation_conn, ZMQ_EVENTS, &status, &sizeof_status)) {
            perror("Error retrieving event status");
            exit(1);
        }
        if((status & ZMQ_POLLIN) == 0) {
            break;
        }
        
        // get message from zmq
        zmq_msg_t message;
        zmq_msg_init(&message);
        zmq_msg_recv(&message, computation_conn, 0);
        
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

int main(int argc, char *argv[]) {
    // main app and window
    GtkApplication *app;
    GtkWidget *window;
    // exit status of GTK
    int status;
    
    // create a zmq context and start a inproc socket
    void *zmq_context = zmq_ctx_new();
    computation_conn = zmq_socket(zmq_context, ZMQ_PAIR);
    zmq_bind(computation_conn, "inproc://test");
    
    // start computational thread
    if(pthread_create(&compute_thread, NULL, test_thread, zmq_context)) {
        fprintf(stderr, "Error spawning computational thread, aborting.\n");
        exit(1);
    }
    
    // create application
    app = gtk_application_new("notesy.base", G_APPLICATION_FLAGS_NONE);
    // connect startup event
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    // get the zmq file descriptor and listen with GTK
    int zmq_fd;
    size_t sizeof_zmq_fd = sizeof(zmq_fd);
    if(zmq_getsockopt(computation_conn, ZMQ_FD, &zmq_fd, &sizeof_zmq_fd)) {
        fprintf(stderr, "Failed to retrieve zmq file descriptor");
        exit(1);
    }
    GIOChannel* zmq_channel = g_io_channel_unix_new(zmq_fd);
    g_io_add_watch(zmq_channel, G_IO_IN|G_IO_ERR|G_IO_HUP, process_zmq, NULL);
    
    // run the app
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // garbage collect
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
    zmq_msg_send(&message, computation_conn, 0);
    
    g_object_unref(app);
    
    return status;
}

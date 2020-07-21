#include <pthread.h>

#include <gtk/gtk.h>

#include <notesy_core/core.h>

#include "thread_comm.h"

// cairo surface to store
static cairo_surface_t *surface = NULL;

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

int main(int argc, char *argv[]) {
    // main app and window
    GtkApplication *app;
    GtkWidget *window;
    // exit status of GTK
    int status;
    // the background thread
    pthread_t compute_thread;
    // the thread context
    void *thread_context;
    
    // create application
    app = gtk_application_new("notesy.base", G_APPLICATION_FLAGS_NONE);
    // connect startup event
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    // initialize thread communication
    thread_context = thread_comm_init();
    
    // start computational thread
    if(pthread_create(&compute_thread, NULL, test_thread, thread_context)) {
        fprintf(stderr, "Error spawning computational thread, aborting.\n");
        exit(1);
    }
    
    // run the app
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // garbage collect
    // shutdown background thread
    thread_comm_shutdown();
    // clean up app
    g_object_unref(app);
    
    return status;
}

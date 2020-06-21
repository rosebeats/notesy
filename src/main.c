#include <gtk/gtk.h>

static void draw_editor(GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width, height;
    GdkRGBA color;
    GtkStyleContext *context;
    
    context = gtk_widget_get_style_context(widget);
    
    width = gtk_widget_get_allocated_width(widget);
    height = gtk_widget_get_allocated_height(widget);
    
    gtk_render_background(context, cr, 0, 0, width, height);
    
    cairo_arc(cr,
              width / 2.0, height / 2.0,
              MIN(width, height) / 2.0,
              0, 2 * G_PI);
    
    gtk_style_context_get_color(context,
                                gtk_style_context_get_state(context),
                                &color);
    gdk_cairo_set_source_rgba(cr, &color);
    
    cairo_fill(cr);
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *editor;

    // create main window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Notesy");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    
    // create the editor window
    editor = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(editor), "draw",
                     G_CALLBACK(draw_editor), NULL);
    gtk_container_add(GTK_CONTAINER(window), editor);
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    GtkWidget *window;
    int status;
    
    // create application
    app = gtk_application_new("notesy.base", G_APPLICATION_FLAGS_NONE);
    // connect startup event
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    // run the app
    status = g_application_run(G_APPLICATION(app), argc, argv);
    // garbage collect
    g_object_unref(app);
    
    return status;
}

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

/* Structure to hold pointers to our main widgets */
typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *entry;
} AppWidgets;

/* Create a new tab with a WebKitWebView loading the given URL */
static void new_tab(AppWidgets *app, const gchar *url) {
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    webkit_web_view_load_uri(web_view, url ? url : "https://google.com");

    /* Wrap the web view in a scrolled window */
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(web_view));
    gtk_widget_show_all(scrolled);

    /* Create a simple label for the tab title */
    GtkWidget *label = gtk_label_new("New Tab");
    gint page_num = gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), scrolled, label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook), page_num);
}

/* Callback for the "New Tab" button */
static void on_new_tab_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = user_data;
    new_tab(app, "https://google.com");
}

/* Callback for the "Back" button */
static void on_back_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = user_data;
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current_page);
    if (child) {
        WebKitWebView *web_view = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(child)));
        if (webkit_web_view_can_go_back(web_view))
            webkit_web_view_go_back(web_view);
    }
}

/* Callback for the "Forward" button */
static void on_forward_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = user_data;
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current_page);
    if (child) {
        WebKitWebView *web_view = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(child)));
        if (webkit_web_view_can_go_forward(web_view))
            webkit_web_view_go_forward(web_view);
    }
}

/* Callback for the "Reload" button */
static void on_reload_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = user_data;
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current_page);
    if (child) {
        WebKitWebView *web_view = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(child)));
        webkit_web_view_reload(web_view);
    }
}

/* Callback for the URL entry: load the URL when Enter is pressed */
static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    AppWidgets *app = user_data;
    const gchar *url = gtk_entry_get_text(entry);
    if (!g_str_has_prefix(url, "http://") && !g_str_has_prefix(url, "https://"))
        url = g_strdup_printf("http://%s", url);

    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current_page);
    if (child) {
        WebKitWebView *web_view = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(child)));
        webkit_web_view_load_uri(web_view, url);
    }
}

/* Activate callback: sets up the window, toolbar, and notebook */
static void activate(GtkApplication *app, gpointer user_data) {
    AppWidgets *widgets = g_malloc(sizeof(AppWidgets));
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "GTK Browser");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 1200, 800);

    /* Vertical box to hold toolbar and notebook */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(widgets->window), vbox);

    /* Create a horizontal box for the navigation toolbar */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *back_button = gtk_button_new_with_label("Back");
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(toolbar), back_button, FALSE, FALSE, 0);

    GtkWidget *forward_button = gtk_button_new_with_label("Forward");
    g_signal_connect(forward_button, "clicked", G_CALLBACK(on_forward_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(toolbar), forward_button, FALSE, FALSE, 0);

    GtkWidget *reload_button = gtk_button_new_with_label("Reload");
    g_signal_connect(reload_button, "clicked", G_CALLBACK(on_reload_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(toolbar), reload_button, FALSE, FALSE, 0);

    GtkWidget *new_tab_button = gtk_button_new_with_label("New Tab");
    g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), widgets);
    gtk_box_pack_start(GTK_BOX(toolbar), new_tab_button, FALSE, FALSE, 0);

    /* URL entry */
    widgets->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->entry), "Enter URL");
    g_signal_connect(widgets->entry, "activate", G_CALLBACK(on_entry_activate), widgets);
    gtk_box_pack_start(GTK_BOX(toolbar), widgets->entry, TRUE, TRUE, 0);

    /* Create a notebook for tabbed browsing */
    widgets->notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), widgets->notebook, TRUE, TRUE, 0);

    /* Create the initial tab */
    new_tab(widgets, "https://google.com");

    /* Apply dark mode styling using a CSS provider */
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window { background-color: #353535; color: #ffffff; }"
        "button { background-color: #454545; color: #ffffff; }"
        "entry { background-color: #252525; color: #ffffff; }"
        "notebook, scrolledwindow { background-color: #353535; color: #ffffff; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                               GTK_STYLE_PROVIDER(css_provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(css_provider);

    gtk_widget_show_all(widgets->window);
}

int main (int argc, char **argv) {
    GtkApplication *app;
    int status;
    app = gtk_application_new("org.example.GTKBrowser", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}


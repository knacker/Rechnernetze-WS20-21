/*
 * Brandenburg University of Technology Cottbus - Senftenberg (BTU)
 * Department of Computer Science, Information and Media Technology
 * Computer Networks and Communication Systems Group
 *
 * Practical course "Introduction to Computer Networks"
 * Task 1: Implementation of a Simple SMTP Client
 *
 * @author: Sebastian Boehm <sebastian.boehm@b-tu.de>
 * @version: 2020-10-27
 *
 */

#include "base64/base64.h"

#include <stdio.h>

#include <gtk/gtk.h>

/** gtk app window widgets */
typedef struct {
    GtkWidget *w_entry_from;
    GtkWidget *w_entry_to;
    GtkWidget *w_entry_cc;
    GtkWidget *w_entry_bcc;
    GtkWidget *w_entry_subject;
    GtkWidget *w_entry_server;
    GtkWidget *w_entry_user;
    GtkWidget *w_entry_pwd;
    GtkWidget *w_txt_msg;
    GtkWidget *w_chk_verbose;
} app_widgets;

/** user authentication data */
typedef struct {
    char *server;
    char *user;
    char *pwd;
} server_auth;

/** email content data */
typedef struct {
    char *from;
    char *to;
    char *cc;
    char *bcc;
    char *subject;
    char *msg;
    int verbose;
} mail_content;

/** flag indicating verbose */
static int verbose = 0;


/**
 * Sets up a new SMTP server connection ... (needs to be implemented)
 *
 * @param auth points to the user authentication data
 */
void
connect_server(server_auth *auth)
{
    printf("\n-----------------------------\n");
    printf("Server   %s\n", auth->server);
    printf("User     %s\n", auth->user);
    printf("Password %s\n", "***********");
}

/**
 * Sends an email ... (needs to be implemented)
 *
 * @param mail points to the email content data
 */
void
send_mail(mail_content *mail)
{
    printf("-----------------------------\n");
    printf("(Verbose mode is %s)\n", mail->verbose ?  "enabled" : "disabled");
    printf("From     %s\n", mail->from);
    printf("To       %s\n", mail->to);
    printf("Cc       %s\n", mail->cc);
    printf("Bcc      %s\n", mail->bcc);
    printf("Subject  %s\n", mail->subject);
    printf("Message \n\n%s\n\n", mail->msg);	
    char *encoded = base64_encode(mail->subject);
    printf("(Example) base64-encoded Subject: %s\n", encoded);
    printf("(Example) base64-decoded Subject: %s\n", base64_decode(encoded));
}

/**
 * Main
 *
 * Creates all GUI components and starts the application
 */
int
main(int argc, char *argv[])
{
    GtkBuilder *builder;
    GtkWidget *window;

    // allocating memory for app widgets
    app_widgets *widgets = g_slice_new(app_widgets);

    gtk_init(&argc, &argv);

    builder = gtk_builder_new_from_file ("glade/window_main.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));

    // get pointers to entry widgets
    widgets->w_entry_from  = GTK_WIDGET(gtk_builder_get_object(builder, "entry_from"));
    widgets->w_entry_to = GTK_WIDGET(gtk_builder_get_object(builder, "entry_to"));
    widgets->w_entry_cc = GTK_WIDGET(gtk_builder_get_object(builder, "entry_cc"));
    widgets->w_entry_bcc = GTK_WIDGET(gtk_builder_get_object(builder, "entry_bcc"));
    widgets->w_entry_subject = GTK_WIDGET(gtk_builder_get_object(builder, "entry_subject"));
    widgets->w_entry_server = GTK_WIDGET(gtk_builder_get_object(builder, "entry_server"));
    widgets->w_entry_user = GTK_WIDGET(gtk_builder_get_object(builder, "entry_user"));
    widgets->w_entry_pwd = GTK_WIDGET(gtk_builder_get_object(builder, "entry_pwd"));
    widgets->w_txt_msg = GTK_WIDGET(gtk_builder_get_object(builder, "txt_msg"));
    widgets->w_chk_verbose = GTK_WIDGET(gtk_builder_get_object(builder, "chk_verbose"));

    // widgets pointer will be passed to all widget handler functions
    gtk_builder_connect_signals(builder, widgets);
    g_object_unref(builder);

    // start window main thread
    gtk_widget_show(window);
    gtk_main();

    // free memory
    g_slice_free(app_widgets, widgets);

    return 0;
}

/**
 * Callback for CheckButton toggled.
 *
 * Sets the verbose indication flag.
 *
 * @param togglebutton points to the ToggleButton window widget
 */
void
on_chk_verbose_toggled(GtkToggleButton *togglebutton)
{
    verbose = gtk_toggle_button_get_active(togglebutton)?1:0;
}

/**
 * Callback for SendButton clicked.
 *
 * Gets the authentication and mail content data from window widgets
 * and calls the corresponding functions for sending the email to the server.
 *
 * @param button points to the SendButton window widget
 * @param app_wdgts points to the app window widgets
 */
void
on_btn_send_clicked(GtkButton *button, app_widgets *app_wdgts)
{
    server_auth *auth = (server_auth *) malloc(sizeof(server_auth));
    mail_content *mail = (mail_content *) malloc(sizeof(mail_content));

    // get server authentication strings
    auth->server = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_server));
    auth->user = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_user));
    auth->pwd = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_pwd));

    connect_server(auth);

    // get mail content strings
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer((GtkTextView *)app_wdgts->w_txt_msg);
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    mail->from = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_from));
    mail->to = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_to));
    mail->cc = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_cc));
    mail->bcc = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_bcc));
    mail->subject = (char *)gtk_entry_get_text(GTK_ENTRY(app_wdgts->w_entry_subject));
    mail->msg = (char *)gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    mail->verbose = verbose;

    send_mail(mail);

    free(auth);
    free(mail);
}

/**
 * Callback for closing the main window.
 *
 * Quits the application.
 */
void
on_window_main_destroy()
{
    gtk_main_quit();
}

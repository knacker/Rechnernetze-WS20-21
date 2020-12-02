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
#include <stdlib.h>

#include <gtk/gtk.h>


#include <sys/types.h>
#include <bits/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SMTP_PORT 25
#define STD_SERVER "romeo.informatik.tu-cottbus.de\0"
#define STD_USER "schusvin\0"
#define STD_MAIL_FROM "schusvin@b-tu.de\0"
#define STD_MAIL_TO "raspemax@b-tu.de\0"
#define IPv4 AF_INET
#define TCP SOCK_STREAM
#define BUFFERSIZE 1024

int client_socket;

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

char* concat_A_with_B(char* a, char* b) {
    const char* a_in = a;
    const char* b_in = b;
    char* output;
    output = malloc(strlen(a)+strlen(b)+1);
    strcpy(output,a_in);
    strcat(output,b_in);

    return output;
}

void smtp_send_message(const char* msg) {
    size_t size = strlen(msg);
    size_t new_size = size + 2;
    char* message_send = malloc(new_size);
    memcpy(message_send,msg,size);
    message_send[size] = '\r';
    message_send[size + 1] = '\n';

    int datasend;

    if (verbose) {
        printf("     ");

        for (int i = 0; i < size; i++) {
            if (msg[i] == '\n') {
                printf("\n     ");
            }else {
                printf("%c", msg[i]);
            }
        }
        printf("\n");
    }

    if ((datasend = send(client_socket, message_send, new_size, 0)) < 0) {
        perror("error at send");
    }
}

char* smtp_receive_message() {
    char buf [BUFFERSIZE];

    int datarecv;

    if ((datarecv = recv(client_socket, buf, BUFFERSIZE, 0)) < 0) {
        perror("error at recv");
        return 0;
    }

    char* retBuffer = malloc(datarecv+1);
    memcpy(retBuffer, buf, datarecv);

    retBuffer[datarecv] = '\0';

    return retBuffer;
}

/**
 * Sets up a new SMTP server connection ... (needs to be implemented)
 *
 * @param auth points to the user authentication data
 */
void
connect_server(server_auth *auth)
{
    // Socket in Zustand active wird erstellt
    client_socket = socket(IPv4, TCP, 0);
    
    if (client_socket < 0) {
        // Fehlercheck
        perror("error at socket creation");
    }

    // Standardwerte beim Auslassen einsetzen
    if (strlen(auth->server) == 0) {
        // falls kein Servername angegeben -> romeo.informatik... verwenden
        auth->server = STD_SERVER;
    }
    if (strlen(auth->user) == 0) {
        auth->user = STD_USER;
    }

    const char* const_server_name = auth->server;

    struct sockaddr_in server_addr;

    // Vor Verwendung mit Nullen fuellen
    memset(&server_addr, 0, sizeof(server_addr));

    // Adresse des Servers setzen
    struct hostent* host = gethostbyname(const_server_name);

    if (host == NULL) {
        perror("no host found");
    }

    // Internetnumber zu ASCII konvertieren 
    char* ipBuffer = malloc(BUFFERSIZE);
    ipBuffer = inet_ntoa(*((struct in_addr*)host->h_addr_list[0]));

    // konvertiere IP-Adresse in numerischen Wert
    inet_aton(ipBuffer, &server_addr.sin_addr);

     // Port des Server auf SMTP-Port setzen
    server_addr.sin_port = htons(SMTP_PORT);

    // Typ auf IPv4 setzen
    server_addr.sin_family = IPv4;

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("error at connect_client");
    }

    // Server begruesst Client
    printf("%s", smtp_receive_message());

    // Client meldet sich an, Server bestaetigt Anmeldung
    smtp_send_message(concat_A_with_B("HELO ",auth->user));
    printf("%s", smtp_receive_message());

    // Benutzernamen und Passwort anfordern
    smtp_send_message("AUTH LOGIN");
    printf("%s", smtp_receive_message());

    // Name und Passwort base64 kodieren
    char* user_encode = base64_encode(auth->user);
    char* pwd_encode = base64_encode(auth->pwd);
    
    // Benutzernamen schicken
    smtp_send_message(user_encode);
    printf("%s", smtp_receive_message());

    // Passwort schicken
    smtp_send_message(pwd_encode);
    printf("%s", smtp_receive_message());

    /** Outputs
    printf("\n\n-----------------------------\n");
    printf("Server   %s\n", auth->server);
    printf("User     %s\n", auth->user);
    printf("Password %s\n\n", "***********");
    */
}

/**
 * Sends an email ... (needs to be implemented)
 *
 * @param mail points to the email content data
 */
void
send_mail(mail_content *mail)
{   
    if (strlen(mail->from) == 0) {
        mail->from = STD_MAIL_FROM;
    }
    if (strlen(mail->to) == 0) {
        mail->to = STD_MAIL_TO;
    }
   
    //Client gibt Absenderadresse an
    smtp_send_message(concat_A_with_B("MAIL FROM: ", mail->from));
    printf("%s",smtp_receive_message());
    
    //Empfängeradresse ankündigen
    smtp_send_message(concat_A_with_B("RCPT TO: ", mail->to));
    printf("%s", smtp_receive_message());
    
    //Datentransfer ankündigen
    smtp_send_message("DATA");
    printf("%s", smtp_receive_message());

    // Client uebermittelt E-Mail
    smtp_send_message(concat_A_with_B("From: ",mail->from));
    smtp_send_message(concat_A_with_B("To: ",mail->to));
    smtp_send_message(concat_A_with_B("Subject: ", mail->subject));

    if (strlen(mail->cc) > 0) {
        smtp_send_message(concat_A_with_B("CC: ",mail->cc));
    }
    if (strlen(mail->bcc) > 0) {
        smtp_send_message(concat_A_with_B("BCC: ", mail->bcc));
    }

    smtp_send_message(concat_A_with_B(mail->msg, "\r\n."));
    printf("%s",smtp_receive_message());

    // Client meldet sich ab
    smtp_send_message("QUIT");
    printf("%s",smtp_receive_message());
    close(client_socket);






    /** Outputs    
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
    */
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

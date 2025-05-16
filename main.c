#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <string.h>

#include <errno.h>

#include "globmatch.h"
#include "tinyosc.h"
#include "osc_config.h"

typedef struct {
    int fd;
    struct sockaddr sa; 
    socklen_t sa_len;
} conT;

typedef struct connectionT {
    conT   con;
    size_t (*send)(struct connectionT *, const void *, size_t);
    size_t (*receive)(struct connectionT *, const void *, size_t);
} connectionT;

typedef int (OscHandler)(tosc_message *, connectionT *);
typedef uint32_t (GenericGetter)(char *, int);
typedef void (GenericSetterDouble)(double);
typedef uint32_t (SendGetter)(char *, int, int);
typedef void     (SendSetterDouble)(int, double);

typedef struct {
    char *address_match;
    char *format;
    // For special purpose addresses
    OscHandler *getter;
    OscHandler *setter;
    // For generic/double addresses
    GenericGetter *generic_getter;
    GenericSetterDouble *generic_setter_double;
    // For "/send/[1-4]/..." generic/double
    SendGetter *send_getter;
    SendSetterDouble *send_setter_double;
} bioscc_handlerT;

#define OSC_BUFFER_SIZE 1024
char OSC_BUFFER[OSC_BUFFER_SIZE];


void send_error_message(connectionT *conn, char *text) {
    int len;
    len = tosc_writeMessage(OSC_BUFFER, OSC_BUFFER_SIZE, "/error", "s", text);
    conn->send(conn, OSC_BUFFER, len);
}

int ack(tosc_message *message, connectionT *conn) {
    int len;
    len = tosc_writeMessage(OSC_BUFFER, OSC_BUFFER_SIZE, "/ack", "", NULL);
    conn->send(conn, OSC_BUFFER, len);
    return(0);
}

int generic_getter_wrapper(tosc_message *m, connectionT *c, GenericGetter *getter) {
    int len = getter(OSC_BUFFER, OSC_BUFFER_SIZE);
    return(c->send(c, OSC_BUFFER, len));
}

int generic_setter_double_wrapper(tosc_message *m, connectionT *c, GenericSetterDouble *setter) {
    // TODO: Range checking?
    float v = tosc_getNextFloat(m);
    setter(v);
    return(0);
}

int message_to_send_number(tosc_message *m, connectionT *conn) {
    // "/send/X"
    //  0123456
    int d = m->buffer[6] - '1';
    if ((d < 0) || (d > 3)) {
        send_error_message(conn, "invalid send number");
        return (-1);
    }
    return(d);
}

int send_input_set_wrapper(tosc_message *m, connectionT *c) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return(send_num);

    int input_num = tosc_getNextInt32(m);
    input_num -= 1;
    if ((input_num < 0) || (input_num > 3)) {
        send_error_message(c, "Invalid input number [1..4]");
        return(-1);
    }
    set_send_input(send_num, input_num);
    return(0);
}

int send_getter_wrapper(tosc_message *m, connectionT *c, SendGetter *getter) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return(send_num);

    int len = getter(OSC_BUFFER, OSC_BUFFER_SIZE, send_num);
    return(c->send(c, OSC_BUFFER, len));
}

int send_setter_double_wrapper(tosc_message *m, connectionT *c, SendSetterDouble *setter) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return(send_num);

    float v = tosc_getNextFloat(m);
    setter(send_num, v);
    return(0);
}


bioscc_handlerT handlers[] = {
/*
**   addr,  type, get, set, generic_get, generic_set, send_get, send_set
*/
    {"/ack", "", ack, ack, NULL, NULL, NULL, NULL,},
        // TODO: special setter for string resolution
    {"/analog_format/resolution", "s", NULL, NULL, get_analog_format_resolution, NULL,  NULL, NULL},
    {"/analog_format/framerate", "s", NULL, NULL, get_analog_format_framerate, set_analog_format_framerate,  NULL, NULL},
        // TODO: special setter for string colorspace 
    {"/analog_format/colourspace", "s", NULL, NULL, get_analog_format_colourspace, NULL,  NULL, NULL},
    {"/analog_format/colorspace", "s", NULL, NULL, get_analog_format_colourspace, NULL,  NULL, NULL},
    {"/clock_offset", "f", NULL, NULL, get_clock_offset, set_clock_offset,  NULL, NULL},
    {"/send/[1-4]/input", "i", NULL, send_input_set_wrapper, NULL, NULL, get_send_input, NULL}, 
    {"/send/[1-4]/scaleX", "f", NULL, NULL, NULL, NULL, get_send_scaleX, set_send_scaleX},
    {"/send/[1-4]/scaleY", "f", NULL, NULL, NULL, NULL, get_send_scaleY, set_send_scaleY},
    {"/send/[1-4]/posX", "f", NULL, NULL, NULL, NULL, get_send_posX, set_send_posX},
    {"/send/[1-4]/posY", "f", NULL, NULL, NULL, NULL, get_send_posY, set_send_posY},
    {"/send/[1-4]/rotation", "f", NULL, NULL, NULL, NULL, get_send_rotation, set_send_rotation},
    {"/send/[1-4]/pitch", "f", NULL, NULL, NULL, NULL, get_send_pitch, set_send_pitch},
    {"/send/[1-4]/yaw", "f", NULL, NULL, NULL, NULL, get_send_yaw, set_send_yaw},
    {"/send/[1-4]/brightness", "f", NULL, NULL, NULL, NULL, get_send_brightness, set_send_brightness},
    {"/send/[1-4]/contrast", "f", NULL, NULL, NULL, NULL, get_send_contrast, set_send_contrast},
    {"/send/[1-4]/saturation", "f", NULL, NULL, NULL, NULL, get_send_saturation, set_send_saturation},
    {"/send/[1-4]/hue", "f", NULL, NULL, NULL, NULL, get_send_hue, set_send_hue},
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


void dispatch_message (tosc_message *osc, connectionT *conn) {
    bioscc_handlerT *h;
    int i = 0;

    h = &handlers[i];
    while (h->address_match) {
        // osc->buffer points to the address which is \0 terminated
        if (globmatch(osc->buffer, h->address_match)) {
            if (osc->format[0] == '\0') {
                // no format string, means get
                if (h->getter) {
                    // If a getter is specified call that
                    h->getter(osc, conn);
                } else 
                if (h->generic_getter) {
                    generic_getter_wrapper(osc, conn, h->generic_getter);
                } else
                if (h->send_getter) {
                    // If a specific getter for "/send/..." is available, use that
                    send_getter_wrapper(osc, conn, h->send_getter);
                } else {
                    send_error_message(conn, "no getter");
                }
            } else {
                if (!strcmp(h->format, osc->format)) {
                    // This will set the config, but
                    // TODO: cache actions to be performed during vsync
                    if (h->setter) {
                        h->setter(osc, conn); 
                    } else 
                    if (h->generic_setter_double) {
                        generic_setter_double_wrapper(osc, conn, h->generic_setter_double);    
                    } else
                    if (h->send_setter_double) {
                        send_setter_double_wrapper(osc, conn, h->send_setter_double);
                    } else {
                        send_error_message(conn, "no setter");
                    }
                } else {
                    send_error_message(conn, "format mismatch");
                }
            }
            break;
        }
        h = &handlers[++i];
    }

    if (!h->address_match) {
        send_error_message(conn, "invalid address");
    }
}

size_t send_wrapper(connectionT *conn, const void *buf, size_t len) {
    int i;
    char *buffer = (char *) buf;
    printf ("SENDING: ");
    for (i=0; i<len; i++) if (isprint(buffer[i])) printf ("%c", buffer[i]); else  printf ("(%02X)", buffer[i]);
    printf ("\n");

    conn->con.sa_len = sizeof(conn->con.sa);

    errno = 0;
    i =  sendto(conn->con.fd, buf, len, 0, &conn->con.sa, conn->con.sa_len);
    if (errno) {
        perror("sending");
    }

    return (i);
}


bool keepRunning = true;

static void sigintHandler(int x) {
  keepRunning = false;
}


int main(int argc, char *argv[]) {
  char buffer[2048];
  connectionT conn;
    
  conn.send = send_wrapper;

  // register the SIGINT handler (Ctrl+C)
  signal(SIGINT, &sigintHandler);

  // open a socket to listen for datagrams (i.e. UDP packets) on port 9000
  conn.con.fd = socket(AF_INET, SOCK_DGRAM, 0);
  fcntl(conn.con.fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking

  struct sockaddr_in sin;

  sin.sin_family = AF_INET;
  sin.sin_port = htons(9000);
  sin.sin_addr.s_addr = INADDR_ANY;
  bind(conn.con.fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));

  printf("tinyosc is now listening on port 9000.\n");
  printf("Press Ctrl+C to stop.\n");

  while (keepRunning) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(conn.con.fd, &readSet);
    struct timeval timeout = {1, 0}; // select times out after 1 second
    if (select(conn.con.fd+1, &readSet, NULL, NULL, &timeout) > 0) {
      int len = 0;
      while ((len = (int) recvfrom(conn.con.fd, buffer, sizeof(buffer), 0, &conn.con.sa, &conn.con.sa_len)) > 0) {
        printf ("RECEIVED [%s]\n", buffer);
        if (tosc_isBundle(buffer)) {
          tosc_bundle bundle;
          tosc_parseBundle(&bundle, buffer, len);
          const uint64_t timetag = tosc_getTimetag(&bundle);
          printf ("Timetag: %llu\n", timetag);
          tosc_message osc;
          while (tosc_getNextMessage(&bundle, &osc)) {
            dispatch_message(&osc, &conn);
          }
        } else {
          tosc_message osc;
          tosc_parseMessage(&osc, buffer, len);
          dispatch_message(&osc, &conn);
        }
      }
    }
  }

  close(conn.con.fd);

  return 0;
}

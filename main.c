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

// connection wrapper

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

// basic handler types

typedef int  (OscHandler)(tosc_message *, connectionT *);
typedef uint32_t (GenericGetter)(char *, int);
typedef void     (GenericSetterDouble)(float);
typedef void     (GenericSetterString)(const char *);
typedef void     (GenericSetterInt)(int);
typedef uint32_t (SendGetter)(char *, int, int);
typedef void     (SendSetterDouble)(int, float);


// enum for argument dispatch

typedef enum {
    OSC_ARG_NONE,       // raw handlers only
    OSC_ARG_INT,        // integer setter/getter
    OSC_ARG_FLOAT,      // generic float get/set
    OSC_ARG_STRING,     // generic string get/set
    OSC_ARG_SEND_FLOAT, // send-indexed float get/set
    OSC_ARG_LUT         // LUTs
} osc_argumentE;

// unified handler struct

typedef struct {
    char                 *address_match;
    const char           *format;
    // raw OSC handlers for special cases
    OscHandler           *raw_getter;
    OscHandler           *raw_setter;
    // which argument form
    osc_argumentE        arg_type;
    // union for generic vs send callbacks
    union {
        struct {
            GenericGetter       *get;
            GenericSetterInt    *set; 
        } integer;
        struct {
            GenericGetter       *get;
            GenericSetterDouble *set;
        } generic;
        struct {
            GenericGetter       *get;
            GenericSetterString *set;
        } string;
        struct {
            SendGetter          *get;
            SendSetterDouble    *set;
        } send;
    } handler;
} osc_handlerT;

#define OSC_BUFFER_SIZE 1024
char OSC_BUFFER[OSC_BUFFER_SIZE];

// error reply

void send_error_message(connectionT *conn, char *text) {
    int len = tosc_writeMessage(OSC_BUFFER, OSC_BUFFER_SIZE, "/error", "s", text);
    conn->send(conn, OSC_BUFFER, len);
}

// special ack

int ack(tosc_message *message, connectionT *conn) {
    int len = tosc_writeMessage(OSC_BUFFER, OSC_BUFFER_SIZE, "/ack", "", NULL);
    conn->send(conn, OSC_BUFFER, len);
    return 0;
}

// wrappers

int generic_getter_wrapper(tosc_message *m, connectionT *c, GenericGetter *getter) {
    int len = getter(OSC_BUFFER, OSC_BUFFER_SIZE);
    return c->send(c, OSC_BUFFER, len);
}

int generic_setter_float_wrapper(tosc_message *m, connectionT *c, GenericSetterDouble *setter) {
    float v = tosc_getNextFloat(m);
    setter(v);
    return 0;
}

int generic_setter_string_wrapper(tosc_message *m, connectionT *c, GenericSetterString *setter) {
    const char *s = tosc_getNextString(m);
    setter(s);
    return 0;
}

int message_to_send_number(tosc_message *m, connectionT *conn) {
    // /send/x/...
    //       ^
    // 0123456

    int d = m->buffer[6] - '1';
    if ((d < 0) || (d > 3)) {
        send_error_message(conn, "Invalid send number");
        return -1;
    }
    return d;
}

char message_to_lut_channel(tosc_message *m, connectionT *conn) {
    // /send/x/lut/c
    //             ^ 
    // 0123456789ABC

    char c = m->buffer[0xC];
    if ((c!='Y') || (c!='R') || (c!='G') || (c!='B')) {
        send_error_message(conn, "Invalid LUT channel");
        return (-1);
    }
    return (c);
}

int message_to_matrix_element(tosc_message *m, connectionT *conn, int *row, int *col) {
    // /analog_format/color_matrix/r_c
    // 0123456789ABCDEF0123456789ABCDE
    //                             ^ ^

    int r, c;

    r = m->buffer[0x1C] - '0';
    c = m->buffer[0x1E] - '0';

    if ((r < 0) || (r > 2) || (c < 0) || (c > 2)) {
        send_error_message(conn, "Matrix index out of bounds");
        return (-1);
    } 

    *row = r;
    *col = c;

    return (0);
}


int send_input_set_wrapper(tosc_message *m, connectionT *c) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return send_num;

    int input_num = tosc_getNextInt32(m) - 1;
    if ((input_num < 0) || (input_num > 3)) {
        send_error_message(c, "Invalid input number [1..4]");
        return -1;
    }
    set_send_input(send_num, input_num);
    return 0;
}

int send_getter_wrapper(tosc_message *m, connectionT *c, SendGetter *getter) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return send_num;

    int len = getter(OSC_BUFFER, OSC_BUFFER_SIZE, send_num);
    return c->send(c, OSC_BUFFER, len);
}

int send_setter_float_wrapper(tosc_message *m, connectionT *c, SendSetterDouble *setter) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return send_num;

    float v = tosc_getNextFloat(m);
    setter(send_num, v);
    return 0;
}

int send_lut_get_wrapper(tosc_message *m, connectionT *c) {
    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return send_num;

    char channel = message_to_lut_channel(m, c);
    if (channel < 0) return (-1);
    
    switch(channel) {
        case 'Y': return(get_send_lut_Y(OSC_BUFFER, OSC_BUFFER_SIZE, send_num));
        case 'R': return(get_send_lut_R(OSC_BUFFER, OSC_BUFFER_SIZE, send_num));
        case 'G': return(get_send_lut_G(OSC_BUFFER, OSC_BUFFER_SIZE, send_num));
        case 'B': return(get_send_lut_B(OSC_BUFFER, OSC_BUFFER_SIZE, send_num));
    }

    return(-1); 
}

int send_lut_set_wrapper(tosc_message *m, connectionT *c) {
    float v[32];
    int i;

    int send_num = message_to_send_number(m, c);
    if (send_num < 0) return send_num;

    char channel = message_to_lut_channel(m, c);
    if (channel < 0) return (-1);

    // TODO: What if we only send <32 floats??
    // Need to make tosc_getNext aware of where o->marker is in relation to message length.

    for (i=0; i<32; i++) {
        v[i] = tosc_getNextFloat(m);
    }
    
    switch(channel) {
        case 'Y': set_send_lut_Y(send_num, v); break;
        case 'R': set_send_lut_R(send_num, v); break;
        case 'G': set_send_lut_G(send_num, v); break;
        case 'B': set_send_lut_B(send_num, v); break;
    }

    return(0); 
}

int matrix_getter(tosc_message *m, connectionT *c) {
    int row, col;
    int idx = message_to_matrix_element(m, c, &row, &col);

    if (idx < 0) return (-1);

    return get_analog_format_color_matrix(OSC_BUFFER, OSC_BUFFER_SIZE, row, col);
}

int matrix_setter(tosc_message *m, connectionT *c) {
    int row, col;
    float v;
    int idx = message_to_matrix_element(m, c, &row, &col);

    v = tosc_getNextFloat(m);

    if (idx < 0) return (-1);

    set_analog_format_color_matrix(row, col, v);

    return (0);
}


int sync_all(tosc_message *m, connectionT *c) {
    int len;

    len = get_clock_offset(OSC_BUFFER,OSC_BUFFER_SIZE);
    c->send(c, OSC_BUFFER, len);
    len = get_sync_mode(OSC_BUFFER,OSC_BUFFER_SIZE);
    c->send(c, OSC_BUFFER, len);
    len = get_analog_format_resolution(OSC_BUFFER,OSC_BUFFER_SIZE);
    c->send(c, OSC_BUFFER, len);
    len = get_analog_format_framerate(OSC_BUFFER,OSC_BUFFER_SIZE);
    c->send(c, OSC_BUFFER, len);
    len = get_analog_format_colourspace(OSC_BUFFER,OSC_BUFFER_SIZE);
    c->send(c, OSC_BUFFER, len);
    for(int row=0; row<3; ++row) for(int col=0; col<3; ++col) {
        len = get_analog_format_color_matrix(OSC_BUFFER,OSC_BUFFER_SIZE,row,col);
        c->send(c, OSC_BUFFER, len);
    }
    for (int send_idx=0; send_idx<4; send_idx++) {
        len = get_send_input(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_scaleX(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_scaleY(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_posX(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_posY(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_rotation(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_pitch(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_yaw(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_brightness(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_contrast(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_saturation(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_hue(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_lut_Y(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_lut_R(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_lut_G(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
        len = get_send_lut_B(OSC_BUFFER,OSC_BUFFER_SIZE,send_idx);
        c->send(c, OSC_BUFFER, len);
    }

    ack(m, c);

    return (0);
}



// handler table

osc_handlerT handlers[] = {
    // addr,                          fmt, raw_get,               raw_set,                arg,              handler
    { "/ack",                       "",  ack,                     ack,                    OSC_ARG_NONE,   { .generic = { NULL, NULL } } },
    { "/sync",                      "",  sync_all,                NULL,                   OSC_ARG_NONE,   { .generic = { NULL, NULL } } },
    { "/sync_mode",                 "s", NULL,                    NULL,                   OSC_ARG_STRING, { .string = {get_sync_mode, set_sync_mode} } } ,

    { "/analog_format/resolution",  "s",  NULL,                   NULL,                   OSC_ARG_STRING, { .string = { get_analog_format_resolution, set_analog_format_resolution} } },
    { "/analog_format/framerate",   "s",  NULL,                   NULL,                   OSC_ARG_FLOAT,  { .generic = { get_analog_format_framerate, set_analog_format_framerate } } },
    { "/analog_format/colourspace", "s",  NULL,                   NULL,                   OSC_ARG_STRING, { .string = { get_analog_format_colourspace, set_analog_format_colourspace } } },
    { "/analog_format/colorspace",  "s",  NULL,                   NULL,                   OSC_ARG_STRING, { .string = { get_analog_format_colourspace, set_analog_format_colourspace } } },
    { "/analog_format/color_matrix/[0-2]_[0-2]", "f", matrix_getter, matrix_setter,       OSC_ARG_FLOAT, { .generic = {NULL, NULL} }  },
    { "/clock_offset",              "f",  NULL,                   NULL,                   OSC_ARG_INT ,   { .integer = { get_clock_offset, set_clock_offset } } },

    { "/send/[1-4]/lut/[YRGB]",     "L",  send_lut_get_wrapper,   send_lut_set_wrapper,   OSC_ARG_LUT, { .generic = {NULL, NULL} } },
    { "/send/[1-4]/input",         "i",   NULL,                   send_input_set_wrapper, OSC_ARG_INT,     { .send    = { get_send_input, NULL } } },

    // send-indexed floats
    #define SEND_ENTRY(path, getter, setter) \
      { path, "f", NULL, NULL, OSC_ARG_SEND_FLOAT, { .send = { getter, setter } } }

    SEND_ENTRY("/send/[1-4]/scaleX",      get_send_scaleX,     set_send_scaleX),
    SEND_ENTRY("/send/[1-4]/scaleY",      get_send_scaleY,     set_send_scaleY),
    SEND_ENTRY("/send/[1-4]/posX",        get_send_posX,       set_send_posX),
    SEND_ENTRY("/send/[1-4]/posY",        get_send_posY,       set_send_posY),
    SEND_ENTRY("/send/[1-4]/rotation",    get_send_rotation,   set_send_rotation),
    SEND_ENTRY("/send/[1-4]/pitch",       get_send_pitch,      set_send_pitch),
    SEND_ENTRY("/send/[1-4]/yaw",         get_send_yaw,        set_send_yaw),
    SEND_ENTRY("/send/[1-4]/brightness",  get_send_brightness, set_send_brightness),
    SEND_ENTRY("/send/[1-4]/contrast",    get_send_contrast,   set_send_contrast),
    SEND_ENTRY("/send/[1-4]/saturation",  get_send_saturation, set_send_saturation),
    SEND_ENTRY("/send/[1-4]/hue",         get_send_hue,        set_send_hue),

    { NULL, NULL, NULL, NULL, OSC_ARG_NONE, { .generic = { NULL, NULL } } }
    #undef SEND_ENTRY
};

// dispatch

void dispatch_message(tosc_message *osc, connectionT *conn) {
    for (osc_handlerT *h = handlers; h->address_match; ++h) {
        if (!globmatch(osc->buffer, h->address_match))
            continue;

        // GET (no args)
        if (osc->format[0] == '\0') {
            if (h->raw_getter) {
                h->raw_getter(osc, conn);
            }
            else if (h->handler.generic.get) {
                generic_getter_wrapper(osc, conn, h->handler.generic.get);
            } else if (h->handler.string.get) {
                generic_getter_wrapper(osc, conn, h->handler.string.get);
            } else if (h->handler.send.get) {
                send_getter_wrapper(osc, conn, h->handler.send.get);
            }
            else {
                send_error_message(conn, "no getter");
            }
            return;
        }

        // SET
        if (strcmp(h->format, osc->format) != 0) {
            if (!(h->arg_type == OSC_ARG_LUT)) {
                send_error_message(conn, "format mismatch");
                return;
            } else if (strcmp(osc->format, "ffffffffffffffffffffffffffffffff") != 0) {
                send_error_message(conn, "LUTs require 32 float arguments: 16 (x,y) pairs");
                send_error_message(conn, osc->format);
                return;
            }
        }

        if (h->raw_setter) {
            h->raw_setter(osc, conn);
            return;
        }

        switch (h->arg_type) {
            case OSC_ARG_FLOAT:
                generic_setter_float_wrapper(osc, conn, h->handler.generic.set);
                break;
            case OSC_ARG_STRING:
                generic_setter_string_wrapper(osc, conn, h->handler.string.set);
                break;
            case OSC_ARG_SEND_FLOAT:
                send_setter_float_wrapper(osc, conn, h->handler.send.set);
                break;
            default:
                send_error_message(conn, "no setter");
        }
        return;
    }

    // no match
    send_error_message(conn, "invalid address");
}

// debug send wrapper

size_t send_wrapper(connectionT *conn, const void *buf, size_t len) {
    int i;
    char *buffer = (char *) buf;
    printf("SENDING: ");
    for (i = 0; i < len; i++) {
        if (isprint((unsigned char)buffer[i])) printf("%c", buffer[i]);
        else printf("(%02X)", buffer[i]);
    }
    printf("\n");

    conn->con.sa_len = sizeof(conn->con.sa);
    errno = 0;
    int sent = sendto(conn->con.fd, buf, len, 0, &conn->con.sa, conn->con.sa_len);
    if (errno) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(conn->con.fd, &wfds);
        if (select(conn->con.fd+1, NULL, &wfds, NULL, NULL) > 0) {
            sent = sendto(conn->con.fd, buf, len, 0, &conn->con.sa, conn->con.sa_len);
            if (sent<0) {
                perror("sending");
            }
        }
    }

    return sent;
}

// main loop

bool keepRunning = true;

static void sigintHandler(int x) {
    (void)x;
    keepRunning = false;
}

int main(int argc, char *argv[]) {
    char buffer[2048];
    connectionT conn;
    conn.send = send_wrapper;

    signal(SIGINT, sigintHandler);

    conn.con.fd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(conn.con.fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(9000);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(conn.con.fd, (struct sockaddr *)&sin, sizeof(sin));

    printf("tinyosc is now listening on port 9000.\n");
    printf("Press Ctrl+C to stop.\n");

    while (keepRunning) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(conn.con.fd, &readSet);
        struct timeval timeout = {1, 0};
        if (select(conn.con.fd+1, &readSet, NULL, NULL, &timeout) > 0) {
            int len;
            while ((len = (int)recvfrom(conn.con.fd, buffer, sizeof(buffer), 0,
                                          &conn.con.sa, &conn.con.sa_len)) > 0) {
                printf("RECEIVED [%s]\n", buffer);
                if (tosc_isBundle(buffer)) {
                    tosc_bundle bundle;
                    tosc_parseBundle(&bundle, buffer, len);
                    uint64_t timetag = tosc_getTimetag(&bundle);
                    printf("Timetag: %llu\n", (unsigned long long)timetag);
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

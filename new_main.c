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


Config config =
{
   .input = {
        {
            .connected = 1,
            .resolution = "1920x1080",
            .framerate = 24.0,
            .colorspace = "YUV",
            .bit_depth = 8,
            .chroma_subsampling = "4:4:4"
        },
        {
            .connected = 1,
            .resolution = "3840x2160",
            .framerate = 29.97,
            .colorspace = "YUV",
            .bit_depth = 10,
            .chroma_subsampling = "4:2:0"
        },
        {
            .connected = 0,
            .resolution = "1x1",
            .framerate = 24.0,
            .colorspace = "YUV",
            .bit_depth = 0,
            .chroma_subsampling = "4:4:4"
        },
        {
            .connected = 0,
            .resolution = "1x1",
            .framerate = 24.0,
            .colorspace = "YUV",
            .bit_depth = 0,
            .chroma_subsampling = "4:4:4"
        }
  },
  .analog_format = {
    .resolution = "1920x1080",
    .framerate = 60.0,
    .colourspace = "RGB",
    .color_matrix = {
      { 1.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 1.0 }
    }
  },
  .clock_offset = 0,
  .sync_mode = "locked",
  .send = {
    {
      .input = 1,
      .scaleX = 1.0,
      .scaleY = 1.0,
      .posX = 0.0,
      .posY = 0.0,
      .rotation = 0.0,
      .pitch = 0.0,
      .yaw = 0.0,
      .brightness = 0.5,
      .contrast = 0.5,
      .saturation = 0.5,
      .hue = 0.0,
      .lut = {
        .Y = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .R = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .G = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .B = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 }
      }
    },
    {
      .input = 2,
      .scaleX = 1.0,
      .scaleY = 1.0,
      .posX = 0.0,
      .posY = 0.0,
      .rotation = 0.0,
      .pitch = 0.0,
      .yaw = 0.0,
      .brightness = 0.5,
      .contrast = 0.5,
      .saturation = 0.5,
      .hue = 0.0,
      .lut = {
        .Y = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .R = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .G = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .B = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 }
      }
    },
    {
      .input = 3,
      .scaleX = 1.0,
      .scaleY = 1.0,
      .posX = 0.0,
      .posY = 0.0,
      .rotation = 0.0,
      .pitch = 0.0,
      .yaw = 0.0,
      .brightness = 0.5,
      .contrast = 0.5,
      .saturation = 0.5,
      .hue = 0.0,
      .lut = {
        .Y = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .R = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .G = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .B = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 }
      }
    },
    {
      .input = 4,
      .scaleX = 1.0,
      .scaleY = 1.0,
      .posX = 0.0,
      .posY = 0.0,
      .rotation = 0.0,
      .pitch = 0.0,
      .yaw = 0.0,
      .brightness = 0.5,
      .contrast = 0.5,
      .saturation = 0.5,
      .hue = 0.0,
      .lut = {
        .Y = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .R = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .G = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 },
        .B = { -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 1.0, 1.0 }
      }
    }
  }
};


/**
*** MESSAGE HANDLING
**/


#define OSC_BUF_SIZE 1024
static char OSC_BUFFER[OSC_BUF_SIZE];

// Connection wrapper

typedef struct {
    int fd;
    struct sockaddr_storage addr;
    socklen_t addr_len;
} conT;

typedef struct connectionT {
    conT   con;
    size_t (*send)(struct connectionT *, const void *, size_t);
    size_t (*receive)(struct connectionT *, const void *, size_t);
} connectionT;

// Unified handler signature
typedef int (*OscHandler)(tosc_message *msg, connectionT *conn);

// Helper macro to send OSC replies
#define send_osc(conn, path, fmt, ...) \
    do { \
        int _len = tosc_writeMessage(OSC_BUFFER, OSC_BUF_SIZE, path, fmt, __VA_ARGS__); \
        (conn)->send(conn, OSC_BUFFER, _len); \
    } while(0)

// Error reply
static void send_error_message(connectionT *conn, const char *text) {
    send_osc(conn, "/error", "s", text);
}

// /ack handler
static int handle_ack(tosc_message *msg, connectionT *conn) {
    send_osc(conn, "/ack", "", NULL);
    return 0;
}

// parse input index 1..4 from /input/{n}/...
static int parse_input_index(tosc_message *msg, connectionT *conn) {
    // /input/n
    // 01234567
    //        ^

    int d = msg->buffer[7] - '0'; 
    if ((d < 0) || (d > 3)) {
        send_error_message(conn, "Invalid input number");
        return -1;
    }
    return d;
}



// parse send index 1..4 from /send/{n}/...
static int parse_send_index(tosc_message *msg, connectionT *conn) {
    // /send/n
    // 0123456
    //       ^

    int d = msg->buffer[6] - '0'; 
    if ((d < 0) || (d > 3)) {
        send_error_message(conn, "Invalid send number");
        return -1;
    }
    return d;
}



// parse matrix row/col from /analog_format/color_matrix/{r}/{c}
static int parse_matrix_coords(tosc_message *msg, connectionT *conn, int *row, int *col) {
    // /analog_format/color_matrix/R/C
    // 0123456789ABCDEF0123456789ABCDE
    //                             ^ ^
    
    int r = msg->buffer[0x1C] - '0';
    int c = msg->buffer[0x1E] - '0';
    if ((r < 0) || (r > 2) || (c < 0) || (c > 2)) {
        send_error_message(conn, "Matrix index out of bounds");
        return -1;
    } 
    *row = r;
    *col = c;
    return 0;
}

char message_to_lut_channel(tosc_message *m, connectionT *conn) {
    // /send/n/lut/C
    // 0123456789ABC
    //             ^

    char c = m->buffer[0xC];
    if (c!='Y' && c!='R' && c!='G' && c!='B') {
        send_error_message(conn, "Invalid LUT channel");
        return -1;
    }
    return c;
}

// Handler generator macros

#define DEFINE_HANDLER_INT(name, field) \
static int name(tosc_message *msg, connectionT *conn) { \
    const char *path = tosc_getAddress(msg); \
    int idx = parse_input_index(msg, conn); if (idx<0) return 0; \
    if (msg->format[0] == '\0') { \
        send_osc(conn, path, "i", config.input[idx].field); \
    } else { \
        int v = tosc_getNextInt32(msg); config.input[idx].field = v; \
    } \
    return 0; \
}

#define DEFINE_HANDLER_STR(name, field) \
static int name(tosc_message *msg, connectionT *conn) { \
    const char *path = tosc_getAddress(msg); \
    int idx = parse_input_index(msg, conn); if (idx<0) return 0; \
    if (msg->format[0] == '\0') { \
        send_osc(conn, path, "s", config.input[idx].field); \
    } else { \
        const char *s = tosc_getNextString(msg); strncpy(config.input[idx].field, s, CONFIG_MAX_STR_LEN-1); \
    } \
    return 0; \
}

#define DEFINE_HANDLER_FLOAT(name, field) \
static int name(tosc_message *msg, connectionT *conn) { \
    const char *path = tosc_getAddress(msg); \
    int idx = parse_input_index(msg, conn); if (idx<0) return 0; \
    if (msg->format[0] == '\0') { \
        send_osc(conn, path, "f", config.input[idx].field); \
    } else { \
        float v = tosc_getNextFloat(msg); config.input[idx].field = v; \
    } \
    return 0; \
}

// Input handlers
DEFINE_HANDLER_STR(handle_input_resolution, resolution)
DEFINE_HANDLER_FLOAT(handle_input_framerate, framerate)
DEFINE_HANDLER_STR(handle_input_colorspace, colorspace)
DEFINE_HANDLER_INT(handle_input_bit_depth, bit_depth)
DEFINE_HANDLER_STR(handle_input_chroma_subsampling, chroma_subsampling)



static int handle_input_connected(tosc_message *msg, connectionT *conn) {
    const char *addr = tosc_getAddress(msg);
    int idx = parse_input_index(msg, conn);
    if (idx < 0) return 0;

    // GET (no format): reply with OSC boolean 'T' or 'F'
    if (msg->format[0] == '\0') {
        send_osc(conn, addr, "T", NULL);
    }
    // SET: expect format 'T' or 'F'
    else if (msg->format[0] == 'T' || msg->format[0] == 'F') {
        config.input[idx].connected = (msg->format[0] == 'T');
    }
    // Error on any other format
    else {
        send_osc(conn, "/error", "s", "Expected boolean T or F");
    }

    return 0;
}

// clock_offset
static int handle_clock_offset(tosc_message *msg, connectionT *conn) {
    const char *path = "/clock_offset";
    if (msg->format[0] == '\0') send_osc(conn, path, "i", config.clock_offset);
    else { int v = tosc_getNextInt32(msg); config.clock_offset = v; }
    return 0;
}

// sync_mode
static int handle_sync_mode(tosc_message *msg, connectionT *conn) {
    const char *path = "/sync_mode";
    if (msg->format[0] == '\0') send_osc(conn, path, "s", config.sync_mode);
    else { const char *s = tosc_getNextString(msg); strncpy(config.sync_mode, s, CONFIG_MAX_STR_LEN-1);}    
    return 0;
}

// analog_format handlers
static int handle_analog_resolution(tosc_message *msg, connectionT *conn) {
    const char *path = "/analog_format/resolution";
    if (msg->format[0] == '\0') send_osc(conn, path, "s", config.analog_format.resolution);
    else { const char *s = tosc_getNextString(msg); strncpy(config.analog_format.resolution, s, CONFIG_MAX_STR_LEN-1);}  return 0;
}
static int handle_analog_framerate(tosc_message *msg, connectionT *conn) {
    const char *path = "/analog_format/framerate";
    if (msg->format[0] == '\0') send_osc(conn, path, "f", config.analog_format.framerate);
    else { float v = tosc_getNextFloat(msg); config.analog_format.framerate=v;} return 0;
}
static int handle_analog_colourspace(tosc_message *msg, connectionT *conn) {
    const char *path = "/analog_format/colourspace";
    if (msg->format[0] == '\0') send_osc(conn, path, "s", config.analog_format.colourspace);
    else { const char *s=tosc_getNextString(msg); strncpy(config.analog_format.colourspace,s,CONFIG_MAX_STR_LEN-1);} return 0;
}
static int handle_analog_color_matrix(tosc_message *msg, connectionT *conn) {
    int r,c; if(parse_matrix_coords(msg,conn,&r,&c)<0) return 0;
    char path[64]; snprintf(path,sizeof(path),"/analog_format/color_matrix/%d/%d",r,c);
    if(msg->format[0]=='\0') send_osc(conn,path,"f",config.analog_format.color_matrix[r][c]);
    else { float v = tosc_getNextFloat(msg); config.analog_format.color_matrix[r][c]=v; }
    return 0;
}

// send_input
static int handle_send_input(tosc_message *msg, connectionT *conn) {
    int idx = parse_send_index(msg,conn); if(idx<0) return 0;
    const char *path = tosc_getAddress(msg);
    if(msg->format[0]=='\0') send_osc(conn,path,"i",config.send[idx].input);
    else { int v=tosc_getNextInt32(msg); config.send[idx].input=v;} return 0;
}

// send floats
#define DEFINE_SEND_FLOAT(name, field) \
static int name(tosc_message *msg, connectionT *conn) { \
    int idx=parse_send_index(msg,conn); if(idx<0) return 0; \
    const char*path=tosc_getAddress(msg); \
    if(msg->format[0]=='\0') send_osc(conn,path,"f",config.send[idx].field); \
    else { float v=tosc_getNextFloat(msg); config.send[idx].field=v;} return 0; \
}

DEFINE_SEND_FLOAT(handle_send_scaleX,scaleX)
DEFINE_SEND_FLOAT(handle_send_scaleY,scaleY)
DEFINE_SEND_FLOAT(handle_send_posX,posX)
DEFINE_SEND_FLOAT(handle_send_posY,posY)
DEFINE_SEND_FLOAT(handle_send_rotation,rotation)
DEFINE_SEND_FLOAT(handle_send_pitch,pitch)
DEFINE_SEND_FLOAT(handle_send_yaw,yaw)
DEFINE_SEND_FLOAT(handle_send_brightness,brightness)
DEFINE_SEND_FLOAT(handle_send_contrast,contrast)
DEFINE_SEND_FLOAT(handle_send_saturation,saturation)
DEFINE_SEND_FLOAT(handle_send_hue,hue)

// send_lut channels
#define DEFINE_SEND_LUT(name,channel) \
static int name(tosc_message *msg, connectionT *conn) { \
    int idx=parse_send_index(msg,conn); if(idx<0) return 0; \
    const char*path=tosc_getAddress(msg); \
    if(msg->format[0]=='\0') { \
        send_osc(conn,path,"ffffffffffffffffffffffffffffffff", \
            config.send[idx].lut.channel[0],config.send[idx].lut.channel[1],config.send[idx].lut.channel[2],config.send[idx].lut.channel[3], \
            config.send[idx].lut.channel[4],config.send[idx].lut.channel[5],config.send[idx].lut.channel[6],config.send[idx].lut.channel[7], \
            config.send[idx].lut.channel[8],config.send[idx].lut.channel[9],config.send[idx].lut.channel[10],config.send[idx].lut.channel[11],\
            config.send[idx].lut.channel[12],config.send[idx].lut.channel[13],config.send[idx].lut.channel[14],config.send[idx].lut.channel[15],\
            config.send[idx].lut.channel[16],config.send[idx].lut.channel[17],config.send[idx].lut.channel[18],config.send[idx].lut.channel[19],\
            config.send[idx].lut.channel[20],config.send[idx].lut.channel[21],config.send[idx].lut.channel[22],config.send[idx].lut.channel[23],\
            config.send[idx].lut.channel[24],config.send[idx].lut.channel[25],config.send[idx].lut.channel[26],config.send[idx].lut.channel[27],\
            config.send[idx].lut.channel[28],config.send[idx].lut.channel[29],config.send[idx].lut.channel[30],config.send[idx].lut.channel[31] \
        ); \
    } else { float v[32]; for(int i=0;i<32;i++) v[i]=tosc_getNextFloat(msg); memcpy(config.send[idx].lut.channel,v,32*sizeof(float)); } \
    return 0; \
}

DEFINE_SEND_LUT(handle_send_lut_Y,Y)
DEFINE_SEND_LUT(handle_send_lut_R,R)
DEFINE_SEND_LUT(handle_send_lut_G,G)
DEFINE_SEND_LUT(handle_send_lut_B,B)


static int sync_all(tosc_message *msg, connectionT *conn);

// Dispatch table
static struct{const char*path_pattern;const char*type_sig;OscHandler handler;}dispatch_table[]={
    {"/ack","",handle_ack},{"/sync","",sync_all},{"/sync_mode","s",handle_sync_mode},
    {"/input/[1-4]/connected","T",handle_input_connected},{"/input/[1-4]/resolution","s",handle_input_resolution},
    {"/input/[1-4]/framerate","f",handle_input_framerate},{"/input/[1-4]/colorspace","s",handle_input_colorspace},
    {"/input/[1-4]/bit_depth","i",handle_input_bit_depth},{"/input/[1-4]/chroma_subsampling","s",handle_input_chroma_subsampling},
    {"/clock_offset","i",handle_clock_offset},{"/analog_format/resolution","s",handle_analog_resolution},
    {"/analog_format/framerate","f",handle_analog_framerate},{"/analog_format/colourspace","s",handle_analog_colourspace},
    {"/analog_format/color_matrix/[0-2]/[0-2]","f",handle_analog_color_matrix},{"/send/[1-4]/input","i",handle_send_input},
    {"/send/[1-4]/scaleX","f",handle_send_scaleX},{"/send/[1-4]/scaleY","f",handle_send_scaleY},
    {"/send/[1-4]/posX","f",handle_send_posX},{"/send/[1-4]/posY","f",handle_send_posY},
    {"/send/[1-4]/rotation","f",handle_send_rotation},{"/send/[1-4]/pitch","f",handle_send_pitch},
    {"/send/[1-4]/yaw","f",handle_send_yaw},{"/send/[1-4]/brightness","f",handle_send_brightness},
    {"/send/[1-4]/contrast","f",handle_send_contrast},{"/send/[1-4]/saturation","f",handle_send_saturation},
    {"/send/[1-4]/hue","f",handle_send_hue},{"/send/[1-4]/lut/Y","*",handle_send_lut_Y},
    {"/send/[1-4]/lut/R","*",handle_send_lut_R},{"/send/[1-4]/lut/G","*",handle_send_lut_G},
    {"/send/[1-4]/lut/B","*",handle_send_lut_B},{NULL,NULL,NULL}};

// Central dispatch
void dispatch_message(tosc_message *osc, connectionT *conn) {
    for(int i=0;dispatch_table[i].path_pattern;i++){
        if(!globmatch((char *)osc->buffer, (char *)dispatch_table[i].path_pattern))continue;
        const char*sig=dispatch_table[i].type_sig;
        if(sig[0]&&strcmp(sig,osc->format)!=0){send_error_message(conn,"format mismatch");return;}
        dispatch_table[i].handler(osc,conn);return;
    }
    send_error_message(conn,"invalid address");
}

// sync_all: broadcast via existing handlers
static int sync_all(tosc_message *msg, connectionT *conn) {

    tosc_message *dummy = msg;

    for(int i=0;dispatch_table[i].path_pattern;++i){
        const char*path=dispatch_table[i].path_pattern;
        if(strcmp(path,"/sync")==0||strcmp(path,"/ack")==0)continue;
        dispatch_table[i].handler(dummy,conn);
    }
    handle_ack(dummy, conn);
    return 0;
}





// debug send wrapper

size_t send_wrapper(connectionT *conn, const void *buf, size_t len) {
    ssize_t sent = sendto(
        conn->con.fd,
        buf, len,
        0,
        (struct sockaddr *)&conn->con.addr,
        conn->con.addr_len
    );

    if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        fd_set wfds;
        for (;;) {
            FD_ZERO(&wfds);
            FD_SET(conn->con.fd, &wfds);
            int r = select(conn->con.fd + 1, NULL, &wfds, NULL, NULL);
            if (r < 0) {
                if (errno == EINTR) continue;
                perror("select");
                return -1;
            }
            if (FD_ISSET(conn->con.fd, &wfds)) {
                sent = sendto(
                    conn->con.fd,
                    buf, len,
                    0,
                    (struct sockaddr *)&conn->con.addr,
                    conn->con.addr_len
                );
                if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
                break;
            }
        }
    } else if (sent < 0) {
        perror("sendto");
    }

    return (size_t)sent;
}

// main loop

bool keepRunning = true;

static void sigintHandler(int x) {
    (void)x;
    keepRunning = false;
}

int main(int argc, char *argv[]) {
    char buffer[2048];
    connectionT conn = {0};
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
        if (select(conn.con.fd + 1, &readSet, NULL, NULL, &timeout) > 0) {
            int len;
            conn.con.addr_len = sizeof(conn.con.addr);
            while ((len = (int)recvfrom(
                conn.con.fd,
                buffer, sizeof(buffer), 0,
                (struct sockaddr *)&conn.con.addr,
                &conn.con.addr_len
            )) > 0) {
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
                    tosc_printOscBuffer(buffer, len);
                    dispatch_message(&osc, &conn);
                }
            }
        }
    }

    close(conn.con.fd);
    return 0;
}

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "globmatch.h"
#include "network.h"
#include "osc_config.h"
#include "tinyosc.h"

#include "osc_config_defaults.c"

/**
*** MESSAGE HANDLING
**/

#define OSC_BUF_SIZE 1024
static char OSC_BUFFER[OSC_BUF_SIZE];

// Helper macro to send OSC replies
#define send_osc(conn, path, fmt, ...)                                         \
  do {                                                                         \
    int _len =                                                                 \
        tosc_writeMessage(OSC_BUFFER, OSC_BUF_SIZE, path, fmt, __VA_ARGS__);   \
    (conn)->send(conn, OSC_BUFFER, _len);                                      \
  } while (0)

// Error reply
static void send_error_message(connectionT *conn, const char *text) {
  printf("ERROR: %s\n", text);
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

  int d = msg->buffer[7] - '1';
  if ((d < 0) || (d > 3)) {
    send_error_message(conn, "Invalid input number");
    tosc_printMessage(msg);
    return -1;
  }
  return d;
}

// parse send index 1..4 from /send/{n}/...
static int parse_send_index(tosc_message *msg, connectionT *conn) {
  // /send/n
  // 0123456
  //       ^

  int d = msg->buffer[6] - '1';
  if ((d < 0) || (d > 3)) {
    send_error_message(conn, "Invalid send number");
    tosc_printMessage(msg);
    return -1;
  }
  return d;
}

// parse matrix row/col from /analog_format/color_matrix/{r}/{c}
static int parse_matrix_coords(tosc_message *msg, connectionT *conn, int *row,
                               int *col) {
  // /analog_format/color_matrix/R/C
  // 0123456789ABCDEF0123456789ABCDE
  //                             ^ ^

  int r = msg->buffer[0x1C] - '0';
  int c = msg->buffer[0x1E] - '0';
  if ((r < 0) || (r > 2) || (c < 0) || (c > 2)) {
    send_error_message(conn, "Matrix index out of bounds");
    tosc_printMessage(msg);
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
  if (c != 'Y' && c != 'R' && c != 'G' && c != 'B') {
    send_error_message(conn, "Invalid LUT channel");
    return -1;
  }
  return c;
}

// Handler generator macros

#define DEFINE_HANDLER_INT(name, field)                                        \
  static int name(tosc_message *msg, connectionT *conn) {                      \
    const char *path = tosc_getAddress(msg);                                   \
    int idx = parse_input_index(msg, conn);                                    \
    if (idx < 0)                                                               \
      return 0;                                                                \
    if (msg->format[0] == '\0') {                                              \
      send_osc(conn, path, "i", config.input[idx].field);                      \
    } else {                                                                   \
      int v = tosc_getNextInt32(msg);                                          \
      config.input[idx].field = v;                                             \
    }                                                                          \
    return 0;                                                                  \
  }

#define DEFINE_HANDLER_STR(name, field)                                        \
  static int name(tosc_message *msg, connectionT *conn) {                      \
    const char *path = tosc_getAddress(msg);                                   \
    int idx = parse_input_index(msg, conn);                                    \
    if (idx < 0)                                                               \
      return 0;                                                                \
    if (msg->format[0] == '\0') {                                              \
      send_osc(conn, path, "s", config.input[idx].field);                      \
    } else {                                                                   \
      const char *s = tosc_getNextString(msg);                                 \
      strncpy(config.input[idx].field, s, CONFIG_MAX_STR_LEN - 1);             \
    }                                                                          \
    return 0;                                                                  \
  }

#define DEFINE_HANDLER_FLOAT(name, field)                                      \
  static int name(tosc_message *msg, connectionT *conn) {                      \
    const char *path = tosc_getAddress(msg);                                   \
    int idx = parse_input_index(msg, conn);                                    \
    if (idx < 0)                                                               \
      return 0;                                                                \
    if (msg->format[0] == '\0') {                                              \
      send_osc(conn, path, "f", config.input[idx].field);                      \
    } else {                                                                   \
      float v = tosc_getNextFloat(msg);                                        \
      config.input[idx].field = v;                                             \
    }                                                                          \
    return 0;                                                                  \
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
  if (idx < 0)
    return 0;

  // GET (no format): reply with OSC boolean 'T' or 'F'
  if (msg->format[0] == '\0') {
    send_osc(conn, addr, config.input[idx].connected ? "T" : "F", NULL);
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
  if (msg->format[0] == '\0')
    send_osc(conn, path, "i", config.clock_offset);
  else {
    int v = tosc_getNextInt32(msg);
    config.clock_offset = v;
  }
  return 0;
}

// sync_mode
static int handle_sync_mode(tosc_message *msg, connectionT *conn) {
  const char *path = "/sync_mode";
  if (msg->format[0] == '\0')
    send_osc(conn, path, "s", config.sync_mode);
  else {
    const char *s = tosc_getNextString(msg);
    strncpy(config.sync_mode, s, CONFIG_MAX_STR_LEN - 1);
  }
  return 0;
}

// analog_format handlers
static int handle_analog_resolution(tosc_message *msg, connectionT *conn) {
  const char *path = "/analog_format/resolution";
  if (msg->format[0] == '\0')
    send_osc(conn, path, "s", config.analog_format.resolution);
  else {
    const char *s = tosc_getNextString(msg);
    strncpy(config.analog_format.resolution, s, CONFIG_MAX_STR_LEN - 1);
  }
  return 0;
}
static int handle_analog_framerate(tosc_message *msg, connectionT *conn) {
  const char *path = "/analog_format/framerate";
  if (msg->format[0] == '\0')
    send_osc(conn, path, "f", config.analog_format.framerate);
  else {
    float v = tosc_getNextFloat(msg);
    config.analog_format.framerate = v;
  }
  return 0;
}
static int handle_analog_colourspace(tosc_message *msg, connectionT *conn) {
  const char *path = "/analog_format/colourspace";
  if (msg->format[0] == '\0')
    send_osc(conn, path, "s", config.analog_format.colourspace);
  else {
    const char *s = tosc_getNextString(msg);
    strncpy(config.analog_format.colourspace, s, CONFIG_MAX_STR_LEN - 1);
  }
  return 0;
}
static int handle_analog_color_matrix(tosc_message *msg, connectionT *conn) {
  int r, c;
  if (parse_matrix_coords(msg, conn, &r, &c) < 0)
    return 0;
  char path[64];
  snprintf(path, sizeof(path), "/analog_format/color_matrix/%d/%d", r, c);
  if (msg->format[0] == '\0')
    send_osc(conn, path, "f", config.analog_format.color_matrix[r][c]);
  else {
    float v = tosc_getNextFloat(msg);
    config.analog_format.color_matrix[r][c] = v;
  }
  return 0;
}

// send_input
static int handle_send_input(tosc_message *msg, connectionT *conn) {
  int idx = parse_send_index(msg, conn);
  if (idx < 0)
    return 0;
  const char *path = tosc_getAddress(msg);
  if (msg->format[0] == '\0')
    send_osc(conn, path, "i", config.send[idx].input);
  else {
    int v = tosc_getNextInt32(msg);
    config.send[idx].input = v;
  }
  return 0;
}

// send floats
#define DEFINE_SEND_FLOAT(name, field)                                         \
  static int name(tosc_message *msg, connectionT *conn) {                      \
    int idx = parse_send_index(msg, conn);                                     \
    if (idx < 0)                                                               \
      return 0;                                                                \
    const char *path = tosc_getAddress(msg);                                   \
    if (msg->format[0] == '\0')                                                \
      send_osc(conn, path, "f", config.send[idx].field);                       \
    else {                                                                     \
      float v = tosc_getNextFloat(msg);                                        \
      config.send[idx].field = v;                                              \
    }                                                                          \
    return 0;                                                                  \
  }

DEFINE_SEND_FLOAT(handle_send_scaleX, scaleX)
DEFINE_SEND_FLOAT(handle_send_scaleY, scaleY)
DEFINE_SEND_FLOAT(handle_send_posX, posX)
DEFINE_SEND_FLOAT(handle_send_posY, posY)
DEFINE_SEND_FLOAT(handle_send_rotation, rotation)
DEFINE_SEND_FLOAT(handle_send_pitch, pitch)
DEFINE_SEND_FLOAT(handle_send_yaw, yaw)
DEFINE_SEND_FLOAT(handle_send_brightness, brightness)
DEFINE_SEND_FLOAT(handle_send_contrast, contrast)
DEFINE_SEND_FLOAT(handle_send_saturation, saturation)
DEFINE_SEND_FLOAT(handle_send_hue, hue)

static int handle_send_lut(tosc_message *msg, connectionT *conn) {
    int idx = parse_send_index(msg, conn);
    if (idx < 0) return 0;

    char ch = message_to_lut_channel(msg, conn);
    if (ch < 0) return 0;

    LutChannel lc;
    switch (ch) {
      case 'Y': lc = LUT_CHANNEL_Y; break;
      case 'R': lc = LUT_CHANNEL_R; break;
      case 'G': lc = LUT_CHANNEL_G; break;
      case 'B': lc = LUT_CHANNEL_B; break;
      default:
        send_error_message(conn, "Invalid LUT channel");
        return 0;
    }

    const char *path = tosc_getAddress(msg);
    // format string of 32 'f's
    char fmt[33];
    for (int i = 0; i < 32; i++) fmt[i] = 'f';
    fmt[32] = '\0';

    if (msg->format[0] == '\0') {
        int len = tosc_writeMessage(
            OSC_BUFFER, OSC_BUF_SIZE, path, fmt,
            config.send[idx].lut[lc].points[0].x,  config.send[idx].lut[lc].points[0].y,
            config.send[idx].lut[lc].points[1].x,  config.send[idx].lut[lc].points[1].y,
            config.send[idx].lut[lc].points[2].x,  config.send[idx].lut[lc].points[2].y,
            config.send[idx].lut[lc].points[3].x,  config.send[idx].lut[lc].points[3].y,
            config.send[idx].lut[lc].points[4].x,  config.send[idx].lut[lc].points[4].y,
            config.send[idx].lut[lc].points[5].x,  config.send[idx].lut[lc].points[5].y,
            config.send[idx].lut[lc].points[6].x,  config.send[idx].lut[lc].points[6].y,
            config.send[idx].lut[lc].points[7].x,  config.send[idx].lut[lc].points[7].y,
            config.send[idx].lut[lc].points[8].x,  config.send[idx].lut[lc].points[8].y,
            config.send[idx].lut[lc].points[9].x,  config.send[idx].lut[lc].points[9].y,
            config.send[idx].lut[lc].points[10].x, config.send[idx].lut[lc].points[10].y,
            config.send[idx].lut[lc].points[11].x, config.send[idx].lut[lc].points[11].y,
            config.send[idx].lut[lc].points[12].x, config.send[idx].lut[lc].points[12].y,
            config.send[idx].lut[lc].points[13].x, config.send[idx].lut[lc].points[13].y,
            config.send[idx].lut[lc].points[14].x, config.send[idx].lut[lc].points[14].y,
            config.send[idx].lut[lc].points[15].x, config.send[idx].lut[lc].points[15].y
        );
        conn->send(conn, OSC_BUFFER, len);
    } else {
        for (int i = 0; i < LUT_CONTROL_POINT_COUNT; i++) {
            float x = tosc_getNextFloat(msg);
            float y = tosc_getNextFloat(msg);
            config.send[idx].lut[lc].points[i].x = x;
            config.send[idx].lut[lc].points[i].y = y;
        }
    }

    return 0;
}


static int sync_all(tosc_message *msg, connectionT *conn);

// Dispatch table
static dispatch_entry dispatch_table[] = {
    {"/ack", "", handle_ack},
    {"/sync", "", sync_all},
    {"/sync_mode", "s", handle_sync_mode},
    {"/input/[1-4]/connected", "T", handle_input_connected},
    {"/input/[1-4]/resolution", "s", handle_input_resolution},
    {"/input/[1-4]/framerate", "f", handle_input_framerate},
    {"/input/[1-4]/colorspace", "s", handle_input_colorspace},
    {"/input/[1-4]/bit_depth", "i", handle_input_bit_depth},
    {"/input/[1-4]/chroma_subsampling", "s", handle_input_chroma_subsampling},
    {"/clock_offset", "i", handle_clock_offset},
    {"/analog_format/resolution", "s", handle_analog_resolution},
    {"/analog_format/framerate", "f", handle_analog_framerate},
    {"/analog_format/colourspace", "s", handle_analog_colourspace},
    {"/analog_format/color_matrix/[0-2]/[0-2]", "f",
     handle_analog_color_matrix},
    {"/send/[1-4]/input", "i", handle_send_input},
    {"/send/[1-4]/scaleX", "f", handle_send_scaleX},
    {"/send/[1-4]/scaleY", "f", handle_send_scaleY},
    {"/send/[1-4]/posX", "f", handle_send_posX},
    {"/send/[1-4]/posY", "f", handle_send_posY},
    {"/send/[1-4]/rotation", "f", handle_send_rotation},
    {"/send/[1-4]/pitch", "f", handle_send_pitch},
    {"/send/[1-4]/yaw", "f", handle_send_yaw},
    {"/send/[1-4]/brightness", "f", handle_send_brightness},
    {"/send/[1-4]/contrast", "f", handle_send_contrast},
    {"/send/[1-4]/saturation", "f", handle_send_saturation},
    {"/send/[1-4]/hue", "f", handle_send_hue},
    {"/send/[1-4]/lut/[YRGB]", "ffffffffffffffffffffffffffffffff", handle_send_lut},
    {NULL, NULL, NULL}};

// Central dispatch
void dispatch_message(tosc_message *osc, connectionT *conn) {
  for (int i = 0; dispatch_table[i].path_pattern; i++) {
    if (!globmatch((char *)osc->buffer, (char *)dispatch_table[i].path_pattern))
      continue;
    const char *sig = dispatch_table[i].type_sig;
    if (sig[0] && strcmp(sig, osc->format) != 0) {
      send_error_message(conn, "format mismatch");
      return;
    }
    dispatch_table[i].handler(osc, conn);
    return;
  }
  send_error_message(conn, "invalid address");
}

// sync_all: broadcast via existing handlers
static int sync_all(tosc_message *msg, connectionT *conn) {
  char local_path[128];
  tosc_message dummy;

  for (dispatch_entry *e = dispatch_table; e->path_pattern; ++e) {
    const char *pat = e->path_pattern;

    // skip sync and ack themselves
    if (strcmp(pat, "/sync") == 0 || strcmp(pat, "/ack") == 0)
      continue;

    // Expand wildcards for /input/[1-4]/...
    if (strncmp(pat, "/input/[1-4]/", 12) == 0) {
      for (int n = 1; n <= 4; ++n) {
        snprintf(local_path, sizeof(local_path), "/input/%d%s", n, pat + 12);
        dummy.buffer = local_path;
        size_t len = strlen(local_path);
        dummy.format = dummy.buffer + len;
        *dummy.format = '\0';
        dummy.marker = dummy.format;
        e->handler(&dummy, conn);
      }
    }
    // Expand wildcards for /send/[1-4]/... (non-LUT)
    else if (strncmp(pat, "/send/[1-4]/", 12) == 0 &&
             strstr(pat, "/lut/") == NULL) {
      for (int n = 1; n <= 4; ++n) {
        snprintf(local_path, sizeof(local_path), "/send/%d%s", n, pat + 11);
        dummy.buffer = local_path;
        size_t len = strlen(local_path);
        dummy.format = dummy.buffer + len;
        *dummy.format = '\0';
        dummy.marker = dummy.format;
        e->handler(&dummy, conn);
      }
    }
    // Expand LUT channels
    else if (strcmp(pat, "/send/[1-4]/lut/[YRGB]") == 0) {
      const char *channels = "YRGB";
      for (int n = 1; n <= 4; ++n) {
        for (int i = 0; i < LUT_CHANNEL_COUNT; ++i) {
          char ch = channels[i];
          snprintf(local_path, sizeof(local_path), "/send/%d/lut/%c", n, ch);
          dummy.buffer = local_path;
          size_t len = strlen(local_path);
          dummy.format = dummy.buffer + len;
          *dummy.format = '\0';
          dummy.marker = dummy.format;
          e->handler(&dummy, conn);
        }
      }
    }
    // Expand matrix elements
    else if (strcmp(pat, "/analog_format/color_matrix/[0-2]/[0-2]") == 0) {
      for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
          snprintf(local_path, sizeof(local_path),
                   "/analog_format/color_matrix/%d/%d", r, c);
          dummy.buffer = local_path;
          size_t len = strlen(local_path);
          dummy.format = dummy.buffer + len;
          *dummy.format = '\0';
          dummy.marker = dummy.format;
          e->handler(&dummy, conn);
        }
      }
    }
    // No wildcard: direct call
    else {
      snprintf(local_path, sizeof(local_path), "%s", pat);
      dummy.buffer = local_path;
      size_t len = strlen(local_path);
      dummy.format = dummy.buffer + len;
      *dummy.format = '\0';
      dummy.marker = dummy.format;
      e->handler(&dummy, conn);
    }
  }

  // final ack
  handle_ack(msg, conn);
  return 0;
}

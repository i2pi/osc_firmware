#ifndef __OSC_CONFIG_H__
#define __OSC_CONFIG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tinyosc.h"
#include "network.h"

#define CONFIG_MAX_STR_LEN           16
#define LUT_CONTROL_POINT_COUNT     16

typedef struct ConfigAnalogFormat {
  char   resolution[CONFIG_MAX_STR_LEN];
  float  framerate;
  char   colourspace[CONFIG_MAX_STR_LEN];
  float  color_matrix[3][3];
} ConfigAnalogFormat;

/* single control point in a 1-D LUT */
typedef struct LutControlPoint {
  float x;
  float y;
} LutControlPoint;

/* now holds exactly 16 control points */
typedef struct ConfigSendLut {
  LutControlPoint points[LUT_CONTROL_POINT_COUNT];
} ConfigSendLut;

typedef enum {
  LUT_CHANNEL_Y = 0,
  LUT_CHANNEL_R,
  LUT_CHANNEL_G,
  LUT_CHANNEL_B,
  LUT_CHANNEL_COUNT
} LutChannel;

typedef struct ConfigSend {
  int             input;
  float           scaleX;
  float           scaleY;
  float           posX;
  float           posY;
  float           rotation;
  float           pitch;
  float           yaw;
  float           brightness;
  float           contrast;
  float           saturation;
  float           hue;
  ConfigSendLut   lut[LUT_CHANNEL_COUNT];
} ConfigSend;

typedef struct ConfigInput {
  char   connected;
  char   resolution[CONFIG_MAX_STR_LEN];
  float  framerate;
  char   colorspace[CONFIG_MAX_STR_LEN];
  char   bit_depth;
  char   chroma_subsampling[CONFIG_MAX_STR_LEN];
} ConfigInput;

typedef struct Config {
  ConfigAnalogFormat analog_format;
  int                clock_offset;
  char               sync_mode[CONFIG_MAX_STR_LEN];
  ConfigInput        input[4];
  ConfigSend         send[4];
} Config;

/* Unified handler signature */
typedef int (*OscHandler)(tosc_message *msg, connectionT *conn);

/* Dispatch table entry */
typedef struct dispatch_entry {
  const char   *path_pattern;
  const char   *type_sig;
  OscHandler    handler;
} dispatch_entry;

void dispatch_message(tosc_message *osc, connectionT *conn);

#endif


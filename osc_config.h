#ifndef __OSC_CONFIG_H__
#define __OSC_CONFIG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_MAX_STR_LEN 16

typedef struct ConfigAnalogFormat {
  char resolution[CONFIG_MAX_STR_LEN];
  float framerate;
  char colourspace[CONFIG_MAX_STR_LEN];
  float color_matrix[3][3];
} ConfigAnalogFormat;

typedef struct ConfigSendLut {
  float Y[32];
  float R[32];
  float G[32];
  float B[32];
} ConfigSendLut;

typedef struct ConfigSend {
  int input;
  float scaleX;
  float scaleY;
  float posX;
  float posY;
  float rotation;
  float pitch;
  float yaw;
  float brightness;
  float contrast;
  float saturation;
  float hue;
  ConfigSendLut lut;
} ConfigSend;

typedef struct ConfigInput {
    char connected;
    char resolution[CONFIG_MAX_STR_LEN];
    float framerate;
    char colorspace[CONFIG_MAX_STR_LEN];
    char bit_depth;
    char chroma_subsampling[CONFIG_MAX_STR_LEN];
} ConfigInput;

typedef struct Config {
  ConfigAnalogFormat analog_format;
  int clock_offset;
  char sync_mode[CONFIG_MAX_STR_LEN];
  ConfigInput input[4];
  ConfigSend send[4];
} Config;

uint32_t get_input_connected(char *buf, int len, int input_idx);
uint32_t get_input_resolution(char *buf, int len, int input_idx);
uint32_t get_input_framerate(char *buf, int len, int input_idx);
uint32_t get_input_colorspace(char *buf, int len, int input_idx);
uint32_t get_input_bit_depth(char *buf, int len, int input_idx);
uint32_t get_input_chroma_subsampling(char *buf, int len, int input_idx);
uint32_t get_analog_format_resolution(char *buf, int len);
uint32_t get_analog_format_framerate(char *buf, int len);
uint32_t get_analog_format_colourspace(char *buf, int len);
uint32_t get_analog_format_color_matrix(char *buf, int len, int row, int col);
uint32_t get_clock_offset(char *buf, int len);
uint32_t get_sync_mode(char *buf, int len);
uint32_t get_send_input(char *buf, int len, int send_idx);
uint32_t get_send_scaleX(char *buf, int len, int send_idx);
uint32_t get_send_scaleY(char *buf, int len, int send_idx);
uint32_t get_send_posX(char *buf, int len, int send_idx);
uint32_t get_send_posY(char *buf, int len, int send_idx);
uint32_t get_send_rotation(char *buf, int len, int send_idx);
uint32_t get_send_pitch(char *buf, int len, int send_idx);
uint32_t get_send_yaw(char *buf, int len, int send_idx);
uint32_t get_send_brightness(char *buf, int len, int send_idx);
uint32_t get_send_contrast(char *buf, int len, int send_idx);
uint32_t get_send_saturation(char *buf, int len, int send_idx);
uint32_t get_send_hue(char *buf, int len, int send_idx);
uint32_t get_send_lut_Y(char *buf, int len, int send_idx);
uint32_t get_send_lut_R(char *buf, int len, int send_idx);
uint32_t get_send_lut_G(char *buf, int len, int send_idx);
uint32_t get_send_lut_B(char *buf, int len, int send_idx);
void set_analog_format_resolution(const char *s);
void set_analog_format_framerate(float v);
void set_analog_format_colourspace(const char *s);
void set_analog_format_color_matrix(int row, int col, float v);
void set_clock_offset(int v);
void set_sync_mode(const char *s);
void set_send_input(int send_idx, int v);
void set_send_scaleX(int send_idx, float v);
void set_send_scaleY(int send_idx, float v);
void set_send_posX(int send_idx, float v);
void set_send_posY(int send_idx, float v);
void set_send_rotation(int send_idx, float v);
void set_send_pitch(int send_idx, float v);
void set_send_yaw(int send_idx, float v);
void set_send_brightness(int send_idx, float v);
void set_send_contrast(int send_idx, float v);
void set_send_saturation(int send_idx, float v);
void set_send_hue(int send_idx, float v);
void set_send_lut_Y(int send_idx, float *v);
void set_send_lut_R(int send_idx, float *v);
void set_send_lut_G(int send_idx, float *v);
void set_send_lut_B(int send_idx, float *v);

#endif 

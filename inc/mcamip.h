#ifndef _MCAMIP_H_
#define _MCAMIP_H_


#define _GNU_SOURCE

/*#include <termcap.h>*/

#include <stdio.h>
#include <string.h>
//#include <strings.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <setjmp.h>

#include <jpeglib.h>

/* timeouts */
#define CONNECT_TIMEOUT     5
#define READ_TIMEOUT		5

/* buffer sizes */
#define TEMP_SIZE 65535
#define HTTP_BUFFER_SIZE	1000000
#define JPEG_BUFFER_SIZE	1000000

/* for text on screen (clock) */
#define TOP_LEFT		0
#define TOP_MIDDLE		1
#define TOP_RIGHT		2
#define BOTTOM_LEFT		3
#define BOTTOM_MIDDLE	4
#define BOTTOM_RIGHT	5

/* global vars */
int debug_flag;
struct passwd *userinfo;
char *home_dir;
char *user_name;
int quiet_flag;
int no_color_flag;
unsigned long long frame_number;
int show_frame_number_flag;

char *http_buffer;
int http_bytes;

char *http_server;
int http_port;
int http_server_status;
int connect_to_http_server_timeout;
struct timeval timeout;
char server_ip_address[512];

char *jpeg_buffer; // the jpeg picture data a filtered out from the camera stream.

int content_length;

double dframes_per_second;
double dreal_frames_per_second;
int color_depth_mode;
int color_depth;

int bytes_send;

int window_open_flag;
char *xbuffer;
int x_display;
int yuv_output;
int jpeg_output;
int stream_output;
int motion_detection;
int record_flag;
char *frame_dir;
int yuv_header_send_flag;

unsigned char *rgb24_buffer;
unsigned char *yuv_buffer;
unsigned char *yuv_buffer_prev;
int time_flag;
int clock_position;
int use_utc_flag;
int report_read_timeout_flag;

struct timeval *start_timeval;
struct timeval *current_timeval;
char *semaphore_filename;
int jpeg_sequence_number;

/* for on screen text */
int on_screen_text;
	
// this last, so all things are defined.
#include "mcamip_proto.h"

/* CCIR spec, taken from ffmpeg source, to match their colors */
#define SCALEBITS 10
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
#define ONE_HALF  (1 << (SCALEBITS - 1))

#define RGB_TO_Y_CCIR(r, g, b) \
((FIX(0.29900*219.0/255.0) * (r) + FIX(0.58700*219.0/255.0) * (g) + \
  FIX(0.11400*219.0/255.0) * (b) + (ONE_HALF + (16 << SCALEBITS))) >> SCALEBITS)

#define RGB_TO_U_CCIR(r1, g1, b1, shift)\
(((- FIX(0.16874*224.0/255.0) * r1 - FIX(0.33126*224.0/255.0) * g1 +         \
     FIX(0.50000*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)

#define RGB_TO_V_CCIR(r1, g1, b1, shift)\
(((FIX(0.50000*224.0/255.0) * r1 - FIX(0.41869*224.0/255.0) * g1 -           \
   FIX(0.08131*224.0/255.0) * b1 + (ONE_HALF << shift) - 1) >> (SCALEBITS + shift)) + 128)


#define DCS_900_DEFAULT_PORT				80
#define DCS_900_DEFAULT_ADDRESS				"10.0.0.151"
#define DEFAULT_FRAME_SPEED					20.0
#define DEFAULT_MOTION_DETECTION_THRESHOLD	10000

#define PVERSION							"0.7.8"


#endif /* _MCAMIP_H_ */


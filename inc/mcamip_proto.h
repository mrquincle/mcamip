#ifndef _MCAMIP_PROTO_H_
#define _MCAMIP_PROTO_H_

/*
mcamip:  display and motion detection for D-Link DCS-900 internet webcam.

mcamip is registered Copyright (C) 2005 <Jan Panteltje>
email: panteltje@yahoo.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

extern int text_to_yuv_buffer(\
	char *text, int x, int y,\
	int fr, int fg, int fb,\
	int br, int bg, int bb,\
	int xsize, int ysize, unsigned char *buffer);
int text_to_rgb24_buffer(\
	char *text, int x, int y,\
	int fr, int fg, int fb,\
	int br, int bg, int bb,\
	int xsize, int ysize, unsigned char *buffer);
extern char *get_time_as_string(int short_format_flag, int ut_flag);
extern int add_clock(unsigned char *buffer, int xsize, int ysize);
extern char *getbuf(void);
extern void putimage(int xsize, int ysize);
extern int openwin(int argc, char *argv[], int xsize, int ysize);
extern void closewin(void);
extern int wingetdepth(void);
extern int resize_window(int xsize, int ysize);
extern char *base64_encode_line(const char *s);
extern char *strsave(char *s);
extern int print_usage();
extern int connect_to_server(char *server, int port, int *socketfd);
extern int send_to_server(int socketfd, char txbuf[]);
extern int process_jpeg_buffer(int argc, char **argv, int treshhold);
extern int main(int argc, char **argv);

//extern void my_error_exit (j_common_ptr cinfo);
extern int video_display();




#endif // _MCAMIP_PROTO_H_


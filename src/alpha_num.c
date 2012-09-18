#include "mcamip.h"
#include "txtfont.h"

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

char *get_time_as_string(int short_format_flag, int ut_flag)
{
time_t now;
char temp[1024];
struct tm *universal_time, *local_time;
char *ptr;

if(debug_flag)
	{
	fprintf(stderr,\
	"get_time_as_string(): arg short_format_flag=%d ut_flag=%d\n",\
	short_format_flag, ut_flag);
	}

/* 
decided to adapt to universal time (UT == GMT).
New format:
from rfc1036.html:
Wdy, DD Mon YY HH:MM:SS TIMEZONE 
*/

now = time(0);
if(ut_flag) 
	{
	universal_time = gmtime(&now);
	if(short_format_flag)
		{
		strftime(temp, 511, "%b %d %Y %H:%M:%S UT", universal_time);
		}
	else
		{
		strftime(temp, 511, "%a, %d %b %Y %H:%M:%S UTC", universal_time);
		}
	}
else /* local time */
	{
	local_time = localtime(&now);

	if(short_format_flag)
		{
		strftime(temp, 511, "%b %d %Y %H:%M:%S", local_time);
		}
	 else
	 	{
		strftime(temp, 511, "%c", local_time);
		}
	}

ptr = strsave(temp);
if(! ptr)
	{
	fprintf(stderr, "get_time_as_string(): cannot allocate space for temp\n");
	return 0;
	}

if(debug_flag)
	{
	fprintf(stderr, "get_time_as_string(): ptr=%s\n", ptr);
	}

return(ptr);
} /* end function get_time_as_string */


/*
add_clock adds a clock with time and date in the correct place and format in a gstream format buffer.
*/
int add_clock(unsigned char *buffer, int xsize, int ysize)
{
char *ptr;
int text_xpos, text_ypos;

if(debug_flag)
	{
	fprintf(stderr, "add_clock(): arg buffer=%p xsize=%d ysize=%d\n", buffer, xsize, ysize);
	}

if(! buffer) return 0;
if(xsize <= 0) return 0;
if(ysize <= 0) return 0;

/* get the time */
if(xsize == 640)
	{
	ptr = (char *)get_time_as_string(0, use_utc_flag);
	}
else
	{
	ptr = (char *)get_time_as_string(1, use_utc_flag);
	}

if(! ptr) return 0;

/* print the time at the required position */
text_xpos = 0; // -Wall
text_ypos = 0; // -Wall
/* short_format_flag, ut_flag */
/* print short or long format time */
if(clock_position == TOP_LEFT)
	{
	text_xpos = 0;
	text_ypos = 0;
	}
else if(clock_position == TOP_MIDDLE)
	{
	text_xpos = (xsize - (strlen(ptr) * TXT_CHAR_WIDTH) ) / 2;
	text_ypos = 0;
	}
else if(clock_position == TOP_RIGHT)
	{
	text_xpos = xsize - (strlen(ptr) * TXT_CHAR_WIDTH) - 1;
	text_ypos = 0;
	}
else if(clock_position == BOTTOM_LEFT)
	{
	text_xpos = 0;
	text_ypos = ysize - TXT_CHAR_HEIGHT; 
	}
else if(clock_position == BOTTOM_MIDDLE)
	{
	text_xpos = (xsize - (strlen(ptr) * TXT_CHAR_WIDTH) ) / 2;
	text_ypos = ysize - TXT_CHAR_HEIGHT; 
	}
else if(clock_position == BOTTOM_RIGHT)
	{
	text_xpos = xsize - (strlen(ptr) * TXT_CHAR_WIDTH) - 1;
	text_ypos = ysize - TXT_CHAR_HEIGHT; 
	}

/* time to yuv buffer */
if(! text_to_rgb24_buffer(ptr, text_xpos, text_ypos,  255, 255, 255,  0, 0, 0,  xsize, ysize, buffer) )
	{
	free(ptr);
	return 0;
	}

/* free the time string, was allocated by get_time_as_string */
free(ptr);

return 1;
} /* end fuction add_clock */


/*
text_to_rgb24_buffer adds a text at pos x, y in the picture
*/
int text_to_rgb24_buffer(\
	char *text, int x, int y,\
	int fr, int fg, int fb,\
	int br, int bg, int bb,\
	int xsize, int ysize, unsigned char *buffer)
{
unsigned char *font;
int fheight, fwidth;
int register bits, row, column, yp, xp;
unsigned char *rowptr;
unsigned char *plotptr;
unsigned char *fp;

if(debug_flag)
	{
	fprintf(stderr,\
	"text_to_yuv_buffer(): arg text=%s\n\
	x=%d y=%d\n\
	fr=%d fg=%d fb=%d\n\
	br=%d bg=%d bb=%d\n\
	xsize=%d ysize=%d buffer=%p\n",\
	text, x, y, fr, fg, fb, br, bg, bb, xsize, ysize, buffer);
	}

/* argument check */
if(! text) return 0;
if(! buffer) return 0;
//if(x < 0) return 0; // allow text scoll left
//if(y < 0) return 0; // allow text scroll up
if(x > xsize) return 0;
if(y > ysize) return 0;
if(xsize <= 0) return 0;
if(ysize <= 0) return 0;

font = txtfont;
fwidth = TXT_CHAR_WIDTH;
fheight = TXT_CHAR_HEIGHT;

while(*text)
	{
	fp = font + fheight * *text;
	for (row = 0; row < fheight; row++)
		{
//		if( (y + row) < 0) continue; // prevent neg register values

		yp = y + row;

		/* y boundary check */
		if(yp >= (ysize - 1) ) continue;
		if(yp <= 0) continue;

		/* calculate start top left character */
		rowptr = buffer + (yp * xsize * 3);

		bits = *fp++;
		for (column = 0; column < fwidth; column++)
			{
//			if( (x + column) < 0) continue; // prevent left scroll from appearing at right again.

			xp = x + column;
			
			/* x boundary check */
			if(xp >= (xsize - 1) ) continue;
			if(xp <= 0) continue;
			
			/* calculate the address in the buffer */
			plotptr = rowptr + (xp * 3);			

			if(plotptr < buffer) continue;
			if(plotptr > (buffer + (xsize * ysize * 3) - 3) ) continue;

			if(bits & (0x80 >> column))
				{
				/* plot a dot in fg color, RGB sequence */
				*plotptr = fr;
				*(plotptr + 1) = fg; 
				*(plotptr + 2) = fb; 
				}
			else		
				{
				/* plot a dot in background color */
				*plotptr = br;
				*(plotptr + 1) = bg; 
				*(plotptr + 2) = bb; 
				}
			}/* end for all columns in character */
		}/* end for all rows in character */
	text++;
	x += fwidth;
	}/* end while all characters in text */

return 1;
} /* end function text_to_rgb24_buffer */ 



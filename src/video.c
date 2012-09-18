#include "mcamip.h"

#include "txtfont.h"
#include "limits.h"

#include "jpeglib.h"

#define X_DISPLAY

/* declaration in stdio.h is there, but does not work */
extern FILE *fmemopen (void *__s, size_t __len, __const char *__modes) __THROW;


struct jpeg_error_manager
{
struct jpeg_error_mgr pub;	/* "public" fields */

jmp_buf setjmp_buffer;	/* for return to caller */
};


typedef struct jpeg_error_manager * my_error_ptr;


/*
Here's the routine that will replace the standard error_exit method:
*/
METHODDEF(void) libjpeg_error_exit (j_common_ptr cinfo)
{
char *ptr;

/*
cinfo->err really points to a jpeg_error_manager struct, so coerce pointer
*/
my_error_ptr myerr = (my_error_ptr) cinfo->err;

/* 
Display the message.
We could postpone this until after returning, if we chose.
*/

ptr = get_time_as_string(0, 0);
fprintf(stderr, "mcamip: %s libjpeg_error_exit(): libjpeg error is ", ptr);
(*cinfo->err->output_message) (cinfo);
free(ptr);

/* Return control to the setjmp point */
longjmp(myerr->setjmp_buffer, 1);
} /* end function libjpeg_error_exit */


int process_jpeg_buffer(int argc, char **argv, int threshold)
{
int a, i, k;
double frame_time, fb;
int width;
int height;
int r, g, b;
int tmp;
struct jpeg_decompress_struct cinfo;
struct jpeg_error_manager jerr;
FILE *infile;				/* source file */
JSAMPARRAY buffer;			/* Output row buffer */
int row_stride;				/* physical row width in output buffer */
static int have_tried_flag;	/* retried putimage on first image, else a white picture, why? */
double y, u, v;
double cr, cg, cb, cu, cv;
unsigned char *y_ptr, *u_ptr, *v_ptr;
int xa, ya;

int motion_detected;
int yuv_buffer_length;
int y_difference, u_difference, v_difference;
int change_level = 0; // -Wall
int diff;
unsigned char *y_ptr_p, *u_ptr_p, *v_ptr_p;

/* for text on screen */
unsigned char *font;
int fheight, fwidth;
char temp[TEMP_SIZE];
FILE *ofptr;
FILE *fptr;
char *ptr, *ptr2;
unsigned char *uptr;
size_t line_size;
time_t this_time;
static time_t old_time = 0;
static char os_text[480][80]; // max 80 characters of 8 width in 640 x space 
static int valid_text;
static int text_line;
static int os_xpos[480], os_ypos[480];
static int os_fr[480], os_fg[480], os_fb[480];
static int os_br[480], os_bg[480], os_bb[480];

/*
For average in motion detection,
patch by Robert Light <robert_light@verizon.net> to make motion detection work in low light.
*/
float avgY;
int avgYcount;

if(debug_flag)
	{
	fprintf(stderr,	"process_jpeg_buffer(): arg argc=%d arv[0]=%s threshold=%d\n",\
	argc, argv[0], threshold);
	}

a = 0; // -Wall

/* pick a color standard */


/* Panteltje's y spec */ cr = 0.3; cg = 0.59; cb = 0.11;

// /* ITU-601-1 Y spec */ cr = 0.299; cg = 0.587; cb = 0.114; /* used by ffplay and likely mplayer */

// /*  CIE-XYZ 1931 Kodak, postscript pdf Y spec */ cr = 0.298954; cg = 0.586434; cb = 0.114612;

// /* ITU-709 NTSC Y spec */ cr = 0.213; cg = 0.715; cb = 0.072;

// /* ITU/EBU 3213 PAL Y spec */ cr = 0.222; cg = 0.707; cb = 0.071;

/* U spec */
cu = .5 / (1.0 - cb);

/* V spec */
cv = .5 / (1.0 - cr);

font = txtfont;
fwidth = TXT_CHAR_WIDTH;
fheight = TXT_CHAR_HEIGHT;

infile = (FILE *)fmemopen(jpeg_buffer, content_length, "r");
if(! infile)
	{
	if(debug_flag)
		{
		fprintf(stderr,\
		"process_jpeg_buffer(): could not open jpeg_buffer for read.\n");
		}

	return 0;
	}

/* allocate and initialize JPEG decompression object */

/* We set up the normal JPEG error routines, then override error_exit. */
cinfo.err = jpeg_std_error(&jerr.pub);

jerr.pub.error_exit = libjpeg_error_exit;

if(setjmp(jerr.setjmp_buffer))
	{
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	if(debug_flag)
		{
		fprintf(stderr, "process_jpeg_buffer(): setjmp\n");
		}

	/* Do not disconnect from the camera in case of some wrong bytes in jpeg buffer, but retry */
	return 1;
//	return 0;
	}

/* Now we can initialize the JPEG decompression object. */
jpeg_create_decompress(&cinfo);

/* specify data source (eg, a file) */
jpeg_stdio_src(&cinfo, infile);

/* read file parameters with jpeg_read_header() */
(void) jpeg_read_header(&cinfo, TRUE);

/* 
set parameters for decompression
Don't need to change any of the defaults set by jpeg_read_header(),
so we do nothing here.
*/

/* Start decompressor */
(void) jpeg_start_decompress(&cinfo);

/* 
Make an output work buffer of the right size.
JSAMPLEs per row in output buffer
*/
row_stride = cinfo.output_width * cinfo.output_components;
width = cinfo.output_width;
height = cinfo.output_height;

if(debug_flag)
	{
	fprintf(stderr,\
	"process_jpeg_buffer(): width=%d height=%d component=%d\n",\
	width, height, cinfo.output_components); 

	fprintf(stderr, "process_jpeg_buffer(): cinfo_out_color_space=%d\n",\
	cinfo.out_color_space);

	if(cinfo.out_color_space != JCS_RGB)
		{
		fprintf(stderr, "process_jpeg_buffer(): no RGB color space\n");
		}
	else fprintf(stderr, "process_jpeg_buffer(): RGB color space\n");
	}

#ifdef X_DISPLAY
if(x_display)
	{
	/* open a window for the size we now know */
	if(!window_open_flag)
		{
		openwin(argc, argv, width, height);
		window_open_flag = 1;
		}

	/* get the xbuffer */
	xbuffer = (char *)getbuf();
	if(! xbuffer)
		{
//		if(debug_flag)
			{
			fprintf(stderr, "process_jpeg_buffer(): could not get xbuffer\n");
			}

//		return 0;
		}

	/* set color_depth (if color_depth_mode == 0) */
	wingetdepth();

	if(debug_flag)
		{
		fprintf(stderr, "process_jpeg_buffer(): colordepth=%d\n", color_depth);
		}

	} /* end if x_display */
#endif

if(! rgb24_buffer)
	{
	rgb24_buffer = (unsigned char *)malloc(width * height * 3);
	if(! rgb24_buffer)
		{	
		fprintf(stderr, "mcamip: process_jpeg_buffer(0: malloc rgb24_buffer failed, aborting\n");

		exit(1);
		}	
	}

if(yuv_output)
	{
	if(! yuv_header_send_flag)
		{
		/* send the mjpeg tools yuv stream header */

		/* stream stream header to get width and height and interlace */

		/*
		YUV4MPEG2 W720 H576 F25:1 I? A0:0 XM2AR002
		FRAME
		data
		*/

		fprintf(stdout,\
		"YUV4MPEG2 W%d H%d F%d:1 I? A0:0 XM2AR002\n", width, height, (int)dframes_per_second);

		yuv_header_send_flag = 1;
		}
	} /* end if yuv_output */

/*
Make a one-row-high sample array that will go away when done with image
*/
buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

/* 
while (scan lines remain to be read)
           jpeg_read_scanlines(...);
Here we use the library's state variable cinfo.output_scanline as the
loop counter, so that we don't have to keep track ourselves.
*/

uptr = rgb24_buffer;
//WARNING programmer beware!!! cinfo.output_scanline starts at 1 
for(ya = 0; ya < cinfo.output_height; ya++) 
	{
	/*
	jpeg_read_scanlines expects an array of pointers to scanlines.
	Here the array is only one element long, but you could ask for
	more than one scanline at a time if that's more convenient.
	*/
	(void) jpeg_read_scanlines(&cinfo, buffer, 1);

	for(i = 0; i < row_stride; i += 3)
		{
		r = (int)buffer[0][i];
		g = (int)buffer[0][i + 1];
		b = (int)buffer[0][i + 2];

		*uptr++ = r;
		*uptr++ = g;
		*uptr++ = b;			
		} /* end for i */

	} /* end while all scan lines */

if(show_frame_number_flag)
	{
	sprintf(temp, "%llu", frame_number);

	if(! text_to_rgb24_buffer(temp,  0, 0,  0, 255, 0,  0, 0, 0,  width, height,  rgb24_buffer) )
		{
		fprintf(stderr, "mcamip: text_to_rgb24_buffer() failed, could not print frame_number.\n"); 

//		exit(1);
		}
	} /* end if show_frame_number_flag */

if(time_flag)
	{
	if(! add_clock(rgb24_buffer, width, height) )
		{
		fprintf(stderr, "mcamip: process_jpeg_buffer(): add_clock() failed, aborting.\n");

		exit(1);
		}

	} /* end if time flag */
		
if(on_screen_text)
	{
	/* get any text to be displayed */
	this_time = time(0);
	if(this_time - old_time >= 1)		
		{
		old_time = this_time;
		sprintf(temp, "%s/mcamip_os.txt", home_dir);
		fptr = fopen(temp, "r");
		if(! fptr)
			{
			if(debug_flag)
				{
				fprintf(stderr,\
				"mcamip: process_jpeg_buffer(): could not open text file %s for read.\n",\
				temp);
				}

			valid_text = 0;
			}
		else		
			{
			/* for all lines */
			text_line = 0;
			while(1)
				{
				ptr = 0;
				line_size = 0;
				a = getline(&ptr, &line_size, fptr);

				if(debug_flag)
					{
					fprintf(stderr,\
					"on_screen_text: a=%d line_size=%ld ptr=%p ptr=%s\n",\
					a, line_size, ptr, ptr);
					}
	
				/* test if file could be read from */
				if(a == -1)
					{
					if(debug_flag)
						{
						fprintf(stderr,"mcamip: process_jpeg_buffer(): getline from %s failed\n", temp);
						}				

					/* prevent memory leak */					
					free(ptr);
					break;
					}
				else
					{
					/* if not a comment line */
					if(*ptr != '#')
						{
						b = sscanf(ptr, "%d %d  %d %d %d  %d %d %d :",\
						&os_xpos[text_line], &os_ypos[text_line],\
						&os_fr[text_line], &os_fg[text_line], &os_fb[text_line],\
						&os_br[text_line], &os_bg[text_line], &os_bb[text_line]);
						
						/* test if enough arguments */
						if(b != 8)
							{
							if(debug_flag)
								{
								fprintf(stderr,\
								"mcamip: process_jpeg_buffer(): sscanf did not return 8, but=%d\n", b);
								}
							}
						else
							{
							ptr2 = strchr(ptr, ':');
							if(!ptr2)
								{
								valid_text = 0;
								}
							else	
								{
								if(debug_flag)
									{
									fprintf(stderr, "on_screen_text: ptr2=%s\n", ptr2);
									}

								strncpy(os_text[text_line], ptr2 + 1, 80); // max 80 chars per line

								/* get rid of linefeed */
								//os_text[strlen(os_text) - 1] = 0;
								os_text[text_line][a - (ptr2 - ptr) - 2] = 0;

								valid_text = 1;
								}
							} /* end if b == 8 */
						} /* end if not '#' a comment */

					} /* end if a != -1 */

				free(ptr);			

				text_line++;
				if(text_line >= 480)
					{
					break;
					}
				} /* end while lines */

			fclose(fptr);
			} /* end if file opened OK */
			
		} /* end if time to update text */			
		
	if(valid_text)
		{
		for(i = 0; i < text_line; i++)
			{
			if(! text_to_rgb24_buffer(os_text[i],\
			os_xpos[i], os_ypos[i],\
			os_fr[i], os_fg[i], os_fb[i],\
			os_br[i], os_bg[i], os_bb[i],\
			width, height,\
			rgb24_buffer) )
				{
				if(debug_flag)
					{
					fprintf(stderr,\
					"mcamip: process_jpeg_buffer(): text_to_rgb24_buffer(): returned error.\n");
					}
				} 
			} /* end for all lines */
		} /* end if valid_text */

	} /* end if on_screen_text */

if(report_read_timeout_flag)
	{
	if(debug_flag)
		{
		fprintf(stderr,\
		"reporting_read_timeout report_read_timeout_flag=%d\n", report_read_timeout_flag);
		}

	/* position just above bottom clock space */
	a = height - 2 * TXT_CHAR_HEIGHT;

	/*
	int text_to_rgb24_buffer(\
	unsigned char *text, int x, int y,\
	int fr, int fg, int fb,\
	int br, int bg, int bb,\
	int xsize, int ysize, char *buffer)
	*/

	text_to_rgb24_buffer("WAS READ TIMEOUT, RECONNECTED TO CAMERA SERVER",\
	0, a, 255, 255, 255, 0, 0, 0, width, height, rgb24_buffer);

	report_read_timeout_flag++;

	frame_time = 1.0 / dframes_per_second;
		
	/* report for 4 seconds */
	fb = 4.0 / frame_time; 

	if(report_read_timeout_flag >= (int)fb) report_read_timeout_flag = 0;

	} /* end if report_read_timeout_flag */

/* rbgb24 to yuv and X */
/* for RGB to YUV */
y_ptr = yuv_buffer;
u_ptr = yuv_buffer + (width * height);
v_ptr = u_ptr + ( (width / 2) * (height / 2) );

// need bracket for gcc-2.95
	{
	/* reserve space for uv matrix */
	double up[cinfo.output_height][width];
	double vp[cinfo.output_height][width];

	uptr = rgb24_buffer;
	k = 0;
	for(ya = 0; ya < height; ya++)
		{
		for(xa = 0; xa < width; xa++)
			{
			/* convert to YUV */
			r = (int)*uptr++;
			g = (int)*uptr++;
			b = (int)*uptr++;

			/* test yuv coding here */
//			y = cr * r + cg * g + cb * b;
//			y = (219.0 / 256.0) * y + 16.5;  /* nominal range: 16..235 */

			y = RGB_TO_Y_CCIR(r, g, b);

			*y_ptr = y;
			y_ptr++;

//			u = cu * (b - y) + 128.0;
//			u = (224.0 / 256.0) * u + 128.5; /* 16..240 */

			u = RGB_TO_U_CCIR(r, g, b, 0);

//			v = cv * (r - y) + 128.0;
//			v = (224.0 / 256.0) * v + 128.5; /* 16..240 */

			v = RGB_TO_V_CCIR(r, g, b, 0);
			
			/*
			0 0
			0 x
			*/

			/* only on even rows and even columns do we output the average U and V for the 4 pixels */
			if(ya % 2)
				{
				if(xa % 2)
					{
					if(no_color_flag)
						{
						*u_ptr = 128.5;
						*v_ptr = 128.5;
						}
					else
						{	
						*u_ptr = ( (up[ya - 1][xa - 1] + up[ya - 1][xa] + up[ya][xa -1] + u ) / 4.0);
						*v_ptr = ( (vp[ya - 1][xa - 1] + vp[ya - 1][xa] + vp[ya][xa -1] + v ) / 4.0);
						}

					u_ptr++;
					v_ptr++;

					} /* end if odd pixel */

				} /* end if odd_line */

			if(x_display)
				{
				switch(color_depth)
					{
					case 16:
						tmp = b >> 3;
						tmp |= (int)(g >> 2) << 5;
						tmp |= (int)(r >> 3) << 11;

						xbuffer[k    ] = tmp & 0xff;
						xbuffer[k + 1] = tmp >> 8;
						k += 2;
						break;
					case 24:
						xbuffer[k    ] = b;
						xbuffer[k + 1] = g;
						xbuffer[k + 2] = r;
						k += 3;
    					break;
					case 32:
						xbuffer[k    ] = b;
						xbuffer[k + 1] = g;
						xbuffer[k + 2] = r;
						k += 4;
						break;
					} /* end switch colordepth */
		
				} /* end if x_display */

			/* fill uv matrix for lookback */
			up[ya][xa] = u;                  
			vp[ya][xa] = v;

			} /* end for xa */
		} /* end for ya */

	} /* end gcc-2.95 brackets */

if(yuv_output || jpeg_output)
	{
	yuv_buffer_length = (width * height) + ( (width / 2) * (height / 2) ) + ( (width / 2) * (height / 2) );

	/* detect motion by default */
	motion_detected = 1;
	if(motion_detection)
		{
		y_difference = 10;
		u_difference = 10;
		v_difference = 10;

		/* compare current yuv buffer to previous one */
		y_ptr = yuv_buffer;
		u_ptr = yuv_buffer + (width * height);
		v_ptr = u_ptr + ( (width / 2) * (height / 2) );
		
		y_ptr_p = yuv_buffer_prev;
		u_ptr_p = yuv_buffer_prev + (width * height);
		v_ptr_p = u_ptr_p + ( (width / 2) * (height / 2) );

		/* set for no change */
		change_level = 0;

		/* compare new to old y matrices */
		avgY = 0;
		avgYcount = 1;
		for(i = 0; i < width * height; i++)
			{
			avgY = (avgY*(avgYcount-1)+*y_ptr)/avgYcount;
			avgYcount++;
			diff = *y_ptr - *y_ptr_p;
			y_ptr++;
			y_ptr_p++;
			if( abs(diff) > y_difference)
				{
				change_level++;
				}
			}
//		fprintf(stderr, "AverageY=%f ", avgY);
		if(avgY <= 90 )
			{
			//increase threshold if it is getting dark - it will take more to register a motion
			threshold += 20000.*(90.-avgY)/55.; //increase by 0-20000 as it gets darker
			}

        if(avgY > 35)
        	{ //if it light enough
			/* compare new to old u matrices */
			for(i = 0; i < (width / 2) * ( height / 2); i++)
				{
				diff = *u_ptr - *u_ptr_p;
				if( abs(diff) > u_difference)
					{
					change_level++;
					}
				}

			/* compare new to old v matrices */
			for(i = 0; i < (width / 2) * ( height / 2); i++)
				{		
				diff = *v_ptr - *v_ptr_p;
				if( abs(diff) > v_difference)
					{
					change_level++;
					}
				}
				
			if(debug_flag)
				{
				fprintf(stderr, "process_jpeg_buffer(): motion_detection: change_level=%d\n", change_level); 
				}
	          
			/* set flag if treshhold exceeded */
			if(change_level < threshold) motion_detected = 0;
			}
		else
			{
			//if it is dark, don't detect motion
			motion_detected = 0;
			}
//		if( motion_detected )fprintf(stderr,"Motion detected, change_level=%d\n",change_level);
//		else fprintf(stderr,"  change_level=%d\n",change_level);
		if(debug_flag)
			{
			if(motion_detected) fprintf(stderr, "process_jpeg_buffer(): \n\nmotion detected!\n");
			else fprintf(stderr, "process_jpeg_buffer(): no motion detected.\n");
			}	

		/* save values so we can compare to those next time */ 
		memcpy(yuv_buffer_prev, yuv_buffer, yuv_buffer_length);

		if(motion_detected)
			{
			/* create semaphore file if motion detected and filename specified */
			if(semaphore_filename)
				{		
				fptr = fopen(semaphore_filename, "w");
				if(! fptr)
					{
//					ptr = get_time_as_string(0, 0);	

					fprintf(stderr,\
					"mcamip: ERROR could not create semaphore file %s  aborting.\n", semaphore_filename);

					exit(1);
					}

				fclose(fptr);
				} /* end if semaphore_filename */
			} /* end if motion_detected */ 
		} /* end if motion detection */

	if(motion_detected)
		{
		if(yuv_output)
			{
			/* output an mjpeg tools yuv frame header */
			/*
			FRAME-HEADER consists of
			 string "FRAME "  (note the space after the 'E')
			 unlimited number of ' ' separated TAGGED-FIELDs
 			 '\n' line terminator
			*/

			fprintf(stdout, "FRAME\n");

			/* output an mjpeg tools yuv frame */
			/* write Y */
			/* write Cb */
			/* write Cv */

			if(debug_flag)
				{
				fprintf(stderr, "yuv frame length=%d\n", yuv_buffer_length);
				}

			a = fwrite(yuv_buffer, sizeof(char), yuv_buffer_length, stdout); 
			if(a != yuv_buffer_length)
				{
				fprintf(stderr, "process_jpeg_buffer() output yuv: could only write %d of %d bytes, aborting.\n",\
				a, yuv_buffer_length);

				exit(1);
				}

			fflush(stdout);
			} /* end if yuv_output */

		if(jpeg_output)
			{
			for(i = jpeg_sequence_number; i < INT_MAX; i++)
				{
				sprintf(temp, "%s/mcamip.%d.jpg", frame_dir, i);
				ofptr = fopen(temp, "r");

				/* do not overwrite, find next one */
				if(ofptr)
					{
					fclose(ofptr);
					continue;
					}

				/* found free sequence number */
				break;
				} /* end for i */

			jpeg_sequence_number = i;
			ofptr = fopen(temp, "w");
			if(! ofptr)
				{	
				fprintf(stderr,\
				"mcamip: process_jpeg_buffer(): could not open file %s for write, aborting.\n", temp);

				exit(1);
				}
				
			/* write jpeg_buffer to this file */
			fwrite(jpeg_buffer, sizeof(char), content_length, ofptr); 	
			
			/* close file */
			fclose(ofptr);

			jpeg_sequence_number++;

			/* update the file */
			sprintf(temp, "%s/jpeg_sequence_number", frame_dir);
			fptr = fopen(temp, "w");
			if(fptr)
				{
				fprintf(fptr, "%d", jpeg_sequence_number);

				fclose(fptr);
				}
			else
				{
				fprintf(stderr, "mcamip: could not write to %s, aborting.\n", temp);

				exit(1);
				}
			} /* end if jpeg_output */

		} /* end in no motion detection, or motion is detected */

	} /* end if yuv_output or jpeg_output */
	
/* Finish decompression */
(void) jpeg_finish_decompress(&cinfo);

/* Release JPEG decompression object */
jpeg_destroy_decompress(&cinfo);

fclose(infile);

/* 
At this point you may want to check to see whether any corrupt-data
warnings occurred (test whether jerr.pub.num_warnings is nonzero).
*/
if(jerr.pub.num_warnings)
	{
	ptr = get_time_as_string(0, 0); 	
	fprintf(stderr, "jpeg lib warnings=%ld occurred at %s\n", jerr.pub.num_warnings, ptr);
	free(ptr);
	}

#ifdef X_DISPLAY
if(x_display)
	{
	/* display the image */
	putimage(width, height);

	/* on the very first image a white screen is the result, this retries */
	if(! have_tried_flag)
		{
		putimage(width, height);

		have_tried_flag = 1;
		}
	} /* end if x_display */
#endif

return 1;
} /* end function process_jpeg_buffer */


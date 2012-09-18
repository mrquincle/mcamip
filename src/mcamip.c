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


                            /* set TABS to 4 to read this */

#include "mcamip.h"


char *base64_encode_line(const char *s)
{
/* this function taken from wget http.c */
static char tbl[64] =
	{
	'A','B','C','D','E','F','G','H',
	'I','J','K','L','M','N','O','P',
	'Q','R','S','T','U','V','W','X',
	'Y','Z','a','b','c','d','e','f',
	'g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v',
	'w','x','y','z','0','1','2','3',  
	'4','5','6','7','8','9','+','/'
	};

int len, i;
char *res;
unsigned char *p;
       
len = strlen(s);
res = (char *)malloc(4 * ((len + 2) / 3) + 1);
p = (unsigned char *)res;

/* Transform the 3x8 bits to 4x6 bits, as required by base64. */
for(i = 0; i < len; i += 3)
	{
	*p++ = tbl[s[0] >> 2];
	*p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
	*p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
	*p++ = tbl[s[2] & 0x3f];
	s += 3;
	}

/* Pad the result if necessary... */
if (i == len + 1) *(p - 1) = '=';
else if (i == len + 2) *(p - 1) = *(p - 2) = '=';

/* ...and zero-teminate it.  */
*p = '\0';

return res;
} /* end function base64_encode_line */


char *strsave(char *s) /* save char array s somewhere */
{
char *p;

p = malloc(strlen(s) +  1);
if(p) strcpy(p, s);

return p;
} /* end function strsave */


int print_usage()
{
fprintf(stderr, "\
Usage:\n\
mcamip -a IP address [-b] [-c] [-d frame_dir] [-e motion sens] [-f fps] [-g] [-h] [-i semaphore_filename] \n\
[-j] [-n] -p IP port [-q] [-s clockpos] [-t] [-u username] [-v] [-w password] [-x] [-y] [-z]\n\n\
-a string            camera URL IP address in decimal with points, or as a URL, default %s\n\
-b                   force black and white to YUV out (color off)\n\
-c                   activate change detection (motion detection), default off, -j or -y must be present\n\
-d string            directory name where single frames are stored, default ~/frames\n\
-e int               motion detection threshold, more is less sensitive, default %d\n\
-f float             speed in frames per second, default %.2f\n\
                      please set camera to auto in video setup\n\
-g                   display in UTC time\n\
-h                   help (this help)\n\
-i string            filename of semaphore file created when motion detected, default none,\n\
                      use with -j or -y, and -c and -e, the program using the semaphore file should erase it.\n\
-j  	             output jpeg single pictures with incrementing numbers in framedir\n\
-n                   show frame number since start\n\
-o                   read and display on screen text in mcamip format from ~/mcamip_os.txt every second\n\
-p int               camera IP port in decimal, default %d\n\
-q                   quit, no messages to stderr.\n\
-s int               clock position, 0=top_left, 1=top_middle, 2=top_right,\n\
                      3=bottom_left, 4=bottom_middle, 5=bottom_right, default bottom_middle\n\
-t                   show time in yuv output\n\
-u username          username for access control\n\
-v                   debug, prints functions and arguments\n\
-w password          password for access control\n\
-x                   show X windows display\n\
-y                   send mjpeg tools yuv format to stdout\n\
-z                   send full camera stream to stdout\n\
",\
DCS_900_DEFAULT_ADDRESS, DEFAULT_MOTION_DETECTION_THRESHOLD, DEFAULT_FRAME_SPEED, DCS_900_DEFAULT_PORT);

return 1;
} /* end function print_usage */


int connect_to_server(char *server, int port, int *socketfd)
{
struct hostent *hp;
struct sockaddr_in sa;
int a;
time_t connect_timer;
char *ptr;

if(! server) return 0;
if(port <= 0) return 0;

if(! quiet_flag)
	{
	ptr = get_time_as_string(0, 0);
	fprintf(stderr, "mcamip: %s getting host %s by name\n", ptr, server);
	free(ptr);
	}
	
hp = gethostbyname(server);
if(hp == 0)
	{
	fprintf(stderr,\
	"gethostbyname: returned NULL cannot get host %s by name.\n", server);

	/* signal FD_SET (main) that this is no longer a valid filedescriptor */
	*socketfd = -1;
	return 0;
	}

/* gethostbyname() leaves port and host address in network byte order */

bzero(&sa, sizeof(sa) );
bcopy(hp->h_addr, (char *)&sa.sin_addr, hp->h_length);


sa.sin_family = AF_INET;
sa.sin_port = htons( (u_short)port);

/* sa.sin_addr and sa.sin_port now in network byte order */

/* create a socket */
*socketfd = socket(hp->h_addrtype, SOCK_STREAM, 0);
if(*socketfd < 0)
	{
	fprintf(stderr, "socket failed\n");
	*socketfd = -1;
	return(0);
	}

/* set for nonblocking socket */
if (fcntl(*socketfd, F_SETFL, O_NONBLOCK) < 0)
	{
	return(0);
	}

sprintf(server_ip_address, "%s", inet_ntoa (sa.sin_addr) );

if(! quiet_flag)
	{
	fprintf(stderr,\
	"mcamip: connecting to %s (%s) port %d timeout %d\n",\
	server, server_ip_address, port, connect_to_http_server_timeout);
	}
	
/* prevent the program from hanging if connect takes a long time, now a return 0 is forced. */
       
/* start the timer */
connect_timer = time(0);

/* keep testing for a connect */
while(1)
	{
	/* connect */
	a = connect(*socketfd, (struct sockaddr*)&sa, sizeof(sa) );
	if(a == 0) break; /* connected */
	if(a < 0)
		{
		if(debug_flag)
			{
			fprintf(stderr, "mcamip: connect() failed because: ");
			perror("");
			}

		/* test for connect time out */
		if( (time(0) - connect_timer) > connect_to_http_server_timeout)
			{
			/* close the socket */
			close(*socketfd);
			
			/* set socketfd to invalid, it was valid! */
			*socketfd = -1;
			fprintf(stderr, "mcamip: connect timeout\n");

			return 0;
			}

		}/* end connect < 0 */
	}/* end while test for a connect */

/* now set socket blocking */
//flags = fcntl(*socketfd, F_GETFL, 0);
//flags &= ~O_NONBLOCK;
//fcntl(*socketfd, F_SETFL, flags);

#define KEEP_ALIVE
#ifdef KEEP_ALIVE_
b = sizeof(int);
//int getsockopt (int SOCKET, int LEVEL, int OPTNAME, void *OPTVAL, socklen_t *OPTLEN-PTR)
a = getsockopt(*socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &b); 
if(debug_flag)
	{
	fprintf(stderr, "SO_KEEPALIVE: a=%d optval=%d\n", a, optval); 
 	}

optval = 1;
//int setsockopt (int SOCKET, int LEVEL, int OPTNAME, void *OPTVAL, socklen_t OPTLEN);
a = setsockopt(*socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int) );
if(debug_flag)
	{
	fprintf(stderr, "setsockopt returned a=%d b=%d\n", a, b);
	}

a = getsockopt(*socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &b); 
if(debug_flag)
	{
	fprintf(stderr, "getsockopt: SO_KEEPALIVE: a=%d optval=%d b=%d\n", a, optval, b); 
	}
#endif // KEEP_ALIVE_
	
return 1;
} /* end function connect_to_server */


int send_to_server(int socketfd, char txbuf[]) /* send data to server */
{
int a;

if(debug_flag)
	{
	fprintf(stderr, "send_to_server(): socketfd=%d txbuf=%p txbuf=\n\%s\n", socketfd, txbuf, txbuf);
	}

if(socketfd < 0) return 0;
if(! txbuf) return 0;

a = write(socketfd, txbuf, strlen(txbuf) );     
if(a < 0)
	{
	fprintf(stderr, "mcamip: send_to_server(): write failed because ");
	perror("");

	return 0;
	}

return 1;
} /* end function send_to_server */


int main(int argc, char **argv)
{
int a;
char temp[TEMP_SIZE];
int socketfd;
int frames;
char *http_bptr;
char *ptr;
char *start_ptr;
char *username;
char *password;
char *access_string;
time_t read_timer;
int restart_flag;
int reconnects;
int threshold;
double dmilli_seconds_per_frame;
unsigned long lstart_usecs, lcurrent_usecs, ldiff_usecs;
FILE *fptr;
char *window_border_info[1];

/*select con io 1 byte at the time*/
setbuf(stdout, NULL);
setbuf(stdin, NULL);

/* get user info */
userinfo = getpwuid(getuid() );

/* get home directory */
home_dir = strsave(userinfo -> pw_dir);
user_name = strsave(userinfo -> pw_name);

username = strsave("");
password = strsave("");

/* set defaults */
debug_flag = 0;
quiet_flag = 0;
no_color_flag = 0;
x_display = 0;
motion_detection = 0;
yuv_output = 0;
jpeg_output = 0;
dframes_per_second = DEFAULT_FRAME_SPEED;
time_flag = 0;
clock_position = BOTTOM_MIDDLE;
use_utc_flag = 0;
threshold = DEFAULT_MOTION_DETECTION_THRESHOLD;

http_server = strsave(DCS_900_DEFAULT_ADDRESS);
if(! http_server)
	{
	fprintf(stderr, "mcamip: could not allocate space for http_server, aborting.\n");
	exit(1);
	}

http_port = DCS_900_DEFAULT_PORT;

connect_to_http_server_timeout = CONNECT_TIMEOUT;

sprintf(temp, "%s/mypictures/frames", home_dir);
frame_dir = strsave(temp);
if(! frame_dir)
	{
	fprintf(stderr, "mcamip: could not allocate space for frame_dir, aborting.\n");

	exit(1);
	}
else {
	fprintf(stdout, "%s: opened %s for frame directory \n", __func__, frame_dir );
}

/* for on screen user text */
on_screen_text = 0;
//os_xpos = 10;
//os_ypos = 4;
//os_fr = 200;
//os_fg = 255;
//os_fb = 170;

//os_br = 100;
//os_bg = 100;
//os_bb = 20;

show_frame_number_flag = 0;

/* end defaults */

/* get command line options */
while(1)
	{
	a = getopt(argc, argv, "a:cbd:e:f:ghi:jnop:qs:tu:vw:xyz");
	if(a == -1) break;

	switch(a)
		{
		case 'a': // camera internet IP address
			http_server = strsave(optarg);
			if(! http_server)
				{
				fprintf(stderr, "mcamip: could not allocate space for http_server, aborting.\n");
				exit(1);
				}

			break;
		case 'b': // force BW
			no_color_flag = 1;
			break;
		case 'c':// enable motion (change) detection 
			motion_detection = 1;
			break;
		case 'd':// specify frame_dir
			free(frame_dir);
			frame_dir = strsave(optarg);
			if(! frame_dir)
				{
				fprintf(stderr, "mcamip: could not allocate space for frame_dir, aborting.\n");

				exit(1);
				}
			break;
		case 'e': // motion detection threshold
			threshold = atoi(optarg);
			break;
		case 'f': // set frames per second for YUV output (mplayer will display in that speed)
			dframes_per_second = atof(optarg);
			break;
		case 'g': // display in UTC time
			use_utc_flag = 1;
			break;
		case 'h': // help
			print_usage();
			exit(1);
			break;
		case 'i':
			semaphore_filename = strsave(optarg);
			if(! semaphore_filename)
				{
				fprintf(stderr, "mcamip: could not alloce space for semaphore_filename, aborting.\n");

				exit(1);
				}
			break;
		case 'j': // jpeg output
			jpeg_output = 1;
			break;
		case 'n': // show frame number
			show_frame_number_flag = 1;
			break;
		case 'o':
			on_screen_text = 1;
			break;
		case 'p': // camera internet port
			sscanf(optarg, "%i", &http_port);
			break;
		case 'q':
			quiet_flag = 1;
			break;
		case 't': // show time in display
			time_flag = 1;
			break;
		case 'u': // username 
			free(username);
			username = strsave(optarg);
			if(! username)
				{
				fprintf(stderr, "mcamip: could not allocate space for user_name, aborting.\n");

				exit(1);
				}
			break;
		case 's': // clock position
			clock_position = atoi(optarg);
			break;
		case 'v': // verbose / debug
			debug_flag = 1;
			break;
		case 'w': // password
			free(password);
			password = strsave(optarg);
			if(! password)
				{
				fprintf(stderr, "mcamip: could not allocate space for password, aborting.\n");

				exit(1);
				}
			break;
		case 'x': // x windows display
			x_display = 1;
			break;
		case 'y': // enable mjpeg tools yuv output to stdout.
			yuv_output = 1;
			break;
		case 'z':
			stream_output = 1;
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 				{
 				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
 				}
			else
				{
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				}
			print_usage();

			exit(1);
			break;			
		default:
			print_usage();

			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

/* ID */
if(! quiet_flag)
	{
	fprintf(stderr, "mcamip-%s copyright Jan Panteltje 21st century.\n", PVERSION);
	}

/* sanity check */
if( (! yuv_output) && (! jpeg_output) && (! x_display) )
	{
	fprintf(stderr,\
	"mcamip: ERROR: no output specified, use at least one of the -j, -x, or -y options, aborting.\n");
	fprintf(stderr, "mcamip -h for help\n");
	exit(1);
	}

if(yuv_output && stream_output)
	{
	fprintf(stderr, "mcamip: ERROR: both yuv output (-y) and stream output (-z) requested to stdout, aborting.\n");
	fprintf(stderr, "mcamip -h for help\n");
	exit(1);
	}

dmilli_seconds_per_frame = 1000.0 / (double)dframes_per_second;

start_timeval = malloc(sizeof(struct timeval) );
if(!start_timeval)
	{
	fprintf(stderr, "mcamip: could not allocate space for start_timeval, aborting.\n");

	exit(1);
	}

current_timeval = malloc(sizeof(struct timeval) );
if(!current_timeval)
	{
	fprintf(stderr, "mcamip: could not allocate space for current_timeval, aborting.\n");

	exit(1);
	}

http_buffer = (char *)malloc(HTTP_BUFFER_SIZE);
if(! http_buffer)
	{
	fprintf(stderr, "Could not allocate space for http_buffer, aborting.\n");

	exit(1);
	}

jpeg_buffer = (char *)malloc(JPEG_BUFFER_SIZE);
if(! jpeg_buffer)
	{
	fprintf(stderr, "Could not allocate space for jpeg_buffer, aborting.\n");

	exit(1);
	}

yuv_buffer = (unsigned char *)malloc( (640 * 480) + (320 * 240) + (320 * 240) );
if(! yuv_buffer)
	{
	fprintf(stderr, "Could not allocate space for yuv_buffer, aborting.\n");

	exit(1);
	} 

yuv_buffer_prev = (unsigned char *)malloc( (640 * 480) + (320 * 240) + (320 * 240) );
if(! yuv_buffer_prev)
	{
	fprintf(stderr, "Could not allocate space for yuv_buffer_prev, aborting.\n");

	exit(1);
	} 

sprintf(temp, "mcamip-%s %s:%d", PVERSION, http_server, http_port);
window_border_info[0] = strsave(temp);
if(! window_border_info)
	{
	fprintf(stderr, "mcamip: could not allocate space for window_border_info, aborting.\n");

	exit(1);
	}

if(debug_flag)
	{	
	fprintf(stderr,\
	"window_border_info=%p window_border_info[0]=%s\n", window_border_info, window_border_info[0]);
	}

sprintf(temp, "%s:%s", username, password);
access_string = base64_encode_line(temp);

if(debug_flag)
	{
	fprintf(stderr, "access_string=%s\n", access_string);
	}

/* try to get the jpeg sequence number from jpeg_sequence_number in frame_dir */
if(jpeg_output)
	{
	sprintf(temp, "%s/jpeg_sequence_number", frame_dir);
	fptr = fopen(temp, "r");
	if(fptr)
		{
		if(debug_flag)
			{
			fprintf(stderr, "found file %s\n", temp);
			}

		fscanf(fptr, "%d", &jpeg_sequence_number);

		fclose(fptr);
		}

	if(debug_flag)
		{
		fprintf(stderr, "open %s failed\n", temp);
		}

	/* if open fails, we are here the first time perhaps */

	} /* end if jpeg_output */

if(debug_flag)
	{
	fprintf(stderr, "jpeg_sequence_number=%d\n", jpeg_sequence_number);
	}

reconnects = 0;
frames = 0;
/*
This value should be smaller then a normally received jpeg.... but we do not know the minimum really.
Theoretically if the jpgs are all much smaller then this, buffer problems in read could result.
*/
content_length = 8000;
while(1)/* main communications loop */
	{
	/* mark the time the current frame was displayed */
	gettimeofday(start_timeval, NULL);

	restart_flag = 0;
	while(1)
		{
		if(connect_to_server(http_server, http_port, &socketfd) ) break;
		fprintf(stderr, "mcamip: could not connect to http server, retry.\n");
		}

	if(! quiet_flag)
		{
		ptr = get_time_as_string(0, 0);
		fprintf(stderr,\
		"mcamip: connected to camera at %s  requesting %d fps  reconnects=%d\n",\
		ptr, (int)dframes_per_second, reconnects);

		free(ptr);
		}
		
	reconnects++;
	
	if(debug_flag)
		{
		fprintf(stderr, "socketfd=%d\n", socketfd);
		}

	sprintf(temp,\
"GET /video.cgi HTTP/1.1\n\
User-Agent: mcamip (rv:%s; X11; Linux)\n\
Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,text/cs s,*/*;q=0.1\n\
Accept-Language: en-us, en;q=0.50\n\
Accept-Encoding: gzip, deflate,compress;q=0.9\n\
Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66\n\
Keep-Alive: 300\n\
Connection: Keep-Alive\n\
Authorization: Basic %s\n\
Referer: http://%s:%d/Jview.htm\n\n",\
	PVERSION, access_string, server_ip_address, http_port);

	if(! send_to_server(socketfd, temp) )
		{
		fprintf(stderr, "mcamip: could not send command to server, aborting.\n");

		exit(1);
		}
		
	http_bptr = http_buffer;
	http_bytes = 0;

	/* read from socket */
	while(1)
		{
		if(debug_flag)
			{
			fprintf(stderr, "before read(): socketfd=%d http_buffer=%p http_bptr=%p\n",\
			socketfd, http_buffer, http_bptr);
			}

		/* start the read timer */
		read_timer = time(0);
		while(1)
			{

			if( (http_bptr < http_buffer) || (http_bytes < 0) || \
			(http_bptr + 500 >= (http_buffer + HTTP_BUFFER_SIZE) ) )
				{
				fprintf(stderr, "http_buffer=%p http_bptr=%p http_bytes=%d HTTP_BUFFER_SIZE=%d\n",\
				http_buffer, http_bptr, http_bytes, HTTP_BUFFER_SIZE);

				fprintf(stderr, "mcamip: ALERT READ BUFFER SPACE VIOLATION, aborting.\n");

				exit(1);
				}

			a = read(socketfd, http_bptr, content_length / 2); //HTTP_BUFFER_SIZE - http_bytes);
			if(debug_flag)
				{
				fprintf(stderr, "read() returned a=%d\n", a);
				}

			if(a > 0) break;
			else if(a == 0) /* EOF server closed connection? */
				{
				fprintf(stderr, "mcamip: read(): returned EOF (power failure, network, interference?)\n");

				report_read_timeout_flag = 1;
				restart_flag = 1;
				break;
				}

			if(debug_flag)
				{
				fprintf(stderr, "mcamip: read() returned error because");
				perror("");
				}
					
			if(errno == EAGAIN)
				{
				/* test for connect time out */
				if( (time(0) - read_timer) > READ_TIMEOUT)
					{
					fprintf(stderr, "mcamip: timeout in read(): (power failure, network, interference?)\n");

					report_read_timeout_flag = 1;
					restart_flag = 1;
					break;
					}

				usleep(1000);
				continue;
				}
			else
				{
				fprintf(stderr, "mcamip: fatal error in read() because\n");
				perror("");

				report_read_timeout_flag = 1;
				restart_flag = 1;
				break;
				}
			} /* end while read timer */
		if(restart_flag) break;

		/* if -z stream output requested, write it */
		if(stream_output)
			{
			fwrite(http_bptr, sizeof(char), a, stdout);
			}

		http_bptr += a;
		if( (http_bptr - http_buffer) >= HTTP_BUFFER_SIZE)
			{
			fprintf(stderr, "mcamip: http_buffer overflow, aborting.\n");

			exit(1);
			}
		http_bytes += a;

		/*
		HTTP/1.0 200 OK
		Server: Camera Web Server/1.0
		Auther: Steven Wu
		MIME-version: 1.0
		Cache-Control: no-cache
		Content-Type:multipart/x-mixed-replace;boundary=--video boundary--
					
		--video boundary--
		Content-length: 8222
		Content-type: image/jpeg
										
		xxxxxxxxactual jpeg picture data
					
		--video boundary--
		Content-length: 8210
		Content-type: image/jpeg
					
		xxxxxxxxactual jpeg picture data
		*/

		/* look for empty line */
		/* look for --video boundary-- */
		/* look for Content-length: dec */
		/* look for Content-type: image/jpeg */
		/* look for empty line */
		/* copy to buffer */

		if(debug_flag)
			{
			fprintf(stderr,\
			"main(): jpeg a=%d jpeg_buffer=%p\n", a, jpeg_buffer);
			}

		/*
		the DCS-900 sends this spelling error in my firmware if it does not understand a packet.
		*/
		if( strstr(http_buffer, "unknwon") )
			{
			fprintf(stderr, "DCS-900 DETECTED UNKNOWN DATA, NETWORK PROBLEM?\n");
			}	

		ptr = strstr(http_buffer, "image/jpeg");
		if(! ptr)
			{
			if(debug_flag)
				{
				fprintf(stderr, "could not find image/jpeg, looping for more\n");
				}

			continue;
			}
	
		if(debug_flag)
			{
			fprintf(stderr, "found Content-type: image/jpeg\n\n");
			}

		start_ptr = ptr + 14;

		ptr = strstr(http_buffer, "Content-length: ");
		if(! ptr) continue;
		if(debug_flag)
			{	
			fprintf(stderr, "found Content-length: \n");
			}

		/* do we have enough bytes ? */
		sscanf(ptr + 16, "%d", &content_length);
		if(debug_flag)
			{
			fprintf(stderr, "content_length=%d\n", content_length);

			fprintf(stderr,\
			"http_bytes=%d start_ptr-http_buffer=%ld\n", http_bytes, start_ptr - http_buffer);
			}

		if(http_bytes < (start_ptr - http_buffer + content_length) )
			{
			if(debug_flag)
				{
				fprintf(stderr, "GETTING MORE\n");
				}

			continue;	
			}

		memcpy(jpeg_buffer, start_ptr, content_length);					

		/* wait until time to display picture */
		while(1)
			{
			/* get elapsed time */
			gettimeofday(current_timeval, NULL);

			/* calculate time since previous picture */
			lcurrent_usecs =\
			current_timeval -> tv_usec + (1000000 * current_timeval -> tv_sec);

			lstart_usecs =\
			start_timeval -> tv_usec + (1000000 * start_timeval -> tv_sec);

			ldiff_usecs = lcurrent_usecs - lstart_usecs;

			/* test if one frame time elapsed */
			if(ldiff_usecs >= (dmilli_seconds_per_frame * 1000.0) ) break;

			usleep(1000);
			} /* end while wait for frame */

		/* mark the time the current frame was displayed */
		gettimeofday(start_timeval, NULL);
		
		/* calculate real frames per second */
		dreal_frames_per_second = 1000000.0 / (double)ldiff_usecs;

		/* warn if system too slow */
		if(! quiet_flag)
			{
			if(dreal_frames_per_second < (dframes_per_second * .75) )
				{
				ptr = get_time_as_string(0, 0);

				fprintf(stderr,\
				"mcamip: WARN %s current frames per second=%.2f this is < 75%% of the requested %.2f,\n\
try a lower value with -f flag.\n",\
				ptr, dreal_frames_per_second, dframes_per_second);
				

				free(ptr);
				}
			}
			
		/* process the jpeg buffer */
		if(! process_jpeg_buffer(1, window_border_info, threshold) )
			{
			fprintf(stderr, "mcamip: process_jpeg_buffer() failed, aborting.\n");
						
			exit(1);
			}

		frame_number++;
		
		if(debug_flag)
			{
			fprintf(stderr,\
			"COPY DOWN frame_number=%llu start_ptr=%p content_length=%d http_bptr=%p len=%ld\n",\
			frame_number, start_ptr, content_length, http_bptr, http_bptr - (start_ptr + content_length) );
			}

		/* copy down http_buffer */

		memmove(http_buffer,\
		start_ptr + content_length,\
		http_bptr - (start_ptr + content_length) );
					
		/* reset http_bptr */

		http_bytes = http_bptr - (start_ptr + content_length);
		http_bptr = http_buffer + http_bytes;

		if(restart_flag) break;

		} /* end while read from socket */

	close(socketfd);

	if(debug_flag)
		{
		fprintf(stderr, "loop\n");
		}
	} /* end main communications loop */

if(debug_flag)
	{
	fprintf(stderr, "mcamip: fell of main loop\n");
	}

exit(0);
} /* end function main */



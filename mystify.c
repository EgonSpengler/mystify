/* mystify, Copyright (c) 2015 Tyler Adam <adam.tyler.j@gmail.com>
 *
 * Clone of the Win95 screensaver "Mystify Your Mind"
 *
 */


/* Options explanation:
	<polygons>
		How many shapes should be drawn. Positive integer (default 2).
	<speed>
		Step size in pixels for each vertex.
		Positive integer. Sane values are 6-16 (default 10).
	<vertices>
		Vertices per shape. Minimim 3, Sane values are 3-6 (default 4).
	<layers>
		How many layers each polygon has. Positive integer.
		Sane values are 1-25 (default 12).
	<fade>
		Should trails fade to black? Possible values "true", "false", "1", "0" (default false)
*/

#include "screenhack.h"
#include <string.h>

#include <getopt.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "vroot.h"

struct {
	int fade;
	int layers;
	int num_vertices;
	int speed;
	int polygons;
} opts;

// Each shape level is a polygon
typedef struct {
	int* vertices; // x,y pairs for each vertex
	int* velocities; // x,y pairs for each vertex
	                 // needed only in first polygon
} polygon;

// A shape is a collection of polygons
typedef struct {
	polygon* polygons;
} shape;


// Returns a random point on the screen
int randomCoord(int max){
	return (int) random() % (max-20) + 10;
}

// Returns an integer value between 2 and speed.
int randomVelocity(){
	return (int) random() % (opts.speed-2) + 2;
}

void drawPolygon(Display *dpy, Drawable d, GC g, polygon p){
	for(int x = 0; x < opts.num_vertices - 1; x++){
		XDrawLine(dpy, d, g, p.vertices[2*x],
		                     p.vertices[2*x+1],
		                     p.vertices[2*(x+1)],
		                     p.vertices[2*(x+1)+1]);
	}
	// Connect first and last point
	XDrawLine(dpy, d, g, p.vertices[2*(opts.num_vertices-1)],
	                     p.vertices[2*(opts.num_vertices-1)+1],
	                     p.vertices[0],
	                     p.vertices[1]);
}

void drawShape(Display *dpy, Drawable d, GC g, shape s, int window_width, int window_height){

	for(int x = opts.layers-1; x >= 0; x--){
		if(s.polygons[x].vertices != NULL)
			drawPolygon(dpy, d, g, s.polygons[x]);
		// after drawing, copy vertices of higher level, or update if highest level
		if(x > 0){
			if(x == opts.layers-1) free(s.polygons[x].vertices);
			s.polygons[x].vertices = s.polygons[x-1].vertices;
		}else{
			// allocate new vertex and velocity arrays
			int* vert_new = (int*) malloc(sizeof(int) * opts.num_vertices * 2);

			for(int y = 0; y < opts.num_vertices; y++){
				// Update X velocity and location
				int new_x = s.polygons[0].vertices[2*y] + s.polygons[0].velocities[2*y];
				if(new_x < 0 || new_x > window_width) // we hit L/R edge, get new random velocity in reverse direction
					s.polygons[0].velocities[2*y] = (s.polygons[0].velocities[2*y] > 0)? randomVelocity()*-1 : randomVelocity();
				vert_new[2*y] = s.polygons[0].vertices[2*y] + s.polygons[0].velocities[2*y];

				// Update Y velocity and location
				int new_y = s.polygons[0].vertices[2*y+1] + s.polygons[0].velocities[2*y+1];
				if(new_y < 0 || new_y > window_height) // we hit T/B edge, get new random velocity in reverse direction
					s.polygons[0].velocities[2*y+1] = (s.polygons[0].velocities[2*y+1] > 0)? randomVelocity()*-1 : randomVelocity();
				vert_new[2*y+1] = s.polygons[0].vertices[2*y+1] + s.polygons[0].velocities[2*y+1];
			}
			s.polygons[0].vertices = vert_new;
		}
	}

}

int main(int argc, char* argv[]) {

	// Set default options
	int defaults[] = {0, 12, 4, 10, 2};
	opts.fade = 0;
	opts.layers = 12;
	opts.num_vertices = 4;
	opts.speed = 10;
	opts.polygons = 2;


    int c;
    int digit_optind = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"fade",     required_argument, 0, 'f'},
			{"layers",   required_argument, 0, 'l'},
			{"vertices", required_argument, 0, 'v'},
			{"speed",    required_argument, 0, 's'},
			{"polygons", required_argument, 0, 'p'},
			{"root",     no_argument,       0,  0 }
		};

		c = getopt_long(argc, argv, "flvsp", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			if(option_index < 5)
				defaults[option_index] = atoi(optarg);
			//else
				//set root option
			break;
		case 'f':
			opts.fade = (strncmp("true", optarg, 4) == 0) ? 1 : 0;
			break;
		case 'l':
			opts.layers = atoi(optarg);
			break;
		case 'v':
			opts.num_vertices = atoi(optarg);
			break;
		case 's':
			opts.speed = atoi(optarg);
			break;
		case 'p':
			opts.polygons = atoi(optarg);
			break;
		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
    }

	Display *dpy;
	Window root;
	XWindowAttributes wa;
	GC g;

	XColor redx, reds;
	XColor greenx, greens;

	Pixmap double_buffer;

	/* open the display (connect to the X server) */
	dpy = XOpenDisplay (getenv ("DISPLAY"));

	/* get the root window */
	root = DefaultRootWindow (dpy);

	/* get attributes of the root window */
	XGetWindowAttributes(dpy, root, &wa);

	/* create a GC for drawing in the window */
	g = XCreateGC (dpy, root, 0, NULL);

	/* create the double buffer */
	double_buffer = XCreatePixmap(dpy, root, wa.width, wa.height, wa.depth);
	XSetForeground(dpy, g, BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)));
	XFillRectangle(dpy, double_buffer, g, 0, 0, wa.width, wa.height);

	/* allocate the red color */
	XAllocNamedColor(dpy,
					 DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy)),
					 "green",
					 &greens, &greenx);
	XAllocNamedColor(dpy,
					 DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy)),
					 "red",
					 &reds, &redx);


	// Initialize shapes and the first level of each
	shape s1, s2;

	s1.polygons = (polygon*) malloc(sizeof(polygon) * opts.layers);
	s1.polygons[0].vertices = (int*) malloc(sizeof(int) * opts.num_vertices * 2);
	s1.polygons[0].velocities = (int*) malloc(sizeof(int) * opts.num_vertices * 2);

	s2.polygons = (polygon*) malloc(sizeof(polygon) * opts.layers);
	s2.polygons[0].vertices = (int*) malloc(sizeof(int) * opts.num_vertices * 2);
	s2.polygons[0].velocities = (int*) malloc(sizeof(int) * opts.num_vertices * 2);

	// Zero polygon structs beyond the first level
	for(int x = 1; x < opts.layers; x++){
		s1.polygons[x].vertices = NULL;
		s1.polygons[x].velocities = NULL;
		s2.polygons[x].vertices = NULL;
		s2.polygons[x].velocities = NULL;
	}

	// Initialize random velocities and coordinates for first-level layers
	for(int x = 0; x < opts.num_vertices; x++){
		s1.polygons[0].vertices[2*x] = randomCoord(wa.width); // random X
		s1.polygons[0].vertices[2*x+1] = randomCoord(wa.height); // random Y
		s1.polygons[0].velocities[2*x] = randomVelocity(); // random vX
		s1.polygons[0].velocities[2*x+1] = randomVelocity(); // random vY

		s2.polygons[0].vertices[2*x] = randomCoord(wa.width); // random X
		s2.polygons[0].vertices[2*x+1] = randomCoord(wa.height); // random Y
		s2.polygons[0].velocities[2*x] = randomVelocity(); // random vX
		s2.polygons[0].velocities[2*x+1] = randomVelocity(); // random vY
	}

	// Drawing //
	while(1) {
		/* Clear the double buffer */
		XSetForeground(dpy, g, BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)));
		XFillRectangle(dpy, double_buffer, g, 0, 0, wa.width, wa.height);

		/* Everything is red for now */
		XSetForeground(dpy, g, reds.pixel);

		// Draw s1
		drawShape(dpy, double_buffer, g, s1, wa.width, wa.height);
		XSetForeground(dpy, g, greens.pixel);
		drawShape(dpy, double_buffer, g, s2, wa.width, wa.height);

		/* copy the buffer to the window */
		XCopyArea(dpy, double_buffer, root, g, 0, 0, wa.width, wa.height, 0, 0);

		/* flush changes and sleep */
		XSync(dpy, 1);
		usleep(20000);
	}

	XCloseDisplay (dpy);
}

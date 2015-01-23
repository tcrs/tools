#if 0
# Copyright (c) 2014, Thomas Spurden <thomas@spurden.name>
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# Print a line to stdout whenever a XRandR diplay is plugged or unplugged
# The line will be of the form:
# + OUTPUTNAME
# for plug, and
# - OUTPUTNAME
# for unplug

# This file can be read by either GNU Make or a C99 compiler.
# Easy way to compile is: make -f thisfile output
# Which will produce a binary called output.

# Dependencies: X11 with XRandR

! := $(lastword $(MAKEFILE_LIST))
%: $!
	$(CC) -pedantic -Wall -O2 -std=c99 -lX11 -lXrandr -o $@ -xc $!

define program
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

static int error_handler(void) 
{
	exit(EXIT_FAILURE);
}

char* strdup(char const* in)
{
	size_t len = strlen(in);
	if(len > 256) {
		return NULL;
	}
	char* to = malloc(len + 1);
	strncpy(to, in, len);
	to[len] = '\0';
	return to;
}

char* connected_outputs[16] = {0};

int set_connected(char const* name, int con)
{
	char** inspos = NULL;
	for(unsigned int i = 0; i < sizeof(connected_outputs) / sizeof(connected_outputs[0]); i += 1) {
		if(connected_outputs[i]) {
			if(strcmp(connected_outputs[i], name) == 0) {
				if(con) {
					return 1;
				} else {
					free(connected_outputs[i]);
					connected_outputs[i] = 0;
					return 1;
				}
			}
		} else {
			inspos = &connected_outputs[i];
		}
	}
	if(con && inspos) {
		*inspos = strdup(name);
		return (*inspos)? 0 : -1;
	}

	return -1;
}

int main(int argc, char **argv) 
{
	XEvent ev;
	Display *dpy;
	int eventBase, errorBase;

	dpy = XOpenDisplay(NULL);

	if(dpy == NULL) {
		fprintf(stderr, "Could not open X11 display\n");
		exit(EXIT_FAILURE);
	}

	if(!XRRQueryExtension(dpy, &eventBase, &errorBase)) {
		fprintf(stderr, "Could not query the XRandR extension\n");
		exit(EXIT_FAILURE);
	}

	{
		XRRScreenResources* res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));
		for(int i = 0; i < res->noutput; i += 1) {
			XRROutputInfo* info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
			set_connected(info->name, info->connection == 0);
			XRRFreeOutputInfo(info);

		}
		XRRFreeScreenResources(res);
	}

	XRRSelectInput(dpy, DefaultRootWindow(dpy), RROutputChangeNotifyMask);
	XSync(dpy, False);
	XSetIOErrorHandler((XIOErrorHandler) error_handler);
	while(1) {
		if(!XNextEvent(dpy, &ev)) {
			if(ev.type != eventBase + RRNotify_OutputChange) {
				continue;
			}

			XRROutputChangeNotifyEvent* change = (XRROutputChangeNotifyEvent*)&ev;
			XRRScreenResources* resources = XRRGetScreenResources(change->display, change->window);
			if(resources == NULL) {
				fprintf(stderr, "Could not get screen resources\n");
				continue;
			}

			XRROutputInfo* info = XRRGetOutputInfo(change->display, resources, change->output);
			if(info == NULL) {
				XRRFreeScreenResources(resources);
				fprintf(stderr, "Could not get output info\n");
				continue;
			}

			if(info->connection == 0) {
				if(set_connected(info->name, 1) == 0) {
					printf("+ %s\n", info->name);
					fflush(stdout);
				}
			}
			else {
				if(set_connected(info->name, 0) == 1) {
					printf("- %s\n", info->name);
					fflush(stdout);
				}
			}

			XRRFreeScreenResources(resources);
			XRRFreeOutputInfo(info);
		}
	}
	return EXIT_SUCCESS;
}

#if 0
endef
#endif

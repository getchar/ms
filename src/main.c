//=============================================================================
// ms
// by Charlie Pashayan
// 2012
// main.c: Main loop of the program.
//
// Copyright (c) 2012 Charlie Pashayan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include "parsec.h"
#include "msio.h"
#include "msdefs.h"
#include "msflags.h"

// draw_banner() draws these
#define BANNER_ELEMENT '*'

void draw_banner();
void show_version(char *progname);
void show_help(char *progname);

int main(int argc, char **argv)
{
  time_t 
    cur_time, // for determining staleness of messages
    future; // expiration date of new message
  flags_type *flags; // holds info gleaned from options
  int 
    i, // loop walker
    printed;
  cur_time = time(NULL);
  flags = eat_args(argc, argv);
  if (DEBUG) {
    dump_flags(flags);
  }
  // first see if there's anything wrong with the input
  if (flags->smth_wrong) {
    if (flags->loquacious) {
      if (flags->is_bad_opt) {
	fprintf(stderr, "Bad option encountered: '%s'\n", flags->bad_opt);
      }
      if (flags->extra_args) {
	fprintf(stderr, "Too many arguments, or arguments out of order..\n");
      }
    }
    fprintf(stderr, "Usage: %s [-hvlsdwunpk#] [<message> [SMHdwmy]]\n", argv[0]);
    if (!flags->forgive) {
      exit(1);
    }
  }
  // draw banner before an
  printed = 0;
  if (flags->show_banner) {
    draw_banner();
  }
  // now do whatever the flags say to do
  if (flags->show_version) {
    ++printed;
    show_version(argv[0]);
  }
  if (flags->show_help) {
    ++printed;
    show_help(argv[0]);
  }
  if (flags->need_message) {
    if (parsec(&future, flags->expiration, cur_time)) {
      create_message(argv[flags->message_index], future);
      if (flags->loquacious) {
	++printed;
	printf("Stored: '%s'\n", argv[flags->message_index]);
	printf("Until: %s", asctime(localtime(&future)));
      }
    } else {
      ++printed;
      printf("No message stored.\n");
    }
  }
  if (flags->do_rundown) {
    printed += rundown(cur_time, flags->show_numbers, flags->display_ms, 
		       flags->retain, flags->kill_list, flags->kill_no);
  }
  if (flags->show_banner && printed) {
    draw_banner();
  }
  exit(0);
  return 0;
}

//=============================================================================
// side effects: Fills the width of the screen with whatever char is designated
//   as BANNER_ELEMENT.
//=============================================================================
void draw_banner()
{
  struct winsize ws;
  int 
    i,
    returned;
  static int width = 0;
  char *banner = NULL;
  if (width == 0) {
    returned = ioctl(0, TIOCGWINSZ, &ws);
    if (returned != 0) {
      printf("Window sizing operation failed.  Exiting.\n");
      exit(1);
    }
    width = ws.ws_col;
  }
  for (i = 0; i < width; ++i) {
    putchar(BANNER_ELEMENT);
  }
  putchar('\n');
}

//=============================================================================
// side effects: Prints program name, version number and author (that's me!).
//=============================================================================
void show_version(char *progname)
{
  printf("%s version %s\n", progname, VERSION);
  printf("by Charlie Pashayan\n");
}

//=============================================================================
// side effects: Dumps a simple help file to stdout.
//=============================================================================
void show_help(char *progname)
{
  printf("Usage: %s <flags> <<message> <duration>>\n", progname);
  printf("Flags:\n");
  printf("\t-h: show this message\n");
  printf("\t-v: show version number\n");
  printf("\t-l: loquacious\n");
  printf("\t-s: silent\n");
  printf("\t-d: display messages\n");
  printf("\t-w: withhold messages\n");
  printf("\t-b: draw border around output\n");
  printf("\t-r: retain expired messages\n");
  printf("\t-e: discard expired messages\n");
  printf("\t-f: forgive errors, carry on\n");
  printf("\t-t: tough on errors, just end\n");
  printf("\t-n: number messages\n");
  printf("\t-p: don't number messages\n");
  printf("\t-kN: kill the Nth message\n");
  printf("\t-u: the next argument is the message\n");
  printf("Duration is a string of units preceded by optional multipliers\n");
  printf("Units:\n");
  printf("\tS: second\n");
  printf("\tM: minute\n");
  printf("\tH: hour\n");
  printf("\td: day\n");
  printf("\tw: week\n");
  printf("\tm: month\n");
  printf("\ty: year\n");
}

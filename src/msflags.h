//=============================================================================
// ms
// by Charlie Pashayan
// 2012
// msflags.h: For interpreting and recording the user's wishes.
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

#ifndef MS_FLAGS_H
#define MS_FLAGS_H

// empty expiration field is replaced with this
#define DEFAULT_TIME ((char*) "d")

// for storing the user's innermost desires
struct _flags_type {
  bool show_version; // output version of program
  bool show_help; // show help screen
  bool show_numbers; // number messages ("del" for deleted)
  bool loquacious; // confirm receipt of new message
  bool show_banner; // draw line of stars above and below normal output
  bool display_ms; // dump ms file
  bool disp_expl; // this is a weird one; display_ms can be set or cleared
  // according to default assumptions UNLESS disp_expl is set.
  // basically this variable means that the user has explicitly said whether or
  // not to display the messages, so only change that behavior if told
  // explicitly by another option later on.
  bool ret_expl; // similar to above, but for retain
  bool need_message; // user has entered a message; record it
  bool extra_args; // user has entered args after the time string
  bool retain; // if yes, don't remove expired messages
  bool is_bad_opt; // user has entered an option that doesn't make sense
  bool smth_wrong; // either bad opts or too many arguments
  bool forgive; // ignore bad input, do what you can
  char *bad_opt; // pointer to the bad option (could be a string of options)
  int message_index; // index of arg determined to be the message
  bool do_rundown; // either show messages or kill them
  char *expiration; // pointer to string containing expiration info
  int kill_no; // number of messages to kill
  int *kill_list; // list of indices of messages to kill
  int kill_list_size; // max size of kill list; for resizing purposes
};

typedef struct _flags_type flags_type;

void dump_flags(flags_type *flags);
void initialize_flags(flags_type **flags, int argc, char **argv);
flags_type *eat_args(int argc, char **argv);
bool make_kill(flags_type *flags, char **argv, int *i, int *j);

//=============================================================================
// input: An int and a double pointer to char, the arguments from the command
//   line.
// output: A pointer to flags_type that contains all the settings required by
//   the args.
// side effects: Allocates memory for the flags_type.
//=============================================================================
flags_type *eat_args(int argc, char **argv)
{
  int 
    i, j, // array walkers
    arglen; // for driving inner loop
  flags_type *flags; // return value
  bool is_message; // basically to escape '-' at the beginning of messages
  initialize_flags(&flags, argc, argv);
  for (i = 1, is_message = false; i < argc; ++i) {
    if (argv[i][0] == '-' && !is_message) {
      // looks like an option; let's process it
      if (argv[i][1] == '\0') {
	if (!flags->is_bad_opt) {
	  // only report first bad opt; what do you want from me?!
	  flags->is_bad_opt = true;
	  flags->bad_opt = argv[i];
	}
      }
      for (j = 1; argv[i][j]; ++j) {
	switch (argv[i][j]) {
	case 'h': 
	  flags->show_help = true;
	  if (!flags->disp_expl) {
	    flags->display_ms = false;
	  }
	  flags->disp_expl = true; 
	  break;
	case 'v': 
	  flags->show_version = true;
	  if (!flags->disp_expl) {
	    flags->display_ms = false;
	  }
	  flags->disp_expl = true; 
	  break;
	case 'b':
	  flags->show_banner = true;
	  break;
	case 'l': 
	  flags->loquacious = true;
	  break;
	case 's': 
	  flags->loquacious = false;
	  break;
	case 'd': 
	  flags->display_ms = true; 
	  flags->disp_expl = true; 
	  break;
	case 'w': 
	  flags->display_ms = false; 
	  flags->disp_expl = true; 
	  break;
	case 'u': 
	  is_message = true; 
	  break;
	case 'n':
	  flags->show_numbers = true;
	  flags->display_ms = true;
	  flags->disp_expl = true;;
	  break;
	case 'p':
	  flags->show_numbers = false;
	  flags->display_ms = true;
	  flags->disp_expl = true;;
	  break;
	case 'r':
	  flags->retain = true;
	  flags->ret_expl = true;
	  break;
	case 'e':
	  flags->retain = false;
	  flags->ret_expl = true;
	  flags->do_rundown = true;
	  break;
	case 'k':
	  make_kill(flags, argv, &i, &j);
	  break;
	case 'f':
	  flags->forgive = true;
	  break;
	case 't':
	  flags->forgive = false;
	  break;
	default: 
	  if (!flags->is_bad_opt) {
	    // only report first bad opt; what do you want from me?!
	    flags->is_bad_opt = true;
	    flags->bad_opt = argv[i];
	  }
	}
      }
    } else {
      // there's at least one arg that's not an option
      // it must be the message
      if (!flags->disp_expl) {
	flags->display_ms = false;
      }
      flags->need_message = true;
      flags->message_index = i;
      // if there's another arg, it's the time code
      if (i < argc-1) {
	++i;
	flags->expiration = argv[i];
	++i;
	// take note of extraneous arguments
	if (i < argc) {
	  flags->extra_args = true;
	}
      } else {
	flags->expiration = DEFAULT_TIME;
      }
    }
  }
  // interperet and set a few more flags
  if (flags->loquacious && flags->kill_no > 0) {
      flags->display_ms = true;
      flags->show_numbers = true;
      if (!flags->ret_expl) {
	flags->retain = true;
      }
  }
  flags->do_rundown |= flags->display_ms || flags->kill_no;
  flags->smth_wrong = flags->extra_args || flags->is_bad_opt;
  return flags;
}

//=============================================================================
// input: A pointer to flags_type, the arg list and pointers to two ints
//   which are used as indices to the arg list.  They should be cued up to the
//   letter 'k'.
// output: A bool that says whether or not the function succeeded in generating
//   a kill instruction.
// side effects: Stores the kill instruction (if one was generated) in the 
//   appropriate field of the flags_type.  Advances the second index pointer
//   to the last digit if any digits were found.
// etiology: Called from eat_arts.  Is essentially part of eat_arts, but put
//   here for cleanliness.
//=============================================================================
bool make_kill(flags_type *flags, char **argv, int *i, int *j)
{
  int
    index, // index of victim
    cur, // index of kill order
    max; // used for resizing kill_list
  ++*j;
  if (argv[*i][*j] == '\0') {
    if (!flags->is_bad_opt) {
      flags->is_bad_opt = true;
      flags->bad_opt = argv[*i-1];
    }
    return false;
  }
  if (!isdigit(argv[*i][*j])) {
    if (!flags->is_bad_opt) {
      flags->is_bad_opt = true;
      flags->bad_opt = argv[*i-1];
    }
    return false;
  }
  index = atoi(argv[*i] + *j);
  while (argv[*i][*j+1] && isdigit(argv[*i][*j+1])) { ++*j; }
  max = flags->kill_list_size;
  cur = flags->kill_no;
  ++flags->kill_no;
  if (cur >= max-1) {
    max *= 2;
    flags->kill_list_size = max;
    flags->kill_list = (int*) realloc(flags->kill_list, 
				      flags->kill_list_size * sizeof(int));
    if (flags->kill_list == NULL) {
      fprintf(stderr, "Could not process kill request.\n");
      return false;
    }
  }
  flags->kill_list[cur] = index;
  if (!flags->disp_expl) {
    flags->display_ms = false;
  }
  flags->disp_expl = true;
  return true;
}


//=============================================================================
// input: A pointer to flags_type.
// side effects: Runs through the flags_type dumping all of its contents to 
//   stdout.  Obviously not much use unless you're me.
//=============================================================================
void dump_flags(flags_type *flags)
{
  int i; // loop var
  printf("show_version = %s\n", flags->show_version? "true": "false");
  printf("show_help = %s\n", flags->show_help? "true": "false");
  printf("show_numbers = %s\n", flags->show_numbers? "true": "false");
  printf("loquacious = %s\n", flags->loquacious? "true": "false");
  printf("show_banner = %s\n", flags->show_banner? "true": "false");
  printf("display_ms = %s\n", flags->display_ms? "true": "false");
  printf("disp_expl = %s\n", flags->disp_expl? "true": "false");
  printf("ret_expl = %s\n", flags->ret_expl? "true": "false");
  printf("need_message = %s\n", flags->need_message? "true": "false");
  printf("extra_args = %s\n", flags->extra_args? "true": "false");
  printf("retain = %s\n", flags->retain? "true": "false");
  printf("is_bad_opt = %s\n", flags->is_bad_opt? "true": "false");
  printf("extra_args = %s\n", flags->extra_args? "true": "false");
  printf("smth_wrong = %s\n", flags->smth_wrong? "true": "false");
  printf("forgive = %s\n", flags->forgive? "true": "false");
  printf("bad_opt = %s\n", flags->bad_opt);
  printf("message_index = %d\n", flags->message_index);
  printf("do_rundown = %s\n", flags->do_rundown? "true": "false");
  printf("expiration = '%s'\n", flags->expiration);
  printf("kill_no = %d\n", flags->kill_no);
  printf("kill_list_size = %d\n", flags->kill_list_size);
  for (i = 0; i < flags->kill_no; ++i) {
    printf("kill: %d\n", flags->kill_list[i]);
  }
}

//=============================================================================
// input: A double pointer to flags_type, as well as the int a double pointer
//   to char that the program itself received from the command line.
// side effects: Allocates a flag_type and points the pointer at its location.
//   Initializes the flags_type with some sensible default values.
// etiology: Called once from main loop of program.
//=============================================================================
void initialize_flags(flags_type **flags, int argc, char **argv)
{
  *flags = (flags_type *) malloc(sizeof(flags_type));
  (*flags)->need_message = false;
  (*flags)->show_version = false;
  (*flags)->show_numbers = false;
  (*flags)->display_ms = true;
  (*flags)->disp_expl = false; 
  (*flags)->show_help = false;
  (*flags)->loquacious = false;
  (*flags)->show_banner = false;
  (*flags)->extra_args = false;
  (*flags)->is_bad_opt = false;
  (*flags)->bad_opt = NULL;
  (*flags)->message_index = -1;
  (*flags)->expiration = NULL;
  (*flags)->kill_no = 0;
  (*flags)->kill_list_size = argc;;
  (*flags)->kill_list = (int *) malloc((*flags)->kill_list_size * 
				       sizeof(int));
  (*flags)->do_rundown = false;
  (*flags)->retain = false;
  (*flags)->forgive = false;
  (*flags)->smth_wrong = false;
  (*flags)->ret_expl = false;
}

#endif


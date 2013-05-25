//=============================================================================
// ms
// by Charlie Pashayan
// 2012
// msio.h: Contains funcionality having to do with reading from and writing to
// the file containing the messages.
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
#include "msdefs.h"
#include <math.h>

// name of file containing messages
#define MS_FILE_NAME ((char*) ".ms_file.txt")
#define DEBUG_MS_FILE_NAME ((char*) "debug_ms.txt")
// when numbering messages, in place of number for deleted messages put:
#define DELETION ((char*) "del")

// data type for storing and manipulating messages when ingesting file
struct _message_type {
  time_t expires; // expiration date of message
  char *ms_text; // text of message (capped with NUL)
  bool keep; // whether message has expired
};
typedef struct _message_type message_type;

// format of ingested message file
struct _message_array {
  int num; // number of elements in array
  message_type *array; // messages themselves
};
typedef struct _message_array message_array;

char *filter_excl(char *str);
int create_message(char *alert, time_t time);
int rundown(time_t cur_time, bool do_no, bool just_clean, 
	    bool retain, int *kill_list, int kls);
message_array *make_array(char *contents, int flen);
char *readup(char *filename, int *filesize);
bool next_date(char **contents, time_t *date);
char *next_ms(char **contents);
char *filename();
bool store_alerts(char *alerts_file, message_array *messages);

//=============================================================================
// output: A C string containing the name of the file containing the alerts.
// side effects: The C string is allocated from the heap, so you might want
//   to free() it when you're done.
//=============================================================================
char *filename()
{
  char 
    *alerts_file, // file name to return
    *file, // absolute path to directory containing file
    *path; // name of file
  if (DEBUG) {
    path = getenv("PWD");
    file = (char*) DEBUG_MS_FILE_NAME;
  } else {
    path = getenv("HOME");
    file = (char*) MS_FILE_NAME;
  }
  alerts_file = (char*) malloc(45); //2 + strlen(path) + strlen(file));
  sprintf(alerts_file, "%s/%s", path, file);
  return alerts_file;
}

//=============================================================================
// input: A C string, presumed to be the message from the command line.
// side effects: It's kind of hard to get '!'s into an argument.  Even if
//   it's double quoted, bash thinks it's a history command.  There are a
//   couple of ways around this.  First, the user can enclose the string in
//   single quotes, which ignore all meta characters.  Because they even ignore
//   backslashes, though, single-quoted strings can't contain apostrophes,
//   which are indistinguishable from closing single quotes.  Another option is
//   to escape the '!' with a backslash.  This function finds and removes all
//   backslashes immediately preceding '!'s.  This is not a perfect solution.
//   But it should be fine.  The only time a user would be surprised by this
//   is if the message included the substring "\!" and was enclosed in single
//   quotes.
// output: Returns the modified string.
//=============================================================================
char *filter_excl(char *str)
{
  int 
    i, // drives loop
    level; // number of backslashes deleted
  level = 0;
  for (i = 0; i <= strlen(str); ++i) {
    if (str[i] == '!' && i > 0 && str[i - 1] == '\\') {
      ++level;
    }
    if (level > 0) {
      str[i - level] = str[i];
    }
  }
  return str;
}

//=============================================================================
// input: An array of chars representing an alert message and a time_t
//   representing the expiration date of the alert.
// output: returns the number of bytes written on success.
// side effects: Appends a line to the alerts file containing the date in
//   epoch time and the message.  Each entry consists of the epoch time,
//   followed by a single space, followed by the alert, followed by a NUL char.
//=============================================================================
int create_message(char *alert, time_t time)
{
  char *alerts_file; // name of file containing messages
  FILE *fp; // pointer to said file
  int len; // of file
  // first open a pointer to the appropriate file for storing the alerts
  alerts_file = filename();
  fp = fopen(alerts_file, "a");
  filter_excl(alert);
  if (fp == NULL) {
    fprintf(stderr, "Unable to open alerts file '%s'.  Exiting.\n", alerts_file);
    exit(1);
  }
  // deposit the data as well as the trailing nul, then close
  if (DEBUG) {
    printf("Wrote '%d %s'\nto %s\n", (int) time, alert, alerts_file);
  }
  len = fprintf(fp, "%d %s", (int) time, alert);
  fputc('\0', fp);
  free(alerts_file);
  fclose(fp);
  return len;
}

//=============================================================================
// input: A C string of the file name and a pointer to an int.
// output: Returns an array of all the bytes contained by the file named by the
//   C string.
// side effects: Allocates the memory to contain the bytes.  Sets the int
//   pointed to by filesize to equal the number of bytes in the file/array.
//=============================================================================
char *readup(char *alerts_file, int *filesize)
{
  char *contents; // raw bytes of file
  FILE *fp; // pointer to alerts file
  message_array *messages; // structured contents of file
  int  
    i, // for looping
    flen; // length of file
  fp = fopen(alerts_file, "r");
  if (fp == NULL) { 
    return NULL;
  }
  fseek(fp, 0L, SEEK_END);
  *filesize = ftell(fp);
  rewind(fp);
  if (DEBUG) {
    printf("File is %d bytes long\n", *filesize);
  }
  contents = (char *) malloc(*filesize);
  fread(contents, 1, *filesize, fp);
  fclose(fp);
  return contents;
}

//=============================================================================
// input: A pointer to char containing the contents of the alerts file and
//   an int representing the number of bytes in the file.
// output: Creates a message_array struct and returns a pointer to it.  The
//   struct contains an array of message_types as well as the number of
//   elements in it and its maximum possible size.
//=============================================================================
message_array *make_array(char *contents, int flen)
{
  message_array *messages; // will store the whole file
  char 
    *cp, // for walking through contents
    *cur_ms; // text of message being ingested
  time_t date;
  int 
    i, // loop var
    max; // number of NULs in the file, probably also of messages
  messages = (message_array *) malloc(sizeof(message_array));
  messages->num = 0;
  max = 0;
  // first figure out the maximum number of messages the file might contain
  for (i = 0; i < flen; ++i) {
    if (contents[i] == '\0') {
      ++max;
    }
  }
  messages->array = (message_type *) malloc(max * sizeof(message_type));
  for (cp = contents; next_date(&cp, &date); ) {
    cur_ms = next_ms(&cp);
    messages->array[messages->num].expires = date;
    messages->array[messages->num].ms_text = cur_ms;
    messages->array[messages->num].keep = true;
    ++messages->num;
  }
  return messages;
}

//=============================================================================
// input: A time_t of the current time, a bool saying whether or not to number
//   the messages, a bool saying whether or not to show the messages, an array
//   of indices of messages to erase and the size of that array.
// output: The number of messages displayed.
// side effects: Opens up the ms file and slurps up the content.  Converts the
//   text data to message_array format.  If desired, desplays the messages.  If
//   desired, messages are preceded by an index number (starting at 1).  This
//   function also cleans up the ms file by removing anything out of date and
//   also removing anything placed on the kill list.
// confessions: This function really does too much and has too many arguments.
//   But it's more efficient because spreading the same functionality across
//   multiple functions would make the interface too complicated and it would
//   also be inefficient as it would require more loops through the same data.
//=============================================================================
int rundown(time_t cur_time, bool do_no, bool do_show, bool retain,
	    int *kill_list, int kls)
{
  char 
    *alerts_file, // file name
    *contents; // raw data from ms file here
  int 
    to_kill, // to keep from dereferencing the index 9k+ times
    no, // indices of messages
    i, // for loops
    shown, // number of messages displayed
    flen, // length of file
    width; // width or message index
  message_array *messages;
  alerts_file = filename();
  contents = readup(alerts_file, &flen);
  if (contents == NULL) {
    return 0;
  }
  messages = make_array(contents, flen);
  width = 1 + log(messages->num) / log(10);
  if (width < strlen(DELETION)) {
    width = strlen(DELETION);
  }
  // first mark for death
  for (i = 0; i < kls; ++i) {
    to_kill = kill_list[i];
    if (to_kill > 0 && to_kill <= messages->num) {
      --to_kill;
      messages->array[to_kill].keep = false;
    }
  }
  // display the messages, mark expired; note this only happens
  // if user wants messages shown; messages can't expire unseen
  if (do_show || !retain) {
    // only loop through if need to show or delete expired
    for (i = 0, no = 1; i < messages->num; ++i) {
      if (!retain && (cur_time >= messages->array[i].expires)) {
	messages->array[i].keep = false;
      }
      if (do_no) {
	if (messages->array[i].keep) {
	  printf("%*d ", width, no++);
	} else {
	  printf("%*s ", width, DELETION);;
	}
      }
      if (DEBUG) {
	printf("%d ", (int) messages->array[i].expires);
      }
      if (do_show) {
	printf("%s\n", messages->array[i].ms_text);
      }
    }
  }
  shown = i;
  store_alerts(alerts_file, messages);
  free(alerts_file);
  free(contents); 
  free(messages);
  return shown;
}

//=============================================================================
// input: A double pointer to char containing the alerts file and a pointer
//   to time_t.
// output: Whether or not the function succeeded in finding a date.
// side effects: Sets the time_t pointer to the date if one is foune.  Drives
//   the char pointer past any leading non digits, past the date (if found) and
//   past the trailing space after the digit, or to the NUL char at the end
//   of the string if no digit is found.  Not that *contents must be a well
//   formed C string, else this will cause a segfault.
// etiology: Drives the main loop of rundown().  Hence returning a 
//   bool.
//=============================================================================
bool next_date(char **contents, time_t *date)
{
  // Important to skip leading non-digits even though there shouldn't be any.
  // If an entry begins with something other than digits, it and all entries
  // after it will be lost.  Now this program will never allow this to happen,
  // but it's possible somebody could open the file and edit it by hand.
  while (**contents && !isdigit(**contents)) { ++*contents; }
  if (!**contents) {
    return false;
  }
  *date = atoi(*contents);
  while (isdigit(**contents)) { ++*contents; }
  if (isspace(**contents)) { ++*contents; }
  return true;
}

//=============================================================================
// input: A double pointer to char; the contents of the alerts file.
// output: Returns a pointer to the (presumed well-formed) C string starting
//   at the address being pointed to.
// side effects: Advances the pointer being pointed to past the C string and
//   past the terminating NUL.
// etiology: Called from the main loop of rundown().
//=============================================================================
char *next_ms( char **contents)
{
  char 
    *head, // beginning of message
    *tail; // for skipping to the end of the string
  head = *contents;
  tail = strchr(*contents, '\0');
  *contents = tail + 1;
  return head;
}

//=============================================================================
// input: A pointer to char containing the name of the ms file and one to the
//   message_array containing the data to store.
// output: A bool that says whether or not the ms file was modified.
// side effects: Modifies the contents of the ms file.
//=============================================================================
bool store_alerts(char *alerts_file, message_array *messages)
{
  FILE *fp; // pointer to message file
  int i; // loop walker
  fp = fopen(alerts_file, "w");
  if (fp == NULL) {
    fprintf(stderr, "Could not save changes to alerts file.\n");
    return false;
  }
  for (i = 0; i < messages->num; ++i) {
    if (messages->array[i].keep) {
      fprintf(fp, "%d %s", (int) messages->array[i].expires,
	      messages->array[i].ms_text);
      fputc('\0', fp);
    }
  }
  fclose(fp);
  return true;
}

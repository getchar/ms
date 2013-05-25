//=============================================================================
// ms
// by Charlie Pashayan
// 2012
// parsec.h: PARses the number of SEConds.
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
#include <ctype.h>
#include <time.h>
#include "msdefs.h"

// units in correct order, so that the index of a code within this string is
// equal to the associated value in the enum below
#define UNIT_CODES "SMHdwmy"
char *unit_codes = (char*) UNIT_CODES;

// for indexing the multipliers for each unit of time
enum _units { usec, umin, uhour, uday, uweek, umon, uyear, n_units };
typedef enum _units units;

// number of seconds in units of time of invariant length
enum _secs_machine { ssec = 1, smin = 60, shour = 60 * smin, sday = 24 * shour,
		     sweek = 7 * sday };
typedef enum _secs_machine secs_machine;

// for tracking errors in parsing time string
enum _parsec_err { CHAR_UNKNOWN = -1, STR_NULL = -2, PTR_NULL = -3 };
typedef enum _parsec_err parsec_err;

// bare numbers in time string are taken as a number of days
units default_unit = uday;

bool parsec(time_t *timeout, char *input, time_t cur_time);
int next_unit(char **input);
int next_mult(char **input);
time_t translate_time(int *mult_table, time_t cur_time);
int process_parsec_error(parsec_err which);

char lastchar; // last char of input read, saved in case of error.

//=============================================================================
// input: A pointer to a time_t and a C string.
// output: Computes based on the input string how long the message should be
//   eligible for display, adds this to the current time and figures returns
//   the resulting date (in seconds since the epoch).
// side effects: Sets lastchar as nul initially.  This will later be used 
//=============================================================================
bool parsec(time_t *timeout, char *input, time_t cur_time)
{
  int mult_table[n_units]; // multipliers for units of time
  int 
    which, // which unit is being called for
    mult; // how many of that unit to add
  char *p; // for walking through string
  lastchar = '\0';
  // clear table
  memset(mult_table, 0, n_units*sizeof(int));
  // assemble multipliers
  for (p = input; *p; ) {
    mult = next_mult(&p);
    which = next_unit(&p);
    if (which < 0) {
      return false;
    }
    mult_table[which] += mult;
  }
  *timeout = translate_time(mult_table, cur_time);
  return true;
}

//=============================================================================
// input: A pointer to  C string.
// output: If the string begins with a sequence of digits, converts them to an
//   int and returns the int.  Otherwise, returns 1. 
// side effects: In the even that the string begins with a number, it advances
//   the pointer containing the string past the number.
// etiology: Called from parsec.  It's called before next_unit and only if
//   the input contains non-null chars.
//=============================================================================
int next_mult(char **input)
{
  int mult; // output value; next multiplier encountered
  if (!isdigit(**input)) {
    return 1;
  }
  mult = atoi(*input);
  for ( ; isdigit(**input); ++(*input)) ;
  return mult;
}

//=============================================================================
// input: A pointer to a C string.
// output: If the first char of the string is a valid code for a unit of time
//   then it returns the index value associated with that unit.  Otherwise
//   it returns an error code.  All error codes are less than 0 and are found
//   in the enum called parsec_errs.
// side effects: Advances the string one char.
// etiology: Called from parsec.  It's only called after next_mult (so it will
//   never encounter a digit) and only when the input contains non-null
//   chars, meaning that it should never be null and never be empty.
//=============================================================================
int next_unit(char **input)
{
  int which; // which code is encountered; index within unit string
  char *cp; // for finding position of (valid) codes in unit string
  // check for errors in the pointer
  if (!input) {
    return PTR_NULL;
  }
  if (!*input) {
    return STR_NULL;
  }
  // if this function received an empty string, there must have been a
  //   multiplier, and it's not an error.  therefore, return the default unit.
  if (!**input) {
    return default_unit;
  }
  cp = strchr(unit_codes, **input);
  lastchar = **input;
  // make sure that input was a valid code
  if (cp == NULL) {
    fprintf(stderr, "Bad character in time string: %c\n", **input);
    return CHAR_UNKNOWN;
  }
  which = cp - unit_codes;
  ++*input;
  return which;
}

//=============================================================================
// input: An array of ints representing the number of instances of each unit
//   of time.  The indices for the array are found in the enum units.
// output: converts the period of time to a number of seconds.
//=============================================================================
time_t translate_time(int *mult_table, time_t cur_time)
{
  struct tm *cal_time; // built in struct for holding parsed time
  int temp_mon; // month of year after seconds have been added but not month
  size_t max = 64; // max size of formatted time
  char *time_string; // to hold formatted time
  cal_time = localtime(&cur_time);
  max = 64; // arbitrary but probably sufficient
  time_string = (char*) malloc(max+1);
  if (DEBUG) {
    strftime(time_string, max, "%a %b %d, %G at %T", cal_time);
    printf("From now: %s\n", time_string);
  }
  //  temp_mon = cal_time->tm_mon; // we'll need this later
  // 12 months always equals a year, and we need to minimize the number of 
  // months because the time functions don't know how to wrap around
  mult_table[uyear] += mult_table[umon] / 12;
  mult_table[umon] %= 12;
  // first adjust all the values with guaranteed lengths
  time_t secs = (cur_time + mult_table[usec] * ssec +
  		 mult_table[umin] * smin + mult_table[uhour] * shour +
  		 mult_table[uday] * sday +mult_table[uweek] * sweek);
  // adjust the tm to match the adjusted time; all that's left is to add
  // the years and months
  cal_time = localtime(&secs);
  temp_mon = cal_time->tm_mon; // we'll need this later
  cal_time->tm_mon += mult_table[umon];
  cal_time->tm_year += mult_table[uyear];
  // if adding the months will cause a wrap around, we need to add the year
  // by hand and then subtract to get to the correct month
  if (mult_table[umon] + temp_mon >= 12) {
    cal_time->tm_year += 1;
    cal_time->tm_mon = temp_mon - (12 - mult_table[umon]);
  }
  if (DEBUG) {
    strftime(time_string, max, "%a %b %d, %G at %T", cal_time);
    printf("Til then: %s\n", time_string);
    secs = mktime(cal_time);
  }
  // return the epoch time of the end date
  secs = mktime(cal_time);
  return secs;

}

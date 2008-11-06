/* Debugging printf, for debugging the library itself.

   We don't assume stdio is working.
   We do assume _write_r is working.
*/

#include "iopload.h"
#include "iopdebug.h"

#ifdef __STDC__
#include "stdarg.h"
#else
#include "varargs.h"
#endif

static char *parse_number ();
static long get_number ();
static void print_number ();
static void write_char (char c);
static void write_string (char* s);

/* Non-zero for big-endian systems.  */
static int big_endian_p;

/* For now hardcode 2 (stderr) as the console file descriptor.
   May wish to let the caller pass in a file descriptor or some such but
   this is only for debugging purposes anyway.  */
#define CONSOLE_FD 2

/* Standalone printf routine.

   The format string has been enhanced so that multiple values can be dumped
   without having to have a %-field for each one (say if you want to dump
   20 words at a certain address).  A modifier of `N' says the next argument
   is a count, and the one after that is a pointer.

   Example: __dprintf (stderr, "%Nx\n", 20, p); /-* print 20 ints at `p' *-/

   Supported formats are: c d u x s p.

   All ints are retrieved a byte at a time so alignment issues are not
   a problem.

   This routine is used in situations where the only debugging capability
   is console output and was written to aid debugging newlib itself.  We don't
   use printf ourselves as we may be debugging it.  We do assume _write_r is
   working.
*/

void
#ifdef __STDC__
__printf (char *fmt, ...)
#else
__printf (fmt, va_alist)
     char *fmt;
     va_dcl
#endif
{
  va_list args;

  /* Which endian are we?  */
  {
    short tmp = 1;
    big_endian_p = *(char *) &tmp == 0;
  }

#ifdef __STDC__
  va_start (args, fmt);
#else
  va_start (args);
#endif

  while (*fmt)
    {
      char c, *p;
      int count;
      long l;

      if (*fmt != '%' || *++fmt == '%')
	{
	  write_char (*fmt++);
	  continue;
	}

      if (*fmt == 'N')
	{
	  count = va_arg (args, int);
	  p = va_arg (args, char *);
	  ++fmt;
	  c = *fmt++;

	  while (--count >= 0)
	    {
	      switch (c)
		{
//		case 'c' :
//		  write_string (unctrl (*p++));
//		  break;
		case 'p' :
		  print_number (16, 1, get_number (p, sizeof (char *), 1));
		  p += sizeof (char *);
		  break;
		case 'd' :
		case 'u' :
		case 'x' :
		  print_number (c == 'x' ? 16 : 10, c != 'd',
				get_number (p, sizeof (int), c != 'd'));
		  p += sizeof (int);
		  break;
		case 's' :
		  write_string (*(char **) p);
		  p += sizeof (char *);
		  break;
		}
	      if (count > 0)
		write_char (' ');
	    }
	}
      else
	{
	  switch (c = *fmt++)
	    {
//	    case 'c' :
//	      c = va_arg (args, int);
//	      write_string (unctrl (c));
//	      break;
	    case 'p' :
	      l = (void *) va_arg (args, char *);
	      print_number (16, 1, l);
	      break;
	    case 'd' :
	    case 'u' :
	    case 'x' :
	      l = va_arg (args, int);
	      print_number (c == 'x' ? 16 : 10, c != 'd', l);
	      break;
	    case 's' :
	      p = va_arg (args, char *);
	      write_string (p);
	      break;
	    }
	}
    }

  va_end (args);
}

static int isdigit(int c) {
	if (c >= '0' && c <= '9') return 1;
	return 0;
}

/* Parse a positive decimal integer at S.
   FIXME: Was used in earlier version, but not currently used.
   Keep for now.  */

static char *
parse_number (s, p)
     char *s;
     long *p;
{
  long x = 0;

  while (isdigit (*s))
    {
      x = (x * 10) + (*s - '0');
      ++s;
    }

  *p = x;
  return s;
}

/* Fetch the number at S of SIZE bytes.  */

static long
get_number (s, size, unsigned_p)
     char *s;
     long size;
     int unsigned_p;
{
  long x;
  unsigned char *p = (unsigned char *) s;

  switch (size)
    {
    case 1 :
      x = *p;
      if (!unsigned_p)
	x = (x ^ 0x80) - 0x80;
      return x;
    case 2 :
      if (big_endian_p)
	x = (p[0] << 8) | p[1];
      else
	x = (p[1] << 8) | p[0];
      if (!unsigned_p)
	x = (x ^ 0x8000) - 0x8000;
      return x;
    case 4 :
      if (big_endian_p)
	x = ((long)p[0] << 24) | ((long)p[1] << 16) | (p[2] << 8) | p[3];
      else
	x = ((long)p[3] << 24) | ((long)p[2] << 16) | (p[1] << 8) | p[0];
      if (!unsigned_p)
	x = (x ^ 0x80000000L) - 0x80000000L;
      return x;
#if 0 /* FIXME: Is there a standard mechanism for knowing if
	 long longs exist?  */
    case 8 :
#endif
    default :
      return 0;
  }
}

/* Print X in base BASE.  */

static void
print_number (base, unsigned_p, n)
     int base;
     int unsigned_p;
     long n;
{
  static char chars[16] = "0123456789abcdef";
  char *p, buf[32];
  unsigned long x;

  if (!unsigned_p && n < 0)
    {
      write_char ('-');
      x = -n;
    }
  else
    x = n;

  p = buf + sizeof (buf);
  *--p = '\0';
  do
    {
      *--p = chars[x % base];
      x /= base;
    }
  while (x != 0);

  write_string (p);
}

/* Write C to the console.
   We go through the file descriptor directly because we can't assume
   stdio is working.  */

static void
write_char (char c)
{
	__putc(c);
}

/* Write S to the console.
   We go through the file descriptor directly because we can't assume
   stdio is working.  */

static void
write_string (char *s)
{
	__puts(s);
}


void __putc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

void __puts(u8 *s) {
	while (*s != 0) {
		__putc(*s++);
	}
}


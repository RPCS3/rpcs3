/////////////////////////////////////////////////////////////////////////////
// Name:        glob.inc
// Purpose:     File and other globbing (included by utils.cpp)
// Author:      Karsten Ballueder
// Modified by:
// Created:     20/5/1998
// RCS-ID:      $Id: glob.inc 33948 2005-05-04 18:57:50Z JS $
// Copyright:   (c) Karsten Ballueder
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// This file includes:
//	wxIsWild(const char *pattern)
//	wxMatchWild(const char *pattern, const char *str, bool dot_special)
//

//---------------------------------------------------------------------------------
#ifndef UNIX_GLOB
# ifdef wx_x
#  define UNIX_GLOB 1
# else
#  define UNIX_GLOB 0
# endif
#endif

#if UNIX_GLOB
# ifdef wx_msw
#  error "Can't use Unix file globbing under Windows!"
# endif
#endif

/*************************************************************************
 *
 * wxIsWild checks whether the pattern contains wildcards, and
 * returns TRUE if it does, and FALSE if it does not (or if the 
 * pattern is NULL -- i.e. no string).
 *
 * The argument is:
 *   
 * 1) pattern - a character string
 */
bool 
wxIsWild (const char *pattern)
{
  while (*pattern)
    {
      switch (*pattern++)
	{
	case '?':
	case '*':
#if UNIX_GLOB
	case '[':
	case '{':		/* } */
#endif
	  return TRUE;
#if UNIX_GLOB
	case '\\':
	  if (!*pattern++)
	    return FALSE;
#endif
	}			/* switch() */
    }				/* while() */
  return FALSE;
}



#if UNIX_GLOB

// Unix Glob()
//
// Pattern        Function
// -----------------------------------------------------
//  '*'         = match 0 or more occurances of anything
// "[abc]"      = match anyof "abc" (ranges supported)
// "{xx,yy,zz}" = match anyof "xx", "yy", or "zz"
// '?'          = match any character
//
//  '\'  is used to "escape" special characters
// Recursive

bool 
wxMatchWild (const char *pattern, const char *str, bool dot_special)
{
  char c;
  const char *cp;
  bool done = FALSE, ret_code, ok;
  // Below is for vi fans
  const char OB = '{', CB = '}';

#if 0
  if (strcmp(pattern, "*.*") == 0)
    pattern = "*"; // Hack for MS-DOS compat.
#endif

  // dot_special means '.' only matches '.'
  if (dot_special && *str == '.' && *pattern != *str)
    return FALSE;

  while ((*pattern != '\0') && (!done) && (((*str == '\0') &&
	       ((*pattern == OB) || (*pattern == '*'))) || (*str != '\0')))
    {
      switch (*pattern)
	{
	case '\\':
	  pattern++;
	  if (*pattern != '\0')
	    pattern++;
	  break;
	case '*':
	  pattern++;
	  ret_code = FALSE;
	  while ((*str != '\0') && (!(ret_code = wxMatchWild (pattern, str++, FALSE))));
	  if (ret_code)
	    {
	      while (*str != '\0')
		str++;
	      while (*pattern != '\0')
		pattern++;
	    }
	  break;
	case '[':
	  pattern++;
	repeat:
	  if ((*pattern == '\0') || (*pattern == ']'))
	    {
	      done = TRUE;
	      break;
	    }
	  if (*pattern == '\\')
	    {
	      pattern++;
	      if (*pattern == '\0')
		{
		  done = TRUE;
		  break;
		}
	    }
	  if (*(pattern + 1) == '-')
	    {
	      c = *pattern;
	      pattern += 2;
	      if (*pattern == ']')
		{
		  done = TRUE;
		  break;
		}
	      if (*pattern == '\\')
		{
		  pattern++;
		  if (*pattern == '\0')
		    {
		      done = TRUE;
		      break;
		    }
		}
	      if ((*str < c) || (*str > *pattern))
		{
		  pattern++;
		  goto repeat;
		}
	    }
	  else if (*pattern != *str)
	    {
	      pattern++;
	      goto repeat;
	    }
	  pattern++;
	  while ((*pattern != ']') && (*pattern != '\0'))
	    {
	      if ((*pattern == '\\') && (*(pattern + 1) != '\0'))
		pattern++;
	      pattern++;
	    }			// while()
	  if (*pattern != '\0')
	    {
	      pattern++, str++;
	    }
	  break;
	case '?':
	  pattern++;
	  str++;
	  break;
	case OB:
	  pattern++;
         while ((*pattern != CB) && (*pattern != '\0'))
	    {
	      cp = str;
	      ok = TRUE;
	      while (ok && (*cp != '\0') && (*pattern != '\0') &&
                 (*pattern != ',') && (*pattern != CB))
		{
		  if (*pattern == '\\')
		    pattern++;
		  ok = (*pattern++ == *cp++);
		}		// while()
	      if (*pattern == '\0')
		{
		  ok = FALSE;
		  done = TRUE;
		  break;
		}
	      else if (ok)
		{
		  str = cp;
                   while ((*pattern != CB) && (*pattern != '\0'))
		    {
		      if (*++pattern == '\\')
			{
                      if (*++pattern == CB)
			    pattern++;
			}
		    }		// while()
		}
	      else
		{
                 while (*pattern != CB && *pattern != ',' && *pattern != '\0')
		    {
		      if (*++pattern == '\\')
			{
                            if (*++pattern == CB || *pattern == ',')
			    pattern++;
			}
		    }		// while()
		}
	      if (*pattern != '\0')
		pattern++;
	    }			// while()
	  break;
	default:
	  if (*str == *pattern)
	    {
	      str++, pattern++;
	    }
	  else
	    {
	      done = TRUE;
	    }
	}			// switch()
    }				// while()
  while (*pattern == '*')
    pattern++;
  return ((*str == '\0') && (*pattern == '\0'));
}

#else /* MS-DOS/Windows glob() */
/*************************************************************************
 *
 *  wxMatchWild matches the given pattern string against 
 *  a text string, and returns TRUE if it matches, FALSE otherwise.
 *
 *  A match means that the entire text string is used up in the matching.
 *  The pattern can contain the following wildcards.
 * 
 *  * -- matches any sequence of characters
 *  ? -- matches one character
 *
 * If one or other or both of the string arguments to wxMatchWild function is  
 * NULL (i.e. there isn't a string), then the function returns FALSE.
 *
 */
static bool wxPatternMatch (const char *pattern, const char *text, size_t i, size_t j);

// @@@@ dotSpecial is ignored by MS-DOS
bool 
wxMatchWild (const char *pattern, const char *text, bool /* dotSpecial */ )
{
  if (pattern == NULL || text == NULL || *pattern == '\0' || *text == '\0')
    return FALSE;
  return wxPatternMatch (pattern, text, 0, 0);
}

/*************************************************************************
 *
 *  wxPatternMatch does the work for wxMatchWild. wxPatternMatch  matches 
 *  the given pattern string against a text string, and returns TRUE if 
 *  it matches, FALSE otherwise. It is assumed that the string arguments
 *  to wxPatternMatch exist.
 *
 *  A match means that the entire text string is used up in the matching.
 *  The pattern can contain the following wildcards.
 * 
 *  * -- matches any sequence of characters
 *  ? -- matches one character
 *
 *  wxPatternMatch works by going down the pattern trying to match the
 *  the same index character in the pattern and string arrays, and stops
 *  when the end of the pattern or text string is reached. However, if a
 *  '*' wildcard is met, the algorithm checks to see whether the remaining 
 *  pattern (after the wildcard) matches the rest of the text (i.e. the 
 *  wxPatternMatch function is called recursively).
 */
// Recursive
static bool 
wxPatternMatch (const char *pattern, const char *text, size_t i, size_t j)
{
  size_t pattern_length = strlen (pattern);
  size_t text_length = strlen (text);
  bool match = FALSE;

#ifdef wx_msw
// MS-DOS file system is case INDEPENDENT
# define EQU(x,y) (wxToLower(x) == wxToLower(y))
#else
# define EQU(x,y) ((x) == (y))
#endif

  while (j < pattern_length && i < text_length)
    {
      if (EQU(text[i], pattern[j]) || pattern[j] == '?')
	{
	  match = TRUE;
	  i++, j++;
	}
      else if (pattern[j] == '*')
	{
	  // If pattern ends in '*'
	  if (++j == pattern_length)
	    {
	      match = TRUE;
	      i = text_length;
	    }
	  else
	    {
	      match = FALSE;
// after wildcard check to see whether rest of pattern matches 
	      // up with rest of text
	      while (i < text_length && match != TRUE)
		{
		  match = wxPatternMatch (pattern, text, i, j);
		  i++;
		}
// text index is decremented so that it points to where 
	      // the text string starts to match the rest of the pattern
	      i--;
	    }
	}
      else if (! EQU(text[i], pattern[j]))
	{
	  j = pattern_length;
	  match = FALSE;
	}
    }
  if (j == pattern_length && i == text_length && match == TRUE)
    {
      return TRUE;
    }
  else
// special case where pattern and text are the same except that pattern
    // also only has '*' wildcards on the end
  if (i == text_length && pattern[j] == '*' && match == TRUE)
    {
      for (; j < pattern_length; j++)
	{
	  if (pattern[j] != '*')
	    return FALSE;
	}
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

#endif /* UNIX_GLOB */
//-----------------------------------------------------------------------------


#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef WIN32
#define STRNICMP strnicmp
#else 
#define STRNICMP strncasecmp
#endif 

typedef struct _tag_spec
{
  short
    id;

  char
    *name;
} tag_spec;

static tag_spec tags[] = {
    { 5,"Image Name" },
    { 7,"Edit Status" },
    { 10,"Priority" },
    { 15,"Category" },
    { 20,"Supplemental Category" },
    { 22,"Fixture Identifier" },
    { 25,"Keyword" },
    { 30,"Release Date" },
    { 35,"Release Time" },
    { 40,"Special Instructions" },
    { 45,"Reference Service" },
    { 47,"Reference Date" },
    { 50,"Reference Number" },
    { 55,"Created Date" },
    { 60,"Created Time" },
    { 65,"Originating Program" },
    { 70,"Program Version" },
    { 75,"Object Cycle" },
    { 80,"Byline" },
    { 85,"Byline Title" },
    { 90,"City" },
    { 95,"Province State" },
    { 100,"Country Code" },
    { 101,"Country" },
    { 103,"Original Transmission Reference" },
    { 105,"Headline" },
    { 110,"Credit" },
    { 115,"Source" },
    { 116,"Copyright String" },
    { 120,"Caption" },
    { 121,"Local Caption" },
    { 122,"Caption Writer" },
    { 200,"Custom Field 1" },
    { 201,"Custom Field 2" },
    { 202,"Custom Field 3" },
    { 203,"Custom Field 4" },
    { 204,"Custom Field 5" },
    { 205,"Custom Field 6" },
    { 206,"Custom Field 7" },
    { 207,"Custom Field 8" },
    { 208,"Custom Field 9" },
    { 209,"Custom Field 10" },
    { 210,"Custom Field 11" },
    { 211,"Custom Field 12" },
    { 212,"Custom Field 13" },
    { 213,"Custom Field 14" },
    { 214,"Custom Field 15" },
    { 215,"Custom Field 16" },
    { 216,"Custom Field 17" },
    { 217,"Custom Field 18" },
    { 218,"Custom Field 19" },
    { 219,"Custom Field 20" }
};

/*
 * We format the output using HTML conventions
 * to preserve control characters and such.
 */
void formatString(FILE *ofile, const char *s, int len)
{
  putc('"', ofile);
  for (; len > 0; --len, ++s) {
    int c = *s;
    switch (c) {
    case '&':
      fputs("&amp;", ofile);
      break;
#ifdef HANDLE_GT_LT
    case '<':
      fputs("&lt;", ofile);
      break;
    case '>':
      fputs("&gt;", ofile);
      break;
#endif
    case '"':
      fputs("&quot;", ofile);
      break;
    default:
      if (iscntrl(c))
        fprintf(ofile, "&#%d;", c);
      else
        putc(*s, ofile);
      break;
    }
  }
  fputs("\"\n", ofile);
}

typedef struct _html_code
{
  short
    len;
  const char
    *code,
    val;
} html_code;

static html_code html_codes[] = {
#ifdef HANDLE_GT_LT
    { 4,"&lt;",'<' },
    { 4,"&gt;",'>' },
#endif
    { 5,"&amp;",'&' },
    { 6,"&quot;",'"' }
};

/*
 * This routine converts HTML escape sequence
 * back to the original ASCII representation.
 * - returns the number of characters dropped.
 */
int convertHTMLcodes(char *s, int len)
{
  if (len <=0 || s==(char*)NULL || *s=='\0')
    return 0;

  if (s[1] == '#')
    {
      int val, o;

      if (sscanf(s,"&#%d;",&val) == 1)
      {
        o = 3;
        while (s[o] != ';')
        {
          o++;
          if (o > 5)
            break;
        }
        if (o < 5)
          strcpy(s+1, s+1+o);
        *s = val;
        return o;
      }
    }
  else
    {
      int
        i,
        codes = sizeof(html_codes) / sizeof(html_code);

      for (i=0; i < codes; i++)
      {
        if (html_codes[i].len <= len)
          if (STRNICMP(s, html_codes[i].code, html_codes[i].len) == 0)
            {
              strcpy(s+1, s+html_codes[i].len);
              *s = html_codes[i].val;
              return html_codes[i].len-1;
            }
      }
    }

  return 0;
}

int formatIPTC(FILE *ifile, FILE *ofile)
{
  unsigned int
    foundiptc,
    tagsfound;

  unsigned char
    recnum,
    dataset;

  char
    *readable,
    *str;

  long
    tagindx,
    taglen;

  int
    i,
    tagcount = sizeof(tags) / sizeof(tag_spec);

  char
    c;

  foundiptc = 0; /* found the IPTC-Header */
  tagsfound = 0; /* number of tags found */

  c = getc(ifile);
  while (c != EOF)
  {
	  if (c == 0x1c)
	    foundiptc = 1;
	  else
      {
        if (foundiptc)
	        return -1;
        else
	        continue;
	    }

    /* we found the 0x1c tag and now grab the dataset and record number tags */
    dataset = getc(ifile);
	  if ((char) dataset == EOF)
	    return -1;
    recnum = getc(ifile);
	  if ((char) recnum == EOF)
	    return -1;
    /* try to match this record to one of the ones in our named table */
    for (i=0; i< tagcount; i++)
    {
      if (tags[i].id == recnum)
          break;
    }
    if (i < tagcount)
      readable = tags[i].name;
    else
      readable = "";

    /* then we decode the length of the block that follows - long or short fmt */
    c = getc(ifile);
	  if (c == EOF)
	    return 0;
	  if (c & (unsigned char) 0x80)
      {
        unsigned char
          buffer[4];

        for (i=0; i<4; i++)
        {
          c = buffer[i] = getc(ifile);
          if (c == EOF)
            return -1;
        }
        taglen = (((long) buffer[ 0 ]) << 24) |
                 (((long) buffer[ 1 ]) << 16) | 
	               (((long) buffer[ 2 ]) <<  8) |
                 (((long) buffer[ 3 ]));
	    }
    else
      {
        unsigned char
          x = c;

        taglen = ((long) x) << 8;
        x = getc(ifile);
        if ((char)x == EOF)
          return -1;
        taglen |= (long) x;
	    }
    /* make a buffer to hold the tag data and snag it from the input stream */
    str = (char *) malloc((unsigned int) (taglen+1));
    if (str == (char *) NULL)
      {
        printf("Memory allocation failed");
        return 0;
      }
    for (tagindx=0; tagindx<taglen; tagindx++)
    {
      c = str[tagindx] = getc(ifile);
      if (c == EOF)
      {
          free(str);
          return -1;
      }
    }
    str[ taglen ] = 0;

    /* now finish up by formatting this binary data into ASCII equivalent */
    if (strlen(readable) > 0)
	    fprintf(ofile, "%d#%d#%s=",(unsigned int)dataset, (unsigned int) recnum, readable);
    else
	    fprintf(ofile, "%d#%d=",(unsigned int)dataset, (unsigned int) recnum);
    formatString( ofile, str, taglen );
    free(str);

	  tagsfound++;

    c = getc(ifile);
  }
  return tagsfound;
}

int tokenizer(unsigned inflag,char *token,int tokmax,char *line,
char *white,char *brkchar,char *quote,char eschar,char *brkused,
int *next,char *quoted);

char *super_fgets(char *b, int *blen, FILE *file)
{
  int
    c,
    len;

  char
    *q;

  len=*blen;
  for (q=b; ; q++)
  {
    c=fgetc(file);
    if (c == EOF || c == '\n')
      break;
    if (((long)q - (long)b + 1 ) >= (long) len)
      {
        long
          tlen;

        tlen=(long)q-(long)b;
        len<<=1;
        b=(char *) realloc((char *) b,(len+2));
        if ((char *) b == (char *) NULL)
          break;
        q=b+tlen;
      }
    *q=(unsigned char) c;
  }
  *blen=0;
  if ((unsigned char *)b != (unsigned char *) NULL)
    {
      int
        tlen;

      tlen=(long)q - (long)b;
      if (tlen == 0)
        return (char *) NULL;
      b[tlen] = '\0';
      *blen=++tlen;
    }
  return b;
}

#define BUFFER_SZ 4096

int main(int argc, char *argv[])
{            
  unsigned int
    length;

  unsigned char
    *buffer;

  int
    i,
    mode; /* iptc binary, or iptc text */

  FILE
    *ifile = stdin,
    *ofile = stdout;

  char
    c,
    *usage = "usage: iptcutil -t | -b [-i file] [-o file] <input >output";

  if( argc < 2 )
    {
      puts(usage);
	    return 1;
    }

  mode = 0;
  length = -1;
  buffer = (unsigned char *)NULL;

  for (i=1; i<argc; i++)
  {
    c = argv[i][0];
    if (c == '-' || c == '/')
      {
        c = argv[i][1];
        switch( c )
        {
        case 't':
	        mode = 1;
#ifdef WIN32
          /* Set "stdout" to binary mode: */
          _setmode( _fileno( ofile ), _O_BINARY );
#endif
	        break;
        case 'b':
	        mode = 0;
#ifdef WIN32
          /* Set "stdin" to binary mode: */
          _setmode( _fileno( ifile ), _O_BINARY );
#endif
	        break;
        case 'i':
          if (mode == 0)
            ifile = fopen(argv[++i], "rb");
          else
            ifile = fopen(argv[++i], "rt");
          if (ifile == (FILE *)NULL)
            {
	            printf("Unable to open: %s\n", argv[i]);
              return 1;
            }
	        break;
        case 'o':
          if (mode == 0)
            ofile = fopen(argv[++i], "wt");
          else
            ofile = fopen(argv[++i], "wb");
          if (ofile == (FILE *)NULL)
            {
	            printf("Unable to open: %s\n", argv[i]);
              return 1;
            }
	        break;
        default:
	        printf("Unknown option: %s\n", argv[i]);
	        return 1;
        }
      }
    else
      {
        puts(usage);
	      return 1;
      }
  }

  if (mode == 0) /* handle binary iptc info */
    formatIPTC(ifile, ofile);

  if (mode == 1) /* handle text form of iptc info */
    {
      char
        brkused,
        quoted,
        *line,
        *token,
        *newstr;

      int
        state,
        next;

      unsigned char
        recnum = 0,
        dataset = 0;

      int
        inputlen = BUFFER_SZ;

      line = (char *) malloc(inputlen);     
      token = (char *)NULL;
      while((line = super_fgets(line,&inputlen,ifile))!=NULL)
      {
        state=0;
        next=0;

        token = (char *) malloc(inputlen);     
        newstr = (char *) malloc(inputlen);     
        while(tokenizer(0, token, inputlen, line, "", "=", "\"", 0,
          &brkused,&next,&quoted)==0)
        {
          if (state == 0)
            {                  
              int
                state,
                next;

              char
                brkused,
                quoted;

              state=0;
              next=0;
              while(tokenizer(0, newstr, inputlen, token, "", "#", "", 0,
                &brkused, &next, &quoted)==0)
              {
                if (state == 0)
                  dataset = (unsigned char) atoi(newstr);
                else
                   if (state == 1)
                     recnum = (unsigned char) atoi(newstr);
                state++;
              }
            }
          else
            if (state == 1)
              {
                int
                  next;

                unsigned long
                  len;

                char
                  brkused,
                  quoted;

                next=0;
                len = strlen(token);
                while(tokenizer(0, newstr, inputlen, token, "", "&", "", 0,
                  &brkused, &next, &quoted)==0)
                {
                  if (brkused && next > 0)
                    {
                      char
                        *s = &token[next-1];

                      len -= convertHTMLcodes(s, strlen(s));
                    }
                }

                fputc(0x1c, ofile);
                fputc(dataset, ofile);
                fputc(recnum, ofile);
                if (len < 0x10000)
                  {
                    fputc((len >> 8) & 255, ofile);
                    fputc(len & 255, ofile);
                  }
                else
                  {
                    fputc(((len >> 24) & 255) | 0x80, ofile);
                    fputc((len >> 16) & 255, ofile);
                    fputc((len >> 8) & 255, ofile);
                    fputc(len & 255, ofile);
                  }
                next=0;
                while (len--)
                  fputc(token[next++], ofile);
              }
          state++;
        }
        free(token);
        token = (char *)NULL;
        free(newstr);
        newstr = (char *)NULL;
      }
      free(line);

      fclose( ifile );
      fclose( ofile );
    }

  return 0;
}

/*
	This routine is a generalized, finite state token parser. It allows
    you extract tokens one at a time from a string of characters.  The
    characters used for white space, for break characters, and for quotes
    can be specified. Also, characters in the string can be preceded by
    a specifiable escape character which removes any special meaning the
    character may have.

	There are a lot of formal parameters in this subroutine call, but
	once you get familiar with them, this routine is fairly easy to use.
	"#define" macros can be used to generate simpler looking calls for
	commonly used applications of this routine.

	First, some terminology:

	token:		used here, a single unit of information in
				the form of a group of characters.

	white space:	space that gets ignored (except within quotes
				or when escaped), like blanks and tabs.  in
				addition, white space terminates a non-quoted
				token.

	break character: a character that separates non-quoted tokens.
				commas are a common break character.  the
				usage of break characters to signal the end
				of a token is the same as that of white space,
				except multiple break characters with nothing
				or only white space between generate a null
				token for each two break characters together.

				for example, if blank is set to be the white
				space and comma is set to be the break
				character, the line ...

				A, B, C ,  , DEF

				... consists of 5 tokens:

				1)	"A"
				2)	"B"
				3)	"C"
				4)	""      (the null string)
				5)	"DEF"

	quote character: 	a character that, when surrounding a group
				of other characters, causes the group of
				characters to be treated as a single token,
				no matter how many white spaces or break
				characters exist in the group.	also, a
				token always terminates after the closing
				quote.	for example, if ' is the quote
				character, blank is white space, and comma
				is the break character, the following
				string ...

				A, ' B, CD'EF GHI

				... consists of 4 tokens:

				1)	"A"
				2)	" B, CD" (note the blanks & comma)
				3)	"EF"
				4)	"GHI"

				the quote characters themselves do
				not appear in the resultant tokens.  the
				double quotes are delimiters i use here for
				documentation purposes only.

	escape character:	a character which itself is ignored but
				which causes the next character to be
				used as is.  ^ and \ are often used as
				escape characters.  an escape in the last
				position of the string gets treated as a
				"normal" (i.e., non-quote, non-white,
				non-break, and non-escape) character.
				for example, assume white space, break
				character, and quote are the same as in the
				above examples, and further, assume that
				^ is the escape character.  then, in the
				string ...

				ABC, ' DEF ^' GH' I ^ J K^ L ^

				... there are 7 tokens:

				1)	"ABC"
				2)	" DEF ' GH"
				3)	"I"
				4)	" "     (a lone blank)
				5)	"J"
				6)	"K L"
				7)	"^"     (passed as is at end of line)


	OK, now that you have this background, here's how to call "tokenizer":

	result=tokenizer(flag,token,maxtok,string,white,break,quote,escape,
		      brkused,next,quoted)

	result: 	0 if we haven't reached EOS (end of string), and
			1 if we have (this is an "int").

	flag:		right now, only the low order 3 bits are used.
			1 => convert non-quoted tokens to upper case
			2 => convert non-quoted tokens to lower case
			0 => do not convert non-quoted tokens
			(this is a "char").

	token:		a character string containing the returned next token
			(this is a "char[]").

	maxtok: 	the maximum size of "token".  characters beyond
			"maxtok" are truncated (this is an "int").

	string: 	the string to be parsed (this is a "char[]").

	white:		a string of the valid white spaces.  example:

			char whitesp[]={" \t"};

			blank and tab will be valid white space (this is
			a "char[]").

	break:		a string of the valid break characters.  example:

			char breakch[]={";,"};

			semicolon and comma will be valid break characters
			(this is a "char[]").

			IMPORTANT:  do not use the name "break" as a C
			variable, as this is a reserved word in C.

	quote:		a string of the valid quote characters.  an example
			would be

			char whitesp[]={"'\"");

			(this causes single and double quotes to be valid)
			note that a token starting with one of these characters
			needs the same quote character to terminate it.

			for example,

			"ABC '

			is unterminated, but

			"DEF" and 'GHI'

			are properly terminated.  note that different quote
			characters can appear on the same line; only for
			a given token do the quote characters have to be
			the same (this is a "char[]").

	escape: 	the escape character (NOT a string ... only one
			allowed).  use zero if none is desired (this is
			a "char").

	brkused:	the break character used to terminate the current
			token.	if the token was quoted, this will be the
			quote used.  if the token is the last one on the
			line, this will be zero (this is a pointer to a
			"char").

	next:		this variable points to the first character of the
			next token.  it gets reset by "tokenizer" as it steps
			through the string.  set it to 0 upon initialization,
			and leave it alone after that.	you can change it
			if you want to jump around in the string or re-parse
			from the beginning, but be careful (this is a
			pointer to an "int").

	quoted: 	set to 1 (true) if the token was quoted and 0 (false)
			if not.  you may need this information (for example:
			in C, a string with quotes around it is a character
			string, while one without is an identifier).

			(this is a pointer to a "char").
*/

/* states */

#define IN_WHITE 0
#define IN_TOKEN 1
#define IN_QUOTE 2
#define IN_OZONE 3

int _p_state;	   /* current state	 */
unsigned _p_flag;  /* option flag	 */
char _p_curquote;  /* current quote char */
int _p_tokpos;	   /* current token pos  */

/* routine to find character in string ... used only by "tokenizer" */

int sindex(char ch,char *string)
{
  char *cp;
  for(cp=string;*cp;++cp)
    if(ch==*cp)
      return (int)(cp-string);	/* return postion of character */
  return -1;			/* eol ... no match found */
}

/* routine to store a character in a string ... used only by "tokenizer" */

void chstore(char *string,int max,char ch)
{
  char c;
  if(_p_tokpos>=0&&_p_tokpos<max-1)
  {
    if(_p_state==IN_QUOTE)
      c=ch;
    else
      switch(_p_flag&3)
      {
	    case 1: 	    /* convert to upper */
	      c=toupper(ch);
	      break;

	    case 2: 	    /* convert to lower */
	      c=tolower(ch);
	      break;

	    default:	    /* use as is */
	      c=ch;
	      break;
      }
    string[_p_tokpos++]=c;
  }
  return;
}

int tokenizer(unsigned inflag,char *token,int tokmax,char *line,
  char *white,char *brkchar,char *quote,char eschar,char *brkused,
    int *next,char *quoted)
{
  int qp;
  char c,nc;

  *brkused=0;		/* initialize to null */
  *quoted=0;		/* assume not quoted  */

  if(!line[*next])	/* if we're at end of line, indicate such */
    return 1;

  _p_state=IN_WHITE;   /* initialize state */
  _p_curquote=0;	   /* initialize previous quote char */
  _p_flag=inflag;	   /* set option flag */

  for(_p_tokpos=0;(c=line[*next]);++(*next))	/* main loop */
  {
    if((qp=sindex(c,brkchar))>=0)  /* break */
    {
      switch(_p_state)
      {
	    case IN_WHITE:		/* these are the same here ...	*/
	    case IN_TOKEN:		/* ... just get out		*/
	    case IN_OZONE:		/* ditto			*/
	      ++(*next);
	      *brkused=brkchar[qp];
	      goto byebye;

	    case IN_QUOTE:		 /* just keep going */
	      chstore(token,tokmax,c);
	      break;
      }
    }
    else if((qp=sindex(c,quote))>=0)  /* quote */
    {
      switch(_p_state)
      {
	    case IN_WHITE:	 /* these are identical, */
	      _p_state=IN_QUOTE; /* change states   */
	      _p_curquote=quote[qp]; /* save quote char */
	      *quoted=1;	/* set to true as long as something is in quotes */
	      break;

	    case IN_QUOTE:
	      if(quote[qp]==_p_curquote) /* same as the beginning quote? */
	      {
	        _p_state=IN_OZONE;
	        _p_curquote=0;
	      }
	      else
	        chstore(token,tokmax,c); /* treat as regular char */
	      break;

	    case IN_TOKEN:
	    case IN_OZONE:
	      *brkused=c; /* uses quote as break char */
	      goto byebye;
      }
    }
    else if((qp=sindex(c,white))>=0) /* white */
    {
      switch(_p_state)
      {
	    case IN_WHITE:
	    case IN_OZONE:
	      break;		/* keep going */

	    case IN_TOKEN:
	      _p_state=IN_OZONE;
	      break;

	    case IN_QUOTE:
	      chstore(token,tokmax,c); /* it's valid here */
	      break;
      }
    }
    else if(c==eschar)  /* escape */
    {
      nc=line[(*next)+1];
      if(nc==0) 		/* end of line */
      {
	    *brkused=0;
	    chstore(token,tokmax,c);
	    ++(*next);
	    goto byebye;
      }
      switch(_p_state)
      {
	    case IN_WHITE:
	      --(*next);
	      _p_state=IN_TOKEN;
	      break;

	    case IN_TOKEN:
	    case IN_QUOTE:
	      ++(*next);
	      chstore(token,tokmax,nc);
	      break;

	    case IN_OZONE:
	      goto byebye;
      }
    }
    else	/* anything else is just a real character */
    {
      switch(_p_state)
      {
	    case IN_WHITE:
	      _p_state=IN_TOKEN; /* switch states */

	    case IN_TOKEN:		 /* these 2 are     */
	    case IN_QUOTE:		 /*  identical here */
	      chstore(token,tokmax,c);
	      break;

	    case IN_OZONE:
	      goto byebye;
      }
    }
  }		/* end of main loop */

byebye:
  token[_p_tokpos]=0;	/* make sure token ends with EOS */

  return 0;
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */

/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/**********************************************************************
 *
 * $XConsortium: list.c,v 1.20 91/01/09 17:13:30 rws Exp $
 *
 * TWM code to deal with the name lists for the NoTitle list and
 * the AutoRaise list
 *
 * 11-Apr-88 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

/*
 * Stolen from TVTWM pl11, updated it to conform to the POSIX 1003.2
 * regex spec, backported VTWM 5.3's internal wildcarding code, and
 * made it work without regex support.
 *
 * D. J. Hawkey Jr. - 10/20/01
 */

#include <stdio.h>
#include <X11/Xatom.h>

#ifndef NO_REGEX_SUPPORT
#include <sys/types.h>
#include <regex.h>
#endif

#include "twm.h"
#include "screen.h"
#include "list.h"
#include "gram.h"
#include "prototypes.h"

#define REGCOMP_FLAGS		(REG_EXTENDED | REG_NOSUB)

struct name_list_struct
{
  name_list *next;		/* pointer to the next name */
  char *name;			/* the name of the window */
#ifndef NO_REGEX_SUPPORT
  regex_t re;			/* compile only once */
#else
  char re;			/* not used */
#endif
  short type;			/* what type of match */
  Atom property;		/* if (type == property) */
  char *ptr;			/* list dependent data */
};

#ifndef NO_REGEX_SUPPORT
static char buffer[256];
#endif

static int match(char *p, char *t);

/***********************************************************************
 *
 * Wrappers to allow code to step through a list
 *
 ***********************************************************************/

name_list *
next_entry(name_list * list)
{
  return (list->next);
}

char *
contents_of_entry(name_list * list)
{
  return (list->ptr);
}

/**********************************************************************/

#ifdef DEBUG
static void
printNameList(char *name, name_list * nptr)
{
  printf("printNameList(): %s=[", name);

  while (nptr)
  {
    printf(" '%s':%d", nptr->name, nptr->type);
    nptr = nptr->next;
  }

  printf(" ]\n");
}
#endif

/***********************************************************************
 *
 *  Procedure:
 *	AddToList - add a window name to the appropriate list
 *
 *  Inputs:
 *	list	- the address of the pointer to the head of a list
 *	name	- a pointer to the name of the window
 *	type	- a bitmask of what to match against
 *	property- a window propery to match against
 *	ptr	- pointer to list dependent data
 *
 *  Special Considerations
 *	If the list does not use the ptr value, a non-null value
 *	should be placed in it.  LookInList returns this ptr value
 *	and procedures calling LookInList will check for a non-null
 *	return value as an indication of success.
 *
 ***********************************************************************
 */

void
AddToList(name_list ** list_head, char *name, int type, char *ptr)
{
  Atom property = None;
  name_list *nptr;

  if (!list_head)
    return;			/* ignore empty inserts */

  nptr = (name_list *) malloc(sizeof(name_list));
  if (nptr == NULL)
  {
    fprintf(stderr, "%s: unable to allocate %lu bytes for name_list\n", ProgramName, sizeof(name_list));
    Done(0);
  }

  nptr->next = *list_head;

  nptr->name = strdup(name);
  if (type & LTYPE_HOST)
  {
    nptr->type = (type & ~LTYPE_HOST) | LTYPE_PROPERTY;
    nptr->property = XA_WM_CLIENT_MACHINE;
  }
  else
  {
    nptr->type = type;
    nptr->property = property;
  }
  nptr->ptr = (ptr == NULL) ? (char *)TRUE : ptr;

  *list_head = nptr;
}

 /********************************************************************\
 *                                                                    *
 * New LookInList code by RJC.                                        *
 *                                                                    *
 * Since we want to be able to look for multiple matches (eg, to      *
 * check which relevant icon regions are on the screen), the basic    *
 * procedure is now MultiLookInList and uses a (pseudo-)continuation  *
 * to keep track of where it is.                                      *
 *                                                                    *
 * LookInList is a trivial specialisation of that.                    *
 *                                                                    *
 * Also, we now allow regular expressions in lists, so here we use    *
 * Henry Spencer's regular expression code.  It is possible that we   *
 * should pre-compile all the regular expressions for maximum         *
 * speed.                                                             *
 *                                                                    *
 \********************************************************************/

int
MatchName(char *name, char *pattern,
#ifndef NO_REGEX_SUPPORT
	  regex_t * compiled,
#else
	  char *compiled,
#endif
	  short type)
{
#ifdef DEBUG
  fprintf(stderr, "MatchName(): compare '%s' with '%s'\n", name, pattern);
#endif

  if (type & LTYPE_ANYTHING)
    return (0);

  if (type & LTYPE_REGEXP)
  {
#ifndef NO_REGEX_SUPPORT
    regex_t re;
    int result;

    if ((result = regcomp(&re, pattern, REGCOMP_FLAGS)) != 0)
    {
      regerror(result, &re, buffer, sizeof(buffer));
      regfree(&re);

      fprintf(stderr, "%s: (1) regcomp(\"%s\") error: %s\n", ProgramName, pattern, buffer);
      return (result);
    }

    result = regexec(&re, name, 0, NULL, 0);
    regfree(&re);

    return (result);
#else
    fprintf(stderr, "%s: (1) no support for regcomp(\"%s\")\n", ProgramName, pattern);
    return (1);
#endif
  }

  if (type & LTYPE_C_REGEXP)
  {
#ifndef NO_REGEX_SUPPORT
    return (regexec(compiled, name, 0, NULL, 0));
#else
    fprintf(stderr, "%s: no support for regexec(\"%s\")\n", ProgramName, name);
    return (1);
#endif
  }

  if (type & LTYPE_STRING)
    return (match(pattern, name));

  fprintf(stderr, "%s: bad list type (%d) comparing \"%s\" with \"%s\"\n", ProgramName, type, name, pattern);
  return (1);
}

static char *
MultiLookInList(name_list * list_head, char *name, XClassHint * class, name_list ** continuation)
{
  name_list *nptr;

#ifdef DEBUG
  fprintf(stderr, "MultiLookInList(): looking for '%s'\n", name);
#endif

  for (nptr = list_head; nptr; nptr = nptr->next)
  {
    /* pre-compile and cache the regex_t */
    if (nptr->type & LTYPE_REGEXP)
    {
#ifndef NO_REGEX_SUPPORT
      int result;

      if ((result = regcomp(&nptr->re, nptr->name, REGCOMP_FLAGS)) != 0)
      {
	regerror(result, &nptr->re, buffer, sizeof(buffer));
	regfree(&nptr->re);

	fprintf(stderr, "%s: (2) regcomp(\"%s\") error: %s\n", ProgramName, nptr->name, buffer);

	nptr->type |= LTYPE_NOTHING;
      }
      else
	nptr->type |= LTYPE_C_REGEXP;
#else
      fprintf(stderr, "%s: (2) no support for regcomp(\"%s\")\n", ProgramName, nptr->name);

      nptr->type |= LTYPE_NOTHING;
#endif

      nptr->type &= ~LTYPE_REGEXP;
    }

    if (nptr->type & LTYPE_NOTHING)
      continue;			/* skip illegal entry */

    if (nptr->type & LTYPE_ANYTHING)
    {
      *continuation = nptr->next;
      return (nptr->ptr);
    }

    if (nptr->type & LTYPE_NAME)
      if (MatchName(name, nptr->name, &nptr->re, nptr->type) == 0)
      {
	*continuation = nptr->next;
	return (nptr->ptr);
      }

    if (class)
    {
      if (nptr->type & LTYPE_RES_NAME)
	if (MatchName(class->res_name, nptr->name, &nptr->re, nptr->type) == 0)
	{
	  *continuation = nptr->next;
	  return (nptr->ptr);
	}

      if (nptr->type & LTYPE_RES_CLASS)
	if (MatchName(class->res_class, nptr->name, &nptr->re, nptr->type) == 0)
	{
	  *continuation = nptr->next;
	  return (nptr->ptr);
	}
    }

  }

  *continuation = NULL;
  return (NULL);
}

char *
LookInList(name_list * list_head, char *name, XClassHint * class)
{
  name_list *rest;
  char *return_name = MultiLookInList(list_head, name, class, &rest);

  return (return_name);
}


char *
LookInNameList(name_list * list_head, char *name)
{
  return (MultiLookInList(list_head, name, NULL, &list_head));
}

/***********************************************************************
 *
 *  Procedure:
 *	GetColorFromList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	TRUE if the name was found
 *	FALSE if the name was not found
 *
 *  Inputs:
 *	list	- a pointer to the head of a list
 *	name	- a pointer to the name to look for
 *	class	- a pointer to the class to look for
 *
 *  Outputs:
 *	ptr	- fill in the list value if the name was found
 *
 ***********************************************************************
 */

int
GetColorFromList(name_list * list_head, char *name, XClassHint * class, Pixel * ptr)
{
  int save;
  char *val = LookInList(list_head, name, class);

  if (val)
  {
    save = Scr->FirstTime;
    Scr->FirstTime = TRUE;
    GetColor(Scr->Monochrome, ptr, val);
    Scr->FirstTime = save;

    return (TRUE);
  }

  return (FALSE);
}

/***********************************************************************
 *
 *  Procedure:
 *	FreeList - free up a list
 *
 ***********************************************************************
 */

void
FreeList(name_list ** list)
{
  name_list *nptr;
  name_list *tmp;

  for (nptr = *list; nptr != NULL;)
  {
    tmp = nptr->next;

#ifndef NO_REGEX_SUPPORT
    if (nptr->type & LTYPE_C_REGEXP)
      regfree(&nptr->re);
#endif
    free(nptr->name);
    free((char *)nptr);

    nptr = tmp;
  }

  *list = NULL;
}



#define ABORT 2

static int regex_match(char *p, char *t);

static int
regex_match_after_star(char *p, char *t)
{
  register int match;
  register int nextp;

  while ((*p == '?') || (*p == '*'))
  {
    if (*p == '?')
      if (!*t++)
	return (ABORT);

    p++;
  }
  if (!*p)
    return (TRUE);

  nextp = *p;
  if (nextp == '\\')
    nextp = p[1];

  match = FALSE;
  while (match == FALSE)
  {
    if (nextp == *t || nextp == '[')
      match = regex_match(p, t);

    if (!*t++)
      match = ABORT;
  }

  return (match);
}

static int
regex_match(char *p, char *t)
{
  register char range_start, range_end;
  int invert;
  int member_match;
  int loop;

  for (; *p; p++, t++)
  {
    if (!*t)
      return ((*p == '*' && *++p == '\0') ? TRUE : ABORT);

    switch (*p)
    {
    case '?':
      break;
    case '*':
      return (regex_match_after_star(p, t));
    case '[':
      {
	p++;
	invert = FALSE;
	if (*p == '!' || *p == '^')
	{
	  invert = TRUE;
	  p++;
	}

	if (*p == ']')
	  return (ABORT);

	member_match = FALSE;
	loop = TRUE;
	while (loop)
	{
	  if (*p == ']')
	  {
	    loop = FALSE;
	    continue;
	  }

	  if (*p == '\\')
	    range_start = range_end = *++p;
	  else
	    range_start = range_end = *p;
	  if (!range_start)
	    return (ABORT);

	  if (*++p == '-')
	  {
	    range_end = *++p;
	    if (range_end == '\0' || range_end == ']')
	      return (ABORT);

	    if (range_end == '\\')
	      range_end = *++p;
	    p++;
	  }

	  if (range_start < range_end)
	  {
	    if (*t >= range_start && *t <= range_end)
	    {
	      member_match = TRUE;
	      loop = FALSE;
	    }
	  }
	  else
	  {
	    if (*t >= range_end && *t <= range_start)
	    {
	      member_match = TRUE;
	      loop = FALSE;
	    }
	  }
	}

	if ((invert && member_match) || !(invert || member_match))
	  return (FALSE);

	if (member_match)
	{
	  while (*p != ']')
	  {
	    if (!*p)
	      return (ABORT);

	    if (*p == '\\')
	      p++;
	    p++;
	  }
	}
	break;
      }
    case '\\':
      p++;

    default:
      if (*p != *t)
	return (FALSE);
    }
  }

  return (!*t);
}

static int
match(char *p, char *t)
{
  if ((p == NULL) || (t == NULL))
    return (TRUE);

  return ((regex_match(p, t) == TRUE) ? FALSE : TRUE);
}


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/

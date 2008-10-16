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
 * $XConsortium: list.h,v 1.12 90/09/14 14:54:42 converse Exp $
 *
 * TWM list handling external definitions
 *
 * 11-Apr-88 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef _LIST_
#define _LIST_

#define LTYPE_NAME	(1<<0)	/* match against window name */
#define LTYPE_RES_NAME	(1<<1)	/* match against resource name */
#define LTYPE_RES_CLASS	(1<<2)	/* match against resource class */
#define LTYPE_PROPERTY	(1<<3)	/* match against a window property */
#define LTYPE_HOST	(1<<4)	/* match against a host name */

#define LTYPE_REGEXP	(1<<8)	/* match as a regular expression */
#define LTYPE_C_REGEXP	(1<<9)	/* match as a compiled regexp */
#define LTYPE_STRING	(1<<10)	/* match as an exact string */
#define LTYPE_ANYTHING	(1<<11)	/* match anything */
#define LTYPE_NOTHING	(1<<12)	/* match nothing */

#define LTYPE_ANY_STRING	(LTYPE_NAME | LTYPE_RES_NAME | \
				 LTYPE_RES_CLASS | LTYPE_STRING)
#define LTYPE_ANY_REGEXP	(LTYPE_NAME | LTYPE_RES_NAME | \
				 LTYPE_RES_CLASS | LTYPE_REGEXP)
#define LTYPE_EXACT_NAME	(LTYPE_NAME | LTYPE_STRING)

typedef struct name_list_struct name_list;


#endif /* _LIST_ */


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/

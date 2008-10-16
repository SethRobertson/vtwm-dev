#ifndef _VTWM_IMAGE_FORMATS_H
#define _VTWM_IMAGE_FORMATS_H

#define IMAGE_TYPE_NONE 0
#define IMAGE_TYPE_XPM 1
#define IMAGE_TYPE_PNG 2


typedef struct _Image
{
  int type;
  Pixmap pixmap;
  Pixmap mask;
  char *data;
  int width;
  int height;
  struct _Image *next;
} Image;


#endif


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/

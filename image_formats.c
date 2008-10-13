/***********************************************************************
 *
 * $XConsortium: image_formats.c,v 1.186 91/07/17 13:58:00 dave Exp $
 *
 * png/pnm/bitmat image handling code
 *
 ***********************************************************************/

#ifndef NO_PNG_SUPPORT
#include <libpng12/png.h>
#endif /*NO_PNG_SUPPORT*/

#include "twm.h"
#include "image_formats.h"
#include "util.h"
#include "screen.h"
#include "prototypes.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Drawing.h>
#include <stdio.h>
#include <string.h>


#ifndef NO_XPM_SUPPORT
static void LoadXPMImage (char *path, Image *img, Pixel color);
#endif
#ifndef NO_PNG_SUPPORT
static void Read3ByteRow(png_structp png_ptr, unsigned char *tmpRow, unsigned char *ImgData, int row_num, int width);
static int ReadPNG(char *Path, unsigned char **ImgData, int *width, int *length);
static void LoadPNGImage(char *path, Image *img);
static int LoadImage(char *path, Image *img, Pixel color);
#endif


/***********************************************************************
 *
 *  Procedure:
 *	FindBitmap - read in a bitmap file and return size
 *
 *  Returned Value:
 *	the pixmap associated with the bitmap
 *      widthp	- pointer to width of bitmap
 *      heightp	- pointer to height of bitmap
 *
 *  Inputs:
 *	name	- the filename to read
 *
 ***********************************************************************
 */

Pixmap FindBitmap (char *name, unsigned int *widthp, unsigned int *heightp)
{
    char *bigname;
    Pixmap pm;

    if (!name) return None;

    /*
     * Generate a full pathname if any special prefix characters (such as ~)
     * are used.  If the bigname is different from name, bigname will need to
     * be freed.
     */
    bigname = ExpandFilename (name);
    if (!bigname) return None;

    /*
     * look along bitmapFilePath resource same as toolkit clients
     */
    pm = XmuLocateBitmapFile (ScreenOfDisplay(dpy, Scr->screen), bigname, NULL,
			      0, (int *)widthp, (int *)heightp, &HotX, &HotY);
    if (pm == None && Scr->IconDirectory && bigname[0] != '/') {
	if (bigname != name) free (bigname);
	/*
	 * Attempt to find icon in old IconDirectory (now obsolete)
	 */
	bigname = (char *) malloc (strlen(name) + strlen(Scr->IconDirectory) +
				   2);
	if (!bigname) {
	    fprintf (stderr,
		     "%s:  unable to allocate memory for \"%s/%s\"\n",
		     ProgramName, Scr->IconDirectory, name);
	    return None;
	}
	(void) sprintf (bigname, "%s/%s", Scr->IconDirectory, name);
	if (XReadBitmapFile (dpy, Scr->Root, bigname, widthp, heightp, &pm,
			     &HotX, &HotY) != BitmapSuccess) {
	    pm = None;
	}
    }
    if (bigname != name) free (bigname);

#ifdef DEBUG
    if (pm == None) {
	fprintf (stderr, "%s:  unable to find bitmap \"%s\"\n",
		 ProgramName, name);
    }
#endif

    return pm;
}

Pixmap GetBitmap (char *name)
{
    return FindBitmap (name, &JunkWidth, &JunkHeight);
}


#ifndef NO_XPM_SUPPORT
#include <X11/xpm.h>

static void LoadXPMImage (char *path, Image *img, Pixel color)
{
    XpmAttributes attributes;
	XpmColorSymbol xpmcolor[1];
    int ErrorStatus;

    attributes.valuemask = XpmReturnPixels;

    attributes.valuemask |= XpmCloseness;
    attributes.closeness = 32768;

	if (color)
	{
		xpmcolor[0].name = NULL;
		xpmcolor[0].value = "none";
		xpmcolor[0].pixel = color;

		attributes.colorsymbols = xpmcolor;
		attributes.numsymbols = 1;
		attributes.valuemask |= XpmColorSymbols;
	}

    /*
     * By default, the XPM library assumes screen 0, so we have
     * to pass in the real values. Submitted by Caveh Frank Jalali
     */
    attributes.valuemask |= XpmVisual | XpmColormap | XpmDepth;
    attributes.visual = Scr->d_visual;
    attributes.colormap = XDefaultColormap(dpy, Scr->screen);
    attributes.depth = Scr->d_depth;

    if( (ErrorStatus = XpmReadFileToPixmap(dpy, Scr->Root, path, &img->pixmap,
					   &img->mask, &attributes)) != XpmSuccess){
      img->pixmap = None;
    }
	else
	{
      img->height = attributes.height;
      img->width = attributes.width;
	  img->type= IMAGE_TYPE_XPM;
    }

}
#endif /* NO_XPM_SUPPORT */




/*
 * SetPixmapsBackground - set the background for the Pixmaps resource images
 */
#ifndef NO_XPM_SUPPORT
int SetPixmapsBackground(Image *image, Drawable drawable, Pixel color)
{
	XpmImage xpmimage;
	XpmAttributes xpmattr;
	XpmColorSymbol xpmcolor[1];
	unsigned int i;

	xpmattr.valuemask = XpmCloseness;
	xpmattr.closeness = 32768;

	/*
	 * By default, the XPM library assumes screen 0, so we have
	 * to pass in the real values. Submitted by Caveh Frank Jalali
	 */
	xpmattr.valuemask |= XpmVisual | XpmColormap | XpmDepth;
	xpmattr.visual = Scr->d_visual;
	xpmattr.colormap = XDefaultColormap(dpy, Scr->screen);
	xpmattr.depth = Scr->d_depth;

	if (XpmCreateXpmImageFromPixmap(dpy, image->pixmap, image->mask,
			&xpmimage, &xpmattr) != XpmSuccess)
		{
			fprintf(stderr,"Failed to XpmCreateImage\n");
		return (0);
		}

	for (i = 0; i < xpmimage.ncolors; i++)
		if (!strcmp(xpmimage.colorTable[i].c_color, "None"))
			break;

	if (i < xpmimage.ncolors)
	{
		XFreePixmap(dpy, image->pixmap);
		XFreePixmap(dpy, image->mask);

		xpmcolor[0].name = NULL;
		xpmcolor[0].value = "none";
		xpmcolor[0].pixel = color;

		xpmattr.colorsymbols = xpmcolor;
		xpmattr.numsymbols = 1;
		xpmattr.valuemask |= XpmColorSymbols;

		XpmCreatePixmapFromXpmImage(dpy, drawable, &xpmimage,
				&image->pixmap, &image->mask, &xpmattr);
	}

	i = xpmimage.ncolors;
	XpmFreeXpmImage(&xpmimage);

	return (i);
}
#endif /* NO_XPM_SUPPORT */



#ifndef NO_PNG_SUPPORT

static void Read3ByteRow(png_structp png_ptr, unsigned char *tmpRow, unsigned char *ImgData, int row_num, int width)
{
int pos;
unsigned char *img_ptr, *tmp_ptr;

png_read_row(png_ptr,tmpRow,NULL);
img_ptr=ImgData + (row_num * width *4);
tmp_ptr=tmpRow;
for (pos=0; pos < width; pos++)
{
*img_ptr=*tmp_ptr;
img_ptr++; tmp_ptr++;
*img_ptr=*tmp_ptr;
img_ptr++; tmp_ptr++;
*img_ptr=*tmp_ptr;
img_ptr++; tmp_ptr++;

 *img_ptr=255; /* Alpha channel */
img_ptr++;
}
}

static int ReadPNG(char *Path, unsigned char **ImgData, int *width, int *length)
{
FILE *f;
unsigned char *tmpRow=NULL;
unsigned char *Tempstr=NULL;
png_structp png_ptr;
png_infop info_ptr;
png_uint_32 png_width, png_height, scanline_width;
int bit_depth, color_type, interlace_type, row;
int channels;

f=fopen(Path,"r");

if (! f) return(FALSE);

/* we have to load the first 4 bytes of the png file, this contains png version info */
Tempstr=calloc(1,4);
if ( fread( Tempstr, 1, 4, f ) != 4 )
{
 free(Tempstr);
 fclose(f);
 return(FALSE);
}

/* check if valid png */
   if (png_sig_cmp(Tempstr, (png_size_t)0, 4 ) ) printf("%s not a png?\n",Path);
   /* create the png reading structure, errors go to stderr */
   png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
   if (png_ptr == NULL)
   {
      fclose(f);
       free(Tempstr);
      return(FALSE);
   }


   /* allocate info struct */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(f);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
       free(Tempstr);
      return(FALSE);
   }

   /* set error handling */
   if (setjmp(png_ptr->jmpbuf))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
       free(Tempstr);
      fclose(f);
      return(FALSE);
   }


    /* prepare the reader to ignore all recognized chunks whose data isn't
     * going to be used, i.e., all chunks recognized by libpng except for
     * IHDR, PLTE, IDAT, IEND, tRNS, bKGD, gAMA, and sRGB : */

    {
#ifndef HANDLE_CHUNK_NEVER
/* prior to libpng-1.2.5, this macro was internal, so we define it here. */
# define HANDLE_CHUNK_NEVER 1
#endif
       /* these byte strings were copied from png.h.
	* If a future libpng version recognizes more chunks, add them
	* to this list.  If a future version of readpng2.c recognizes
	* more chunks, delete them from this list. */
       png_byte png_chunk_types_to_ignore[]=
	  { 99,  72,  82,  77, '\0', /* cHRM */
	   104,  73,  83,  84, '\0', /* hIST */
	   105,  67,  67,  80, '\0', /* iCCP */
	   105,  84,  88, 116, '\0', /* iTXt */
	   111,  70,  70, 115, '\0', /* oFFs */
	   112,  67,  65,  76, '\0', /* pCAL */
	   115,  67,  65,  76, '\0', /* sCAL */
	   112,  72,  89, 115, '\0', /* pHYs */
	   115,  66,  73,  84, '\0', /* sBIT */
	   115,  80,  76,  84, '\0', /* sPLT */
	   116,  69,  88, 116, '\0', /* tEXt */
	   116,  73,  77,  69, '\0', /* tIME */
	   122,  84,  88, 116, '\0'}; /* zTXt */
#define NUM_PNG_CHUNK_TYPES_TO_IGNORE 13

    png_set_keep_unknown_chunks(png_ptr, HANDLE_CHUNK_NEVER,
	png_chunk_types_to_ignore, NUM_PNG_CHUNK_TYPES_TO_IGNORE);
    }


    /* set input method */
   png_init_io(png_ptr, f);

   /* tell libpng we have already read some bytes */
   png_set_sig_bytes(png_ptr, 4 );
   /* read all info */
   png_read_info(png_ptr, info_ptr);
   /* get some characteristics of the file */
   png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

	*width=(int) png_width;
	*length=(int) png_height;

	/*expand bit depth */
   png_set_expand(png_ptr);
   png_set_interlace_handling(png_ptr);
   png_read_update_info(png_ptr,info_ptr);

   /* get size of scanline */
   scanline_width = png_get_rowbytes( png_ptr, info_ptr );

channels=png_get_channels(png_ptr,info_ptr);

/* allocate texture memory */
   *ImgData = (unsigned char *) malloc(png_width * png_height *4);
   if (channels ==3) tmpRow = (unsigned char *) malloc(scanline_width * sizeof(unsigned char));

   /* read the image line by line into the user's buffer */
   for (row = 0; row < png_height; row++)
   {
	if (channels==3)
	{
		Read3ByteRow(png_ptr, tmpRow, *ImgData, row, png_width);
	}
	else png_read_row( png_ptr, (unsigned char *)((*ImgData) + (row*scanline_width)), NULL );
   }


   /* finish reading the file */
   png_read_end(png_ptr, NULL);

   png_destroy_read_struct(&png_ptr,&info_ptr,NULL);

fclose(f);

free(Tempstr);
if (tmpRow) free(tmpRow);

return(TRUE);
}



static void LoadPNGImage(char *path, Image *img)
{
XImage *ximg;
unsigned char *PngData=NULL, *ptr;
char *ClipData=NULL, *clip_ptr;
int  x, y, clipcellmask=1;
XColor Color;
Colormap cmap;

if (ReadPNG(path, &PngData, &img->width, &img->height))
{
	Scr->screen=DefaultScreen(dpy);
    cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
	ximg=XCreateImage(dpy,Scr->d_visual,Scr->d_depth,ZPixmap,0,NULL,img->width,img->height,Scr->d_depth,0);
	ximg->data=malloc(ximg->bytes_per_line * ximg->height);

	ClipData=calloc(img->width * img->height,1);

	ptr=PngData;
	clip_ptr=ClipData;
	for (y=0; y < img->height; y++)
	{
		for (x=0; x < img->width; x++)
		{
			Color.red=(*ptr << 8);
			ptr++;
			Color.green=(*ptr << 8);
			ptr++;
			Color.blue=(*ptr << 8);
			ptr++;

			Color.flags=DoRed | DoGreen | DoBlue;
			XAllocColor(dpy,cmap,&Color);
			XPutPixel(ximg,x,y,Color.pixel);

			if (*ptr > 0) *clip_ptr |=  clipcellmask;
			ptr++;
			if (clipcellmask == 128)
			{
				 clip_ptr++;
				 clipcellmask=1;
			}
			else clipcellmask=clipcellmask << 1;

		}
	}

	img->pixmap=XCreatePixmap(dpy,RootWindow(dpy,Scr->screen),img->width,img->height,Scr->d_depth);
	XPutImage(dpy,img->pixmap,Scr->NormalGC,ximg,0,0,0,0,img->width,img->height);

	img->mask=XCreatePixmapFromBitmapData(dpy,RootWindow(dpy,Scr->screen),ClipData,img->width,img->height,1,0,1);
	img->type=IMAGE_TYPE_PNG;

	XDestroyImage(ximg);

}

}
#endif




/***********************************************************************
 *
 *  Procedure:
 *	GetUnknownIcon - read in the bitmap file for the unknown icon
 *
 *  Inputs:
 *	name - the filename to read
 *
 ***********************************************************************
 */

void
GetUnknownIcon(char *name)
{
	Scr->unknownName = name;
}


static int LoadImage(char *path, Image *img, Pixel color)
{
char *extn;

extn=strchr(path,'.');

#ifndef NO_XPM_SUPPORT
if (strcasecmp(extn,".xpm")==0) LoadXPMImage(path, img, color);
#endif /* NO_XPM_SUPPORT */

#ifndef NO_PNG_SUPPORT
if (strcasecmp(extn,".png")==0) LoadPNGImage(path, img);
#endif /* NO_PNG_SUPPORT */

if (img->type !=IMAGE_TYPE_NONE) return(TRUE);
return(FALSE);
}

Image *FindImage (char *name, Pixel color)
{
    char *bigname;
	Image *newimage;

    /*
     * Generate a full pathname if any special prefix characters (such as ~)
     * are used.  If the bigname is different from name, bigname will need to
     * be freed.
     */
    bigname = ExpandFilename (name);
    if (!bigname) return None;

    newimage = (Image *)calloc(1,sizeof(Image));
	LoadImage(bigname,newimage,color);

	/*
	 * Do for pixmaps what XmuLocateBitmapFile() does for bitmaps,
	 * because, apparently, XmuLocatePixmapFile() doesn't!
	 */

	/* This iterates through the bitmap search path */
	if ((newimage->type==IMAGE_TYPE_NONE) && Scr->BitmapFilePath && (bigname[0] != '/'))
	{
		char *path = Scr->BitmapFilePath, *term;

		while (path && (newimage->type==IMAGE_TYPE_NONE))
		{
			if ((term = strchr(path, ':'))) *term = 0;

			if (bigname != name) free(bigname);
			if (!(bigname = (char *)malloc(strlen(name) + strlen(path) + 2)))
				fprintf(stderr, "%s:  unable to allocate memory for \"%s/%s\"\n",
						ProgramName, path, name);
			else
			{
				(void)sprintf(bigname, "%s/%s", path, name);

				LoadImage(bigname,newimage,color);
			}

			if (term)
			{
				*term = ':';
				path = term + 1;
			}
			else
				path = NULL;


		}
	}

    if ((newimage->type==IMAGE_TYPE_NONE) && Scr->IconDirectory && bigname[0] != '/') {
      if (bigname != name) free (bigname);
      /*
       * Attempt to find icon pixmap in old IconDirectory (now obsolete)
       */
      bigname = (char *) malloc (strlen(name) + strlen(Scr->IconDirectory) + 2);
      if (!bigname) {
	fprintf (stderr,
		 "%s:  unable to allocate memory for \"%s/%s\"\n",
		 ProgramName, Scr->IconDirectory, name);
	return None;
      }
      (void) sprintf (bigname, "%s/%s", Scr->IconDirectory, name);


		LoadImage(bigname,newimage,color);
    }

    if(newimage->type != IMAGE_TYPE_NONE){
    }
    else {
      fprintf (stderr, "%s:  unable to find image \"%s\"\n", ProgramName, name);
		free(newimage);
		newimage=NULL;
    }

    if (bigname != name) free (bigname);
    return newimage;
}


/*
 * Copyright 2001, 2002 David J. Hawkey Jr.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder or the author not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. The copyright holder
 * and the author make no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDER AND THE AUTHOR DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDER OR THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * sound.c
 *
 * D. J. Hawkey Jr. - 6/22/01 8/16/01 11/15/02
 */
#include "twm.h"
#include "screen.h"
#include "prototypes.h"

#ifdef NO_SOUND_SUPPORT

/* stub function for gram.y */
int
SetSound(char *function, char *filename, int volume)
{
  return (1);
}

#else /* NO_SOUND_SUPPORT */

#include <X11/Xmu/CharSet.h>
#include <string.h>
#include <stdlib.h>
#include <rplay.h>
#include "gram.h"
#include "parse.h"
#include "twm.h"
#include "sound.h"
#include "prototypes.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

extern int parse_keyword();	/* in parse.c */
extern void twmrc_error_prefix();	/* in gram.y */

typedef struct sound_keyword
{
  char *name;
  int value;
} sound_keyword;

static sound_keyword sound_keywords[] = {
  {"(vtwm start)", S_START},
  {"(vtwm stop)", S_STOP},
  {"(client map)", S_CMAP},
  {"(client unmap)", S_CUNMAP},
  {"(menu map)", S_MMAP},
  {"(menu unmap)", S_MUNMAP},
  {"(info unmap)", S_IUNMAP},
  {"(autopan event)", S_APAN},
  {"(bell event)", S_BELL}
};

#define MAX_SOUNDKEYWORDS (sizeof(sound_keywords) / sizeof(sound_keyword))

typedef struct sound_entry
{
  int func;
  RPLAY *rp;
} sound_entry;

static sound_entry *sound_entries = NULL;
static int sound_count = 0;
static int sound_size = 0;

static int sound_fd = -1;
static int sound_vol = 63;	/* 1/4 attenuation */
static int sound_state = 0;
static char sound_host[MAXHOSTNAMELEN + 1] = "";

/* for qsort() */
static int
compare(void *p, void *q)
{
  sound_entry *pp = (sound_entry *) p, *qq = (sound_entry *) q;

  return (pp->func - qq->func);
}

static int
adjustVolume(int volume)
{
  float vol;

  if (volume > 100)
    volume = 100;

  /* volume for rplay is 1 to 255, not 1 to 100 */
  vol = (float)volume / 100.0;
  volume = vol * 255.0;

  return (volume);
}

int
OpenSound(void)
{
  if (sound_fd < 0)
  {
    if (sound_host[0] == '\0')
    {
      strncpy(sound_host, rplay_default_host(), MAXHOSTNAMELEN);
      sound_host[MAXHOSTNAMELEN] = '\0';
    }

    if ((sound_fd = rplay_open(sound_host)) >= 0)
    {
      qsort((void *)sound_entries, (size_t) sound_count, (size_t) sizeof(sound_entry), compare);

      sound_state = 1;
    }
  }

  return (sound_fd);
}

void
CloseSound(void)
{
  int i;

  for (i = 0; i < sound_count; i++)
    rplay_destroy(sound_entries[i].rp);

  if (sound_entries != NULL)
    free((void *)sound_entries);

  if (sound_fd >= 0)
    rplay_close(sound_fd);

  sound_entries = NULL;
  sound_count = 0;
  sound_size = 0;

  sound_fd = -1;
  sound_vol = 63;
  sound_state = 0;
  sound_host[0] = '\0';
}

int
SetSound(char *function, char *filename, int volume)
{
  sound_entry *sptr;
  int i, func, subfunc;

  func = parse_keyword(function, &subfunc);
  if (func != FKEYWORD && func != FSKEYWORD)
  {
    XmuCopyISOLatin1Lowered(function, function);

    for (i = 0; i < MAX_SOUNDKEYWORDS; i++)
      if (strcmp(function, sound_keywords[i].name) == 0)
      {
	func = FKEYWORD;
	subfunc = sound_keywords[i].value;
	break;
      }
  }

  if (func == FKEYWORD || func == FSKEYWORD)
  {
    if (sound_count >= sound_size)
    {
      sound_size += 10;
      sptr = (sound_entry *) realloc((sound_entry *) sound_entries, sound_size * sizeof(sound_entry));
      if (sptr == NULL)
      {
	twmrc_error_prefix();
	fprintf(stderr, "unable to allocate %d bytes for sound_entries\n", sound_size * sizeof(sound_entry));
	Done();
      }
      else
	sound_entries = sptr;
    }

    sptr = &sound_entries[sound_count];

    sptr->func = subfunc;
    if ((sptr->rp = rplay_create(RPLAY_PLAY)) == NULL)
    {
      twmrc_error_prefix();
      fprintf(stderr, "unable to add to sound list\n");
      Done();
    }

    sound_count++;

    if (volume < 0)
      volume = sound_vol;
    else
      volume = adjustVolume(volume);

    if (rplay_set(sptr->rp, RPLAY_INSERT, 0, RPLAY_SOUND, filename, RPLAY_VOLUME, volume, NULL) < 0)
    {
      twmrc_error_prefix();
      fprintf(stderr, "unable to set \"%s\" in sound list\n", filename);
      Done();
    }

    return (1);
  }

  twmrc_error_prefix();
  fprintf(stderr, "unknown function \"%s\" for sound_entry\n", function);

  return (0);
}

int
PlaySound(int function)
{
  register int i, low, mid, high;

  if (sound_fd < 0 || sound_state == 0)
    return (1);			/* pretend success */

  low = 0;
  high = sound_count - 1;
  while (low <= high)
  {
    mid = (low + high) / 2;
    i = sound_entries[mid].func - function;
    if (i < 0)
      low = mid + 1;
    else if (i > 0)
      high = mid - 1;
    else
    {
      rplay(sound_fd, sound_entries[mid].rp);
      return (1);
    }
  }

  return (0);
}

int
PlaySoundAdhoc(char *filename)
{
  RPLAY *rp;
  int i;

  if (sound_fd < 0 || sound_state == 0)
    return (1);			/* pretend success */

  if ((rp = rplay_create(RPLAY_PLAY)) == NULL)
  {
    twmrc_error_prefix();
    fprintf(stderr, "unable to create sound \"%s\"\n", filename);
    return (0);
  }

  if ((i = rplay_set(rp, RPLAY_INSERT, 0, RPLAY_SOUND, filename, RPLAY_VOLUME, sound_vol, NULL)) >= 0)
    rplay(sound_fd, rp);

  rplay_destroy(rp);

  if (i < 0)
  {
    twmrc_error_prefix();
    fprintf(stderr, "unable to set sound \"%s\"\n", filename);
    return (0);
  }

  return (1);
}

void
SetSoundHost(char *host)
{
  strncpy(sound_host, host, MAXHOSTNAMELEN);
  sound_host[MAXHOSTNAMELEN] = '\0';
}

void
SetSoundVolume(int volume)
{
  if (volume < 0)
    volume = 0;

  sound_vol = adjustVolume(volume);
}

int
ToggleSounds(void)
{
  return ((sound_state ^= 1));
}

#endif /* NO_SOUND_SUPPORT */

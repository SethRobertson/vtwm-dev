
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
 * sound.h
 *
 * D. J. Hawkey Jr. - 6/22/01 11/15/02
 */

#ifndef _SOUND_
#define _SOUND_

/* must not overlap the function defines */
#define S_START		900
#define S_STOP		901
#define S_CMAP		902
#define S_CUNMAP	903
#define S_MMAP		904
#define S_MUNMAP	905
#define S_IUNMAP	906
#define S_APAN		907
#define S_BELL		908

extern int OpenSound(), SetSound(char *function, char *filename, int volume), ToggleSounds();
extern int PlaySound(int function), PlaySoundAdhoc(char *filename);
extern void CloseSound(), SetSoundHost(char *host), SetSoundVolume(int volume);

#endif	/* _SOUND_ */

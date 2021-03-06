/*****************************************************************************
 * Copyright (c) 1997 - 2013 by SR Research Ltd., All Rights Reserved        *
 *                                                                           *
 * This software is provided as is without warranty of any kind.  The entire *
 * risk as to the results and performance of this software is assumed by the *
 * user. SR Research Ltd. disclaims all warranties, either express or implied*
 * ,including but not limited, the implied warranties of merchantability,    *
 * fitness for a particular purpose, title and noninfringement, with respect *
 * to this software.                                                         *
 *                                                                           *
 *                                                                           *
 * For non-commercial use by Eyelink licencees only                          *
 *                                                                           *
 * Windows 95/98/NT/2000/XP sample experiment in C                           *
 * For use with Version 2.0 of EyeLink Windows API                           *
 *****************************************************************************/

#ifndef __SR_RESEARCH__GCWINDOW_H__
#define __SR_RESEARCH__GCWINDOW_H__

#include <sdl_expt.h>

#define WINDOW_SIZE 50 /* Dimensions of the white square */
#define DIFF_THRESH 25 /* Abs diff for x or y to change before trigger */

extern DISPLAYINFO
    dispinfo;               /* display information: size, colors, refresh rate*/
extern SDL_Surface *window; /* SDL surface for drawing */
extern SDL_Color target_background_color; /* SDL color for the background */
extern SDL_Color
    target_foreground_color; /* SDL color for the foreground drawing (text,
                                calibration target, etc)*/

int run_trials(void); /* This code sequences trials within a block. */
void clear_full_screen_window(
    SDL_Color c); /* Clear the window with a specific color */

/* Run gaze-contingent window trial */
int gc_window_trial(SDL_Surface *fgbm,
                    SDL_Surface *bgbm,
                    int wwidth,
                    int wheight,
                    UINT32 time_limit);

#define SETCOLOR(c, x, y, z)                                                   \
    c.r = x;                                                                   \
    c.g = y;                                                                   \
    c.b = z;
#define _stricmp strcasecmp
#define min(a, b) ((a) < (b)) ? a : b
#endif

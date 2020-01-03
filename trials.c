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

#include "gcwindow.h"
#include "sdl_text_support.h"

#ifdef MACOSX
#include <SDL_gfx/SDL_rotozoom.h>
#include <SDL_image/SDL_image.h>
#else
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#endif

SDL_Color target_fg = {255, 255, 255};
SDL_Color target_bg = {0, 0, 0};

/********* PREPARE BITMAPS FOR TRIALS  **********/

SDL_Surface *fgbm = NULL;
SDL_Surface *bgbm = NULL;

/*
    Creates blank surfaces
    If hwsurface == 1 then fgbm is hwsurface
    if hwsurface == 2 then bgbm is hwsurface.
*/
SDL_Surface *blank_bitmap(SDL_Color c, int hwsurface, int width, int height) {
    SDL_Surface *surface = SDL_CreateRGBSurface(
        (hwsurface == 1) ? (SDL_HWSURFACE | SDL_ASYNCBLIT) : SDL_SWSURFACE,
        width,
        height,
        dispinfo.bits,
        0,
        0,
        0,
        0);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, c.r, c.g, c.b));
    return surface;
}

/*  Create foreground and background bitmaps of picture
    EyeLink graphics: blank display with box at center
    type: 0 = blank background, 1= blank fovea (mask), 2 = blurred background
*/
static int create_image_bitmaps() {
    clear_full_screen_window(target_bg);

    // Create white foreground and black background
    fgbm = blank_bitmap(target_fg, 1, window->w, window->h);
    bgbm = blank_bitmap(target_bg, 0, window->w, window->h);

    eyecmd_printf("clear_screen 0");         /* clear EyeLink display */
    eyecmd_printf("draw_box %d %d %d %d 15", /* Box around fixation point */
                  SCRWIDTH / 2 - 16,
                  SCRHEIGHT / 2 - 16,
                  SCRWIDTH / 2 + 16,
                  SCRHEIGHT / 2 + 16);

    if (!fgbm || !bgbm) /* Check that both bitmaps exist */
    {
        eyemsg_printf("ERROR: could not load image");
        alert_printf("ERROR: could not load an image file");
        if (fgbm)
            SDL_FreeSurface(fgbm);
        if (bgbm)
            SDL_FreeSurface(bgbm);
        return SKIP_TRIAL;
    }
    return 0;
}

/*********** TRIAL SELECTOR **********/
/*  FOR EACH TRIAL:
    - set title, TRIALID
    - Create bitmaps and EyeLink display graphics
    - Check for errors in creating bitmaps
    - Run the trial recording loop
    - Delete bitmaps
    - Return any error code

      Given trial number, execute trials
      Returns trial result code
*/
int do_gcwindow_trial() {
    int i;

    set_offline_mode(); /* Must be offline to draw to EyeLink screen */

    eyecmd_printf("record_status_message 'GC IMAGE (MASK)' ");
    eyemsg_printf("TRIALID GCTXTM");
    eyemsg_printf("!V TRIAL_VAR_DATA IMAGE MASK IMAGE");

    if (create_image_bitmaps()) {
        eyemsg_printf("ERROR: could not create bitmap");
        return SKIP_TRIAL;
    }

    bitmap_to_backdrop(
        bgbm,
        0,
        0,
        0,
        0,
        0,
        0,
        (UINT16)(
            BX_MAXCONTRAST |
            ((eyelink_get_tracker_version(NULL) >= 2) ? 0 : BX_GRAYSCALE)));

    /* Small WINDOW_SIZExWINDOW_SIZE px window */
    i = gc_window_trial(fgbm, bgbm, WINDOW_SIZE, WINDOW_SIZE, 60000L);
    SDL_FreeSurface(fgbm);
    fgbm = NULL;
    SDL_FreeSurface(bgbm);
    bgbm = NULL;
    return i;
}

/*********** TRIAL LOOP **************/

/* This code sequences trials within a block. It calls run_trial() to execute a
 * trial, then interperts result code. It places a result message in the EDF
 * file This example allows trials to be repeated from the tracker ABORT menu.
 */
int run_trials(void) {
    int i;
    int trial;
    /* INITIAL CALIBRATION: matches following trials */
    SETCOLOR(
        target_foreground_color, 0, 0, 0); /* color of calibration target */
    SETCOLOR(target_background_color,
             200,
             200,
             200); /* background for drift correction */
    set_calibration_colors(
        &target_foreground_color,
        &target_background_color); /* tell EXPTSPPT the colors */

    if (SCRWIDTH != 800 || SCRHEIGHT != 600)
        alert_printf(
            "Display mode is not 800x600, resizing will slow loading.");

    /* TRIAL_VAR_LABELS message is recorded for EyeLink Data Viewer analysis It
     * specifies the list of trial variables for the trial This should be
     * written once only and put before the recording of individual trials
     */
    eyemsg_printf("TRIAL_VAR_LABELS TYPE CENTRAL PERIPHERAL");

    /* PERFORM CAMERA SETUP, CALIBRATION */
    do_tracker_setup();

    /* Single trial for the latency measurement */
    /* drop out if link closed */
    if (eyelink_is_connected() == 0 || break_pressed()) {
        return ABORT_EXPT;
    }

    /* RUN THE TRIAL */
    i = do_gcwindow_trial(trial);
    end_realtime_mode();

    switch (i) /* REPORT ANY ERRORS */
    {
        case ABORT_EXPT: /* handle experiment abort or disconnect */
            eyemsg_printf("EXPERIMENT ABORTED");
            return ABORT_EXPT;
        case REPEAT_TRIAL: /* trial restart requested */
            eyemsg_printf("TRIAL REPEATED");
            trial--;
            break;
        case SKIP_TRIAL: /* skip trial */
            eyemsg_printf("TRIAL ABORTED");
            break;
        case TRIAL_OK: /* successful trial */
            eyemsg_printf("TRIAL OK");
            break;
        default: /* other error code */
            eyemsg_printf("TRIAL ERROR");
            break;
    }

    return 0;
}

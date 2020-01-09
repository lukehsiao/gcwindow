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

#include "gc.h"  // use #include <gazecursor.h> for EyeLink version
#include "gcwindow.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/**
 * End recording: adds 100 msec of data to catch final events
 */
static void end_trial(void) {
    clear_full_screen_window(target_background_color);  // hide display
    pump_delay(100);  // provide a small amount of delay for last data
    stop_recording();
    while (getkey()) {
    };
}

/**
 * Run a single trial, recording to EDF file and sending data through link This
 * example draws to a bitmap, then copies it to display for fast stimulus onset
 *
 * The order of operations is:
 * - create both foreground and background bitmaps
 * - uses the eyelink_newest_float_sample() to get latest update
 * - initialize window with initialize_gc_window()
 * - moves window with redraw_gc_window()
 *
 * @param fgmb       The bitmap to display within window.
 * @param bgbm       The bitmap to display outside window.
 * @param wwidth     The width of the window in pixels.
 * @param wheight    The height of the window in pixels.
 * @param mask       Flags whether to treat window as a mask.
 * @param time_limit The maximum time the stimuli are displayed.
 */
int gc_window_trial(SDL_Surface *fgbm,
                    SDL_Surface *bgbm,
                    int wwidth,
                    int wheight,
                    UINT32 time_limit,
                    int fd) {
    UINT32 trial_start = 0;  // trial start time (for timeout)
    UINT32 drawing_time;     // retrace-to-draw delay
    int button;              // the button pressed (0 if timeout)
    int error;               // trial result code

    ALLF_DATA evt;          // buffer to hold sample and event data
    int first_display = 1;  // used to determine first drawing of display
    int eye_used = 0;       // indicates which eye's data to display
    float x, y;             // gaze position
    float x_new, y_new;     // new gaze sample
    int valid = 0;          // track whether a valid gaze sample is obtained
    MICRO m1, m2;           // Used to calculate drawing time
    int triggered = 0;      // Flag when the change has been triggered
    unsigned char buf[64];  // Store serial data from Arduino
    int rdlen, wlen;        // Number of bytes read and written
    unsigned char *p;       // pointer for manupulating recieved string

    // Double check everything is connected
    if (!eyelink_is_connected())
        return ABORT_EXPT;

    // make sure display is blank
    SDL_Color black = {0, 0, 0};
    clear_full_screen_window(black);

    // ensure the eye tracker has enough time to switch modes (to start
    // recording).
    set_offline_mode();
    pump_delay(50);

    // Start data recording to EDF file, BEFORE DISPLAYING STIMULUS. You should
    // always start recording 50-100 msec before required otherwise you may
    // lose a few msec of data.
    error = start_recording(1, 1, 1, 1);  // record with link data enabled
    if (error != 0)
        return error;

    // DONT DISPLAY OUR IMAGES TO SUBJECT until we have first gaze postion!
    SDL_BlitSurface(bgbm, NULL, window, NULL);
    initialize_dynamic_cursor(window, min(wwidth, wheight), fgbm);

    // wait for link sample data
    if (!eyelink_wait_for_block_start(100, 1, 0)) {
        end_trial();
        alert_printf("ERROR: No link samples received!");
        return TRIAL_ERROR;
    }

    // determine which eye(s) are available
    eye_used = eyelink_eye_available();

    // reset keys and buttons from tracker
    eyelink_flush_keybuttons(0);

    // First, initialize with a single valid sample.
    while (!valid) {
        if (eyelink_newest_float_sample(NULL) > 0) {
            eyelink_newest_float_sample(&evt);

            x = evt.fs.gx[eye_used];
            y = evt.fs.gy[eye_used];

            // make sure pupil is present
            if (x != MISSING_DATA && y != MISSING_DATA &&
                evt.fs.pa[eye_used] > 0) {
                valid = 1;
            }
        }
    }

    // Send Arduino the command to switch LEDs
    wlen = write(fd, "g\n", 2);
    if (wlen != 2) {
        printf("Error from write: %d, %d\n", wlen, errno);
        end_trial();
        return TRIAL_ERROR;
    }
    if (tcdrain(fd) != 0) {
        printf("Error from tcdrain: %s\n", strerror(errno));
        end_trial();
        return TRIAL_ERROR;
    }

    while (1) {
        // First, check if recording aborted
        if ((error = check_recording()) != 0) {
            return error;
        }
        // Check if trial time limit expired
        if (current_time() > trial_start + time_limit && trial_start != 0) {
            printf("TIMEOUT\n");
            end_trial();
            button = 0;  // trial result message is 0 if timeout
            break;
        }

        // check for program termination or ALT-F4 or CTRL-C keys
        if (break_pressed()) {
            end_trial();
            return ABORT_EXPT;
        }

        // check for local ESC key to abort trial (useful in debugging)
        if (escape_pressed()) {
            end_trial();
            return SKIP_TRIAL;
        }

        // Check for eye-tracker buttons pressed
        // This is the preferred way to get response data or end trials
        button = eyelink_last_button_press(NULL);
        if (button != 0)  // button number, or 0 if none pressed
        {
            printf("ENDBUTTON %d\n", button);
            end_trial();
            break;
        }

        // check for new sample update
        if (eyelink_newest_float_sample(NULL) > 0) {
            eyelink_newest_float_sample(&evt);

            x_new = evt.fs.gx[eye_used];
            y_new = evt.fs.gy[eye_used];

            // make sure pupil is present
            if (x != MISSING_DATA && y != MISSING_DATA &&
                evt.fs.pa[eye_used] > 0) {
                // Show the white square.
                // Only trigger change when there is a large enough diff
                if (abs(x - x_new) >= DIFF_THRESH &&
                    abs(y - y_new) >= DIFF_THRESH && !triggered) {
                    current_micro(&m1);

                    // Draw the window at the bottom left corner
                    draw_gaze_cursor(WINDOW_SIZE / 2, 1080 - (WINDOW_SIZE / 2));
                    current_micro(&m2);

                    // Log an estimate of the drawing delay
                    printf("Drawing delay: %6.3f ms\n",
                           m2.msec + m2.usec / 1000.0 - m1.msec +
                               m1.usec / 1000.0);

                    x = x_new;
                    y = y_new;
                    triggered = 1;

                    // Blocking read call while we wait for the arduino's
                    // end-to-end measurement.
                    rdlen = read(fd, buf, sizeof(buf) - 1);
                    if (rdlen > 0) {
                        buf[rdlen] = 0;
                        printf("Read %d:", rdlen);
                        for (p = buf; rdlen-- > 0; p++) {
                            printf(" 0x%02x", *p);
                            if (*p < ' ')
                                *p = '\0';  // replace any control chars
                        }
                        printf("\n    \"%s\"\n\n", buf);
                    } else if (rdlen < 0) {
                        printf("Error from read: %d: %s\n",
                               rdlen,
                               strerror(errno));
                        end_trial();
                        return TRIAL_ERROR;
                    } else {
                        printf("Nothing read. EOF?\n");
                    }

                    end_trial();
                    break;
                }

                if (first_display) {
                    first_display = 0;
                    SDL_BlitSurface(bgbm, NULL, window, NULL);
                }

            } else {
                // Don't move window during blink
            }
        }
    }

    // dump any accumulated key presses
    while (getkey())
        ;

    // Call this at the end of the trial, to handle special conditions
    return check_record_exit();
}

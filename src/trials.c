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

#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

SDL_Color target_fg = {255, 255, 255};
SDL_Color target_bg = {0, 0, 0};

// Global bitmaps for trials
SDL_Surface *fgbm = NULL;
SDL_Surface *bgbm = NULL;

/**
 * Setup the serial port interface attributes to 8-bit, no parity, 1 stop bit.
 *
 * @param fd    File descriptor of the serial port.
 * @param speed The baud rate to use.
 */
int set_interface_attribs(int fd, int speed) {
    struct termios tty;

    if (!isatty(fd)) {
        printf("fd is not a TTY\n");
        return -1;
    }

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8-bit characters
    tty.c_cflag &= ~PARENB;   // no parity bit
    tty.c_cflag &= ~CSTOPB;   // only need 1 stop bit
    tty.c_cflag &= ~CRTSCTS;  // no hardware flowcontrol

    tty.c_lflag |= ICANON | ISIG;  // canonical input
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN);

    tty.c_iflag &= ~IGNCR;  // preserve carriage return
    tty.c_iflag &= ~INPCK;
    tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // no SW flowcontrol

    tty.c_oflag &= ~OPOST;

    tty.c_cc[VEOL] = 0;
    tty.c_cc[VEOL2] = 0;
    tty.c_cc[VEOF] = 0x04;

    if (cfsetospeed(&tty, speed) < 0 || cfsetispeed(&tty, speed) < 0) {
        printf("unable to set correct baud rates.\n");
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * Creates blank surfaces
 *
 * If hwsurface == 1 then fgbm is hwsurface
 * if hwsurface == 2 then bgbm is hwsurface.
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

/**
 * Create foreground and background bitmaps of picture.
 *
 * EyeLink graphics: blank display with box at center
 * type: 0 = blank background, 1 = blank fovea (mask), 2 = blurred background
 */
static int create_image_bitmaps() {
    clear_full_screen_window(target_bg);

    // Create white foreground and black background
    fgbm = blank_bitmap(target_fg, 1, window->w, window->h);
    bgbm = blank_bitmap(target_bg, 0, window->w, window->h);

    eyecmd_printf("clear_screen 0");          // clear EyeLink display
    eyecmd_printf("draw_box %d %d %d %d 15",  // Box around fixation point
                  SCRWIDTH / 2 - 16,
                  SCRHEIGHT / 2 - 16,
                  SCRWIDTH / 2 + 16,
                  SCRHEIGHT / 2 + 16);

    if (!fgbm || !bgbm)  // Check that both bitmaps exist
    {
        alert_printf("ERROR: could not load an image file");
        if (fgbm)
            SDL_FreeSurface(fgbm);
        if (bgbm)
            SDL_FreeSurface(bgbm);
        return SKIP_TRIAL;
    }
    return 0;
}

/**
 * FOR EACH TRIAL:
 * - Create bitmaps and EyeLink display graphics
 * - Check for errors in creating bitmaps
 * - Run the trial recording loop
 * - Delete bitmaps
 *
 * @return: trial result code
 */
int do_gcwindow_trial(int fd) {
    int i;

    set_offline_mode();  // Must be offline to draw to EyeLink screen

    eyecmd_printf("record_status_message 'GC IMAGE (MASK)' ");

    if (create_image_bitmaps()) {
        printf("ERROR: could not create bitmap\n");
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
    i = gc_window_trial(fgbm, bgbm, WINDOW_SIZE, WINDOW_SIZE, 60000L, fd);
    SDL_FreeSurface(fgbm);
    fgbm = NULL;
    SDL_FreeSurface(bgbm);
    bgbm = NULL;
    return i;
}

/**
 * Trial loop.
 *
 * Run a series of trials, recording the results.
 */
int run_trials(void) {
    int i;
    int trial;
    int fd;  // File descriptor for the serial port
    char *portname = SERIALTERMINAL;

    // Open the serial port to the Arduino
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return ABORT_EXPT;
    }

    // baudrate 115200, 8 bits, no parity, 1 stop bit
    if (set_interface_attribs(fd, BAUD) != 0) {
        printf("Error setting serial interface attribs.\n");
        close(fd);
        return ABORT_EXPT;
    }

    // This is needed so that the handshake has time to complete
    sleep(1);

    // INITIAL CALIBRATION: matches following trials
    // color of calibration target
    SETCOLOR(target_foreground_color, 0, 0, 0);
    // background for drift correction
    SETCOLOR(target_background_color, 200, 200, 200);
    // tell EXPTSPPT the colors
    set_calibration_colors(&target_foreground_color, &target_background_color);

    if (SCRWIDTH != 800 || SCRHEIGHT != 600)
        alert_printf(
            "Display mode is not 800x600, resizing will slow loading.");

    // TRIAL_VAR_LABELS message is recorded for EyeLink Data Viewer analysis.
    // It specifies the list of trial variables for the trial This should be
    // written once only and put before the recording of individual trials

    // Repeat the experiment NUM_TRIALS times to get distribution
    for (trial = 0; trial < NUM_TRIALS; trial++) {
        // drop out if link closed
        if (eyelink_is_connected() == 0 || break_pressed()) {
            close(fd);
            return ABORT_EXPT;
        }

        // RUN THE TRIAL
        i = do_gcwindow_trial(fd);

        // Report errors
        switch (i) {
            case ABORT_EXPT:  // handle experiment abort or disconnect
                printf("EXPERIMENT ABORTED\n");
                close(fd);
                return ABORT_EXPT;
            case REPEAT_TRIAL:  // trial restart requested
                printf("TRIAL REPEATED\n");
                trial--;
                break;
            case SKIP_TRIAL:  // skip trial
                printf("TRIAL ABORTED\n");
                break;
            case TRIAL_OK:  // successful trial
                printf("TRIAL OK\n");
                break;
            default:  // other error code
                printf("TRIAL ERROR\n");
                break;
        }
    }

    // Close serial connection
    close(fd);

    return 0;
}

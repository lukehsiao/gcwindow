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

#include <stdlib.h>
#include <string.h>

DISPLAYINFO dispinfo;  // display information: size, colors, refresh rate

// Name for experiment: goes in task bar, and in EDF file
char program_name[100] = "Eyelink Gaze-Contingent Latency Experiment";
SDL_Surface *window = NULL;

// The colors of the target and background for calibration and drift correct
SDL_Color target_background_color = {192, 192, 192};
SDL_Color target_foreground_color = {0, 0, 0};
char *trackerip = NULL;

int exit_eyelink() {
    close_expt_graphics();       // tell EXPTSPPT to release window
    close_eyelink_connection();  // disconnect from tracker
    return 0;
}

int end_expt(char *our_file_name) {
    // END: close, transfer EDF file
    set_offline_mode();  // set offline mode so we can transfer file
    pump_delay(500);     // delay so tracker is ready
    eyecmd_printf("close_data_file");  // close the data file

    if (break_pressed())
        return exit_eyelink();  // don't get file if we aborted experiment

    // make sure we created a file
    if (our_file_name[0] && eyelink_is_connected()) {
        // tell EXPTSPPT to release window
        close_expt_graphics();
        receive_data_file(our_file_name, "", 0);
    }

    return exit_eyelink();
}

int get_tracker_sw_version(char *verstr) {
    int ln = 0;
    int st = 0;
    ln = strlen(verstr);
    while (ln > 0 && verstr[ln - 1] == ' ')
        verstr[--ln] = 0;  // trim

    // find the start of the version number
    st = ln;
    while (st > 0 && verstr[st - 1] != ' ')
        st--;
    return atoi(&verstr[st]);
}

int app_main(char *trackerip, DISPLAYINFO *disp) {
    UINT16 i, j;
    char our_file_name[260] = "TEST";
    char verstr[50];
    int eyelink_ver = 0;
    int tracker_software_ver = 0;

    if (trackerip)
        set_eyelink_address(trackerip);

    // abort if we can't open the tracker
    if (open_eyelink_connection(0))
        return -1;

    set_offline_mode();

    // initialize getkey() system
    flush_getkey_queue();

    eyelink_ver = eyelink_get_tracker_version(verstr);
    if (eyelink_ver == 3)
        tracker_software_ver = get_tracker_sw_version(verstr);

    // register window with EXPTSPPT
    if (init_expt_graphics(NULL, disp))
        return exit_eyelink();

    window = SDL_GetVideoSurface();

    get_display_information(&dispinfo);

    // NOTE: Camera display does not support 16-color modes
    // NOTE: Picture display examples don't work well with 256-color modes
    //        However, all other sample programs should work well.
    if (dispinfo.palsize == 16) /* 16-color modes not functional */
    {
        alert_printf("This program cannot use 16-color displays");
        return exit_eyelink();
    }

    // 256-color modes: palettes not supported by this example
    if (dispinfo.palsize)
        alert_printf("This program is not optimized for 256-color displays");

    // Size for calibration target and focal spot
    i = SCRWIDTH / 60;
    j = SCRWIDTH / 300;
    if (j < 2)
        j = 2;
    set_target_size(i, j);  // tell DLL the size of target features

    // tell EXPTSPPT the colors
    set_calibration_colors(&target_foreground_color, &target_background_color);

    set_cal_sounds("", "", "");
    set_dcorr_sounds("", "off", "off");

    clear_full_screen_window(target_background_color);

    SDL_Flip(window);

    // If file name set, open it
    if (our_file_name[0]) {
        // add extension
        if (!strstr(our_file_name, "."))
            strcat(our_file_name, ".EDF");
        i = open_data_file(our_file_name);
        if (i != 0) {
            alert_printf("Cannot create EDF file '%s'", our_file_name);
            return exit_eyelink();
        }
        eyecmd_printf("add_file_preamble_text 'RECORDED BY %s' ", program_name);
    }

    // Now configure tracker for display resolution
    eyecmd_printf("screen_pixel_coords = %ld %ld %ld %ld",
                  dispinfo.left,
                  dispinfo.top,
                  dispinfo.right,
                  dispinfo.bottom);

    // Setup calibration type
    eyecmd_printf("calibration_type = HV9");

    if (dispinfo.refresh > 40) {
        printf("FRAMERATE %1.2f Hz.\n", dispinfo.refresh);
    }

    // SET UP TRACKER CONFIGURATION
    // set parser saccade thresholds (conservative settings)
    if (eyelink_ver >= 2) {
        eyecmd_printf(
            "select_parser_configuration 0");  // 0 = standard sensitivity
        if (eyelink_ver == 2)  // turn off scenelink camera stuff
        {
            eyecmd_printf("scene_camera_gazemap = NO");
        }
    } else {
        eyecmd_printf("saccade_velocity_threshold = 35");
        eyecmd_printf("saccade_acceleration_threshold = 9500");
    }

    // NOTE: set contents before sending messages!
    // set EDF file contents
    eyecmd_printf("file_event_filter = "
                  "LEFT,RIGHT,FIXATION,SACCADE,BLINK,MESSAGE,BUTTON,INPUT");
    eyecmd_printf(
        "file_sample_data = LEFT,RIGHT,GAZE,AREA,GAZERES,STATUS%s,INPUT",
        (tracker_software_ver >= 4) ? ",HTARGET" : "");

    // set link data (used for gaze cursor)
    eyecmd_printf(
        "link_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,BUTTON,INPUT");
    eyecmd_printf(
        "link_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS%s,INPUT",
        (tracker_software_ver >= 4) ? ",HTARGET" : "");

    // make sure we're still alive
    if (!eyelink_is_connected() || break_pressed())
        return end_expt(our_file_name);

    i = run_trials();
    return end_expt(our_file_name);
}

void clear_full_screen_window(SDL_Color c) {
    SDL_FillRect(window, NULL, SDL_MapRGB(window->format, c.r, c.g, c.b));
    SDL_Flip(window);
    SDL_FillRect(window, NULL, SDL_MapRGB(window->format, c.r, c.g, c.b));
}

int parseArgs(int argc, char **argv, char **trackerip, DISPLAYINFO *disp) {
    int i = 0;
    memset(disp, 0, sizeof(DISPLAYINFO));
    for (i = 1; i < argc; i++) {
        if (_stricmp(argv[i], "-tracker") == 0 && argv[i + 1]) {
            *trackerip = argv[i + 1];
            i++;
        } else {
            printf("%d \n", i);
            printf("usage %s \n", argv[0]);
            printf("\t options: \n");
            printf("\t[-tracker <tracker address > ] eg. 100.1.1.1 \n");
            return 1;
        }
    }

    // Values of the Zisworks Monitor we are targetting
    disp->width = 1920;
    disp->height = 1080;
    disp->bits = 24;
    disp->refresh = 240.0;

    return 0;
}

int main(int argc, char **argv) {
    DISPLAYINFO disp;
    char *trackerip = NULL;
    int rv = parseArgs(argc, argv, &trackerip, &disp);
    if (rv)
        return rv;

    app_main(trackerip, &disp);
    return 0;
}

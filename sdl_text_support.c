/*******************************************************************************
 * EYELINK EXPT SUPPORT: SDL PLATFORM (Coppied from WIN32 PLATFORM)			   *
 * (c) 1996-2013 by SR Research Ltd.                                           *
 * by Suganthan Subramaniam                                                    *
 * EYELINK II: 3 February 2003                                                 *
 *     For non-commercial use only                                             *
 * Provides text support for sdl							                   *
 * This module is for user applications. Use is granted for non-commercial     *
 * applications by Eyelink licencees only                                      *
 *                                                                             *
 *                                                                             *
 **************************************** WARNING ******************************
 *                                                                             *
 * UNDER NO CIRCUMSTANCES SHOULD PARTS OF THESE FILES BE COPIED OR COMBINED.   *
 * This will make your code impossible to upgrade to new releases in the future*
 * and SR Research will not give tech support for reorganized code.            *
 *                                                                             *
 * This file should not be modified. If you must modify it, copy the entire    *
 * file with a new name, and change the the new file.                          *
 *******************************************************************************/

#include <stdio.h>  // for vsprintf() function
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#ifdef __APPLE__  // MACOSX
#include <SDL_ttf/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#include "fontmappings.h"
#include "sdl_text_support.h"
#ifndef WIN32
#define min(x, y) (x > y) ? y : x
#define _stricmp strcasecmp
#endif

static TTF_Font *font = NULL;
#define FONTNAME "Arial.ttf"
#define FONTSIZE 30
#define MAX_CHAR_PERLINE 1024
static int marginx = 0;
static int marginy = 0;
static int marginw = 0;
static int marginh = 0;
static double linespacing = (9.0 / 10.0);

static FONT_MAP fontmap[] = FONTMAPPINGS;

void set_line_spacing(double linespace) { linespacing = linespace; }
void set_margin(int left, int right, int top, int bottom) {
    marginx = left;
    marginy = top;
    marginw = right;
    marginh = bottom;
}

void adjustPlacement(SDL_Rect *rect, int scrwidth, int format);

char *get_font_file_name(char *fontname) {
    int i = 0;
    for (; fontmap[i].filename && fontmap[i].font_face && fontmap[i].font_name;
         i++) {
        if (_stricmp(fontmap[i].font_name, fontname) == 0)
            return fontmap[i].filename;
    }
    return FONTNAME;  // default font
}
#if defined(__APPLE__)  //(MACOSX) && defined(MACAPP)
#include <CoreFoundation/CoreFoundation.h>
#endif
char *get_resources_path() {
#if defined(WIN32)
    return getenv("windir");
#elif defined(__APPLE__)  // defined(MACOSX) && defined(MACAPP)
    static char wd[1024] = {0};
    if (!strlen(wd)) {
        CFURLRef appRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        CFStringRef appPath =
            CFURLCopyFileSystemPath(appRef, kCFURLPOSIXPathStyle);
        const char *pathPtr =
            CFStringGetCStringPtr(appPath, CFStringGetSystemEncoding());
        strcpy(wd, pathPtr);
        strcat(wd, "/Contents/Resources");
        return wd;
    }
    return wd;

#else
    return NULL;
#endif
}
int get_new_font(char *fontname, int font_size, int bold) {
    char fontpath[512] = {0};
    fontname = get_font_file_name(fontname);
    // printf("fontname is %s \n", fontname);

    if (get_resources_path()) {
        strcpy(fontpath, get_resources_path());
#ifdef _WIN32 || _WIN64
        strcat(fontpath, "\\");
        strcat(fontpath, "Fonts\\");
#else
        strcat(fontpath, "/");
#endif
        // printf("after get resources path, footpath is %s\n", fontpath);
    }
    strcat(fontpath, fontname);

    // printf("connect footpath and name is %s %s\n", fontpath, fontname);

    if (font) /* close the font first */
    {
        TTF_CloseFont(font);
        font = NULL;
    }
    font = TTF_OpenFont(fontpath, font_size);
    if (!font) {
        return -1;
    }

    if (bold)
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);
    return 0;
}

int get_font_height() {
    if (font)
        return TTF_FontHeight(font);
    return -1;
}

void adjustPlacement(SDL_Rect *rect, int scrwidth, int format) {
    if (format & JUSTIFY_RIGHT)
        rect->x = scrwidth - rect->w;
    if (format & CENTER)
        rect->x = (scrwidth - rect->w) / 2;
}

void sdl_printf(SDL_Surface *surface,
                char *text,
                int format,
                int x,
                int y,
                SDL_Color foreground) {
    char temp[MAX_CHAR_PERLINE];
    int width = 0;
    int height = 0;
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    memset(temp, 0, MAX_CHAR_PERLINE);

    if (!font)
        get_new_font(FONTNAME, FONTSIZE, 0);
    if (!font) {
        printf("Font cannot be loaded \n");
        return;
    }
    TTF_SizeText(font, text, &width, &height);

    if (y + height + marginh > surface->h)
        return;  // skip out of margin.

    if (x + width > surface->w - marginw) {
        int stop = 0;
        int stop1 = 0;
        int stop2 = 0;
        strncpy(temp, text, MAX_CHAR_PERLINE - 1);
        {  // look for \n or \r
            int i = 0;
            for (i = 0; i < MAX_CHAR_PERLINE - 1; i++) {
                if (temp[i] == '\n' || temp[i] == '\r') {
                    stop1 = i;
                    break;
                }
            }
        }
        if (!stop) {  // no line breaks found. break at a white space
            int i = 0;
            for (i = MAX_CHAR_PERLINE - 2; i >= 0; i--) {
                int w = 0, h = 0;
                char last = temp[i + 1];
                temp[i + 1] = 0;
                TTF_SizeText(font, temp, &w, &h);
                if ((temp[i] == ' ' || temp[i] == '\t') &&
                    w + x < surface->w - marginw) {
                    stop2 = i;
                    break;
                }
                temp[i + 1] = last;
            }
        }

        if (stop1 && stop2)
            stop = min(stop1, stop2);
        else if (stop1)
            stop = stop1;
        else if (stop2)
            stop = stop2;
        if (stop) {
            SDL_Surface *textsurface = NULL;
            temp[stop] = 0;
            textsurface = TTF_RenderText_Blended(font, temp, foreground);
            if (!textsurface) {
                printf("Could not create text surface \n");
                return;
            }
            rect.w = textsurface->w;
            rect.h = textsurface->h;
            adjustPlacement(&rect, surface->w, format);
            SDL_BlitSurface(textsurface, NULL, surface, &rect);
            if (format & WRAP && strlen(temp) && strlen(temp) < strlen(text)) {
                int i = stop + 1;
                if (stop != stop1)  // line break dont skip the spaces
                {
                    for (; i < MAX_CHAR_PERLINE - 1; i++) {
                        if (temp[i] != ' ')
                            break;
                    }
                }
                sdl_printf(surface,
                           text + i,
                           format,
                           x,
                           y + (int)((double)height * (linespacing)),
                           foreground);
            }
            SDL_FreeSurface(textsurface);
            return;
        }
    } else {
        SDL_Surface *textsurface = NULL;
        if (strlen(text) > MAX_CHAR_PERLINE)
            text[MAX_CHAR_PERLINE - 1] = 0;  // chop the line.
        strcpy(temp, text);
        textsurface = TTF_RenderText_Blended(font, temp, foreground);
        if (!textsurface) {
            printf("Could not create text surface \n");
            return;
        }
        rect.w = textsurface->w;
        rect.h = textsurface->h;
        adjustPlacement(&rect, surface->w, format);
        SDL_BlitSurface(textsurface, NULL, surface, &rect);
        SDL_FreeSurface(textsurface);
    }
}

/*******************************************************************************
 * sdl_demo_printf
 * Purpose: printf like text printing routine to a surface.
 * Input  : surface
 *			x  - x position on the resource
 *			y  - y position on the resource.
 *          fsize - font size
 *          ftype - font type
 *			text - format string
 *			...  - arguments
 * Output :
 *******************************************************************************/
void graphic_printf(SDL_Surface *surface,
                    SDL_Color fg,
                    int format,
                    int x,
                    int y,
                    const char *fmt,
                    ...) {
    va_list argptr;
    char *msgbuf = malloc(sizeof(char) * 1024 * 4);

    if (format & WRAP) {
        x += marginx;
        y += marginy;
    }
    va_start(argptr, fmt);  // Create the text string, similar to printf()
    vsprintf(msgbuf, fmt, argptr);
    va_end(argptr);
    sdl_printf(surface, msgbuf, format, x, y, fg);
    free(msgbuf);
}

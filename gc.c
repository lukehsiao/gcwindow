/*******************************************************************************
 * EYELINK EXPT SUPPORT: SDL PLATFORM										   *
 * (c) 1996-2013 by SR Research Ltd.                                           *
 * by Suganthan Subramaniam                                                    *
 * EYELINK II: 3 February 2003                                                 *
 *     For non-commercial use only                                             *
 * Provides hardware acclareted cursor drawing with SDL_Flip                   *
 * This module is for user applications. Use is granted for non-commercial     *
 * applications by Eyelink licencees only                                      *
 *                                                                             *
 *
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

#include <SDL/SDL.h>
#ifndef __APPLE__ //MACOSX
#define FLAGS SDL_HWSURFACE|SDL_HWACCEL|SDL_ASYNCBLIT
#include <SDL_gfxPrimitives.h>
#else
#include <SDL_gfx/SDL_gfxPrimitives.h>
#define FLAGS SDL_SWSURFACE
#define transparent trans // conflict with quicktime
#endif
#define UNDEFINED -32768
#define ISPAGEFLIP(x) (x->flags &(SDL_HWSURFACE|SDL_DOUBLEBUF))
#if defined (WIN32) && !defined(_WIN64)
#define Flip(x) while(SDL_FlipEx(x)<0)
#else
#define Flip(x) while(SDL_Flip(x)<0)
#endif


SDL_Surface *main_window = NULL; /* main window */
SDL_Surface *cursor = NULL; /* motion cursor */
SDL_Surface *src = NULL;    /* for dynamic cursors */
SDL_Rect last = {UNDEFINED,UNDEFINED,UNDEFINED,UNDEFINED};   /* last cursor position */
SDL_Rect current = {UNDEFINED,UNDEFINED,UNDEFINED,UNDEFINED};
SDL_Rect cursorrect = {0,0,130,130}; /* cursor size */
SDL_Surface *main_clone = NULL;
SDL_Color transparent = { 255,255,255,255};


extern  void free_cursor();

int initcursor(SDL_Surface * source, int w, int h)
{
	free_cursor();
	main_clone = SDL_CreateRGBSurface(FLAGS,main_window->w,main_window->h, 32,0,0,0,0);
	SDL_BlitSurface(main_window, NULL, main_clone, NULL);
	src = source;
	if(!source)
	{
		Sint16 radius = (w/2)-1;
		Sint16 gap = (w/4)-1;

		cursor = SDL_CreateRGBSurface(FLAGS,w,h, 32,0,0,0,0);
		if(cursor && !(cursor->flags & SDL_HWSURFACE))
			printf("Hardware surface could not be created. Blitting may be slow \n");
		
		SDL_SetColorKey(cursor, SDL_SRCCOLORKEY|SDL_RLEACCEL, SDL_MapRGB(cursor->format,
			transparent.r,transparent.g, transparent.b));
		SDL_FillRect(cursor,NULL,SDL_MapRGB(cursor->format,
			transparent.r,transparent.g, transparent.b));
  		filledEllipseRGBA(cursor, radius, radius,radius, radius, 255,0,0,255);
		filledEllipseRGBA(cursor, radius, radius, (radius-gap), (radius-gap), 
			transparent.r,transparent.g, transparent.b, 255);


	}
	cursorrect.w = w;
	cursorrect.h = h;

	if(source && !(source->flags & SDL_HWSURFACE))
		printf("Given source is not a hardware surface. Blitting may be slow \n");
	return 0;

}
void free_cursor()
{
	if(cursor)
		SDL_FreeSurface(cursor);
	if(main_clone)
		SDL_FreeSurface(main_clone);
	
	cursor = NULL; /* motion cursor */
	src = NULL;    /* for dynamic cursors */
	main_clone = NULL;

	last.x = last.y = last.w = last.h =UNDEFINED;   /* last cursor position */
	current.x = current.y = current.w = current.h = UNDEFINED;
	cursorrect.x = cursorrect.y  = 0;
	cursorrect.w = cursorrect.h = 130;
	


}

int draw_gaze_cursor(int x, int y)
{
	SDL_Rect rct = {x-cursorrect.w/2, y-cursorrect.h/2, cursorrect.w, cursorrect.h};
	if(!(last.x == UNDEFINED && last.x == last.y && last.w == last.h && last.x == last.w )) // erase cursor
		SDL_BlitSurface(main_clone,&last,main_window,&last);	

	if(cursor)
		SDL_BlitSurface(cursor,NULL,main_window,&rct);
	else if(src)
		SDL_BlitSurface(src,&rct,main_window,&rct);
	Flip(main_window);
	if(ISPAGEFLIP(main_window))
	{
		last = current;
		current = rct;
	}
	else
		last = rct;
	return 0;
}

int erase_gaze_cursor()
{
	if(!(last.x == UNDEFINED && last.x == last.y && last.w == last.h && last.x == last.w )) // erase cursor
	{
		SDL_BlitSurface(main_clone,&last,main_window,&last);	
		Flip(main_window);
		if(ISPAGEFLIP(main_window))
			SDL_BlitSurface(main_clone, &current,main_window,&current);	

	}
	last.x = last.y = last.w = last.h = current.x = current.y = current.w = current.h = UNDEFINED;
	return 0;
}


/*******************************************************************************
 * Initialize the cursor sub system
 *******************************************************************************/
void initialize_cursor(SDL_Surface * surface, int size)
{
	main_window = surface;
	initcursor(NULL,size, size);
}
void initialize_dynamic_cursor(SDL_Surface * surface, int size, SDL_Surface *copySrc )
{
	main_window = surface;
	initcursor(copySrc,size,size);
}




#ifdef TEST_APP
int main (int argc, char ** argv)
{
	SDL_Surface *img = NULL;
	SDL_Surface  *source;
	SDL_Color fg = {50,80,20};	
	SDL_Color bg = {255,255,255};
	int i =0;
	int x =0, y =0;
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
	main_window = SDL_SetVideoMode( 1280,1024,32,FLAGS|SDL_FULLSCREEN|SDL_DOUBLEBUF);
	source = SDL_CreateRGBSurface(FLAGS,main_window->w,main_window->h, 32,0,0,0,0);
	SDL_FillRect(source,NULL,SDL_MapRGB(source->format, fg.r, fg.g, fg.b));

	img = SDL_LoadBMP("t1.bmp");
	
	

	SDL_FillRect(main_window,NULL,SDL_MapRGB(main_window->format, bg.r, bg.g, bg.b));
	SDL_BlitSurface(img,NULL,main_window,NULL);
	SDL_Flip(main_window);
	SDL_FillRect(main_window,NULL,SDL_MapRGB(main_window->format, bg.r, bg.g, bg.b));
	SDL_BlitSurface(img,NULL,main_window,NULL);
	SDL_FreeSurface(img);
	initcursor(NULL, 150,150);
	SDL_Delay(500);
	for(i =0; i < 100; i ++)
	{
		Uint32 start = SDL_GetTicks();
		draw_gaze_cursor(x++, y ++);
		printf("cursor drawing time %d \n", SDL_GetTicks() - start);
		
	}

	erase_gaze_cursor();
	SDL_Delay(1500);

	x = 400; y = 500;
	for(i =0; i < 100; i ++)
	{
		Uint32 start = SDL_GetTicks();
		draw_gaze_cursor(x++, y ++);
		printf("cursor drawing time %d \n", SDL_GetTicks() - start);
		
	}
	SDL_Quit();
	return 0;
}
#endif

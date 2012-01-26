#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
                fprintf (stderr, "SDL init error: %\n", SDL_GetError());
                return EXIT_FAILURE;
        }
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(SDL_DISABLE);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_Quit();
        return 0;
}

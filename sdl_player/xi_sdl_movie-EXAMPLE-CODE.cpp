/* Here is the Main.cpp file that demonstrates using the class, it's very straight forward as well. */

// Header file for SDL
#include "SDL.h"

// This is the header file for the library
#include "smpeg.h"

// Link in the needed libraries
#pragma comment( lib, "sdlmain.lib")
#pragma comment( lib, "sdl.lib")
#pragma comment( lib, "smpeg.lib")

#include "SDL_Movie.h"

int main( int argc, char *argv[] )
{
  if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 )
	{
      printf("Unable to init SDL: %s\n", SDL_GetError());
      return -1;
	}
  atexit(SDL_Quit);

  SDL_Surface* screen = SDL_SetVideoMode( 640, 480, 32, SDL_HWSURFACE );
  if ( screen == NULL )
	{
      printf("Unable to set 640x480 video: %s\n", SDL_GetError());
      return -1;
	}

  // The movie variable
  SDL_Movie mov1;
	
  // Load the specified file and draw it to the 'screen' surface
  // In this case, we are allowed to scale the X and Y dimensions by 2 maximually
  mov1.Load( "demo.mpg", screen, 2, 2 );

  // Set the display at
  //mov1.SetPosition( 100, 100 );

  // Scale the movie by 2
  mov1.ScaleBy( 2 );

  // Event variable
  SDL_Event event;

  // Loop flag
  int done = 0;

  // Key structure
  Uint8* keys;

  while(done == 0)
	{
      // Simple event loop
      while ( SDL_PollEvent(&event) )
		{
          if ( event.type == SDL_QUIT )  {  done = 1;  }

          if ( event.type == SDL_KEYDOWN )
			{
              if ( event.key.keysym.sym == SDLK_ESCAPE ) { done = 1; }
			}
		}

      // Update key states
      keys = SDL_GetKeyState( 0 );

      // Play
      if( keys[SDLK_p] )
		{
          mov1.Play();
          keys[SDLK_p] = 0;
		}

      // Pause
      if( keys[SDLK_a] )
		{
          mov1.Pause();
          keys[SDLK_a] = 0;
		}

      // Stop
      if( keys[SDLK_s] )
		{
          mov1.Stop();

          // If we stop, we want to start from the beginning again
          mov1.Rewind();

          // Clear the movie surface so it's no longer showing the last frame
          mov1.ClearScreen();

          keys[SDLK_s] = 0;
        }

      // Rewind
      if( keys[SDLK_r] )
        {
          mov1.Rewind();
          keys[SDLK_r] = 0;
        }

      // NOTE: DOES NOT WORK
      if( keys[SDLK_LEFT] )
        {
          // Still trying to figure this one out, how to go backwards
          mov1.Skip( -.5 );
          keys[SDLK_LEFT] = 0;
        }

      // Skip forward .5 seconds in the movie
      if( keys[SDLK_RIGHT] )
        {
          mov1.Skip( .5 );
          keys[SDLK_RIGHT] = 0;
        }

      // Clear the main screen (might not want to have in a game)
      SDL_FillRect( screen, 0, 0 );

      // Display the movie at the internal location (setposition)
      if( mov1.GetStatus() == SMPEG_PLAYING )
        mov1.Display();

      // Display the movie at a specified location
      //mov1.DisplayAt( 0, 0 );

      // Flip the main screen
      SDL_Flip( screen );
    }

  return 0;
}


/*
So basically, if you look though the key inputs, there is play, pause, stop, rewind, seek forward, and no seek backward yet because it doesn't seem to work. Bare minimal to work with this class is as follows:


// Create a movie variable
SDL_Movie mov1;
// Load a movie file, set the surface it should be rendered to
mov1.Load( "demo.mpg", screen );
// Start playback (only needs to be called once, NOT once per frame)
mov1.Play();
// Finally draw the movie (called every frame)
mov1.Display();


And that's a wrap. I will probabally improive upon this a lot more and put it into a second tutorial when I next get time. This is just to get something out there for any interested, feel free to make your modifications as needed [wink]. So if there's any other requests with that project, just let me know!


Peregrin, that's a great find! The main problem I see with using SMPEG now is that it is under GLP, so you do have to release all your soruce code, which for something like E4E, it's not a bad thing, but if you wanted to make a game, then you'd not want to. If you don't beat me to it, I'll also take a look into using that program with SDL as well, for a LPGL alternative with more modern codecs would be great! [smile] 
*/

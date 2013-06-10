# Build xieps_sdl_player as executable
bin_PROGRAMS = xieps_sdl_player
# ../src/xi_data.h ../src/xi_data.c ../src/xi_error.c ../src/xi_error.h ../src/xi_enums.h ../src/xieps.h ../src/xieps.c
xieps_sdl_player_SOURCES = xi_sdl.c xi_sdl.h xi_sdl_player.c xi_sdl_player.h
xieps_sdl_player_CFLAGS = $(GLIB_CFLAGS) $(SDL_CFLAGS) $(SDL_GFX_CFLAGS) $(SDL_MIXER_CFLAGS) $(SDL_IMAGE_CFLAGS)
xieps_sdl_player_LDADD = ../src/libxieps.la ../demo/libxiepsdemo.la $(GLIB_LIBS) $(SDL_LIBS) $(SDL_GFX_LIBS) $(SDL_MIXER_LIBS) $(SDL_IMAGE_LIBS)
# TODO: xieps_demo lib is a hard dependency, for now. Needs to be dynamically loaded later at some point.

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Docs:
# Autoconf:
#   https://www.gnu.org/software/autoconf/
# Automake:
#   https://www.gnu.org/software/automake/
# Libtool:
#   https://www.gnu.org/software/libtool/
#   https://www.gnu.org/software/automake/manual/libtool.html#Top
# GNU M4:
#   https://www.gnu.org/software/m4/


# Standard `configure.ac' Layout
# https://www.gnu.org/software/autoconf/manual/autoconf.html#Autoconf-Input-Layout
#      
# --------------------------------------------------------------
# Autoconf requirements
# AC_INIT(package, version, bug-report-address)
# information on the package
# checks for programs
# checks for libraries
# checks for header files
# checks for types
# checks for structures
# checks for compiler characteristics
# checks for library functions
# checks for system services
# AC_CONFIG_FILES([file...])
# AC_OUTPUT
# --------------------------------------------------------------


AC_PREREQ([2.68])
AC_INIT([xieps], 0.0.0, [the letter b dot the letter c AT hush com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR(build-aux)

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC_C99 # Use the C99 standard. See also AC_PROG_CC_STDC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL


# Libtool
# https://www.gnu.org/software/automake/manual/libtool.html#Top
LT_INIT

# fairly severe build strictness
# change foreign to gnu or gnits to comply with gnu standards
AM_INIT_AUTOMAKE

# Checks for libraries.
# PKG_CHECK_MODULES considered harmful?
# See: http://stackoverflow.com/questions/10220946/pkg-check-modules-considered-harmful#10229811
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32.3])
PKG_CHECK_MODULES([SDL], [sdl >= 1.2.12])
PKG_CHECK_MODULES([SDL_GFX], [SDL_gfx >= 1.2.12])
PKG_CHECK_MODULES([SDL_IMAGE], [SDL_image >= 1.2.12])
PKG_CHECK_MODULES([SDL_MIXER], [SDL_mixer >= 1.2.12])

# Gettext
# https://www.gnu.org/software/automake/manual/automake.html#gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h unistd.h libintl.h])
# TODO: Don't know what I even need AC_CHECK_HEADER(S) for unless I'm
# doing something alternative if a header doesn't exist.
# AC_CHECK_HEADER([glib.h]) # not found AC_CHECK_HEADER([SDL/SDL.h])
# AC_CHECK_HEADER([SDL/SDL_rotozoom.h])
# AC_CHECK_HEADER([SDL/SDL_gfxPrimitives.h])
# AC_CHECK_HEADER([SDL/SDL_framerate.h])

# Checks for library functions.
AC_FUNC_MALLOC

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([atexit])

AC_CONFIG_FILES([Makefile
                 demo/Makefile
                 src/Makefile
                 tests/Makefile
                 sdl_player/Makefile])
                 # po/Makefile.in -- removed because was throwing error
AC_OUTPUT

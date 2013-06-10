#ifndef __XIEPS_H__
#define __XIEPS_H__

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)

#include "xi_enums.h"
#include "xi_error.h"
#include "xi_data.h"


/* void         xi_log_to_file   (gchar const *log_domain, GLogLevelFlags log_level, gchar const *message, gpointer user_data); */
/* void         xi_setup_logging (); */
/* GHashTable*  xi_read_settings (GError **err); */

#endif

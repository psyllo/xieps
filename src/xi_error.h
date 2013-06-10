#ifndef __XI_ERROR_H__
#define __XI_ERROR_H__

#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)


GQuark    xi_error_quark();


#endif

#include "xi_error.h"

GQuark
xi_error_quark()
{
  g_quark_from_string("xi-error-quark");
}

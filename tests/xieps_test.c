#include <glib.h>
#include "../src/xieps.h"

void
test_xi_get_value()
{
  GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
  gchar *foo_key = "foo";
  gchar *bad_key = "XXXXX";
  gchar *foo_val = "FOO";
  gchar *default_value = "NOT_FOUND";

  /* Setup Hash table with value(s) */
  g_hash_table_insert(table, foo_key, foo_val);

  /* Test all permutations of the parameters */
  {
    gboolean found_p = xi_get_value(NULL, NULL, NULL, NULL, NULL);
    g_assert(!found_p);
  }
  {
    /* Search should fail unless there is a variable to return result to */
    gboolean found_p = xi_get_value(table, foo_key, NULL, NULL, NULL);
    g_assert(!found_p);
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    found_p = xi_get_value(NULL, NULL, NULL, NULL, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
    g_message("domain(quark): %d, code: %d, msg: %s", err->domain, err->code, err->message);
    /* TODO: Use regex to validate err->message */
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    found_p = xi_get_value(table, NULL, NULL, NULL, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
    g_message("domain(quark): %d, code: %d, msg: %s", err->domain, err->code, err->message);
    /* TODO: Use regex to validate err->message */
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    found_p = xi_get_value(table, foo_key, NULL, NULL, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
    g_message("domain(quark): %d, code: %d, msg: %s", err->domain, err->code, err->message);
    /* TODO: Use regex to validate err->message */
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    gpointer value = NULL;
    found_p = xi_get_value(table, foo_key, &value, NULL, &err);
    g_assert(found_p);
    g_assert(err == NULL);
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    gpointer value = NULL;
    found_p = xi_get_value(table, foo_key, &value, (gpointer *)&default_value, &err);
    g_assert(found_p);
    g_assert(err == NULL);
    g_assert(g_strcmp0(value, foo_val) == 0);
  }
  {
    GError *err = NULL;
    gboolean found_p = FALSE;
    gpointer value = NULL;
    found_p = xi_get_value(table, bad_key, &value, (gpointer *)&default_value, &err);
    g_assert(!found_p);
    g_assert(err == NULL);
    g_assert(g_strcmp0(value, default_value) == 0);
  }
}

void
test_xi_get_int()
{
  GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
  gchar *foo_key = "foo";
  gchar *bad_key = "XXXXX";
  gint foo_val = 25;
  gint default_value = 99;

  /* Setup Hash table with value(s) */
  g_hash_table_insert(table, foo_key, GINT_TO_POINTER(foo_val));

  /* Test all permutations of the parameters */

  {
    gboolean found_p = xi_get_int(NULL, NULL, NULL, 0, NULL);
    g_assert(!found_p);
  }
  {
    GError *err = NULL;
    gboolean found_p = xi_get_int(NULL, NULL, NULL, 0, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
  }
  {
    GError *err = NULL;
    gboolean found_p = xi_get_int(table, NULL, NULL, 0, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
  }
  {
    GError *err = NULL; 
    gboolean found_p = xi_get_int(table, foo_key, NULL, 0, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
  }
  {
    GError *err = NULL; 
    gint value = 123;
    gboolean found_p = xi_get_int(table, foo_key, &value, 0, &err);
    g_assert(found_p);
    g_assert(value == foo_val);
    g_assert(err == NULL);
  }
  {
    GError *err = NULL; 
    gboolean found_p = xi_get_int(table, foo_key, NULL, default_value, &err);
    g_assert(!found_p);
    g_assert(err != NULL);
  }
  {
    GError *err = NULL;
    gint value = 123;
    gboolean found_p = xi_get_int(table, foo_key, &value, 0, &err);
    g_assert(found_p);
    g_assert(value == foo_val);
    g_assert(err == NULL);
  }
  {
    GError *err = NULL;
    gint value = 123;
    gboolean found_p = xi_get_int(table, bad_key, &value, default_value, &err);
    g_assert(!found_p);
    g_assert(value == default_value);
    g_assert(value != foo_val);
    g_assert(err == NULL);
  }
}

int
main (int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);
  g_test_add_func ("/xieps/xi_get_value", test_xi_get_value);
  g_test_add_func ("/xieps/xi_get_int", test_xi_get_int);
  return g_test_run();
}

/* The g_test_add() function can be used to hook up tests with Fixtures: */
/*   g_test_add ("/scanner/symbols",        // test case name */
/*               ScannerFixture,            // fixture structure type */
/*               NULL,                      // unused data argument */
/*               scanner_fixture_setup,     // fixture setup */
/*               test_scanner_symbols,      // test function */
/*               scanner_fixture_teardown); // fixture teardown */


/* TODO: write test 
   GString* result_str = NULL;
   g_hash_table_insert(options, "title", "Quinn Goes Hiking");
   xi_get_GString(options, "titlex", &result_str, "FooBarDefault", NULL);
*/

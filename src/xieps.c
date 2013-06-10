#include "xieps.h"

/* TODO: The role of this file is not clear */

/* gboolean */
/* xi_turn_page(GHashTable *book, GError **err) */
/* { */
/*   return FALSE; */
/* } */

/* GHashTable* */
/* xi_load_book_from_xml_path(gchar const *book_xml_path, GError **err) { */
/*   GHashTable* book = g_hash_table_new(g_str_hash, g_str_equal); */
/*   return book; */
/* } */

/* GHashTable* */
/* xi_read_settings(GError **err) { */
/*   GHashTable* settings = g_hash_table_new(g_str_hash, g_str_equal); */

/*   /\* TODO: Read settings from file combined with system settings detection *\/ */

/*   g_hash_table_insert(settings, "screen_w", GINT_TO_POINTER(800)); */
/*   g_hash_table_insert(settings, "screen_h", GINT_TO_POINTER(600)); */
/*   g_hash_table_insert(settings, "screen_bpp", GINT_TO_POINTER(32)); */
/*   return settings; */
/* } */

/* void xi_log_to_file(gchar const *log_domain, GLogLevelFlags log_level, */
/*                     gchar const *message, gpointer user_data) */
/* { */
/*   /\* TODO: Are there Glib functions to replace these fopen, printf, */
/*      fprintf, fclose ones? The answer I think is GLib IO Channels: */
/*      https://developer.gnome.org/glib/stable/glib-IO-Channels.html *\/ */
/*   char *filename = "xieps.log"; */
/*   FILE *logfile = fopen(filename, "a"); */
/*   if (logfile == NULL) { */
/*     printf(_("Cannot log to file: %s\n"), filename); */
/*     printf(_("Rerouted to console: %s\n"), message); */
/*     return; */
/*   } */
/*   fprintf (logfile, "%s\n", message); */
/*   fclose(logfile); */
/* } */

/* void */
/* xi_setup_logging() */
/* { */
/*   g_log_set_handler(NULL, */
/*                     G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING */
/*                     | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_DEBUG */
/*                     | G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL, */
/*                     xi_log_to_file, NULL); */
/* } */

/* int main(int argc, char **argv) */
/* { */
/*   xi_setup_logging(); */

/*   gchar const *book_name = "quinn1"; */
/*   GHashTable* book = NULL; */
/*   { */
/*     { */
/*       /\* Load book *\/ */
/*       GError *load_err = NULL; */
/*       book = xi_load_book_from_xml_path("demo/book_quinn1.xml", &load_err); */
/*       if(load_err != NULL) { */
/*         g_error(_("Book failed to load: %s"), load_err->message); */
/*       } */
/*     } */
/*     { */
/*       /\* Load settings *\/ */
/*       GHashTable *settings = NULL; */
/*       { */
/*         GError *settings_err = NULL; */
/*         settings = xi_read_settings(&settings_err); */
/*         if(settings_err != NULL) { */
/*           g_error(_("Failed to read settings properly: %s"), settings_err->message); */
/*         } */
/*       } */
/*       { */
/*         /\* Launch book via SDL *\/ */
/*         GError *launch_err = NULL; */
/*         xi_sdl_launch(book, settings, &launch_err); */
/*         if(launch_err != NULL) { */
/*           g_error(_("Book failed to launch: %s"), launch_err->message); */
/*         } */
/*       } */
/*     } */
/*   } */

/*   return 0; */
/* } */

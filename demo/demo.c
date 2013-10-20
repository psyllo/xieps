#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "../src/xieps.h"
#include "gettext.h"

GHashTable*
demo_create_start_button(){
  GHashTable* button = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_add(button, "text", _("Start"));
  return button;
}

GHashTable*
demo_create_main_menu(){
  GHashTable* menu = g_hash_table_new(g_str_hash, g_str_equal);
  GHashTable* start_button = demo_create_start_button()
  g_hash_table_add(menu, "start_button", start_button);
  return menu;
}

GHashTable*
demo_create_book() {
  GHashTable* book = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_add(book, "name", "quinn1");
  g_hash_table_add(book, "title", "Quinn Goes Hiking");
  g_hash_table_add(book, "splash", "assets/splash.png");
  g_hash_table_add(book, "main_menu", demo_create_main_menu());
  return book;
}


int
main(int argc, char** argv)
{
  xi_launch(demo_create_book());
}

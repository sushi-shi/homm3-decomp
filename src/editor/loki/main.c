/*
 * main.c - Loki h3maped object 0: the program entry, the splash window and
 * the glade tree loader, compiled as C. The window code has the shape of
 * Glade's generated interface.c. The file name is not recorded.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "editor/loki/support.h"

/* cppbridge.cpp: the loaded glade tree and the editor's start-up. */
GladeXML *xml_obj;
void init_objects (int argc, char *argv[]);

static volatile int splash_exposed = 0;

GladeXML *
grabWidgetTree (const char *filename, const char *root)
{
  GladeXML *xml = glade_xml_new (filename, root);
  if (xml == NULL)
    {
      g_warning ("something bad happened while creating the interface");
      exit (1);
    }
  glade_xml_signal_autoconnect (xml);
  return xml;
}

gboolean
on_splash_expose (GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  splash_exposed = 1;
  return FALSE;
}

GtkWidget*
create_splash_window (void)
{
  GtkWidget *splash_window;
  GtkWidget *splash_pixmap;

  add_pixmap_directory ("./");
  splash_window = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_object_set_data (GTK_OBJECT (splash_window), "splash_window", splash_window);
  gtk_window_set_position (GTK_WINDOW (splash_window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (splash_window), TRUE);
  gtk_window_set_policy (GTK_WINDOW (splash_window), TRUE, TRUE, FALSE);

  splash_pixmap = create_pixmap (splash_window, "splash.xpm");
  gtk_widget_ref (splash_pixmap);
  gtk_object_set_data_full (GTK_OBJECT (splash_window), "splash_pixmap", splash_pixmap,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_signal_connect (GTK_OBJECT (splash_window), "expose_event",
                      GTK_SIGNAL_FUNC (on_splash_expose),
                      NULL);
  gtk_widget_show (splash_pixmap);
  gtk_container_add (GTK_CONTAINER (splash_window), splash_pixmap);

  gtk_widget_show (splash_window);
  gdk_window_set_decorations (splash_window->window, 0);
  while (!splash_exposed)
    gtk_main_iteration ();
  return splash_window;
}

int
main (int argc, char *argv[])
{
  int show_splash = 1;
  int i;
  GdkVisual *visual;
  GtkWidget *window = NULL;
  GtkWidget *unused;
  char *glade_file = "heroes-iii-level-editor.glade";
  FILE *fp;
  GtkWidget *splash = NULL;

  setbuf (stdout, NULL);
  setbuf (stderr, NULL);

  fp = fopen (glade_file, "r");
  if (fp == NULL)
    {
      g_error ("\n\nCannot read GLADE file [%s]!\n  Aborting.\n\n\n", glade_file);
      return 1;
    }
  fclose (fp);

  srand (time (NULL));
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  glade_init ();

  for (i = 1; i < argc; i++)
    {
      if (strcasecmp (argv[i], "--no-splash") == 0)
        {
          show_splash = 0;
          argv[i] = NULL;
        }
      if (strcasecmp (argv[i], "--help") == 0)
        {
          fprintf (stderr, "\nUSAGE: %s [--no-splash] [mapfile]\n\n", argv[0]);
          _exit (0);
        }
    }

  if (show_splash)
    splash = create_splash_window ();

  visual = gdk_visual_get_system ();
  if (visual->depth != 16 && visual->depth != 24)
    {
      xml_obj = grabWidgetTree (glade_file, "bad_color_dlg");
      window = glade_xml_get_widget (xml_obj, "bad_color_dlg");
    }
  else
    {
      xml_obj = grabWidgetTree (glade_file, NULL);
      window = glade_xml_get_widget (xml_obj, "MainWindow");
      init_objects (argc, argv);
    }

  if (window == NULL)
    g_warning ("Hhm. Seems that there's a screwup in your .glade XML file.");
  else
    {
      gtk_widget_show (window);
      if (show_splash)
        gtk_widget_hide (splash);
      gtk_main ();
    }
  return 0;
}

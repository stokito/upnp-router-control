/* urc-main.c
 *
 * Copyright 2021 Daniele Napolitano <dnax88@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "urc-gui.h"
#include "urc-upnp.h"

/* Options variables */
static gboolean opt_version = FALSE;
gboolean opt_debug = FALSE;
gchar* opt_bindif = NULL;
guint opt_bindport = 0;

/* Options schema */
static GOptionEntry entries[] = 
{
    { "if", 'i', 0, G_OPTION_ARG_STRING, &opt_bindif, "The network interface used (all if omitted)", NULL },
    { "port", 'p', 0, G_OPTION_ARG_INT, &opt_bindport, "Use a specific source port", NULL },
    { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version and exit", NULL },
    { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug, "Allow debug messages", NULL },
    { NULL }
};

static void
urc_print_version()
{
    g_print("%s %s\n", PACKAGE, VERSION);
}

static void
urc_activate_cb (GApplication *app, gpointer user_data)
{
    g_set_application_name(_("UPnP Router Control"));

    gtk_window_set_default_icon_name (APPLICATION_ID);

    /* Initialize the GUI */
    urc_gui_init(app);

    /* Enter in the main loop */
    gtk_main();
}

static void
urc_startup_cb (GApplication *app, gpointer user_data)
{
    /* Initialize the UPnP subsystem */
    upnp_init();
}

int
main(int argc, char** argv)
{
    GtkApplication *app;
    int status;

    GError *error = NULL;
    GOptionContext *context = NULL;
    
    /* gettext */
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);  
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
     
    /* commandline options */
    context = g_option_context_new ("- A simple program to manage UPnP IGD compliant routers");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    
    if (!g_option_context_parse (context, &argc, &argv, &error))
        g_warning ("option parsing failed: %s\n", error->message);
    
    if (opt_version) {
        /* print version and exit */
        urc_print_version();
        return EXIT_SUCCESS;
    }
    else {
      app = gtk_application_new ("org.upnproutercontrol.UPnPRouterControl", G_APPLICATION_FLAGS_NONE);
      g_signal_connect (app, "activate", G_CALLBACK (urc_activate_cb), NULL);
      g_signal_connect (app, "startup", G_CALLBACK (urc_startup_cb), NULL);
      status = g_application_run (G_APPLICATION (app), argc, argv);
      g_object_unref (app);
      return status;
    }
}

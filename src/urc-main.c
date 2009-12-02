/* urc-main.c
 *
 * Copyright (C) 2009 Daniele Napolitano
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "urc-gui.h"
#include "urc-upnp.h"

/* Options variables */
static gboolean opt_version = FALSE;
static gboolean opt_debug = FALSE;
static gchar* opt_bindif = NULL;
static guint opt_bindport = 0;

/* Options schema */
static GOptionEntry entries[] = 
{
  { "if", 'i', 0, G_OPTION_ARG_STRING, &opt_bindif, "The network interface to use (autodetect if omitted)", NULL },
  { "port", 'p', 0, G_OPTION_ARG_INT, &opt_bindport, "Use a specific source port", NULL },
  { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version and exit", NULL },
  { "debug", 0, 0, G_OPTION_ARG_NONE, &opt_debug, "Allow debug messages", NULL },
  { NULL }
};

static void
version()
{
    g_print("%s %s\n", PACKAGE, VERSION);
}

int 
main(int argc, char** argv)
{
    GError *error = NULL;
    GOptionContext *context = NULL;
    
    /* gettext */
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);  
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
     
    /* commandline options */
    context = g_option_context_new ("- A simple program to manage UPnP IGD compliant routers");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    
    if (!g_option_context_parse (context, &argc, &argv, &error))
        g_warning ("option parsing failed: %s\n", error->message);
    
    if(opt_version)
        /* print version and exit */
        version();
    else
    {
        gtk_init(&argc, &argv);
	
        gtk_window_set_default_icon_name ("upnp-router-control");
    
        g_set_application_name(_("UPnP Router Control"));
        
        gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),
			    DATADIR G_DIR_SEPARATOR_S "icons");
        
        /* Required initialisation */
        g_thread_init (NULL);
        g_type_init ();
        
        /* Initialize the GUI */
        gui_init();
        
        /* Initialize the UPnP subsystem */
        if( ! upnp_init(opt_bindif, opt_bindport, opt_debug) )
            return 1;
        
        /* Enter in the main loop */
        gtk_main();        
    
    }
    
    return 0;  
}


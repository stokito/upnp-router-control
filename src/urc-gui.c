/* urc-gui.c
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

#define UI_FILE DATADIR"/upnp-router-control/upnp-router-control.glade"

enum Columns
{
    UPNP_COLUMN_ENABLED,
    UPNP_COLUMN_DESC,
    UPNP_COLUMN_PROTOCOL,
    UPNP_COLUMN_INT_PORT,
    UPNP_COLUMN_EXT_PORT,
    UPNP_COLUMN_LOCAL_IP,
    UPNP_COLUMN_REM_IP   
};

typedef struct
{
    GtkBuilder* builder;
    
    GtkWidget *main_window,
              *treeview,
              *router_name_label,
              *router_url_eventbox,
              *router_url_label,
              *wan_status_label,
              *ip_label,
              *down_rate_label,
              *up_rate_label;
} GuiContext;

static GuiContext* gui;

/* Clean the Treeview */
void gui_clear_ports_list_treeview (void)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      more;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));
        more = gtk_tree_model_get_iter_first (model, &iter);

        while (more)
                more = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

/* Add a port mapped in the treeview list */
void gui_add_mapped_port(const gboolean enabled, const gchar *description, const gchar *protocol, const guint internal_port,
                         const guint external_port, const gchar *internal_host, const gchar *remote_host)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));
    
    gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model),
                        &iter,
                        UPNP_COLUMN_ENABLED, enabled,
                        UPNP_COLUMN_DESC, description,
                        UPNP_COLUMN_PROTOCOL, protocol,
                        UPNP_COLUMN_INT_PORT, internal_port,
                        UPNP_COLUMN_EXT_PORT, external_port,
                        UPNP_COLUMN_LOCAL_IP, internal_host,
                        UPNP_COLUMN_REM_IP, remote_host,
                        -1);
}

/* Button remove callback */
static void on_button_remove_clicked (GtkWidget *button,
                                      gpointer   user_data)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreeSelection *selection;
    
    gchar* remote_host;
    guint external_port;
    gchar* protocol;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->treeview));
    
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;
    
    gtk_tree_model_get(model, &iter,
                       UPNP_COLUMN_PROTOCOL, &protocol,
                       UPNP_COLUMN_EXT_PORT, &external_port,
                       UPNP_COLUMN_REM_IP, &remote_host,
                       -1);
    
    delete_port_mapped (user_data, protocol, external_port, remote_host);
    
}


/* Callback for enable checkbox */
static void list_enable_toggled_cb (GtkCellRendererToggle *widget,
                             gchar                 *path,
                             GtkWidget             *treeview)
{
    gboolean enabled;
    
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->treeview));

    if (!gtk_tree_model_get_iter_from_string(model, &iter, path))
        return;

    gtk_tree_model_get(model, &iter,
                       UPNP_COLUMN_ENABLED, &enabled,
                       -1);

    /* set value in store */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                        UPNP_COLUMN_ENABLED, !enabled,
                        -1);

}

/* Initialize model for Treeview */
static void gui_init_treeview()
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    int i;
    char         *headers[8] = {_("Enabled"),
                                _("Description"),
                                _("Protocol"),
                                _("Int. Port"),
                                _("Ext. Port"),
                                _("Local IP"),
                                _("Remote IP"),
                                NULL };
    
    GtkListStore *store;

    store = gtk_list_store_new (7,
                                G_TYPE_BOOLEAN, /* Enabled           */
                                G_TYPE_STRING,  /* Description       */
                                G_TYPE_STRING,  /* Protocol          */
                                G_TYPE_UINT,    /* Internal port     */
                                G_TYPE_UINT,    /* External port     */
                                G_TYPE_STRING,  /* Internal host     */
                                G_TYPE_STRING); /* Remove host Value */
    
    model = GTK_TREE_MODEL (store);
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->treeview));
    g_assert (selection != NULL);
        
    for (i = 0; headers[i] != NULL; i++) {
                GtkCellRenderer   *renderer;
                GtkTreeViewColumn *column;

                column = gtk_tree_view_column_new ();
                if(i == 0)
                {
                    renderer = gtk_cell_renderer_toggle_new ();
                    gtk_tree_view_column_pack_end (column, renderer, FALSE);

                    gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "active", i);
                                                    
                    g_signal_connect(G_OBJECT(renderer), "toggled",
                         G_CALLBACK(list_enable_toggled_cb),
                         NULL);

                    g_object_set(renderer,
                     "radio", FALSE,
                     "activatable", TRUE,
                      NULL);                    
                }
                else
                {
                    renderer = gtk_cell_renderer_text_new ();
                    gtk_tree_view_column_pack_end (column, renderer, TRUE);
                    gtk_tree_view_column_set_title (column, headers[i]);
                    
                    gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "text", i);
                }                
  
                gtk_tree_view_column_set_sort_column_id(column, i);
                
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_FIXED);
                
                gtk_tree_view_insert_column (GTK_TREE_VIEW (gui->treeview),
                                             column, -1);
                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);
                gtk_tree_view_column_set_resizable (column, TRUE);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (gui->treeview),
                                 model);
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    
}

void gui_disable_download_speed()
{
    gtk_widget_set_sensitive(gui->down_rate_label, FALSE);
    gtk_label_set_text (GTK_LABEL(gui->down_rate_label), _("n.a.") );
}

/* Update router speeds, values in KiB/sec */
void gui_set_download_speed(const gdouble down_speed)
{
    gchar* str;
    gchar* unit;
    gfloat value;
    /* Method of divisions is too expensive? */
        
    /* Down speed */
    if(down_speed >= 1024)
    {
        value = down_speed / 1024.0;
        unit = g_strdup("MiB/s");
    }
    else if(down_speed >= 1048576)
    {
        value = down_speed / 1048576.0;
        unit = g_strdup("GiB/s");
    }
    else
    {
        value = down_speed;
        unit = g_strdup("KiB/s");
    }
    str = g_strdup_printf("%0.1f %s", value, unit);
    g_free(unit);
    gtk_label_set_text (GTK_LABEL(gui->down_rate_label), str);
    g_free(str);
    
    gtk_widget_set_sensitive(gui->down_rate_label, TRUE);
    
}

void gui_disable_upload_speed()
{
    gtk_widget_set_sensitive(gui->up_rate_label, FALSE);
    gtk_label_set_text (GTK_LABEL(gui->up_rate_label), _("n.a.") );
}

/* Update router speeds, values in KiB/sec */
void gui_set_upload_speed(const gdouble up_speed)
{
    gchar* str;
    gchar* unit;
    gfloat value;
    /* Method of divisions is too expensive? */
    
    /* Up speed */     
    if(up_speed >= 1024)
    {
        value = up_speed / 1024.0;
        unit = g_strdup("MiB/s");
    }
    else if(up_speed >= 1048576)
    {
        value = up_speed / 1048576.0;
        unit = g_strdup("GiB/s");
    }
    else
    {
        value = up_speed;
        unit = g_strdup("KiB/s");
    }
    str = g_strdup_printf("%0.1f %s", value, unit);
    g_free(unit);
    gtk_label_set_text (GTK_LABEL(gui->up_rate_label), str);
    g_free(str);
    
    gtk_widget_set_sensitive(gui->up_rate_label, TRUE);
    
}

/* Set WAN state label */
void gui_set_conn_status(const gchar *state)
{
    gchar* str = NULL;

    if(g_strcmp0("Connected", state) == 0)
            str = g_strdup( _("<b>WAN status:</b> <span color=\"#009000\"><b>Connected</b></span>"));
    if(g_strcmp0("Disconnected", state) == 0)
            str = g_strdup( _("<b>WAN status:</b> <span color=\"#900000\"><b>Disconnected</b></span>"));
    if(g_strcmp0("Connecting", state) == 0)
            str = g_strdup( _("<b>WAN status:</b> <span color=\"#0000a0\"><b>Connecting</b></span>"));
    if(g_strcmp0("Disconnecting", state) == 0)
            str = g_strdup( _("<b>WAN status:</b> <span color=\"#c04000\"><b>Disconnecting</b></span>"));
    
    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), str);
    
    gtk_widget_set_sensitive(gui->wan_status_label, TRUE);
    
}

/* Set WAN state label unknown */
void gui_disable_conn_status()
{
    
    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), _("<b>WAN status:</b> unknown"));
    
}

/* Set external IP address */
void gui_set_ext_ip(const gchar *ip)
{
    gchar* str;
    
    if(ip == NULL)
        str = g_strdup( _("<b>IP:</b> none"));
    else
        str = g_strdup_printf( _("<b>IP:</b> %s"), ip);

    gtk_label_set_markup (GTK_LABEL(gui->ip_label), str);
    g_free(str);  
    
    gtk_widget_set_sensitive(gui->ip_label, TRUE);
    
}

/* Set external IP address */
void gui_disable_ext_ip()
{
    
    gtk_label_set_markup (GTK_LABEL(gui->ip_label), _("<b>IP:</b> unknown"));
 
}

/* onMouseOver URL label callback */
static gboolean on_mouseover_cb (GtkWidget        *widget,
                                 GdkEventCrossing *event,
                                 gpointer          user_data)
{
    GdkCursor *cursor;
    cursor = gdk_cursor_new_for_display (gdk_display_get_default(), GDK_HAND2);
    gdk_window_set_cursor (widget->window, cursor);
    return FALSE;
}

/* onMouseOut URL label callback */
static gboolean on_mouseout_cb (GtkWidget        *widget,
                                GdkEventCrossing *event,
                                gpointer          user_data)
{
    GdkCursor *cursor;
    cursor = gdk_cursor_new_for_display (gdk_display_get_default(), GDK_LEFT_PTR);
    gdk_window_set_cursor (widget->window, cursor);

    return FALSE;
}

/* onClick URL label callback */
static gboolean on_mousepress_cb (GtkWidget      *widget,
                                  GdkEventButton *event,
                                  gpointer        user_data)
{
    gchar* str;
    guint ret;    
    str = g_strdup_printf("xdg-open %s", (gchar*) user_data);
    ret = system(str);
    g_free(str);

    return FALSE;
}

/* Set router informations */
void gui_set_router_info (const gchar *router_friendly_name,
                          const gchar *router_conf_url,
                          const gchar *router_brand,
                          const gchar *router_brand_website,
                          const gchar *router_model_name,
                          const gchar *router_model_number)
{
    gchar* str;
    
    if(gui == NULL)
        return;
     
    str = g_strdup_printf( _("<b>Router name:</b> %s"), router_friendly_name);
    gtk_label_set_markup (GTK_LABEL(gui->router_name_label), str);
    g_free(str);
    
    gtk_widget_set_sensitive(gui->router_name_label, TRUE);
    
    str = g_strdup_printf( _("<b>Brand:</b> %s\n<b>Brand website:</b> %s\n<b>Model Name:</b> %s\n<b>Model Number:</b> %s"),
                           router_brand,
                           router_brand_website,
                           router_model_name,
                           router_model_number);
    
    gtk_widget_set_tooltip_markup(gui->router_name_label, str); 
    g_free(str);  
    
    str = g_strdup_printf( "<span color=\"blue\"><u>%s</u></span>", router_conf_url);
    gtk_label_set_markup (GTK_LABEL(gui->router_url_label), str);
    g_free(str);
    
    gtk_widget_set_sensitive(gui->router_url_label, TRUE);
    
    gtk_widget_set_tooltip_text(gui->router_url_label, _("Open the router config in the default browser"));
    
    gtk_signal_connect ( GTK_OBJECT(gui->router_url_eventbox), "enter-notify-event",
		    		     GTK_SIGNAL_FUNC(on_mouseover_cb), NULL
		               );
	gtk_signal_connect ( GTK_OBJECT(gui->router_url_eventbox), "leave-notify-event",
		    		     GTK_SIGNAL_FUNC(on_mouseout_cb), NULL
		               );
	gtk_signal_connect ( GTK_OBJECT(gui->router_url_eventbox), "button-press-event",
		    		     GTK_SIGNAL_FUNC(on_mousepress_cb), (gchar *) router_conf_url
		               );    

}

void gui_set_button_delete_callback_data(gpointer data)
{
    g_signal_connect(gtk_builder_get_object (gui->builder, "button_remove"), "clicked",
                     G_CALLBACK(on_button_remove_clicked), data);
}

void gui_enable()
{
    if(gui == NULL)
        return;
    
    gtk_widget_set_sensitive(gui->router_name_label, TRUE);
    gtk_widget_set_sensitive(gui->wan_status_label, TRUE);
    gtk_widget_set_sensitive(gui->ip_label, TRUE);
    gtk_widget_set_sensitive(gui->down_rate_label, TRUE);
    gtk_widget_set_sensitive(gui->up_rate_label, TRUE);
    gtk_widget_set_sensitive(gui->router_url_label, TRUE);
}

void gui_disable()
{
    if(gui == NULL)
        return;
    
    gui_clear_ports_list_treeview();    

    gtk_label_set_markup (GTK_LABEL(gui->router_name_label), _("<b>Router name:</b> not available"));
    gtk_widget_set_sensitive(gui->router_name_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), _("<b>WAN status:</b> not available"));
    gtk_widget_set_sensitive(gui->wan_status_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(gui->ip_label), _("<b>IP:</b> not available"));
    gtk_widget_set_sensitive(gui->ip_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(gui->down_rate_label), _("n.a."));
    gtk_widget_set_sensitive(gui->down_rate_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(gui->up_rate_label), _("n.a."));
    gtk_widget_set_sensitive(gui->up_rate_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(gui->router_url_label), _("not available"));
    gtk_widget_set_sensitive(gui->router_url_label, FALSE);
    
}

/* Menu About activate callback */
static void on_about_activate_cb (GtkMenuItem *menuitem, 
                                  gpointer     user_data)
{
    gchar* authors[] = {
		"Daniele Napolitano <dnax88@gmail.com>",
		NULL
	};
    gchar* artists[] = {
		"Icon design: Frédéric Bellaiche - http://www.quantum-bits.org",
		NULL
	};
	/* Feel free to put your names here translators :-) */
	gchar* translators = _("translator-credits");
	
	gtk_show_about_dialog (GTK_WINDOW(gui->main_window),
             "authors", authors,
		     "artists", artists,
		     "translator-credits", strcmp("translator-credits", translators) ? translators : NULL,
             "comments", _("A simple program to manage UPnP compliant routers"),
             "copyright", "Copyright © 2009 Daniele Napolitano \"DnaX\"",
             "version", VERSION,
             "website", "http://launchpad.net/upnp-port-mapper",
		     "logo-icon-name", "upnp-router-control",
             NULL);
}

void gui_init()
{
    GtkBuilder* builder;
    GError* error = NULL;
    
    g_print("* Initializing GUI...\n");
    
    gui = g_malloc( sizeof(GuiContext) );
    
    builder = gui->builder = gtk_builder_new ();
    if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }
    
    gui->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    g_assert (gui->main_window != NULL);
    
    gui->treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview1"));
    
    gui->router_name_label = GTK_WIDGET (gtk_builder_get_object (builder, "router_name_label"));
    gui->router_url_label = GTK_WIDGET (gtk_builder_get_object (builder, "router_url_label"));
    gui->wan_status_label = GTK_WIDGET (gtk_builder_get_object (builder, "wan_status_label"));
    gui->ip_label = GTK_WIDGET (gtk_builder_get_object (builder, "ip_label"));
    gui->down_rate_label = GTK_WIDGET (gtk_builder_get_object (builder, "down_rate_label"));
    gui->up_rate_label = GTK_WIDGET (gtk_builder_get_object (builder, "up_rate_label"));
    gui->router_url_eventbox = GTK_WIDGET (gtk_builder_get_object (builder, "router_url_eventbox"));
    
    g_signal_connect(gtk_builder_get_object (builder, "menuitem_about"), "activate",
                         G_CALLBACK(on_about_activate_cb), NULL);
    
    g_signal_connect(gtk_builder_get_object (builder, "menuitem_exit"), "activate",
                         G_CALLBACK(gtk_main_quit), NULL);
    
    g_signal_connect(G_OBJECT(gui->main_window), "destroy",
                         G_CALLBACK(gtk_main_quit), NULL);
                             
    gui_init_treeview();
    
    gui_disable();
    
    g_print("* Showing GUI...\n");
    
    gtk_widget_show_all(gui->main_window);
    
}

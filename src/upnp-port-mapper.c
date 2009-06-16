/* upnp-port-mapper.c
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
#include <glade/glade.h>
#include <libgupnp/gupnp-control-point.h>

#include "upnp-port-mapper.h"

#define GLADE_FILE DATADIR"/upnp-port-mapper/upnp-port-mapper.glade"

router_info router;
GladeXML* glade;

ui_context ui;

/* Button remove callback */
void on_button_remove_clicked (GtkWidget *button,
                               gpointer   user_data)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreeSelection *selection;
    GError *error = NULL;
    
    gchar* remote_host;
    guint external_port;
    gchar* protocol;
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui.treeview));
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui.treeview));
    
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;
    
    gtk_tree_model_get(model, &iter,
                       UPNP_COLUMN_PROTOCOL, &protocol,
                       UPNP_COLUMN_EXT_PORT, &external_port,
                       UPNP_COLUMN_REM_IP, &remote_host,
                       -1);
    
    gupnp_service_proxy_send_action (router.wan_service,
				   /* Action name and error location */
				   "DeletePortMapping", &error,
				   /* IN args */
				   "NewRemoteHost",
				   G_TYPE_STRING, remote_host,
				   "NewExternalPort",
				   G_TYPE_UINT, external_port,
				   "NewProtocol",
				   G_TYPE_STRING, protocol,
				   NULL,
				   NULL);
    
    if (error == NULL) {
	    
	    g_print("*** Removed entry: Port %d (%s) IP %s\n", external_port, protocol, remote_host );
        
    } else {
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
    }    
    
}

/* Clean the Treeview */
static void clear_port_mapping_treeview (void)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      more;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui.treeview));
        more = gtk_tree_model_get_iter_first (model, &iter);

        while (more)
                more = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

/* Retrive ports mapped and populate the treeview */
static void make_port_mapping_list()
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    
    gchar* remote_host;
    guint external_port;
    
    gchar* protocol;
    guint internal_port;
    gchar* internal_host;    
    
    gboolean enabled;   
    gchar* description;   
    guint lease_time;
    GError *error = NULL;
    int i;
    
    /* Clear previus entries */
    clear_port_mapping_treeview();
    
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (ui.treeview));

    g_print("\e[1;32m==> List port mapped (%d)\e[0;0m\n", router.PortMappingNumberOfEntries);
    
    for(i = 0; i < router.PortMappingNumberOfEntries; i++)
    {
        gupnp_service_proxy_send_action (router.wan_service,
				   /* Action name and error location */
				   "GetGenericPortMappingEntry", &error,
				   /* IN args */
				   "NewPortMappingIndex",
				   G_TYPE_UINT, i,
				   NULL,
				   /* OUT args */
				   "NewRemoteHost",
				   G_TYPE_STRING, &remote_host,
				   "NewExternalPort",
				   G_TYPE_UINT, &external_port,
				   "NewProtocol",
				   G_TYPE_STRING, &protocol,
				   "NewInternalPort",
				   G_TYPE_UINT, &internal_port,
				   "NewInternalClient",
				   G_TYPE_STRING, &internal_host,
				   "NewEnabled",
				   G_TYPE_BOOLEAN, &enabled,
				   "NewPortMappingDescription",
				   G_TYPE_STRING, &description,
				   "NewLeaseDuration",
				   G_TYPE_UINT, &lease_time,
				   NULL);
	    
	    if (error == NULL) {
	        g_print (" * %s [%s]\n", description, enabled ? "X" : " " );
	        
	        g_print ("   RemoteIP: %s, ExtPort: %d %s, IntPort: %d, IntIP: %s\n",
	                 remote_host,
	                 external_port,
	                 protocol,
	                 internal_port,
	                 internal_host
	                 );
            
            if(external_port > 0)
            {
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
            g_free (description);
            g_free (protocol);
            g_free (internal_host);
            g_free (remote_host);
        } else {
            g_printerr ("Error: %s\n", error->message);
            g_error_free (error);
        } 
    }
}

/* Callback for enable checkbox */
static void list_enable_toggled_cb (GtkCellRendererToggle *widget,
                             gchar                 *path,
                             GtkWidget             *treeview)
{
    gboolean enabled;
    
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui.treeview));

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
static void setup_treeview()
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
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui.treeview));
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
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);

                gtk_tree_view_insert_column (GTK_TREE_VIEW (ui.treeview),
                                             column,
                                             -1);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (ui.treeview),
                                 model);
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    
}

/* Retrive download and upload speeds */
static gboolean update_data_rate_cb (gpointer data)
{
    GError *error = NULL;
    static guint old_total_bytes_received = 0;
    guint current_total_bytes_received;
    double data_rate_down;
    
    static guint old_total_bytes_sent = 0;
    guint current_total_bytes_sent;
    double data_rate_up;
    
    gchar *str;
    
    /* download speed */
    gupnp_service_proxy_send_action (router.wan_device,
				   /* Action name and error location */
				   "GetTotalBytesReceived", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesReceived",
				   G_TYPE_UINT, &current_total_bytes_received,
				   NULL);
	    
	if (error == NULL) {	    
	       
        gtk_widget_set_sensitive(ui.down_rate_label, TRUE);   
	    
	    if(old_total_bytes_received == 0)
	        data_rate_down = 0.0;
	    else
	        data_rate_down = (current_total_bytes_received - old_total_bytes_received) / 1024.0;
	        
        str = g_strdup_printf("%0.1f KiB/s", data_rate_down);
        gtk_label_set_text (GTK_LABEL(ui.down_rate_label), str);
        g_free(str);
        old_total_bytes_received = current_total_bytes_received;
        
    } else {
        gtk_widget_set_sensitive(ui.down_rate_label, FALSE);
        gtk_label_set_text (GTK_LABEL(ui.down_rate_label), _("n.a.") );   
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
    
    /* upload speed */
    gupnp_service_proxy_send_action (router.wan_device,
				   /* Action name and error location */
				   "GetTotalBytesSent", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesSent",
				   G_TYPE_UINT, &current_total_bytes_sent,
				   NULL);
	    
	if (error == NULL) {
	    /* Action sended succefully */
	    gtk_widget_set_sensitive(ui.up_rate_label, TRUE); 
	    
	    if(old_total_bytes_sent == 0)
	        data_rate_up = 0.0;
	    else
	        data_rate_up = (current_total_bytes_sent - old_total_bytes_sent) / 1024.0;
	        
        str = g_strdup_printf("%0.1f KiB/s", data_rate_up);
        gtk_label_set_text (GTK_LABEL(ui.up_rate_label), str);
        g_free(str);
        old_total_bytes_sent = current_total_bytes_sent;
        
    } else {
        /* Errors on sending action */
        gtk_widget_set_sensitive(ui.up_rate_label, FALSE);
        gtk_label_set_text (GTK_LABEL(ui.up_rate_label), _("n.a.") );
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
        
    return TRUE;
}

/* Set WAN state label */
static void gui_set_state(const gchar *state)
{
    gchar* str;
    if(g_strcmp0("Connected", state) == 0)
            str = g_strdup( _("<b>WAN status:</b> <span color=\"#009000\"><b>Connected</b></span>"));
        else
            str = g_strdup( _("<b>WAN status:</b> <span color=\"green\">Disconnected</span>"));
    
    gtk_label_set_markup (GTK_LABEL(ui.wan_status_label), str);
    
    gtk_widget_set_sensitive(ui.wan_status_label, TRUE);
    
}

/* Retrive connection infos: connection status, uptime and last error. */
static gboolean get_conn_status (gpointer data)
{
    GError *error = NULL;
    gchar* conn_status;
    gchar* last_conn_error;
    guint  uptime = 0;
    
    g_print("\e[1;36mRequest for connection status info... ");
    
    /* download speed */
    gupnp_service_proxy_send_action (router.wan_device,
				   /* Action name and error location */
				   "GetStatusInfo", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewConnectionStatus",
				   G_TYPE_STRING, &conn_status,
				   "NewLastConnectionError",
				   G_TYPE_STRING, &last_conn_error,
				   "NewUptime",
				   G_TYPE_UINT, &uptime,
				   NULL);
				   
    if (error == NULL)
    {
        gtk_widget_set_sensitive(ui.wan_status_label, TRUE);
        gui_set_state(conn_status);
        if(g_strcmp0("Connected", conn_status) == 0)
            router.connected = TRUE;
        else
            router.connected = FALSE;
        
        g_print("\e[1;32msuccessful\e[0;0m\n");
        g_print("\e[1;37mConnection info:\e[0m Status: %s, Uptime: %i sec.\n", conn_status, uptime);
        
        if(g_strcmp0("ERROR_NONE", last_conn_error) != 0)
            g_print("\e[33mLast connection error:\e[0m %s\n", last_conn_error);
        
        return TRUE;        
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");
        gtk_widget_set_sensitive(ui.wan_status_label, TRUE);
        gtk_label_set_text (GTK_LABEL(ui.wan_status_label), _("unknown") );   
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
}

/* Retrive external IP address */
static gboolean get_external_ip (gpointer data)
{
    GError *error = NULL;
    gchar* ext_ip_addr;
    
    g_print("\e[1;36mRequest for external IP address... ");
    
    /* download speed */
    gupnp_service_proxy_send_action (router.wan_device,
				   /* Action name and error location */
				   "GetExternalIPAddress", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewExternalIPAddress",
				   G_TYPE_STRING, &ext_ip_addr,
				   NULL);
				   
    if (error == NULL)
    {
        router.external_ip = g_strdup(ext_ip_addr);
        
        g_print("\e[1;32msuccessful \e[1;37m[%s]\e[0;0m\n", router.external_ip);
        
        gchar *str;
        str = g_strdup_printf( _("<b>IP:</b> %s"), router.external_ip);
        gtk_label_set_markup (GTK_LABEL(ui.ip_label), str);
        gtk_widget_set_sensitive(ui.ip_label, TRUE);
        g_free(str); 
        return TRUE;        
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");
        gtk_widget_set_sensitive(ui.ip_label, FALSE);
        gtk_label_set_text (GTK_LABEL(ui.ip_label), _("unknown") );   
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
}

/* Service event callback */
static void service_proxy_event_cb (GUPnPServiceProxy *proxy,
                                        const char *variable,
                                        GValue *value,
                                        gpointer user_data)
{
    
    /* Numebr of port mapped entries */
    if(g_strcmp0("PortMappingNumberOfEntries", variable) == 0)
    {
        router.PortMappingNumberOfEntries = g_value_get_uint(value);
        g_print("\e[1;33mEvent:\e[0;0m Ports mapped: %d\n", router.PortMappingNumberOfEntries);
        make_port_mapping_list();           
    }
    /* Got external IP */
    else if(g_strcmp0("ExternalIPAddress", variable) == 0)
    {
        router.external_ip = g_strdup(g_value_get_string(value));
        g_print("\e[1;33mEvent:\e[0;0m External IP: %s\n", router.external_ip);
        
        gchar *str;
        str = g_strdup_printf( _("<b>IP:</b> %s"), router.external_ip);
        gtk_label_set_markup (GTK_LABEL(ui.ip_label), str);
        gtk_widget_set_sensitive(ui.ip_label, TRUE);
        g_free(str);           
    }
    /* WAN connection status changed */
    else if(g_strcmp0("ConnectionStatus", variable) == 0)
    {        
        gtk_widget_set_sensitive(ui.wan_status_label, TRUE);
        gui_set_state(g_value_get_string(value));
        if(g_strcmp0("Connected", g_value_get_string(value)) == 0)
            router.connected = TRUE;
        else
            router.connected = FALSE;
        g_print("\e[1;33mEvent:\e[0;0m Connection status: %s\n", g_value_get_string(value) );
    }
}

/* onMouseOver URL label callback */
static gboolean on_mouseover_cb (GtkWidget        *widget,
                                 GdkEventCrossing *event,
                                 gpointer          user_data)
{
    gtk_label_set_markup (GTK_LABEL(ui.router_url_label),
                _("<span color=\"blue\"><u>Open router config</u></span>") );
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
    gtk_label_set_markup (GTK_LABEL(ui.router_url_label),
                _("<span color=\"blue\">Open router config</span>") );
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
    str = g_strdup_printf("xdg-open %s", router.http_address);
    ret = system(str);
    g_free(str);

    return FALSE;
}

/* Device available callback */
static void device_proxy_available_cb (GUPnPControlPoint *cp,
                                       GUPnPServiceProxy *proxy)
{
  GList *child;
  GError *error = NULL;

  char *string_buffer = NULL;
  static char *connect_service = NULL;
  const char *service_type = NULL;
  const char *service_id = NULL;
  const char *device_type = NULL;
  
  g_print("==> Device Available: \e[1;31m%s\e[0;0m\n",
           gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy)));
  device_type = gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO (proxy));
  
  /* Is a gateway? */
  if(g_strcmp0(device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:1") == 0)
  {
    router.main_device = GUPNP_DEVICE_INFO (proxy);
    router.friendly_name = gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy));
    router.vendor = gupnp_device_info_get_manufacturer(GUPNP_DEVICE_INFO (proxy));
    router.http_address = gupnp_device_info_get_presentation_url(GUPNP_DEVICE_INFO (proxy));
    
    gchar* str;
  
    str = g_strdup_printf( _("<b>Name:</b> %s"), router.friendly_name);
    gtk_label_set_markup (GTK_LABEL(ui.router_name_label), str);
    gtk_widget_set_sensitive(ui.router_name_label, TRUE);
    g_free(str);
    
    gtk_label_set_markup (GTK_LABEL(ui.router_url_label),
                _("<span color=\"blue\">Open router config</span>") );
    gtk_widget_set_sensitive(ui.router_url_label, TRUE);
    str = g_strdup_printf("Go to %s", router.http_address);
    gtk_widget_set_tooltip_text(ui.router_url_label, str);
    g_free(str);

    gtk_signal_connect( GTK_OBJECT(ui.router_url_eventbox), "enter-notify-event",
		    		GTK_SIGNAL_FUNC(on_mouseover_cb), NULL
		    );
	gtk_signal_connect( GTK_OBJECT(ui.router_url_eventbox), "leave-notify-event",
		    		GTK_SIGNAL_FUNC(on_mouseout_cb), NULL
		    );
	gtk_signal_connect( GTK_OBJECT(ui.router_url_eventbox), "button-press-event",
		    		GTK_SIGNAL_FUNC(on_mousepress_cb), NULL
		    );    
  }

  /* Enum services */
  child = gupnp_device_info_list_services(GUPNP_DEVICE_INFO (proxy));
  
  if (g_list_length(child) > 0)
    g_print("\e[1;32mEnum services...\e[0;0m\n");
  
  while( child )
  {
    
    service_type = gupnp_service_info_get_service_type(GUPNP_SERVICE_INFO (child->data));
    service_id = gupnp_service_info_get_id(GUPNP_SERVICE_INFO (child->data));
    g_print(" > Service Available: %s\n", service_id );
    
    /* Is a IP forwarding service? */
    if(g_strcmp0 (service_type, "urn:schemas-upnp-org:service:Layer3Forwarding:1") == 0)
    {
        gupnp_service_proxy_send_action (child->data,
				   /* Action name and error location */
				   "GetDefaultConnectionService", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewDefaultConnectionService",
				   G_TYPE_STRING, &string_buffer,
				   NULL);
	    
	    if (error == NULL) {
	        char** connect_result = NULL;
            connect_result = g_strsplit(string_buffer, ",", 2);
	        //g_print("Connect device: %s, Connect service: %s\n", connect_result[0], connect_result[1]);
	        connect_service = g_strdup(connect_result[1]);
	        g_strfreev(connect_result);
            g_free (string_buffer);
        
        } else {
            g_printerr ("Error: %s\n", error->message);
            g_error_free (error);
        }

	    
    } 
    /* Is a WAN Interface Config service? */
    else if(g_strcmp0 (service_type, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1") == 0)
    {
        router.wan_device = child->data;
        
        /* Start data rate timer */
        g_timeout_add(1000, update_data_rate_cb, NULL);
    } 
    /* Is a WAN Connection service? */
    else if(g_strcmp0 (service_id, connect_service) == 0 || 
            g_strcmp0 (service_type, "urn:schemas-upnp-org:service:WANIPConnection:1") == 0)
    {
        
        router.wan_service = child->data;
        
        gupnp_service_proxy_set_subscribed(child->data, TRUE);
        
        get_conn_status(NULL);
        get_external_ip(NULL);
        
        g_print("   => Subscribed to WANIPConn events\n");
        
        gupnp_service_proxy_add_notify (child->data,
                                        "PortMappingNumberOfEntries",
                                        G_TYPE_UINT,
                                        service_proxy_event_cb,
                                        NULL);
        gupnp_service_proxy_add_notify (child->data,
                                        "ExternalIPAddress",
                                        G_TYPE_STRING,
                                        service_proxy_event_cb,
                                        NULL);
        gupnp_service_proxy_add_notify (child->data,
                                        "ConnectionStatus",
                                        G_TYPE_STRING,
                                        service_proxy_event_cb,
                                        NULL);
    }
    else
        g_object_unref (child->data);
        
    child = g_list_delete_link (child, child);
  }
  
  
  /* Enum subdevices */
  child = gupnp_device_info_list_devices(GUPNP_DEVICE_INFO (proxy));
  
  if (g_list_length(child) > 0)
    g_print("\e[1;32mEnum sub-devices (recursive)...\e[0;0m\n");
  
  while( child )
  {

    g_print(" * Sub-Device Available: %s\n",
        gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (child->data)));
    
    /* Recursive pass */
    device_proxy_available_cb (NULL, child->data);
    
    g_object_unref (child->data);
    child = g_list_delete_link (child, child);
  }  
  
}

/* Device unavailable callback */
static void device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                                         GUPnPServiceProxy *proxy,
                                         gpointer           user_data)
{ 
    const char *device_type = NULL;
    
    g_print("==> Device Unavailable: \e[1;31m%s\e[0;0m\n",
            gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy)));
    
    device_type = gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO (proxy));
     
    if(g_strcmp0(device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:1") != 0)
        return;
    
    clear_port_mapping_treeview();    

    gtk_label_set_markup (GTK_LABEL(ui.router_name_label), _("<b>Name:</b>"));
    gtk_widget_set_sensitive(ui.router_name_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(ui.wan_status_label), _("<b>WAN status:</b>"));
    gtk_widget_set_sensitive(ui.wan_status_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(ui.ip_label), _("<b>IP:</b>"));
    gtk_widget_set_sensitive(ui.ip_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(ui.down_rate_label), "");
    gtk_widget_set_sensitive(ui.down_rate_label, FALSE);
    
    gtk_label_set_markup (GTK_LABEL(ui.up_rate_label), "");
    gtk_widget_set_sensitive(ui.up_rate_label, FALSE);
    
    gtk_widget_set_sensitive(ui.router_url_label, FALSE);
    
}

/* Menu About activate callback */
void on_about_activate_cb (GtkMenuItem *menuitem, 
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
	
	gtk_show_about_dialog (GTK_WINDOW(ui.main_window),
             "authors", authors,
		     "artists", artists,
		     "translator-credits", strcmp("translator-credits", translators) ? translators : NULL,
             "comments", _("A simple program to map the ports on the router via UPnP"),
             "copyright", "Copyright © 2009 Daniele Napolitano \"DnaX\"",
             "version", VERSION,
             "website", "http://launchpad.net/upnp-port-mapper",
		     "logo-icon-name", "upnp-port-mapper",
             NULL);
}

/* Initialize GUI and retrive widgets pointers from GladeXML */
static GladeXML* InitGUI()
{
    g_print("* Initializing GUI...\n");
    
    GladeXML* glade_xml; 
    
    glade_xml = glade_xml_new (GLADE_FILE, NULL, NULL);
    g_assert (glade_xml != NULL);
    
    glade_xml_signal_autoconnect (glade_xml);
    
    ui.main_window = glade_xml_get_widget (glade_xml, "window1"); 
    g_assert (ui.main_window != NULL);
    
    ui.treeview = glade_xml_get_widget (glade_xml, "treeview1");
    
    ui.router_name_label = glade_xml_get_widget (glade_xml, "router_name_label");
    ui.router_url_label = glade_xml_get_widget (glade_xml, "router_url_label");
    ui.wan_status_label = glade_xml_get_widget (glade_xml, "wan_status_label");
    ui.ip_label = glade_xml_get_widget (glade_xml, "ip_label");
    ui.down_rate_label = glade_xml_get_widget (glade_xml, "down_rate_label");
    ui.up_rate_label = glade_xml_get_widget (glade_xml, "up_rate_label");
    ui.router_url_eventbox = glade_xml_get_widget (glade_xml, "router_url_eventbox");
    
    g_signal_connect(G_OBJECT(glade_xml_get_widget (glade_xml, "button_remove")), "clicked",
                         G_CALLBACK(on_button_remove_clicked),
                         NULL);
    
    g_signal_connect(G_OBJECT(glade_xml_get_widget (glade_xml, "about_menuitem")), "activate",
                         G_CALLBACK(on_about_activate_cb),
                         NULL);
    
    setup_treeview();
    
    g_print("* Showing GUI...\n");
    gtk_widget_show_all(ui.main_window);   
    
    return glade_xml;
}

/* Main... :) */
int main (int argc, char **argv)
{
  GUPnPContext *context;
  GUPnPControlPoint *cp;
  
  /* Required initialisation */
  g_thread_init (NULL);
  g_type_init ();
  
  
  #ifdef ENABLE_NLS
    /* Internationalization */
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
  #endif /* ENABLE_NLS */
  
  gtk_init (&argc, &argv);
  
  gtk_window_set_default_icon_name("upnp-port-mapper");
  
  g_set_application_name("UPnP Port Mapper");
    
  /* Create a new GUPnP Context.  By here we are using the default GLib main
     context, and connecting to the current machine's default IP on an
     automatically generated port. */
  context = gupnp_context_new (NULL, NULL, 0, NULL);

  /* Create a Control Point targeting WAN IP Connection services */
  cp = gupnp_control_point_new
    (context, "upnp:rootdevice");

  /* The service-proxy-available signal is emitted when any services which match
     our target are found, so connect to it */
  g_signal_connect (cp,
		    "device-proxy-available",
		    G_CALLBACK (device_proxy_available_cb),
		    glade);
  
  g_signal_connect (cp,
		    "device-proxy-unavailable",
		    G_CALLBACK (device_proxy_unavailable_cb),
		    glade);

  /* Tell the Control Point to start searching */
  gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);
  g_print("* UPnP Resource discovery started...\n");
  
  glade = InitGUI();
  
  /* Enter the main loop. This will start the search and result in callbacks to
     service_proxy_available_cb. */
  gtk_main();

  /* Clean up */
  g_object_unref (cp);
  g_object_unref (context);
  
  return 0;
}

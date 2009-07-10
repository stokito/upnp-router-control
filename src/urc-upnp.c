/* urc-upnp.c
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

#include <glib.h>
#include <libgupnp/gupnp-control-point.h>

#include "urc-gui.h"
#include "urc-upnp.h"

typedef struct
{
    GUPnPDeviceInfo *main_device;
    gchar* friendly_name;
    gchar* brand;
    gchar* brand_website;
    gchar* model_name;
    gchar* model_number;
    gchar* http_address;
    
    guint PortMappingNumberOfEntries;
    
    gchar* external_ip;
    
    gboolean connected;
    
    guint port_request_timeout;
    
    GUPnPServiceProxy *wan_service;
    GUPnPServiceProxy *wan_device;

} RouterInfo;


typedef struct
{
    gboolean enabled;
    gchar*   description;
    gchar*   protocol;
    guint    internal_port;
    guint    external_port;
    gchar*   internal_host;
    gchar*   remote_host;
    guint    lease_time;

} PortForwardInfo;

enum Protocols
{
    TCP, UDP
};

static gboolean opt_debug = FALSE;

void delete_port_mapped(GUPnPServiceProxy *wan_service, const gchar *protocol, const guint external_port, const gchar *remote_host)
{
    GError *error = NULL;
    
    gupnp_service_proxy_send_action (wan_service,
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

static PortForwardInfo* get_mapped_port(RouterInfo *router, guint index)
{

    GError *error = NULL;    
    PortForwardInfo* port;
    
    port = g_malloc( sizeof(PortForwardInfo) );    
    
    gupnp_service_proxy_send_action (router->wan_service,
				   /* Action name and error location */
				   "GetGenericPortMappingEntry", &error,
				   /* IN args */
				   "NewPortMappingIndex",
				   G_TYPE_UINT, index,
				   NULL,
				   /* OUT args */
				   "NewRemoteHost",
				   G_TYPE_STRING, &port->remote_host,
				   "NewExternalPort",
				   G_TYPE_UINT, &port->external_port,
				   "NewProtocol",
				   G_TYPE_STRING, &port->protocol,
				   "NewInternalPort",
				   G_TYPE_UINT, &port->internal_port,
				   "NewInternalClient",
				   G_TYPE_STRING, &port->internal_host,
				   "NewEnabled",
				   G_TYPE_BOOLEAN, &port->enabled,
				   "NewPortMappingDescription",
				   G_TYPE_STRING, &port->description,
				   "NewLeaseDuration",
				   G_TYPE_UINT, &port->lease_time,
				   NULL);
    
    if (error) {
        
        if(error->code != 713) {
            g_printerr ("Error %d: %s\n", error->code, error->message);
            g_error_free (error);
        }
        return NULL;
    }
    
    return port;
    
}


void discovery_mapped_ports_list(RouterInfo *router)
{
    PortForwardInfo* port_info;
    guint index = 0;
    
    /* Clear previus entries */
    gui_clear_ports_list_treeview();
    
    g_print("\e[1;32m==> Getting mapped ports list...\e[0;0m\n");
    
    while( (port_info = get_mapped_port(router, index)) != NULL )
    {
        
        g_print (" * %s [%s]\n", port_info->description, port_info->enabled ? "X" : " " );
	        
	    g_print ("   RemoteIP: %s, ExtPort: %d %s, IntPort: %d, IntIP: %s\n",
	             port_info->remote_host,
	             port_info->external_port,
	             port_info->protocol,
	             port_info->internal_port,
	             port_info->internal_host
	             );
            
        if(port_info->external_port > 0) {
                gui_add_mapped_port(port_info->enabled, port_info->description, port_info->protocol, port_info->internal_port,
                                    port_info->external_port, port_info->internal_host, port_info->remote_host);
        }
        
        g_free (port_info->description);
        g_free (port_info->protocol);
        g_free (port_info->internal_host);
        g_free (port_info->remote_host);
        g_free (port_info);
        
        index++;
    }
        
}

static gboolean get_mapped_ports_list_timeout(gpointer data)
{
    discovery_mapped_ports_list( (RouterInfo *) data );
    
    return FALSE;
}

/* Retrive ports mapped and populate the treeview */
static void get_mapped_ports_list(RouterInfo *router)
{
    
    PortForwardInfo* port_info;
    int i;
    
    /* Clear previus entries */
    gui_clear_ports_list_treeview();
    
    if(router->PortMappingNumberOfEntries != 0)
        g_print("\e[1;32m==> List port mapped (%d)\e[0;0m\n", router->PortMappingNumberOfEntries);
    
    for(i = 0; i < router->PortMappingNumberOfEntries; i++)
    {
        port_info = get_mapped_port(router, i);
	    
	    if (port_info != NULL)
	    {
	        g_print (" * %s [%s]\n", port_info->description, port_info->enabled ? "X" : " " );
	        
	        g_print ("   RemoteIP: %s, ExtPort: %d %s, IntPort: %d, IntIP: %s\n",
	                 port_info->remote_host,
	                 port_info->external_port,
	                 port_info->protocol,
	                 port_info->internal_port,
	                 port_info->internal_host
	                 );
            
            if(port_info->external_port > 0)
            {
                gui_add_mapped_port(port_info->enabled, port_info->description, port_info->protocol, port_info->internal_port,
                                    port_info->external_port, port_info->internal_host, port_info->remote_host);
            }
            g_free (port_info->description);
            g_free (port_info->protocol);
            g_free (port_info->internal_host);
            g_free (port_info->remote_host);
            g_free (port_info);
        }
    }
}

/* Retrive connection infos: connection status, uptime and last error. */
static gboolean get_conn_status (RouterInfo *router)
{
    GError *error = NULL;
    gchar* conn_status;
    gchar* last_conn_error;
    guint  uptime = 0;
    
    g_print("\e[36mRequest for connection status info... ");
    
    /* download speed */
    gupnp_service_proxy_send_action (router->wan_device,
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
        if(g_strcmp0("Connected", conn_status) == 0)
            router->connected = TRUE;
        else
            router->connected = FALSE;
        
        g_print("\e[32msuccessful\e[0;0m\n");
        g_print("\e[36mConnection info:\e[0m Status: %s, Uptime: %i sec.\n", conn_status, uptime);
        
        gui_set_conn_status(conn_status);
        
        if(g_strcmp0("ERROR_NONE", last_conn_error) != 0)
            g_print("\e[33mLast connection error:\e[0m %s\n", last_conn_error);
        
        return TRUE;        
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");
        
        gui_disable_conn_status();  
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
}

/* Retrive download and upload speeds */
static gboolean update_data_rate_cb (gpointer data)
{
    GError *error = NULL;
    GUPnPServiceProxy *wan_device;
    
    static guint old_total_bytes_received = 0;
    guint current_total_bytes_received;
    double data_rate_down;
    
    static guint old_total_bytes_sent = 0;
    guint current_total_bytes_sent;
    double data_rate_up;
    
    wan_device = (GUPnPServiceProxy *) data;
    
    /* download speed */
    gupnp_service_proxy_send_action (wan_device,
				   /* Action name and error location */
				   "GetTotalBytesReceived", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesReceived",
				   G_TYPE_UINT, &current_total_bytes_received,
				   NULL);
	    
	if (error == NULL) {	    
	       
        if(old_total_bytes_received == 0)
	        data_rate_down = 0.0;
	    else
	        data_rate_down = (current_total_bytes_received - old_total_bytes_received) / 1024.0;
	        
        gui_set_download_speed(data_rate_down);
        
        old_total_bytes_received = current_total_bytes_received;
        
    } else {
        gui_disable_download_speed();
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
    
    /* upload speed */
    gupnp_service_proxy_send_action (wan_device,
				   /* Action name and error location */
				   "GetTotalBytesSent", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesSent",
				   G_TYPE_UINT, &current_total_bytes_sent,
				   NULL);
	    
	if (error == NULL) {
	    
	    if(old_total_bytes_sent == 0)
	        data_rate_up = 0.0;
	    else
	        data_rate_up = (current_total_bytes_sent - old_total_bytes_sent) / 1024.0;
	        
        gui_set_upload_speed(data_rate_up);
        
        old_total_bytes_sent = current_total_bytes_sent;
        
    } else {
        gui_disable_upload_speed();

        g_printerr ("\e[1;31mError:\e[1;0m %s\e[0m\n", error->message);
        g_error_free (error);
        return FALSE;
    }
        
    return TRUE;
}

/* Retrive external IP address */
static gboolean get_external_ip (RouterInfo *router)
{
    GError *error = NULL;
    gchar* ext_ip_addr;
    
    g_print("\e[36mRequest for external IP address... ");
    
    /* download speed */
    gupnp_service_proxy_send_action (router->wan_device,
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
        router->external_ip = g_strdup(ext_ip_addr);
        
        g_print("\e[32msuccessful \e[0m[%s]\n", router->external_ip);
        
        if( g_strcmp0(router->external_ip, "0.0.0.0") == 0 )
            gui_set_ext_ip (NULL);
        else
            gui_set_ext_ip (router->external_ip);
        
        return TRUE;        
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");
        
        gui_disable_ext_ip ();
        
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
}

/* Service event callback */
static void service_proxy_event_cb (GUPnPServiceProxy *proxy,
                                        const char *variable,
                                        GValue *value,
                                        gpointer data)
{
    
    RouterInfo *router;
    
    router = (RouterInfo *) data;
    
    /* Numebr of port mapped entries */
    if(g_strcmp0("PortMappingNumberOfEntries", variable) == 0)
    {
        g_source_remove(router->port_request_timeout);
        
        router->PortMappingNumberOfEntries = g_value_get_uint(value);
        g_print("\e[33mEvent:\e[0;0m Ports mapped: %d\n", router->PortMappingNumberOfEntries);
        
        get_mapped_ports_list(router);
    }
    /* Got external IP */
    else if(g_strcmp0("ExternalIPAddress", variable) == 0)
    {
        router->external_ip = g_strdup(g_value_get_string(value));
        g_print("\e[33mEvent:\e[0;0m External IP: %s\n", router->external_ip);
        
        gui_set_ext_ip (router->external_ip);
    }
    /* WAN connection status changed */
    else if(g_strcmp0("ConnectionStatus", variable) == 0)
    {        
        gui_set_conn_status (g_value_get_string(value));
        
        if(g_strcmp0("Connected", g_value_get_string(value)) == 0)
            router->connected = TRUE;
        else
            router->connected = FALSE;
        g_print("\e[33mEvent:\e[0;0m Connection status: %s\n", g_value_get_string(value) );
    }
}


/* Device available callback */
static void device_proxy_available_cb (GUPnPControlPoint *cp,
                                       GUPnPServiceProxy *proxy,
                                       RouterInfo        *router)
{
    GList *child;
    GError *error = NULL;

    char *string_buffer = NULL;
    static char *connect_service = NULL;
    const char *service_type = NULL;
    const char *service_id = NULL;
    const char *device_type = NULL;
    
    static int level = 0;
    int i;
    
    if(opt_debug)
    {
        if(level == 0)
        {
            for(i = 0; i < level; i++)
                g_print("    ");
            
            g_print("==> Device Available: \e[31m%s\e[0;0m\n",
                gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy)));
        }
    }
   
    device_type = gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO (proxy));
  
    /* Is a gateway? */
    if(g_strcmp0(device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:1") == 0)
    {
        router->main_device = GUPNP_DEVICE_INFO (proxy);
        router->friendly_name = gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy));
        router->brand = gupnp_device_info_get_manufacturer(GUPNP_DEVICE_INFO (proxy));
        router->http_address = gupnp_device_info_get_presentation_url(GUPNP_DEVICE_INFO (proxy));
        router->brand_website = gupnp_device_info_get_manufacturer_url(GUPNP_DEVICE_INFO (proxy));
        router->model_name = gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO (proxy));
        router->model_number = gupnp_device_info_get_model_number(GUPNP_DEVICE_INFO (proxy));
        
        gui_set_router_info (router->friendly_name,
                             router->http_address,
                             router->brand,
                             router->brand_website,
                             router->model_name,
                             router->model_number);

    }

  
    /* Enum services */
    child = gupnp_device_info_list_services(GUPNP_DEVICE_INFO (proxy));
    
    if(opt_debug)
    {
        for(i = 0; i < level; i++)
            g_print("    ");
  
        if (g_list_length(child) > 0)
            g_print("    \e[1;32mEnum services...\e[0;0m\n");
    }
  
    while( child )
    {
    
        service_type = gupnp_service_info_get_service_type(GUPNP_SERVICE_INFO (child->data));
        service_id = gupnp_service_info_get_id(GUPNP_SERVICE_INFO (child->data));    
        
        if(opt_debug)
        {
            for(i = 0; i < level; i++)
                g_print("    ");
    
            g_print("      \e[32mService:\e[0m %s\n", service_id );
        }
    
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
            router->wan_device = child->data;
        
            /* Start data rate timer */
            g_timeout_add_seconds(1, update_data_rate_cb, router->wan_device);
            
            /* Start port request timer */
            router->port_request_timeout = g_timeout_add_seconds(5, get_mapped_ports_list_timeout, router);
        } 
        /* Is a WAN Connection service? */
        else if(g_strcmp0 (service_id, connect_service) == 0 || 
                g_strcmp0 (service_type, "urn:schemas-upnp-org:service:WANIPConnection:1") == 0)
        {
        
            router->wan_service = child->data;
            
            gui_set_button_delete_callback_data(router->wan_device);
        
            gupnp_service_proxy_set_subscribed(child->data, TRUE);
            
            if(opt_debug)
            {
                
                for(i = 0; i < level; i++)
                    g_print("    ");
        
                g_print("      \e[32m** Subscribed to WANIPConn events\e[0m\n");
            }
        
            get_external_ip(router);
            get_conn_status(router);
        
            gupnp_service_proxy_add_notify (child->data,
                                            "PortMappingNumberOfEntries",
                                            G_TYPE_UINT,
                                            service_proxy_event_cb,
                                            router);
            gupnp_service_proxy_add_notify (child->data,
                                            "ExternalIPAddress",
                                            G_TYPE_STRING,
                                            service_proxy_event_cb,
                                            router);
            gupnp_service_proxy_add_notify (child->data,
                                            "ConnectionStatus",
                                            G_TYPE_STRING,
                                            service_proxy_event_cb,
                                            router);
                                        
        }
        else
            g_object_unref (child->data);
        
        child = g_list_delete_link (child, child);
    }
  
  
    /* Enum subdevices */
    child = gupnp_device_info_list_devices(GUPNP_DEVICE_INFO (proxy));
  
    if (opt_debug && g_list_length(child) > 0)
    {
        for(i = 0; i < level; i++)
            g_print("    ");
          
        g_print("    \e[1;32mEnum sub-devices...\e[0;0m\n");
    }
  
    while( child )
    {
        if (opt_debug)
        {
            for(i = 0; i < level; i++)
                g_print("    ");
            g_print("      \e[32mSub-Device: \e[31m%s\e[0m\n",
                gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (child->data)));
            level = level + 1;
        }
        /* Recursive pass */
        device_proxy_available_cb (NULL, child->data, router);
        
        if (opt_debug) level = level - 1;
    
        g_object_unref (child->data);
        child = g_list_delete_link (child, child);
    }  
  
}

/* Device unavailable callback */
static void device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                                         GUPnPServiceProxy *proxy,
                                         RouterInfo        *router)
{ 
    const char *device_type = NULL;
    
    if (opt_debug)
        g_print("==> Device Unavailable: \e[31m%s\e[0;0m\n",
            gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO (proxy)));
    
    device_type = gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO (proxy));
     
    if(g_strcmp0(device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:1") != 0)
        return;
    
    gui_disable();
    
}


gboolean upnp_init(const char *host_ip, const gboolean debug)
{
    GUPnPContext *context;
    GUPnPControlPoint *cp;
    GError *error = NULL;
    RouterInfo* router;
    
    opt_debug = debug;
    
    g_print("* Starting UPnP Resource discovery... ");
    
    router = g_malloc( sizeof(RouterInfo) );
  
    /* Create a new GUPnP Context.  By here we are using the default GLib main
     context, and connecting to the current machine's default IP on an
     automatically generated port. */
    context = gupnp_context_new (NULL, host_ip, 0, &error);
    
    if(error != NULL)
    {
        g_error("Error: %s", error->message);
        g_error_free (error);
        return FALSE;
    }

    /* Create a Control Point targeting WAN IP Connection services */
    cp = gupnp_control_point_new
    (context, "upnp:rootdevice");

    /* The service-proxy-available signal is emitted when any services which match
     our target are found, so connect to it */
    g_signal_connect (cp,
		    "device-proxy-available",
		    G_CALLBACK (device_proxy_available_cb),
		    router);
  
    g_signal_connect (cp,
		    "device-proxy-unavailable",
		    G_CALLBACK (device_proxy_unavailable_cb),
		    router);

    /* Tell the Control Point to start searching */
    gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);
    
    g_print("done\n");
    
    return TRUE;
}
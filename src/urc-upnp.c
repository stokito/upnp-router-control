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

#include "config.h"

#include <string.h>
#include <glib.h>
#include <libgupnp/gupnp.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#include "urc-gui.h"
#include "urc-upnp.h"

extern gboolean opt_debug;
extern char* opt_bindif;
extern guint opt_bindport;


static const gchar* client_ip = NULL;
GUPnPContextManager *context_mngr = NULL;

const gchar* get_client_ip()
{
	return client_ip;
}

gboolean delete_port_mapped(GUPnPServiceProxy *wan_service, const gchar *protocol, const guint external_port, const gchar *remote_host, GError **error)
{
    GError *err = NULL;

    gupnp_service_proxy_send_action (wan_service,
				   /* Action name and error location */
				   "DeletePortMapping", &err,
				   /* IN args */
				   "NewRemoteHost",
				   G_TYPE_STRING, remote_host,
				   "NewExternalPort",
				   G_TYPE_UINT, external_port,
				   "NewProtocol",
				   G_TYPE_STRING, protocol,
				   NULL,
				   NULL);

    if (err == NULL) {

	    g_print("\e[36m*** Removed entry:\e[0m Port %d (%s)\n", external_port, protocol);

	    error = NULL;
	    return TRUE;

    } else {

        g_printerr ("\e[31m[EE]\e[0m DeletePortMapping: %s (%i)\n", err->message, err->code);

        *error = g_error_copy(err);
        g_error_free (err);

        return FALSE;
    }
}

gboolean add_port_mapping(GUPnPServiceProxy *wan_service, PortForwardInfo* port_info, GError **error)
{
    GError *err = NULL;

    gupnp_service_proxy_send_action (wan_service,
				   /* Action name and error location */
				   "AddPortMapping", &err,
				   /* IN args */
				   "NewRemoteHost",
				   G_TYPE_STRING, port_info->remote_host,
				   "NewExternalPort",
				   G_TYPE_UINT, port_info->external_port,
				   "NewProtocol",
				   G_TYPE_STRING, port_info->protocol,
				   "NewInternalPort",
				   G_TYPE_UINT, port_info->internal_port,
				   "NewInternalClient",
				   G_TYPE_STRING, port_info->internal_host,
				   "NewEnabled",
				   G_TYPE_BOOLEAN, port_info->enabled,
				   "NewPortMappingDescription",
				   G_TYPE_STRING, port_info->description,
				   "NewLeaseDuration",
				   G_TYPE_UINT, port_info->lease_time,
				   NULL,
				   NULL);

    if (err == NULL) {

	    g_print ("\e[36m*** Added entry: \e[0m%s\n", port_info->description );
	    g_print ("    RemoteIP: %s, ExtPort: %d %s, IntPort: %d, IntIP: %s\n",
	                 port_info->remote_host,
	                 port_info->external_port,
	                 port_info->protocol,
	                 port_info->internal_port,
	                 port_info->internal_host
	                 );

	    GSList *port_list = NULL;
	    port_list = g_slist_prepend(port_list, port_info);

	    gui_add_mapped_ports(port_list);

	    g_slist_free(port_list);

	    error = NULL;
	    return TRUE;

    } else {
        g_printerr ("\e[31m[EE]\e[0m AddPortMapping: %s (%i)\n", err->message, err->code);

        *error = g_error_copy(err);
        g_error_free (err);

        return FALSE;
    }
}

static PortForwardInfo* get_mapped_port(RouterInfo *router, guint index)
{

    GError *error = NULL;
    PortForwardInfo* port;

    port = g_malloc( sizeof(PortForwardInfo) );

    gupnp_service_proxy_send_action (router->wan_conn_service,
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

        // error 713: end of ports array
        // error 402: invalid args
        if(error->code != 713 && error->code != 402) {
            g_printerr ("\e[31m[EE]\e[0m GetGenericPortMappingEntry: %s (%i)\n", error->message, error->code);
            g_error_free (error);
        }
        return NULL;
    }

    return port;

}

/* Retrive ports mapped and populate the treeview */
void discovery_mapped_ports_list(RouterInfo *router)
{
    PortForwardInfo* port_info;
    guint index = 0;
    GSList *port_list = NULL;
    GSList *iter;

    g_print("\e[1;32m==> Getting mapped ports list...\e[0;0m\n");

    while( (port_info = get_mapped_port(router, index)) != NULL )
    {

        g_print (" * %s [%s]\n", port_info->description, port_info->enabled ? "enabled" : "disabled" );

	    g_print ("   ExtPort: %d %s, IntPort: %d, IntIP: %s, RemoteIP: %s\n",
	             port_info->external_port,
	             port_info->protocol,
	             port_info->internal_port,
	             port_info->internal_host,
	             port_info->remote_host
	             );

        if(port_info->external_port > 0)
            port_list = g_slist_prepend(port_list, port_info);

        index++;
    }

    /* Clear previus entries */
    gui_clear_ports_list_treeview();

    /* Update GUI treeview */
    gui_add_mapped_ports(port_list);

    /* Destroy the list */
    iter = port_list;
    while(iter)
    {
        port_info = (PortForwardInfo*) iter->data;

        g_free (port_info->description);
        g_free (port_info->protocol);
        g_free (port_info->internal_host);
        g_free (port_info->remote_host);
        g_free (port_info);

        iter = g_slist_next (iter);
    }
    g_slist_free (port_list);
}

/* Retrive connection infos: connection status, uptime and last error. */
gboolean get_conn_status (RouterInfo *router)
{
    GError *error = NULL;
    gchar* conn_status;
    gchar* last_conn_error;
    guint  uptime = 0;

    g_print("\e[36mRequest for connection status info... ");

    gupnp_service_proxy_send_action (router->wan_conn_service,
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

        g_printerr ("\e[31m[EE]\e[0m GetStatusInfo: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        return FALSE;
    }
}

/* Retrive download and upload speeds */
static gboolean update_data_rate_cb (gpointer data)
{
    GError *error = NULL;
    GUPnPServiceProxy *wan_device;
    GTimeVal begin_time;
    GTimeVal end_time;
    gdouble duration_secs;

    static guint old_total_bytes_received = 0;
    guint current_total_bytes_received;
    double data_rate_down;

    static guint old_total_bytes_sent = 0;
    guint current_total_bytes_sent;
    double data_rate_up;

    wan_device = ((RouterInfo *) data)->wan_common_ifc;

    /* download speed */
    g_get_current_time (&begin_time);
    gupnp_service_proxy_send_action (wan_device,
				   /* Action name and error location */
				   "GetTotalBytesReceived", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesReceived",
				   G_TYPE_UINT, &current_total_bytes_received,
				   NULL);

	g_get_current_time (&end_time);

	if (error == NULL) {

        if(old_total_bytes_received == 0)
	        data_rate_down = 0.0;
	    else {

            // UPnP query time
            duration_secs = 1 + (((double)end_time.tv_sec - begin_time.tv_sec) * G_USEC_PER_SEC +
                                        (end_time.tv_usec - begin_time.tv_usec)) / G_USEC_PER_SEC;

	        if(opt_debug)
	            g_print("\e[34mGetTotalBytesReceived() duration: %fs\e[0m\n", duration_secs);

	        data_rate_down = ((current_total_bytes_received - old_total_bytes_received) / (duration_secs)  ) / 1024.0;
	    }

        gui_set_download_speed(data_rate_down);
        gui_set_total_received(current_total_bytes_received);

        old_total_bytes_received = current_total_bytes_received;

    } else {
        gui_disable_download_speed();
        gui_disable_total_received();

        g_printerr ("\e[31m[EE]\e[0m GetTotalBytesReceived: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        error = NULL;
    }

    /* upload speed */
    g_get_current_time (&begin_time);
    gupnp_service_proxy_send_action (wan_device,
				   /* Action name and error location */
				   "GetTotalBytesSent", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewTotalBytesSent",
				   G_TYPE_UINT, &current_total_bytes_sent,
				   NULL);

	g_get_current_time (&end_time);

	if (error == NULL) {

	    if(old_total_bytes_sent == 0)
	        data_rate_up = 0.0;
	    else {

	        // UPnP query time
	        duration_secs = 1 + (((double)end_time.tv_sec - begin_time.tv_sec) * G_USEC_PER_SEC +
                                        (end_time.tv_usec - begin_time.tv_usec)) / G_USEC_PER_SEC;

            if(opt_debug)
	            g_print("\e[34mGetTotalBytesSent() duration:     %fs\e[0m\n", duration_secs);

	        data_rate_up = (current_total_bytes_sent - old_total_bytes_sent) / (duration_secs) / 1024.0;
	    }
        gui_set_upload_speed(data_rate_up);
        gui_set_total_sent(current_total_bytes_sent);

        old_total_bytes_sent = current_total_bytes_sent;

    } else {
        gui_disable_upload_speed();
        gui_disable_total_sent();

        g_printerr ("\e[31m[EE]\e[0m GetTotalBytesSent: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    gui_update_graph();

    ((RouterInfo *) data)->data_rate_timer = g_timeout_add_full(G_PRIORITY_HIGH, 1000, update_data_rate_cb, data, NULL);

    return FALSE;
}


/* Retrive external IP address */
gboolean get_external_ip (RouterInfo *router)
{
    GError *error = NULL;
    gchar *ext_ip_addr = NULL;

    g_print("\e[36mRequest for external IP address... ");

    gupnp_service_proxy_send_action (router->wan_conn_service,
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
        if(router->external_ip != NULL)
            g_free(router->external_ip);

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

        g_printerr ("\e[31m[EE]\e[0m GetExternalIPAddress: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        return FALSE;
    }
}

/* Retrive RSIP and NAT availability */
gboolean get_nat_rsip_status (RouterInfo *router)
{
    GError *error = NULL;

    g_print("\e[36mRequest for NAT and RSIP availability... ");

    /* download speed */
    gupnp_service_proxy_send_action (router->wan_conn_service,
				   /* Action name and error location */
				   "GetNATRSIPStatus", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewRSIPAvailable",
				   G_TYPE_BOOLEAN, &router->rsip_available,
				   "NewNATEnabled",
				   G_TYPE_BOOLEAN, &router->nat_enabled,
				   NULL);

    if (error == NULL)
    {
        g_print("\e[32msuccessful \e[0m[RSIP=%s, NAT=%s]\n", router->rsip_available == TRUE ? "yes" : "no", router->nat_enabled == TRUE ? "yes" : "no" );

        return TRUE;
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");

        g_printerr ("\e[31m[EE]\e[0m GetNATRSIPStatus: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        return FALSE;
    }
}

/* Retrive WAN link properties */
gboolean get_wan_link_properties (RouterInfo *router)
{
    GError *error = NULL;

    gchar *access_type, *physical_link_status;
    guint upstream_max_bitrate, downstream_max_bitrate;

    g_print("\e[36mRequest for WAN link properties... ");

    /* download speed */
    gupnp_service_proxy_send_action (router->wan_common_ifc,
				   /* Action name and error location */
				   "GetCommonLinkProperties", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewWANAccessType",
				   G_TYPE_STRING, &access_type,
				   "NewLayer1UpstreamMaxBitRate",
				   G_TYPE_UINT, &upstream_max_bitrate,
				   "NewLayer1DownstreamMaxBitRate",
				   G_TYPE_UINT, &downstream_max_bitrate,
				   "NewPhysicalLinkStatus",
				   G_TYPE_STRING, &physical_link_status,
				   NULL);

    if (error == NULL)
    {
        g_print("\e[32msuccessful\e[0m\n");
        g_print("\e[36mWAN link properties:\e[0m access_type=%s, link_status=%s, max_up=%u, max_down=%u\n",
                     access_type, physical_link_status, upstream_max_bitrate, downstream_max_bitrate );

        return TRUE;
    }
    else
    {
        g_print("\e[1;31mfailed\e[0;0m\n");

        g_printerr ("\e[31m[EE]\e[0m GetCommonLinkProperties: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        return FALSE;
    }
}

static gchar* get_default_connection_service (GUPnPServiceProxy *proxy, int level)
{
    gchar *string_buffer = NULL;
    gchar *connect_service = NULL;
    GError *error = NULL;
    int i;

    if(opt_debug)
    {
        for(i = 0; i < level; i++)
            g_print("    ");

        g_print("      \e[32m** Getting DefaultConnectionService...\e[0m\n");
    }

    gupnp_service_proxy_send_action (proxy,
				   /* Action name and error location */
				   "GetDefaultConnectionService", &error,
				   /* IN args */
				   NULL,
				   /* OUT args */
				   "NewDefaultConnectionService",
				   G_TYPE_STRING, &string_buffer,
				   NULL);

    if (error == NULL) {

        if(string_buffer != NULL && g_strcmp0(string_buffer, "") != 0)
        {
            char** connect_result = NULL;
            connect_result = g_strsplit(string_buffer, ",", 2);

            if(opt_debug)
            {
                for(i = 0; i < level; i++)
                    g_print("    ");

                g_print("      \e[32mConnectionService:\e[0m %s\n", string_buffer);
            }
            //connect_device = g_strdup(connect_result[0]);
            connect_service = g_strdup(connect_result[1]);
            g_strfreev(connect_result);

        } else {
            g_print("\e[0;31m[WW]\e[0m GetDefaultConnectionService: empty\e[0;0m\n");
            //connect_device = NULL;
            connect_service = NULL;
        }
        // yes, can free NULL string
        g_free (string_buffer);

    } else {
        g_printerr ("\e[31m[EE]\e[0m GetDefaultConnectionService: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    return connect_service;
}

static gboolean get_mapped_ports_list_timeout(gpointer data)
{
    discovery_mapped_ports_list( (RouterInfo *) data );

    ((RouterInfo *) data)->port_request_timeout = g_timeout_add_seconds(10, get_mapped_ports_list_timeout, data);

    return FALSE;
}

static gboolean get_connection_status_timeout(gpointer data)
{
    get_conn_status( (RouterInfo *) data );

    ((RouterInfo *) data)->connection_status_timeout = g_timeout_add_seconds(10, get_connection_status_timeout, data);

    return FALSE;
}

static gboolean get_external_ip_timeout(gpointer data)
{
    get_external_ip( (RouterInfo *) data );

    ((RouterInfo *) data)->external_ip_timeout = g_timeout_add_seconds(10, get_external_ip_timeout, data);

    return FALSE;
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
        /* deactivate manual request timer */
        g_source_remove(router->port_request_timeout);

        g_print("\e[33mEvent:\e[0;0m Ports mapped: %d\n", g_value_get_uint(value));

        discovery_mapped_ports_list(router);
    }
    /* Got external IP */
    else if(g_strcmp0("ExternalIPAddress", variable) == 0)
    {
        /* deactivate manual request timer */
        g_source_remove(router->external_ip_timeout);

        if(router->external_ip != NULL)
            g_free(router->external_ip);

        router->external_ip = g_strdup(g_value_get_string(value));
        g_print("\e[33mEvent:\e[0;0m External IP: %s\n", router->external_ip);

        // check if IP is really null (workaround for Netgear DG834)
        if(g_strcmp0(router->external_ip, "0.0.0.0") == 0)
            get_external_ip(router);
        else
            gui_set_ext_ip (router->external_ip);
    }
    /* WAN connection status changed */
    else if(g_strcmp0("ConnectionStatus", variable) == 0)
    {
        /* deactivate manual request timer */
        g_source_remove(router->connection_status_timeout);

        gui_set_conn_status (g_value_get_string(value));

        if(g_strcmp0("Connected", g_value_get_string(value)) == 0)
            router->connected = TRUE;
        else
            router->connected = FALSE;
        g_print("\e[33mEvent:\e[0;0m Connection status: %s\n", g_value_get_string(value) );
    }
    else
        /* Got an unmanaged event */
    	g_print("\e[33mEvent:\e[0;0m %s [Not managed]", variable);
}

#ifdef HAVE_LIBCURL
gpointer download_router_icon (gpointer data)
{
    FILE *local_file;
    gchar *filename;
    CURL *curl_handle;
    CURLcode ret;
    char error_buffer[CURL_ERROR_SIZE];

    if(data == NULL)
        return 0;

    filename = g_strconcat(g_get_tmp_dir(), "/upnp-router-control.icon", NULL);

    g_print("\e[36mDownloading router icon...\e[0;0m\n");

    curl_handle = curl_easy_init();

    if(curl_handle) {

        local_file = fopen(filename, "w");

        curl_easy_setopt(curl_handle, CURLOPT_URL, (char*)data);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, local_file);
        curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
        /* do not download images > 500KiB */
        curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, 512000);
        /* 1 minute of timeout */
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60);

        ret = curl_easy_perform(curl_handle);

        fclose(local_file);

        if(ret != 0) {
            g_print("\e[31m[EE]\e[0m Error downloading icon of the router: %s\n", error_buffer);

        } else {
            g_print("\e[32mRouter icon downloaded\e[0;0m\n");
            gui_set_router_icon(filename);
        }

        curl_easy_cleanup(curl_handle);

    } else {
        g_print("\e[31m[EE]\e[0m Error loading libcurl, will not download router icon");
    }

    g_free(filename);
    g_free(data);

    return 0;
}
#endif /* HAVE_LIBCURL */

static gchar* parse_presentation_url(gchar *presentation_url, const gchar *device_location)
{
    gchar** url_split = NULL;
    gchar *output_url_string;

    if(presentation_url == NULL)
    {
        url_split = g_strsplit(device_location, ":", 0);

        output_url_string = g_strconcat(url_split[0], ":", url_split[1], "/", NULL);
        g_strfreev(url_split);

        if(opt_debug)
            g_print("        Rewrited URL: %s\n", output_url_string);
    }
    /* workaround for urls like "/login" without base url */
    else if(presentation_url != NULL && g_str_has_prefix(presentation_url, "http") == FALSE)
    {

        url_split = g_strsplit(device_location, ":", 0);
        output_url_string = g_strconcat(url_split[0], ":", url_split[1], presentation_url, NULL);
        g_strfreev(url_split);

        g_free(presentation_url);

        if(opt_debug)
            g_print("        Rewrited URL: %s\n", output_url_string);
    }
    else
    {
        output_url_string = presentation_url;
    }

    return output_url_string;
}

/* useful function for debug strings */
static void print_indent(guint8 tabs)
{
    int i;
    
    for(i = 0; i < tabs; i++)
            g_print("    ");
}

/* function to compare UPnP device/service string and check
 * version is at least the one needed */
static int device_service_cmp(const char *devserv1, const char *devserv2, int ver2)
{
    int ver1;
    size_t len;
    int res;

    if(devserv1 == NULL && devserv2 == NULL)
        return 0;
    if(devserv1 == NULL)
        return -1;
    if(devserv2 == NULL)
        return 1;
    len = strlen(devserv2);
    res = memcmp(devserv1, devserv2, len);
    if(res != 0)
        return res;
    ver1 = atoi(devserv1+len);
    if(ver1 >= ver2)
        return 0;
    return -1;
}

/* Device available callback */
static void device_proxy_available_cb (GUPnPControlPoint *cp,
                                       GUPnPServiceProxy *proxy,
                                       RouterInfo        *router)
{
    GList *child;

    static char *connect_service = NULL;
    const char *service_type = NULL;
    const char *service_id = NULL;
    const char *device_type = NULL;

    gchar *router_icon_url, *icon_mime_type;
    int icon_depth, icon_width, icon_height;

    static int level = 0;

    if(level == 0) {
    
        g_print ("==> Device Available: \e[31m%s\e[0;0m\n",
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy)));
    }

    device_type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));

    /* Is an IGD device? */
    if(device_service_cmp (device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:", 1) == 0)
    {
        /* do nothing if there is already a device stored */
        if(router->main_device != NULL)
            return;

        router->main_device = GUPNP_DEVICE_INFO (proxy);
        router->friendly_name = gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy));
        router->brand = gupnp_device_info_get_manufacturer (GUPNP_DEVICE_INFO (proxy));
        router->http_address = gupnp_device_info_get_presentation_url (GUPNP_DEVICE_INFO (proxy));
        router->brand_website = gupnp_device_info_get_manufacturer_url (GUPNP_DEVICE_INFO (proxy));
        router->model_description = gupnp_device_info_get_model_description (GUPNP_DEVICE_INFO (proxy));
        router->model_name = gupnp_device_info_get_model_name (GUPNP_DEVICE_INFO (proxy));
        router->model_number = gupnp_device_info_get_model_number (GUPNP_DEVICE_INFO (proxy));
        router->upc = gupnp_device_info_get_upc (GUPNP_DEVICE_INFO (proxy));
        router->udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));

        router_icon_url = gupnp_device_info_get_icon_url (GUPNP_DEVICE_INFO (proxy),
                                       NULL, -1, -1, -1, FALSE,
                                       &icon_mime_type, &icon_depth,
                                       &icon_width, &icon_height);

        if(opt_debug) {
        
           	g_print ("   Model description: %s\n", router->model_description);
           	g_print ("          Model name: %s\n", router->model_name);
           	g_print ("        Model number: %s\n", router->model_number);
           	g_print ("               Brand: %s\n", router->brand);
           	g_print ("    Presentation URL: %s\n", router->http_address);
           	g_print ("                 UPC: %s\n", router->upc);
           	g_print ("  Unique Device Name: %s\n", router->udn);

           	if(router_icon_url != NULL) {
           	
           	    g_print ("            Icon URL: %s\n", router_icon_url);
                g_print ("      Icon mime/type: %s\n", icon_mime_type);
                g_print ("          Icon depth: %d\n", icon_depth);
                g_print ("          Icon width: %d\n", icon_width);
                g_print ("         Icon height: %d\n", icon_height);
                g_free (icon_mime_type);
            }
        }

        #ifdef HAVE_LIBCURL
        // start the download in a separate thread
        g_thread_new("down_router_ico", download_router_icon, router_icon_url);
        #endif

        router->http_address = parse_presentation_url(router->http_address,
                               gupnp_device_info_get_location(GUPNP_DEVICE_INFO (proxy)));

        /* workaround for empty <friendlyName> property */
        if(g_strcmp0 (router->friendly_name, "") == 0) {
        
            if(g_strcmp0 (router->model_name, "") == 0)
                router->friendly_name = g_strdup (router->model_description);
            else
                router->friendly_name = g_strdup (router->model_name);
        }

        gui_set_router_info (router->friendly_name,
                             router->http_address,
                             router->brand,
                             router->brand_website,
                             router->model_description,
                             router->model_name,
                             router->model_number);

    }
    /* There is only a WANConnectionDevice? */
    else if(device_service_cmp (device_type, "urn:schemas-upnp-org:device:WANConnectionDevice:", 1) == 0 && level == 0 )
    {
    	/* do nothing if there is already a device stored */
        if(router->main_device != NULL)
            return;

    	router->main_device = GUPNP_DEVICE_INFO (proxy);
        router->friendly_name = gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy));
        router->brand = gupnp_device_info_get_manufacturer (GUPNP_DEVICE_INFO (proxy));
        router->http_address = gupnp_device_info_get_presentation_url (GUPNP_DEVICE_INFO (proxy));
        router->brand_website = gupnp_device_info_get_manufacturer_url (GUPNP_DEVICE_INFO (proxy));
        router->model_description = gupnp_device_info_get_model_description (GUPNP_DEVICE_INFO (proxy));
        router->model_name = gupnp_device_info_get_model_name (GUPNP_DEVICE_INFO (proxy));
        router->model_number = gupnp_device_info_get_model_number (GUPNP_DEVICE_INFO (proxy));
        router->upc = gupnp_device_info_get_upc (GUPNP_DEVICE_INFO (proxy));
        router->udn = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));

        if(opt_debug) {
        
           	g_print ("   Model description: %s\n", router->model_description);
           	g_print ("          Model name: %s\n", router->model_name);
           	g_print ("        Model number: %s\n", router->model_number);
           	g_print ("               Brand: %s\n", router->brand);
           	g_print ("    Presentation URL: %s\n", router->http_address);
           	g_print ("                 UPC: %s\n", router->upc);
           	g_print ("  Unique Device Name: %s\n", router->udn);
        }

        router->http_address = parse_presentation_url (router->http_address,
                               gupnp_device_info_get_location(GUPNP_DEVICE_INFO (proxy)));

        /* workaround for empty <friendlyName> property or standard name */
        if(g_strcmp0 (router->friendly_name, "") == 0 || g_strcmp0 (router->friendly_name, "WANConnectionDevice") == 0)
        {
            if(g_strcmp0(router->model_name, "") == 0)
                router->friendly_name = g_strdup (router->model_description);
            else
                router->friendly_name = g_strdup (router->model_name);
        }

        gui_set_router_info (router->friendly_name,
                             router->http_address,
                             router->brand,
                             router->brand_website,
                             router->model_description,
                             router->model_name,
                             router->model_number);
    }

    /* Enum services */
    child = gupnp_device_info_list_services (GUPNP_DEVICE_INFO (proxy));

    if(opt_debug) {
    
        print_indent(level);

        if (g_list_length (child) > 0)
            g_print ("    \e[1;32mEnum services...\e[0;0m\n");
    }

    while( child ) {

        service_type = gupnp_service_info_get_service_type (GUPNP_SERVICE_INFO (child->data));
        service_id = gupnp_service_info_get_id (GUPNP_SERVICE_INFO (child->data));

        if(opt_debug) {
        
            print_indent (level);
            g_print("      \e[32mService:\e[0m %s\n", service_id );

            print_indent (level);
            g_print("         Type: %s\n", service_type );


            /* Introspect current service */
            const GList *actions_list;
            GUPnPServiceIntrospection *service_introspect;

            service_introspect = gupnp_service_info_get_introspection (child->data, NULL);

            if(service_introspect != NULL) {

                actions_list = gupnp_service_introspection_list_action_names (service_introspect);

                if(actions_list != NULL) {
                
                    do {
                        print_indent(level);
                        g_print("               > %s\n", (gchar*) actions_list->data);

                    } while ((actions_list = actions_list->next));
                }
            }
        }

        /* Is a IP forwarding service? */
        if( (device_service_cmp (service_type, "urn:schemas-upnp-org:service:Layer3Forwarding:", 1) == 0) ||
        	(device_service_cmp (service_type, "urn:schemas-upnp-org:service:L3Forwarding:", 1) == 0) )
        {

            if(connect_service != NULL)
                g_free (connect_service);

            connect_service = get_default_connection_service (child->data, level);

        }
        /* Is a WAN IFC service? */
        else if(device_service_cmp (service_type, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:", 1) == 0)
        {
            router->wan_common_ifc = child->data;

            /* Get common WAN link properties */
            get_wan_link_properties (router);

            /* Start data rate timer */
            update_data_rate_cb (router);

        }
        /* Is a WAN IP Connection service or other? */
        else if(device_service_cmp (service_type, "urn:schemas-upnp-org:service:WANIPConnection:", 1) == 0)
        {

            router->wan_conn_service = child->data;

            gui_set_ports_buttons_callback_data (router->wan_conn_service);

            gui_set_refresh_callback_data (router);

            if(opt_debug) {
            
                print_indent (level);

        		char **str = NULL;

                str = g_strsplit (service_id, ":", 0);
                g_print ("      \e[32m** Subscribed to %s events\e[0m\n", str[g_strv_length (str)-1]);
                g_strfreev (str);
            }

            /* Get external IP */
            get_external_ip (router);
            /* Get connection status info */
            get_conn_status (router);
            /* Get RSIP and NAT status */
            get_nat_rsip_status (router);

        	/* Subscribe to events */
        	gupnp_service_proxy_set_subscribed (child->data, TRUE);

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

            /* Start port request timeout at 5 sec */
            router->port_request_timeout = g_timeout_add_seconds (5, get_mapped_ports_list_timeout, router);

            /* Start connection status request timeout at 6 sec */
            router->connection_status_timeout = g_timeout_add_seconds (6, get_connection_status_timeout, router);

            /* Start external IP request timeout at 7 sec */
            router->external_ip_timeout = g_timeout_add_seconds (7, get_external_ip_timeout, router);

        }
        else
            g_object_unref (child->data);

        child = g_list_delete_link (child, child);
    }


    /* Enum subdevices */
    child = gupnp_device_info_list_devices (GUPNP_DEVICE_INFO (proxy));

    if (opt_debug && g_list_length (child) > 0) {
    
        print_indent (level);
        g_print ("    \e[1;32mEnum sub-devices...\e[0;0m\n");
    }

    while (child) {
    
        if (opt_debug) {
        
            print_indent (level);
            g_print ("      \e[32mSub-Device: \e[31m%s\e[0m\n",
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (child->data)));
        }

        level = level + 1;

        /* Recursive pass */
        device_proxy_available_cb (NULL, child->data, router);

        level = level - 1;

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

    g_print ("==> Device Unavailable: \e[31m%s\e[0;0m\n",
            gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy)));

    device_type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));

    if(router->udn == gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy)) ) {

    	g_source_remove (router->port_request_timeout);
    	g_source_remove (router->connection_status_timeout);
    	g_source_remove (router->external_ip_timeout);
    	g_source_remove (router->data_rate_timer);
    	gui_set_router_icon (NULL);
    	gui_disable ();

    	router->main_device = NULL;

    	if(router->external_ip != NULL) {
    	
    	    g_free (router->external_ip);
    	    router->external_ip = NULL;
    	}

    	if(router->upc != NULL)
            g_free (router->upc);

        g_free (router->friendly_name);
        g_free (router->brand);
        g_free (router->http_address);
        g_free (router->brand_website);
        g_free (router->model_description);
        g_free (router->model_name);
        g_free (router->model_number);
		g_free (router);
    }

}

static void on_context_available (GUPnPContextManager *context_manager,
							 GUPnPContext *context,
						     gpointer user_data)
{
	GUPnPControlPoint *cp;
	RouterInfo* router;

	g_print ("* Starting UPnP Resource discovery... ");
	
	/* Create a Control Point targeting RootDevice */
    cp = gupnp_control_point_new (context, "upnp:rootdevice");

	router = g_malloc( sizeof(RouterInfo) );
    router->main_device = NULL;
    router->external_ip = NULL;

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
	gupnp_context_manager_manage_control_point(context_manager, cp);

    g_print ("done\n");

    client_ip = gupnp_context_get_host_ip (context);

	g_object_unref(cp);
}

gboolean upnp_init()
{
    /* Create a new GUPnP Context. */
	context_mngr = gupnp_context_manager_create (opt_bindport);

	g_signal_connect(context_mngr, "context-available",
					 G_CALLBACK(on_context_available),
                     NULL);    
 
    return TRUE;
}

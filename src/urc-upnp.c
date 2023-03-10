/* urc-upnp.c
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

#include <string.h>
#include <glib.h>
#include <libgupnp/gupnp.h>
#include <libgssdp/gssdp.h>

#include "urc-gui.h"
#include "urc-graph.h"
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
    GError *local_error = NULL;
    GUPnPServiceProxyAction *action = NULL;

    action = gupnp_service_proxy_action_new(
                "DeletePortMapping",
                /* IN args */
                "NewRemoteHost",
                G_TYPE_STRING, remote_host,
                "NewExternalPort",
                G_TYPE_UINT, external_port,
                "NewProtocol",
                G_TYPE_STRING, protocol,
                NULL
    );

    gupnp_service_proxy_call_action(wan_service,
                                    action,
                                    NULL,
                                    &local_error);

    if (local_error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action, &local_error, NULL);
    gupnp_service_proxy_action_unref (action);

    if (local_error == NULL) {
        g_print("\e[36m*** Removed entry:\e[0m Port %d (%s)\n", external_port, protocol);

        error = NULL;
        return TRUE;
    }

    out:
    if (local_error != NULL) {
        g_warning ("\e[31m[EE]\e[0m DeletePortMapping: %s (%i)\n", local_error->message, local_error->code);

        *error = g_error_copy(local_error);
        g_error_free (local_error);

        return FALSE;
    }

    return FALSE;
}

gboolean add_port_mapping(GUPnPServiceProxy *wan_service, PortForwardInfo* port_info, GError **error)
{
    GError *local_error = NULL;
    GUPnPServiceProxyAction *action = NULL;

    action = gupnp_service_proxy_action_new(
                "AddPortMapping",
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
                NULL
            );

    gupnp_service_proxy_call_action(wan_service,
                                    action,
                                    NULL,
                                    &local_error);

    if (local_error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action, &local_error, NULL);
    gupnp_service_proxy_action_unref (action);

    if (local_error == NULL) {

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

    }

    out:
    if (local_error != NULL) {
        g_printerr ("\e[31m[EE]\e[0m AddPortMapping: %s (%i)\n", local_error->message, local_error->code);

        *error = g_error_copy(local_error);
        g_error_free (local_error);

        return FALSE;
    }

    return FALSE;
}

static PortForwardInfo* get_mapped_port(RouterInfo *router, guint index)
{

    GError *error = NULL;
    GUPnPServiceProxyAction *action = NULL;
    PortForwardInfo* port;

    port = g_malloc( sizeof(PortForwardInfo) );

    action = gupnp_service_proxy_action_new(
                "GetGenericPortMappingEntry",
                /* IN args */
                "NewPortMappingIndex",
                G_TYPE_UINT, index,
                NULL
    );

    gupnp_service_proxy_call_action(router->wan_conn_service,
                                    action,
                                    NULL,
                                    &error);

   if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
                   /* Error location */
                   &error,
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

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    out:
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

        g_print ("   local %s:%d ext %s:%d [%s]\n",
                 port_info->internal_host,
                 port_info->internal_port,
                 strlen(port_info->remote_host) > 0 ? port_info->remote_host : "*",
                 port_info->external_port,
                 port_info->protocol
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
    GUPnPServiceProxyAction *action = NULL;
    gchar* conn_status;
    gchar* last_conn_error;
    guint  uptime = 0;

    g_print("\e[36mRequest for connection status info... ");

    action = gupnp_service_proxy_action_new(
        "GetStatusInfo",
        NULL
    );

    gupnp_service_proxy_call_action(router->wan_conn_service,
                                    action,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
                   /* Error location */
                   &error,
                   /* OUT args */
                   "NewConnectionStatus",
                   G_TYPE_STRING, &conn_status,
                   "NewLastConnectionError",
                   G_TYPE_STRING, &last_conn_error,
                   "NewUptime",
                   G_TYPE_UINT, &uptime,
                   NULL);

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    if (error == NULL) {
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

    out:
    if (error != NULL) {
        g_print("\e[1;31mfailed\e[0;0m\n");

        gui_disable_conn_status();

        g_printerr ("\e[31m[EE]\e[0m GetStatusInfo: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }
    return FALSE;
}

/* Retrive download and upload speeds */
static gboolean update_data_rate_cb (gpointer data)
{
    GError *error = NULL;
    GUPnPServiceProxyAction *action_in = NULL;
    GUPnPServiceProxyAction *action_out = NULL;
    GUPnPServiceProxy *wan_device;
    gint64 begin_time;
    gint64 end_time;
    gdouble duration_secs;

    static guint old_total_bytes_received = 0;
    guint current_total_bytes_received;
    double data_rate_down;

    static guint old_total_bytes_sent = 0;
    guint current_total_bytes_sent;
    double data_rate_up;

    wan_device = ((RouterInfo *) data)->wan_common_ifc;

    action_in = gupnp_service_proxy_action_new(
        "GetTotalBytesReceived",
        NULL
    );

    action_out = gupnp_service_proxy_action_new(
        "GetTotalBytesSent",
        NULL
    );

    /* download speed */
    begin_time = g_get_monotonic_time();
    gupnp_service_proxy_call_action(wan_device,
                                    action_in,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto error_in;
    }

    gupnp_service_proxy_action_get_result (action_in,
                   /* Error location */
                   &error,
                   /* OUT args */
                   "NewTotalBytesReceived",
                   G_TYPE_UINT, &current_total_bytes_received,
                   NULL);

    g_clear_pointer (&action_in, gupnp_service_proxy_action_unref);

    end_time  = g_get_monotonic_time();

    if (error == NULL) {

        if(old_total_bytes_received == 0)
            data_rate_down = 0.0;
        else {

            // UPnP query time
            duration_secs = 1 + ((double)end_time - begin_time) / G_USEC_PER_SEC;

            if(opt_debug)
                g_print("\e[34mGetTotalBytesReceived() duration: %fs\e[0m\n", duration_secs);

            data_rate_down = ((current_total_bytes_received - old_total_bytes_received) / (duration_secs)  ) / 1024.0;
        }

        gui_set_download_speed(data_rate_down);
        gui_set_total_received(current_total_bytes_received);

        old_total_bytes_received = current_total_bytes_received;

    }

    error_in:
    if (error != NULL) {
        gui_disable_download_speed();
        gui_disable_total_received();

        g_printerr ("\e[31m[EE]\e[0m GetTotalBytesReceived: %s (%i)\n", error->message, error->code);
        g_error_free (error);
        error = NULL;
    }

    /* upload speed */
    begin_time = g_get_monotonic_time();
    gupnp_service_proxy_call_action(wan_device,
                                    action_out,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto error_out;
    }

    gupnp_service_proxy_action_get_result (action_out,
                   /* Error location */
                   &error,
                   /* OUT args */
                   "NewTotalBytesSent",
                   G_TYPE_UINT, &current_total_bytes_sent,
                   NULL);

    g_clear_pointer (&action_out, gupnp_service_proxy_action_unref);

    end_time  = g_get_monotonic_time();

    if (error == NULL) {

        if(old_total_bytes_sent == 0)
            data_rate_up = 0.0;
        else {

            // UPnP query time
            duration_secs = 1 + ((double)end_time - begin_time) / G_USEC_PER_SEC;

            if(opt_debug)
                g_print("\e[34mGetTotalBytesSent() duration:     %fs\e[0m\n", duration_secs);

            data_rate_up = (current_total_bytes_sent - old_total_bytes_sent) / (duration_secs) / 1024.0;
        }
        gui_set_upload_speed(data_rate_up);
        gui_set_total_sent(current_total_bytes_sent);

        old_total_bytes_sent = current_total_bytes_sent;
    }

    error_out:
    if (error != NULL) {
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
    GUPnPServiceProxyAction *action = NULL;
    gchar *ext_ip_addr = NULL;

    g_print("\e[36mRequest for external IP address... ");

    action = gupnp_service_proxy_action_new(
        "GetExternalIPAddress",
        NULL
    );

    gupnp_service_proxy_call_action(router->wan_conn_service,
                                    action,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
       /* Error location */
       &error,
       /* OUT args */
       "NewExternalIPAddress",
       G_TYPE_STRING, &ext_ip_addr,
       NULL);

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    if (error == NULL) {
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

    out:
    if (error != NULL) {
        g_print("\e[1;31mfailed\e[0;0m\n");

        gui_disable_ext_ip ();

        g_printerr ("\e[31m[EE]\e[0m GetExternalIPAddress: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    return FALSE;
}

/* Retrive RSIP and NAT availability */
gboolean get_nat_rsip_status (RouterInfo *router)
{
    GError *error = NULL;
    GUPnPServiceProxyAction *action = NULL;

    g_print("\e[36mRequest for NAT and RSIP availability... ");

    action = gupnp_service_proxy_action_new(
        "GetNATRSIPStatus",
        NULL
    );

    gupnp_service_proxy_call_action(router->wan_conn_service,
                                    action,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
                   /* Error location */
                   &error,
                   /* OUT args */
                   "NewRSIPAvailable",
                   G_TYPE_BOOLEAN, &router->rsip_available,
                   "NewNATEnabled",
                   G_TYPE_BOOLEAN, &router->nat_enabled,
                   NULL);

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    if (error == NULL) {
        g_print("\e[32msuccessful \e[0m[RSIP=%s, NAT=%s]\n", router->rsip_available == TRUE ? "yes" : "no", router->nat_enabled == TRUE ? "yes" : "no" );
        return TRUE;
    }

    out:
    if (error != NULL) {
        g_print("\e[1;31mfailed\e[0;0m\n");

        g_printerr ("\e[31m[EE]\e[0m GetNATRSIPStatus: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    return FALSE;
}

/* Retrive WAN link properties */
gboolean get_wan_link_properties (RouterInfo *router)
{
    GError *error = NULL;
    GUPnPServiceProxyAction *action = NULL;

    gchar *access_type, *physical_link_status;
    guint upstream_max_bitrate, downstream_max_bitrate;

    g_print("\e[36mRequest for WAN link properties... ");

    action = gupnp_service_proxy_action_new(
        "GetCommonLinkProperties",
        NULL
    );

    gupnp_service_proxy_call_action(router->wan_common_ifc,
                                    action,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
        /* Error location */
        &error,
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

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    if (error == NULL) {
        g_print("\e[32msuccessful\e[0m\n");
        g_print("\e[36mWAN link properties:\e[0m access_type=%s, link_status=%s, max_up=%u, max_down=%u\n",
                     access_type, physical_link_status, upstream_max_bitrate, downstream_max_bitrate );

        return TRUE;
    }

    out:
    if (error != NULL) {
        g_print("\e[1;31mfailed\e[0;0m\n");

        g_printerr ("\e[31m[EE]\e[0m GetCommonLinkProperties: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    return FALSE;
}

static gchar*
get_default_connection_service (GUPnPServiceProxy *proxy, int level)
{
    GError *error = NULL;
    GUPnPServiceProxyAction *action = NULL;
    gchar *string_buffer = NULL;
    gchar *connect_service = NULL;
    int i;

    if (opt_debug) {
        for(i = 0; i < level; i++)
            g_print("    ");

        g_print("      \e[32m** Getting DefaultConnectionService...\e[0m\n");
    }

    action = gupnp_service_proxy_action_new(
        "GetDefaultConnectionService",
        NULL
    );

    gupnp_service_proxy_call_action(proxy,
                                    action,
                                    NULL,
                                    &error);

    if (error != NULL) {
        goto out;
    }

    gupnp_service_proxy_action_get_result (action,
           /* Error location */
           &error,
           /* OUT args */
           "NewDefaultConnectionService",
           G_TYPE_STRING, &string_buffer,
           NULL);

    g_clear_pointer (&action, gupnp_service_proxy_action_unref);

    if (error == NULL) {

        if (string_buffer != NULL && strnlen(string_buffer, 32) > 0) {
            char** connect_result = NULL;
            connect_result = g_strsplit(string_buffer, ",", 2);

            if(opt_debug) {
                for(i = 0; i < level; i++)
                    g_print("    ");

                g_print("      \e[32mConnectionService:\e[0m %s\n", string_buffer);
            }
            connect_service = g_strdup(connect_result[1]);
            g_strfreev(connect_result);

        }
        else {
            g_print("\e[0;31m[WW]\e[0m GetDefaultConnectionService: empty\e[0;0m\n");

            connect_service = NULL;
        }
        // yes, can free NULL string
        g_free (string_buffer);
    }

    out:
    if (error != NULL) {
        g_printerr ("\e[31m[EE]\e[0m GetDefaultConnectionService: %s (%i)\n", error->message, error->code);
        g_error_free (error);
    }

    return connect_service;
}

static gboolean
urc_upnp_refresh_data_timeout(gpointer data) {
    urc_upnp_refresh_data( (RouterInfo *) data );
    
    // Every 5 minutes.
    ((RouterInfo *) data)->refresh_timeout = g_timeout_add_seconds(300, urc_upnp_refresh_data_timeout, data);

    return FALSE;
}

void
urc_upnp_refresh_data(RouterInfo *router)
{ 
    g_print("Refresh data...\n");
    get_conn_status( (RouterInfo *) router);
    get_external_ip( (RouterInfo *) router);
    get_nat_rsip_status( (RouterInfo *) router);
    discovery_mapped_ports_list( (RouterInfo *) router);
}


/* Service event callback */
static void
service_proxy_event_cb (GUPnPServiceProxy *proxy,
                        const char *variable,
                        GValue *value,
                        gpointer data)
{

    RouterInfo *router;
    router = (RouterInfo *) data;

    /* Numebr of port mapped entries */
    if(g_strcmp0("PortMappingNumberOfEntries", variable) == 0)
    {

        g_print("\e[33mEvent:\e[0;0m Ports mapped: %d\n", g_value_get_uint(value));

        discovery_mapped_ports_list(router);
    }
    /* Got external IP */
    else if(g_strcmp0("ExternalIPAddress", variable) == 0)
    {
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

static gchar*
parse_presentation_url(gchar *presentation_url, const gchar *device_location)
{
    gchar** url_split = NULL;
    gchar *output_url_string;

    if(presentation_url == NULL)
    {
        url_split = g_strsplit(device_location, ":", 0);

        output_url_string = g_strconcat(url_split[0], ":", url_split[1], "/", NULL);
        g_strfreev(url_split);

        if(opt_debug)
            g_print("        Rewritten URL: %s\n", output_url_string);
    }
    /* workaround for urls like "/login" without base url */
    else if(presentation_url != NULL && g_str_has_prefix(presentation_url, "http") == FALSE)
    {

        url_split = g_strsplit(device_location, ":", 0);
        output_url_string = g_strconcat(url_split[0], ":", url_split[1], presentation_url, NULL);
        g_strfreev(url_split);

        g_free(presentation_url);

        if(opt_debug)
            g_print("        Rewritten URL: %s\n", output_url_string);
    }
    else
    {
        output_url_string = presentation_url;
    }

    return output_url_string;
}

/* useful function for debug strings */
static void
print_indent(guint8 tabs)
{
    int i;
    
    for(i = 0; i < tabs; i++)
            g_print("    ");
}

/* function to compare UPnP device/service string and check
 * version is at least the one needed */
static int
device_service_cmp(const char *devserv1, const char *devserv2, int ver2)
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

static void
urc_set_main_device(GUPnPServiceProxy *proxy,
                    RouterInfo        *router,
                    gboolean           is_complaiant_igd_device)
{
    const SoupURI *url_base;

    if (is_complaiant_igd_device) {
        g_print ("*** Selected IGD compliant device\n");
    }
    else {
        g_print ("*** Selected simple device (not IGD complaiant)\n");
    }

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
    router->device_descriptor = gupnp_device_info_get_location(GUPNP_DEVICE_INFO (proxy));
    router->data_rate_timer = 0;

    url_base = gupnp_device_info_get_url_base (GUPNP_DEVICE_INFO (proxy));
    router->device_ip = url_base->host;

    g_print ("UPnP descriptor: %s\n", router->device_descriptor);

    if (opt_debug) {

        g_print ("   Model description: %s\n", router->model_description);
        g_print ("          Model name: %s\n", router->model_name);
        g_print ("        Model number: %s\n", router->model_number);
        g_print ("               Brand: %s\n", router->brand);
        g_print ("    Presentation URL: %s\n", router->http_address);
        g_print ("                 UPC: %s\n", router->upc);
        g_print ("  Unique Device Name: %s\n", router->udn);
        g_print ("                  IP: %s\n", router->device_ip);
    }

    router->http_address = parse_presentation_url(router->http_address,
                           gupnp_device_info_get_location(GUPNP_DEVICE_INFO (proxy)));

    /* workaround for empty <friendlyName> property or standard name */
    if(g_strcmp0 (router->friendly_name, "") == 0 || g_strcmp0 (router->friendly_name, "WANConnectionDevice") == 0)
    {
        if(g_strcmp0(router->model_name, "") == 0)
            router->friendly_name = g_strdup (router->model_description);
        else
            router->friendly_name = g_strdup (router->model_name);
    }

    gui_set_router_info (router);
}

static void
urc_gupnp_introspection_callback (GUPnPServiceInfo *info,
                                  GUPnPServiceIntrospection *introspection,
                                  const GError *error,
                                  gpointer user_data)
{

    const GList *actions_list;
    actions_list = gupnp_service_introspection_list_action_names (introspection);

    if(actions_list != NULL) {
        do {
            g_print("               > %s\n", (gchar*) actions_list->data);

        } while ((actions_list = actions_list->next));
    }
}

static void
device_proxy_available_cb (GUPnPControlPoint *cp,
                           GUPnPServiceProxy *proxy,
                           RouterInfo        *router)
{
    GList *services;
    GList *subdevices;

    static char *connect_service = NULL;
    const char *service_type = NULL;
    const char *service_id = NULL;
    const char *device_type = NULL;

    static int level = 0;

    if(level == 0) {

        g_print ("==> Device Available: \e[31m%s\e[0;0m\n",
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy)));
    }
    
    /* do nothing if there is already a device stored (and it's not a subdevice) */
    if(router->main_device != NULL && level == 0)
        return;

    device_type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));

    /* Is an IGD device? */
    if(device_service_cmp (device_type, "urn:schemas-upnp-org:device:InternetGatewayDevice:", 1) == 0)
    {
        urc_set_main_device(proxy, router, TRUE);

    }
    /* There is only a WANConnectionDevice as root device? */
    else if(device_service_cmp (device_type, "urn:schemas-upnp-org:device:WANConnectionDevice:", 1) == 0 && level == 0 )
    {
        urc_set_main_device(proxy, router, FALSE);
    }

    /* Enum services (only on managed devices) */
    if (router->main_device != NULL)
    {


        services = gupnp_device_info_list_services (GUPNP_DEVICE_INFO (proxy));

        if (opt_debug) {
            print_indent(level);

            if (g_list_length (services) > 0)
                g_print ("    \e[1;32mEnum services...\e[0;0m\n");
        }

        while( services ) {

            service_type = gupnp_service_info_get_service_type (GUPNP_SERVICE_INFO (services->data));
            service_id = gupnp_service_info_get_id (GUPNP_SERVICE_INFO (services->data));

            if(opt_debug) {

                print_indent (level);
                g_print("      \e[32mService:\e[0m %s\n", service_id );

                print_indent (level);
                g_print("         Type: %s\n", service_type );


                /* Introspect current service */
                gupnp_service_info_get_introspection_async (services->data, urc_gupnp_introspection_callback, &level);

            }

            /* Is a IP forwarding service? */
            if( (device_service_cmp (service_type, "urn:schemas-upnp-org:service:Layer3Forwarding:", 1) == 0) ||
                (device_service_cmp (service_type, "urn:schemas-upnp-org:service:L3Forwarding:", 1) == 0) )
            {

                if(connect_service != NULL)
                    g_free (connect_service);

                connect_service = get_default_connection_service (services->data, level);

            }
            /* Is a WAN IFC service? */
            else if(device_service_cmp (service_type, "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:", 1) == 0)
            {
                router->wan_common_ifc = services->data;

                /* Get common WAN link properties */
                get_wan_link_properties (router);

                /* Start data rate timer */
                update_data_rate_cb (router);
                
                urc_enable_graph ();

            }
            /* Is a WAN IP Connection service or other? */
            else if(device_service_cmp (service_type, "urn:schemas-upnp-org:service:WANIPConnection:", 1) == 0)
            {

                router->wan_conn_service = services->data;
                gui_activate_buttons();

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
                /* Gets mapped port list */
                discovery_mapped_ports_list(router);

                /* Subscribe to events */
                gupnp_service_proxy_set_subscribed (services->data, TRUE);

                gupnp_service_proxy_add_notify (services->data,
                                                "PortMappingNumberOfEntries",
                                                G_TYPE_UINT,
                                                service_proxy_event_cb,
                                                router);
                gupnp_service_proxy_add_notify (services->data,
                                                "ExternalIPAddress",
                                                G_TYPE_STRING,
                                                service_proxy_event_cb,
                                                router);
                gupnp_service_proxy_add_notify (services->data,
                                                "ConnectionStatus",
                                                G_TYPE_STRING,
                                                service_proxy_event_cb,
                                                router);

                /**
                 * Because some routers are not notify about changes,
                 * let's polling the data every 5 minutes.
                 */
                router->refresh_timeout = g_timeout_add_seconds (300, urc_upnp_refresh_data_timeout, router);
            }
            else
                g_object_unref (services->data);

            services = g_list_delete_link (services, services);
        }
    }


    /* Enum subdevices */
    subdevices = gupnp_device_info_list_devices (GUPNP_DEVICE_INFO (proxy));

    if (opt_debug && g_list_length (subdevices) > 0) {
        print_indent (level);
        g_print ("    \e[1;32mEnum sub-devices...\e[0;0m\n");
    }

    while (subdevices) {
    
        if (opt_debug) {
            print_indent (level);
            g_print ("      \e[32mSub-Device: \e[31m%s\e[0m\n",
                gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (subdevices->data)));
        }

        level = level + 1;

        /* Recursive pass */
        device_proxy_available_cb (NULL, subdevices->data, router);

        level = level - 1;

        g_object_unref (subdevices->data);
        subdevices = g_list_delete_link (subdevices, subdevices);
    }

}

static void
device_proxy_unavailable_cb (GUPnPControlPoint *cp,
                             GUPnPServiceProxy *proxy,
                             RouterInfo        *router)
{
    g_print ("==> Device Unavailable: \e[31m%s\e[0;0m\n",
            gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy)));

    if(g_strcmp0(router->udn, gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy))) == 0 ) {

        g_source_remove (router->refresh_timeout);
        if (router->data_rate_timer > 0) {
          g_source_remove (router->data_rate_timer);
        }

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

static void
on_context_available (GUPnPContextManager *context_manager,
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
     * our target are found, so connect to it */
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

    client_ip = gssdp_client_get_host_ip (GSSDP_CLIENT(context));

    g_print ("\e[1;37m%s:\e[0;0m host IP %s network %s\n",
             gssdp_client_get_interface(GSSDP_CLIENT(context)),
             client_ip,
             gssdp_client_get_network (GSSDP_CLIENT(context))
    );
    
    g_object_unref(cp);
}

static void
on_context_unavailable (GUPnPContextManager *context_manager,
                        GUPnPContext *context,
                        gpointer user_data)
{
    g_print ("* Detected network unavailable, trying to reconnect...\n");
}


gboolean
upnp_init()
{
    GUPnPWhiteList *white_list;

    /* Create a new GUPnP Context. */
    context_mngr = gupnp_context_manager_create (opt_bindport);
    g_assert (context_mngr != NULL);

    if (opt_bindif != NULL) {
        white_list = gupnp_context_manager_get_white_list
                                    (context_mngr);
        gupnp_white_list_add_entry (white_list, opt_bindif);
        gupnp_white_list_set_enabled (white_list, TRUE);
    }

    g_signal_connect(context_mngr, "context-available",
                 G_CALLBACK(on_context_available),
                 NULL);
    g_signal_connect(context_mngr, "context-unavailable",
                 G_CALLBACK(on_context_unavailable),
                 NULL);


    return TRUE;
}

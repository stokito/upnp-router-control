/* urc-graph.c
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

#include <math.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "urc-graph.h"

#define GRAPH_POINTS 90
#define FRAME_WIDTH 4

cairo_surface_t *background = NULL;
cairo_surface_t *graph = NULL;

static GList *downspeed_values = NULL;
static GList *upspeed_values = NULL;

// graph fullscale default value
static guint net_max = 2;

void init_graph()
{
    SpeedValue *speed;
    gint i;

    // fill speed graph lists
    for(i = 0; i <= GRAPH_POINTS; i++) {
        speed = g_malloc(sizeof(SpeedValue));
        speed->speed = 0;
        speed->valid = FALSE;
        upspeed_values = g_list_prepend(upspeed_values, speed);
    }

    for(i = 0; i <= GRAPH_POINTS; i++) {
        speed = g_malloc(sizeof(SpeedValue));
        speed->speed = 0;
        speed->valid = FALSE;
        downspeed_values = g_list_prepend(downspeed_values, speed);
    }
}

static void
clear_graph_background()
{
    if (background) {
		cairo_surface_destroy(background);
		background = NULL;
	}
}

static void
clear_graph_data()
{
    if (graph) {
		cairo_surface_destroy(graph);
		graph = NULL;
	}
}

static void
graph_draw_background (GtkWidget *widget)
{
    cairo_t *cr;
    cairo_pattern_t *pat;
    cairo_text_extents_t extents;
    gchar *label;
    GtkStyle *style;
    double draw_width, draw_height;
    GtkAllocation allocation;

    const double fontsize = 8.0;
    const double rmargin = 3.5 * fontsize;
	const guint indent = 24;
	const guint x_frame_count = 6;
	guint y_frame_count = 3;
    double dash[2] = { 1.0, 2.0 };
    double x_frame_size, y_frame_size;
    double x, y;
    gint i;
    float label_value;
    
    gtk_widget_get_allocation (widget, &allocation);

	background = cairo_image_surface_create( CAIRO_FORMAT_ARGB32,
	                                         allocation.width,
					                         allocation.height);
	cr = cairo_create(background);

    draw_width = allocation.width - 2 * FRAME_WIDTH;
    draw_height = allocation.height - 2 * FRAME_WIDTH;

	cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);

    // determines the number of grids based on height
	switch ( (int) (draw_height) / 30 )
	{
	    case 0:
	    case 1:
		    y_frame_count = 1;
		    break;
	    case 2:
	    case 3:
		    y_frame_count = 2;
		    break;
	    case 4:
		    y_frame_count = 4;
		    break;
	    default:
		    y_frame_count = 5;

	}

    style = gtk_widget_get_style (widget);

    cairo_set_font_size (cr, fontsize);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width (cr, 1);

    // white to gray faded background
    pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, draw_height - 15.0);
    cairo_pattern_add_color_stop_rgb (pat, 0, 1, 1, 1);
    cairo_pattern_add_color_stop_rgb (pat, 1, 0.9, 0.9, 0.9);
    cairo_rectangle (cr, rmargin + indent, 0, draw_width - rmargin - indent, draw_height - 15.0);
    cairo_set_source (cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy (pat);

    // draw grid
	cairo_set_dash (cr, dash, 2, 0);
	gdk_cairo_set_source_color (cr, &style->fg[GTK_STATE_NORMAL]);

    // drawing vertical grid
    x_frame_size = (draw_width - rmargin - indent) / x_frame_count;
    for(i = 0; i <= x_frame_count; i++) {

        x = rmargin + indent + (x_frame_size * i);

        if(i == 0)
            label = g_strdup_printf(_("%u seconds"), GRAPH_POINTS - (GRAPH_POINTS / x_frame_count) * i);

        else
            label = g_strdup_printf("%u", GRAPH_POINTS - (GRAPH_POINTS / x_frame_count) * i);

        cairo_text_extents (cr, label, &extents);
		cairo_move_to (cr, x - extents.width/2, draw_height+2);
		cairo_show_text (cr, label);
		g_free(label);

		cairo_move_to (cr, x, 0);
        cairo_line_to (cr, x, draw_height - 10.0);

    }

    // drawing horizontal grid
    y_frame_size = (draw_height - 15.0) / y_frame_count;
    for(i = 0; i <= y_frame_count; i++) {
        y = y_frame_size * i;

        label_value = net_max - ((float) net_max / y_frame_count) * i;

        if(label_value < 1024)
            label = g_strdup_printf("%0.1f KiB/s", label_value);
        if(label_value > 1024)
            label = g_strdup_printf("%0.1f MiB/s", label_value / 1024);

        cairo_text_extents (cr, label, &extents);
		cairo_move_to (cr, rmargin + indent - extents.width - 10 , y + extents.height/2);
		cairo_show_text (cr, label);
		g_free(label);

        cairo_move_to (cr, rmargin + indent - 5, y);
        cairo_line_to (cr, draw_width - rmargin + indent + 5, y);

    }
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.75);
    cairo_stroke(cr);

    cairo_destroy (cr);
}

static void
graph_set_fullscale (double tmp_net_max)
{
    //g_debug("net_max: %d tmp_net_max: %d\n", net_max, (int) ceil(tmp_net_max));

    // workaround for values <= 10
    if(tmp_net_max != net_max && tmp_net_max <= 10) {

        static unsigned cur_fullscale;

        if(tmp_net_max <= 2)
            net_max = 2;
        else if(tmp_net_max <= 6)
            net_max = 6;
        else
            net_max = 10;

        if(net_max == cur_fullscale)
            return;

        cur_fullscale = net_max;
        //g_debug("Updated full scale: to %d\n", net_max);
        clear_graph_background();
        return;
    }

    if(ceil(tmp_net_max) > net_max || ceil(tmp_net_max) < (0.8 * net_max - 10))
    {
        // an upper margin
        net_max = (int) ceil(1.1 * tmp_net_max) + 10;

        // round to multiples of 10
        if(net_max % 10 != 0)
            net_max = net_max - (net_max % 10);

        //g_debug("Updated full scale: to %d\n", net_max);
        clear_graph_background();
    }
}

static void
graph_draw_data (GtkWidget *widget)
{
    cairo_t *cr;
    GtkAllocation allocation;
    double draw_width, draw_height;
    const double fontsize = 8.0;
    const double rmargin = 3.5 * fontsize;
	const guint indent = 24;
    double x, y;
    gint i;
    GList *list;
    SpeedValue *speed_value;
    guint tmp_net_max = 0;
    
    gtk_widget_get_allocation (widget, &allocation);

	graph = cairo_image_surface_create( CAIRO_FORMAT_ARGB32,
	                                    allocation.width,
					                    allocation.height);
	cr = cairo_create(graph);

    draw_width = allocation.width - 2 * FRAME_WIDTH;
    draw_height = allocation.height - 2 * FRAME_WIDTH;

    cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);

    /* draw load lines */
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_dash (cr, NULL, 0, 0);
    cairo_set_line_width (cr, 1.00);

    /* upload speed */
    list = upspeed_values;
    speed_value = list->data;

    // first point
    if(speed_value->valid == TRUE) {
        // 2 is: FRAME_WIDTH / 2
        y = draw_height - 15 - 2 - speed_value->speed * (draw_height - 15 - 2) / net_max;
        x = draw_width;
        cairo_move_to (cr, x, y + 0.5);
    }

    for(i = GRAPH_POINTS; i >= 0; i--) {

        speed_value = list->data;

        if(speed_value->valid == TRUE) {
            // 2 is: FRAME_WIDTH / 2
            y = draw_height - 15 - 2 - speed_value->speed * (draw_height - 15 - 2) / net_max;
            x = rmargin + indent + ((draw_width - rmargin - indent) / GRAPH_POINTS) * i;

            cairo_line_to (cr, x, y + 0.5);

            if(tmp_net_max < speed_value->speed)
                tmp_net_max = ceil(speed_value->speed);

        }

        list = list->next;
    }

    // upload line color
    cairo_set_source_rgb (cr, 0.52, 0.28, 0.60);
    cairo_stroke(cr);

    /* download speed */
    list = downspeed_values;
    speed_value = list->data;

    if(speed_value->valid == TRUE) {
        // 2 is: FRAME_WIDTH / 2
        y = draw_height - 15 - 2 - speed_value->speed * (draw_height - 15 - 2) / net_max;
        x = draw_width;
        cairo_move_to (cr, x, y + 0.5);
    }

    for(i = GRAPH_POINTS; i >= 0; i--) {

        speed_value = list->data;

        if(speed_value->valid == TRUE) {
            // 2 is: FRAME_WIDTH / 2
            // 15 is: bottom space
            y = draw_height - 15 - 2 - speed_value->speed * (draw_height - 15 - 2) / net_max;
            x = rmargin + indent + ((draw_width - rmargin - indent) / GRAPH_POINTS) * i;

            cairo_line_to (cr, x, y + 0.5);

            if(tmp_net_max < speed_value->speed)
                tmp_net_max = ceil(speed_value->speed);
        }

        list = list->next;
    }
    // download line color
    cairo_set_source_rgb (cr, 0.18, 0.49, 0.70);
    cairo_stroke(cr);

    cairo_destroy (cr);

    graph_set_fullscale(tmp_net_max);
}

void update_download_graph_data(SpeedValue *speed)
{
    GList *tmp_elem;

    downspeed_values = g_list_prepend(downspeed_values, speed);

    tmp_elem = g_list_nth(downspeed_values, GRAPH_POINTS+1);
    if(tmp_elem) {
        g_free(tmp_elem->data);
        downspeed_values = g_list_delete_link(downspeed_values, tmp_elem);
    }

    if(speed->speed > net_max)
        graph_set_fullscale(speed->speed);

    clear_graph_data();
}

void update_upload_graph_data(SpeedValue *speed)
{
    GList *tmp_elem;

    upspeed_values = g_list_prepend(upspeed_values, speed);

    tmp_elem = g_list_nth(upspeed_values, GRAPH_POINTS+1);
    if(tmp_elem) {
        g_free(tmp_elem->data);
        upspeed_values = g_list_delete_link(upspeed_values, tmp_elem);
    }

    if(speed->speed > net_max)
        graph_set_fullscale(speed->speed);

    clear_graph_data();
}

void disable_graph_data()
{
    GList *elem;

    if(upspeed_values == NULL || downspeed_values == NULL)
        return;

    elem = upspeed_values;
    do {
        ((SpeedValue *)elem->data)->valid = FALSE;
        ((SpeedValue *)elem->data)->speed = 0.0;
        elem = elem->next;
    } while(elem);

    elem = downspeed_values;
    do {
        ((SpeedValue *)elem->data)->valid = FALSE;
        ((SpeedValue *)elem->data)->speed = 0.0;
        elem = elem->next;
    } while(elem);

    clear_graph_data ();
}

gboolean
on_drawing_area_configure_event (GtkWidget         *widget,
		                         GdkEventConfigure *event,
		                         gpointer           data_ptr)
{
    clear_graph_background();

	clear_graph_data ();

	return TRUE;
}


gboolean
on_drawing_area_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event,
                              gpointer        user_data)
{
    cairo_t *cr;

    if(graph == NULL)
        graph_draw_data (widget);

    if(background == NULL)
        graph_draw_background (widget);

    cr = gdk_cairo_create (gtk_widget_get_window (widget));

    cairo_rectangle (cr,
                         event->area.x, event->area.y,
                         event->area.width, event->area.height);
    cairo_clip (cr);

    // draw background
    cairo_set_source_surface(cr, background, 0.0, 0.0);
    cairo_paint(cr);

    // draw graph data
    cairo_set_source_surface(cr, graph, 0.0, 0.0);
    cairo_paint(cr);

    cairo_destroy (cr);

    return FALSE;
}

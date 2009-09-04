/**
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2009 Nguyen Thanh Trung <nguyenthanh.trung@nomovok.com>
 * Copyright (C) 2009 Dam Quang Tuan <damquang.tuan@nomovok.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-fpsbin
 *
 * FIXME:Describe fpsbin here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! fpsbin ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstfpsbin.h"

GST_DEBUG_CATEGORY_STATIC(gst_fps_bin_debug);
#define GST_CAT_DEFAULT gst_fps_bin_debug

/* Filter signals and args */
enum {
	/* FILL ME */
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_FPS,
	PROP_SILENT
};

/**
 * the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS("ANY")
		);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS("ANY")
		);

GST_BOILERPLATE(GstFpsBin, gst_fps_bin, GstBin,
		GST_TYPE_BIN);

static void gst_fps_bin_set_property(GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec);
static void gst_fps_bin_get_property(GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec);

static void gst_fps_bin_set_fps(GstFpsBin * fps_bin, guint numerator, guint demonirator);

/** support for set_fps functions */
static void gst_fps_bin_stop_pipeline(GstFpsBin * fps_bin);
static void gst_fps_bin_start_pipeline(GstFpsBin * fps_bin);

/** these stuff used for testing */
static gboolean gst_fps_bin_count_secs(gpointer data);
static gboolean gst_fps_bin_change_fps(gpointer data);

/** GObject vmethod implementations */

/**
 * init base object for fps_bin
 * This function is generated automatically using gst-template
 */
static void gst_fps_bin_base_init(gpointer gclass) {
	GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

	gst_element_class_set_details_simple(element_class,
			"FpsBin",
			"Filter/Bin/Video",
			"Video fps adjust bin",
			"Nguyen Thanh Trung <nguyenthanh.trung@nomovok.com> & Dam Quang Tuan <damquang.tuan@nomovok.com>");

	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&src_factory));
	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&sink_factory));
}

#define FPS_MAX_NUMERATOR		G_MAXINT
#define FPS_MAX_DENOMINATOR		1
#define FPS_MIN_NUMERATOR		0
#define FPS_MIN_DENOMINATOR		1
#define FPS_DEFAULT_NUMERATOR	30
#define FPS_DEFAULT_DENOMINATOR	1

/**
 * initialize the fpsbin's class
 * autogenerated by gst-template
 */
static void gst_fps_bin_class_init(GstFpsBinClass * klass) {
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = gst_fps_bin_set_property;
	gobject_class->get_property = gst_fps_bin_get_property;

	g_object_class_install_property(gobject_class, PROP_FPS,
			gst_param_spec_fraction("fps", "fps", "frame rate value",
			FPS_MIN_NUMERATOR, FPS_MIN_DENOMINATOR,
			FPS_MAX_NUMERATOR, FPS_MAX_DENOMINATOR,
			FPS_DEFAULT_NUMERATOR, FPS_DEFAULT_DENOMINATOR,
			G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, PROP_SILENT,
			g_param_spec_boolean("silent", "Silent", "Produce verbose output ?",
			FALSE, G_PARAM_READWRITE));
}

/**
 * initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 * auto-generate by gst-template
 */
static void gst_fps_bin_init(GstFpsBin * fps_bin, GstFpsBinClass * gclass) {
	GstPad *pad, *gpad;

	// init child elements
	fps_bin->videorate = gst_element_factory_make("videorate", "fps_bin_videorate");
	g_return_if_fail(fps_bin->videorate != NULL);

	fps_bin->capsfilter = gst_element_factory_make("capsfilter", "fps_bin_capsfilter");
	g_return_if_fail(fps_bin->capsfilter != NULL);

	// link them together
	gst_bin_add_many(GST_BIN(fps_bin), fps_bin->videorate, fps_bin->capsfilter, NULL);
	gst_element_link(fps_bin->videorate, fps_bin->capsfilter);

	// ghost the pads to our bin
	// sink pad
	pad = gst_element_get_static_pad(fps_bin->videorate, "sink");
	gpad = gst_ghost_pad_new("sink", pad);
	gst_object_unref(pad);
	gst_pad_set_active(gpad, TRUE);
	gst_element_add_pad(GST_ELEMENT(fps_bin), gpad);

	// src pad
	pad = gst_element_get_static_pad(fps_bin->capsfilter, "src");
	gpad = gst_ghost_pad_new("src", pad);
	gst_object_unref(pad);
	gst_pad_set_active(gpad, TRUE);
	gst_element_add_pad(GST_ELEMENT(fps_bin), gpad);

	fps_bin->silent = FALSE;

	// default value for fps
	fps_bin->numerator = 30;
	fps_bin->denominator = 1;

	// default value for testing variable
	fps_bin->secs = 0;
	g_timeout_add_seconds(1, gst_fps_bin_count_secs, fps_bin);
	g_timeout_add_seconds(1, gst_fps_bin_change_fps, fps_bin);
	gst_fps_bin_set_fps(fps_bin, 30, 1);
}

static void gst_fps_bin_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
	GstFpsBin *filter = GST_FPSBIN(object);

	switch (prop_id) {
		case PROP_FPS:
		{
			gint numerator = 0, denominator = 0;
			numerator = gst_value_get_fraction_numerator(value);
			denominator = gst_value_get_fraction_denominator(value);
			g_warning("New value for set_property: %d/%d", numerator, denominator);
			gst_fps_bin_set_fps(filter, numerator, denominator);
			g_warning("Name of paramspec '%s'", g_param_spec_get_name(pspec));
		}
			break;
		case PROP_SILENT:
			filter->silent = g_value_get_boolean(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gst_fps_bin_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
	GstFpsBin *filter = GST_FPSBIN(object);

	switch (prop_id) {
		case PROP_FPS:
			gst_value_set_fraction(value, filter->numerator, filter->denominator);
			break;
		case PROP_SILENT:
			g_value_set_boolean(value, filter->silent);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * set the fps value for out bin.
 * The fps value will be applied at run time
 * Gstreamer use fraction for storing fps value, so we have 2 values here: numerator and denominator
 *
 * @param fps_bin GstFpsBin* fps_bin element
 * @param numerator guint numerator value of fps
 * @param denominator guint denominator value of fps
 */
static void gst_fps_bin_set_fps(GstFpsBin* fps_bin, guint numerator, guint denominator) {
	g_warning("numerator: %d", numerator);
	g_warning("denominator: %d", denominator);
	GstCaps * caps = gst_caps_new_simple("video/x-raw-yuv",
			"framerate", GST_TYPE_FRACTION, numerator, denominator,
			NULL);
	g_warning("Change fps to %d/%d", numerator, denominator);
	gst_fps_bin_stop_pipeline(fps_bin);
	g_object_set(fps_bin->capsfilter, "caps", caps, NULL);
	gst_fps_bin_start_pipeline(fps_bin);
	fps_bin->numerator = numerator;
	fps_bin->denominator = denominator;
	gst_caps_unref(caps);
}

/**
 * stop the pipeline if needed
 * We need to stop pipeline and then start it again for renegotiation process
 *
 * @param fps_bin GstFpsBin* our bin to get the pipeline
 */
static void gst_fps_bin_stop_pipeline(GstFpsBin* fps_bin) {
	GstElement * pipeline = GST_ELEMENT(gst_element_get_parent(fps_bin));
	if (pipeline != NULL) {
		gst_element_set_state(pipeline, GST_STATE_READY);
	}
}

/**
 * start the pipeline if needed
 *
 * @param fps_bin GstFpsBin* our bin to get the pipeline
 */
static void gst_fps_bin_start_pipeline(GstFpsBin* fps_bin) {
	GstElement * pipeline = GST_ELEMENT(gst_element_get_parent(fps_bin));
	if (pipeline != NULL) {
		gst_element_set_state(pipeline, GST_STATE_PLAYING);
	}
}

/**
 * testing stuff:
 * we used this function to count the number of seconds passed
 *
 * This's callback function for g_idle_add*()
 */
static gboolean gst_fps_bin_count_secs(gpointer data) {
	GstFpsBin * fps_bin = GST_FPSBIN(data);
	fps_bin->secs++;
	return TRUE;
}

/**
 * testing stuff:
 * this function will check if the passed secs % 10 == 0 then it will change fps
 * 30/1 -> 1/1 and vice vesa
 *
 * This's callback function for g_idle_add*()
 */
static gboolean gst_fps_bin_change_fps(gpointer data) {
	GstFpsBin * fps_bin = GST_FPSBIN(data);
	GValue fraction = {0, };

	if ( (fps_bin->secs > 0) && (fps_bin->secs % 10 ==0) ) {
		if (fps_bin->numerator == fps_bin->denominator) {
			fps_bin->numerator = 30;
		} else {
			fps_bin->numerator = 1;
		}
		fps_bin->denominator = 1;

		// now we set the new fps
		{
			g_warning("now, the value in fps_bin: %d/%d", fps_bin->numerator, fps_bin->denominator);
			g_value_init(&fraction, GST_TYPE_FRACTION);
			gst_value_set_fraction(&fraction, fps_bin->numerator, fps_bin->denominator);
			{
				gint numerator, denominator;
				numerator = gst_value_get_fraction_numerator(&fraction);
				denominator = gst_value_get_fraction_denominator(&fraction);
				g_warning("The value inside fraction: %d/%d", numerator, denominator );
			}
			g_object_set(data, "fps", fraction, NULL);
			//g_object_set_property(G_OBJECT(fps_bin), "fps", &fraction);
			{
				gint numerator, denominator;
				numerator = gst_value_get_fraction_numerator(&fraction);
				denominator = gst_value_get_fraction_denominator(&fraction);
				g_warning("The value inside fraction 2: %d/%d", numerator, denominator );
			}
		}
		//gst_fps_bin_set_fps(fps_bin, fps_bin->numerator, fps_bin->denominator);
	}
	return TRUE;
}

/**
 * entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean fpsbin_init(GstPlugin * fpsbin) {
	/* debug category for fltering log messages
	 *
	 * exchange the string 'Template fpsbin' with your description
	 */
	GST_DEBUG_CATEGORY_INIT(gst_fps_bin_debug, "fpsbin",
			0, "Template fpsbin");

	return gst_element_register(fpsbin, "fpsbin", GST_RANK_NONE,
			GST_TYPE_FPSBIN);
}

/**
 * gstreamer looks for this structure to register fpsbins
 *
 * exchange the string 'Template fpsbin' with your fpsbin description
 */
GST_PLUGIN_DEFINE(
		GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		"fpsbin",
		"Template fpsbin",
		fpsbin_init,
		VERSION,
		"LGPL",
		"GStreamer",
		"http://gstreamer.net/"
		)

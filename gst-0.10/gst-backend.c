/*
 * Copyright (C) 2008 Felipe Contreras.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* 
Based on: 
- https://github.com/sreerenjb/gstplayer/blob/master/gst-backend.c
- https://github.com/OpenViX/enigma2/blob/master/lib/service/servicemp3.cpp
*/
 
#include "gst-backend.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <glib-object.h>

static GstElement *g_gst_playbin = NULL;
static GstElement *g_audioSink   = NULL;
static GstElement *g_videoSink   = NULL;
static GstSeekFlags g_seek_flags  = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
#ifdef PLATFORM_I686
static GstPlayFlags g_playbin_flags = ( GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO );
#else
static GstPlayFlags g_playbin_flags = ( GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO); // 0x47 - GST_PLAY_FLAG_TEXT; 
#endif

static PlaybackInfo_t g_playback_info;

static gboolean g_is_local_file = 0;
static gint64  g_duration = 0;
static gchar  *g_filename = NULL;
static gchar  *g_download_buffer_path = NULL;
static guint64 g_ring_buffer_max_size = -0;
static gint64  g_buffer_duration = -1;
static gint    g_buffer_size = 0;
static StrPair_t **g_ptr_http_header_fields = NULL;
static int  g_sfd = -1;     /* Used to wake up main loop when new message in the gst bus is available */

static struct
{
    gint64 check_timestamp;
    gint64 decoder_time;
    gint64 playbin_time;
} g_eos_fix;

/* Include common functions */
#include "tracks.h"

/* return timestamp in miliseconds */
static gint64 getTimestamp()
{
#if 1
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
#else
    struct timespec tsnow;
    if(0 == clock_gettime(CLOCK_MONOTONIC, &tsnow))
    {
        return (gint64)tsnow.tv_sec * 1000 + tsnow.tv_nsec / 1000000L;
    }
    return 0;
#endif
}

static gint match_sinktype(GstElement *element, gpointer type)
{
    //printf("SULGE: [%s]\n", g_type_name(G_OBJECT_TYPE(element)));
    return strcmp(g_type_name(G_OBJECT_TYPE(element)), (const char*)type);
}


static void InfoStructChanged()
{
    PlaybackInfo_t *ptrP = &g_playback_info;
    
    fprintf(stderr, "{\"PLAYBACK_INFO\":{ \"isPlaying\":%s, \"isPaused\":%s, \"isForwarding\":%s, \"isSeeking\":%s, \"isCreationPhase\":%s,", \
    DUMP_BOOL(ptrP->isPlaying), DUMP_BOOL(ptrP->isPaused), DUMP_BOOL(ptrP->isForwarding), DUMP_BOOL(ptrP->isSeeking), DUMP_BOOL(ptrP->isCreationPhase) );
    fprintf(stderr, "\"BackWard\":%f, \"SlowMotion\":%d, \"Speed\":%d, \"AVSync\":%d,", ptrP->BackWard, ptrP->SlowMotion, ptrP->Speed, ptrP->AVSync);
    fprintf(stderr, " \"isVideo\":%s, \"isAudio\":%s, \"isSubtitle\":%s }}\n", \
    DUMP_BOOL(ptrP->isVideo), DUMP_BOOL(ptrP->isAudio), DUMP_BOOL(ptrP->isSubtitle) );
    /* fprintf(stderr, "{\"PLAYBACK_BUFFERING\":{\"percent\":%d}}\n", percent); g_playback_info.BufferingPercent */
}

static int ChangeSpeed(const gboolean result, const float fSpeed)
{
    if(result)
    {
        g_playback_info.isForwarding = 0;
        g_playback_info.BackWard     = 0;
        g_playback_info.SlowMotion   = 0;
        g_playback_info.Speed        = 1;
            
        int iSpeed = (int)fSpeed;
        int iRSpeed = (int) (1.0 / fSpeed);
        if(1 < iRSpeed)
        {
            /* SLOWMOTION */
            g_playback_info.SlowMotion = iRSpeed;
        }
        else if(1 < iSpeed)
        {
            /* FASTFORWARD */
            g_playback_info.isForwarding = 1;
            g_playback_info.Speed        = iSpeed;
        }
        else if(0 > iSpeed)
        {
            /* FASTBACKWARD */
            g_playback_info.BackWard     = iSpeed;
            
        }
        else if(1 != iSpeed)
        {
            /* This should never happen */
            //printf("ChangeSpeed - this should never happen fSpeed[%f]\n", fSpeed);
            backend_set_speed(1.0); /* force normal speed */
        }
        InfoStructChanged();
        return 0;
    }
    
    return -1;
}

static void gstAboutToFinishCallback(GstElement* object, gpointer userdata)
{
    //g_playback_info.isPlaying = 0;

    /* There is no need to wake up sleep
     * 1s delay is acceptable 
     */
}

static void gsElementAddedCallback(GstBin *bin, GstElement *element, gpointer data)
{

    gchar *elementname = gst_element_get_name(element);

    if (g_str_has_prefix(elementname, "queue2"))
    {
        if(g_download_buffer_path && strlen(g_download_buffer_path))
        {
            g_object_set(G_OBJECT(element), "temp-template", g_download_buffer_path, NULL);
        }
        else
        {
            g_object_set(G_OBJECT(element), "temp-template", NULL, NULL);
        }
    }
    else if (g_str_has_prefix(elementname, "uridecodebin")
        || g_str_has_prefix(elementname, "decodebin2"))
    {
        /*
         * Listen for queue2 element added to uridecodebin/decodebin2 as well.
         * Ignore other bins since they may have unrelated queues
         */
        g_signal_connect(element, "element-added", G_CALLBACK(gsElementAddedCallback), data);
    }
    g_free(elementname);
}

static void gstSourceChangedCallback(GObject *object, GParamSpec *pspec, gpointer data)
{
    if(!g_ptr_http_header_fields)
    {
        return;
    }
    
    GstElement* element = NULL;
    g_object_get(g_gst_playbin, "source", &element, NULL);
    if (element) 
    {
        // First check if the source element has a cookies property
        // of the format we expect
        GParamSpec* pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(element), "cookies");
        if (!pspec || G_TYPE_STRV != pspec->value_type)
        {
            gst_object_unref(element);
            return;
        }
        
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), "ssl-strict") != 0)
        {
            g_object_set(G_OBJECT(element), "ssl-strict", FALSE, NULL);
        }
       
        StrPair_t **headerField = g_ptr_http_header_fields;
        while(*headerField)
        {
            /*
            printf("headerField[%p]\n", (*headerField));
            printf("headerField->pKey [%s]\n", (*headerField)->pKey);
            printf("headerField->pVal [%s]\n", (*headerField)->pVal);
            */
            if(!strncmp((*headerField)->pKey, "proxy-id", 8))
            {
                g_object_set(element, "proxy-id", (*headerField)->pVal, NULL);
            }
            else if(!strncmp((*headerField)->pKey, "proxy-pw", 8))
            {
                g_object_set(element, "proxy-pw", (*headerField)->pVal, NULL);
            }
            else if(!strncmp((*headerField)->pKey, "proxy", 5))
            {
                g_object_set(element, "proxy", (*headerField)->pVal, NULL);
            }
            else if(!strncmp((*headerField)->pKey, "User-Agent", 10))
            {
                g_object_set(element, "user-agent", (*headerField)->pVal, NULL);
            }
            else if(!strncmp((*headerField)->pKey, "Cookie", 10))
            {
                gchar **cookies = g_strsplit((*headerField)->pVal, ",", -1);
                g_object_set (element, "cookies", cookies, NULL);
                g_strfreev (cookies);
            }
            else
            {
                GstStructure *extraHeaders = gst_structure_new("extra-headers", (*headerField)->pKey, G_TYPE_STRING, (*headerField)->pVal, 0);
                g_object_set(element, "extra-headers", extraHeaders, NULL);
                gst_structure_free(extraHeaders);
            }
            ++headerField;
        }
    }

    gst_object_unref(element);
}

static GstBusSyncReply gstBusSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
    if(0 < g_sfd)
    {
        int savedErrno = errno; /* In case we change 'errno' */
        write(g_sfd, "x", 1);   /* wake up main loop */
        errno = savedErrno;
    }
    
    return GST_BUS_PASS;
}

static gboolean gstBusCall(GstBus *bus, GstMessage *msg)
{     
    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_TAG: 
    {
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: 
    {
        /* We are only interested in state-changed messages from the pipeline */
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(g_gst_playbin))
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

            if(old_state != new_state)
            {
                // g_print ("\nPipeline state changed from %s to %s:\n",
                // gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                switch(new_state)
                {
                    case GST_STATE_PAUSED:
                    {
                        g_playback_info.isPaused = 1;
                        backend_query_duration(NULL);
                        break;
                    }
                    case GST_STATE_PLAYING:
                    {
                        backend_query_duration(NULL);
                        g_playback_info.isPaused = 0;
                        break;
                    }
                    case GST_STATE_NULL:
                    case GST_STATE_READY:
                    default:
                        break;
                }
                

                GstStateChange transition = (GstStateChange)GST_STATE_TRANSITION(old_state, new_state);
         
                switch(transition)
                {
                    case GST_STATE_CHANGE_NULL_TO_READY:
                    {
                    }
                        break;
                    case GST_STATE_CHANGE_READY_TO_PAUSED:
                    {
                        GstIterator *children = NULL;
                        if (g_audioSink)
                        {
                            gst_object_unref(GST_OBJECT(g_audioSink));
                            g_audioSink = NULL;
                        }
                        if (g_videoSink)
                        {
                            gst_object_unref(GST_OBJECT(g_videoSink));
                            g_videoSink = NULL;
                        }
                        children = gst_bin_iterate_recurse(GST_BIN(g_gst_playbin));
                        g_audioSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBAudioSink"));

                        gst_iterator_free(children);
                        children = gst_bin_iterate_recurse(GST_BIN(g_gst_playbin));
                        g_videoSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBVideoSink"));
                        gst_iterator_free(children);

                        //m_is_live = (gst_element_get_state(g_gst_playbin, NULL, NULL, 0LL) == GST_STATE_CHANGE_NO_PREROLL);
                    }    break;
                    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
                    {
         
                    }    break;
                    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
                    {
            
                    }    break;
                    case GST_STATE_CHANGE_PAUSED_TO_READY: 
                    {
                        if (g_audioSink)
                        {
                            gst_object_unref(GST_OBJECT(g_audioSink));
                            g_audioSink = NULL;
                        }
                        if (g_videoSink)
                        {
                            gst_object_unref(GST_OBJECT(g_videoSink));
                            g_videoSink = NULL;
                        }
                    }    break;
                    case GST_STATE_CHANGE_READY_TO_NULL:
                    {
                        g_playback_info.isPlaying = 0;
                    }   break;
                }
                InfoStructChanged();
            }
        }
        break;
    }
    case GST_MESSAGE_EOS:
        {
            g_playback_info.isPlaying = 0;
            InfoStructChanged();
            break;
        }
    case GST_MESSAGE_ERROR:
        {
            gchar *debug = NULL;
            GError *err  = NULL;

            gst_message_parse_error(msg, &err, &debug);
            //fprintf(stderr, "Debug: %s\n", debug);
            g_free(debug);
            
            fprintf(stderr, "{\"GST_ERROR\":{\"msg\":\"%s\",\"code\":%i}}\n", err->message, err->code);
            g_error_free(err);
            
            g_playback_info.isPlaying = 0;
            InfoStructChanged();
            break;
        }
    case GST_MESSAGE_DURATION:
    {
        break;
    }
    case GST_MESSAGE_ASYNC_DONE:
    {
        g_playback_info.AVSync = 1;
        TracksMessageAsyncDone();
        InfoStructChanged();
        backend_query_duration(NULL);
        backend_query_position(NULL);
        break;
    } 
    
    case GST_MESSAGE_ELEMENT:
    {
        if ( gst_is_missing_plugin_message(msg) )
        {
            gchar *description = gst_missing_plugin_message_get_description(msg);
            if( description )
            {
                g_debug("GStreamer plugin [%s] not available!\n", description);
                fprintf(stderr, "{\"GST_MISSING_PLUGIN\":{\"msg\":\"%s\"}}\n", description);
                g_free(description);
            }
        }
        else
        {
            const GstStructure *msgstruct = gst_message_get_structure(msg);
            if(NULL != msgstruct)
            {
                const gchar *eventname = gst_structure_get_name(msgstruct);
                if( eventname )
                {
                    if (!strcmp(eventname, "eventSizeChanged") || !strcmp(eventname, "eventSizeAvail"))
                    {
                        int aspect = 0;
                        int width  = 0;
                        int height = 0;
                        gst_structure_get_int (msgstruct, "aspect_ratio", &aspect);
                        gst_structure_get_int (msgstruct, "width", &width);
                        gst_structure_get_int (msgstruct, "height", &height);
                        //UpdateVideoTrackInf_1(aspect, width, height);
                        // printf("eventSizeChanged\n");
                    }
                    else if (!strcmp(eventname, "eventFrameRateChanged") || !strcmp(eventname, "eventFrameRateAvail"))
                    {
                        int framerate = 0;
                        gst_structure_get_int (msgstruct, "frame_rate", &framerate);
                        //UpdateVideoTrackInf_2((unsigned int)framerate);
                        // printf("eventFrameRateChanged framerate[%d]\n", framerate);

                    }
                    else if (!strcmp(eventname, "eventProgressiveChanged") || !strcmp(eventname, "eventProgressiveAvail"))
                    {
                        int progressive = 0;
                        gst_structure_get_int (msgstruct, "progressive", &progressive);
                        // printf("eventProgressiveChanged\n");
                    }
                }
            }
        }
        break;
    }
    case GST_MESSAGE_BUFFERING:
    {
        gint percent = 0;
        gst_message_parse_buffering (msg, &percent);
        g_playback_info.BufferingPercent = percent;
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
    {
        /* Get a new clock */
        gst_element_set_state(g_gst_playbin, GST_STATE_PAUSED);
        gst_element_set_state(g_gst_playbin, GST_STATE_PLAYING);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

void backend_gst_poll()
{              
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(g_gst_playbin));
    GstMessage *message = NULL;
    while((message = gst_bus_pop(bus)))
    {
        gstBusCall(bus, message);
        gst_message_unref (message);
    }
    gst_object_unref(bus);
}

const PlaybackInfo_t* backend_get_playback_info()
{
    return &g_playback_info;
}

void backend_init(int *argc, char **argv[], const int sfd)
{
    g_sfd = sfd;
    gst_init(argc, argv);
}

int backend_play(gchar *filename, gchar *download_buffer_path, guint64 ring_buffer_max_size, gint64 buffer_duration, gint buffer_size, StrPair_t **http_header_fields)
{
    backend_stop();
    g_filename               = filename;
    g_download_buffer_path   = download_buffer_path;
    g_ring_buffer_max_size   = ring_buffer_max_size;
    g_buffer_duration        = buffer_duration;
    g_buffer_size            = buffer_size;
    g_ptr_http_header_fields = http_header_fields;
    
    GstPlayFlags flags = g_playbin_flags;

    g_gst_playbin = gst_element_factory_make("playbin2", "gst-player"); //playbin
    if(g_gst_playbin)
    {
        g_signal_connect(g_gst_playbin, "about-to-finish", G_CALLBACK(gstAboutToFinishCallback), NULL);
        if( strstr(filename, "://") )
        {
            if(g_ptr_http_header_fields)
            {
                g_signal_connect(g_gst_playbin, "notify::source", G_CALLBACK(gstSourceChangedCallback), NULL);
            }
            
            if(0 < ring_buffer_max_size || 0 < buffer_duration || 0 < buffer_size)
            {
                if(g_download_buffer_path && strlen(g_download_buffer_path) && ring_buffer_max_size)
                {
                    /* use progressive download buffering */
                    flags |= GST_PLAY_FLAG_DOWNLOAD;
                    g_signal_connect(G_OBJECT(g_gst_playbin), "element-added", G_CALLBACK(gsElementAddedCallback), NULL);
                    /* limit file size */
                    g_object_set(g_gst_playbin, "ring-buffer-max-size", (guint64)(ring_buffer_max_size * 1024LL * 1024LL), NULL);
                }
                /*
                 * regardless whether or not we configured a progressive download file, use a buffer as well
                 * (progressive download might not work for all formats)
                 */
                flags |= GST_PLAY_FLAG_BUFFERING;
                /* increase the default 2 second / 2 MB buffer limitations to 5s / 5MB */
                if(buffer_duration > 0)
                {
                    g_object_set(G_OBJECT(g_gst_playbin), "buffer-duration", buffer_duration * GST_SECOND, NULL);
                }
                if(g_buffer_size > 0)
                {
                    /* buffer_size should be set in KB */
                    g_object_set(G_OBJECT(g_gst_playbin), "buffer-size", buffer_size*1024, NULL);
                }
            }
        }
        else
        {
            g_is_local_file = TRUE;
        }

        {
            GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(g_gst_playbin));
            // gst_bus_add_watch(bus, bus_call, NULL);
            gst_bus_set_sync_handler(bus, gstBusSyncHandler, NULL);
            gst_object_unref (bus);
        }

        {
            gchar *uri = NULL;

            if(gst_uri_is_valid(filename))
            {
                uri = g_strdup(filename);
            }
            else if(g_path_is_absolute(filename))
            {
                uri = g_filename_to_uri(filename, NULL, NULL);
            }
            else
            {
                gchar *tmp = NULL;
                tmp = g_build_filename(g_get_current_dir(), filename, NULL);
                uri = g_filename_to_uri(tmp, NULL, NULL);
                g_free(tmp);
            }

            g_debug ("%s", uri);
            g_object_set(G_OBJECT (g_gst_playbin), "uri", uri, NULL);
            
            g_object_set(G_OBJECT (g_gst_playbin), "flags", flags, NULL);
            g_free(uri);
        }

        GstStateChangeReturn sts = gst_element_set_state(g_gst_playbin, GST_STATE_PLAYING);
        if(sts)
        {
            g_playback_info.isPlaying = 1;
            g_playback_info.BufferingPercent = -1;
            ChangeSpeed(TRUE, 1.0);
            InfoStructChanged();
            return 0;
        }
    }
    return -1;
}

int backend_stop()
{
    int ret = 0;
    if (g_audioSink)
    {
        gst_object_unref(GST_OBJECT(g_audioSink));
        g_audioSink = NULL;
    }
    if (g_videoSink)
    {
        gst_object_unref(GST_OBJECT(g_videoSink));
        g_videoSink = NULL;
    }

    if(g_gst_playbin)
    {
        GstState st = GST_STATE_NULL;
        GstStateChangeReturn res = gst_element_set_state(g_gst_playbin, GST_STATE_NULL);
        res = gst_element_get_state(g_gst_playbin, &st, 0, 10 * GST_SECOND);
        
        /* We give 5s for close */
        if(GST_STATE_CHANGE_SUCCESS == res
           && GST_STATE_NULL == st)
        {
            gst_object_unref(GST_OBJECT(g_gst_playbin));
            g_gst_playbin = NULL;
        }
        else
        {
            ret = -1;
        }

    }
    
    memset(&g_playback_info, 0, sizeof(g_playback_info));
    InfoStructChanged();
    return ret;
}

int backend_pause()
{
    GstStateChangeReturn sts = gst_element_set_state(g_gst_playbin, GST_STATE_PAUSED);
    
    return (GST_STATE_CHANGE_FAILURE == sts) ? -1 : 0;
}

int backend_resume()
{
    if(g_playback_info.isForwarding)
    {
        backend_set_speed(1.0);
    }

    GstStateChangeReturn sts = gst_element_set_state(g_gst_playbin, GST_STATE_PLAYING);
    return (GST_STATE_CHANGE_FAILURE == sts) ? -1 : 0;
}

int backend_reset()
{
    gboolean result = gst_element_seek( g_gst_playbin, 1.0,
                                        GST_FORMAT_TIME,
                                        g_seek_flags,
                                        GST_SEEK_TYPE_SET, 0,
                                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE );
    return ChangeSpeed(result, 1.0);
}

int backend_seek(const double seconds)
{
    gint64 time_nanoseconds = (gint64)(seconds * GST_SECOND);
    gboolean result = gst_element_seek( g_gst_playbin, 1.0,
                                        GST_FORMAT_TIME,
                                        g_seek_flags,
                                        GST_SEEK_TYPE_CUR, time_nanoseconds,
                                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE );
    return ChangeSpeed(result, 1.0);
}

int backend_seek_absolute(const double seconds)
{
    gint64 time_nanoseconds = (gint64)(seconds * GST_SECOND);
    gboolean result = gst_element_seek( g_gst_playbin, 1.0,
                                        GST_FORMAT_TIME,
                                        g_seek_flags,
                                        GST_SEEK_TYPE_SET, time_nanoseconds,
                                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE );
                   

    memset(&g_eos_fix, 0, sizeof(g_eos_fix));
    return ChangeSpeed(result, 1.0);
}

int backend_set_speed(const float speed)
{
    GstSeekFlags seek_flags = 1.0 == speed ? g_seek_flags : GST_SEEK_FLAG_NONE;
    
    gboolean result = gst_element_seek( g_gst_playbin, speed,
                                        GST_FORMAT_TIME,
                                        seek_flags,
                                        GST_SEEK_TYPE_NONE, 0,
                                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE );
    return ChangeSpeed(result, speed);
}

int backend_query_position(int64_t *mseconds)
{
    if(mseconds)
    {
        *mseconds = 0;
    }
    
    if (!g_gst_playbin || !g_playback_info.isPlaying)
    {
        return -1;
    }
    
    gboolean result = FALSE;
    GstState st;
    GstStateChangeReturn res = gst_element_get_state(g_gst_playbin, &st, 0, 0);
    //fprintf(stderr, "{gst_element_get_state res[%d] st[%d] GST_STATE_CHANGE_SUCCESS[%d], GST_STATE_PLAYING[%d]\n}", (int)res, (int)st, GST_STATE_CHANGE_SUCCESS, GST_STATE_PLAYING) ;
    if(GST_STATE_CHANGE_SUCCESS == res
       && GST_STATE_PLAYING == st)
    {
        GstFormat format    = GST_FORMAT_TIME;
        gint64 playbin_time = GST_CLOCK_TIME_NONE;
        gint64 decoder_time = GST_CLOCK_TIME_NONE;
        
        result = gst_element_query_position(g_gst_playbin, &format, &playbin_time);
        if(!result)
        {
            playbin_time = 0;
        }
        /*
        else
        {
            test(playbin_time);
        }
        */
        
        if ( g_audioSink || g_videoSink)
        {
            g_signal_emit_by_name(g_audioSink?g_audioSink:g_videoSink, "get-decoder-time", &decoder_time);
            
            /* EOS fix start */
            gint64 timestamp = getTimestamp();
            if(0 == g_eos_fix.check_timestamp) g_eos_fix.check_timestamp = timestamp;
            
            //fprintf(stderr, "{d:%lld p:%lld}\n", decoder_time, playbin_time);
            if(decoder_time  == g_eos_fix.decoder_time && 
               playbin_time  == g_eos_fix.playbin_time )
            {
                if(3000 < (timestamp-g_eos_fix.check_timestamp) )
                {
                    backend_stop();
                }
            }
            else
            {
                g_eos_fix.check_timestamp = timestamp;
                g_eos_fix.decoder_time    = decoder_time;
                g_eos_fix.playbin_time    = playbin_time;
            }
            /* EOS fix end */
        }
        else
        {
            decoder_time = playbin_time;
        }
        
        //fprintf(stderr, "{p:%lld result[%d]}\n", playbin_time, result);
        if (result && GST_FORMAT_TIME == format)
        {
            result = FALSE;
            if(GST_CLOCK_TIME_IS_VALID(decoder_time))
            {
                /* validate time from decoder */
                gint64 diff = (decoder_time > playbin_time) ? decoder_time - playbin_time : playbin_time - decoder_time;
                if( GST_TIME_AS_SECONDS(diff) < 180) 
                {
                    result = TRUE;
                    if(mseconds)
                    {
                        *mseconds = (int)GST_TIME_AS_MSECONDS(decoder_time);
                    }
                    else
                    {
                        fprintf(stderr, "{\"J\":{\"ms\":%lld}}\n", (int64_t)GST_TIME_AS_MSECONDS(decoder_time));
                    }
                }
            }
        }
    }
    return result ? 0 : -1;
}

int backend_query_duration(double *length)
{
    double tmpLength = 0;
    GstFormat format = GST_FORMAT_TIME;
    gint64 duration = GST_CLOCK_TIME_NONE;
    gboolean result;

    result = gst_element_query_duration (g_gst_playbin, &format, &duration);
    if (!result || format != GST_FORMAT_TIME)
    {
        if(length)
        {
            *length = 0;
        }
        return -1;
    }
    g_duration = duration;
    tmpLength = duration / ((double) GST_SECOND);
    if(!length)
    {
        fprintf(stderr, "{\"PLAYBACK_LENGTH\":{\"length\":%lf, \"sts\":0}}\n", tmpLength);
        /* To do: fill audio list, video list .etc */
    }
    else
    {
        *length = tmpLength;
    }        
    return 0;
}

void backend_deinit()
{
    backend_stop();
    g_sfd = -1;
}

int backend_set_download_timeout(const uint64_t mseconds)
{
    // n/a
    return 0;
}

int backend_set_is_live(const uint8_t live)
{
    // n/a
    return 0;
}
/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Philippe Khalaf <burger@speedy.org>
 *
 * GstIFDSrc.c:
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/**
 * SECTION:element-fdsrc
 * @see_also: #GstFdSink
 *
 * Read data from a unix file descriptor.
 *
 * To generate data, enter some data on the console followed by enter.
 * The above mentioned pipeline should dump data packets to the console.
 *
 * If the #GstIFDSrc:timeout property is set to a value bigger than 0, fdsrc will
 * generate an element message named
 * <classname>&quot;GstIFDSrcTimeout&quot;</classname>
 * if no data was received in the given timeout.
 * The message's structure contains one field:
 * <itemizedlist>
 * <listitem>
 *   <para>
 *   #guint64
 *   <classname>&quot;timeout&quot;</classname>: the timeout in microseconds that
 *   expired when waiting for data.
 *   </para>
 * </listitem>
 * </itemizedlist>
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * echo "Hello GStreamer" | gst-launch -v fdsrc ! fakesink dump=true
 * ]| A simple pipeline to read from the standard input and dump the data
 * with a fakesink as hex ascii block.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sys/types.h>

#ifdef G_OS_WIN32
#include <io.h>                 /* lseek, open, close, read */
#undef lseek
#define lseek _lseeki64
#undef off_t
#define off_t guint64
#endif

#include <sys/stat.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _MSC_VER
#undef stat
#define stat _stat
#define fstat _fstat
#define S_ISREG(m)	(((m)&S_IFREG)==S_IFREG)
#endif
#include <stdlib.h>
#include <errno.h>

#include "gstifdsrc.h"

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (ifdsrc_debug);
#define GST_CAT_DEFAULT ifdsrc_debug

#define DEFAULT_FD              0
#define DEFAULT_TIMEOUT         0

enum
{
  PROP_0,

  PROP_FD,
  PROP_TIMEOUT,
  PROP_IS_LIVE,
  
  PROP_LAST
};

static void ifdsrc_uri_handler_init (gpointer g_iface, gpointer iface_data);

#define _do_init \
  G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, ifdsrc_uri_handler_init); \
  GST_DEBUG_CATEGORY_INIT (ifdsrc_debug, "ifdsrc", 0, "ifdsrc element");
#define ifdsrc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstIFDSrc, ifdsrc, GST_TYPE_PUSH_SRC, _do_init);

static void ifdsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void ifdsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void ifdsrc_dispose (GObject * obj);

static gboolean ifdsrc_start (GstBaseSrc * bsrc);
static gboolean ifdsrc_stop (GstBaseSrc * bsrc);
static gboolean ifdsrc_unlock (GstBaseSrc * bsrc);
static gboolean ifdsrc_unlock_stop (GstBaseSrc * bsrc);
static gboolean ifdsrc_is_seekable (GstBaseSrc * bsrc);
static gboolean ifdsrc_get_size (GstBaseSrc * src, guint64 * size);
static gboolean ifdsrc_do_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean ifdsrc_query (GstBaseSrc * src, GstQuery * query);

static GstFlowReturn ifdsrc_create (GstPushSrc * psrc, GstBuffer ** outbuf);

static guint64 get_timestamp()
{
#if 0
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
#else
    struct timespec tsnow;
    if(0 == clock_gettime(CLOCK_MONOTONIC, &tsnow))
    {
        return (guint64)tsnow.tv_sec * 1000 + tsnow.tv_nsec / 1000000L;
    }
    return 0;
#endif
}

static void
ifdsrc_class_init (GstIFDSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpush_src_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstpush_src_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->set_property = ifdsrc_set_property;
  gobject_class->get_property = ifdsrc_get_property;
  gobject_class->dispose = ifdsrc_dispose;

  g_object_class_install_property (gobject_class, PROP_FD,
      g_param_spec_int ("ifd", "ifd", "An open file descriptor to read from",
          0, G_MAXINT, DEFAULT_FD, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
          
   g_object_class_install_property (gobject_class, PROP_IS_LIVE,
         g_param_spec_boolean ("is_live",
        "Is live",
        "Is live stream (TRUE = live stream not seekable)",
        FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
  
  /**
   * GstIFDSrc:timeout
   *
   * Post a message after timeout microseconds
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMEOUT,
      g_param_spec_uint64 ("timeout", "Timeout",
          "Post a message after timeout microseconds (0 = disabled)", 0,
          G_MAXUINT64, DEFAULT_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "Filedescriptor Source",
      "Source/File",
      "Read from a file descriptor", "Erik Walthinsen <omega@cse.ogi.edu>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (ifdsrc_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (ifdsrc_stop);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (ifdsrc_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (ifdsrc_unlock_stop);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (ifdsrc_is_seekable);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (ifdsrc_get_size);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (ifdsrc_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (ifdsrc_query);

  gstpush_src_class->create = GST_DEBUG_FUNCPTR (ifdsrc_create);
}

static void
ifdsrc_init (GstIFDSrc * fdsrc)
{
  fdsrc->new_fd = DEFAULT_FD;
  fdsrc->seekable_fd = FALSE;
  fdsrc->fd = -1;
  fdsrc->size = -1;
  fdsrc->timeout = DEFAULT_TIMEOUT;
  fdsrc->uri = g_strdup_printf ("ifd://0");
  fdsrc->curoffset = 0;
  fdsrc->seek_failed = FALSE;
  fdsrc->is_live = FALSE;
}

static void
ifdsrc_dispose (GObject * obj)
{
  GstIFDSrc *src = ifdsrc (obj);

  g_free (src->uri);
  src->uri = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
ifdsrc_update_fd (GstIFDSrc * src, guint64 size)
{
  struct stat stat_results;

  GST_DEBUG_OBJECT (src, "ifdset %p, old_fd %d, new_fd %d", src->fdset, src->fd,
      src->new_fd);
  //gst_base_src_set_automatic_eos (GST_BASE_SRC (src), FALSE);

  /* we need to always update the fdset since it may not have existed when
   * ifdsrc_update_fd () was called earlier */
   /*
  if (src->fdset != NULL) {
    GstPollFD fd = GST_POLL_FD_INIT;

    if (src->fd >= 0) {
      fd.fd = src->fd;
      // this will log a harmless warning, if it was never added 
      gst_poll_remove_fd (src->fdset, &fd);
    }

    fd.fd = src->new_fd;
    gst_poll_add_fd (src->fdset, &fd);
    gst_poll_fd_ctl_read (src->fdset, &fd, TRUE);
  }
  */


  if (src->fd != src->new_fd) {
    GST_INFO_OBJECT (src, "Updating to fd %d", src->new_fd);

    src->fd = src->new_fd;

    GST_INFO_OBJECT (src, "Setting size to fd %" G_GUINT64_FORMAT, size);
    src->size = size;

    g_free (src->uri);
    src->uri = g_strdup_printf ("ifd://%d", src->fd);

    if (fstat (src->fd, &stat_results) < 0)
      goto not_seekable;

    if (!S_ISREG (stat_results.st_mode))
      goto not_seekable;

    /* Try a seek of 0 bytes offset to check for seekability */
    if (lseek (src->fd, 0, SEEK_CUR) < 0)
      goto not_seekable;

    GST_INFO_OBJECT (src, "marking fd %d as seekable", src->fd);
    src->seekable_fd = TRUE;

    gst_base_src_set_dynamic_size (GST_BASE_SRC (src), TRUE);
  }
  return;

not_seekable:
  {
    GST_INFO_OBJECT (src, "marking fd %d as NOT seekable", src->fd);
    src->seekable_fd = FALSE;
    gst_base_src_set_dynamic_size (GST_BASE_SRC (src), FALSE);
  }
}

static gboolean
ifdsrc_start (GstBaseSrc * bsrc)
{
  GstIFDSrc *src = ifdsrc (bsrc);

  src->curoffset = 0;

  if ((src->fdset = gst_poll_new (TRUE)) == NULL)
    goto socket_pair;

  ifdsrc_update_fd (src, -1);

  return TRUE;

  /* ERRORS */
socket_pair:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
        GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
ifdsrc_stop (GstBaseSrc * bsrc)
{
  GstIFDSrc *src = ifdsrc (bsrc);

  if (src->fdset) {
    gst_poll_free (src->fdset);
    src->fdset = NULL;
  }

  return TRUE;
}

static gboolean
ifdsrc_unlock (GstBaseSrc * bsrc)
{
  GstIFDSrc *src = ifdsrc (bsrc);

  //printf("Flushing >>>>>>>>>>>>>>>\n");
  GST_LOG_OBJECT (src, "Flushing");
  GST_OBJECT_LOCK (src);
  gst_poll_set_flushing (src->fdset, TRUE);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static gboolean
ifdsrc_unlock_stop (GstBaseSrc * bsrc)
{
  GstIFDSrc *src = ifdsrc (bsrc);

  //printf("No longer flushing >>>>>>>>>>>>>>>\n");
  GST_LOG_OBJECT (src, "No longer flushing");
  GST_OBJECT_LOCK (src);
  gst_poll_set_flushing (src->fdset, FALSE);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static void
ifdsrc_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstIFDSrc *src = ifdsrc (object);

  switch (prop_id) {
    case PROP_FD:
      src->new_fd = g_value_get_int (value);

      /* If state is ready or below, update the current fd immediately
       * so it is reflected in get_properties and uri */
      GST_OBJECT_LOCK (object);
      if (GST_STATE (GST_ELEMENT (src)) <= GST_STATE_READY) {
        GST_DEBUG_OBJECT (src, "state ready or lower, updating to use new fd");
        ifdsrc_update_fd (src, -1);
      } else {
        GST_DEBUG_OBJECT (src, "state above ready, not updating to new fd yet");
      }
      GST_OBJECT_UNLOCK (object);
      break;
    case PROP_TIMEOUT:
      src->timeout = g_value_get_uint64 (value);
      GST_DEBUG_OBJECT (src, "poll timeout set to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (src->timeout));
      break;
    case PROP_IS_LIVE:
      src->is_live = g_value_get_boolean (value);
      GST_DEBUG ("is_live set to [%d]", src->is_live);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ifdsrc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstIFDSrc *src = ifdsrc (object);

  switch (prop_id) {
    case PROP_FD:
      g_value_set_int (value, src->fd);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint64 (value, src->timeout);
      break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, src->is_live);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
ifdsrc_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstIFDSrc *src    = 0;
  GstBuffer *buf   = 0;
  gssize readbytes = 0;
  guint blocksize  = 0;
  GstMapInfo info;

  gboolean try_again = FALSE;
  gint retval = 0;

  src = ifdsrc (psrc);

  blocksize = GST_BASE_SRC (src)->blocksize;

  /* create the buffer */
  buf = gst_buffer_new_allocate (NULL, blocksize, NULL);
  if (G_UNLIKELY (buf == NULL))
    goto alloc_failed;

  gst_buffer_map (buf, &info, GST_MAP_WRITE);

  
  //do {
  //  readbytes = read (src->fd, info.data, blocksize);
  //  GST_LOG_OBJECT (src, "read %" G_GSSIZE_FORMAT, readbytes);
  //} while (readbytes == -1 && errno == EINTR);  /* retry if interrupted */
  if (!src->seek_failed && src->timeout > 0)
  {
      guint64 timestamp = get_timestamp();
      do 
      {
        try_again = FALSE;

        retval = gst_poll_wait (src->fdset, 10000*GST_USECOND);
        GST_LOG_OBJECT (src, "poll returned %d", retval);

        if (G_UNLIKELY (retval == -1)) 
        {
          if (errno == EINTR || errno == EAGAIN) 
          {
            /* retry if interrupted */
            try_again = TRUE;
          }
          else if (errno == EBUSY)
          {
            goto stopped;
          } 
          else 
          {
            goto poll_error;
          }
        }
        else if (G_UNLIKELY (retval == 0)) 
        {
            if (!src->seek_failed)
            {
                do
                {
                    readbytes = read (src->fd, info.data, blocksize);
                } while(readbytes == -1 && errno == EINTR); /* retry if interrupted */
                //printf("timeout [%llu] < [%llu]\n", get_timestamp() - timestamp,  src->timeout);
                if (0 == readbytes && (get_timestamp() - timestamp) < src->timeout)
                {
                    try_again = TRUE;
                }
            }
        }
      } while (G_UNLIKELY (try_again)); /* retry if interrupted or timeout */
  }
  else
  {
    do
    {
        readbytes = read (src->fd, info.data, blocksize);
    } while(readbytes == -1 && errno == EINTR); /* retry if interrupted */
  }
  
  if (src->seek_failed)
  {
    readbytes = 0;
  }
  
  if (readbytes < 0)
    goto read_error;

  gst_buffer_unmap (buf, &info);
  gst_buffer_resize (buf, 0, readbytes);

  if (readbytes == 0)
    goto eos;

  GST_BUFFER_OFFSET (buf) = src->curoffset;
  GST_BUFFER_TIMESTAMP (buf) = GST_CLOCK_TIME_NONE;
  src->curoffset += readbytes;

  GST_LOG_OBJECT (psrc, "Read buffer of size %" G_GSSIZE_FORMAT, readbytes);

  /* we're done, return the buffer */
  *outbuf = buf;

  return GST_FLOW_OK;

  /* ERRORS */
#ifndef HAVE_WIN32
poll_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("poll on file descriptor: %s.", g_strerror (errno)));
    GST_DEBUG_OBJECT (psrc, "Error during poll");
    return GST_FLOW_ERROR;
  }
stopped:
  {
    GST_DEBUG_OBJECT (psrc, "Poll stopped");
    return GST_FLOW_FLUSHING;
  }
#endif
alloc_failed:
  {
    GST_ERROR_OBJECT (src, "Failed to allocate %u bytes", blocksize);
    return GST_FLOW_ERROR;
  }
eos:
  {
    GST_DEBUG_OBJECT (psrc, "Read 0 bytes. EOS.");
    gst_buffer_unref (buf);
    //printf("Read 0 bytes. EOS. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    return GST_FLOW_EOS;
  }
read_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("read on file descriptor: %s.", g_strerror (errno)));
    GST_DEBUG_OBJECT (psrc, "Error reading from fd");
    gst_buffer_unmap (buf, &info);
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static gboolean
ifdsrc_query (GstBaseSrc * basesrc, GstQuery * query)
{
  gboolean ret = FALSE;
  GstIFDSrc *src = ifdsrc (basesrc);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->uri);
      ret = TRUE;
      break;
    default:
      ret = FALSE;
      break;
  }

  if (!ret)
    ret = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);

  return ret;
}

static gboolean
ifdsrc_is_seekable (GstBaseSrc * bsrc)
{
  GstIFDSrc *src = ifdsrc (bsrc);

  return src->seekable_fd;
}

static gboolean
ifdsrc_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstIFDSrc *src = ifdsrc (bsrc);
  struct stat stat_results;

  //if (src->size != -1) {
  //  *size = src->size;
  //  return TRUE;
  //}

  if (!src->seekable_fd) {
    /* If it isn't seekable, we won't know the length (but fstat will still
     * succeed, and wrongly say our length is zero. */
    return FALSE;
  }

  if (fstat (src->fd, &stat_results) < 0)
    goto could_not_stat;

  *size = stat_results.st_size;
  if (src->timeout > 0)
  {
    *size += 1;
  }
  src->size = *size;
  //printf(">>>>>>>>>>>>> size [%llu]\n", *size);

  return TRUE;

  /* ERROR */
could_not_stat:
  {
    return FALSE;
  }
}

static gboolean
ifdsrc_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  gint res;
  gint64 offset;
  GstIFDSrc *src = ifdsrc (bsrc);

  offset = segment->start;

  /* No need to seek to the current position */
  if (offset == src->curoffset)
  {
    return TRUE;
  }
  
  if (src->is_live || (src->timeout > 0 && offset >= src->size))
  {
     src->seek_failed = TRUE;
     return TRUE;
  }
  
  res = lseek (src->fd, offset, SEEK_SET);
  if (G_UNLIKELY (res < 0 || res != offset))
  {
    if (src->timeout > 0)
    {
        src->seek_failed = TRUE;
    }
    else
    {
        goto seek_failed;
    }
  }

  segment->position = segment->start;
  segment->time = segment->start;

  return TRUE;

seek_failed:
  src->seek_failed = TRUE;
  GST_DEBUG_OBJECT (src, "lseek returned %" G_GINT64_FORMAT, offset);
  return FALSE;
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
ifdsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
ifdsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "ifd", NULL };

  return protocols;
}

static gchar *
ifdsrc_uri_get_uri (GstURIHandler * handler)
{
  GstIFDSrc *src = ifdsrc (handler);

  /* FIXME: make thread-safe */
  return g_strdup (src->uri);
}

static gboolean
ifdsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** err)
{
  gchar *protocol, *q;
  GstIFDSrc *src = ifdsrc (handler);
  gint fd;
  guint64 size = (guint64) - 1;

  GST_INFO_OBJECT (src, "checking uri %s", uri);

  protocol = gst_uri_get_protocol (uri);
  if (strcmp (protocol, "ifd") != 0) {
    g_set_error (err, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Wrong protocol for fdsrc in uri: '%s'", uri);
    g_free (protocol);
    return FALSE;
  }
  g_free (protocol);

  if (sscanf (uri, "ifd://%d", &fd) != 1 || fd < 0) {
    g_set_error (err, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Bad file descriptor number in uri: '%s'", uri);
    return FALSE;
  }

  if ((q = g_strstr_len (uri, -1, "?"))) {
    gchar *sp;

    GST_INFO_OBJECT (src, "found ?");

    if ((sp = g_strstr_len (q, -1, "size="))) {
      if (sscanf (sp, "size=%" G_GUINT64_FORMAT, &size) != 1) {
        GST_INFO_OBJECT (src, "parsing size failed");
        size = -1;
      } else {
        GST_INFO_OBJECT (src, "found size %" G_GUINT64_FORMAT, size);
      }
    }
  }

  src->new_fd = fd;

  GST_OBJECT_LOCK (src);
  if (GST_STATE (GST_ELEMENT (src)) <= GST_STATE_READY) {
    ifdsrc_update_fd (src, size);
  }
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static void
ifdsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = ifdsrc_uri_get_type;
  iface->get_protocols = ifdsrc_uri_get_protocols;
  iface->get_uri = ifdsrc_uri_get_uri;
  iface->set_uri = ifdsrc_uri_set_uri;
}




/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "ifdsrc", GST_RANK_NONE, ifdsrc_get_type());
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "ifdsrc"
#endif

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'IPTVPlayer ifdsrc' with your plugin description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    plugin,
    "IPTVPlayer ifdsrc",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
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

#ifndef GST_BACKEND_H
#define GST_BACKEND_H

#include <stdint.h>
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>

#define DUMP_BOOL(x) 0 == x ? "false"  : "true"

typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

typedef struct PlaybackInfo_s 
{ 
    unsigned char isReady;
    unsigned char isPlaying;
    unsigned char isPaused;
    unsigned char isForwarding;
    unsigned char isSeeking;
    unsigned char isCreationPhase;
 
    float BackWard;
    int   SlowMotion;
    int   Speed;
    int   AVSync;
    int   BufferingPercent;
    
    unsigned char isVideo;
    unsigned char isAudio;
    unsigned char isSubtitle;


} PlaybackInfo_t;

typedef struct StrPair_s
{
    char *pKey;
    char *pVal;
}StrPair_t;

typedef struct TrackDescription_s 
{
    int                   Id;
    char *                Name;
    char *                Encoding;
    unsigned int          frame_rate;
    int                   width;
    int                   height;
    
} TrackDescription_t;

void backend_init(int *argc, char **argv[], const int sfd);
void backend_deinit();
int  backend_play(gchar *filename, gchar *download_buffer_path, guint64 ring_buffer_max_size, gint64 buffer_duration, gint buffer_size, StrPair_t **http_header_fields);
int  backend_stop();
int  backend_seek(const double seconds);
int  backend_seek_absolute(const double seconds);
int  backend_set_speed(const float speed);
int  backend_reset();
int  backend_pause();
int  backend_resume();
int  backend_query_position(int64_t *ms);
int  backend_query_duration(double *length);
TrackDescription_t* backend_get_tracks_list(const char type, int *num);
TrackDescription_t* backend_get_current_track(const char type);
int backend_set_track(const char type, const int id);
const PlaybackInfo_t* backend_get_playback_info();
void backend_gst_poll();
int backend_set_download_timeout(const uint64_t mseconds);
int backend_set_is_live(const uint8_t live);

#endif /* GST_BACKEND_H */

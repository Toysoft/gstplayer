#ifndef GST_TRACKS_COMMON_FUNCTIONS
#define GST_TRACKS_COMMON_FUNCTIONS

#include <malloc.h>
#include "gst-backend.h"


/* Track */
int tracksReady = 0;
TrackDescription_t *g_audio_tracks = NULL;
TrackDescription_t *g_video_tracks = NULL;
int g_audio_num = 0;
int g_video_num = 0;
int g_audio_idx = -1;
int g_video_idx = -1;

static void UpdateVideoTrackInf()
{
    backend_get_current_track('v');
}

static TrackDescription_t* GetVideoTrackForUpdate()
{
    if(g_video_tracks && g_video_idx < g_video_num && g_video_idx >= 0)
    {
        return &g_video_tracks[g_video_idx];
    }
    return NULL;
}

void UpdateVideoTrackInf_1(const int aspect, const int width, const int height)
{
    TrackDescription_t *pVidTrack = GetVideoTrackForUpdate();
    if(pVidTrack)
    {
        int updated = 0;
        if(pVidTrack->width != width)
        {
            pVidTrack->width = width;
            updated = 1;
        }
        if(pVidTrack->height != height)
        {
            pVidTrack->height = height;
            updated = 1;
        }
        if(updated)
        {
            UpdateVideoTrackInf();
        }
    }
}

void UpdateVideoTrackInf_2(const unsigned int framerate)
{
    TrackDescription_t *pVidTrack = GetVideoTrackForUpdate();
    if(pVidTrack)
    {
        int updated = 0;
        if(pVidTrack->frame_rate != framerate)
        {
            pVidTrack->frame_rate = framerate;
            updated = 1;
        }
        if(updated)
        {
            UpdateVideoTrackInf();
        }
    }
}

static int GetCurrentTrack(const char *type, int *idx)
{
    //if (*idx == -1)
    {
        g_object_get(G_OBJECT (g_gst_playbin), type, idx, NULL);
    }
    return *idx;
}

static int SelectAudioStream(int i)
{
    int current_audio;
    g_object_set (G_OBJECT (g_gst_playbin), "current-audio", i, NULL);
    g_object_get (G_OBJECT (g_gst_playbin), "current-audio", &current_audio, NULL);
    g_audio_idx = current_audio;
    if ( current_audio == i )
    {
        return 0;
    }
    return -1;
}

static int SelectAudioTrack(unsigned int i)
{
    int ret = -1;
    if (g_audio_num > 0)
    {
        int validposition = 0;
        int64_t ppos = 0;
        
        if (0 == backend_query_position(&ppos))
        {
            validposition = 1;
            ppos -= 1000;
            if (ppos < 0)
            {
                ppos = 0;
            }
        }

        ret = SelectAudioStream(i);
        if (!ret)
        {
            if (validposition)
            {
                /* flush */
                double dpos = ppos/1000.0;
                backend_seek_absolute(dpos);
            }
        }
    }
    else
    {
        g_audio_idx = i;
        ret = 0;
    }
    return ret;
}

static int SetStr(char **set, const char *val)
{
    int ret = -1;
    if (NULL != set && NULL != val)
    {
        if (NULL != (*set))
        {
            free(*set);
        }
        *set = strdup(val);
        ret = 0;
    }
    return ret;
}

static void FillAudioTracks()
{
    gint i = 0;
    gint n_audio = 0;

    g_object_get(g_gst_playbin, "n-audio", &n_audio, NULL);
    
    //m_audioStreams.clear();
    if (NULL != g_audio_tracks)
    {
        return;
    }
    
    g_audio_tracks = malloc(sizeof(TrackDescription_t) * n_audio);
    memset(g_audio_tracks, 0, sizeof(TrackDescription_t) * n_audio);
    
    int j = 0;
    for (i = 0; i < n_audio; i++)
    {
        gchar *g_codec   = NULL;
        gchar *g_lang    = NULL;
        GstTagList *tags = NULL;
        GstPad* pad = 0;
        g_signal_emit_by_name (g_gst_playbin, "get-audio-pad", i, &pad);
#if GST_VERSION_MAJOR < 1
        GstCaps* caps = gst_pad_get_negotiated_caps(pad);
#else
        GstCaps* caps = gst_pad_get_current_caps(pad);
#endif
        if (!caps)
        {
            continue;
        }
        GstStructure* str = gst_caps_get_structure(caps, 0);
        const gchar *g_type = gst_structure_get_name(str);
        
        TrackDescription_t *track = &g_audio_tracks[j];
        track->Id = i;
        SetStr(&(track->Name), "und");
        SetStr(&(track->Encoding), (char *)g_type);
        
        g_codec = NULL;
        g_lang = NULL;
        g_signal_emit_by_name (g_gst_playbin, "get-audio-tags", i, &tags);
#if GST_VERSION_MAJOR < 1
        if (tags && gst_is_tag_list(tags))
#else
        if (tags && GST_IS_TAG_LIST(tags))
#endif
        {
            if (gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &g_codec))
            {
                SetStr(&(track->Encoding), (char *)g_codec);
                g_free(g_codec);
            }
            if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang))
            {
                SetStr(&(track->Name), (char *)g_lang);
                g_free(g_lang);
            }
            gst_tag_list_free(tags);
        }
        gst_caps_unref(caps);
        ++j;
    }
    g_audio_num = j;
}

static void FillVideoTracks() 
{ 
    GstPad *videopad = NULL; 
    int n_video = 0; 
    int i = 0; 
    int j = 0;
    g_object_get(g_gst_playbin, "n-video", &n_video, NULL);

    if (NULL != g_video_tracks)
    {
        return;
    }
    
    if (n_video > 0) 
    { 
        g_video_tracks = malloc(sizeof(TrackDescription_t) * n_video);
        memset(g_video_tracks, 0, sizeof(TrackDescription_t) * n_video);
        
        for (i = 0; i < n_video; i++) 
        {
            g_signal_emit_by_name(g_gst_playbin, "get-video-pad", i, &videopad); 
            if (videopad) 
            { 
#if GST_VERSION_MAJOR < 1
                GstCaps* caps = gst_pad_get_negotiated_caps(videopad);
#else
                GstCaps* caps = gst_pad_get_current_caps(videopad);
#endif
                if (caps)
                {
                    GstStructure *str = gst_caps_get_structure (caps, 0); 
                    if(str)
                    {
                        const gchar *g_type = gst_structure_get_name(str);
                        int num = 0;
                        int denom = 0;
                        
                        TrackDescription_t *track = &g_video_tracks[j];
                        track->Id = i;
                        SetStr(&(track->Name), "und");
                        SetStr(&(track->Encoding), (char *)g_type);
                        
                        gst_structure_get_int(str, "width",  &track->width); 
                        gst_structure_get_int(str, "height", &track->height);
                        if(gst_structure_get_fraction(str, "framerate", &num, &denom) && num > 0 && denom > 0)
                        {
                            if(denom == 1001)
                            {
                                denom = 1000;
                            }
                            track->frame_rate = num * 1000 / denom;
                        }
                        ++j;
                    }
                    gst_caps_unref(caps);
                }
                gst_object_unref (videopad); 
            }
        }
        g_video_num = j;
    } 
} 

static void TracksMessageAsyncDone()
{
    if (0 == g_audio_num)
    {
        int audio_idx = g_audio_idx;
        FillAudioTracks();
        backend_get_tracks_list('a', NULL);
        GetCurrentTrack("current-audio", &g_audio_idx);
        if (audio_idx >= 0 && audio_idx != g_audio_idx)
        {
            backend_set_track('a', audio_idx);
            //SelectAudioStream(audio_idx);
        }
        backend_get_current_track('a');
    }
    
    if (0 == g_video_num)
    {
        int video_idx = g_video_idx;
        FillVideoTracks();
        backend_get_tracks_list('v', NULL);
        GetCurrentTrack("current-video", &g_video_idx);
        if (video_idx >= 0 && video_idx != g_video_idx)
        {
            backend_set_track('v', video_idx);
        }
        backend_get_current_track('v');
    }
}

TrackDescription_t* backend_get_tracks_list(const char type, int *num)
{
    int localNum = 0;
    TrackDescription_t *pTracks = NULL;
    /* At now we need only audio track list */
    if ('a' == type)
    {
        pTracks = g_audio_tracks;
        localNum = g_audio_num;
    }
    
    if (NULL != num)
    {
        *num = localNum;
    }
    
    if (NULL != pTracks)
    {
        int i = 0;
        fprintf(stderr, "{\"%c_l\": [", type);
        for (i = 0; i < localNum; ++i) 
        {
            if(0 < i)
            {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "{\"id\":%d,\"e\":\"%s\",\"n\":\"%s\"}", pTracks[i].Id , pTracks[i].Encoding, pTracks[i].Name);
        }
        fprintf(stderr, "]}\n");
    }
    
    return pTracks; 
}

TrackDescription_t* backend_get_current_track(const char type)
{
    int idx = -1;
    int num = 0;
    TrackDescription_t *pTracks = NULL;
    
    TrackDescription_t *track = NULL;
    if ('a' == type)
    {
        pTracks = g_audio_tracks;
        idx     = GetCurrentTrack("current-audio", &g_audio_idx);
        num     = g_audio_num;
    }
    else if ('v' == type)
    {
        pTracks = g_video_tracks;
        idx     = GetCurrentTrack("current-video", &g_video_idx);
        num     = g_video_num;
    }
    
    if (idx >= 0 && idx < num && NULL != pTracks)
    {
        track = &pTracks[idx];
        if ('a' == type)
        {
            fprintf(stderr, "{\"%c_%c\":{\"id\":%d,\"e\":\"%s\",\"n\":\"%s\"}}\n", type, 'c', track->Id , track->Encoding, track->Name);
        }
        else // video
        {
            // information about only current video track will be stored
            fprintf(stderr, "{\"%c_%c\":{\"id\":%d,\"e\":\"%s\",\"n\":\"%s\",\"w\":%d,\"h\":%d,\"f\":%u}}\n", type, 'c', track->Id , track->Encoding, track->Name, track->width, track->height, track->frame_rate);
        }
    }
    return track; 
}

int backend_set_track(const char type, const int id)
{
    int ret = -1;
    if ('a' == type)
    {
        ret = SelectAudioTrack(id);
    }
    return ret;
}

#else
#error
#endif //GST_TRACKS_COMMON_FUNCTIONS
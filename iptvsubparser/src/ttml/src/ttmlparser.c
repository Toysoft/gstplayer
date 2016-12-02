#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "expat.h"
#include "subtitles.h"

static int ParseTimingValueTTML(int64_t *timing_value, const char *s)
{
    static const int factors[3] = {100, 10, 1};
    int h1 = 0;
    int m1 = 0;
    int s1 = 0;
    int d1 = 0;
    char fraction[4] = {'\0'};
    int factor = 0;
    int status = -1;
    
    /* Try clock-time format */
    /* TODO: handle format hours:minutes:seconds:frames (e.g. "00:07:15:06") */
    /* hours:minutes:seconds.fraction (e.g. "00:07:15.25") */
    if (sscanf(s, "%2d:%2d:%2d.%3s", &h1, &m1, &s1, fraction) == 4)
    {
        if (1 == sscanf(fraction, "%3d", &d1) )
        {
            factor = factors[strnlen(fraction, sizeof(fraction)-1)-1];
            status = 0;
        }
    } 
    else if (sscanf(s, "%2d:%2d:%2d", &h1, &m1, &s1) == 3)
    {
        status = 0;
    }
    
    if (0 == status)
    {
        (*timing_value) = ( (int64_t)h1 * 3600 * 1000 +
                            (int64_t)m1 * 60 * 1000 +
                            (int64_t)s1 * 1000 +
                            (int64_t)d1 * factor ) * 1000;
    }
    
    if (0 != status)
    {
        /* Try offset-time format */
        float value = 0;
        if (sscanf(s, "%f%3s", &value, fraction) == 2)
        {
            if (0 == strncmp(fraction, "h", sizeof(fraction)))
            {
                value *= 3600000;
                status = 0;
            } 
            else if (0 == strncmp(fraction, "m", sizeof(fraction)))
            {
                value *= 60000;
                status = 0;
            }
            else if (0 == strncmp(fraction, "s", sizeof(fraction)))
            {
                value *= 1000;
                status = 0;
            }
            else if (0 == strncmp(fraction, "ms", sizeof(fraction)))
            {
                status = 0;
            }
            /* TODO: add support for "f" (frames), "t" (ticks) metric */
            
            if (0 == status)
            {
                (*timing_value) = (int64_t)(value) * 1000;
            }
        }
    }
    
    return status;
}

static int tagnamecmp( char const* tagname, char const* needle )
{
    if( !strncasecmp( "tt:", tagname, 3 ) )
        tagname += 3;

    return strcasecmp( tagname, needle );
}

static void XMLCALL StartElementTTML(void *userData, const char *name, const char **attr)
{
    demux_sys_t *p_sys = (demux_sys_t *)userData;
    if (p_sys->ttml.i_status != VLC_SUCCESS)
    {
        return;
    }
    
    if (0 == tagnamecmp(name, "p"))
    {
        p_sys->ttml.b_isParagraph = 1;
        p_sys->ttml.i_start = -1;
        p_sys->ttml.i_stop = -1;
    }
    
    if (0 == tagnamecmp(name, "p") || 0 == tagnamecmp(name, "span"))
    {
        int i = 0;
        for (i = 0; attr[i]; i += 2) 
        {
            int64_t *timing = NULL;
            if (0 == strcmp("begin", attr[i]))
            {
                timing = &(p_sys->ttml.i_start);
            }
            else if (0 == strcmp("end", attr[i]))
            {
                timing = &(p_sys->ttml.i_stop);
            }
            
            if (timing)
            {
                ParseTimingValueTTML(timing, attr[i+1]);
            }
        }
    }
    
    if (p_sys->ttml.b_isParagraph && 0 == tagnamecmp(name, "br"))
    {
        if (p_sys->ttml.p_text)
        {
            uint32_t oldSize = strlen(p_sys->ttml.p_text);
            p_sys->ttml.p_text = realloc_or_free(p_sys->ttml.p_text, oldSize+2);
            if (p_sys->ttml.p_text)
            {
                p_sys->ttml.p_text[oldSize] = '\n';
                p_sys->ttml.p_text[oldSize+1] = '\0';
            }
        }
    }
}

static void AddSubtitlesTTML(demux_sys_t *p_sys)
{
    if (p_sys->ttml.i_status != VLC_SUCCESS)
    {
        return;
    }
    
    if( p_sys->i_subtitles >= p_sys->ttml.i_max )
    {
        p_sys->ttml.i_max += 500;
        if( !( p_sys->subtitle = realloc_or_free( p_sys->subtitle,
                                          sizeof(subtitle_t) * p_sys->ttml.i_max ) ) )
        {
            p_sys->i_subtitles = 0;
            p_sys->ttml.i_status = VLC_ENOMEM;
            return;
        }
    }
    
    p_sys->subtitle[p_sys->i_subtitles].i_start  = p_sys->ttml.i_start >= 0 ? p_sys->ttml.i_start : 0;
    p_sys->subtitle[p_sys->i_subtitles].i_stop   = p_sys->ttml.i_stop >= 0 ? p_sys->ttml.i_stop : 0;
    p_sys->subtitle[p_sys->i_subtitles].psz_text = strdup(p_sys->ttml.p_text);
    p_sys->i_subtitles += 1;
}  


static void XMLCALL DataElementTTML(void *userData, const char *content, int length)
{
    demux_sys_t *p_sys = (demux_sys_t *)userData;
    if (p_sys->ttml.i_status != VLC_SUCCESS)
    {
        return;
    }
    
    if (p_sys->ttml.b_isParagraph)
    {
        char *tmp = malloc(length+1);
        strncpy(tmp, content, length);
        tmp[length] = '\0';
        strtrim(tmp, "\n");
        uint32_t newSize = strnlen(tmp, length); 
        if (newSize)
        {
            if (p_sys->ttml.p_text)
            {
                uint32_t oldSize = strlen(p_sys->ttml.p_text);
                p_sys->ttml.p_text = realloc_or_free(p_sys->ttml.p_text, newSize+oldSize+1);
                if (p_sys->ttml.p_text)
                {
                    strncpy(p_sys->ttml.p_text+oldSize, tmp, newSize);
                    p_sys->ttml.p_text[newSize+oldSize] = '\0';
                }
            }
            else
            {
                p_sys->ttml.p_text = tmp;
                return;
            }
        }
        free(tmp);
    }
}

static void XMLCALL EndElementTTML(void *userData, const char *name)
{
    demux_sys_t *p_sys = (demux_sys_t *)userData;
    if (p_sys->ttml.i_status != VLC_SUCCESS)
    {
        return;
    }
    
    if (0 == tagnamecmp(name, "p"))
    {
        p_sys->ttml.b_isParagraph = 0;
        if (p_sys->ttml.p_text)
        {
            strtrim(p_sys->ttml.p_text, " \n\r\t");
            if (p_sys->ttml.p_text[0] != '\0')
            {
                strnormalize_space(p_sys->ttml.p_text);
                AddSubtitlesTTML(p_sys);
            }
            free(p_sys->ttml.p_text);
            p_sys->ttml.p_text = NULL;
            p_sys->ttml.i_start = -1;
            p_sys->ttml.i_stop = -1;
        }
    }
}

int ReadSubtitltesTTML(demux_sys_t *p_sys, const char *buffer)
{
    int status = VLC_SUCCESS;
    XML_Parser parser = XML_ParserCreate("utf-8");
    XML_SetUserData(parser, p_sys);
    XML_SetElementHandler(parser, StartElementTTML, EndElementTTML);
    XML_SetCharacterDataHandler(parser, DataElementTTML);
    p_sys->ttml.i_max = 0;
    p_sys->ttml.i_status = status;
    p_sys->ttml.p_text = NULL;
    p_sys->ttml.i_start = -1;
    p_sys->ttml.i_stop = -1;
    p_sys->ttml.b_isParagraph = false;
    
    if (XML_Parse(parser, buffer, strlen(buffer), 1) == XML_STATUS_ERROR)
    {
        status = VLC_EGENERIC;
    }
    XML_ParserFree(parser);
    if (p_sys->ttml.p_text)
    {
        free(p_sys->ttml.p_text);
        p_sys->ttml.p_text = NULL;
    }
    return status;
}

/*
 * Copyright (c) 2010  Aurelien Jacobs <aurel@gnuage.org>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <stdio.h>

#include "../include/htmlsubtitles.h"

#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

static size_t av_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    while (++len < size && *src)
        *dst++ = *src++;
    if (len <= size)
        *dst = 0;
    return len + strlen(src) - 1;
}

static int html_color_parse(void *log_ctx, const char *str)
{
    /*
    uint8_t rgba[4];
    if (av_parse_color(rgba, str, strcspn(str, "\" >"), log_ctx) < 0)
    {
        return -1;
    }
    return rgba[0] | rgba[1] << 8 | rgba[2] << 16;
    */
    return -1;
}

enum {
    PARAM_UNKNOWN = -1,
    PARAM_SIZE,
    PARAM_COLOR,
    PARAM_FACE,
    PARAM_NUMBER
};

typedef struct SrtStack {
    char tag[128];
    char param[PARAM_NUMBER][128];
} SrtStack;

static void rstrip_spaces_buf(char *buf)
{
    size_t len = strlen(buf);
    while (len > 0 && buf[len - 1] == ' ')
    {
        buf[--len] = '\0';
    }
}

void ff_htmlmarkup_to_ass(void *log_ctx, char *dst, const char *in)
{
    char *param;
    char buffer[128];
    char tmp[128];
    int len;
    int tag_close;
    int sptr = 1;
    int line_start = 1;
    int an = 0;
    int end = 0;
    int dstIdx = 0;
    SrtStack stack[16];

    stack[0].tag[0] = 0;
    strcpy(stack[0].param[PARAM_SIZE],  "{\\fs}");
    strcpy(stack[0].param[PARAM_COLOR], "{\\c}");
    strcpy(stack[0].param[PARAM_FACE],  "{\\fn}");

    for (; !end && *in; in++) 
    {
        switch (*in) 
        {
        case '\r':
            break;
        case '\n':
            if (line_start)
            {
                break;
            }
            dst[dstIdx++] = '\n';
            line_start = 1;
            break;
        case ' ':
            if (!line_start)
            {
                dst[dstIdx++] = ' ';
            }
            break;
        case '{':    /* skip all {\xxx} substrings except for {\an%d}
                        and all microdvd like styles such as {Y:xxx} */
            len = 0;
            an += sscanf(in, "{\\an%*1u}%n", &len) >= 0 && len > 0;
            if ((an != 1 && (len = 0, sscanf(in, "{\\%*[^}]}%n", &len) >= 0 && len > 0)) ||
                (len = 0, sscanf(in, "{%*1[CcFfoPSsYy]:%*[^}]}%n", &len) >= 0 && len > 0))
            {
                in += len - 1;
            } 
            else
            {
                dst[dstIdx++] = *in;
            }
            break;
        case '<':
            tag_close = in[1] == '/';
            len = 0;
            if (sscanf(in+tag_close+1, "%127[^>]>%n", buffer, &len) >= 1 && len > 0)
            {
                const char *tagname = buffer;
                while (*tagname == ' ')
                {
                    tagname++;
                }
                if ((param = strchr(tagname, ' ')))
                {
                    *param++ = 0;
                }
                
                if ((!tag_close && sptr < FF_ARRAY_ELEMS(stack)) ||
                    ( tag_close && sptr > 0 && !strcmp(stack[sptr-1].tag, tagname))) 
                {
                    /*
                    int i;
                    int j;
                    */                    
                    int unknown = 0;
                    in += len + tag_close;
                    if (!tag_close)
                    {
                        memset(stack+sptr, 0, sizeof(*stack));
                    }
                    if (!strcmp(tagname, "font")) 
                    {
                        if (tag_close) 
                        {
                            ;
                            /*
                            for (i=PARAM_NUMBER-1; i>=0; i--)
                            {
                                if (stack[sptr-1].param[i][0])
                                {
                                    for (j=sptr-2; j>=0; j--)
                                    {
                                        if (stack[j].param[i][0]) 
                                        {
                                            av_bprintf(dst, "%s", stack[j].param[i]);
                                            break;
                                        }
                                    }
                                }
                            }
                            */
                        } 
                        else
                        {
                            while (param)
                            {
                                if (!strncmp(param, "size=", 5))
                                {
                                    unsigned font_size;
                                    param += 5 + (param[5] == '"');
                                    if (sscanf(param, "%u", &font_size) == 1)
                                    {
                                        snprintf(stack[sptr].param[PARAM_SIZE],
                                             sizeof(stack[0].param[PARAM_SIZE]),
                                             "{\\fs%u}", font_size);
                                    }
                                } 
                                else if (!strncmp(param, "color=", 6))
                                {
                                    param += 6 + (param[6] == '"');
                                    snprintf(stack[sptr].param[PARAM_COLOR],
                                         sizeof(stack[0].param[PARAM_COLOR]),
                                         "{\\c&H%X&}",
                                         html_color_parse(log_ctx, param));
                                }
                                else if (!strncmp(param, "face=", 5))
                                {
                                    param += 5 + (param[5] == '"');
                                    len = strcspn(param,
                                                  param[-1] == '"' ? "\"" :" ");
                                    av_strlcpy(tmp, param,
                                               FFMIN(sizeof(tmp), len+1));
                                    param += len;
                                    snprintf(stack[sptr].param[PARAM_FACE],
                                             sizeof(stack[0].param[PARAM_FACE]),
                                             "{\\fn%s}", tmp);
                                }
                                if ((param = strchr(param, ' ')))
                                {
                                    param++;
                                }
                            }
                            /*
                            for (i=0; i<PARAM_NUMBER; i++)
                            {
                                if (stack[sptr].param[i][0])
                                {
                                    av_bprintf(dst, "%s", stack[sptr].param[i]);
                                }
                            }
                            */
                        }
                    } 
                    else if (!tagname[1] && strspn(tagname, "bisu") == 1)
                    {
                        ; /*av_bprintf(dst, "{\\%c%d}", tagname[0], !tag_close);*/
                    }
                    else 
                    {
                        unknown = 1;
                        ; /*snprintf(tmp, sizeof(tmp), "</%s>", tagname);*/
                    }
                    if (tag_close)
                    {
                        sptr--;
                    }
                    else if (unknown && !strstr(in, tmp))
                    {
                        in -= len + tag_close;
                        dst[dstIdx++] = *in;
                    } 
                    else
                    {
                        av_strlcpy(stack[sptr++].tag, tagname,
                                   sizeof(stack[0].tag));
                    }
                    break;
                }
            }
        default:
            dst[dstIdx++] = *in;
            break;
        }
        if (*in != ' ' && *in != '\r' && *in != '\n')
        {
            line_start = 0;
        }
    }

    while (dstIdx >= 2 && !strncmp(&dst[dstIdx - 2], "\\N", 2))
    {
        dstIdx -= 2;
    }
    
    dst[dstIdx] = 0;
    rstrip_spaces_buf(dst);
}

#include <stdint.h>
#include <stdbool.h>

enum
{
    SUB_TYPE_UNKNOWN = -1,
    SUB_TYPE_MICRODVD,
    SUB_TYPE_SUBRIP,
    SUB_TYPE_SSA1,
    SUB_TYPE_SSA2_4,
    SUB_TYPE_ASS,
    SUB_TYPE_VPLAYER,
    SUB_TYPE_SAMI,
    SUB_TYPE_SUBVIEWER, /* SUBVIEWER 2 */
    SUB_TYPE_DVDSUBTITLE, /* Mplayer calls it subviewer2 */
    SUB_TYPE_MPL2,
    SUB_TYPE_AQT,
    SUB_TYPE_PJS,
    SUB_TYPE_MPSUB,
    SUB_TYPE_JACOSUB,
    SUB_TYPE_PSB,
    SUB_TYPE_RT,
    SUB_TYPE_DKS,
    SUB_TYPE_SUBVIEW1, /* SUBVIEWER 1 - mplayer calls it subrip09,
                         and Gnome subtitles SubViewer 1.0 */
    SUB_TYPE_VTT,
    SUB_TYPE_SBV
};

typedef struct
{
    int     i_line_count;
    int     i_line;
    char    **line;
} text_t;

typedef struct
{
    int64_t i_start;
    int64_t i_stop;

    char    *psz_text;
} subtitle_t;

typedef struct
{
    int         i_type;
    char       *psz_type_name;
    text_t      txt;

    int64_t     i_microsecperframe;

    char        *psz_header;
    int         i_subtitle;
    int         i_subtitles;
    subtitle_t  *subtitle;

    int64_t     i_length;

    struct
    {
        bool b_inited;

        int i_comment;
        int i_time_resolution;
        int i_time_shift;
    } jss;
    struct
    {
        bool  b_inited;

        float f_total;
        float f_factor;
    } mpsub;
} demux_sys_t;

int VLC_SubtitleDemuxOpen( const char *subStr, const int i_microsecperframe, demux_sys_t **pp_sys );
void VLC_SubtitleDemuxClose( demux_sys_t *p_sys );


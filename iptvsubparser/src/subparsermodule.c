#include "Python.h"

#include "vlc/include/subtitles.h"
#include "ffmpeg/include/htmlsubtitles.h"

#define IPTV_ULL_TYPE unsigned long long 
#define IPTV_LL_TYPE long long 
#define IPTV_UI_TYPE unsigned int 

static const char SUB_PARSER_VERSION[] = "0.4";
static const IPTV_UI_TYPE MAX_SUBTITLE_TEXT_SIZE = 1024;


static PyObject * get_version(PyObject *self, PyObject *unused)
{
    return PyString_FromString(SUB_PARSER_VERSION);
}

static uint32_t words(const char sentence[ ])
{
    uint32_t counted = 0;
    const char *it = sentence;
    uint32_t inword = 0;

    do
    {
        switch(*it) 
        {
            case '\0': 
            case ' ': 
            case '\t': 
            case '\n': 
            case '\r':
                if (inword) 
                {
                    inword = 0; 
                    counted++;
                }
                break;
            default: inword = 1;
        } 
    }while(*it++);

    return counted;
}

static char * ass_get_text(char *str)
{
    // Events are stored in the Block in this order:
    // ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    // 91,0,Default,,0,0,0,,maar hij smaakt vast tof.
    int i = 0;
    char *p_str = str;
    while(i < 8 && *p_str != '\0')
    {
        if (*p_str == ',')
            i++;
        p_str++;
    }
    // replace new line tat to real new line: '\\N' and '\\n' -> ' \n
    char *p_newline = NULL;
    while(((p_newline = strstr(p_str, "\\N")) != NULL) || ((p_newline = strstr(p_str, "\\n")) != NULL))
    {
        *(p_newline) = ' ';
        *(p_newline + 1) = '\n';
    }
    return p_str;
}

static char *get_text(char *str, const int i_type, int b_removeTags, char *tmpBuffer)
{
    char *strPtr = str;
    if (0 != b_removeTags)
    {
        if (SUB_TYPE_SSA1 == i_type ||
            SUB_TYPE_SSA2_4 == i_type ||
            SUB_TYPE_ASS == i_type)
        {
            strPtr = ass_get_text(str);
        }

        ff_htmlmarkup_to_ass(NULL, tmpBuffer, strPtr);
        return tmpBuffer;
    }
    return str;
}

static PyObject * _subparser_parse(PyObject *self, PyObject *args)
{
    const char *inputStr = NULL;
    int i_microsecperframe = 0;
    int b_setEndTime = 1;
    int b_removeTags = 1;
    int i_CPS = 12;
    int i_WPM = 138;
    int i = 0;
    char *pszText = NULL;
    PyObject *retObj = NULL; 
    PyObject *list = NULL;
    PyObject *elem = NULL;
    demux_sys_t *p_sys = NULL;
    
    if (!PyArg_ParseTuple(args, "si|iiii", &inputStr, &i_microsecperframe, &b_removeTags, &b_setEndTime, &i_CPS, &i_WPM))
    {
        return NULL;
    }
    
    if (0 == i_CPS) 
    {
        PyErr_SetString(PyExc_ZeroDivisionError, "CPS can not be set to 0");
        return NULL;
    }
    
    if (0 == i_WPM) 
    {
        PyErr_SetString(PyExc_ZeroDivisionError, "WPM can not be set to 0");
        return NULL;
    }
    
    pszText = malloc(MAX_SUBTITLE_TEXT_SIZE);
    if (NULL == pszText)
    {
        PyErr_SetString(PyExc_MemoryError, "Can not allocate memory for tmp buffer");
        return NULL;
    }
    
    retObj = PyDict_New();  
    
    if ( 0 != VLC_SubtitleDemuxOpen( inputStr, i_microsecperframe, &p_sys ) || NULL == p_sys )
    {
        free(pszText);
        return retObj;
    }
    
    if (0 != b_setEndTime)
    {
        for (i=0; i<p_sys->i_subtitles; ++i)
        {
            /* Fix previous item's end time if needed */
            if (i > 0 && 0 > p_sys->subtitle[i-1].i_stop)
            {
                /* Calculate end time based on "Subtitle reading speeds in different languages:" -> 
                 * http://www.raco.cat/index.php/QuadernsTraduccio/article/viewFile/265461/353045
                 */
                /* calc end time based on CPS (characters per second) */
                const int64_t timeBasedOnCPS = (strlen(p_sys->subtitle[i-1].psz_text) * 1000000) / i_CPS; 
                /* calc end time based on WPM (Words per minute) */
                const int64_t timeBasedOnWPM = (words(p_sys->subtitle[i-1].psz_text) * 60000000) / i_WPM ; 
                int64_t time = timeBasedOnCPS > timeBasedOnWPM ? timeBasedOnCPS : timeBasedOnWPM;
                if (time < 1500000) /* at least 1,5s */
                {
                    time = 1500000;
                }
                
                if ( (p_sys->subtitle[i-1].i_start + time) < p_sys->subtitle[i].i_start)
                {
                    p_sys->subtitle[i-1].i_stop = p_sys->subtitle[i-1].i_start + time;
                }
                else
                {
                    p_sys->subtitle[i-1].i_stop = p_sys->subtitle[i].i_start - 10000; /* end 10ms before next subtitles */
                }
            }
            
            /* Fix last subtitle */
            if (i == (p_sys->i_subtitles - 1) && 0 > p_sys->subtitle[i].i_stop)
            {
                const int64_t timeBasedOnCPS = (int64_t)(strlen(p_sys->subtitle[i].psz_text) * 1000000) / i_CPS; 
                const int64_t timeBasedOnWPM = (int64_t)(words(p_sys->subtitle[i].psz_text) * 60000000) / i_WPM ; 
                int64_t time = timeBasedOnCPS > timeBasedOnWPM ? timeBasedOnCPS : timeBasedOnWPM;
                if (time < 2000000) /* at least 2s */
                {
                    time = 2000000;
                }
                p_sys->subtitle[i].i_stop = time;
            }
        }
    }
    
    
    /* create list */
    list = PyList_New(p_sys->i_subtitles);
    
    for (i=0; i<p_sys->i_subtitles; ++i)
    {

        /* add elems to list */
        elem = Py_BuildValue("{s:I,s:I,s:s}", "start", (IPTV_UI_TYPE)(p_sys->subtitle[i].i_start / 1000), "end", (IPTV_UI_TYPE)(p_sys->subtitle[i].i_stop / 1000), "text", get_text(p_sys->subtitle[i].psz_text, p_sys->i_type, b_removeTags, pszText));
        PyList_SetItem(list, i, elem); // still reference no need to Py_DECREF
    }
    free(pszText);
    pszText = NULL;
    
    /* add created list to return dict */
    PyDict_SetItemString(retObj, "list", list);
    Py_DECREF(list); // PyDict_SetItemString not still reference but incement it
    
    /* add subtitles format to return dict */
    /* elem = PyInt_FromLong((long) p_sys->i_type); */
    elem = PyString_FromString( p_sys->psz_type_name );
    PyDict_SetItemString(retObj, "type", elem); 
    Py_DECREF(elem); // PyDict_SetItemString not still reference but incement it
    
    VLC_SubtitleDemuxClose( p_sys );
    
    return retObj;
}


static PyMethodDef _subparserMethods[] = {
    {"parse", (PyCFunction)_subparser_parse, METH_VARARGS,
     "return subtitles atom list parsed from given string (inputStr, i_microsecperframe, b_removeTags, b_setEndTime, i_CPS, i_WPM\n\n"
     " inputStr - (string encoded with UTF-8)\n"
     " i_microsecperframe - (microseconds per frame - used when subtitles use fps)\n"
     " b_removeTags - optional, default: True (True, False)\n"
     " b_setEndTime - optional, default: True (True - default, False)\n"
     " i_CPS - optional, default: 12 (characters per second - used for end time calculation if needed)\n"
     " i_WPM - optional, default: 138 (words per minute - used for end time calculation if needed)\n"
    },
    {"version", (PyCFunction)get_version, METH_NOARGS, 
     "return version (str) of the sub parser module\n\n" 
    },
    {NULL,NULL,0,NULL}
};

PyMODINIT_FUNC init_subparser(void)
{
    Py_InitModule("_subparser", _subparserMethods);
}
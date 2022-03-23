/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/



#ifndef _XEVD_APP_ARGS_H_

#define _XEVD_APP_ARGS_H_



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XEVD_ARGS_VAL_TYPE_MANDATORY       (1<<0) /* mandatory or not */
#define XEVD_ARGS_VAL_TYPE_NONE            (0<<1) /* no value */
#define XEVD_ARGS_VAL_TYPE_INTEGER         (10<<1) /* integer type value */
#define XEVD_ARGS_VAL_TYPE_STRING          (20<<1) /* string type value */

#define XEVD_ARGS_GET_CMD_OPT_VAL_TYPE(x)  ((x) & ~XEVD_ARGS_VAL_TYPE_MANDATORY)

#define XEVD_ARGS_NO_KEY                   (127)
#define XEVD_ARGS_KEY_LONG_CONFIG          "config"

#define MAX_NUM_CONF_FILES 16



typedef struct _XEVD_ARGS_OPTION
{
    char   key; /* option keyword. ex) -f */
    char   key_long[32]; /* option long keyword, ex) --file */
    int    val_type; /* value type */
    int  * flag; /* flag to setting or not */
    void * val; /* actual value */
    char   desc[512]; /* description of option */
} XEVD_ARGS_OPTION;


static int xevd_args_search_long_arg(XEVD_ARGS_OPTION * opts, const char * argv)
{
    int oidx = 0;
    XEVD_ARGS_OPTION * o;

    o = opts;

    while(o->key != 0)
    {
        if(!strcmp(argv, o->key_long))
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}


static int xevd_args_search_short_arg(XEVD_ARGS_OPTION * ops, const char argv)
{
    int oidx = 0;
    XEVD_ARGS_OPTION * o;

    o = ops;

    while(o->key != 0)
    {
        if(o->key != XEVD_ARGS_NO_KEY && o->key == argv)
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}

static int xevd_args_read_value(XEVD_ARGS_OPTION * ops, const char * argv)
{
    if(argv == NULL) return -1;
    if(argv[0] == '-' && (argv[1] < '0' || argv[1] > '9')) return -1;

    switch(XEVD_ARGS_GET_CMD_OPT_VAL_TYPE(ops->val_type))
    {
        case XEVD_ARGS_VAL_TYPE_INTEGER:
            *((int*)ops->val) = atoi(argv);
            break;

        case XEVD_ARGS_VAL_TYPE_STRING:
            strcpy((char*)ops->val, argv);
            break;

        default:
            return -1;
    }
    return 0;
}

static int xevd_args_get_help(XEVD_ARGS_OPTION * ops, int idx, char * help)
{
    int optional = 0;
    char vtype[32];
    XEVD_ARGS_OPTION * o = ops + idx;

    switch(XEVD_ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case XEVD_ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            break;
        case XEVD_ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            break;
        case XEVD_ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            break;
    }
    optional = !(o->val_type & XEVD_ARGS_VAL_TYPE_MANDATORY);

    if(o->key != XEVD_ARGS_NO_KEY)
    {
        sprintf(help, "  -%c, --%s [%s]%s\n    : %s", o->key, o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    else
    {
        sprintf(help, "  --%s [%s]%s\n    : %s", o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    return 0;
}

static int xevd_parse_cfg(FILE * fp, XEVD_ARGS_OPTION * ops)
{
    char * parser;
    char line[256] = "", tag[50] = "", val[256] = "";
    int oidx;

    while(fgets(line, sizeof(line), fp))
    {
        parser = strchr(line, '#');
        if(parser != NULL) *parser = '\0';

        parser = strtok(line, "= \t");
        if(parser == NULL) continue;
        strcpy(tag, parser);

        parser = strtok(NULL, "=\n");
        if(parser == NULL) continue;
        strcpy(val, parser);

        oidx = xevd_args_search_long_arg(ops, tag);
        if(oidx < 0) continue;

        if(XEVD_ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
           XEVD_ARGS_VAL_TYPE_NONE)
        {
            if(xevd_args_read_value(ops + oidx, val)) continue;
        }
        else
        {
            *((int*)ops[oidx].val) = 1;
        }
        *ops[oidx].flag = 1;
    }
    return 0;
}


static int xevd_parse_cmd(int argc, const char * argv[], XEVD_ARGS_OPTION * ops,
                          int * idx)
{
    int    aidx; /* arg index */
    int    oidx; /* option index */

    aidx = *idx + 1;

    if(aidx >= argc || argv[aidx] == NULL) goto NO_MORE;
    if(argv[aidx][0] != '-') goto ERR;

    if(argv[aidx][1] == '-')
    {
        /* long option */
        oidx = xevd_args_search_long_arg(ops, argv[aidx] + 2);
        if(oidx < 0) goto ERR;
    }
    else if(strlen(argv[aidx]) == 2)
    {
        /* short option */
        oidx = xevd_args_search_short_arg(ops, argv[aidx][1]);
        if(oidx < 0) goto ERR;
    }
    else
    {
        goto ERR;
    }

    if(XEVD_ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) != XEVD_ARGS_VAL_TYPE_NONE)
    {
        if(aidx + 1 >= argc) goto ERR;
        if(xevd_args_read_value(ops + oidx, argv[aidx + 1])) goto ERR;
        *idx = *idx + 1;
    }
    else
    {
        *((int*)ops[oidx].val) = 1;
    }
    *ops[oidx].flag = 1;
    *idx = *idx + 1;

    return ops[oidx].key;


NO_MORE:
    return 0;

ERR:
    return -1;
}

int xevd_args_parse_all(int argc, const char * argv[],
                               XEVD_ARGS_OPTION * ops)
{
    int i, ret = 0, idx = 0;
    XEVD_ARGS_OPTION *o;
    const char *fname_cfg = NULL;
    FILE *fp;

    int num_configs = 0;
    int posConfFiles[MAX_NUM_CONF_FILES];
    memset(&posConfFiles, -1, sizeof(int) * MAX_NUM_CONF_FILES);

    /* config file parsing */
    for(i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "--"XEVD_ARGS_KEY_LONG_CONFIG))
        {
            if(i + 1 < argc)
            {
                num_configs++;
                posConfFiles[num_configs - 1] = i + 1;
            }
        }
    }
    for (int i = 0; i < num_configs; i++)
    {
        fname_cfg = argv[posConfFiles[i]];

        if(fname_cfg)
        {
            fp = fopen(fname_cfg, "r");
            if(fp == NULL) return -1; /* config file error */

            if(xevd_parse_cfg(fp, ops))
            {
                fclose(fp);
                return -1; /* config file error */
            }
            fclose(fp);
        }
    }

    /* command line parsing */
    while(1)
    {
        ret = xevd_parse_cmd(argc, argv, ops, &idx);
        if(ret <= 0) break;
    }

    /* check mandatory argument */
    o = ops;

    while(o->key != 0)
    {
        if(o->val_type & XEVD_ARGS_VAL_TYPE_MANDATORY)
        {
            if(*o->flag == 0)
            {
                /* not filled all mandatory argument */
                return o->key;
            }
        }
        o++;
    }
    return ret;
}


static char op_fname_inp[256] = "\0";
static char op_fname_out[256] = "\0";
static int  op_max_frm_num = 0;
static int  op_threads = 1; /* Default value */
static int  op_use_pic_signature = 0;
static int  op_out_bit_depth = 8; /* default value */
static int  op_out_chroma_format = 1;

typedef enum _STATES
{
    STATE_DECODING,
    STATE_BUMPING
} STATES;

typedef enum _OP_FLAGS
{

    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_FLAG_VERBOSE,
    OP_THREADS,
    OP_FLAG_MAX

} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = { 0 };

static XEVD_ARGS_OPTION options[] = \
{

    {
        'i', "input", XEVD_ARGS_VAL_TYPE_STRING | XEVD_ARGS_VAL_TYPE_MANDATORY,
            &op_flag[OP_FLAG_FNAME_INP], op_fname_inp,
            "file name of input bitstream"
    },
    {
        'o', "output", XEVD_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], op_fname_out,
        "file name of decoded output"
    },
    {
        'f',  "frames", XEVD_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], &op_max_frm_num,
        "maximum number of frames to be decoded"
    },
    {
        'm',  "threads", XEVD_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_THREADS], &op_threads,
        "Force to use a specific number of threads. default: 1"
    },
    {
        's',  "signature", XEVD_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], &op_use_pic_signature,
        "conformance check using picture signature (HASH)"
    },
    {
        'v',  "verbose", XEVD_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_VERBOSE], &op_verbose,
        "verbose level\n"
        "\t 0: no message\n"
        "\t 1: simple messages (default)\n"
        "\t 2: frame-level messages\n"
    },
    {
        XEVD_ARGS_NO_KEY,  "output-bit-depth", XEVD_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_out_bit_depth,
        "output bitdepth (8(default), 10) "
    },
    { 0, "", XEVD_ARGS_VAL_TYPE_NONE, NULL, NULL, "" } /* termination */

};

#define XEVD_NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)

#endif /*_XEVD_APP_ARGS_H_ */

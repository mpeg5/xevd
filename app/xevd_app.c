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

#include "xevd.h"
#include "xevd_app_util.h"
#include "xevd_app_args.h"

#if LINUX
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#define VERBOSE_NONE               VERBOSE_0
#define VERBOSE_SIMPLE             VERBOSE_1
#define VERBOSE_FRAME              VERBOSE_2

#define MAX_BS_BUF                 (16*1024*1024) /* byte */

typedef enum _STATES
{
    STATE_DECODING,
    STATE_BUMPING
} STATES;

static void print_usage(void)
{
    int i;
    char str[1024];

    logv0("< Usage >\n");

    for(i=0; i<XEVD_NUM_ARG_OPTION; i++)
    {
        if(xevd_args_get_help(options, i, str) < 0) return;
        logv0("%s\n", str);
    }
}

static int read_bitstream(FILE * fp, int * pos, unsigned char * bs_buf)
{
    int read_size, bs_size;
    unsigned char b = 0;

    bs_size = 0;
    read_size = 0;

    if(!fseek(fp, *pos, SEEK_SET))
    {
        /* read size first */
        if(XEVD_NAL_UNIT_LENGTH_BYTE == fread(&bs_size, 1, XEVD_NAL_UNIT_LENGTH_BYTE, fp))
        {
            if(bs_size <= 0)
            {
                logv0("Invalid bitstream size![%d]\n", bs_size);
                return -1;
            }

            while(bs_size)
            {
                /* read byte */
                if (1 != fread(&b, 1, 1, fp))
                {
                    logv0("Cannot read bitstream!\n");
                    return -1;
                }
                bs_buf[read_size] = b;
                read_size++;
                bs_size--;
            }
        }
        else
        {
            if(feof(fp)) {logv2("End of file\n");}
            else {logv0("Cannot read bitstream size!\n")};

            return -1;
        }
    }
    else
    {
        logv0("Cannot seek bitstream!\n");
        return -1;
    }

    return read_size;
}

static void print_stat(XEVD_STAT * stat, int ret)
{
    int i, j;

    if(XEVD_SUCCEEDED(ret))
    {
        if(stat->nalu_type < XEVD_SPS_NUT)
        {
            logv2("%c-slice", stat->stype == XEVD_ST_I ? 'I' : stat->stype == XEVD_ST_P ? 'P' : 'B');

            logv2(" (%d bytes", stat->read);
            logv2(", poc=%d, tid=%d, ", (int)stat->poc, (int)stat->tid);

            for (i = 0; i < 2; i++)
            {
                logv2("[L%d ", i);
                for (j = 0; j < stat->refpic_num[i]; j++) logv2("%d ", stat->refpic[i][j]);
                logv2("] ");
            }
        }
        else if(stat->nalu_type == XEVD_SPS_NUT)
        {
            logv2("Sequence Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == XEVD_PPS_NUT)
        {
            logv2("Picture Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == XEVD_SEI_NUT)
        {
            logv2("SEI message: ");
            if (ret == XEVD_OK)
            {
                logv2("MD5 check OK");
            }
            else if (ret == XEVD_ERR_BAD_CRC)
            {
                logv2("MD5 check mismatch!");
            }
            else if (ret == XEVD_WARN_CRC_IGNORED)
            {
                logv2("MD5 check ignored!");
            }
        }
        else
        {
            logv0("Unknown bitstream");
        }

        logv2("\n");
    }
    else
    {
        logv0("Decoding error = %d\n", ret);
    }
}

static int set_extra_config(XEVD id)
{
    int  ret, size, value;

    if(op_use_pic_signature)
    {
        value = 1;
        size = 4;
        ret = xevd_config(id, XEVD_CFG_SET_USE_PIC_SIGNATURE, &value, &size);
        if(XEVD_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }

    if (op_fname_opl[0])
    {
        value = 1;
        size = 4;
        ret = xevd_config(id, XEVD_CFG_SET_USE_OPL_OUTPUT, &value, &size);
        if (XEVD_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }

    return 0;
}

static int write_dec_img(XEVD id, char * fname, XEVD_IMGB * img, XEVD_IMGB * imgb_t)
{
    imgb_cpy(imgb_t, img);
    if(imgb_write(op_fname_out, imgb_t)) return -1;
    return XEVD_OK;
}

int main(int argc, const char **argv)
{
    STATES             state = STATE_DECODING;
    unsigned char    * bs_buf = NULL;
    XEVD               id = NULL;
    XEVD_CDSC          cdsc;
    XEVD_BITB          bitb;
    XEVD_IMGB        * imgb;
    /*temporal buffer for video bit depth less than 10bit */
    XEVD_IMGB        * imgb_t = NULL;
    XEVD_STAT          stat;
    XEVD_OPL           opl;
    int                ret;
    XEVD_CLK           clk_beg, clk_tot;
    int                bs_cnt, pic_cnt;
    int                bs_size, bs_read_pos = 0;
    int                w, h;
    FILE             * fp_bs = NULL;
    int                decod_frames = 0;

    /* parse options */
    ret = xevd_args_parse_all(argc, argv, options);
    if(ret != 0)
    {
        if(ret > 0) logv0("-%c argument should be set\n", ret);
        print_usage();
        return -1;
    }

    logv1("eXtra-fast Essential Video Decoder\n");

    /* open input bitstream */
    fp_bs = fopen(op_fname_inp, "rb");
    if(fp_bs == NULL)
    {
        logv0("ERROR: cannot open bitstream file = %s\n", op_fname_inp);
        print_usage();
        return -1;
    }

    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        /* remove decoded file contents if exists */
        FILE * fp;
        fp = fopen(op_fname_out, "wb");
        if(fp == NULL)
        {
            logv0("ERROR: cannot create a decoded file\n");
            print_usage();
            return -1;
        }
        fclose(fp);
    }

    if (op_flag[OP_FLAG_FNAME_OPL])
    {
        /* remove opl file contents if exists */
        FILE * fp;
        fp = fopen(op_fname_opl, "wb");
        if (fp == NULL)
        {
            logv0("ERROR: cannot create an opl file\n");
            print_usage();
            return -1;
        }
        fclose(fp);
    }

    bs_buf = malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logv0("ERROR: cannot allocate bit buffer, size=%d\n", MAX_BS_BUF);
        return -1;
    }
    cdsc.task_cnt = (int)op_parallel_task;
    

    id = xevd_create(&cdsc, NULL);
    if(id == NULL)
    {
        logv0("ERROR: cannot create XEVD decoder\n");
        return -1;
    }
    if(set_extra_config(id))
    {
        logv0("ERROR: cannot set extra configurations\n");
        return -1;
    }

    pic_cnt = 0;
    clk_tot = 0;
    bs_cnt  = 0;
    w = h   = 0;

    int process_status = XEVD_OK;

    while(1)
    {
        if (state == STATE_DECODING)
        {
            memset(&stat, 0, sizeof(XEVD_STAT));

            bs_size = read_bitstream(fp_bs, &bs_read_pos, bs_buf);

            if (bs_size <= 0)
            {
                state = STATE_BUMPING;
                logv2("bumping process starting...\n");
                continue;
            }

            bs_read_pos += (XEVD_NAL_UNIT_LENGTH_BYTE + bs_size);
            stat.read += XEVD_NAL_UNIT_LENGTH_BYTE;
            bitb.addr = bs_buf;
            bitb.ssize = bs_size;
            bitb.bsize = MAX_BS_BUF;

            logv2("[%4d] NALU --> ", bs_cnt);
            bs_cnt++;

            clk_beg = xevd_clk_get();

            /* main decoding block */
            ret = xevd_decode(id, &bitb, &stat);

            clk_tot += xevd_clk_from(clk_beg);

            if (op_verbose == VERBOSE_SIMPLE && stat.nalu_type < XEVD_SPS_NUT)
            {
                int total_time = ((int)xevd_clk_msec(clk_tot) / 1000);
                int h = total_time / 3600;
                total_time = total_time % 3600;
                int m = total_time / 60;
                total_time = total_time % 60;
                int s = total_time;

                logv1("[ %d / %d frames ] [ %.2f frame/sec ] [ %2dh %2dm %2ds ] \r"
                    , decod_frames, op_max_frm_num, ((float)(decod_frames + 1) * 1000) / ((float)xevd_clk_msec(clk_tot))
                    , h, m, s);
                fflush(stdout);
                decod_frames++;
            }

            if(XEVD_FAILED(ret))
            {
                logv0("failed to decode bitstream\n");
                goto END;
            }

            print_stat(&stat, ret);

            if(stat.read - XEVD_NAL_UNIT_LENGTH_BYTE != bs_size)
            {
                logv0("\t=> different reading of bitstream (in:%d, read:%d)\n", bs_size, stat.read);
            }

            process_status = ret;
        }

        if(stat.fnum >= 0 || state == STATE_BUMPING)
        {
            ret = xevd_pull(id, &imgb, &opl);
            if(ret == XEVD_ERR_UNEXPECTED)
            {
                logv2("bumping process completed\n");
                goto END;
            }
            else if(XEVD_FAILED(ret))
            {
                logv0("failed to pull the decoded image\n");
                return -1;
            }
        }
        else
        {
            imgb = NULL;
        }

        if(imgb)
        {
            w = imgb->aw[0];
            h = imgb->ah[0];

            int internal_codec_bit_depth, size = 4;
            xevd_config(id, XEVD_CFG_GET_CODEC_BIT_DEPTH, &internal_codec_bit_depth, &size);
            op_out_bit_depth = op_out_bit_depth == 0 ? internal_codec_bit_depth : op_out_bit_depth;

            if(op_flag[OP_FLAG_FNAME_OUT])
            {
                if(imgb_t == NULL)
                {
                    imgb_t = imgb_alloc(w, h, XEVD_CS_SET(XEVD_CS_GET_FORMAT(imgb->cs), op_out_bit_depth, 0));
                    if(imgb_t == NULL)
                    {
                        logv0("failed to allocate temporay image buffer\n");
                        return -1;
                    }
                }
                write_dec_img(id, op_fname_out, imgb, imgb_t);
            }

            if (op_flag[OP_FLAG_FNAME_OPL])
            {
                FILE* fp_opl = fopen(op_fname_opl, "a");
                if (fp_opl == NULL)
                {
                    logv0("ERROR: cannot create an opl file\n");
                    print_usage();
                    return -1;
                }

                fprintf(fp_opl, "%d %d %d ", opl.poc, w, h);
                for (int i = 0; i < 3; ++i)
                {
                    for (int j = 0; j < 16; ++j)
                    {
                        unsigned int byte = (unsigned char) opl.digest[i][j];
                        fprintf(fp_opl, "%02x", byte);
                    }
                    fprintf(fp_opl, " ");
                }

                fprintf(fp_opl, "\n");

                fclose(fp_opl);
            }

            imgb->release(imgb);
            pic_cnt++;
        }
        fflush(stdout);
        fflush(stderr);
    }

END:

    logv1("=======================================================================================\n");
    logv1("Resolution                        = %d x %d\n", w, h);
    logv1("Processed NALUs                   = %d\n", bs_cnt);
    logv1("Decoded frame count               = %d\n", pic_cnt);
    if(pic_cnt > 0)
    {
        logv1("total decoding time               = %d msec,",
                (int)xevd_clk_msec(clk_tot));
        logv1(" %.3f sec\n",
            (float)(xevd_clk_msec(clk_tot) /1000.0));

        logv1("Average decoding time for a frame = %d msec\n",
                (int)xevd_clk_msec(clk_tot)/pic_cnt);
        logv1("Average decoding speed            = %.3f frames/sec\n",
                ((float)pic_cnt*1000)/((float)xevd_clk_msec(clk_tot)));
    }
    logv1("=======================================================================================\n");

    if(id) xevd_delete(id);
    if(imgb_t) imgb_free(imgb_t);
    if(fp_bs) fclose(fp_bs);
    if(bs_buf) free(bs_buf);

    return process_status;
}

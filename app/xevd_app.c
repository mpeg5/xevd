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

#define MAX_BS_BUF                 16*1024*1024 /* byte */

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
    int ret, read_size, bs_size;
    unsigned char nalu_len_buf[4], b = 0;
    XEVD_INFO info;

    bs_size = 0;
    read_size = 0;

    if(!fseek(fp, *pos, SEEK_SET))
    {
        /* read size first */
        if(XEVD_NAL_UNIT_LENGTH_BYTE == fread(nalu_len_buf, 1, XEVD_NAL_UNIT_LENGTH_BYTE, fp))
        {
            ret = xevd_info(nalu_len_buf, XEVD_NAL_UNIT_LENGTH_BYTE, 1, &info);
            if (XEVD_FAILED(ret)) {
                logv0("Cannot get bitstream information\n");
                return XEVD_ERR;
            }
            bs_size = info.nalu_len;

            if(bs_size <= 0)
            {
                logv0("Invalid bitstream size![%d]\n", bs_size);
                return XEVD_ERR;
            }

            while(bs_size)
            {
                /* read byte */
                if (1 != fread(&b, 1, 1, fp))
                {
                    logv0("Cannot read bitstream!\n");
                    return XEVD_ERR;
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

            return XEVD_ERR;
        }
    }
    else
    {
        logv0("Cannot seek bitstream!\n");
        return XEVD_ERR;
    }

    return read_size;
}

static void print_stat(XEVD_STAT * stat, int ret)
{
    int i, j;

    if(XEVD_SUCCEEDED(ret))
    {
        if(stat->nalu_type < XEVD_NUT_SPS)
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
        else if(stat->nalu_type == XEVD_NUT_SPS)
        {
            logv2("Sequence Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == XEVD_NUT_PPS)
        {
            logv2("Picture Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == XEVD_NUT_APS)
        {
            logv2("Adaptation Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == XEVD_NUT_SEI)
        {
            logv2("SEI message: ");
            if (ret == XEVD_OK)
            {
                logv2("MD5 check OK");
            }
            else if (ret == XEVD_WARN_CRC_IGNORED)
            {
                logv2("MD5 check ignored!");
            }
            else
            {
                logv2("Unknow SEI message");
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
        if (ret == XEVD_ERR_BAD_CRC)
        {
            logv0("MD5 check mismatch!\n");
        }
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
            return XEVD_ERR;
        }
    }

    return 0;
}

static int get_extra_config(XEVD id)
{
    int width, height, coded_width, coded_height, color_space, max_coding_delay;
    int ret, size;

    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_WIDTH, &width, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get width\n");
        return XEVD_ERR;
    }
    logv2("width = %d\n", width);

    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_HEIGHT, &height, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get height\n");
        return XEVD_ERR;
    }
    logv2("height = %d\n", height);

    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_CODED_WIDTH, &coded_width, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get coded_width\n");
        return XEVD_ERR;
    }
    logv2("coded_width = %d\n", coded_width);

    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_CODED_HEIGHT, &coded_height, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get coded_height\n");
        return XEVD_ERR;
    }
    logv2("coded_height = %d\n", coded_height);

    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_COLOR_SPACE, &color_space, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get color_space\n");
        return XEVD_ERR;
    }
    switch(color_space)
    {
    case XEVD_CS_YCBCR400_10LE:
        logv2("color_space = XEVD_CS_YCBCR400_10LE\n");
        break;
    case XEVD_CS_YCBCR420_10LE:
        logv2("color_space = XEVD_CS_YCBCR420_10LE\n");
        break;
    case XEVD_CS_YCBCR422_10LE:
        logv2("color_space = XEVD_CS_YCBCR422_10LE\n");
        break;
    case XEVD_CS_YCBCR444_10LE:
        logv2("color_space = XEVD_CS_YCBCR444_10LE\n");
        break;
    default:
        logv2("unknown color space\n");
    }
    size = 4;
    ret = xevd_config(id, XEVD_CFG_GET_MAX_CODING_DELAY, &max_coding_delay, &size);
    if (XEVD_FAILED(ret))
    {
        logv2("failed to get max_coding_delay\n");
        return XEVD_ERR;
    }
    logv2("max_coding_delay = %d\n", max_coding_delay);
    return 0;
}

/* sequence level header */
static int write_y4m_header(char * fname, XEVD_IMGB * img)
{

    int color_format = XEVD_CS_GET_FORMAT(img->cs);
    int bit_depth  =   op_out_bit_depth;
    int len = 80;
    int buff_len = 0;
    char buf[80] = { '\0', };
    char c_buf[16] = { '\0', };
    FILE          * fp;

    if (color_format == XEVD_CF_YCBCR420)
    {
        if (bit_depth == 8) strcpy(c_buf, "420mpeg2");
        else if (bit_depth == 10) strcpy(c_buf, "420p10");
    }
    else if (color_format == XEVD_CF_YCBCR422)
    {
        if (bit_depth == 8) strcpy(c_buf, "422");
        else if (bit_depth == 10) strcpy(c_buf, "422p10");
    }
    else if (color_format == XEVD_CF_YCBCR444)
    {
        if (bit_depth == 8)  strcpy(c_buf, "444");
        else if (bit_depth == 10) strcpy(c_buf, "444p10");
    }
    else if (color_format == XEVD_CF_YCBCR400)
    {
        if (bit_depth == 8)  strcpy(c_buf, "mono");
    }

    if (strlen(c_buf) == 0)
    {
        logv0("Color format is not suuported by y4m");
        return XEVD_ERR;
    }

    /*setting fps to 30 by default as there is no fps related parameter */
    buff_len = snprintf(buf, len, "YUV4MPEG2 W%d H%d F%d:%d Ip C%s\n", \
        img->w[0], img->h[0], 30, 1, c_buf);


    fp = fopen(fname, "ab");
    if (fp == NULL)
    {
        logv0("cannot open file = %s\n", fname);
        return XEVD_ERR;
    }
    if (buff_len != fwrite(buf, 1, buff_len, fp))
    {
        fclose(fp);
        return XEVD_ERR;
    }
    fclose(fp);
    return XEVD_OK;

}
/* Frame level header or separator */
static int write_y4m_frame_header(char * fname)
{
    FILE * fp;
    fp = fopen(fname, "ab");
    if (fp == NULL)
    {
        logv0("cannot open file = %s\n", fname);
        return XEVD_ERR;
    }
    if (6 != fwrite("FRAME\n", 1, 6, fp))
    {
        fclose(fp);
        return XEVD_ERR;
    }
    fclose(fp);
    return XEVD_OK;

}


static int write_dec_img(XEVD id, char * fname, XEVD_IMGB * img, XEVD_IMGB * imgb_t, int flag_y4m)
{
    imgb_cpy(imgb_t, img);
    if (flag_y4m)
    {
        if(write_y4m_frame_header(op_fname_out)) return XEVD_ERR;
    }
    if(imgb_write(op_fname_out, imgb_t)) return XEVD_ERR;
    return XEVD_OK;
}

int main(int argc, const char **argv)
{
    STATES             state = STATE_DECODING;
    unsigned char    * bs_buf = NULL;
    XEVD               id = NULL;
    XEVD_CDSC          cdsc;
    XEVD_BITB           bitb;
    XEVD_IMGB        *  imgb;
    /*temporal buffer for video bit depth less than 10bit */
    XEVD_IMGB        *  imgb_t = NULL;
    XEVD_STAT          stat;
    int                ret, proc_ret;
    XEVD_CLK            clk_beg, clk_tot;
    int                bs_cnt, pic_cnt;
    int                bs_size, bs_read_pos = 0;
    int                al_w, al_h;
    int                act_w = 0, act_h = 0;
    FILE             * fp_bs = NULL;
    int                decod_frames = 0;
    int                is_y4m = 0;
    int                dim_changed = 0;

    int                vcl_cnt = 0;


    /* parse options */
    ret = xevd_args_parse_all(argc, argv, options);
    if(ret != 0)
    {
        if(ret > 0) logv0("-%c argument should be set\n", ret);
        print_usage();
        return XEVD_ERR;
    }

    logv1("eXtra-fast Essential Video Decoder\n");
    /* open input bitstream */
    fp_bs = fopen(op_fname_inp, "rb");
    if(fp_bs == NULL)
    {
        logv0("ERROR: cannot open bitstream file = %s\n", op_fname_inp);
        print_usage();
        return XEVD_ERR;
    }

    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        char fext[5];

        if(strlen(op_fname_out) < 5) /* x.yuv or x.y4m */
        {
            logv0("ERROR: invalide output file name\n");
            return XEVD_ERR;
        }
        strncpy(fext, op_fname_out + strlen(op_fname_out) - 4, 4);
        fext[1] = toupper(fext[1]);
        fext[2] = toupper(fext[2]);
        fext[3] = toupper(fext[3]);

        if(strncmp(fext, ".YUV", 4) == 0)
        {
            is_y4m = 0;
        }
        else if(strncmp(fext,".Y4M", 4) == 0)
        {
            is_y4m = 1;
        }
        else
        {
            logv0("ERROR: unknown output format\n");
            return XEVD_ERR;
        }
        /* remove decoded file contents if exists */
        FILE * fp;
        fp = fopen(op_fname_out, "wb");
        if(fp == NULL)
        {
            logv0("ERROR: cannot create a decoded file\n");
            print_usage();
            return XEVD_ERR;
        }
        fclose(fp);
    }

    bs_buf = malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logv0("ERROR: cannot allocate bit buffer, size=%d\n", MAX_BS_BUF);
        return XEVD_ERR;
    }
    cdsc.threads = (int)op_threads;

    id = xevd_create(&cdsc, NULL);
    if(id == NULL)
    {
        logv0("ERROR: cannot create XEVD decoder\n");
        return XEVD_ERR;
    }
    if(set_extra_config(id))
    {
        logv0("ERROR: cannot set extra configurations\n");
        return XEVD_ERR;
    }

    pic_cnt = 0;
    clk_tot = 0;
    bs_cnt  = 0;
    al_w = al_h   = 0;
    proc_ret = 0;

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

            bitb.ts[XEVD_TS_DTS] = vcl_cnt;

            bs_cnt++;
            logv2("[%4d] NALU --> ", bs_cnt);

            clk_beg = xevd_clk_get();

            /* main decoding block */
            ret = xevd_decode(id, &bitb, &stat);
            if(stat.fnum >= 0 && (stat.nalu_type == XEVD_NUT_IDR || stat.nalu_type == XEVD_NUT_NONIDR))
            {
                vcl_cnt++;
            }

            clk_tot += xevd_clk_from(clk_beg);

            print_stat(&stat, ret);

            if(XEVD_FAILED(ret))
            {
                logv0("failed to decode bitstream\n");
                proc_ret = XEVD_ERR;
                goto END;
            }

            if(stat.read - XEVD_NAL_UNIT_LENGTH_BYTE != bs_size)
            {
                logv0("\t=> different reading of bitstream (in:%d, read:%d)\n",
                    bs_size, stat.read - XEVD_NAL_UNIT_LENGTH_BYTE);
            }

            /* print extra config */
            //get_extra_config(id);

        }
        if(stat.fnum >= 0 || state == STATE_BUMPING)
        {
            ret = xevd_pull(id, &imgb);

            if(ret == XEVD_ERR_UNEXPECTED)
            {
                logv2("bumping process completed\n");
                proc_ret = XEVD_OK;
                goto END;
            }
            else if(XEVD_FAILED(ret))
            {
                logv0("failed to pull the decoded image\n");
                proc_ret = XEVD_ERR;
                goto END;
            }
        }
        else
        {
            imgb = NULL;
        }

        if(imgb)
        {
            logv2("pts: %lld | dts: %lld\n", imgb->ts[XEVD_TS_PTS], imgb->ts[XEVD_TS_DTS]);
            if (al_w != imgb->aw[0] || al_h != imgb->ah[0])
            {
                dim_changed = 1;
            }
            else
            {
                dim_changed = 0;
            }

            al_w = imgb->aw[0];
            al_h = imgb->ah[0];

            act_w = imgb->w[0];
            act_h = imgb->h[0];

            if(op_flag[OP_FLAG_FNAME_OUT])
            {

                if(imgb_t == NULL || dim_changed)
                {
                    if(imgb_t)
                    {
                        free(imgb_t);
                        imgb_t = NULL;
                    }
                    imgb_t = imgb_alloc(al_w, al_h, XEVD_CS_SET(XEVD_CS_GET_FORMAT(imgb->cs), op_out_bit_depth, 0));
                    if(imgb_t == NULL)
                    {
                        logv0("failed to allocate temporay image buffer\n");
                        proc_ret = XEVD_ERR;
                        goto END;
                    }
                    //Copy the actual width and height of input image to temporary image.
                    int chroma_format = XEVD_CS_GET_FORMAT(imgb->cs);
                    int np = (chroma_format == XEVD_CF_YCBCR400) ? 1 : 3;

                    for (int i = 0; i < np; i++)
                    {
                        imgb_t->w[i] = imgb->w[i];
                        imgb_t->h[i] = imgb->h[i];
                    }
                }

                if (!pic_cnt && is_y4m)
                {
                    if(write_y4m_header(op_fname_out, imgb))
                    {
                        proc_ret = XEVD_ERR;
                        goto END;
                    }
                }
                write_dec_img(id, op_fname_out, imgb, imgb_t, is_y4m);
            }

            imgb->release(imgb);
            pic_cnt++;
        }
        fflush(stdout);
        fflush(stderr);
    }

END:
    logv1_line("Summary");
    logv1("Resolution                        = %d x %d\n", act_w, act_h);
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
    logv1_line(NULL);

    if(id) xevd_delete(id);
    if(imgb_t) imgb_free(imgb_t);
    if(fp_bs) fclose(fp_bs);
    if(bs_buf) free(bs_buf);

    return proc_ret;
}


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

#ifndef _XEVD_APP_UTIL_H_
#define _XEVD_APP_UTIL_H_


#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>

/* logging functions */
static void log_msg(char * filename, int line, const char *fmt, ...)
{
    char str[1024]={'\0',};
    if(filename != NULL && line >= 0) sprintf(str, "[%s:%d] ", filename, line);
    va_list args;
    va_start(args, fmt);
    vsprintf(str + strlen(str), fmt, args);
    va_end(args);
    printf("%s", str);
}

static void log_line(char * pre)
{
    int i, len;
    char str[128]={'\0',};
    const int chars = 80;
    for(i = 0 ; i< 3; i++) {str[i] = '=';}
    str[i] = '\0';

    len = (pre == NULL)? 0: (int)strlen(pre);
    if(len > 0)
    {
        sprintf(str + 3, " %s ", pre);
        len = (int)strlen(str);
    }

    for(i = len ; i< chars; i++) {str[i] = '=';}
    str[chars] = '\0';
    printf("%s\n", str);
}


#if defined(__GNUC__)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define logerr(args...) log_msg(__FILENAME__, __LINE__, args)

#define logv0(args...) {if(op_verbose >= VERBOSE_0) {log_msg(NULL, -1, args);}}
#define logv1(args...) {if(op_verbose >= VERBOSE_1) {log_msg(NULL, -1, args);}}
#define logv2(args...) {if(op_verbose >= VERBOSE_2) {log_msg(NULL, -1, args);}}
#else
#define __FILENAME__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define logerr(args, ...) log_msg(__FILENAME__, __LINE__, args, __VA_ARGS__)
#define logv0(args,...) \
    {if(op_verbose >= VERBOSE_0){log_msg(NULL, -1, args,__VA_ARGS__);}}
#define logv1(args,...) \
    {if(op_verbose >= VERBOSE_1){log_msg(NULL, -1, args,__VA_ARGS__);}}
#define logv2(args,...) \
    {if(op_verbose >= VERBOSE_2){log_msg(NULL, -1, args,__VA_ARGS__);}}
#endif
#define logv1_line(pre) {if(op_verbose >= VERBOSE_1) {log_line(pre);}}
#define logv2_line(pre) {if(op_verbose >= VERBOSE_2) {log_line(pre);}}


#define VERBOSE_0                  0
#define VERBOSE_1                  1
#define VERBOSE_2                  2

static int op_verbose = VERBOSE_1;

/* Clocks */
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>

#define XEVD_CLK             DWORD
#define XEVD_CLK_PER_SEC     (1000)
#define XEVD_CLK_PER_MSEC    (1)
#define XEVD_CLK_MAX         ((XEVD_CLK)(-1))
#define xevd_clk_get()       GetTickCount()

#elif __linux__ || __CYGWIN__ || __APPLE__
#include <time.h>
#include <sys/time.h>
#define XEVD_CLK             unsigned long
#define XEVD_CLK_MAX         ((XEVD_CLK)(-1))
#define XEVD_CLK_PER_SEC     (10000)
#define XEVD_CLK_PER_MSEC    (10)
static XEVD_CLK xevd_clk_get(void)
{
    XEVD_CLK clk;
    struct timeval t;
    gettimeofday(&t, NULL);
    clk = t.tv_sec*10000L + t.tv_usec/100L;
    return clk;
}

#else
#error THIS PLATFORM CANNOT SUPPORT CLOCK
#endif

#define xevd_clk_diff(t1, t2) \
    (((t2) >= (t1)) ? ((t2) - (t1)) : ((XEVD_CLK_MAX - (t1)) + (t2)))

static XEVD_CLK xevd_clk_from(XEVD_CLK from) \
{
    XEVD_CLK now = xevd_clk_get(); \
    return xevd_clk_diff(from, now); \
}

#define xevd_clk_msec(clk) \
    ((int)((clk + (XEVD_CLK_PER_MSEC/2))/XEVD_CLK_PER_MSEC))
#define xevd_clk_sec(clk)  \
    ((int)((clk + (XEVD_CLK_PER_SEC/2))/XEVD_CLK_PER_SEC))

#define XEVDA_CLIP(n,min,max) (((n)>(max))? (max) : (((n)<(min))? (min) : (n)))
/* rounded right shift */
#define XEVDA_RRSHIFT(v, s) ((s>0)? (((v) + (1<<((s) - 1))) >> (s)):(v))
/* rounded toward ceil(inf.) with right shift. Assume v>=0 & s>=0 */
#define XEVDA_CRSHIFT(v, s) (((v) + (1<<(s)) - 1) >> (s))

static int imgb_read(FILE * fp, XEVD_IMGB * img)
{
    int f_w, f_h;
    int size_l, size_cb;
    int chroma_format = XEVD_CS_GET_FORMAT(img->cs);
    int w_shift = ((chroma_format == XEVD_CF_YCBCR420) || (chroma_format == XEVD_CF_YCBCR422)) ? 1 : 0;
    int h_shift = chroma_format == XEVD_CF_YCBCR420 ? 1 : 0;

    f_w = img->w[0];
    f_h = img->h[0];

    if((XEVD_CS_GET_BIT_DEPTH(img->cs)) == 8)
    {
        size_l = f_w * f_h;
        if(fread(img->a[0], 1, size_l, fp) != (unsigned)size_l)
        {
            return XEVD_ERR;
        }
        size_cb = XEVDA_CRSHIFT(f_w, w_shift) * XEVDA_CRSHIFT(f_h, h_shift);
        if(chroma_format != XEVD_CF_YCBCR400)
        {
            if(fread(img->a[1], 1, size_cb, fp) != (unsigned)size_cb)
            {
                return XEVD_ERR;
            }
            if(fread(img->a[2], 1, size_cb, fp) != (unsigned)size_cb)
            {
                return XEVD_ERR;
            }
        }
    }
    else if(((XEVD_CS_GET_BIT_DEPTH(img->cs)) >= 10))
    {

        size_l = f_w * f_h * sizeof(short);
        if(fread(img->a[0], 1, size_l, fp) != (unsigned)size_l)
        {
            return XEVD_ERR;
        }
        size_cb = XEVDA_CRSHIFT(f_w, w_shift) * XEVDA_CRSHIFT(f_h, h_shift) *
            sizeof(short);
        if(chroma_format != XEVD_CF_YCBCR400)
        {
            if(fread(img->a[1], 1, size_cb, fp) != (unsigned)size_cb)
            {
                return XEVD_ERR;
            }
            if(fread(img->a[2], 1, size_cb, fp) != (unsigned)size_cb)
            {
                return XEVD_ERR;
            }
        }
    }
    else
    {
        logv0("not supported color space\n");
        return XEVD_ERR;
    }

    return XEVD_OK;
}

static int imgb_write(char * fname, XEVD_IMGB * img)
{
    unsigned char * p8;
    int             i, j, bd;
    int             cs_w_off, cs_h_off;
    FILE          * fp;

    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logv0("cannot open file = %s\n", fname);
        return XEVD_ERR;
    }
    if ((img->cs & 0xff) != XEVD_CF_YCBCR400)
    {
        cs_w_off = 2;
        cs_h_off = 2;
    }
    else
    {
         cs_w_off = 1;
         cs_h_off = 1;
    }
    if((XEVD_CS_GET_BIT_DEPTH(img->cs)) == 8)
        bd = 1;
    else if((XEVD_CS_GET_BIT_DEPTH(img->cs)) >= 10)
        bd = 2;
    else
    {
        logv0("cannot support the color space\n");
        fclose(fp);
        return XEVD_ERR;
    }
    for(i = 0; i < img->np; i++)
    {
        p8 = (unsigned char *)img->a[i] + (img->s[i] * img->y[i]) + (img->x[i] * bd);
        int tw, th, tcl, tcr, tct, tcb;
        tw = img->w[i];
        th = img->h[i];
        tcl = tcr = tct = tcb = 0;

        if (!i)
        {
            tw = img->w[i] - (cs_w_off * (img->crop_l + img->crop_r));
            th = img->h[i] - (cs_h_off * (img->crop_t + img->crop_b));
            tcl = img->crop_l * cs_w_off;
            tcr = img->crop_r * cs_w_off;
            tct = img->crop_t * cs_h_off;
            tcb = img->crop_b * cs_h_off;
        }
        else
        {
            tw = img->w[i] - (img->crop_l + img->crop_r);
            th = img->h[i] - (img->crop_t + img->crop_b);
            tcl = img->crop_l;
            tcr = img->crop_r;
            tct = img->crop_t;
            tcb = img->crop_b;
        }

        for (j = 0; j < tct; j++)
        {
            p8 += img->s[i];
        }

        for(j = 0; j < th; j++)
        {
            fwrite(p8 + tcl * bd, tw * bd, 1, fp);
            p8 += img->s[i];
        }
    }
    fclose(fp);
    return 0;
}

static void __imgb_cpy_plane(void *src, void *dst, int bw, int h, int s_src, int s_dst)
{
    int i;
    unsigned char *s, *d;

    s = (unsigned char*)src;
    d = (unsigned char*)dst;

    for(i = 0; i < h; i++)
    {
        memcpy(d, s, bw);
        s += s_src;
        d += s_dst;
    }
}

static int write_data(char * fname, unsigned char * data, int size)
{
    FILE * fp;

    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logv0("cannot open an writing file=%s\n", fname);
        return XEVD_ERR;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 0;
}

static void imgb_conv_8b_to_16b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
    if (imgb_src->crop_idx)
    {
        imgb_dst->crop_idx = imgb_src->crop_idx;
        imgb_dst->crop_l = imgb_src->crop_l;
        imgb_dst->crop_r = imgb_src->crop_r;
        imgb_dst->crop_t = imgb_src->crop_t;
        imgb_dst->crop_b = imgb_src->crop_b;
    }
}

static void imgb_conv_16b_to_8b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{

    int i, j, k, t0, add;

    short         * s;
    unsigned char * d;

    add = 1 << (shift - 1);

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVDA_CLIP(t0, 0, 255));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = d + imgb_dst->s[i];
        }
    }
    if (imgb_src->crop_idx)
    {
        imgb_dst->crop_idx = imgb_src->crop_idx;
        imgb_dst->crop_l = imgb_src->crop_l;
        imgb_dst->crop_r = imgb_src->crop_r;
        imgb_dst->crop_t = imgb_src->crop_t;
        imgb_dst->crop_b = imgb_src->crop_b;
    }
}

static void imgb_cpy(XEVD_IMGB * dst, XEVD_IMGB * src)
{
    int i, bd;

    if(src->cs == dst->cs)
    {
        if(src->cs == XEVD_CS_YCBCR420_10LE || src->cs == XEVD_CS_YCBCR400_10LE) bd = 2;
        else bd = 1;

        for(i = 0; i < src->np; i++)
        {
            __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i],
                             src->s[i], dst->s[i]);

        }
    }
    else if((src->cs == XEVD_CS_YCBCR420 && dst->cs == XEVD_CS_YCBCR420_10LE) || (src->cs == XEVD_CS_YCBCR400 && dst->cs == XEVD_CS_YCBCR400_10LE) )
    {
        imgb_conv_8b_to_16b(dst, src, 2);
    }
    else if((src->cs == XEVD_CS_YCBCR420_10LE && dst->cs == XEVD_CS_YCBCR420) ||  (src->cs == XEVD_CS_YCBCR400_10LE && dst->cs == XEVD_CS_YCBCR400))
    {
        imgb_conv_16b_to_8b(dst, src, 2);
    }
    else
    {
        logv0("ERROR: unsupported image copy\n");
        return;
    }
    for( i = 0; i < XEVD_TS_NUM; i++)
    {
        dst->ts[i] = src->ts[i];
    }
    if (src->crop_idx)
    {
        dst->crop_idx = src->crop_idx;
        dst->crop_l = src->crop_l;
        dst->crop_r = src->crop_r;
        dst->crop_t = src->crop_t;
        dst->crop_b = src->crop_b;
    }
    dst->imgb_active_pps_id = src->imgb_active_pps_id;
    dst->imgb_active_aps_id = src->imgb_active_aps_id;
}

static void imgb_conv_shift_left_8b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for(i = 0; i < imgb_dst->np; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}
static void imgb_conv_shift_right_8b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{

    int i, j, k, t0, add;

    short         * s;
    unsigned char * d;

    if(shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    for(i = 0; i < imgb_dst->np; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVDA_CLIP(t0, 0, 255));
            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = d + imgb_dst->s[i];
        }
    }
}

static void imgb_conv_shift_left(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for(i = 0; i < imgb_dst->np; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (unsigned short)(s[k] << shift);
            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_conv_shift_right(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src, int shift)
{

    int i, j, k, t0, add;

    int clip_min = 0;
    int clip_max = 0;

    unsigned short * s;
    unsigned short * d;
    if(shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    clip_max = (1 << (XEVD_CS_GET_BIT_DEPTH(imgb_dst->cs))) - 1;

    for(i = 0; i < imgb_dst->np; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (XEVDA_CLIP(t0, clip_min, clip_max));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_cpy_bd(XEVD_IMGB * dst, XEVD_IMGB * src)
{
    int i, bd;
    int bit_depth = XEVD_CS_GET_BIT_DEPTH(src->cs);
    int idc = XEVD_CS_GET_FORMAT(src->cs);
    if(src->cs == dst->cs)
    {
        if(bit_depth >= 10)
            bd = 2;
        else
            bd = 1;
        for(i = 0; i < src->np; i++)
        {
            __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i], src->s[i], dst->s[i]);
        }
    }
    else if(bit_depth == 8)
    {
        imgb_conv_shift_left_8b(dst, src, ((XEVD_CS_GET_BIT_DEPTH(dst->cs)) - XEVD_CS_GET_BIT_DEPTH(src->cs))); //complicated formula because of colour space macors
    }
    else if(bit_depth == 10)
    {
        if((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 8)
            imgb_conv_shift_right_8b(dst, src, 2);
        else
            imgb_conv_shift_left(dst, src, ((XEVD_CS_GET_BIT_DEPTH(dst->cs)) - XEVD_CS_GET_BIT_DEPTH(src->cs))); //complicated formula because of colour space macors
    }
    else if(bit_depth == 12)
    {
        if((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 8)
            imgb_conv_shift_right_8b(dst, src, 4);
        else if((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 10)
            imgb_conv_shift_right(dst, src, ((XEVD_CS_GET_BIT_DEPTH(src->cs)) - XEVD_CS_GET_BIT_DEPTH(dst->cs)));
        else
            imgb_conv_shift_left(dst, src, ((XEVD_CS_GET_BIT_DEPTH(dst->cs)) - XEVD_CS_GET_BIT_DEPTH(src->cs))); //complicated formula because of colour space macors
    }
    else if(bit_depth == 14)
    {
        if((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 8)
            imgb_conv_shift_right_8b(dst, src, 6);
        else if(((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 10) || ((XEVD_CS_GET_BIT_DEPTH(dst->cs)) == 12))
            imgb_conv_shift_right(dst, src, ((XEVD_CS_GET_BIT_DEPTH(src->cs)) - XEVD_CS_GET_BIT_DEPTH(dst->cs)));
        else
            imgb_conv_shift_left(dst, src, ((XEVD_CS_GET_BIT_DEPTH(dst->cs)) - XEVD_CS_GET_BIT_DEPTH(src->cs))); //complicated formula because of colour space macors
    }
    else
    {
        logv0("ERROR: unsupported image copy\n");
        return;
    }
    for(i = 0; i < 4; i++)
    {
        dst->ts[i] = src->ts[i];
    }
    if(src->crop_idx)
    {
        dst->crop_idx = src->crop_idx;
        dst->crop_l = src->crop_l;
        dst->crop_r = src->crop_r;
        dst->crop_t = src->crop_t;
        dst->crop_b = src->crop_b;
    }
    dst->imgb_active_pps_id = src->imgb_active_pps_id;
    dst->imgb_active_aps_id = src->imgb_active_aps_id;
}
static void imgb_cpy_inp_to_codec(XEVD_IMGB * dst, XEVD_IMGB * src)
{
    int i, bd;
    int src_bd, dst_bd;
    src_bd = XEVD_CS_GET_BIT_DEPTH(src->cs);
    dst_bd = XEVD_CS_GET_BIT_DEPTH(dst->cs);
    if(src_bd == 8)
    {
        imgb_conv_shift_left_8b(dst, src, dst_bd - src_bd);
    }
    else
    {
        if(src->cs == dst->cs)
        {
            bd = 2;
            for(i = 0; i < src->np; i++)
            {
                __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i], src->s[i], dst->s[i]);
            }
        }
        else if(src->cs > dst->cs)
        {
            imgb_conv_shift_right(dst, src, src_bd - dst_bd);
        }
        else
        {
            imgb_conv_shift_left(dst, src, dst_bd - src_bd);
        }
    }
    for(i = 0; i < 4; i++)
    {
        dst->ts[i] = src->ts[i];
    }
    if(src->crop_idx)
    {
        dst->crop_idx = src->crop_idx;
        dst->crop_l = src->crop_l;
        dst->crop_r = src->crop_r;
        dst->crop_t = src->crop_t;
        dst->crop_b = src->crop_b;
    }
    dst->imgb_active_pps_id = src->imgb_active_pps_id;
    dst->imgb_active_aps_id = src->imgb_active_aps_id;
}

static void imgb_cpy_codec_to_out(XEVD_IMGB * dst, XEVD_IMGB * src)
{
    int i, bd;
    int src_bd, dst_bd;
    src_bd = XEVD_CS_GET_BIT_DEPTH(src->cs);
    dst_bd = XEVD_CS_GET_BIT_DEPTH(dst->cs);
    if(dst_bd == 8)
    {
        imgb_conv_shift_right_8b(dst, src, src_bd - dst_bd);
    }
    else
    {
        if(src->cs == dst->cs)
        {
            bd = 2;
            for(i = 0; i < src->np; i++)
            {
                __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i], src->s[i], dst->s[i]);
            }
        }
        else if(src->cs > dst->cs)
        {
            imgb_conv_shift_right(dst, src, src_bd - dst_bd);
        }
        else
        {
            imgb_conv_shift_left(dst, src, dst_bd - src_bd);
        }
    }
    for(i = 0; i < 4; i++)
    {
        dst->ts[i] = src->ts[i];
    }
    if(src->crop_idx)
    {
        dst->crop_idx = src->crop_idx;
        dst->crop_l = src->crop_l;
        dst->crop_r = src->crop_r;
        dst->crop_t = src->crop_t;
        dst->crop_b = src->crop_b;
    }
    dst->imgb_active_pps_id = src->imgb_active_pps_id;
    dst->imgb_active_aps_id = src->imgb_active_aps_id;
}

static void imgb_free(XEVD_IMGB * imgb)
{
    int i;
    for(i = 0; i < XEVD_IMGB_MAX_PLANE; i++)
    {
        if(imgb->baddr[i]) free(imgb->baddr[i]);
    }
    free(imgb);
}

XEVD_IMGB * imgb_alloc(int w, int h, int cs)
{
    int i;
    XEVD_IMGB * imgb;

    imgb = (XEVD_IMGB *)malloc(sizeof(XEVD_IMGB));
    if(imgb == NULL)
    {
        logv0("cannot create image buffer\n");
        return NULL;
    }
    memset(imgb, 0, sizeof(XEVD_IMGB));
    int chroma_format = XEVD_CS_GET_FORMAT(cs);
    int w_shift = (chroma_format == XEVD_CF_YCBCR420) || (chroma_format == XEVD_CF_YCBCR422) ? 1 : 0;
    int h_shift = chroma_format == XEVD_CF_YCBCR420 ? 1 : 0;
    int np = chroma_format == XEVD_CF_YCBCR400 ? 1 : 3;

    if(XEVD_CS_GET_BIT_DEPTH(cs)==8)
    {
        for(i = 0; i < np; i++)
        {
            imgb->w[i] = imgb->aw[i] = imgb->s[i] = w;
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]);
            if(imgb->a[i] == NULL)
            {
                logv0("cannot allocate picture buffer\n");
                return NULL;
            }

            if(i == 0)
            {
                w = (w + 1) >> w_shift;
                h = (h + 1) >> h_shift;
            }
        }
        imgb->np = np;
    }
     else if(cs == XEVD_CS_YCBCR420_10LE
         || cs == XEVD_CS_YCBCR420_12LE || cs == XEVD_CS_YCBCR420_14LE
         || cs == XEVD_CS_YCBCR400_10LE || cs == XEVD_CS_YCBCR400_12LE || cs == XEVD_CS_YCBCR400_14LE
         || cs == XEVD_CS_YCBCR422_10LE || cs == XEVD_CS_YCBCR444_10LE  )
    {
        for(i = 0; i < np; i++)
        {
            imgb->w[i] = imgb->aw[i] = w;
            imgb->s[i] = w * sizeof(short);
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]);
            if(imgb->a[i] == NULL)
            {
                logv0("cannot allocate picture buffer\n");
                return NULL;
            }

            if(i == 0)
            {
                w = (w + 1) >> w_shift;
                h = (h + 1) >> h_shift;
            }
        }
        imgb->np = np;
    }
    else
    {
        logv0("unsupported color space\n");
        if(imgb)free(imgb);
        return NULL;
    }

    imgb->cs = cs;
    return imgb;
}


#endif /* _XEVD_APP_UTIL_H_ */

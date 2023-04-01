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

#include "xevd_def.h"

void xevd_get_nbr_b(int x, int y, int cuw, int cuh, pel *src, int s_src, u16 avail_cu, pel nb[N_C][N_REF][MAX_CU_SIZE * 3], int scup, u32 * map_scu
                  , int w_scu, int h_scu, int ch_type, int constrained_intra_pred ,u8* map_tidx
                  , int bit_depth, int chroma_format_idc)
{
    int  i, j;
    int  scuw = (ch_type == Y_C) ? (cuw >> MIN_CU_LOG2) : (cuw >> (MIN_CU_LOG2 - (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))));
    int  scuh = (ch_type == Y_C) ? (cuh >> MIN_CU_LOG2) : (cuh >> (MIN_CU_LOG2 - (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))));
    scuh = ((ch_type != Y_C) && (chroma_format_idc == 2)) ? scuh * 2 : scuh;
    int  unit_size = (ch_type == Y_C) ? MIN_CU_SIZE : (MIN_CU_SIZE >> 1);
    unit_size = ((ch_type != Y_C) && (chroma_format_idc == 3)) ? unit_size * 2 : unit_size;
    int  x_scu = PEL2SCU(ch_type == Y_C ? x : x << (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)));
    int  y_scu = PEL2SCU(ch_type == Y_C ? y : y << (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
    pel *tmp = src;
    pel *left = nb[ch_type][0] + 2;
    pel *up = nb[ch_type][1] + cuh;

    if (IS_AVAIL(avail_cu, AVAIL_UP_LE) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu - 1])) &&
        (map_tidx[scup] == map_tidx[scup - w_scu - 1]))
    {
        xevd_mcpy(up - 1, src - s_src - 1, cuw * sizeof(pel));
    }
    else
    {
        up[-1] = 1 << (bit_depth - 1);
    }

    for (i = 0; i < (scuw + scuh); i++)
    {
        int is_avail = (y_scu > 0) && (x_scu + i < w_scu);
        if (is_avail && MCU_GET_COD(map_scu[scup - w_scu + i]) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu + i])) &&
            (map_tidx[scup] == map_tidx[scup - w_scu + i]))
        {
            xevd_mcpy(up + i * unit_size, src - s_src + i * unit_size, unit_size * sizeof(pel));
        }
        else
        {
            xevd_mset_16b(up + i * unit_size, 1 << (bit_depth - 1), unit_size);
        }
    }

    src--;
    for (i = 0; i < (scuh + scuw); ++i)
    {
        int is_avail = (x_scu > 0) && (y_scu + i < h_scu);
        if (is_avail && MCU_GET_COD(map_scu[scup - 1 + i * w_scu]) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - 1 + i * w_scu])) &&
            (map_tidx[scup] == map_tidx[scup - 1 + i * w_scu]))
        {
            for (j = 0; j < unit_size; ++j)
            {
                left[i * unit_size + j] = *src;
                src += s_src;
            }
        }
        else
        {
            xevd_mset_16b(left + i * unit_size, 1 << (bit_depth - 1), unit_size);
            src += (s_src * unit_size);
        }
    }
    left[-1] = up[-1];
}

void ipred_hor_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            dst[j] = src_le[0];
        }
        dst += w; src_le++;
    }
}

static const int lut_size_plus1[MAX_CU_LOG2 + 1] = { 2048, 1365, 819, 455, 241, 124, 63, 32 }; // 1/(w+1) = k >> 12

void xevd_ipred_vert(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            dst[j] = src_up[j];
        }
        dst += w;
    }
}

int xevd_get_dc(const int numerator, const int w, const int h)
{
    const int log2_w = xevd_tbl_log2[w];
    const int log2_h = xevd_tbl_log2[h];
    const int shift_w = 12;

    int basic_shift = log2_w, log2_asp_ratio = 0;

    if (log2_w > log2_h)
    {
        basic_shift = log2_h;
        log2_asp_ratio = log2_w - log2_h;
    }
    else if (log2_w < log2_h)
    {
        basic_shift = log2_w;
        log2_asp_ratio = log2_h - log2_w;
    }

  return (numerator * lut_size_plus1[log2_asp_ratio]) >> (basic_shift + shift_w);
}

void ipred_dc_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int dc = 0;
    int wh, i, j;

    for (i = 0; i < h; i++) dc += src_le[i];
    for (j = 0; j < w; j++) dc += src_up[j];
    dc = (dc + w) >> (xevd_tbl_log2[w] + 1);

    wh = w * h;

    for(i = 0; i < wh; i++)
    {
        dst[i] = (pel)dc;
    }
}

void xevd_ipred_plane(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h
    , int bit_depth
)
{
    pel *rsrc;
    int  coef_h = 0, coef_v = 0;
    int  a, b, c, x, y;
    int  w2 = w >> 1;
    int  h2 = h >> 1;
    int  ib_mult[6] = {13, 17, 5, 11, 23, 47};
    int  ib_shift[6] = {7, 10, 11, 15, 19, 23};
    int  idx_w = xevd_tbl_log2[w] < 2 ? 0 : xevd_tbl_log2[w] - 2;
    int  idx_h = xevd_tbl_log2[h] < 2 ? 0 : xevd_tbl_log2[h] - 2;
    int  im_h, is_h, im_v, is_v, temp, temp2;

    im_h = ib_mult[idx_w];
    is_h = ib_shift[idx_w];
    im_v = ib_mult[idx_h];
    is_v = ib_shift[idx_h];

    if(avail_lr == LR_01 || avail_lr == LR_11)
    {
        rsrc = src_up + w2;
        for(x = 1; x < w2 + 1; x++)
        {
            coef_h += x * (rsrc[-x] - rsrc[x]);
        }

        rsrc = src_ri + (h2 - 1);
        for(y = 1; y < h2 + 1; y++)
        {
            coef_v += y * (rsrc[y] - rsrc[-y]);
        }

        a = (src_ri[h - 1] + src_up[0]) << 4;
        b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
        c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;

        temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;

        for(y = 0; y < h; y++)
        {
            temp2 = temp;
            for(x = w - 1; x >= 0; x--)
            {

                dst[x] = XEVD_CLIP3(0, (1 << bit_depth) - 1, temp2 >> 5);

                temp2 += b;
            }
            temp += c; dst += w;
        }
    }
    else
    {
        rsrc = src_up + (w2 - 1);
        for(x = 1; x < w2 + 1; x++)
        {
            coef_h += x * (rsrc[x] - rsrc[-x]);
        }

        rsrc = src_le + (h2 - 1);
        for(y = 1; y < h2 + 1; y++)
        {
            coef_v += y * (rsrc[y] - rsrc[-y]);
        }

        a = (src_le[h - 1] + src_up[w - 1]) << 4;
        b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
        c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;

        temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;

        for(y = 0; y < h; y++)
        {
            temp2 = temp;
            for(x = 0; x < w; x++)
            {

                dst[x] = XEVD_CLIP3(0, (1 << bit_depth) - 1, temp2 >> 5);

                temp2 += b;
            }
            temp += c; dst += w;
        }
    }
}

void xevd_ipred_bi(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int bit_depth)
{
    int x, y;
    int ishift_x = xevd_tbl_log2[w];
    int ishift_y = xevd_tbl_log2[h];
    int ishift = XEVD_MIN(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a, b, c, wt, wxy, tmp;
    int predx;
    int ref_up[MAX_CU_SIZE], ref_le[MAX_CU_SIZE], up[MAX_CU_SIZE], le[MAX_CU_SIZE], wy[MAX_CU_SIZE];
    int ref_ri[MAX_CU_SIZE], ri[MAX_CU_SIZE];
    int dst_tmp[MAX_CU_SIZE][MAX_CU_SIZE];
    int wc, tbl_wc[6] = {-1, 341, 205, 114, 60, 31};
    int log2_w = xevd_tbl_log2[w];
    int log2_h = xevd_tbl_log2[h];
    int multi_w = lut_size_plus1[log2_w];
    int shift_w = 12;

    wc = ishift_x > ishift_y ? ishift_x - ishift_y : ishift_y - ishift_x;
    xevd_assert(wc <= 5);
    wc = tbl_wc[wc];

    for(x = 0; x < w; x++) ref_up[x] = src_up[x];
    for(y = 0; y < h; y++) ref_le[y] = src_le[y];
    for(y = 0; y < h; y++) ref_ri[y] = src_ri[y];

    if(avail_lr == LR_11)
    {
        for(y = 0; y < h; y++)
        {
            for(x = 0; x < w; x++)
            {
                dst_tmp[y][x] = (ref_le[y] * (w - x) + ref_ri[y] * (x + 1) + (w >> 1)) * multi_w >> shift_w;
            }
        }

        for(x = 0; x < w; x++)
        {
            for(y = 0; y < h; y++)
            {
                tmp = (ref_up[x] * (h - 1 - y) + dst_tmp[h - 1][x] * (y + 1) + (h >> 1)) >> log2_h;
                dst[y * w + x] = (dst_tmp[y][x] + tmp + 1) >> 1;
            }
        }
    }
    else if(avail_lr == LR_01)
    {
        a = src_up[-1];
        b = src_ri[h];
        c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 9))) >> (ishift + 10);
        wt = (c << 1) - a - b;

        for(x = w - 1; x >= 0; x--)
        {
            up[x] = b - ref_up[x];
            ref_up[x] <<= ishift_y;
        }
        tmp = 0;
        for(y = 0; y < h; y++)
        {
            ri[y] = a - ref_ri[y];
            ref_ri[y] <<= ishift_x;
            wy[y] = tmp;
            tmp += wt;
        }

        for(y = 0; y < h; y++)
        {
            predx = ref_ri[y];
            wxy = 0;
            for(x = w - 1; x >= 0; x--)
            {
                predx += ri[y];
                ref_up[x] += up[x];
                dst[x] = ((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy;
                dst[x] = XEVD_CLIP3(0, (1 << bit_depth) - 1, dst[x]);
                wxy += wy[y];
            }
            dst += w;
        }
    }
    else
    {
        a = src_up[w];
        b = src_le[h];
        c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 9))) >> (ishift + 10);
        wt = (c << 1) - a - b;

        for(x = 0; x < w; x++)
        {
            up[x] = b - ref_up[x];
            ref_up[x] <<= ishift_y;
        }
        tmp = 0;
        for(y = 0; y < h; y++)
        {
            le[y] = a - ref_le[y];
            ref_le[y] <<= ishift_x;
            wy[y] = tmp;
            tmp += wt;
        }

        for(y = 0; y < h; y++)
        {
            predx = ref_le[y];
            wxy = 0;
            for(x = 0; x < w; x++)
            {
                predx += le[y];
                ref_up[x] += up[x];
                dst[x] = XEVD_CLIP3(0, (1 << bit_depth) - 1, (((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy));
                wxy += wy[y];
            }
            dst += w;
        }
    }
}

#define GET_REF_POS(mt,d_in,d_out,offset) \
    (d_out) = ((d_in) * (mt)) >> 10;\
    (offset) = (((d_in) * (mt)) >> 5) - ((d_out) << 5);

#define ADI_4T_FILTER_BITS                 7
#define ADI_4T_FILTER_OFFSET              (1<<(ADI_4T_FILTER_BITS-1))

static __inline pel ipred_ang_val(pel * src_up, pel * src_le, pel * src_ri, u16 avail_lr, int ipm, int i, int j, int w, int pos_min, int pos_max, int h
    , int bit_depth)
{
    int offset;
    int t_dx, t_dy;
    int x, y, xn, yn, xn_n1, yn_n1, xn_p2, yn_p2;
    const int dxy = (ipm > IPD_HOR || ipm < IPD_VER) ? -1 : 1;
    const int * filter;
    const int(*tbl_filt)[4];
    int filter_offset, filter_bits;
    const int * mt = xevd_tbl_ipred_dxdy[ipm];
    pel * src_ch = NULL;
    int num_selections = 0;
    int use_x;
    int p, pn, pn_n1, pn_p2;
    pel temp_pel = 0;
    int refpos = 0;

    x = INT_MAX;
    y = INT_MAX;

    tbl_filt = xevd_tbl_ipred_adi;
    filter_offset = ADI_4T_FILTER_OFFSET;
    filter_bits = ADI_4T_FILTER_BITS;

    xevd_assert(ipm < IPD_CNT);

    if(ipm < IPD_VER)
    {
        /* case x-line */
        GET_REF_POS(mt[0], j + 1, t_dx, offset);

        if((avail_lr == LR_01 || avail_lr == LR_11) && i >= (w - t_dx))
        {
            GET_REF_POS(mt[1], w - i, t_dy, offset);
            x = w;
            y = j - t_dy;
            refpos = 2;
        }
        else
        {
            x = i + t_dx;
            y = -1;
            refpos = 0;
        }
    }
    else if(ipm > IPD_HOR)
    {
        if (avail_lr == LR_01 || avail_lr == LR_11)
        {
            GET_REF_POS(mt[1], w - i, t_dy, offset);

            if(j < t_dy)
            {
                GET_REF_POS(mt[0], w - i, t_dx, offset);
                x = i + t_dx;
                y = -1;
                refpos = 0;
            }
            else
            {
                x = w;
                y = j - t_dy;
                refpos = 2;
            }
        }
        else
        {
            GET_REF_POS(mt[1], i + 1, t_dy, offset);
            x = -1;
            y = j + t_dy;
            refpos = 1;
        }
    }
    else
    {
        GET_REF_POS(mt[1], i + 1, t_dy, offset);

        if(j < t_dy)
        {
            GET_REF_POS(mt[0], j + 1, t_dx, offset);
            x = i - t_dx;
            y = -1;
            refpos = 0;
        }
        else
        {
            if (avail_lr == LR_01)
            {
                GET_REF_POS(mt[1], w - i, t_dy, offset);
                x = w;
                y = j + t_dy;
                refpos = 2;
            }
            else
            {
                x = -1;
                y = j - t_dy;
                refpos = 1;
            }
        }
    }

    xevd_assert(x != INT_MAX);
    xevd_assert(y != INT_MAX);

    if(refpos == 0)
    {
        if(dxy < 0)
        {
            xn_n1 = x - 1;
            xn = x + 1;
            xn_p2 = x + 2;
        }
        else
        {
            xn_n1 = x + 1;
            xn = x - 1;
            xn_p2 = x - 2;
        }

        use_x = 1;
        ++num_selections;
        src_ch = src_up;
    }
    else if(refpos == 1)
    {
        if(dxy < 0)
        {
            yn_n1 = y - 1;
            yn = y + 1;
            yn_p2 = y + 2;
        }
        else
        {
            yn_n1 = y + 1;
            yn = y - 1;
            yn_p2 = y - 2;
        }

        use_x = 0;
        ++num_selections;
        src_ch = src_le;
    }
    else if(refpos == 2)
    {
        if(dxy > 0)
        {
            yn_n1 = y - 1;
            yn = y + 1;
            yn_p2 = y + 2;
        }
        else
        {
            yn_n1 = y + 1;
            yn = y - 1;
            yn_p2 = y - 2;
        }

        use_x = 0;
        ++num_selections;
        src_ch = src_ri;
    }

    xevd_assert(num_selections == 1);
    xevd_assert(src_ch != NULL);

    if(use_x)
    {
        pn_n1 = xn_n1;
        p = x;
        pn = xn;
        pn_p2 = xn_p2;
    }
    else
    {
        pn_n1 = yn_n1;
        p = y;
        pn = yn;
        pn_p2 = yn_p2;
    }

    pn_n1 = XEVD_MAX(XEVD_MIN(pn_n1, pos_max), pos_min);
    p = XEVD_MAX(XEVD_MIN(p, pos_max), pos_min);
    pn = XEVD_MAX(XEVD_MIN(pn, pos_max), pos_min);
    pn_p2 = XEVD_MAX(XEVD_MIN(pn_p2, pos_max), pos_min);
    filter = (tbl_filt + offset)[0];

    temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + filter_offset) >> filter_bits;

    return XEVD_CLIP3(0, (1 << bit_depth) - 1, temp_pel);

}

void ipred_ang(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    int i, j;
    const int pos_max = w + h - 1;
    const int pos_min = - 1;

    for(j = 0; j < h; j++)
    {
        for(i = 0; i < w; i++)
        {
            dst[i] = ipred_ang_val(src_up, src_le, src_ri, avail_lr, ipm, i, j, w, pos_min, pos_max, h, bit_depth);
        }
        dst += w;
    }
}

void ipred_ul(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i<h; i++)
    {
        for (j = 0; j<w; j++)
        {
            int diag = i - j;
            if (diag > 0) {
                dst[j] = src_le[diag - 1];
            }
            else if (diag == 0)
            {
                dst[j] = src_up[-1];
            }
            else
            {
                dst[j] = src_up[-diag - 1];
            }
        }
        dst += w;
    }
}

void ipred_ur(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i<h; i++)
    {
        for (j = 0; j<w; j++)
        {
            dst[j] = (src_up[i + j + 1] + src_le[i + j + 1]) >> 1;
        }
        dst += w;
    }
}

/* intra prediction for baseline profile */
void xevd_ipred_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm, int w, int h)
{
    switch(ipm)
    {
        case IPD_VER_B:
            xevd_ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR_B:
            ipred_hor_b(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_DC_B:
            ipred_dc_b(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UL_B:
            ipred_ul(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;

        case IPD_UR_B:
            ipred_ur(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        default:
            xevd_trace("\n illegal intra prediction mode\n");
            break;
    }
}

void xevd_ipred_uv_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm_c, int ipm, int w, int h)
{
    switch(ipm_c)
    {

        case IPD_DC_C_B:
            ipred_dc_b(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR_C_B:
            ipred_hor_b(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_VER_C_B:
            xevd_ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UL_C_B:
            ipred_ul(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;

        case IPD_UR_C_B:
            ipred_ur(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        default:
            xevd_trace("\n illegal chroma intra prediction mode\n");
            break;
    }
}

void xevd_get_mpm_b(int x_scu, int y_scu, int cuw, int cuh, u32 *map_scu, u8 *cod_eco, s8 *map_ipm, int scup, int w_scu,
                   u8 ** mpm, u16 avail_lr, u8 mpm_ext[8], u8 pms[IPD_CNT] /* 10 third MPM */, u8 * map_tidx)
{
    u8 ipm_l = IPD_DC, ipm_u = IPD_DC;

    if(x_scu > 0 && MCU_GET_IF(map_scu[scup - 1]) && cod_eco[scup - 1] && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        ipm_l = map_ipm[scup - 1] + 1;
    }
    if(y_scu > 0 && MCU_GET_IF(map_scu[scup - w_scu]) && cod_eco[scup - w_scu] && (map_tidx[scup] == map_tidx[scup - w_scu]))
    {
        ipm_u = map_ipm[scup - w_scu] + 1;
    }
    *mpm = (u8*)&xevd_tbl_mpm[ipm_l][ipm_u];
}

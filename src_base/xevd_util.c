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
#include <math.h>
#include "xevd.h"
#include "xevd_util.h"

#define TX_SHIFT1(log2_size, bd)   ((log2_size) - 1 + bd - 8)
#define TX_SHIFT2(log2_size)   ((log2_size) + 6)

#if ENC_DEC_TRACE
FILE *fp_trace;
int fp_trace_print = 0;
int fp_trace_counter = 0;
#endif

#if X86_SSE
#if (defined(_WIN64) || defined(_WIN32)) && !defined(__GNUC__)
#include <intrin.h >
#elif defined( __GNUC__)
#ifndef _XCR_XFEATURE_ENABLED_MASK
#define _XCR_XFEATURE_ENABLED_MASK 0
#endif
static void __cpuid(int* info, int i)
{
    __asm__ __volatile__(
        "cpuid" : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
                : "a" (i), "c" (0));
}

static unsigned long long __xgetbv(unsigned int i)
{
    unsigned int eax, edx;
    __asm__ __volatile__(
        "xgetbv;" : "=a" (eax), "=d"(edx)
                  : "c" (i));
    return ((unsigned long long)edx << 32) | eax;
}
#endif
#define GET_CPU_INFO(A,B) ((B[((A >> 5) & 0x03)] >> (A & 0x1f)) & 1)

int xevd_check_cpu_info()
{
    int support_sse  = 0;
    int support_avx  = 0;
    int support_avx2 = 0;
    int cpu_info[4]  = { 0 };
    __cpuid(cpu_info, 0);
    int id_cnt = cpu_info[0];

    if (id_cnt >= 1)
    {
        __cpuid(cpu_info, 1);
        support_sse |= GET_CPU_INFO(XEVD_CPU_INFO_SSE41, cpu_info);
        int os_use_xsave = GET_CPU_INFO(XEVD_CPU_INFO_OSXSAVE, cpu_info);
        int cpu_support_avx = GET_CPU_INFO(XEVD_CPU_INFO_AVX, cpu_info);

        if (os_use_xsave && cpu_support_avx)
        {
            unsigned long long xcr_feature_mask = __xgetbv(_XCR_XFEATURE_ENABLED_MASK);
            support_avx = (xcr_feature_mask & 0x6) || 0;
            if (id_cnt >= 7)
            {
                __cpuid(cpu_info, 7);
                support_avx2 = support_avx && GET_CPU_INFO(XEVD_CPU_INFO_AVX2, cpu_info);
            }
        }
    }

    return (support_sse << 1) | support_avx | (support_avx2 << 2);
}
#endif

static void imgb_delete(XEVD_IMGB * imgb)
{
    int i;
    xevd_assert_r(imgb);

    for(i=0; i<XEVD_IMGB_MAX_PLANE; i++)
    {
        if (imgb->baddr[i]) xevd_mfree(imgb->baddr[i]);
    }
    xevd_mfree(imgb);
}

static int imgb_addref(XEVD_IMGB * imgb)
{
    xevd_assert_rv(imgb, XEVD_ERR_INVALID_ARGUMENT);
    return xevd_atomic_inc(&imgb->refcnt);
}

static int imgb_getref(XEVD_IMGB * imgb)
{
    xevd_assert_rv(imgb, XEVD_ERR_INVALID_ARGUMENT);
    return imgb->refcnt;
}

static int imgb_release(XEVD_IMGB * imgb)
{
    int refcnt;
    xevd_assert_rv(imgb, XEVD_ERR_INVALID_ARGUMENT);
    refcnt = xevd_atomic_dec(&imgb->refcnt);
    if(refcnt == 0)
    {
        imgb_delete(imgb);
    }
    return refcnt;
}

const int xevd_chroma_format_idc_to_imgb_cs[4] =
{
    XEVD_CS_YCBCR400_10LE,
    XEVD_CS_YCBCR420_10LE,
    XEVD_CS_YCBCR422_10LE,
    XEVD_CS_YCBCR444_10LE
};

XEVD_IMGB * xevd_imgb_create(int w, int h, int cs, int opt, int pad[XEVD_IMGB_MAX_PLANE], int align[XEVD_IMGB_MAX_PLANE])
{
    int i, p_size, a_size, bd;
    XEVD_IMGB * imgb;

    imgb = (XEVD_IMGB *)xevd_malloc(sizeof(XEVD_IMGB));
    xevd_assert_rv(imgb, NULL);
    xevd_mset(imgb, 0, sizeof(XEVD_IMGB));
    imgb->imgb_active_pps_id = -1;
    imgb->imgb_active_aps_id = -1;


    int bit_depth = (XEVD_CS_GET_BIT_DEPTH(cs));
    int idc = XEVD_CS_GET_FORMAT(cs);
    int np = idc == XEVD_CF_YCBCR400 ? 1 : 3;

    if(bit_depth >= 8 && bit_depth <= 14)
    {
        if(bit_depth == 8) bd = 1;
        else /*if(cs == XEVD_COLORSPACE_YUV420_10LE)*/ bd = 2;
        for(i=0;i<np;i++)

        {
            imgb->w[i] = w;
            imgb->h[i] = h;
            imgb->x[i] = 0;
            imgb->y[i] = 0;

            a_size = (align != NULL)? align[i] : 0;
            p_size = (pad != NULL)? pad[i] : 0;

            if (a_size)
            {
                imgb->aw[i] = XEVD_ALIGN(w, a_size);
                imgb->ah[i] = XEVD_ALIGN(h, a_size);
            }
            else
            {
                imgb->aw[i] = w;
                imgb->ah[i] = h;
            }
            imgb->padl[i] = imgb->padr[i]=imgb->padu[i]=imgb->padb[i]=p_size;

            imgb->s[i] = (imgb->aw[i] + imgb->padl[i] + imgb->padr[i]) * bd;
            imgb->e[i] = imgb->ah[i] + imgb->padu[i] + imgb->padb[i];

            imgb->bsize[i] = imgb->s[i]*imgb->e[i];
            imgb->baddr[i] = xevd_malloc(imgb->bsize[i]);

            imgb->a[i] = ((u8*)imgb->baddr[i]) + imgb->padu[i]*imgb->s[i] +
                imgb->padl[i]*bd;

            if(i == 0)
            {

                if((XEVD_GET_CHROMA_W_SHIFT(idc-10)))
                    w = (w + 1) >> (XEVD_GET_CHROMA_W_SHIFT(idc-10));
                if((XEVD_GET_CHROMA_H_SHIFT(idc-10)))
                    h = (h + 1) >> (XEVD_GET_CHROMA_H_SHIFT(idc-10));

            }
        }
        imgb->np = np;
    }
    else
    {
        xevd_trace("unsupported color space\n");
        xevd_mfree(imgb);
        return NULL;
    }
    imgb->addref = imgb_addref;
    imgb->getref = imgb_getref;
    imgb->release = imgb_release;
    imgb->cs = cs;
    imgb->addref(imgb);

    return imgb;
}

int xevd_atomic_inc(volatile int *pcnt)
{
    int ret;
    ret = *pcnt;
    ret++;
    *pcnt = ret;
    return ret;
}

int xevd_atomic_dec(volatile int *pcnt)
{
    int ret;
    ret = *pcnt;
    ret--;
    *pcnt = ret;
    return ret;
}

XEVD_PIC * xevd_picbuf_lc_alloc(int w, int h, int pad_l, int pad_c, int *err, int idc, int bit_depth)
{
    XEVD_PIC *pic = NULL;
    XEVD_IMGB *imgb = NULL;
    int ret, opt, align[XEVD_IMGB_MAX_PLANE], pad[XEVD_IMGB_MAX_PLANE];
    int w_scu, h_scu, f_scu, size;

    /* allocate PIC structure */
    pic = xevd_malloc(sizeof(XEVD_PIC));
    xevd_assert_gv(pic != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mset(pic, 0, sizeof(XEVD_PIC));

    opt = XEVD_IMGB_OPT_NONE;

    /* set align value*/
    align[0] = MIN_CU_SIZE;
    align[1] = MIN_CU_SIZE >> 1;
    align[2] = MIN_CU_SIZE >> 1;

    /* set padding value*/
    pad[0] = pad_l;
    pad[1] = pad_c;
    pad[2] = pad_c;



    int cs = idc == 0 ? XEVD_CS_YCBCR400_10LE : (idc == 1 ? XEVD_CS_YCBCR420_10LE : (idc == 2 ? XEVD_CS_YCBCR422_10LE : XEVD_CS_YCBCR444_10LE));
    imgb = xevd_imgb_create(w, h, cs, opt, pad, align);


    xevd_assert_gv(imgb != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);

    /* set XEVD_PIC */
    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];
    pic->y     = imgb->a[0];
    pic->u     = imgb->a[1];
    pic->v     = imgb->a[2];

    pic->w_l   = imgb->w[0];
    pic->h_l   = imgb->h[0];
    pic->w_c   = imgb->w[1];
    pic->h_c   = imgb->h[1];

    pic->s_l   = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c   = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->pad_l = pad_l;
    pic->pad_c = pad_c;

    pic->imgb  = imgb;

    /* allocate maps */
    w_scu = (pic->w_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    h_scu = (pic->h_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    f_scu = w_scu * h_scu;

    size = sizeof(s8) * f_scu * REFP_NUM;
    pic->map_refi = xevd_malloc_fast(size);
    xevd_assert_gv(pic->map_refi, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mset_x64a(pic->map_refi, -1, size);

    size = sizeof(s16) * f_scu * REFP_NUM * MV_D;
    pic->map_mv = xevd_malloc_fast(size);
    xevd_assert_gv(pic->map_mv, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mset_x64a(pic->map_mv, 0, size);

    if(err)
    {
        *err = XEVD_OK;
    }
    return pic;

ERR:
    if(pic)
    {
        xevd_mfree(pic->map_mv);

        xevd_mfree(pic->map_refi);
        xevd_mfree(pic);
    }
    if(err) *err = ret;
    return NULL;
}

void xevd_picbuf_lc_free(XEVD_PIC *pic)
{
    XEVD_IMGB *imgb;

    if(pic)
    {
        imgb = pic->imgb;

        if(imgb)
        {
            imgb->release(imgb);

            pic->y = NULL;
            pic->u = NULL;
            pic->v = NULL;
            pic->w_l = 0;
            pic->h_l = 0;
            pic->w_c = 0;
            pic->h_c = 0;
            pic->s_l = 0;
            pic->s_c = 0;
        }
        xevd_mfree(pic->map_mv);

        xevd_mfree(pic->map_refi);
        xevd_mfree(pic);
    }
}

static void picbuf_expand(pel *a, int s, int w, int h, int exp)
{
    int i, j;
    pel pixel;
    pel *src, *dst;

    /* left */
    src = a;
    dst = a - exp;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* right */
    src = a + (w - 1);
    dst = a + w;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* upper */
    src = a - exp;
    dst = a - exp - (exp * s);

    for(i = 0; i < exp; i++)
    {
        xevd_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }

    /* below */
    src = a + ((h - 1)*s) - exp;
    dst = a + ((h - 1)*s) - exp + s;

    for(i = 0; i < exp; i++)
    {
        xevd_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }
}

void xevd_picbuf_lc_expand(XEVD_PIC *pic, int exp_l, int exp_c)
{
    picbuf_expand(pic->y, pic->s_l, pic->w_l, pic->h_l, exp_l);
    picbuf_expand(pic->u, pic->s_c, pic->w_c, pic->h_c, exp_c);
    picbuf_expand(pic->v, pic->s_c, pic->w_c, pic->h_c, exp_c);
}

void xevd_poc_derivation(XEVD_SPS * sps, int tid, XEVD_POC *poc)
{
    int sub_gop_length = (int)pow(2.0, sps->log2_sub_gop_length);
    int expected_tid = 0;
    int doc_offset, poc_offset;

    if (tid == 0)
    {
        poc->poc_val = poc->prev_poc_val + sub_gop_length;
        poc->prev_doc_offset = 0;
        poc->prev_poc_val = poc->poc_val;
        return;
    }
    doc_offset = (poc->prev_doc_offset + 1) % sub_gop_length;
    if (doc_offset == 0)
    {
        poc->prev_poc_val += sub_gop_length;
    }
    else
    {
        expected_tid = 1 + (int)log2(doc_offset);
    }
    while (tid != expected_tid)
    {
        doc_offset = (doc_offset + 1) % sub_gop_length;
        if (doc_offset == 0)
        {
            expected_tid = 0;
        }
        else
        {
            expected_tid = 1 + (int)log2(doc_offset);
        }
    }
    poc_offset = (int)(sub_gop_length * ((2.0 * doc_offset + 1) / (int)pow(2.0, tid) - 2));
    poc->poc_val = poc->prev_poc_val + poc_offset;
    poc->prev_doc_offset = doc_offset;
}


void xevd_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
                    XEVD_REFP(*refp)[REFP_NUM],
                    int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAX_NUM_MVP], s16 mvp[MAX_NUM_MVP][MV_D])
{

    if (IS_AVAIL(avail, AVAIL_LE))
    {
        refi[0] = 0;
        mvp[0][MV_X] = map_mv[scup - 1][lidx][MV_X];
        mvp[0][MV_Y] = map_mv[scup - 1][lidx][MV_Y];
    }
    else
    {
        refi[0] = 0;
        mvp[0][MV_X] = 1;
        mvp[0][MV_Y] = 1;
    }

    if (IS_AVAIL(avail, AVAIL_UP))
    {
        refi[1] = 0;
        mvp[1][MV_X] = map_mv[scup - w_scu][lidx][MV_X];
        mvp[1][MV_Y] = map_mv[scup - w_scu][lidx][MV_Y];
    }
    else
    {
        refi[1] = 0;
        mvp[1][MV_X] = 1;
        mvp[1][MV_Y] = 1;
    }

    if (IS_AVAIL(avail, AVAIL_UP_RI))
    {
        refi[2] = 0;
        mvp[2][MV_X] = map_mv[scup - w_scu + (cuw >> MIN_CU_LOG2)][lidx][MV_X];
        mvp[2][MV_Y] = map_mv[scup - w_scu + (cuw >> MIN_CU_LOG2)][lidx][MV_Y];
    }
    else
    {
        refi[2] = 0;
        mvp[2][MV_X] = 1;
        mvp[2][MV_Y] = 1;
    }
    refi[3] = 0;
    mvp[3][MV_X] = refp[0][lidx].map_mv[scup][0][MV_X];
    mvp[3][MV_Y] = refp[0][lidx].map_mv[scup][0][MV_Y];
}

void xevd_get_motion_skip_baseline(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u16 avail_lr)
{
    xevd_mset(mvp, 0, MAX_NUM_MVP * REFP_NUM * MV_D * sizeof(s16));
    xevd_mset(refi, REFI_INVALID, MAX_NUM_MVP * REFP_NUM * sizeof(s8));
    xevd_get_motion(scup, REFP_0, map_refi, map_mv, (XEVD_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_0], mvp[REFP_0]);
    if (slice_type == SLICE_B)
    {
        xevd_get_motion(scup, REFP_1, map_refi, map_mv, (XEVD_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_1], mvp[REFP_1]);
    }
}

BOOL xevd_check_bi_applicability(int slice_type, int cuw, int cuh)
{
    BOOL is_applicable = FALSE;

    if (slice_type == SLICE_B)
    {
        is_applicable = TRUE;
    }

    return is_applicable;
}

void xevd_get_mv_dir(XEVD_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D])
{
    s16 mvc[MV_D];
    int dpoc_co, dpoc_L0, dpoc_L1;

    mvc[MV_X] = refp[REFP_1].map_mv[scup][0][MV_X];
    mvc[MV_Y] = refp[REFP_1].map_mv[scup][0][MV_Y];

    dpoc_co = refp[REFP_1].poc - refp[REFP_1].list_poc[0];
    dpoc_L0 = poc - refp[REFP_0].poc;
    dpoc_L1 = refp[REFP_1].poc - poc;

    if(dpoc_co == 0)
    {
        mvp[REFP_0][MV_X] = 0;
        mvp[REFP_0][MV_Y] = 0;
        mvp[REFP_1][MV_X] = 0;
        mvp[REFP_1][MV_Y] = 0;
    }
    else
    {
        mvp[REFP_0][MV_X] = dpoc_L0 * mvc[MV_X] / dpoc_co;
        mvp[REFP_0][MV_Y] = dpoc_L0 * mvc[MV_Y] / dpoc_co;
        mvp[REFP_1][MV_X] = -dpoc_L1 * mvc[MV_X] / dpoc_co;
        mvp[REFP_1][MV_Y] = -dpoc_L1 * mvc[MV_Y] / dpoc_co;
    }
}

int xevd_get_avail_cu(int neb_scua[MAX_NEB2], u32 * map_cu, u8* map_tidx)
{
    int slice_num_x;
    u16 avail_cu = 0;

    xevd_assert(neb_scua[NEB_X] >= 0);

    slice_num_x = MCU_GET_SN(map_cu[neb_scua[NEB_X]]);

    /* left */
    if (neb_scua[NEB_A] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_A]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_A]]))
    {
        avail_cu |= AVAIL_LE;
    }
    /* up */
    if (neb_scua[NEB_B] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_B]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_B]]))
    {
        avail_cu |= AVAIL_UP;
    }
    /* up-right */
    if (neb_scua[NEB_C] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_C]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_C]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_C]]))
        {
            avail_cu |= AVAIL_UP_RI;
        }
    }
    /* up-left */
    if (neb_scua[NEB_D] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_D]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_D]]))
    {
        avail_cu |= AVAIL_UP_LE;
    }
    /* low-left */
    if (neb_scua[NEB_E] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_E]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_E]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_E]]))
        {
            avail_cu |= AVAIL_LO_LE;
        }
    }
    /* right */
    if (neb_scua[NEB_H] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_H]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_H]]))
    {
        avail_cu |= AVAIL_RI;
    }
    /* low-right */
    if (neb_scua[NEB_I] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_I]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_I]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_I]]))
        {
            avail_cu |= AVAIL_LO_RI;
        }
    }

    return avail_cu;
}

u16 xevd_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int curr_scup = x_scu + y_scu * w_scu;

    if (x_scu > 0 && !MCU_GET_IF(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) &&
        (map_tidx[curr_scup] == map_tidx[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) - 1]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if (y_scu > 0)
    {
        if (!MCU_GET_IF(map_scu[scup - w_scu]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if (!MCU_GET_IF(map_scu[scup - w_scu + scuw - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw - 1]))
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if (x_scu > 0 && !MCU_GET_IF(map_scu[scup - w_scu - 1]) && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }
        // xxu check??
        if (x_scu + scuw < w_scu  && MCU_IS_COD_NIF(map_scu[scup - w_scu + scuw]) && MCU_GET_COD(map_scu[scup - w_scu + scuw]) &&
            (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if (x_scu + scuw < w_scu && !MCU_GET_IF(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) + scuw]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) + scuw]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

u16 xevd_get_avail_intra(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int log2_cuw, int log2_cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int log2_scuw, log2_scuh, scuw, scuh;

    log2_scuw = log2_cuw - MIN_CU_LOG2;
    log2_scuh = log2_cuh - MIN_CU_LOG2;
    scuw = 1 << log2_scuw;
    scuh = 1 << log2_scuh;
    int curr_scup = x_scu + y_scu * w_scu;

    if (x_scu > 0 && MCU_GET_COD(map_scu[scup - 1]) && map_tidx[curr_scup] == map_tidx[scup - 1])
    {
        SET_AVAIL(avail, AVAIL_LE);

        if (y_scu + scuh + scuw - 1 < h_scu  && MCU_GET_COD(map_scu[scup + (w_scu * (scuw + scuh)) - w_scu - 1]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (w_scu * (scuw + scuh)) - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if (y_scu > 0)
    {
        if (map_tidx[scup] == map_tidx[scup - w_scu])
        {
            SET_AVAIL(avail, AVAIL_UP);
        }
        if (map_tidx[scup] == map_tidx[scup - w_scu + scuw - 1])
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if (x_scu > 0 && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if (x_scu + scuw < w_scu  && MCU_GET_COD(map_scu[scup - w_scu + scuw]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if (y_scu + scuh + scuw - 1 < h_scu  && MCU_GET_COD(map_scu[scup + (w_scu * (scuw + scuh - 1)) + scuw]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (w_scu * (scuw + scuh - 1)) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

int xevd_picbuf_signature(XEVD_PIC *pic, u8 signature[N_C][16])
{
    return xevd_md5_imgb(pic->imgb, signature);
}

/* MD5 functions */
#define MD5FUNC(f, w, x, y, z, msg1, s,msg2 )  ( w += f(x, y, z) + msg1 + msg2,  w = w<<s | w>>(32-s),  w += x )
#define FF(x, y, z) (z ^ (x & (y ^ z)))
#define GG(x, y, z) (y ^ (z & (x ^ y)))
#define HH(x, y, z) (x ^ y ^ z)
#define II(x, y, z) (y ^ (x | ~z))

static void xevd_md5_trans(u32 *buf, u32 *msg)
{
    register u32 a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5FUNC(FF, a, b, c, d, msg[ 0],  7, 0xd76aa478); /* 1 */
    MD5FUNC(FF, d, a, b, c, msg[ 1], 12, 0xe8c7b756); /* 2 */
    MD5FUNC(FF, c, d, a, b, msg[ 2], 17, 0x242070db); /* 3 */
    MD5FUNC(FF, b, c, d, a, msg[ 3], 22, 0xc1bdceee); /* 4 */

    MD5FUNC(FF, a, b, c, d, msg[ 4],  7, 0xf57c0faf); /* 5 */
    MD5FUNC(FF, d, a, b, c, msg[ 5], 12, 0x4787c62a); /* 6 */
    MD5FUNC(FF, c, d, a, b, msg[ 6], 17, 0xa8304613); /* 7 */
    MD5FUNC(FF, b, c, d, a, msg[ 7], 22, 0xfd469501); /* 8 */

    MD5FUNC(FF, a, b, c, d, msg[ 8],  7, 0x698098d8); /* 9 */
    MD5FUNC(FF, d, a, b, c, msg[ 9], 12, 0x8b44f7af); /* 10 */
    MD5FUNC(FF, c, d, a, b, msg[10], 17, 0xffff5bb1); /* 11 */
    MD5FUNC(FF, b, c, d, a, msg[11], 22, 0x895cd7be); /* 12 */

    MD5FUNC(FF, a, b, c, d, msg[12],  7, 0x6b901122); /* 13 */
    MD5FUNC(FF, d, a, b, c, msg[13], 12, 0xfd987193); /* 14 */
    MD5FUNC(FF, c, d, a, b, msg[14], 17, 0xa679438e); /* 15 */
    MD5FUNC(FF, b, c, d, a, msg[15], 22, 0x49b40821); /* 16 */

    /* Round 2 */
    MD5FUNC(GG, a, b, c, d, msg[ 1],  5, 0xf61e2562); /* 17 */
    MD5FUNC(GG, d, a, b, c, msg[ 6],  9, 0xc040b340); /* 18 */
    MD5FUNC(GG, c, d, a, b, msg[11], 14, 0x265e5a51); /* 19 */
    MD5FUNC(GG, b, c, d, a, msg[ 0], 20, 0xe9b6c7aa); /* 20 */

    MD5FUNC(GG, a, b, c, d, msg[ 5],  5, 0xd62f105d); /* 21 */
    MD5FUNC(GG, d, a, b, c, msg[10],  9,  0x2441453); /* 22 */
    MD5FUNC(GG, c, d, a, b, msg[15], 14, 0xd8a1e681); /* 23 */
    MD5FUNC(GG, b, c, d, a, msg[ 4], 20, 0xe7d3fbc8); /* 24 */

    MD5FUNC(GG, a, b, c, d, msg[ 9],  5, 0x21e1cde6); /* 25 */
    MD5FUNC(GG, d, a, b, c, msg[14],  9, 0xc33707d6); /* 26 */
    MD5FUNC(GG, c, d, a, b, msg[ 3], 14, 0xf4d50d87); /* 27 */
    MD5FUNC(GG, b, c, d, a, msg[ 8], 20, 0x455a14ed); /* 28 */

    MD5FUNC(GG, a, b, c, d, msg[13],  5, 0xa9e3e905); /* 29 */
    MD5FUNC(GG, d, a, b, c, msg[ 2],  9, 0xfcefa3f8); /* 30 */
    MD5FUNC(GG, c, d, a, b, msg[ 7], 14, 0x676f02d9); /* 31 */
    MD5FUNC(GG, b, c, d, a, msg[12], 20, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    MD5FUNC(HH, a, b, c, d, msg[ 5],  4, 0xfffa3942); /* 33 */
    MD5FUNC(HH, d, a, b, c, msg[ 8], 11, 0x8771f681); /* 34 */
    MD5FUNC(HH, c, d, a, b, msg[11], 16, 0x6d9d6122); /* 35 */
    MD5FUNC(HH, b, c, d, a, msg[14], 23, 0xfde5380c); /* 36 */

    MD5FUNC(HH, a, b, c, d, msg[ 1],  4, 0xa4beea44); /* 37 */
    MD5FUNC(HH, d, a, b, c, msg[ 4], 11, 0x4bdecfa9); /* 38 */
    MD5FUNC(HH, c, d, a, b, msg[ 7], 16, 0xf6bb4b60); /* 39 */
    MD5FUNC(HH, b, c, d, a, msg[10], 23, 0xbebfbc70); /* 40 */

    MD5FUNC(HH, a, b, c, d, msg[13],  4, 0x289b7ec6); /* 41 */
    MD5FUNC(HH, d, a, b, c, msg[ 0], 11, 0xeaa127fa); /* 42 */
    MD5FUNC(HH, c, d, a, b, msg[ 3], 16, 0xd4ef3085); /* 43 */
    MD5FUNC(HH, b, c, d, a, msg[ 6], 23,  0x4881d05); /* 44 */

    MD5FUNC(HH, a, b, c, d, msg[ 9],  4, 0xd9d4d039); /* 45 */
    MD5FUNC(HH, d, a, b, c, msg[12], 11, 0xe6db99e5); /* 46 */
    MD5FUNC(HH, c, d, a, b, msg[15], 16, 0x1fa27cf8); /* 47 */
    MD5FUNC(HH, b, c, d, a, msg[ 2], 23, 0xc4ac5665); /* 48 */

    /* Round 4 */
    MD5FUNC(II, a, b, c, d, msg[ 0],  6, 0xf4292244); /* 49 */
    MD5FUNC(II, d, a, b, c, msg[ 7], 10, 0x432aff97); /* 50 */
    MD5FUNC(II, c, d, a, b, msg[14], 15, 0xab9423a7); /* 51 */
    MD5FUNC(II, b, c, d, a, msg[ 5], 21, 0xfc93a039); /* 52 */

    MD5FUNC(II, a, b, c, d, msg[12],  6, 0x655b59c3); /* 53 */
    MD5FUNC(II, d, a, b, c, msg[ 3], 10, 0x8f0ccc92); /* 54 */
    MD5FUNC(II, c, d, a, b, msg[10], 15, 0xffeff47d); /* 55 */
    MD5FUNC(II, b, c, d, a, msg[ 1], 21, 0x85845dd1); /* 56 */

    MD5FUNC(II, a, b, c, d, msg[ 8],  6, 0x6fa87e4f); /* 57 */
    MD5FUNC(II, d, a, b, c, msg[15], 10, 0xfe2ce6e0); /* 58 */
    MD5FUNC(II, c, d, a, b, msg[ 6], 15, 0xa3014314); /* 59 */
    MD5FUNC(II, b, c, d, a, msg[13], 21, 0x4e0811a1); /* 60 */

    MD5FUNC(II, a, b, c, d, msg[ 4],  6, 0xf7537e82); /* 61 */
    MD5FUNC(II, d, a, b, c, msg[11], 10, 0xbd3af235); /* 62 */
    MD5FUNC(II, c, d, a, b, msg[ 2], 15, 0x2ad7d2bb); /* 63 */
    MD5FUNC(II, b, c, d, a, msg[ 9], 21, 0xeb86d391); /* 64 */

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void xevd_md5_init(XEVD_MD5 *md5)
{
    md5->h[0] = 0x67452301;
    md5->h[1] = 0xefcdab89;
    md5->h[2] = 0x98badcfe;
    md5->h[3] = 0x10325476;

    md5->bits[0] = 0;
    md5->bits[1] = 0;
}

void xevd_md5_update(XEVD_MD5 *md5, void *buf_t, u32 len)
{
    u8 *buf;
    u32 i, idx, part_len;

    buf = (u8*)buf_t;

    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        xevd_mcpy(md5->msg + idx, buf, part_len);
        xevd_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            xevd_md5_trans(md5->h, (u32 *)(buf + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        xevd_mcpy(md5->msg + idx, buf + i, len - i);
    }
}

void xevd_md5_update_16(XEVD_MD5 *md5, void *buf_t, u32 len)
{
    u16 *buf;
    u32 i, idx, part_len, j;
    u8 t[512];

    buf = (u16 *)buf_t;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    len = len * 2;
    for(j = 0; j < len; j += 2)
    {
        t[j] = (u8)(*(buf));
        t[j + 1] = *(buf) >> 8;
        buf++;
    }

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        xevd_mcpy(md5->msg + idx, t, part_len);
        xevd_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            xevd_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        xevd_mcpy(md5->msg + idx, t + i, len - i);
    }
}

void xevd_md5_finish(XEVD_MD5 *md5, u8 digest[16])
{
    u8 *pos;
    int cnt;

    cnt = (md5->bits[0] >> 3) & 0x3F;
    pos = md5->msg + cnt;
    *pos++ = 0x80;
    cnt = 64 - 1 - cnt;

    if(cnt < 8)
    {
        xevd_mset(pos, 0, cnt);
        xevd_md5_trans(md5->h, (u32 *)md5->msg);
        xevd_mset(md5->msg, 0, 56);
    }
    else
    {
        xevd_mset(pos, 0, cnt - 8);
    }

    xevd_mcpy((md5->msg + 14 * sizeof(u32)), &md5->bits[0], sizeof(u32));
    xevd_mcpy((md5->msg + 15 * sizeof(u32)), &md5->bits[1], sizeof(u32));

    xevd_md5_trans(md5->h, (u32 *)md5->msg);
    xevd_mcpy(digest, md5->h, 16);
    xevd_mset(md5, 0, sizeof(XEVD_MD5));
}

int xevd_md5_imgb(XEVD_IMGB *imgb, u8 digest[N_C][16])
{
    XEVD_MD5 md5[N_C];
    int i, j;
    for(i = 0; i < imgb->np; i++)
    {
        xevd_md5_init(&md5[i]);

        for(j = imgb->y[i]; j < imgb->h[i]; j++)
        {
            xevd_md5_update(&md5[i], ((u8 *)imgb->a[i]) + j*imgb->s[i] + imgb->x[i] , imgb->w[i] * 2);
        }

        xevd_md5_finish(&md5[i], digest[i]);
    }

    return XEVD_OK;
}

void init_scan(u16 *scan, int size_x, int size_y, int scan_type)
{
    int x, y, l, pos, num_line;

    pos = 0;
    num_line = size_x + size_y - 1;

    if(scan_type == COEF_SCAN_ZIGZAG)
    {
        /* starting point */
        scan[pos] = 0;
        pos++;

        /* loop */
        for(l = 1; l < num_line; l++)
        {
            if(l % 2) /* decreasing loop */
            {
                x = XEVD_MIN(l, size_x - 1);
                y = XEVD_MAX(0, l - (size_x - 1));

                while(x >= 0 && y < size_y)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x--;
                    y++;
                }
            }
            else /* increasing loop */
            {
                y = XEVD_MIN(l, size_y - 1);
                x = XEVD_MAX(0, l - (size_y - 1));
                while(y >= 0 && x < size_x)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x++;
                    y--;
                }
            }
        }
    }
}

int xevd_scan_tbl_init(XEVD_CTX * ctx)
{
    int x, y, scan_type;
    int size_y, size_x;

    ctx->scan_tables = (XEVD_SCAN_TABLES*) malloc(sizeof(XEVD_SCAN_TABLES));
    memset(ctx->scan_tables->xevd_scan_tbl, 0, COEF_SCAN_ZIGZAG * MAX_CU_LOG2 * MAX_CU_LOG2 * sizeof(u16*));
    memset(ctx->scan_tables->xevd_inv_scan_tbl, 0, COEF_SCAN_ZIGZAG * MAX_CU_LOG2 * MAX_CU_LOG2 * sizeof(u16*));

    for(scan_type = 0; scan_type < COEF_SCAN_TYPE_NUM; scan_type++)
    {
        if (scan_type != COEF_SCAN_ZIGZAG)
            continue;
        for(y = 0; y < MAX_CU_LOG2 - 1; y++)
        {
            size_y = 1 << (y + 1);
            for(x = 0; x < MAX_CU_LOG2 - 1; x++)
            {
                size_x = 1 << (x + 1);

                ctx->scan_tables->xevd_scan_tbl[scan_type][x][y] = (u16*)xevd_malloc_fast(size_y * size_x * sizeof(u16));
                init_scan(ctx->scan_tables->xevd_scan_tbl[scan_type][x][y], size_x, size_y, scan_type);
                ctx->scan_tables->xevd_inv_scan_tbl[scan_type][x][y] = (u16*)xevd_malloc_fast(size_y * size_x * sizeof(u16));
                xevd_init_inverse_scan_sr(ctx->scan_tables->xevd_inv_scan_tbl[scan_type][x][y], ctx->scan_tables->xevd_scan_tbl[scan_type][x][y], size_x, size_y, scan_type);
            }
        }
    }
    return XEVD_OK;
}

int xevd_scan_tbl_delete(XEVD_CTX * ctx)
{
    if(!ctx) return XEVD_OK;

    int x, y, scan_type;

    for(scan_type = 0; scan_type < COEF_SCAN_TYPE_NUM; scan_type++)
    {
        if (scan_type != COEF_SCAN_ZIGZAG)
            continue;
        for(y = 0; y < MAX_CU_LOG2 - 1; y++)
        {
            for(x = 0; x < MAX_CU_LOG2 - 1; x++)
            {
                if(ctx->scan_tables->xevd_scan_tbl[scan_type][x][y] != NULL)
                {
                    free(ctx->scan_tables->xevd_scan_tbl[scan_type][x][y]);
                    ctx->scan_tables->xevd_scan_tbl[scan_type][x][y] = NULL;
                }

                if (ctx->scan_tables->xevd_inv_scan_tbl[scan_type][x][y] != NULL)
                {
                    free(ctx->scan_tables->xevd_inv_scan_tbl[scan_type][x][y]);
                    ctx->scan_tables->xevd_inv_scan_tbl[scan_type][x][y] = NULL;
                }
            }
        }
    }
    free(ctx->scan_tables);
    ctx->scan_tables = NULL;

    return XEVD_OK;
}

int xevd_get_split_mode(s8 *split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (* split_mode_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int ret = XEVD_OK;
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVD_CONV_LOG2(cuw) - XEVD_CONV_LOG2(cuh));

    if(cuw < 8 && cuh < 8)
    {
        *split_mode = NO_SPLIT;
        return ret;
    }

    *split_mode = (*split_mode_buf)[cud][shape][pos];

    return ret;
}

void xevd_set_split_mode(s8 split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (*split_mode_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVD_CONV_LOG2(cuw) - XEVD_CONV_LOG2(cuh));

    if(cuw >= 8 || cuh >= 8)
        (*split_mode_buf)[cud][shape][pos] = split_mode;
}

u16 xevd_check_eco_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u8 * cod_eco, u8* map_tidx)
{
    int scup = y_scu *  w_scu + x_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    u16 avail_lr = 0;
    int curr_scup = x_scu + y_scu * w_scu;
    if (x_scu > 0 && cod_eco[scup - 1] && (map_tidx[curr_scup] == map_tidx[scup - 1]))
    {
        avail_lr += 1;
    }
    if (x_scu + scuw < w_scu && cod_eco[scup + scuw] && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        avail_lr += 2;
    }
    return avail_lr;
}

u16 xevd_check_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u32 * map_scu, u8* map_tidx)
{
    int scup = y_scu *  w_scu + x_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    u16 avail_lr = 0;
    int curr_scup = x_scu + y_scu * w_scu;

    if(x_scu > 0 && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[curr_scup] == map_tidx[scup - 1]))
    {
        avail_lr+=1;
    }

    if(x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup+scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        avail_lr+=2;
    }

    return avail_lr;
}

/*******************************************/
/* Neighbor location: Graphical indication */
/*                                         */
/*          B2 UP___________B1 B0          */
/*          LE|               |            */
/*            |               |            */
/*            |               |            */
/*            |      cu    cuh|            */
/*            |               |            */
/*            |               |            */
/*          A1|_____cuw_______|            */
/*          A0                             */
/*                                         */
/*******************************************/

#define SAME_MV(MV0, MV1) ((MV0[MV_X] == MV1[MV_X]) && (MV0[MV_Y] == MV1[MV_Y]))
#define SAME_MVF(refi0, vx0, vy0, refi1, vx1, vy1)   ((refi0 == refi1) && (vx0 == vx1) && (vy0 == vy1))

void xevd_get_ctx_last_pos_xy_para(int ch_type, int width, int height, int *result_offset_x, int *result_offset_y, int *result_shift_x, int *result_shift_y)
{
    int convertedWidth = XEVD_CONV_LOG2(width) - 2;
    int convertedHeight = XEVD_CONV_LOG2(height) - 2;
    convertedWidth = (convertedWidth < 0) ? 0 : convertedWidth;
    convertedHeight = (convertedHeight < 0) ? 0 : convertedHeight;

    *result_offset_x = (ch_type != Y_C) ? 0 : ((convertedWidth * 3) + ((convertedWidth + 1) >> 2));
    *result_offset_y = (ch_type != Y_C) ? 0 : ((convertedHeight * 3) + ((convertedHeight + 1) >> 2));
    *result_shift_x = (ch_type != Y_C) ? convertedWidth - XEVD_CONV_LOG2(width >> 4) : ((convertedWidth + 3) >> 2);
    *result_shift_y = (ch_type != Y_C) ? convertedHeight - XEVD_CONV_LOG2(height >> 4) : ((convertedHeight + 3) >> 2);

    if (ch_type == Y_C)
    {
        if (convertedWidth >= 4)
        {
            *result_offset_x += ((width >> 6) << 1) + (width >> 7);
            *result_shift_x = 2;
        }
        if (convertedHeight >= 4)
        {
            *result_offset_y += ((height >> 6) << 1) + (height >> 7);
            *result_shift_y = 2;
        }
    }
}

void xevd_init_inverse_scan_sr(u16 *scan_inv, u16 *scan_orig, int width, int height, int scan_type)
{
    int x, num_line;

    num_line = width*height;
    if ( (scan_type == COEF_SCAN_ZIGZAG) || (scan_type == COEF_SCAN_DIAG) || (scan_type == COEF_SCAN_DIAG_CG) )
    {
        for ( x = 0; x < num_line; x++)
        {
            int scan_pos = scan_orig[x];
            assert(scan_pos >= 0);
            assert(scan_pos < num_line);
            scan_inv[scan_pos] = x;
        }
    }
    else
    {
        xevd_trace("Not supported scan_type\n");
    }
}


void xevd_eco_sbac_ctx_initialize(SBAC_CTX_MODEL *model, s16 *ctx_init_model, u16 num_ctx, u8 slice_type, u8 slice_qp)
{
    s32 i, slope, offset;
    u16 mps, state;
    const int qp = XEVD_CLIP3(0, 51, slice_qp);
    
    ctx_init_model += ((slice_type == SLICE_B) * num_ctx);

    for(i = 0; i < num_ctx; i++)
    {
        const int init_value = *(ctx_init_model);
        slope = (init_value & 14) << 4;
        slope = (init_value & 1) ? -slope : slope;
        offset = ((init_value >> 4) & 62) << 7;
        offset = ((init_value >> 4) & 1) ? -offset : offset;
        offset += 4096;

        state = XEVD_CLIP3(1, 511, (slope * qp + offset) >> 4);
        if(state > 256)
        {
            state = 512 - state;
            mps = 0;
        }
        else
        {
            mps = 1;
        }
        model[i] = (state << 1) + mps;

        ctx_init_model++;
    }
}

int xevd_split_part_count(int split_mode)
{
    switch (split_mode)
    {
    case SPLIT_BI_VER:
    case SPLIT_BI_HOR:
        return 2;
    case SPLIT_TRI_VER:
    case SPLIT_TRI_HOR:
        return 3;
    case SPLIT_QUAD:
        return 4;
    default:
        // NO_SPLIT
        return 0;
    }
}

int xevd_split_get_part_size(int split_mode, int part_num, int length)
{
    int ans = length;
    switch (split_mode)
    {
    case SPLIT_QUAD:
    case SPLIT_BI_HOR:
    case SPLIT_BI_VER:
        ans = length >> 1;
        break;
    case SPLIT_TRI_HOR:
    case SPLIT_TRI_VER:
        if (part_num == 1)
            ans = length >> 1;
        else
            ans = length >> 2;
        break;
    }
    return ans;
}

int xevd_split_get_part_size_idx(int split_mode, int part_num, int length_idx)
{
    int ans = length_idx;
    switch (split_mode)
    {
    case SPLIT_QUAD:
    case SPLIT_BI_HOR:
    case SPLIT_BI_VER:
        ans = length_idx - 1;
        break;
    case SPLIT_TRI_HOR:
    case SPLIT_TRI_VER:
        if (part_num == 1)
            ans = length_idx - 1;
        else
            ans = length_idx - 2;
        break;
    }
    return ans;
}
SPLIT_DIR xevd_split_get_direction(SPLIT_MODE mode)
{
    switch (mode)
    {
    case SPLIT_BI_HOR:
    case SPLIT_TRI_HOR:
        return SPLIT_HOR;
    default:
        return SPLIT_VER;
    }
}

int xevd_split_is_vertical(SPLIT_MODE mode)
{
    return xevd_split_get_direction(mode) == SPLIT_VER ? 1 : 0;
}

int xevd_split_is_horizontal(SPLIT_MODE mode)
{
    return xevd_split_get_direction(mode) == SPLIT_HOR ? 1 : 0;
}

void xevd_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVD_SPLIT_STRUCT* split_struct)
{
    int i;
    int log_cuw, log_cuh;
    int cup_w, cup_h;

    split_struct->part_count = xevd_split_part_count(split_mode);
    log_cuw = XEVD_CONV_LOG2(cuw);
    log_cuh = XEVD_CONV_LOG2(cuh);
    split_struct->x_pos[0] = x0;
    split_struct->y_pos[0] = y0;
    split_struct->cup[0] = cup;

    switch (split_mode)
    {
    case NO_SPLIT:
    {
        split_struct->width[0] = cuw;
        split_struct->height[0] = cuh;
        split_struct->log_cuw[0] = log_cuw;
        split_struct->log_cuh[0] = log_cuh;
    }
    break;

    case SPLIT_QUAD:
    {
        split_struct->width[0] = cuw >> 1;
        split_struct->height[0] = cuh >> 1;
        split_struct->log_cuw[0] = log_cuw - 1;
        split_struct->log_cuh[0] = log_cuh - 1;
        for (i = 1; i < split_struct->part_count; ++i)
        {
            split_struct->width[i] = split_struct->width[0];
            split_struct->height[i] = split_struct->height[0];
            split_struct->log_cuw[i] = split_struct->log_cuw[0];
            split_struct->log_cuh[i] = split_struct->log_cuh[0];
        }
        split_struct->x_pos[1] = x0 + split_struct->width[0];
        split_struct->y_pos[1] = y0;
        split_struct->x_pos[2] = x0;
        split_struct->y_pos[2] = y0 + split_struct->height[0];
        split_struct->x_pos[3] = split_struct->x_pos[1];
        split_struct->y_pos[3] = split_struct->y_pos[2];
        cup_w = (split_struct->width[0] >> MIN_CU_LOG2);
        cup_h = ((split_struct->height[0] >> MIN_CU_LOG2) << log2_culine);
        split_struct->cup[1] = cup + cup_w;
        split_struct->cup[2] = cup + cup_h;
        split_struct->cup[3] = split_struct->cup[1] + cup_h;
        split_struct->cud[0] = cud + 2;
        split_struct->cud[1] = cud + 2;
        split_struct->cud[2] = cud + 2;
        split_struct->cud[3] = cud + 2;
    }
    break;

    default:
    {
        if (xevd_split_is_vertical(split_mode))
        {
            for (i = 0; i < split_struct->part_count; ++i)
            {
                split_struct->width[i] = xevd_split_get_part_size(split_mode, i, cuw);
                split_struct->log_cuw[i] = xevd_split_get_part_size_idx(split_mode, i, log_cuw);
                split_struct->height[i] = cuh;
                split_struct->log_cuh[i] = log_cuh;
                if (i)
                {
                    split_struct->x_pos[i] = split_struct->x_pos[i - 1] + split_struct->width[i - 1];
                    split_struct->y_pos[i] = split_struct->y_pos[i - 1];
                    split_struct->cup[i] = split_struct->cup[i - 1] + (split_struct->width[i - 1] >> MIN_CU_LOG2);
                }
            }
        }
        else
        {
            for (i = 0; i < split_struct->part_count; ++i)
            {
                split_struct->width[i] = cuw;
                split_struct->log_cuw[i] = log_cuw;
                split_struct->height[i] = xevd_split_get_part_size(split_mode, i, cuh);
                split_struct->log_cuh[i] = xevd_split_get_part_size_idx(split_mode, i, log_cuh);
                if (i)
                {
                    split_struct->y_pos[i] = split_struct->y_pos[i - 1] + split_struct->height[i - 1];
                    split_struct->x_pos[i] = split_struct->x_pos[i - 1];
                    split_struct->cup[i] = split_struct->cup[i - 1] + ((split_struct->height[i - 1] >> MIN_CU_LOG2) << log2_culine);
                }
            }
        }
        switch (split_mode)
        {
        case SPLIT_BI_VER:
            split_struct->cud[0] = cud + 1;
            split_struct->cud[1] = cud + 1;
            break;
        case SPLIT_BI_HOR:
            split_struct->cud[0] = cud + 1;
            split_struct->cud[1] = cud + 1;
            break;
        default:
            // Triple tree case
            split_struct->cud[0] = cud + 2;
            split_struct->cud[1] = cud + 1;
            split_struct->cud[2] = cud + 2;
            break;
        }
    }
    break;
    }
}
void xevd_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h)
{
    int h;
    int copy_size = (1 << log2_copy_w) * (int)sizeof(s16);
    s16 *tmp_src = src;
    s16 *tmp_dst = dst;
    for (h = 0; h < (1<< log2_copy_h); h++)
    {
        xevd_mcpy(tmp_dst, tmp_src, copy_size);
        tmp_dst += dst_stride;
        tmp_src += src_stride;
    }
}

int xevd_get_luma_cup(int x_scu, int y_scu, int cu_w_scu, int cu_h_scu, int w_scu)
{
    return (y_scu + (cu_h_scu >> 1)) * w_scu + x_scu + (cu_w_scu >> 1);
}


void xevd_picbuf_expand(XEVD_CTX * ctx, XEVD_PIC * pic)
{
    xevd_picbuf_lc_expand(pic, pic->pad_l, pic->pad_c);
}

XEVD_PIC * xevd_picbuf_alloc(PICBUF_ALLOCATOR * pa, int * ret, int bit_depth)
{
    return xevd_picbuf_lc_alloc(pa->w, pa->h, pa->pad_l, pa->pad_c, ret, pa->idc, bit_depth);
}

void xevd_picbuf_free(PICBUF_ALLOCATOR * pa, XEVD_PIC * pic)
{
    xevd_picbuf_lc_free(pic);
}

int xevd_picbuf_check_signature(XEVD_PIC * pic, u8 signature[N_C][16]
,int bit_depth)
{
    u8 pic_sign[N_C][16] = { {0} };
    int ret;

    /* execute MD5 digest here */
    ret = xevd_picbuf_signature(pic, pic_sign);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    if (memcmp(signature, pic_sign, N_C * 16) != 0)
    {
        return XEVD_ERR_BAD_CRC;
    }

    return XEVD_OK;
}

void xevd_set_dec_info(XEVD_CTX * ctx, XEVD_CORE * core)
{
    s8 (*map_refi)[REFP_NUM];
    s16 (*map_mv)[REFP_NUM][MV_D];

    u32  *map_scu;
    s8   *map_ipm;
    int   w_cu;
    int   h_cu;
    int   scup;
    int   w_scu;
    int   i, j;
    int   flag;

    u32  *map_cu_mode;

    scup = core->scup;
    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;
    w_scu = ctx->w_scu;
    map_refi = ctx->map_refi + scup;
    map_scu = ctx->map_scu + scup;
    map_mv = ctx->map_mv + scup;

    map_ipm = ctx->map_ipm + scup;

    flag = (core->pred_mode == MODE_INTRA) ? 1 : 0;
    map_cu_mode = ctx->map_cu_mode + scup;


    u32 temp1, temp2;

    if (core->pred_mode == MODE_SKIP)
    {
        MCU_SET_SF(map_scu[0]);
    }
    else
    {
        MCU_CLR_SF(map_scu[0]);
    }

    if (core->is_coef_sub[Y_C][0])
    {
        MCU_SET_CBFL(map_scu[0]);
    }
    else
    {
        MCU_CLR_CBFL(map_scu[0]);
    }

    MCU_SET_LOGW(map_cu_mode[0], core->log2_cuw);
    MCU_SET_LOGH(map_cu_mode[0], core->log2_cuh);
    MCU_SET_IF_SN_QP(map_scu[0], flag, ctx->slice_num, core->qp);
    map_refi[0][REFP_0] = core->refi[REFP_0];
    map_refi[0][REFP_1] = core->refi[REFP_1];

    map_mv[0][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
    map_mv[0][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
    map_mv[0][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
    map_mv[0][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];

    for (i = 1; i < w_cu; i = i << 1)
    {
        xevd_mcpy(map_scu + i, map_scu, i * sizeof(map_scu[0]));
        xevd_mcpy(map_cu_mode + i, map_cu_mode, i * sizeof(map_cu_mode[0]));
        xevd_mcpy(map_mv + i, map_mv, 4 * i * sizeof(s16));
        xevd_mcpy(map_refi + i, map_refi, 2 * i * sizeof(s8));

    }

     xevd_mset(map_ipm, core->ipm[0], w_cu*sizeof(core->ipm[0]));

    for (i = 1; i < h_cu; i++)
    {
        xevd_mcpy(map_mv + i*w_scu, map_mv, 4*w_cu*sizeof(s16));
        xevd_mcpy(map_refi + i*w_scu, map_refi, 2*w_cu*sizeof(s8));
        xevd_mcpy(map_scu + i*w_scu, map_scu, w_cu*sizeof(map_scu[0]));
        xevd_mcpy(map_cu_mode + i*w_scu, map_cu_mode, w_cu*sizeof(map_cu_mode[0]));
        xevd_mcpy(map_ipm + i*w_scu, map_ipm, w_cu*sizeof(core->ipm[0]));
    }

#if MVF_TRACE
    // Trace MVF in decoder
    {
        map_refi = ctx->map_refi + scup;
        map_scu = ctx->map_scu + scup;
        map_mv = ctx->map_mv + scup;

        for (i = 0; i < h_cu; i++)
        {
            for (j = 0; j < w_cu; j++)
            {
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR(" x: ");
                XEVD_TRACE_INT(j);
                XEVD_TRACE_STR(" y: ");
                XEVD_TRACE_INT(i);

                XEVD_TRACE_STR(" ref0: ");
                XEVD_TRACE_INT(map_refi[j][REFP_0]);
                XEVD_TRACE_STR(" mv: ");
                XEVD_TRACE_MV(map_mv[j][REFP_0][MV_X], map_mv[j][REFP_0][MV_Y]);

                XEVD_TRACE_STR(" ref1: ");
                XEVD_TRACE_INT(map_refi[j][REFP_1]);
                XEVD_TRACE_STR(" mv: ");
                XEVD_TRACE_MV(map_mv[j][REFP_1][MV_X], map_mv[j][REFP_1][MV_Y]);

                XEVD_TRACE_STR("\n");
            }
            map_refi += w_scu;
            map_mv += w_scu;
            map_scu += w_scu;
        }
    }
#endif
}


int xevd_info(void * bits, int bits_size, int is_annexb, XEVD_INFO * info)
{
    int i, t = 0;
    unsigned char * p = (unsigned char *)bits;

    // default negative value in case of missing of syntax
    info->nalu_len = -1;
    info->nalu_type = -1;
    info->nalu_tid = -1;

    // Annex-B parsing
    if(is_annexb && (bits_size >= 4))
    {
        for(i=0; i<4; i++) {
            t = (t << 8) | p[i];
        }
        info->nalu_len = t;

        bits_size -= 4;
        p += 4;
    }

    // nal header parsing
    if(bits_size >= 2) {
        // forbidden_zero_bit
        if ((p[0] & 0x80) != 0) return XEVD_ERR_MALFORMED_BITSTREAM;
        // nal_unit_type
        info->nalu_type = (p[0] >> 1) & 0x3F;
        // nuh_tempral_id
        info->nalu_tid = ((p[0] & 0x01) << 2) | ((p[1] >> 6) & 0x03);

        bits_size -= 2;
        p += 2;
    }
    return XEVD_OK;
}


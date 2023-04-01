/* The copyright in this software is being made available under the BSD
   License, included below. This software may be subject to contributor and
   other third party rights, including patent rights, and no such rights are
   granted under this license.

   Copyright (c) 2020, Samsung Electronics Co., Ltd.
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


#include "xevdm_def.h"
#include "xevd_util.h"
#include <math.h>

#define TX_SHIFT1(log2_size, bd)   ((log2_size) - 1 + bd - 8)
#define TX_SHIFT2(log2_size)   ((log2_size) + 6)

XEVD_PIC * xevdm_picbuf_alloc_exp(int w, int h, int pad_l, int pad_c, int *err, int idc, int bitdepth)
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
    pic->y = imgb->a[0];
    pic->u = imgb->a[1];
    pic->v = imgb->a[2];

    pic->w_l = imgb->w[0];
    pic->h_l = imgb->h[0];
    pic->w_c = imgb->w[1];
    pic->h_c = imgb->h[1];

    pic->s_l = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->pad_l = pad_l;
    pic->pad_c = pad_c;

    pic->imgb = imgb;

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

    size = sizeof(s16) * f_scu * REFP_NUM * MV_D;
    pic->map_unrefined_mv = xevd_malloc_fast(size);
    xevd_assert_gv(pic->map_unrefined_mv, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mset_x64a(pic->map_unrefined_mv, 0, size);


    if (err)
    {
        *err = XEVD_OK;
    }
    return pic;

ERR:
    if (pic)
    {
        xevd_mfree(pic->map_mv);
        xevd_mfree(pic->map_unrefined_mv);
        xevd_mfree(pic->map_refi);
        xevd_mfree(pic);
    }
    if (err) *err = ret;
    return NULL;
}

void xevdm_picbuf_free(PICBUF_ALLOCATOR* pa, XEVD_PIC *pic)
{
    XEVD_IMGB *imgb;
    if (pic)
    {
        imgb = pic->imgb;

        if (imgb)
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
        xevd_mfree(pic->map_unrefined_mv);
        xevd_mfree(pic->map_refi);
        xevd_mfree(pic);
    }
}

XEVD_IMGB * xevd_imgb_generate(int w, int h, int padl, int padc, int idc, int bit_depth)
{
    XEVD_IMGB *imgb = NULL;
    int align[XEVD_IMGB_MAX_PLANE] = { MIN_CU_SIZE, MIN_CU_SIZE >> 1, MIN_CU_SIZE >> 1 };
    int pad[XEVD_IMGB_MAX_PLANE] = { padl, padc, padc, };
    imgb = xevd_imgb_create(w, h, XEVD_CS_SET(idc + 10, bit_depth, 0), 0, pad, align);
    if (imgb == NULL)
    {
        xevd_trace("Cannot generate image buffer\n");
        return imgb;
    }
    return imgb;
}
void xevd_imgb_destroy(XEVD_IMGB *imgb)
{
    if (imgb)
    {
        imgb->release(imgb);
    }
}
void scaling_mv(int ratio, s16 mvp[MV_D], s16 mv[MV_D])
{
    int tmp_mv;
    tmp_mv = mvp[MV_X] * ratio;
    tmp_mv = tmp_mv == 0 ? 0 : tmp_mv > 0 ? (tmp_mv + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION : -((-tmp_mv + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
    mv[MV_X] = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, tmp_mv);

    tmp_mv = mvp[MV_Y] * ratio;
    tmp_mv = tmp_mv == 0 ? 0 : tmp_mv > 0 ? (tmp_mv + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION : -((-tmp_mv + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
    mv[MV_Y] = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, tmp_mv);
}

void xevdm_get_mmvd_mvp_list(s8(*map_refi)[REFP_NUM], XEVD_REFP refp[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, int h_scu, int scup, u16 avail, int log2_cuw, int log2_cuh, int slice_t
    , int real_mv[][2][3], u32 *map_scu, int REF_SET[][XEVD_MAX_NUM_ACTIVE_REF_FRAME], u16 avail_lr
    , u32 curr_ptr, u8 num_refp[REFP_NUM]
    , XEVD_HISTORY_BUFFER history_buffer, int admvp_flag, XEVD_SH* sh, int log2_max_cuwh, u8* map_tidx, int mmvd_idx)
{
    int ref_mvd = 0;
    int ref_mvd1 = 0;
    int list0_weight;
    int list1_weight;
    int ref_sign = 0;
    int ref_sign1 = 0;
    int ref_mvd_cands[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    int hor0var[MMVD_MAX_REFINE_NUM] = { 0 };
    int ver0var[MMVD_MAX_REFINE_NUM] = { 0 };
    int hor1var[MMVD_MAX_REFINE_NUM] = { 0 };
    int ver1Var[MMVD_MAX_REFINE_NUM] = { 0 };
    int base_mv_idx = 0;
    int base_mv[25][2][3];
    s16 smvp[REFP_NUM][MAXM_NUM_MVP][MV_D];
    s8 srefi[REFP_NUM][MAXM_NUM_MVP];
    int base_mv_t[25][2][3];
    int base_type[3][MAXM_NUM_MVP];
    int cur_set;
    int total_num = MMVD_BASE_MV_NUM * MMVD_MAX_REFINE_NUM;
    int k;
    int cuw = (1 << log2_cuw);
    int cuh = (1 << log2_cuh);
    int list0_r;
    int list1_r;
    int poc0, poc1, poc_c;

    int base_mv_p[25][3][3];
    int small_cu = (cuw*cuh <= NUM_SAMPLES_BLOCK) ? 1 : 0;

    int base_st = mmvd_idx == -1 ? 0 : (mmvd_idx & 127) >> 5;
    int base_ed = mmvd_idx == -1 ? MMVD_BASE_MV_NUM : base_st + 1;

    int group_st = mmvd_idx == -1 ? 0 : mmvd_idx >> 7;
    int group_ed = mmvd_idx == -1 ? (small_cu ? 1 : 3) : group_st + 1;

    int mmvd_v_st = mmvd_idx == -1 ? 0 : (mmvd_idx & 31);
    int mmvd_v_ed = mmvd_idx == -1 ? MMVD_MAX_REFINE_NUM : mmvd_v_st + 1;

    if (admvp_flag == 0)
    {
        xevdm_get_motion_skip_baseline(slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, srefi, smvp, avail);
    }
    else
    {
        xevdm_get_motion_merge_main(curr_ptr, slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, h_scu, srefi, smvp, map_scu, avail_lr
            , NULL, history_buffer, 0, (XEVD_REFP(*)[2])refp, sh, log2_max_cuwh, map_tidx);
    }

    if (slice_t == SLICE_B)
    {
        for (k = base_st; k < base_ed; k++)
        {
            base_mv[k][REFP_0][MV_X] = smvp[REFP_0][k][MV_X];
            base_mv[k][REFP_0][MV_Y] = smvp[REFP_0][k][MV_Y];
            base_mv[k][REFP_1][MV_X] = smvp[REFP_1][k][MV_X];
            base_mv[k][REFP_1][MV_Y] = smvp[REFP_1][k][MV_Y];
            base_mv[k][REFP_0][REFI] = srefi[REFP_0][k];
            base_mv[k][REFP_1][REFI] = srefi[REFP_1][k];
        }
    }
    else
    {
        for (k = base_st; k < base_ed; k++)
        {
            base_mv[k][REFP_0][MV_X] = smvp[REFP_0][k][MV_X];
            base_mv[k][REFP_0][MV_Y] = smvp[REFP_0][k][MV_Y];
            base_mv[k][REFP_1][MV_X] = smvp[REFP_1][0][MV_X];
            base_mv[k][REFP_1][MV_Y] = smvp[REFP_1][0][MV_Y];
            base_mv[k][REFP_0][REFI] = srefi[REFP_0][k];
            base_mv[k][REFP_1][REFI] = srefi[REFP_1][0];
        }
    }

    for (k = base_st; k < base_ed; k++)
    {
        ref_sign = 1;
        ref_sign1 = 1;

        base_mv_t[k][REFP_0][MV_X] = base_mv[k][REFP_0][MV_X];
        base_mv_t[k][REFP_0][MV_Y] = base_mv[k][REFP_0][MV_Y];
        base_mv_t[k][REFP_0][REFI] = base_mv[k][REFP_0][REFI];

        base_mv_t[k][REFP_1][MV_X] = base_mv[k][REFP_1][MV_X];
        base_mv_t[k][REFP_1][MV_Y] = base_mv[k][REFP_1][MV_Y];
        base_mv_t[k][REFP_1][REFI] = base_mv[k][REFP_1][REFI];

        list0_r = base_mv_t[k][REFP_0][REFI];
        list1_r = base_mv_t[k][REFP_1][REFI];

        if ((base_mv_t[k][REFP_0][REFI] != REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] != REFI_INVALID))
        {
            base_type[0][k] = 0;
            base_type[1][k] = 1;
            base_type[2][k] = 2;
        }
        else if ((base_mv_t[k][REFP_0][REFI] != REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] == REFI_INVALID))
        {
            if (slice_t == SLICE_P)
            {
                int cur_ref_num = num_refp[REFP_0];
                base_type[0][k] = 1;
                base_type[1][k] = 1;
                base_type[2][k] = 1;

                if (cur_ref_num == 1)
                {
                    base_mv_p[k][0][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][1][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI];
                }
                else
                {
                    base_mv_p[k][0][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][1][REFI] = !base_mv_t[k][REFP_0][REFI];
                    if (cur_ref_num < 3)
                    {
                        base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI];
                    }
                    else
                    {
                        base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI] < 2 ? 2 : 1;
                    }
                }

                if (cur_ref_num == 1)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    base_mv_p[k][1][MV_X] = base_mv_t[k][REFP_0][MV_X] + 3;
                    base_mv_p[k][1][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    base_mv_p[k][2][MV_X] = base_mv_t[k][REFP_0][MV_X] - 3;
                    base_mv_p[k][2][MV_Y] = base_mv_t[k][REFP_0][MV_Y];
                }
                else if (cur_ref_num == 2)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    poc0 = REF_SET[0][base_mv_p[k][0][REFI]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][1][REFI]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][1][MV_X] = XEVD_CLIP3(-32768, 32767, ref_sign  * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][1][MV_Y] = XEVD_CLIP3(-32768, 32767, ref_sign1 * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][2][MV_X] = base_mv_t[k][REFP_0][MV_X] - 3;
                    base_mv_p[k][2][MV_Y] = base_mv_t[k][REFP_0][MV_Y];
                }
                else if (cur_ref_num >= 3)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];
                    base_mv_p[k][0][REFI] = base_mv_p[k][REFP_0][REFI];

                    poc0 = REF_SET[0][base_mv_p[k][0][REFI]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][1][REFI]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][1][MV_X] = XEVD_CLIP3(-32768, 32767, ref_sign  * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][1][MV_Y] = XEVD_CLIP3(-32768, 32767, ref_sign1 * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

                    poc0 = REF_SET[0][base_mv_p[k][0][2]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][2][2]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][2][MV_X] = XEVD_CLIP3(-32768, 32767, ref_sign  * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][2][MV_Y] = XEVD_CLIP3(-32768, 32767, ref_sign1 * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                }
            }
            else
            {
                base_type[0][k] = 1;
                base_type[1][k] = 0;
                base_type[2][k] = 2;

                list0_weight = 1 << MVP_SCALING_PRECISION;
                list1_weight = 1 << MVP_SCALING_PRECISION;
                poc0 = REF_SET[REFP_0][list0_r];
                poc_c = curr_ptr;
                if ((num_refp[REFP_1] > 1) && ((REF_SET[REFP_1][1] - poc_c) == (poc_c - poc0)))
                {
                    base_mv_t[k][REFP_1][REFI] = 1;
                }
                else
                {
                    base_mv_t[k][REFP_1][REFI] = 0;
                }
                poc1 = REF_SET[REFP_1][base_mv_t[k][REFP_1][REFI]];

                list1_weight = ((poc_c - poc1) << MVP_SCALING_PRECISION) / ((poc_c - poc0));
                if ((list1_weight * base_mv_t[k][0][0]) < 0)
                {
                    ref_sign = -1;
                }

                base_mv_t[k][REFP_1][MV_X] = XEVD_CLIP3(-32768, 32767, ref_sign * ((XEVD_ABS(list1_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

                list1_weight = ((poc_c - poc1) << MVP_SCALING_PRECISION) / ((poc_c - poc0));
                if ((list1_weight * base_mv_t[k][0][1]) < 0)
                {
                    ref_sign1 = -1;
                }

                base_mv_t[k][REFP_1][MV_Y] = XEVD_CLIP3(-32768, 32767, ref_sign1 * ((XEVD_ABS(list1_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
            }
        }
        else if ((base_mv_t[k][REFP_0][REFI] == REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] != REFI_INVALID))
        {
            base_type[0][k] = 2;
            base_type[1][k] = 0;
            base_type[2][k] = 1;

            list0_weight = 1 << MVP_SCALING_PRECISION;
            list1_weight = 1 << MVP_SCALING_PRECISION;
            poc1 = REF_SET[1][list1_r];
            poc_c = curr_ptr;
            if ((num_refp[REFP_0] > 1) && ((REF_SET[REFP_0][1] - poc_c) == (poc_c - poc1)))
            {
                base_mv_t[k][REFP_0][REFI] = 1;
            }
            else
            {
                base_mv_t[k][REFP_0][REFI] = 0;
            }
            poc0 = REF_SET[REFP_0][base_mv_t[k][REFP_0][REFI]];

            list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
            if ((list0_weight * base_mv_t[k][REFP_1][MV_X]) < 0)
            {
                ref_sign = -1;
            }
            base_mv_t[k][REFP_0][MV_X] = XEVD_CLIP3(-32768, 32767, ref_sign * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_1][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

            list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
            if ((list0_weight * base_mv_t[k][REFP_1][MV_Y]) < 0)
            {
                ref_sign1 = -1;
            }
            base_mv_t[k][REFP_0][MV_Y] = XEVD_CLIP3(-32768, 32767, ref_sign1 * ((XEVD_ABS(list0_weight * base_mv_t[k][REFP_1][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
        }
        else
        {
            base_type[0][k] = 3;
            base_type[1][k] = 3;
            base_type[2][k] = 3;
        }
    }

    for (base_mv_idx = base_st; base_mv_idx < base_ed; base_mv_idx++)
    {
        int list0_r, list1_r;
        int poc0, poc1, poc_c;

        if (small_cu)
        {
            base_type[0][base_mv_idx] = 1;
        }

        for (cur_set = group_st; cur_set < group_ed; cur_set++)
        {
            if (base_type[cur_set][base_mv_idx] == 0)
            {
                base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_t[base_mv_idx][REFP_0][MV_X];
                base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_t[base_mv_idx][REFP_0][MV_Y];
                base_mv[base_mv_idx][REFP_0][REFI] = base_mv_t[base_mv_idx][REFP_0][REFI];

                base_mv[base_mv_idx][REFP_1][MV_X] = base_mv_t[base_mv_idx][REFP_1][MV_X];
                base_mv[base_mv_idx][REFP_1][MV_Y] = base_mv_t[base_mv_idx][REFP_1][MV_Y];
                base_mv[base_mv_idx][REFP_1][REFI] = base_mv_t[base_mv_idx][REFP_1][REFI];
            }
            else if (base_type[cur_set][base_mv_idx] == 1)
            {
                if (slice_t == SLICE_P)
                {
                    base_mv[base_mv_idx][REFP_0][REFI] = base_mv_p[base_mv_idx][cur_set][REFI];
                    base_mv[base_mv_idx][REFP_1][REFI] = -1;

                    base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_p[base_mv_idx][cur_set][MV_X];
                    base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_p[base_mv_idx][cur_set][MV_Y];
                }
                else
                {
                    base_mv[base_mv_idx][REFP_0][REFI] = base_mv_t[base_mv_idx][REFP_0][REFI];
                    base_mv[base_mv_idx][REFP_1][REFI] = -1;

                    base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_t[base_mv_idx][REFP_0][MV_X];
                    base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_t[base_mv_idx][REFP_0][MV_Y];
                }
            }
            else if (base_type[cur_set][base_mv_idx] == 2)
            {
                base_mv[base_mv_idx][REFP_0][REFI] = -1;
                base_mv[base_mv_idx][REFP_1][REFI] = base_mv_t[base_mv_idx][REFP_1][REFI];

                base_mv[base_mv_idx][REFP_1][MV_X] = base_mv_t[base_mv_idx][REFP_1][MV_X];
                base_mv[base_mv_idx][REFP_1][MV_Y] = base_mv_t[base_mv_idx][REFP_1][MV_Y];
            }
            else if (base_type[cur_set][base_mv_idx] == 3)
            {
                base_mv[base_mv_idx][REFP_0][REFI] = -1;
                base_mv[base_mv_idx][REFP_1][REFI] = -1;
            }

            list0_r = base_mv[base_mv_idx][REFP_0][REFI];
            list1_r = base_mv[base_mv_idx][REFP_1][REFI];

            ref_sign = 1;
            if (slice_t == SLICE_B)
            {
                if ((list0_r != -1) && (list1_r != -1))
                {
                    poc0 = REF_SET[0][list0_r];
                    poc1 = REF_SET[1][list1_r];
                    poc_c = curr_ptr;
                    if ((poc0 - poc_c) * (poc_c - poc1) > 0)
                    {
                        ref_sign = -1;
                    }
                }
            }

            for (k = mmvd_v_st; k < mmvd_v_ed; k++)
            {
                list0_weight = 1 << MVP_SCALING_PRECISION;
                list1_weight = 1 << MVP_SCALING_PRECISION;
                ref_mvd = ref_mvd_cands[(int)(k / 4)];
                ref_mvd1 = ref_mvd_cands[(int)(k / 4)];

                if ((list0_r != -1) && (list1_r != -1))
                {
                    poc0 = REF_SET[0][list0_r];
                    poc1 = REF_SET[1][list1_r];
                    poc_c = curr_ptr;

                    if (XEVD_ABS(poc1 - poc_c) >= XEVD_ABS(poc0 - poc_c))
                    {
                        list0_weight = (XEVD_ABS(poc0 - poc_c) << MVP_SCALING_PRECISION) / (XEVD_ABS(poc1 - poc_c));
                        ref_mvd = XEVD_CLIP3(-32768, 32767, (list0_weight * ref_mvd_cands[(int)(k / 4)] + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
                    }
                    else
                    {
                        list1_weight = (XEVD_ABS(poc1 - poc_c) << MVP_SCALING_PRECISION) / (XEVD_ABS(poc0 - poc_c));
                        ref_mvd1 = XEVD_CLIP3(-32768, 32767, (list1_weight * ref_mvd_cands[(int)(k / 4)] + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
                    }

                    ref_mvd = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, ref_mvd);
                    ref_mvd1 = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, ref_mvd1);
                }

                if ((k % 4) == 0)
                {
                    hor0var[k] = ref_mvd;
                    hor1var[k] = ref_mvd1 * ref_sign;
                    ver0var[k] = 0;
                    ver1Var[k] = 0;
                }
                else if ((k % 4) == 1)
                {
                    hor0var[k] = ref_mvd * -1;
                    hor1var[k] = ref_mvd1 * -1 * ref_sign;
                    ver0var[k] = 0;
                    ver1Var[k] = 0;
                }
                else if ((k % 4) == 2)
                {
                    hor0var[k] = 0;
                    hor1var[k] = 0;
                    ver0var[k] = ref_mvd;
                    ver1Var[k] = ref_mvd1 * ref_sign;
                }
                else
                {
                    hor0var[k] = 0;
                    hor1var[k] = 0;
                    ver0var[k] = ref_mvd * -1;
                    ver1Var[k] = ref_mvd1 * -1 * ref_sign;
                }

                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][MV_X] = base_mv[base_mv_idx][REFP_0][MV_X] + hor0var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][MV_Y] = base_mv[base_mv_idx][REFP_0][MV_Y] + ver0var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][MV_X] = base_mv[base_mv_idx][REFP_1][MV_X] + hor1var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][MV_Y] = base_mv[base_mv_idx][REFP_1][MV_Y] + ver1Var[k];

                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][REFI] = base_mv[base_mv_idx][REFP_0][REFI];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][REFI] = base_mv[base_mv_idx][REFP_1][REFI];
            }
        }
    }
}

void xevdm_check_motion_availability(int scup, int cuw, int cuh, int w_scu, int h_scu, int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], u32* map_scu, u16 avail_lr, int num_mvp, int is_ibc, u8* map_tidx)
{
    int dx = 0;
    int dy = 0;

    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    xevd_mset(valid_flag, 0, 5 * sizeof(int));

    if (avail_lr == LR_11)
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu - 1; // H
        neb_addr[1] = scup + (scuh - 1) * w_scu + scuw; // inverse H
        neb_addr[2] = scup - w_scu;

        if (is_ibc)
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup - w_scu + scuw;
            neb_addr[4] = scup - w_scu - 1;

            if (is_ibc)
            {
                valid_flag[3] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
                valid_flag[3] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
    else if (avail_lr == LR_01)
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu + scuw; // inverse H
        neb_addr[1] = scup - w_scu; // inverse D
        neb_addr[2] = scup - w_scu - 1;  // inverse E

        if (is_ibc)
        {
            valid_flag[0] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
            valid_flag[0] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup + scuh * w_scu + scuw; // inverse I
            neb_addr[4] = scup - w_scu + scuw; // inverse A

            if (is_ibc)
            {
                valid_flag[3] = (y_scu + scuh < h_scu && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
                valid_flag[3] = (y_scu + scuh < h_scu && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
    else
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu - 1; // H
        neb_addr[1] = scup - w_scu + scuw - 1; // D
        neb_addr[2] = scup - w_scu + scuw;  // E

        if (is_ibc)
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup + scuh * w_scu - 1; // I
            neb_addr[4] = scup - w_scu - 1; // A

            if (is_ibc)
            {
                valid_flag[3] = (y_scu + scuh < h_scu && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
                valid_flag[3] = (y_scu + scuh < h_scu && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
}

s8 xevdm_get_first_refi(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag, u8* map_tidx)
{
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    s8  refi = 0, default_refi;
    s16 default_mv[MV_D];

    xevdm_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, 0, map_tidx);
    xevdm_get_default_motion(neb_addr, valid_flag, 0, lidx, map_refi, map_mv, &default_refi, default_mv
        , map_scu, map_unrefined_mv, scup, w_scu, history_buffer, hmvp_flag);

    assert(mvr_idx < 5);
    //neb-position is coupled with mvr index
    if (valid_flag[mvr_idx])
    {
        refi = REFI_IS_VALID(map_refi[neb_addr[mvr_idx]][lidx]) ? map_refi[neb_addr[mvr_idx]][lidx] : default_refi;
    }
    else
    {
        refi = default_refi;
    }

    return refi;
}

void xevdm_get_default_motion(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
    , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu, XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag)
{
    int k;
    int found = 0;
    s8  tmp_refi = 0;

    *refi = 0;
    mv[MV_X] = 0;
    mv[MV_Y] = 0;

    for (k = 0; k < 2; k++)
    {
        if (valid_flag[k])
        {
            tmp_refi = REFI_IS_VALID(map_refi[neb_addr[k]][lidx]) ? map_refi[neb_addr[k]][lidx] : REFI_INVALID;
            if (tmp_refi == cur_refi)
            {
                found = 1;
                *refi = tmp_refi;

                if (MCU_GET_DMVRF(map_scu[neb_addr[k]]))
                {
                    mv[MV_X] = map_unrefined_mv[neb_addr[k]][lidx][MV_X];
                    mv[MV_Y] = map_unrefined_mv[neb_addr[k]][lidx][MV_Y];
                }
                else
                {
                    mv[MV_X] = map_mv[neb_addr[k]][lidx][MV_X];
                    mv[MV_Y] = map_mv[neb_addr[k]][lidx][MV_Y];
                }
                break;
            }
        }
    }

    if (!found)
    {
        for (k = 0; k < 2; k++)
        {
            if (valid_flag[k])
            {
                tmp_refi = REFI_IS_VALID(map_refi[neb_addr[k]][lidx]) ? map_refi[neb_addr[k]][lidx] : REFI_INVALID;
                if (tmp_refi != REFI_INVALID)
                {
                    found = 1;
                    *refi = tmp_refi;

                    if (MCU_GET_DMVRF(map_scu[neb_addr[k]]))
                    {
                        mv[MV_X] = map_unrefined_mv[neb_addr[k]][lidx][MV_X];
                        mv[MV_Y] = map_unrefined_mv[neb_addr[k]][lidx][MV_Y];
                    }
                    else
                    {
                        mv[MV_X] = map_mv[neb_addr[k]][lidx][MV_X];
                        mv[MV_Y] = map_mv[neb_addr[k]][lidx][MV_Y];
                    }
                    break;
                }
            }
        }
    }
    if (hmvp_flag)
    {
        if (!found)
        {
            for (k = 1; k <= XEVD_MIN(history_buffer.currCnt, ALLOWED_CHECKED_AMVP_NUM); k++)
            {
                tmp_refi = REFI_IS_VALID(history_buffer.history_refi_table[history_buffer.currCnt - k][lidx]) ? history_buffer.history_refi_table[history_buffer.currCnt - k][lidx] : REFI_INVALID;
                if (tmp_refi == cur_refi)
                {
                    found = 1;
                    *refi = tmp_refi;
                    mv[MV_X] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_X];
                    mv[MV_Y] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_Y];
                    break;
                }
            }
        }

        if (!found)
        {
            for (k = 1; k <= XEVD_MIN(history_buffer.currCnt, ALLOWED_CHECKED_AMVP_NUM); k++)
            {
                tmp_refi = REFI_IS_VALID(history_buffer.history_refi_table[history_buffer.currCnt - k][lidx]) ? history_buffer.history_refi_table[history_buffer.currCnt - k][lidx] : REFI_INVALID;
                if (tmp_refi != REFI_INVALID)
                {
                    found = 1;
                    *refi = tmp_refi;
                    mv[MV_X] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_X];
                    mv[MV_Y] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_Y];
                    break;
                }
            }
        }
    }
}

void xevdm_get_motion_from_mvr(u8 mvr_idx, int poc, int scup, int lidx, s8 cur_refi, int num_refp, \
    s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVD_REFP(*refp)[REFP_NUM], \
    int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAXM_NUM_MVP][MV_D], s8 refi[MAXM_NUM_MVP], u32* map_scu, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, int hmvp_flag, u8* map_tidx
)
{
    int i, t0, poc_refi_cur;
    int ratio[XEVD_MAX_NUM_REF_PICS];
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    int rounding = mvr_idx > 0 ? 1 << (mvr_idx - 1) : 0;
    s8 default_refi;
    s16 default_mv[MV_D];
    s16 mvp_temp[MV_D];
    xevdm_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, 0, map_tidx);
    xevdm_get_default_motion(neb_addr, valid_flag, cur_refi, lidx, map_refi, map_mv, &default_refi, default_mv
        , map_scu, map_unrefined_mv, scup, w_scu, history_buffer, hmvp_flag);

    poc_refi_cur = refp[cur_refi][lidx].poc;
    for (i = 0; i < num_refp; i++)
    {
        t0 = poc - refp[i][lidx].poc;

        ratio[i] = ((poc - poc_refi_cur) << MVP_SCALING_PRECISION) / t0;
    }
    assert(mvr_idx < 5);

    if (valid_flag[mvr_idx])
    {
        refi[0] = REFI_IS_VALID(map_refi[neb_addr[mvr_idx]][lidx]) ? map_refi[neb_addr[mvr_idx]][lidx] : REFI_INVALID;
        if (refi[0] == cur_refi)
        {
            if (MCU_GET_DMVRF(map_scu[neb_addr[mvr_idx]]))
            {
                mvp_temp[MV_X] = map_unrefined_mv[neb_addr[mvr_idx]][lidx][MV_X];
                mvp_temp[MV_Y] = map_unrefined_mv[neb_addr[mvr_idx]][lidx][MV_Y];
            }
            else
            {
                mvp_temp[MV_X] = map_mv[neb_addr[mvr_idx]][lidx][MV_X];
                mvp_temp[MV_Y] = map_mv[neb_addr[mvr_idx]][lidx][MV_Y];
            }
        }
        else if (refi[0] == REFI_INVALID)
        {
            refi[0] = default_refi;
            if (refi[0] == cur_refi)
            {
                mvp_temp[MV_X] = default_mv[MV_X];
                mvp_temp[MV_Y] = default_mv[MV_Y];
            }
            else
            {
                scaling_mv(ratio[refi[0]], default_mv, mvp_temp);
            }
        }
        else
        {
            if (MCU_GET_DMVRF(map_scu[neb_addr[mvr_idx]]))
            {
                scaling_mv(ratio[refi[0]], map_unrefined_mv[neb_addr[mvr_idx]][lidx], mvp_temp);
            }
            else
            {
                scaling_mv(ratio[refi[0]], map_mv[neb_addr[mvr_idx]][lidx], mvp_temp);
            }
        }
    }
    else
    {
        refi[0] = default_refi;
        if (refi[0] == cur_refi)
        {
            mvp_temp[MV_X] = default_mv[MV_X];
            mvp_temp[MV_Y] = default_mv[MV_Y];
        }
        else
        {
            scaling_mv(ratio[refi[0]], default_mv, mvp_temp);
        }
    }
    mvp[0][MV_X] = (mvp_temp[MV_X] >= 0) ? (((mvp_temp[MV_X] + rounding) >> mvr_idx) << mvr_idx) : -(((-mvp_temp[MV_X] + rounding) >> mvr_idx) << mvr_idx);
    mvp[0][MV_Y] = (mvp_temp[MV_Y] >= 0) ? (((mvp_temp[MV_Y] + rounding) >> mvr_idx) << mvr_idx) : -(((-mvp_temp[MV_Y] + rounding) >> mvr_idx) << mvr_idx);
}

void xevdm_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP(*refp)[REFP_NUM],
    int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAXM_NUM_MVP], s16 mvp[MAXM_NUM_MVP][MV_D])
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
#if MERGE_MVP
static int xevd_get_right_below_scup_merge(int scup, int cuw, int cuh, int w_scu, int h_scu, int bottom_right, int log2_max_cuwh)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu + scuw - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (bottom_right == 0)            // fetch bottom sample
    {
        if (y_scu + 1 >= h_scu)
            return -1;
        else if (((y_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (y_scu << MIN_CU_LOG2 >> log2_max_cuwh))
            return -1; // check same CTU row, align to spec
        else
            return ((y_scu + 1) >> 1 << 1) * w_scu + (x_scu >> 1 << 1);
    }
    else if (bottom_right == 1)        // fetch bottom-to-right sample
    {
        if (x_scu + 1 >= w_scu)
            return -1;
        else if (((x_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (x_scu << MIN_CU_LOG2 >> log2_max_cuwh))
            return -1; // check same CTU column, align to spec
        else
            return (y_scu >> 1 << 1) * w_scu + ((x_scu + 1) >> 1 << 1);
    }
    return -1;
}

static int xevd_get_right_below_scup_merge_suco(int scup, int cuw, int cuh, int w_scu, int h_scu, int bottom_right, int log2_max_cuwh)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (bottom_right == 0)            // fetch bottom sample
    {
        if (y_scu + 1 >= h_scu)
            return -1;
        else if (((y_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (y_scu << MIN_CU_LOG2 >> log2_max_cuwh))
            return -1; // check same CTU row, align to spec
        else
            return ((y_scu + 1) >> 1 << 1) * w_scu + ((x_scu + 1) >> 1 << 1);  // bottom sample
    }
    else if (bottom_right == 1)        // fetch bottom-to-left sample
    {
        if (x_scu < 0)
            return -1;
        else if (((x_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (x_scu << MIN_CU_LOG2 >> log2_max_cuwh))
            return -1; // check same CTU column, align to spec
        else
            return (y_scu >> 1 << 1) * w_scu + (x_scu >> 1 << 1);
    }
    return -1;
}

static int xevd_get_right_below_scup(int scup, int cuw, int cuh, int w_scu, int h_scu)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu + scuw - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (x_scu + 1 < w_scu && y_scu + 1 < h_scu)
    {
        return (y_scu + 1)*w_scu + (x_scu + 1);
    }
    else if (x_scu + 1 < w_scu)
    {
        return y_scu*w_scu + (x_scu + 1);
    }
    else if (y_scu + 1 < h_scu)
    {
        return (y_scu + 1)*w_scu + x_scu;
    }
    else
    {
        return y_scu*w_scu + x_scu;
    }
}
#endif

BOOL xevdm_check_bi_applicability(int slice_type, int cuw, int cuh, int is_sps_admvp)
{
    BOOL is_applicable = FALSE;

    if (slice_type == SLICE_B)
    {
        if (!is_sps_admvp || cuw + cuh > 12)
        {
            is_applicable = TRUE;
        }
    }

    return is_applicable;
}

__inline static void check_redundancy(int slice_type, s16 mvp[REFP_NUM][MAXM_NUM_MVP][MV_D], s8 refi[REFP_NUM][MAXM_NUM_MVP], int *count)
{
    int i;
    int cnt = *count;

    if (cnt > 0)
    {
        if (refi != NULL)
        {
            for (i = (cnt)-1; i >= 0; i--)
            {
                if (refi[REFP_0][cnt] == refi[REFP_0][i] && mvp[REFP_0][cnt][MV_X] == mvp[REFP_0][i][MV_X] && mvp[REFP_0][cnt][MV_Y] == mvp[REFP_0][i][MV_Y])
                {
                    if (slice_type != SLICE_B || (refi[REFP_1][cnt] == refi[REFP_1][i] && mvp[REFP_1][cnt][MV_X] == mvp[REFP_1][i][MV_X] && mvp[REFP_1][cnt][MV_Y] == mvp[REFP_1][i][MV_Y]))
                    {
                        cnt--;
                        break;
                    }
                }
            }
        }
        else
        {
            for (i = cnt - 1; i >= 0; i--)
            {
                if (mvp[REFP_0][cnt][MV_X] == mvp[REFP_0][i][MV_X] && mvp[REFP_0][cnt][MV_Y] == mvp[REFP_0][i][MV_Y])
                {
                    if (slice_type != SLICE_B || (mvp[REFP_1][cnt][MV_X] == mvp[REFP_1][i][MV_X] && mvp[REFP_1][cnt][MV_Y] == mvp[REFP_1][i][MV_Y]))
                    {
                        cnt--;
                        break;
                    }
                }
            }
        }
        *count = cnt;
    }
}

void xevdm_get_merge_insert_mv(s8* refi_dst, s16 *mvp_dst_L0, s16 *mvp_dst_L1, s8* map_refi_src, s16* map_mv_src, int slice_type, int cuw, int cuh, int is_sps_admvp)
{
    refi_dst[REFP_0 * MAXM_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_0]) ? map_refi_src[REFP_0] : REFI_INVALID;
    mvp_dst_L0[MV_X] = map_mv_src[REFP_0 * REFP_NUM + MV_X];
    mvp_dst_L0[MV_Y] = map_mv_src[REFP_0 * REFP_NUM + MV_Y];

    if (slice_type == SLICE_B)
    {
        if (!REFI_IS_VALID(map_refi_src[REFP_0]))
        {
            refi_dst[REFP_1 * MAXM_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_1]) ? map_refi_src[REFP_1] : REFI_INVALID;
            mvp_dst_L1[MV_X] = map_mv_src[REFP_1 * REFP_NUM + MV_X];
            mvp_dst_L1[MV_Y] = map_mv_src[REFP_1 * REFP_NUM + MV_Y];
        }
        else if (!xevdm_check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
        {
            refi_dst[REFP_1 * MAXM_NUM_MVP] = REFI_INVALID; // TBD: gcc10 triggers stringop-overflow at this line
            mvp_dst_L1[MV_X] = 0;
            mvp_dst_L1[MV_Y] = 0;
        }
        else
        {
            refi_dst[REFP_1 * MAXM_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_1]) ? map_refi_src[REFP_1] : REFI_INVALID;
            mvp_dst_L1[MV_X] = map_mv_src[REFP_1 * REFP_NUM + MV_X];
            mvp_dst_L1[MV_Y] = map_mv_src[REFP_1 * REFP_NUM + MV_Y];
        }
    }
}

void xevdm_get_motion_merge_main(int ptr, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D],
    XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu, s8 refi[REFP_NUM][MAXM_NUM_MVP], s16 mvp[REFP_NUM][MAXM_NUM_MVP][MV_D], u32 *map_scu, u16 avail_lr
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVD_HISTORY_BUFFER history_buffer, u8 ibc_flag, XEVD_REFP(*refplx)[REFP_NUM], XEVD_SH* sh, int log2_max_cuwh, u8* map_tidx)
{
    BOOL tmpvBottomRight = 0; // Bottom first
    int is_sps_admvp = 1;
    int small_cu = 0;

    if (cuw*cuh <= NUM_SAMPLES_BLOCK)
    {
        small_cu = 1;
    }

    int k, cnt = 0;
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    s16 tmvp[REFP_NUM][MV_D];
    int scup_tmp;
    int cur_num, i, idx0, idx1;
    int c_win = 0;
    xevd_mset(mvp, 0, MAXM_NUM_MVP * REFP_NUM * MV_D * sizeof(s16));
    xevd_mset(refi, REFI_INVALID, MAXM_NUM_MVP * REFP_NUM * sizeof(s8));
    s8 refidx = REFI_INVALID;
    s8  *p_ref_dst = NULL;
    s16 *p_map_mv_dst_L0 = NULL;
    s16 *p_map_mv_dst_L1 = NULL;
    s8  *p_ref_src = NULL;
    s16 *p_map_mv_src = NULL;
    for (k = 0; k < MAX_NUM_POSSIBLE_SCAND; k++)
    {
        valid_flag[k] = 0;
    }
    xevdm_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, ibc_flag, map_tidx);

    for (k = 0; k < 5; k++)
    {
        p_ref_dst = &(refi[0][cnt]);
        p_map_mv_dst_L0 = mvp[REFP_0][cnt];
        p_map_mv_dst_L1 = mvp[REFP_1][cnt];
        p_ref_src = map_refi[neb_addr[k]];
        p_map_mv_src = &(map_mv[neb_addr[k]][0][0]);

        if (valid_flag[k])
        {
            if ((NULL != map_unrefined_mv) && MCU_GET_DMVRF(map_scu[neb_addr[k]]))
            {
                p_ref_src = map_refi[neb_addr[k]];
                p_map_mv_src = &(map_unrefined_mv[neb_addr[k]][0][0]);
            }

            xevdm_get_merge_insert_mv(p_ref_dst, p_map_mv_dst_L0, p_map_mv_dst_L1, p_ref_src, p_map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
        }
        if (cnt == (small_cu ? MAX_NUM_MVP_SMALL_CU - 1 : MAXM_NUM_MVP - 1))
        {
            break;
        }
    }

    int tmvp_cnt_pos0 = 0, tmvp_cnt_pos1 = 0;
    int tmvp_added = 0;

    if (!tmvp_added)
    {// TMVP-central
        s8 availablePredIdx = 0;

        int x_scu = (scup % w_scu);
        int y_scu = (scup / w_scu);
        int scu_col = ((x_scu + (cuw >> 1 >> MIN_CU_LOG2)) >> 1 << 1) + ((y_scu + (cuh >> 1 >> MIN_CU_LOG2)) >> 1 << 1) * w_scu; // 8x8 grid
        xevdm_get_mv_collocated(
            refplx,
            ptr, scu_col, scup, w_scu, h_scu, tmvp, &availablePredIdx, sh);

        tmvp_cnt_pos0 = cnt;
        if (availablePredIdx != 0)
        {
            p_ref_dst = &(refi[0][cnt]);
            p_map_mv_dst_L0 = mvp[REFP_0][cnt];
            p_map_mv_dst_L1 = mvp[REFP_1][cnt];
            s8 refs[2] = { -1, -1 };
            refs[0] = (availablePredIdx == 1 || availablePredIdx == 3) ? 0 : -1;
            refs[1] = (availablePredIdx == 2 || availablePredIdx == 3) ? 0 : -1;
            p_ref_src = refs;
            p_map_mv_src = &(tmvp[0][0]);
            xevdm_get_merge_insert_mv(p_ref_dst, p_map_mv_dst_L0, p_map_mv_dst_L1, p_ref_src, p_map_mv_src, slice_type, cuw, cuh, is_sps_admvp);

            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
            tmvp_cnt_pos1 = cnt;
            if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                tmvp_added = 1;
            if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
            {
                return;
            }
        }
    } // TMVP-central
    if (!tmvp_added)
    {// Bottom first
        s8 availablePredIdx = 0;
        tmpvBottomRight = 0;
        if (avail_lr == LR_01)
            scup_tmp = xevd_get_right_below_scup_merge_suco(scup, cuw, cuh, w_scu, h_scu, tmpvBottomRight, log2_max_cuwh);
        else
            scup_tmp = xevd_get_right_below_scup_merge(scup, cuw, cuh, w_scu, h_scu, tmpvBottomRight, log2_max_cuwh);
        if (scup_tmp != -1)  // if available, add it to candidate list
        {
            xevdm_get_mv_collocated(refplx, ptr, scup_tmp, scup, w_scu, h_scu, tmvp, &availablePredIdx, sh);
            tmvp_cnt_pos0 = cnt;
            if (availablePredIdx != 0)
            {
                p_ref_dst = &(refi[0][cnt]);
                p_map_mv_dst_L0 = mvp[REFP_0][cnt];
                p_map_mv_dst_L1 = mvp[REFP_1][cnt];
                s8 refs[2] = { -1, -1 };
                refs[0] = (availablePredIdx == 1 || availablePredIdx == 3) ? 0 : -1;
                refs[1] = (availablePredIdx == 2 || availablePredIdx == 3) ? 0 : -1;
                p_ref_src = refs;
                p_map_mv_src = &(tmvp[0][0]);
                xevdm_get_merge_insert_mv(p_ref_dst, p_map_mv_dst_L0, p_map_mv_dst_L1, p_ref_src, p_map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
                check_redundancy(slice_type, mvp, refi, &cnt);
                cnt++;
                tmvp_cnt_pos1 = cnt;
                if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                    tmvp_added = 1;
                if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
                {
                    return;
                }
            }
        }
    }
    if (!tmvp_added)
    {
        s8 availablePredIdx = 0;
        if (avail_lr == LR_01)
            scup_tmp = xevd_get_right_below_scup_merge_suco(scup, cuw, cuh, w_scu, h_scu, !tmpvBottomRight, log2_max_cuwh);
        else
            scup_tmp = xevd_get_right_below_scup_merge(scup, cuw, cuh, w_scu, h_scu, !tmpvBottomRight, log2_max_cuwh);
        if (scup_tmp != -1)  // if available, add it to candidate list
        {
            xevdm_get_mv_collocated(refplx, ptr, scup_tmp, scup, w_scu, h_scu, tmvp, &availablePredIdx, sh);

            tmvp_cnt_pos0 = cnt;
            if (availablePredIdx != 0)
            {
                p_ref_dst = &(refi[0][cnt]);
                p_map_mv_dst_L0 = mvp[REFP_0][cnt];
                p_map_mv_dst_L1 = mvp[REFP_1][cnt];
                s8 refs[2] = { -1, -1 };
                refs[0] = (availablePredIdx == 1 || availablePredIdx == 3) ? 0 : -1;
                refs[1] = (availablePredIdx == 2 || availablePredIdx == 3) ? 0 : -1;
                p_ref_src = refs;
                p_map_mv_src = &(tmvp[0][0]);
                xevdm_get_merge_insert_mv(p_ref_dst, p_map_mv_dst_L0, p_map_mv_dst_L1, p_ref_src, p_map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
                check_redundancy(slice_type, mvp, refi, &cnt);
                cnt++;
                tmvp_cnt_pos1 = cnt;
                if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                    tmvp_added = 1;
                if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
                {
                    return;
                }
            }
        }
    }

    if (cnt < (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
    {

        for (k = 3; k <= XEVD_MIN(history_buffer.currCnt, small_cu ? ALLOWED_CHECKED_NUM_SMALL_CU : ALLOWED_CHECKED_NUM); k += 4)
        {
            p_ref_dst = &(refi[0][cnt]);
            p_map_mv_dst_L0 = mvp[REFP_0][cnt];
            p_map_mv_dst_L1 = mvp[REFP_1][cnt];
            p_ref_src = history_buffer.history_refi_table[history_buffer.currCnt - k];
            p_map_mv_src = &(history_buffer.history_mv_table[history_buffer.currCnt - k][0][0]);
            xevdm_get_merge_insert_mv(p_ref_dst, p_map_mv_dst_L0, p_map_mv_dst_L1, p_ref_src, p_map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
            if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
            {
                return;
            }

        }
    }
    // B slice mv combination
    if (xevdm_check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
    {
        int priority_list0[MAXM_NUM_MVP*MAXM_NUM_MVP] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3, 0, 4, 1, 4, 2, 4, 3, 4 };
        int priority_list1[MAXM_NUM_MVP*MAXM_NUM_MVP] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2, 4, 0, 4, 1, 4, 2, 4, 3 };
        cur_num = cnt;
        for (i = 0; i < cur_num*(cur_num - 1) && cnt != (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP); i++)
        {
            idx0 = priority_list0[i];
            idx1 = priority_list1[i];

            if (REFI_IS_VALID(refi[REFP_0][idx0]) && REFI_IS_VALID(refi[REFP_1][idx1]))
            {
                refi[REFP_0][cnt] = refi[REFP_0][idx0];
                mvp[REFP_0][cnt][MV_X] = mvp[REFP_0][idx0][MV_X];
                mvp[REFP_0][cnt][MV_Y] = mvp[REFP_0][idx0][MV_Y];

                refi[REFP_1][cnt] = refi[REFP_1][idx1];
                mvp[REFP_1][cnt][MV_X] = mvp[REFP_1][idx1][MV_X];
                mvp[REFP_1][cnt][MV_Y] = mvp[REFP_1][idx1][MV_Y];
                cnt++;
            }
        }
        if (cnt == (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP))
        {
            return;
        }
    }

    for (k = cnt; k < (small_cu ? MAX_NUM_MVP_SMALL_CU : MAXM_NUM_MVP); k++)
    {
        refi[REFP_0][k] = 0;
        mvp[REFP_0][k][MV_X] = 0;
        mvp[REFP_0][k][MV_Y] = 0;
        if (!xevdm_check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
        {
            refi[REFP_1][k] = REFI_INVALID;
            mvp[REFP_1][k][MV_X] = 0;
            mvp[REFP_1][k][MV_Y] = 0;
        }
        else
        {
            refi[REFP_1][k] = 0;
            mvp[REFP_1][k][MV_X] = 0;
            mvp[REFP_1][k][MV_Y] = 0;
        }
    }
}

void xevdm_get_motion_skip_baseline(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVD_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, s8 refi[REFP_NUM][MAXM_NUM_MVP], s16 mvp[REFP_NUM][MAXM_NUM_MVP][MV_D], u16 avail_lr)
{
    xevd_mset(mvp, 0, MAXM_NUM_MVP * REFP_NUM * MV_D * sizeof(s16));
    xevd_mset(refi, REFI_INVALID, MAXM_NUM_MVP * REFP_NUM * sizeof(s8));
    xevdm_get_motion(scup, REFP_0, map_refi, map_mv, (XEVD_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_0], mvp[REFP_0]);
    if (slice_type == SLICE_B)
    {
        xevdm_get_motion(scup, REFP_1, map_refi, map_mv, (XEVD_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_1], mvp[REFP_1]);
    }
}

void xevdm_clip_mv_pic(int x, int y, int maxX, int maxY, s16 mvp[REFP_NUM][MV_D])
{
    int minXY = -PIC_PAD_SIZE_L;
    mvp[REFP_0][MV_X] = (x + mvp[REFP_0][MV_X]) < minXY ? -(x + minXY) : mvp[REFP_0][MV_X];
    mvp[REFP_1][MV_X] = (x + mvp[REFP_1][MV_X]) < minXY ? -(x + minXY) : mvp[REFP_1][MV_X];
    mvp[REFP_0][MV_Y] = (y + mvp[REFP_0][MV_Y]) < minXY ? -(y + minXY) : mvp[REFP_0][MV_Y];
    mvp[REFP_1][MV_Y] = (y + mvp[REFP_1][MV_Y]) < minXY ? -(y + minXY) : mvp[REFP_1][MV_Y];

    mvp[REFP_0][MV_X] = (x + mvp[REFP_0][MV_X]) > maxX ? (maxX - x) : mvp[REFP_0][MV_X];
    mvp[REFP_1][MV_X] = (x + mvp[REFP_1][MV_X]) > maxX ? (maxX - x) : mvp[REFP_1][MV_X];
    mvp[REFP_0][MV_Y] = (y + mvp[REFP_0][MV_Y]) > maxY ? (maxY - y) : mvp[REFP_0][MV_Y];
    mvp[REFP_1][MV_Y] = (y + mvp[REFP_1][MV_Y]) > maxY ? (maxY - y) : mvp[REFP_1][MV_Y];
}

void xevdm_get_mv_dir(XEVD_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D]
    , int sps_admvp_flag
)
{
    s16 mvc[MV_D];
    int dpoc_co, dpoc_L0, dpoc_L1;

    mvc[MV_X] = refp[REFP_1].map_mv[scup][0][MV_X];
    mvc[MV_Y] = refp[REFP_1].map_mv[scup][0][MV_Y];

    dpoc_co = refp[REFP_1].poc - refp[REFP_1].list_poc[0];
    dpoc_L0 = poc - refp[REFP_0].poc;
    dpoc_L1 = refp[REFP_1].poc - poc;

    if (dpoc_co == 0)
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

u16 xevdm_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int curr_scup = x_scu + y_scu * w_scu;

    if (x_scu > 0 && !MCU_GET_IF(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) &&
        (map_tidx[curr_scup] == map_tidx[scup - 1]) && !MCU_GET_IBC(map_scu[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) - 1]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) - 1]) && !MCU_GET_IBC(map_scu[scup + (scuh * w_scu) - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if (y_scu > 0)
    {
        if (!MCU_GET_IF(map_scu[scup - w_scu]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu]) && !MCU_GET_IBC(map_scu[scup - w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if (!MCU_GET_IF(map_scu[scup - w_scu + scuw - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw - 1]) &&
            !MCU_GET_IBC(map_scu[scup - w_scu + scuw - 1]))
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if (x_scu > 0 && !MCU_GET_IF(map_scu[scup - w_scu - 1]) && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu - 1]) &&
            !MCU_GET_IBC(map_scu[scup - w_scu - 1]))
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

    if (x_scu + scuw < w_scu && !MCU_GET_IF(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]) &&
        !MCU_GET_IBC(map_scu[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) + scuw]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) + scuw]) &&
            (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) + scuw]) && !MCU_GET_IBC(map_scu[scup + (scuh * w_scu) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

u16 xevdm_get_avail_ibc(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    if (x_scu > 0 && MCU_GET_IBC(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && MCU_GET_IBC(map_scu[scup + (scuh * w_scu) - 1]) &&
            (map_tidx[scup] == map_tidx[scup + (scuh * w_scu) - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if (y_scu > 0)
    {
        if (MCU_GET_IBC(map_scu[scup - w_scu]) && (map_tidx[scup] == map_tidx[scup - w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if (MCU_GET_IBC(map_scu[scup - w_scu + scuw - 1]) && (map_tidx[scup] == map_tidx[scup - w_scu + scuw - 1]))
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if (x_scu > 0 && MCU_GET_IBC(map_scu[scup - w_scu - 1]) && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[scup] == map_tidx[scup - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if (x_scu + scuw < w_scu  && MCU_IS_COD_NIF(map_scu[scup - w_scu + scuw]) && MCU_GET_COD(map_scu[scup - w_scu + scuw]) && (map_tidx[scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if (x_scu + scuw < w_scu && MCU_GET_IBC(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) + scuw]) && MCU_GET_IBC(map_scu[scup + (scuh * w_scu) + scuw]) &&
            (map_tidx[scup] == map_tidx[scup + (scuh * w_scu) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

void xevdm_check_split_mode(int *split_allow, int log2_cuw, int log2_cuh, int boundary, int boundary_b, int boundary_r, int log2_max_cuwh
    , const int parent_split, int* same_layer_split, const int node_idx, const int* parent_split_allow, int qt_depth, int btt_depth
    , int x, int y, int im_w, int im_h
    , u8 *remaining_split, int sps_btt_flag
    , MODE_CONS mode_cons)
{
    if (!sps_btt_flag)
    {
        xevd_mset(split_allow, 0, sizeof(int) * SPLIT_CHECK_NUM);
        split_allow[SPLIT_QUAD] = 1;
        return;
    }

    int log2_sub_cuw, log2_sub_cuh;
    int long_side, ratio;
    int cu_max, from_boundary_b;
    cu_max = 1 << (log2_max_cuwh - 1);
    from_boundary_b = (y >= im_h - im_h%cu_max) && !(x >= im_w - im_w % cu_max);

    xevd_mset(split_allow, 0, sizeof(int) * SPLIT_CHECK_NUM);
    {
        split_allow[SPLIT_QUAD] = 0;

        if (log2_cuw == log2_cuh)
        {
            split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(log2_cuw, 1);
            split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(log2_cuw, 1);
            split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
            split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));
        }
        else
        {
            if (log2_cuw > log2_cuh)
            {
                {
                    split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(log2_cuw, log2_cuw - log2_cuh + 1);

                    log2_sub_cuw = log2_cuw - 1;
                    log2_sub_cuh = log2_cuh;
                    long_side = log2_sub_cuw > log2_sub_cuh ? log2_sub_cuw : log2_sub_cuh;
                    ratio = XEVD_ABS(log2_sub_cuw - log2_sub_cuh);

                    split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(long_side, ratio);
                    if (from_boundary_b && (ratio == 3 || ratio == 4))
                    {
                        split_allow[SPLIT_BI_VER] = 1;
                    }

                    split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
                    split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));
                }
            }
            else
            {
                log2_sub_cuh = log2_cuh - 1;
                log2_sub_cuw = log2_cuw;
                long_side = log2_sub_cuw > log2_sub_cuh ? log2_sub_cuw : log2_sub_cuh;
                ratio = XEVD_ABS(log2_sub_cuw - log2_sub_cuh);

                split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(long_side, ratio);
                split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(log2_cuh, log2_cuh - log2_cuw + 1);
                split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
                split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));

            }
        }
    }

    if (boundary)
    {
        split_allow[NO_SPLIT] = 0;
        split_allow[SPLIT_TRI_VER] = 0;
        split_allow[SPLIT_TRI_HOR] = 0;
        split_allow[SPLIT_QUAD] = 0;
        if (boundary_r)
        {
            if (split_allow[SPLIT_BI_VER])
            {
                split_allow[SPLIT_BI_HOR] = 0;
            }
            else
            {
                split_allow[SPLIT_BI_HOR] = 1;
            }
        }
        else
        {
            if (split_allow[SPLIT_BI_HOR])
            {
                split_allow[SPLIT_BI_VER] = 0;
            }
            else
            {
                split_allow[SPLIT_BI_VER] = 1;
            }
        }
    }

    if (mode_cons == eOnlyInter)
    {
        int cuw = 1 << log2_cuw;
        int cuh = 1 << log2_cuh;
        for (int mode = SPLIT_BI_VER; mode < SPLIT_QUAD; ++mode)
            split_allow[mode] &= xevdm_get_mode_cons_by_split(mode, cuw, cuh) == eAll;
    }
}

int xevdm_get_suco_flag(s8* suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (* suco_flag_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int ret = XEVD_OK;
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVD_CONV_LOG2(cuw) - XEVD_CONV_LOG2(cuh));

    *suco_flag = (*suco_flag_buf)[cud][shape][pos];

    return ret;
}

void xevdm_set_suco_flag(s8  suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*suco_flag_buf)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVD_CONV_LOG2(cuw) - XEVD_CONV_LOG2(cuh));

    (*suco_flag_buf)[cud][shape][pos] = suco_flag;

}

u8 xevdm_check_suco_cond(int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, u8 suco_max_depth, u8 suco_depth, u8 log2_min_cu_size)
{
    int suco_log2_maxsize = XEVD_MIN((log2_max_cuwh - suco_max_depth), 6);
    int suco_log2_minsize = XEVD_MAX((suco_log2_maxsize - suco_depth), XEVD_MAX(4, log2_min_cu_size));
    if (XEVD_MIN(cuw, cuh) < (1 << suco_log2_minsize) || XEVD_MAX(cuw, cuh) > (1 << suco_log2_maxsize))
    {
        return 0;
    }

    if (boundary)
    {
        return 0;
    }

    if (split_mode == NO_SPLIT || split_mode == SPLIT_BI_HOR || split_mode == SPLIT_TRI_HOR)
    {
        return 0;
    }

    if (split_mode != SPLIT_QUAD && cuw <= cuh)
    {
        return 0;
    }

    return 1;
}

void xevdm_get_ctx_some_flags(int x_scu, int y_scu, int cuw, int cuh, int w_scu, u32* map_scu, u8* cod_eco, u32* map_cu_mode, u8* ctx, u8 slice_type
    , int sps_cm_init_flag, u8 ibc_flag, u8 ibc_log_max_size, u8* map_tidx, int eco_flag)
{
    int nev_info[NUM_CNID][3];
    int scun[3], avail[3];
    int scup = y_scu * w_scu + x_scu;
    int scuw = cuw >> MIN_CU_LOG2, scuh = cuh >> MIN_CU_LOG2;
    int num_pos_avail;
    int i, j;

    if ((slice_type == SLICE_I && ibc_flag == 0) || (slice_type == SLICE_I && (cuw > (1 << ibc_log_max_size) || cuh > (1 << ibc_log_max_size))))
    {
        return;
    }

    for (i = 0; i < NUM_CNID; i++)
    {
        nev_info[i][0] = nev_info[i][1] = nev_info[i][2] = 0;
        ctx[i] = 0;
    }

    scun[0] = scup - w_scu;
    scun[1] = scup - 1 + (scuh - 1)*w_scu;
    scun[2] = scup + scuw + (scuh - 1)*w_scu;
    if(eco_flag){
        avail[0] = y_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[0]]) && cod_eco[scun[0]]);
        avail[1] = x_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[1]]) && cod_eco[scun[1]]);
        avail[2] = x_scu + scuw >= w_scu ? 0 : ((map_tidx[scup] == map_tidx[scun[2]]) && cod_eco[scun[2]]);
    }
    else
    {
    avail[0] = y_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[0]]) && MCU_GET_COD(map_scu[scun[0]]));
    avail[1] = x_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[1]]) && MCU_GET_COD(map_scu[scun[1]]));
    avail[2] = x_scu + scuw >= w_scu ? 0 : ((map_tidx[scup] == map_tidx[scun[2]]) && MCU_GET_COD(map_scu[scun[2]]));
    }
    num_pos_avail = 0;

    for (j = 0; j < 3; j++)
    {
        if (avail[j])
        {
            nev_info[CNID_SKIP_FLAG][j] = MCU_GET_SF(map_scu[scun[j]]);
            nev_info[CNID_PRED_MODE][j] = MCU_GET_IF(map_scu[scun[j]]);

            if (slice_type != SLICE_I)
            {
                nev_info[CNID_AFFN_FLAG][j] = MCU_GET_AFF(map_scu[scun[j]]);
            }

            if (ibc_flag == 1)
            {
                nev_info[CNID_IBC_FLAG][j] = MCU_GET_IBC(map_scu[scun[j]]);
            }

            num_pos_avail++;
        }
    }

    //decide ctx
    for (i = 0; i < NUM_CNID; i++)
    {
        if (num_pos_avail == 0)
        {
            ctx[i] = 0;
        }
        else
        {
            ctx[i] = nev_info[i][0] + nev_info[i][1] + nev_info[i][2];

            if (i == CNID_SKIP_FLAG)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVD_MIN(ctx[i], NUM_CTX_SKIP_FLAG - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_IBC_FLAG)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVD_MIN(ctx[i], NUM_CTX_IBC_FLAG - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_PRED_MODE)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVD_MIN(ctx[i], NUM_CTX_PRED_MODE - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_MODE_CONS)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVD_MIN(ctx[i], NUM_CTX_MODE_CONS - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_AFFN_FLAG)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVD_MIN(ctx[i], NUM_CTX_AFFINE_FLAG - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
        }
    }
}

void xevdm_mv_rounding_s32(s32 hor, int ver, s32 * rounded_hor, s32 * rounded_ver, s32 right_shift, int left_shift)
{
    int offset = (right_shift > 0) ? (1 << (right_shift - 1)) : 0;
    *rounded_hor = ((hor + offset - (hor >= 0)) >> right_shift) << left_shift;
    *rounded_ver = ((ver + offset - (ver >= 0)) >> right_shift) << left_shift;
}

void xevdm_rounding_s32(s32 comp, s32 *rounded_comp, int right_shift, int left_shift)
{
    int offset = (right_shift > 0) ? (1 << (right_shift - 1)) : 0;
    *rounded_comp = ((comp + offset - (comp >= 0)) >> right_shift) << left_shift;
}

void xevdm_derive_affine_subblock_size_bi(s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL* mem_band_conditions_for_eif_are_satisfied)
{
    int w = cuw;
    int h = cuh;
#if MC_PRECISION_ADD
    int mc_prec_add = MC_PRECISION_ADD;
#else
    int mc_prec_add = 0;
#endif
    int mv_wx, mv_wy;
    int l = 0;

    *sub_w = cuw;
    *sub_h = cuh;

    for (l = 0; l < REFP_NUM; l++)
    {
        if (REFI_IS_VALID(refi[l]))
        {
            int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;

            // convert to 2^(storeBit + bit) precision
            dmv_hor_x = ((ac_mv[l][1][MV_X] - ac_mv[l][0][MV_X]) << 7) >> xevd_tbl_log2[cuw];     // deltaMvHor
            dmv_hor_y = ((ac_mv[l][1][MV_Y] - ac_mv[l][0][MV_Y]) << 7) >> xevd_tbl_log2[cuw];
            if (vertex_num == 3)
            {
                dmv_ver_x = ((ac_mv[l][2][MV_X] - ac_mv[l][0][MV_X]) << 7) >> xevd_tbl_log2[cuh]; // deltaMvVer
                dmv_ver_y = ((ac_mv[l][2][MV_Y] - ac_mv[l][0][MV_Y]) << 7) >> xevd_tbl_log2[cuh];
            }
            else
            {
                dmv_ver_x = -dmv_hor_y;                                                    // deltaMvVer
                dmv_ver_y = dmv_hor_x;
            }

            mv_wx = XEVD_MAX(abs(dmv_hor_x), abs(dmv_hor_y)), mv_wy = XEVD_MAX(abs(dmv_ver_x), abs(dmv_ver_y));
            int sub_lut[4] = { 32, 16, 8, 8 };
            if (mv_wx > 4)
            {
                w = 4;
            }
            else if (mv_wx == 0)
            {
                w = cuw;
            }
            else
            {
                w = sub_lut[mv_wx - 1];
            }

            if (mv_wy > 4)
            {
                h = 4;
            }
            else if (mv_wy == 0)
            {
                h = cuh;
            }
            else
            {
                h = sub_lut[mv_wy - 1];
            }

            *sub_w = XEVD_MIN(*sub_w, w);
            *sub_h = XEVD_MIN(*sub_h, h);
        }
    }

    int apply_eif = xevdm_check_eif_applicability_bi(ac_mv, refi, cuw, cuh, vertex_num, mem_band_conditions_for_eif_are_satisfied);

    if (!apply_eif)
    {
        *sub_w = XEVD_MAX(*sub_w, AFFINE_ADAPT_EIF_SIZE);
        *sub_h = XEVD_MAX(*sub_h, AFFINE_ADAPT_EIF_SIZE);
    }
}

void xevdm_derive_affine_subblock_size(s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL* mem_band_conditions_for_eif_are_satisfied)
{
    int w = cuw;
    int h = cuh;
#if MC_PRECISION_ADD
    int mc_prec_add = MC_PRECISION_ADD;
#else
    int mc_prec_add = 0;
#endif
    int mv_wx, mv_wy;

    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;

    // convert to 2^(storeBit + bit) precision
    dmv_hor_x = ((ac_mv[1][MV_X] - ac_mv[0][MV_X]) << 7) >> xevd_tbl_log2[cuw];     // deltaMvHor
    dmv_hor_y = ((ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << 7) >> xevd_tbl_log2[cuw];
    if (vertex_num == 3)
    {
        dmv_ver_x = ((ac_mv[2][MV_X] - ac_mv[0][MV_X]) << 7) >> xevd_tbl_log2[cuh]; // deltaMvVer
        dmv_ver_y = ((ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << 7) >> xevd_tbl_log2[cuh];
    }
    else
    {
        dmv_ver_x = -dmv_hor_y;                                                    // deltaMvVer
        dmv_ver_y = dmv_hor_x;
    }

    mv_wx = XEVD_MAX(abs(dmv_hor_x), abs(dmv_hor_y)), mv_wy = XEVD_MAX(abs(dmv_ver_x), abs(dmv_ver_y));
    int sub_lut[4] = { 32, 16, 8, 8 };
    if (mv_wx > 4)
    {
        w = 4;
    }
    else if (mv_wx == 0)
    {
        w = cuw;
    }
    else
    {
        w = sub_lut[mv_wx - 1];
    }

    if (mv_wy > 4)
    {
        h = 4;
    }
    else if (mv_wy == 0)
    {
        h = cuh;
    }
    else
    {
        h = sub_lut[mv_wy - 1];
    }

    *sub_w = w;
    *sub_h = h;

    int apply_eif = xevdm_check_eif_applicability_uni(ac_mv, cuw, cuh, vertex_num, mem_band_conditions_for_eif_are_satisfied);

    if (!apply_eif)
    {
        *sub_w = XEVD_MAX(*sub_w, AFFINE_ADAPT_EIF_SIZE);
        *sub_h = XEVD_MAX(*sub_h, AFFINE_ADAPT_EIF_SIZE);
    }
}

static void calculate_affine_motion_model_parameters(s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, int d_hor[MV_D], int d_ver[MV_D], int mv_additional_precision)
{
    assert(MV_X == 0 && MV_Y == 1);
    assert(vertex_num == 2 || vertex_num == 3);

    // convert to 2^(storeBit + bit) precision

    for (int comp = MV_X; comp < MV_D; ++comp)
        d_hor[comp] = ((ac_mv[1][comp] - ac_mv[0][comp]) << mv_additional_precision) >> xevd_tbl_log2[cuw];

    for (int comp = MV_X; comp < MV_D; ++comp)
    {
        if (vertex_num == 3)
            d_ver[comp] = ((ac_mv[2][comp] - ac_mv[0][comp]) << mv_additional_precision) >> xevd_tbl_log2[cuh]; // deltaMvVer
        else
            d_ver[comp] = comp == MV_X ? -d_hor[1 - comp] : d_hor[1 - comp];
    }
}

static void calculate_bounding_box_size(int w, int h, s16 ac_mv[VER_NUM][MV_D], int d_hor[MV_D], int d_ver[MV_D], int mv_precision, int *b_box_w, int *b_box_h)
{
    int corners[MV_D][4] = { {0}, };

    corners[MV_X][0] = 0;
    corners[MV_X][1] = corners[MV_X][0] + (w + 1) * (d_hor[MV_X] + (1 << mv_precision));
    corners[MV_X][2] = corners[MV_X][0] + (h + 1) * d_ver[MV_X];
    corners[MV_X][3] = corners[MV_X][1] + corners[MV_X][2] - corners[MV_X][0];

    corners[MV_Y][0] = 0;
    corners[MV_Y][1] = corners[MV_Y][0] + (w + 1) * d_hor[MV_Y];
    corners[MV_Y][2] = corners[MV_Y][0] + (h + 1) * (d_ver[MV_Y] + (1 << mv_precision));
    corners[MV_Y][3] = corners[MV_Y][1] + corners[MV_Y][2] - corners[MV_Y][0];

    int max[MV_D] = { 0, }, min[MV_D] = { 0, }, diff[MV_D] = { 0, };

    for (int coord = MV_X; coord < MV_D; ++coord)
    {
        max[coord] = XEVD_MAX(XEVD_MAX(corners[coord][0], corners[coord][1]), XEVD_MAX(corners[coord][2], corners[coord][3]));

        min[coord] = XEVD_MIN(XEVD_MIN(corners[coord][0], corners[coord][1]), XEVD_MIN(corners[coord][2], corners[coord][3]));

        diff[coord] = (max[coord] - min[coord] + (1 << mv_precision) - 1) >> mv_precision; //ceil
    }

    *b_box_w = diff[MV_X] + 1 + 1;
    *b_box_h = diff[MV_Y] + 1 + 1;
}

static BOOL check_eif_num_fetched_lines_restrictions(s16 ac_mv[VER_NUM][MV_D], int d_hor[MV_D], int d_ver[MV_D], int mv_precision)
{
    if (d_ver[MV_Y] < -1 << mv_precision)
        return FALSE;

    if ((XEVD_MAX(0, d_ver[MV_Y]) + abs(d_hor[MV_Y])) * (1 + EIF_SUBBLOCK_SIZE) > (EIF_NUM_ALLOWED_FETCHED_LINES_FOR_THE_FIRST_LINE - 2) << mv_precision)
        return FALSE;

    return TRUE;
}

BOOL xevdm_check_eif_applicability_uni(s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied)
{
    assert(mem_band_conditions_are_satisfied);

    int mv_additional_precision = MAX_CU_LOG2;
    int mv_precision = 2 + mv_additional_precision;

    int d_hor[MV_D] = { 0, 0 }, d_ver[MV_D] = { 0, 0 };

    calculate_affine_motion_model_parameters(ac_mv, cuw, cuh, vertex_num, d_hor, d_ver, mv_additional_precision);

    *mem_band_conditions_are_satisfied = FALSE;

    int bounding_box_w = 0, bounding_box_h = 0;
    calculate_bounding_box_size(EIF_SUBBLOCK_SIZE, EIF_SUBBLOCK_SIZE, ac_mv, d_hor, d_ver, mv_precision, &bounding_box_w, &bounding_box_h);

    *mem_band_conditions_are_satisfied = bounding_box_w * bounding_box_h <= MAX_MEMORY_ACCESS_BI;

    if (!check_eif_num_fetched_lines_restrictions(ac_mv, d_hor, d_ver, mv_precision))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL xevdm_check_eif_applicability_bi(s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied)
{
    if (mem_band_conditions_are_satisfied)
    {
        *mem_band_conditions_are_satisfied = TRUE;
    }

    int mv_additional_precision = MAX_CU_LOG2;
    int mv_precision = 2 + mv_additional_precision;

    for (int lidx = 0; lidx <= PRED_L1; lidx++)
    {
        if (REFI_IS_VALID(refi[lidx]))
        {
            BOOL mem_band_conditions_are_satisfied_lx = FALSE;
            BOOL is_eif_applicable = xevdm_check_eif_applicability_uni(ac_mv[lidx], cuw, cuh, vertex_num, &mem_band_conditions_are_satisfied_lx);

            if (mem_band_conditions_are_satisfied)
                *mem_band_conditions_are_satisfied &= mem_band_conditions_are_satisfied_lx;

            if (!is_eif_applicable)
                return FALSE;
        }
    }

    return TRUE;
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

int xevdm_derive_affine_constructed_candidate(int poc, XEVD_REFP(*refp)[REFP_NUM], int cuw, int cuh, int cp_valid[VER_NUM], s16 cp_mv[REFP_NUM][VER_NUM][MV_D], int cp_refi[REFP_NUM][VER_NUM], int cp_idx[VER_NUM], int model_idx, int ver_num, s16 mrg_list_cp_mv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D], s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], int *mrg_idx, int mrg_list_cp_num[AFF_MAX_CAND])
{
    int lidx, i;
    int valid_model[2] = { 0, 0 };
    s32 cpmv_tmp[REFP_NUM][VER_NUM][MV_D];
    int tmp_hor, tmp_ver;
    int shiftHtoW = 7 + xevd_tbl_log2[cuw] - xevd_tbl_log2[cuh]; // x * cuWidth / cuHeight

                                                               // early terminate
    if (*mrg_idx >= AFF_MAX_CAND)
    {
        return 0;
    }

    // check valid model and decide ref idx
    if (ver_num == 2)
    {
        int idx0 = cp_idx[0], idx1 = cp_idx[1];

        if (!cp_valid[idx0] || !cp_valid[idx1])
        {
            return 0;
        }

        for (lidx = 0; lidx < REFP_NUM; lidx++)
        {
            if (REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1])
            {
                valid_model[lidx] = 1;
            }
        }
    }
    else if (ver_num == 3)
    {
        int idx0 = cp_idx[0], idx1 = cp_idx[1], idx2 = cp_idx[2];

        if (!cp_valid[idx0] || !cp_valid[idx1] || !cp_valid[idx2])
        {
            return 0;
        }

        for (lidx = 0; lidx < REFP_NUM; lidx++)
        {
            if (REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && REFI_IS_VALID(cp_refi[lidx][idx2]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1] && cp_refi[lidx][idx0] == cp_refi[lidx][idx2])
            {
                valid_model[lidx] = 1;
            }
        }
    }
    else
    {
        xevd_assert(0);
    }

    // set merge index and vertex num for valid model
    if (valid_model[0] || valid_model[1])
    {
        mrg_list_cp_num[*mrg_idx] = ver_num;
    }
    else
    {
        return 0;
    }

    for (lidx = 0; lidx < REFP_NUM; lidx++)
    {
        if (valid_model[lidx])
        {
            mrg_list_refi[*mrg_idx][lidx] = cp_refi[lidx][cp_idx[0]];
            for (i = 0; i < ver_num; i++)
            {
                cpmv_tmp[lidx][cp_idx[i]][MV_X] = (s32)cp_mv[lidx][cp_idx[i]][MV_X];
                cpmv_tmp[lidx][cp_idx[i]][MV_Y] = (s32)cp_mv[lidx][cp_idx[i]][MV_Y];
            }

            // convert to LT, RT[, [LB], [RB]]
            switch (model_idx)
            {
            case 0: // 0 : LT, RT, LB
                break;
            case 1: // 1 : LT, RT, RB
                cpmv_tmp[lidx][2][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][1][MV_X];
                cpmv_tmp[lidx][2][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][1][MV_Y];
                break;
            case 2: // 1 : LT, LB, RB
                cpmv_tmp[lidx][1][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][2][MV_X];
                cpmv_tmp[lidx][1][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][2][MV_Y];
                break;
            case 3: // 4 : RT, LB, RB
                cpmv_tmp[lidx][0][MV_X] = cpmv_tmp[lidx][1][MV_X] + cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][3][MV_X];
                cpmv_tmp[lidx][0][MV_Y] = cpmv_tmp[lidx][1][MV_Y] + cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][3][MV_Y];
                break;
            case 4: // 5 : LT, RT
                break;
            case 5: // 6 : LT, LB
                tmp_hor = +((cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][0][MV_Y]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_X] << 7);
                tmp_ver = -((cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][0][MV_X]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_Y] << 7);
                xevdm_mv_rounding_s32(tmp_hor, tmp_ver, &cpmv_tmp[lidx][1][MV_X], &cpmv_tmp[lidx][1][MV_Y], 7, 0);
                break;
            default:
                xevd_assert(0);
            }

            for (i = 0; i < ver_num; i++)
            {
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_X] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, cpmv_tmp[lidx][i][MV_X]);
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_Y] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, cpmv_tmp[lidx][i][MV_Y]);
            }
        }
        else
        {
            mrg_list_refi[*mrg_idx][lidx] = -1;
            for (i = 0; i < ver_num; i++)
            {
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_X] = 0;
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_Y] = 0;
            }
        }
    }

    (*mrg_idx)++;

    return 1;
}

void xevdm_derive_affine_model_mv(int scup, int scun, int lidx, s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, s16 mvp[VER_NUM][MV_D], u32 *map_affine, int cur_cp_num, int log2_max_cuwh
    , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D])
{
    s16 neb_mv[VER_NUM][MV_D] = { { 0, }, };
    int i;
    int neb_addr[VER_NUM];
    int neb_log_w = MCU_GET_AFF_LOGW(map_affine[scun]);
    int neb_log_h = MCU_GET_AFF_LOGH(map_affine[scun]);
    int neb_w = 1 << neb_log_w;
    int neb_h = 1 << neb_log_h;
    int neb_x, neb_y;
    int cur_x, cur_y;
    int max_bit = 7;
    int diff_w = max_bit - neb_log_w;
    int diff_h = max_bit - neb_log_h;
    int dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y, hor_base, ver_base;
    s32 tmp_hor, tmp_ver;
    int neb_cp_num = (MCU_GET_AFF(map_scu[scun]) == 1) ? 2 : 3;

    neb_addr[0] = scun - MCU_GET_AFF_XOFF(map_affine[scun]) - w_scu * MCU_GET_AFF_YOFF(map_affine[scun]);
    neb_addr[1] = neb_addr[0] + ((neb_w >> MIN_CU_LOG2) - 1);
    neb_addr[2] = neb_addr[0] + ((neb_h >> MIN_CU_LOG2) - 1) * w_scu;
    neb_addr[3] = neb_addr[2] + ((neb_w >> MIN_CU_LOG2) - 1);

    neb_x = (neb_addr[0] % w_scu) << MIN_CU_LOG2;
    neb_y = (neb_addr[0] / w_scu) << MIN_CU_LOG2;
    cur_x = (scup % w_scu) << MIN_CU_LOG2;
    cur_y = (scup / w_scu) << MIN_CU_LOG2;

    for (i = 0; i < VER_NUM; i++)
    {
        if (MCU_GET_DMVRF(map_scu[neb_addr[i]]))
        {
            neb_mv[i][MV_X] = map_unrefined_mv[neb_addr[i]][lidx][MV_X];
            neb_mv[i][MV_Y] = map_unrefined_mv[neb_addr[i]][lidx][MV_Y];
        }
        else
        {
            neb_mv[i][MV_X] = map_mv[neb_addr[i]][lidx][MV_X];
            neb_mv[i][MV_Y] = map_mv[neb_addr[i]][lidx][MV_Y];
        }
    }

    int is_top_ctu_boundary = FALSE;
    if ((neb_y + neb_h) % (1 << log2_max_cuwh) == 0 && (neb_y + neb_h) == cur_y)
    {
        is_top_ctu_boundary = TRUE;
        neb_y += neb_h;

        neb_mv[0][MV_X] = neb_mv[2][MV_X];
        neb_mv[0][MV_Y] = neb_mv[2][MV_Y];
        neb_mv[1][MV_X] = neb_mv[3][MV_X];
        neb_mv[1][MV_Y] = neb_mv[3][MV_Y];
    }

    dmv_hor_x = (neb_mv[1][MV_X] - neb_mv[0][MV_X]) << diff_w;    // deltaMvHor
    dmv_hor_y = (neb_mv[1][MV_Y] - neb_mv[0][MV_Y]) << diff_w;

    if (cur_cp_num == 3 && !is_top_ctu_boundary)
    {
        dmv_ver_x = (neb_mv[2][MV_X] - neb_mv[0][MV_X]) << diff_h;  // deltaMvVer
        dmv_ver_y = (neb_mv[2][MV_Y] - neb_mv[0][MV_Y]) << diff_h;
    }
    else
    {
        dmv_ver_x = -dmv_hor_y;                                      // deltaMvVer
        dmv_ver_y = dmv_hor_x;
    }
    hor_base = neb_mv[0][MV_X] << max_bit;
    ver_base = neb_mv[0][MV_Y] << max_bit;

    // derive CPMV 0
    tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    xevdm_mv_rounding_s32(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0);
    mvp[0][MV_X] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_hor);
    mvp[0][MV_Y] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_ver);

    // derive CPMV 1
    tmp_hor = dmv_hor_x * (cur_x - neb_x + cuw) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x + cuw) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    xevdm_mv_rounding_s32(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0);
    mvp[1][MV_X] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_hor);
    mvp[1][MV_Y] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_ver);

    // derive CPMV 2
    if (cur_cp_num == 3)
    {
        tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y + cuh) + hor_base;
        tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y + cuh) + ver_base;
        xevdm_mv_rounding_s32(tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0);
        mvp[2][MV_X] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_hor);
        mvp[2][MV_Y] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, tmp_ver);
    }
}

/* inter affine mode */
void xevdm_get_affine_motion_scaling(int poc, int scup, int lidx, s8 cur_refi, int num_refp, \
    s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVD_REFP(*refp)[REFP_NUM], \
    int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAXM_NUM_MVP][VER_NUM][MV_D], s8 refi[MAXM_NUM_MVP]
    , u32* map_scu, u32* map_affine, int vertex_num, u16 avail_lr, int log2_max_cuwh, s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8* map_tidx)
{
    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int cnt_lt = 0, cnt_rt = 0, cnt_lb = 0;
    int i, j, k;
    s16 mvp_tmp[VER_NUM][MV_D];
    int neb_addr[3];
    int valid_flag[3];
    int cnt_tmp = 0;
    s16 mvp_cand_lt[AFFINE_MAX_NUM_LT][MV_D];
    int neb_addr_lt[AFFINE_MAX_NUM_LT];
    int valid_flag_lt[AFFINE_MAX_NUM_LT];
    s16 mvp_cand_rt[AFFINE_MAX_NUM_RT][MV_D];
    int neb_addr_rt[AFFINE_MAX_NUM_RT];
    int valid_flag_rt[AFFINE_MAX_NUM_RT];
    s16 mvp_cand_lb[AFFINE_MAX_NUM_LB][MV_D];
    int neb_addr_lb[AFFINE_MAX_NUM_LB];
    int valid_flag_lb[AFFINE_MAX_NUM_LB];
    int cnt_rb = 0;
    s16 mvp_cand_rb[AFFINE_MAX_NUM_RB][MV_D];
    int neb_addr_rb[AFFINE_MAX_NUM_RB];
    int valid_flag_rb[AFFINE_MAX_NUM_RB];
    //-------------------  INIT  -------------------//
#if INCREASE_MVP_NUM
    for (i = 0; i < ORG_MAX_NUM_MVP; i++)
#else
    for (i = 0; i < MAXM_NUM_MVP; i++)
#endif
    {
        for (j = 0; j < VER_NUM; j++)
        {
            mvp[i][j][MV_X] = 0;
            mvp[i][j][MV_Y] = 0;
        }
        refi[i] = 0;
    }

    //-------------------  Model based affine MVP  -------------------//

    // left inherited affine MVP, first of {A0, A1}
    neb_addr[0] = scup + w_scu * scuh - 1;       // A0
    neb_addr[1] = scup + w_scu * (scuh - 1) - 1; // A1
    valid_flag[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[1]]);

    for (k = 0; k < 2; k++)
    {
        if (valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
            && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xevdm_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);

            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if (cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    // above inherited affine MVP, first of {B0, B1, B2}
    neb_addr[0] = scup - w_scu + scuw;           // B0
    neb_addr[1] = scup - w_scu + scuw - 1;       // B1
    neb_addr[2] = scup - w_scu - 1;              // B2
    valid_flag[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[1]]);
    valid_flag[2] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[2]]);
    for (k = 0; k < 3; k++)
    {
        if (valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
            && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xevdm_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);

            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if (cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    // right inherited affine MVP, first of {C0, C1}
    neb_addr[0] = scup + w_scu * scuh + scuw;       // C0
    neb_addr[1] = scup + w_scu * (scuh - 1) + scuw; // C1
    valid_flag[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr[1]]);

    for (k = 0; k < 2; k++)
    {
        if (valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
            && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xevdm_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);

            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if (cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    //-------------------  LT  -------------------//
    for (i = 0; i < AFFINE_MAX_NUM_LT; i++)
    {
        mvp_cand_lt[i][MV_X] = 0;
        mvp_cand_lt[i][MV_Y] = 0;
    }

    neb_addr_lt[0] = scup - w_scu - 1;
    neb_addr_lt[1] = scup - w_scu;
    neb_addr_lt[2] = scup - 1;

    valid_flag_lt[0] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[0]]) && !MCU_GET_IF(map_scu[neb_addr_lt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_lt[0]]);
    valid_flag_lt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[1]]) && !MCU_GET_IF(map_scu[neb_addr_lt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_lt[1]]);
    valid_flag_lt[2] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[2]]) && !MCU_GET_IF(map_scu[neb_addr_lt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[2]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_lt[2]]);

    for (k = 0; k < AFFINE_MAX_NUM_LT; k++)
    {
        if (valid_flag_lt[k] && REFI_IS_VALID(map_refi[neb_addr_lt[k]][lidx]))
        {
            refi[cnt_lt] = map_refi[neb_addr_lt[k]][lidx];
            if (refi[cnt_lt] == cur_refi)
            {
                if (MCU_GET_DMVRF(map_scu[neb_addr_lt[k]]))
                {
                    mvp_cand_lt[cnt_lt][MV_X] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_X];
                    mvp_cand_lt[cnt_lt][MV_Y] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_lt[cnt_lt][MV_X] = map_mv[neb_addr_lt[k]][lidx][MV_X];
                    mvp_cand_lt[cnt_lt][MV_Y] = map_mv[neb_addr_lt[k]][lidx][MV_Y];
                }
                cnt_lt++;
                break;
            }
        }
    }

    //-------------------  RT  -------------------//
    for (i = 0; i < AFFINE_MAX_NUM_RT; i++)
    {
        mvp_cand_rt[i][MV_X] = 0;
        mvp_cand_rt[i][MV_Y] = 0;
    }

    neb_addr_rt[0] = scup - w_scu + scuw;
    neb_addr_rt[1] = scup - w_scu + scuw - 1;
    neb_addr_rt[2] = scup + scuw;

    valid_flag_rt[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[0]]) && !MCU_GET_IF(map_scu[neb_addr_rt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_rt[0]]);
    valid_flag_rt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_rt[1]]) && !MCU_GET_IF(map_scu[neb_addr_rt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_rt[1]]);
    valid_flag_rt[2] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[2]]) && !MCU_GET_IF(map_scu[neb_addr_rt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[2]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_rt[2]]);

    for (k = 0; k < AFFINE_MAX_NUM_RT; k++)
    {
        if (valid_flag_rt[k] && REFI_IS_VALID(map_refi[neb_addr_rt[k]][lidx]))
        {
            refi[cnt_rt] = map_refi[neb_addr_rt[k]][lidx];
            if (refi[cnt_rt] == cur_refi)
            {
                if (MCU_GET_DMVRF(map_scu[neb_addr_rt[k]]))
                {
                    mvp_cand_rt[cnt_rt][MV_X] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_X];
                    mvp_cand_rt[cnt_rt][MV_Y] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_rt[cnt_rt][MV_X] = map_mv[neb_addr_rt[k]][lidx][MV_X];
                    mvp_cand_rt[cnt_rt][MV_Y] = map_mv[neb_addr_rt[k]][lidx][MV_Y];
                }
                cnt_rt++;
                break;
            }
        }
    }

    //-------------------  LB  -------------------//
    for (i = 0; i < AFFINE_MAX_NUM_LB; i++)
    {
        mvp_cand_lb[i][MV_X] = 0;
        mvp_cand_lb[i][MV_Y] = 0;
    }

    neb_addr_lb[0] = scup + w_scu * scuh - 1;        // A0
    neb_addr_lb[1] = scup + w_scu * (scuh - 1) - 1;  // A1

    valid_flag_lb[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_lb[0]]) && !MCU_GET_IF(map_scu[neb_addr_lb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_lb[0]]);
    valid_flag_lb[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lb[1]]) && !MCU_GET_IF(map_scu[neb_addr_lb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_lb[1]]);

    for (k = 0; k < AFFINE_MAX_NUM_LB; k++)
    {
        if (valid_flag_lb[k] && REFI_IS_VALID(map_refi[neb_addr_lb[k]][lidx]))
        {
            refi[cnt_lb] = map_refi[neb_addr_lb[k]][lidx];
            if (refi[cnt_lb] == cur_refi)
            {
                if (MCU_GET_DMVRF(map_scu[neb_addr_lb[k]]))
                {
                    mvp_cand_lb[cnt_lb][MV_X] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_X];
                    mvp_cand_lb[cnt_lb][MV_Y] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_lb[cnt_lb][MV_X] = map_mv[neb_addr_lb[k]][lidx][MV_X];
                    mvp_cand_lb[cnt_lb][MV_Y] = map_mv[neb_addr_lb[k]][lidx][MV_Y];
                }
                cnt_lb++;
                break;
            }
        }
    }

    //-------------------  RB  -------------------//
    for (i = 0; i < AFFINE_MAX_NUM_RB; i++)
    {
        mvp_cand_rb[i][MV_X] = 0;
        mvp_cand_rb[i][MV_Y] = 0;
    }

    neb_addr_rb[0] = scup + w_scu * scuh + scuw;
    neb_addr_rb[1] = scup + w_scu * (scuh - 1) + scuw;

    valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_rb[0]]) && !MCU_GET_IF(map_scu[neb_addr_rb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[0]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);
    valid_flag_rb[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rb[1]]) && !MCU_GET_IF(map_scu[neb_addr_rb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[1]]) &&
        (map_tidx[scup] == map_tidx[neb_addr_rb[1]]);

    for (k = 0; k < AFFINE_MAX_NUM_RB; k++)
    {
        if (valid_flag_rb[k] && REFI_IS_VALID(map_refi[neb_addr_rb[k]][lidx]))
        {
            refi[cnt_rb] = map_refi[neb_addr_rb[k]][lidx];
            if (refi[cnt_rb] == cur_refi)
            {
                if (MCU_GET_DMVRF(map_scu[neb_addr_rb[k]]))
                {
                    mvp_cand_rb[cnt_rb][MV_X] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_X];
                    mvp_cand_rb[cnt_rb][MV_Y] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_rb[cnt_rb][MV_X] = map_mv[neb_addr_rb[k]][lidx][MV_X];
                    mvp_cand_rb[cnt_rb][MV_Y] = map_mv[neb_addr_rb[k]][lidx][MV_Y];
                }
                cnt_rb++;
                break;
            }
        }
    }

    //-------------------  organize  -------------------//
    {
        if (cnt_lt && cnt_rt && (vertex_num == 2 || (cnt_lb || cnt_rb)))
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lb[0][MV_Y];

            if (cnt_lb == 0 && cnt_rb > 0)
            {
                mvp[cnt_tmp][2][MV_X] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, mvp_cand_rb[0][MV_X] + mvp_cand_lt[0][MV_X] - mvp_cand_rt[0][MV_X]);
                mvp[cnt_tmp][2][MV_Y] = (s16)XEVD_CLIP3(XEVD_INT16_MIN, XEVD_INT16_MAX, mvp_cand_rb[0][MV_Y] + mvp_cand_lt[0][MV_Y] - mvp_cand_rt[0][MV_Y]);
            }
            cnt_tmp++;
        }
        if (cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, left
        if (cnt_lb)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lb[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_lb[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lb[0][MV_Y];
            cnt_tmp++;
        }

        // Add translation mv, right
        else if (cnt_rb)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_rb[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rb[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_rb[0][MV_Y];
            cnt_tmp++;
        }

        if (cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, above
        if (cnt_rt)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_rt[0][MV_Y];
            cnt_tmp++;
        }
        if (cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, above left
        if (cnt_lt)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lt[0][MV_Y];
            cnt_tmp++;
        }
        if (cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add zero MVP
        for (; cnt_tmp < AFF_MAX_NUM_MVP; cnt_tmp++)
        {
            mvp[cnt_tmp][0][MV_X] = 0;
            mvp[cnt_tmp][0][MV_Y] = 0;
            mvp[cnt_tmp][1][MV_X] = 0;
            mvp[cnt_tmp][1][MV_Y] = 0;
            mvp[cnt_tmp][2][MV_X] = 0;
            mvp[cnt_tmp][2][MV_Y] = 0;
            cnt_tmp++;
        }
    }
}

/* merge affine mode */
int xevdm_get_affine_merge_candidate(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVD_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu, u16 avail,
    s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], s16 mrg_list_cpmv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D], int mrg_list_cp_num[AFF_MAX_CAND], u32* map_scu, u32* map_affine, int log2_max_cuwh
    , s16(*map_unrefined_mv)[REFP_NUM][MV_D], u16 avail_lr, XEVD_SH * sh, u8* map_tidx)
{
    int lidx, i, k;
    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int cnt = 0;
    s16 tmvp[REFP_NUM][MV_D];
    s8  availablePredIdx = 0;
    //-------------------  Model based affine MVP  -------------------//
    {
        int neb_addr[5];
        int valid_flag[5];
        int top_left[7];

        if (avail_lr == LR_01)
        {
            neb_addr[0] = scup + w_scu * (scuh - 1) + scuw; // A1
            neb_addr[1] = scup - w_scu;                     // B1
            neb_addr[2] = scup - w_scu - 1;                 // B0
            neb_addr[3] = scup + w_scu * scuh + scuw;       // A0
            neb_addr[4] = scup - w_scu + scuw;              // B2

            valid_flag[0] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]);
            valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]);
            valid_flag[2] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]);
            valid_flag[3] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && MCU_GET_AFF(map_scu[neb_addr[3]]);
            valid_flag[4] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && MCU_GET_AFF(map_scu[neb_addr[4]]);
        }
        else
        {
            neb_addr[0] = scup + w_scu * (scuh - 1) - 1; // A1
            neb_addr[1] = scup - w_scu + scuw - 1;       // B1
            neb_addr[2] = scup - w_scu + scuw;           // B0
            neb_addr[3] = scup + w_scu * scuh - 1;       // A0
            neb_addr[4] = scup - w_scu - 1;              // B2

            valid_flag[0] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]);
            valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]);
            valid_flag[2] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]);
            valid_flag[3] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && MCU_GET_AFF(map_scu[neb_addr[3]]);
            valid_flag[4] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && MCU_GET_AFF(map_scu[neb_addr[4]]);
        }

        valid_flag[0] = valid_flag[0] && (map_tidx[scup] == map_tidx[neb_addr[0]]);
        valid_flag[1] = valid_flag[1] && (map_tidx[scup] == map_tidx[neb_addr[1]]);
        valid_flag[2] = valid_flag[2] && (map_tidx[scup] == map_tidx[neb_addr[2]]);
        valid_flag[3] = valid_flag[3] && (map_tidx[scup] == map_tidx[neb_addr[3]]);
        valid_flag[4] = valid_flag[4] && (map_tidx[scup] == map_tidx[neb_addr[4]]);

        for (k = 0; k < 5; k++)
        {
            if (valid_flag[k])
            {
                top_left[k] = neb_addr[k] - MCU_GET_AFF_XOFF(map_affine[neb_addr[k]]) - w_scu * MCU_GET_AFF_YOFF(map_affine[neb_addr[k]]);
            }
        }

        if (valid_flag[2] && valid_flag[1] && top_left[1] == top_left[2])
        {
            valid_flag[2] = 0;
        }

        if (valid_flag[3] && valid_flag[0] && top_left[0] == top_left[3])
        {
            valid_flag[3] = 0;
        }

        if ((valid_flag[4] && valid_flag[0] && top_left[4] == top_left[0]) || (valid_flag[4] && valid_flag[1] && top_left[4] == top_left[1]))
        {
            valid_flag[4] = 0;
        }

        for (k = 0; k < 5; k++)
        {
            if (valid_flag[k])
            {
                // set vertex number: affine flag == 1, set to 2 vertex, otherwise, set to 3 vertex
                mrg_list_cp_num[cnt] = (MCU_GET_AFF(map_scu[neb_addr[k]]) == 1) ? 2 : 3;

                for (lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    if (REFI_IS_VALID(map_refi[neb_addr[k]][lidx]))
                    {
                        mrg_list_refi[cnt][lidx] = map_refi[neb_addr[k]][lidx];
                        xevdm_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mrg_list_cpmv[cnt][lidx], map_affine, mrg_list_cp_num[cnt], log2_max_cuwh, map_scu, map_unrefined_mv);
                    }
                    else // set to default value
                    {
                        mrg_list_refi[cnt][lidx] = -1;
                        for (i = 0; i < VER_NUM; i++)
                        {
                            mrg_list_cpmv[cnt][lidx][i][MV_X] = 0;
                            mrg_list_cpmv[cnt][lidx][i][MV_Y] = 0;
                        }
                    }
                }
                cnt++;
            }

            if (cnt >= AFF_MODEL_CAND) // one candidate in current stage
            {
                break;
            }
        }
    }

    //-------------------  control point based affine MVP  -------------------//
    {
        s16 cp_mv[REFP_NUM][VER_NUM][MV_D];
        int cp_refi[REFP_NUM][VER_NUM];
        int cp_valid[VER_NUM];

        int neb_addr_lt[AFFINE_MAX_NUM_LT];
        int neb_addr_rt[AFFINE_MAX_NUM_RT];
        int neb_addr_lb[AFFINE_MAX_NUM_LB];
        int neb_addr_rb[AFFINE_MAX_NUM_RB];

        int valid_flag_lt[AFFINE_MAX_NUM_LT];
        int valid_flag_rt[AFFINE_MAX_NUM_RT];
        int valid_flag_lb[AFFINE_MAX_NUM_LB];
        int valid_flag_rb[AFFINE_MAX_NUM_RB];

        //------------------  INIT  ------------------//
        for (i = 0; i < VER_NUM; i++)
        {
            for (lidx = 0; lidx < REFP_NUM; lidx++)
            {
                cp_mv[lidx][i][MV_X] = 0;
                cp_mv[lidx][i][MV_Y] = 0;
                cp_refi[lidx][i] = -1;
            }
            cp_valid[i] = 0;
        }

        //-------------------  LT  -------------------//
        neb_addr_lt[0] = scup - w_scu - 1;
        neb_addr_lt[1] = scup - w_scu;
        neb_addr_lt[2] = scup - 1;

        valid_flag_lt[0] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[0]]) && !MCU_GET_IF(map_scu[neb_addr_lt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[0]]);
        valid_flag_lt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[1]]) && !MCU_GET_IF(map_scu[neb_addr_lt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[1]]);
        valid_flag_lt[2] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[2]]) && !MCU_GET_IF(map_scu[neb_addr_lt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[2]]);

        valid_flag_lt[0] = valid_flag_lt[0] && (map_tidx[scup] == map_tidx[neb_addr_lt[0]]);
        valid_flag_lt[1] = valid_flag_lt[1] && (map_tidx[scup] == map_tidx[neb_addr_lt[1]]);
        valid_flag_lt[2] = valid_flag_lt[2] && (map_tidx[scup] == map_tidx[neb_addr_lt[2]]);

        for (k = 0; k < AFFINE_MAX_NUM_LT; k++)
        {
            if (valid_flag_lt[k])
            {
                for (lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    cp_refi[lidx][0] = map_refi[neb_addr_lt[k]][lidx];
                    if (MCU_GET_DMVRF(map_scu[neb_addr_lt[k]]))
                    {
                        cp_mv[lidx][0][MV_X] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_X];
                        cp_mv[lidx][0][MV_Y] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_Y];
                    }
                    else
                    {
                        cp_mv[lidx][0][MV_X] = map_mv[neb_addr_lt[k]][lidx][MV_X];
                        cp_mv[lidx][0][MV_Y] = map_mv[neb_addr_lt[k]][lidx][MV_Y];
                    }
                }
                cp_valid[0] = 1;
                break;
            }
        }

        //-------------------  RT  -------------------//
        neb_addr_rt[0] = scup - w_scu + scuw;
        neb_addr_rt[1] = scup - w_scu + scuw - 1;

        valid_flag_rt[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[0]]) && !MCU_GET_IF(map_scu[neb_addr_rt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[0]]);
        valid_flag_rt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_rt[1]]) && !MCU_GET_IF(map_scu[neb_addr_rt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[1]]);

        neb_addr_rt[2] = scup + scuw;                 // RIGHT
        valid_flag_rt[2] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[2]]) && !MCU_GET_IF(map_scu[neb_addr_rt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[2]]);

        valid_flag_rt[0] = valid_flag_rt[0] && (map_tidx[scup] == map_tidx[neb_addr_rt[0]]);
        valid_flag_rt[1] = valid_flag_rt[1] && (map_tidx[scup] == map_tidx[neb_addr_rt[1]]);
        valid_flag_rt[2] = valid_flag_rt[2] && (map_tidx[scup] == map_tidx[neb_addr_rt[2]]);

        for (k = 0; k < AFFINE_MAX_NUM_RT; k++)
        {
            if (valid_flag_rt[k])
            {
                for (lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    cp_refi[lidx][1] = map_refi[neb_addr_rt[k]][lidx];
                    if (MCU_GET_DMVRF(map_scu[neb_addr_rt[k]]))
                    {
                        cp_mv[lidx][1][MV_X] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_X];
                        cp_mv[lidx][1][MV_Y] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_Y];
                    }
                    else
                    {
                        cp_mv[lidx][1][MV_X] = map_mv[neb_addr_rt[k]][lidx][MV_X];
                        cp_mv[lidx][1][MV_Y] = map_mv[neb_addr_rt[k]][lidx][MV_Y];
                    }
                }
                cp_valid[1] = 1;
                break;
            }
        }

        //-------------------  LB  -------------------//
        if (avail_lr == LR_10 || avail_lr == LR_11)
        {
            neb_addr_lb[0] = scup + w_scu * scuh - 1;        // A0
            neb_addr_lb[1] = scup + w_scu * (scuh - 1) - 1;  // A1

            valid_flag_lb[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_lb[0]]) && !MCU_GET_IF(map_scu[neb_addr_lb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[0]]);
            valid_flag_lb[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lb[1]]) && !MCU_GET_IF(map_scu[neb_addr_lb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[1]]);

            valid_flag_lb[0] = valid_flag_lb[0] && (map_tidx[scup] == map_tidx[neb_addr_lb[0]]);
            valid_flag_lb[1] = valid_flag_lb[1] && (map_tidx[scup] == map_tidx[neb_addr_lb[1]]);

            for (k = 0; k < AFFINE_MAX_NUM_LB; k++)
            {
                if (valid_flag_lb[k])
                {
                    for (lidx = 0; lidx < REFP_NUM; lidx++)
                    {
                        cp_refi[lidx][2] = map_refi[neb_addr_lb[k]][lidx];
                        if (MCU_GET_DMVRF(map_scu[neb_addr_lb[k]]))
                        {
                            cp_mv[lidx][2][MV_X] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_X];
                            cp_mv[lidx][2][MV_Y] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_Y];
                        }
                        else
                        {
                            cp_mv[lidx][2][MV_X] = map_mv[neb_addr_lb[k]][lidx][MV_X];
                            cp_mv[lidx][2][MV_Y] = map_mv[neb_addr_lb[k]][lidx][MV_Y];
                        }
                    }
                    cp_valid[2] = 1;
                    break;
                }
            }
        }
        else
        {
            neb_addr_lb[0] = scup + w_scu * scuh - 1;

            s32 SameCtuRow = ((y_scu + scuh) << MIN_CU_LOG2 >> log2_max_cuwh) == (y_scu << MIN_CU_LOG2 >> log2_max_cuwh);
            valid_flag_lb[0] = x_scu > 0 && (y_scu + scuh < h_scu) && SameCtuRow;


            valid_flag_lb[0] = valid_flag_lb[0] && (map_tidx[scup] == map_tidx[neb_addr_lb[0]]) && (map_tidx[scup] == map_tidx[scup - 1]);
            if (valid_flag_lb[0])
            {

                neb_addr_lb[0] = ((x_scu - 1) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid

                xevdm_get_mv_collocated(refp, poc, neb_addr_lb[0], scup, w_scu, h_scu, tmvp, &availablePredIdx, sh);

                if ((availablePredIdx == 1) || (availablePredIdx == 3))
                {
                    cp_refi[REFP_0][2] = 0;
                    cp_mv[REFP_0][2][MV_X] = tmvp[REFP_0][MV_X];
                    cp_mv[REFP_0][2][MV_Y] = tmvp[REFP_0][MV_Y];
                }
                else
                {
                    cp_refi[0][2] = REFI_INVALID;
                    cp_mv[REFP_0][2][MV_X] = 0;
                    cp_mv[REFP_0][2][MV_Y] = 0;
                }

                if (((availablePredIdx == 2) || (availablePredIdx == 3)) && slice_type == SLICE_B)

                {
                    cp_refi[REFP_1][2] = 0;
                    cp_mv[REFP_1][2][MV_X] = tmvp[REFP_1][MV_X];
                    cp_mv[REFP_1][2][MV_Y] = tmvp[REFP_1][MV_Y];
                }
                else
                {
                    cp_refi[REFP_1][2] = REFI_INVALID;
                    cp_mv[REFP_1][2][MV_X] = 0;
                    cp_mv[REFP_1][2][MV_Y] = 0;
                }
            }
            if (REFI_IS_VALID(cp_refi[REFP_0][2]) || REFI_IS_VALID(cp_refi[REFP_1][2]))
            {
                cp_valid[2] = 1;
            }
        }

        //-------------------  RB  -------------------//
        if (avail_lr == LR_01 || avail_lr == LR_11)
        {
            neb_addr_rb[0] = scup + w_scu * scuh + scuw;
            valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_rb[0]]) && !MCU_GET_IF(map_scu[neb_addr_rb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[0]]);

            neb_addr_rb[1] = scup + w_scu * (scuh - 1) + scuw;
            valid_flag_rb[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rb[1]]) && !MCU_GET_IF(map_scu[neb_addr_rb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[1]]);

            valid_flag_rb[0] = valid_flag_rb[0] && (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);
            valid_flag_rb[1] = valid_flag_rb[1] && (map_tidx[scup] == map_tidx[neb_addr_rb[1]]);

            for (k = 0; k < AFFINE_MAX_NUM_RB; k++)
            {
                if (valid_flag_rb[k])
                {
                    for (lidx = 0; lidx < REFP_NUM; lidx++)
                    {

                        cp_refi[lidx][3] = map_refi[neb_addr_rb[k]][lidx];

                        if (MCU_GET_DMVRF(map_scu[neb_addr_rb[k]]))
                        {
                            cp_mv[lidx][3][MV_X] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_X];
                            cp_mv[lidx][3][MV_Y] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_Y];
                        }
                        else
                        {
                            cp_mv[lidx][3][MV_X] = map_mv[neb_addr_rb[k]][lidx][MV_X];
                            cp_mv[lidx][3][MV_Y] = map_mv[neb_addr_rb[k]][lidx][MV_Y];
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            s32 isSameCtuLine = ((y_scu + scuh) << MIN_CU_LOG2 >> log2_max_cuwh) == (y_scu << MIN_CU_LOG2 >> log2_max_cuwh);
            valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && isSameCtuLine;

            neb_addr_rb[0] = ((x_scu + scuw) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid
            valid_flag_rb[0] = valid_flag_rb[0] && (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);

            if (valid_flag_rb[0])
            {
                s16 tmvp[REFP_NUM][MV_D];
                s8 availablePredIdx = 0;

                neb_addr_rb[0] = ((x_scu + scuw) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid
                xevdm_get_mv_collocated(refp, poc, neb_addr_rb[0], scup, w_scu, h_scu, tmvp, &availablePredIdx, sh);

                if ((availablePredIdx == 1) || (availablePredIdx == 3))
                {
                    cp_refi[0][3] = 0;
                    cp_mv[0][3][MV_X] = tmvp[REFP_0][MV_X];
                    cp_mv[0][3][MV_Y] = tmvp[REFP_0][MV_Y];
                }
                else
                {
                    cp_refi[0][3] = REFI_INVALID;
                    cp_mv[0][3][MV_X] = 0;
                    cp_mv[0][3][MV_Y] = 0;
                }

                if (((availablePredIdx == 2) || (availablePredIdx == 3)) && slice_type == SLICE_B)
                {
                    cp_refi[1][3] = 0;
                    cp_mv[1][3][MV_X] = tmvp[REFP_1][MV_X];
                    cp_mv[1][3][MV_Y] = tmvp[REFP_1][MV_Y];
                }
                else
                {
                    cp_refi[1][3] = REFI_INVALID;
                    cp_mv[1][3][MV_X] = 0;
                    cp_mv[1][3][MV_Y] = 0;
                }
            }
        }

        if (REFI_IS_VALID(cp_refi[REFP_0][3]) || REFI_IS_VALID(cp_refi[REFP_1][3]))
        {
            cp_valid[3] = 1;
        }

        //-------------------  insert model  -------------------//
        int const_order[6] = { 0, 1, 2, 3, 4, 5 };
        int const_num = 6;

        int idx = 0;
        int const_model[6][VER_NUM] =
        {
            { 0, 1, 2 },          // 0: LT, RT, LB
            { 0, 1, 3 },          // 1: LT, RT, RB
            { 0, 2, 3 },          // 2: LT, LB, RB
            { 1, 2, 3 },          // 3: RT, LB, RB
            { 0, 1 },             // 4: LT, RT
            { 0, 2 },             // 5: LT, LB
        };

        int cp_num[6] = { 3, 3, 3, 3, 2, 2 };
        for (idx = 0; idx < const_num; idx++)
        {
            int const_idx = const_order[idx];
            xevdm_derive_affine_constructed_candidate(poc, refp, cuw, cuh, cp_valid, cp_mv, cp_refi, const_model[const_idx], const_idx, cp_num[const_idx], mrg_list_cpmv, mrg_list_refi, &cnt, mrg_list_cp_num);
        }
    }

    // Zero padding
    int cnt_wo_padding = cnt;
    {
        int cp_idx;
        for (; cnt < AFF_MAX_CAND; cnt++)
        {
            mrg_list_cp_num[cnt] = 2;
            for (lidx = 0; lidx < REFP_NUM; lidx++)
            {
                for (cp_idx = 0; cp_idx < 2; cp_idx++)
                {
                    mrg_list_cpmv[cnt][lidx][cp_idx][MV_X] = 0;
                    mrg_list_cpmv[cnt][lidx][cp_idx][MV_Y] = 0;
                }
            }
            mrg_list_refi[cnt][REFP_0] = 0;
            mrg_list_refi[cnt][REFP_1] = (slice_type == SLICE_B) ? 0 : REFI_INVALID;
        }
    }

    return cnt_wo_padding; // return value only used for encoder
}


int xevdm_get_ctx_sig_coeff_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVD_CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int diag = pos_x + pos_y;
    int num_sig_coeff = 0;
    int ctx_idx;
    int ctx_ofs;

    if (pos_x < width_m1)
    {
        num_sig_coeff += pdata[1] != 0;
        if (pos_x < width_m1 - 1)
        {
            num_sig_coeff += pdata[2] != 0;
        }
        if (pos_y < height_m1)
        {
            num_sig_coeff += pdata[width + 1] != 0;
        }
    }

    if (pos_y < height_m1)
    {
        num_sig_coeff += pdata[width] != 0;
        if (pos_y < height_m1 - 1)
        {
            num_sig_coeff += pdata[2 * width] != 0;
        }
    }

    ctx_idx = XEVD_MIN(num_sig_coeff, 4) + 1;

    if (diag < 2)
    {
        ctx_idx = XEVD_MIN(ctx_idx, 2);
    }

    if (ch_type == Y_C)
    {
        ctx_ofs = diag < 2 ? 0 : (diag < 5 ? 2 : 7);
    }
    else
    {
        ctx_ofs = diag < 2 ? 0 : 2;
    }

    return ctx_ofs + ctx_idx;
}

int xevdm_get_ctx_gtA_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVD_CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int num_gtA = 0;
    int diag = pos_x + pos_y;

    if (pos_x < width_m1)
    {
        num_gtA += XEVD_ABS16(pdata[1]) > 1;
        if (pos_x < width_m1 - 1)
        {
            num_gtA += XEVD_ABS16(pdata[2]) > 1;
        }
        if (pos_y < height_m1)
        {
            num_gtA += XEVD_ABS16(pdata[width + 1]) > 1;
        }
    }

    if (pos_y < height_m1)
    {
        num_gtA += XEVD_ABS16(pdata[width]) > 1;
        if (pos_y < height_m1 - 1)
        {
            num_gtA += XEVD_ABS16(pdata[2 * width]) > 1;
        }
    }

    num_gtA = XEVD_MIN(num_gtA, 3) + 1;
    if (ch_type == Y_C)
    {
        num_gtA += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
    }
    return num_gtA;
}

int xevdm_get_ctx_gtB_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVD_CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int diag = pos_x + pos_y;
    int num_gtB = 0;

    if (pos_x < width_m1)
    {
        num_gtB += XEVD_ABS16(pdata[1]) > 2;
        if (pos_x < width_m1 - 1)
        {
            num_gtB += XEVD_ABS16(pdata[2]) > 2;
        }
        if (pos_y < height_m1)
        {
            num_gtB += XEVD_ABS16(pdata[width + 1]) > 2;
        }
    }

    if (pos_y < height_m1)
    {
        num_gtB += XEVD_ABS16(pdata[width]) > 2;
        if (pos_y < height_m1 - 1)
        {
            num_gtB += XEVD_ABS16(pdata[2 * width]) > 2;
        }
    }

    num_gtB = XEVD_MIN(num_gtB, 3) + 1;
    if (ch_type == Y_C)
    {
        num_gtB += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
    }
    return num_gtB;
}

int xevdm_get_ctx_remain_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVD_CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);

    pdata = pcoeff + pos_x + (pos_y << log2_w);



    int numPos = 0;
    int sumAbsAll = 0;
    if (pos_x < width_m1)
    {
        sumAbsAll += abs(pdata[1]);
        numPos += pdata[1] != 0;
        if (pos_x < width_m1 - 1)
        {
            sumAbsAll += abs(pdata[2]);
            numPos += pdata[2] != 0;
        }
        if (pos_y < height_m1)
        {
            sumAbsAll += abs(pdata[width + 1]);
            numPos += pdata[width + 1] != 0;
        }
    }
    if (pos_y < height_m1)
    {
        sumAbsAll += abs(pdata[width]);
        numPos += pdata[width] != 0;
        if (pos_y < height_m1 - 1)
        {
            sumAbsAll += abs(pdata[2 * width]);
            numPos += pdata[2 * width] != 0;
        }
    }

    int uiVal = (sumAbsAll - numPos);
    int iOrder;
    for (iOrder = 0; iOrder < MAX_GR_ORDER_RESIDUAL; iOrder++)
    {
        if ((1 << (iOrder + 3)) >(uiVal + 4))
        {
            break;
        }
    }
    return (iOrder == MAX_GR_ORDER_RESIDUAL ? (MAX_GR_ORDER_RESIDUAL - 1) : iOrder);
}

int xevdm_get_rice_para(s16 *pcoeff, int blkpos, int width, int height, int base_level)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVD_CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int sum_abs = 0;

    if (pos_x < width_m1)
    {
        sum_abs += XEVD_ABS16(pdata[1]);
        if (pos_x < width_m1 - 1)
        {
            sum_abs += XEVD_ABS16(pdata[2]);
        }
        if (pos_y < height_m1)
        {
            sum_abs += XEVD_ABS16(pdata[width + 1]);
        }
    }

    if (pos_y < height_m1)
    {
        sum_abs += XEVD_ABS16(pdata[width]);
        if (pos_y < height_m1 - 1)
        {
            sum_abs += XEVD_ABS16(pdata[2 * width]);
        }
    }
    sum_abs = XEVD_MAX(XEVD_MIN(sum_abs - 5 * base_level, 31), 0);
    return g_go_rice_para_coeff[sum_abs];
}

void xevdm_init_scan_sr(int *scan, int size_x, int size_y, int width, int height, int scan_type)
{
    int x, y, l, pos, num_line;

    pos = 0;
    num_line = size_x + size_y - 1;
    if (scan_type == COEF_SCAN_ZIGZAG)
    {
        /* starting point */
        scan[pos] = 0;
        pos++;

        /* loop */
        for (l = 1; l < num_line; l++)
        {
            if (l % 2) /* decreasing loop */
            {
                x = XEVD_MIN(l, size_x - 1);
                y = XEVD_MAX(0, l - (size_x - 1));

                while (x >= 0 && y < size_y)
                {
                    scan[pos] = y * width + x;
                    pos++;
                    x--;
                    y++;
                }
            }
            else /* increasing loop */
            {
                y = XEVD_MIN(l, size_y - 1);
                x = XEVD_MAX(0, l - (size_y - 1));
                while (y >= 0 && x < size_x)
                {
                    scan[pos] = y * width + x;
                    pos++;
                    x++;
                    y--;
                }
            }
        }
    }
}


int xevdm_get_transform_shift(int log2_size, int type, int bit_depth)
{
    return (type == 0) ? TX_SHIFT1(log2_size, bit_depth) : TX_SHIFT2(log2_size);
}

void xevdm_split_get_split_rdo_order(int cuw, int cuh, SPLIT_MODE splits[MAX_SPLIT_NUM])
{
    if (cuw < cuh)
    {
        splits[1] = SPLIT_BI_HOR;
        splits[2] = SPLIT_BI_VER;
    }
    else
    {
        splits[1] = SPLIT_BI_VER;
        splits[2] = SPLIT_BI_HOR;
    }
    splits[3] = SPLIT_TRI_VER;
    splits[4] = SPLIT_TRI_HOR;
    splits[5] = SPLIT_QUAD;
    splits[0] = NO_SPLIT;
}

void xevdm_split_get_suco_order(int suco_flag, SPLIT_MODE mode, int suco_order[SPLIT_MAX_PART_COUNT])
{
    int i, i2;
    if (suco_flag)
    {
        // Reverse order of partitions
        switch (mode)
        {
        case SPLIT_QUAD:
            suco_order[0] = 1;
            suco_order[1] = 0;
            suco_order[2] = 3;
            suco_order[3] = 2;
            break;
        default:
            i2 = 0;
            for (i = xevd_split_part_count(mode); i > 0; --i)
            {
                suco_order[i2++] = i - 1;
            }
        }
    }
    else
    {
        // Direct order of partitions
        for (i = 0; i < xevd_split_part_count(mode); ++i)
        {
            suco_order[i] = i;
        }
    }
}

int  xevdm_split_is_TT(SPLIT_MODE mode)
{
    return (mode == SPLIT_TRI_HOR) || (mode == SPLIT_TRI_VER) ? 1 : 0;
}

int  xevdm_split_is_BT(SPLIT_MODE mode)
{
    return (mode == SPLIT_BI_HOR) || (mode == SPLIT_BI_VER) ? 1 : 0;
}

#if SIMD_CLIP
__inline void do_clip(__m128i *vreg, __m128i *vbdmin, __m128i *vbdmax) { *vreg = _mm_min_epi16(*vbdmax, _mm_max_epi16(*vbdmin, *vreg)); }

void clip_simd(const pel* src, int src_stride, pel *dst, int dst_stride, int width, int height, const int clp_rng_min, const int clp_rng_max)
{
    __m128i vzero = _mm_setzero_si128();
    __m128i vbdmin = _mm_set1_epi16(clp_rng_min);
    __m128i vbdmax = _mm_set1_epi16(clp_rng_max);

    if ((width & 3) == 0)
    {
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col += 4)
            {
                __m128i val;
                val = _mm_loadl_epi64((const __m128i *)&src[col]);
                val = _mm_cvtepi16_epi32(val);
                val = _mm_packs_epi32(val, vzero);
                do_clip(&val, &vbdmin, &vbdmax);
                _mm_storel_epi64((__m128i *)&dst[col], val);
            }
            src += src_stride;
            dst += dst_stride;
        }
    }
    else
    {
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col++)
            {
                dst[col] = XEVD_CLIP3(clp_rng_min, clp_rng_max, src[col]);
            }
            src += src_stride;
            dst += dst_stride;
        }
    }
}
#endif

u8 xevdm_check_ats_inter_info_coded(int cuw, int cuh, int pred_mode, int tool_ats)
{
    int min_size = 8;
    int max_size = 1 << MAX_TR_LOG2;
    u8  mode_hori, mode_vert, mode_hori_quad, mode_vert_quad;
    if (!tool_ats || pred_mode == MODE_INTRA || cuw > max_size || cuh > max_size || pred_mode == MODE_IBC)
    {
        mode_hori = mode_vert = mode_hori_quad = mode_vert_quad = 0;
    }
    else
    {
        //vertical mode
        mode_vert = cuw >= min_size ? 1 : 0;
        mode_vert_quad = cuw >= min_size * 2 ? 1 : 0;
        mode_hori = cuh >= min_size ? 1 : 0;
        mode_hori_quad = cuh >= min_size * 2 ? 1 : 0;
    }
    return (mode_vert << 0) + (mode_hori << 1) + (mode_vert_quad << 2) + (mode_hori_quad << 3);
}

void xevdm_get_tu_size(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* log2_tuw, int* log2_tuh)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    if (ats_inter_idx == 0)
    {
        *log2_tuw = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
        *log2_tuh = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
        return;
    }

    assert(ats_inter_idx <= 4);
    if (is_ats_inter_horizontal(ats_inter_idx))
    {
        *log2_tuw = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
        *log2_tuh = is_ats_inter_quad_size(ats_inter_idx) ? log2_cuh - 2 : log2_cuh - 1;
        *log2_tuh = (*log2_tuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : *log2_tuh;
    }
    else
    {
        *log2_tuw = is_ats_inter_quad_size(ats_inter_idx) ? log2_cuw - 2 : log2_cuw - 1;
        *log2_tuw = (*log2_tuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : *log2_tuw;
        *log2_tuh = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
    }
}

static void get_tu_pos_offset(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* x_offset, int* y_offset)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;

    if (ats_inter_idx == 0)
    {
        *x_offset = 0;
        *y_offset = 0;
        return;
    }

    if (is_ats_inter_horizontal(ats_inter_idx))
    {
        *x_offset = 0;
        *y_offset = ats_inter_pos == 0 ? 0 : cuh - (is_ats_inter_quad_size(ats_inter_idx) ? cuh / 4 : cuh / 2);
    }
    else
    {
        *x_offset = ats_inter_pos == 0 ? 0 : cuw - (is_ats_inter_quad_size(ats_inter_idx) ? cuw / 4 : cuw / 2);
        *y_offset = 0;
    }
}

void xevdm_get_ats_inter_trs(u8 ats_inter_info, int log2_cuw, int log2_cuh, u8* ats_cu, u8* ats_mode)
{
    if (ats_inter_info == 0)
    {
        return;
    }

    if (log2_cuw > 5 || log2_cuh > 5)
    {
        *ats_cu = 0;
        *ats_mode = 0;
    }
    else
    {
        u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
        u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
        u8 t_idx_h, t_idx_v;

        //Note: 1 is DCT8 and 0 is DST7
        if (is_ats_inter_horizontal(ats_inter_idx))
        {
            t_idx_h = 0;
            t_idx_v = ats_inter_pos == 0 ? 1 : 0;
        }
        else
        {
            t_idx_v = 0;
            t_idx_h = ats_inter_pos == 0 ? 1 : 0;
        }
        *ats_cu = 1;
        *ats_mode = (t_idx_h << 1) | t_idx_v;
    }
}

void xevdm_set_cu_cbf_flags(u8 cbf_y, u8 ats_inter_info, int log2_cuw, int log2_cuh, u32 *map_scu, int w_scu)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
    int x_offset, y_offset, log2_tuw, log2_tuh;
    int x, y, w, h;
    int w_cus = 1 << (log2_cuw - MIN_CU_LOG2);
    int h_cus = 1 << (log2_cuh - MIN_CU_LOG2);
    u32 *cur_map;
    if (ats_inter_info)
    {
        get_tu_pos_offset(ats_inter_info, log2_cuw, log2_cuh, &x_offset, &y_offset);
        xevdm_get_tu_size(ats_inter_info, log2_cuw, log2_cuh, &log2_tuw, &log2_tuh);
        x_offset >>= MIN_CU_LOG2;
        y_offset >>= MIN_CU_LOG2;
        w = 1 << (log2_tuw - MIN_CU_LOG2);
        h = 1 << (log2_tuh - MIN_CU_LOG2);

        // Clear CbF of CU
        cur_map = map_scu;
        for (y = 0; y < h_cus; ++y, cur_map += w_scu)
        {
            for (x = 0; x < w_cus; ++x)
            {
                MCU_CLR_CBFL(cur_map[x]);
            }
        }

        if (cbf_y)
        {
            // Set CbF only on coded part
            cur_map = map_scu + y_offset * w_scu + x_offset;
            for (y = 0; y < h; ++y, cur_map += w_scu)
            {
                for (x = 0; x < w; ++x)
                {
                    MCU_SET_CBFL(cur_map[x]);
                }
            }
        }
    }
    else
    {
        assert(0);
    }
}

#if REMOVE_UNIBLOCKS_4x4
char is_inter_applicable_log2(int log2_cuw, int log2_cuh)
{
    return (log2_cuw > MIN_CU_LOG2 || log2_cuh > MIN_CU_LOG2) ? 1 : 0;
}

char is_inter_applicable(int cuw, int cuh)
{
    return (cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE) ? 1 : 0;
}
#endif

void xevdm_get_mv_collocated(XEVD_REFP(*refp)[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], s8 *availablePredIdx, XEVD_SH* sh)
{
    *availablePredIdx = 0;

    int temporal_mvp_asigned_flag = sh->temporal_mvp_asigned_flag;
    int collocated_from_list_idx = (sh->slice_type == SLICE_P) ? REFP_0 : REFP_1;  // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    int collocated_from_ref_idx = 0;        // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    int collocated_mvp_source_list_idx = REFP_0;  // Specifies source (List ID) in collocated pic that provides MV information (Applicability is function of NoBackwardPredFlag)

    if (sh->temporal_mvp_asigned_flag)
    {
        collocated_from_list_idx = sh->collocated_from_list_idx;
        collocated_from_ref_idx = sh->collocated_from_ref_idx;
        collocated_mvp_source_list_idx = sh->collocated_mvp_source_list_idx;
    }

    XEVD_REFP colPic = (refp[collocated_from_ref_idx][collocated_from_list_idx]);  // col picture is ref idx 0 and list 1

    int neb_addr_coll = scup;     // Col
    int dpoc_co[REFP_NUM] = { 0, 0 };
    int dpoc[REFP_NUM] = { 0, 0 };
    int ver_refi[REFP_NUM] = { -1, -1 };
    xevd_mset(mvp, 0, sizeof(s16) * REFP_NUM * MV_D);


    s8(*map_refi_co)[REFP_NUM] = colPic.map_refi;
    dpoc[REFP_0] = poc - refp[0][REFP_0].poc;
    dpoc[REFP_1] = poc - refp[0][REFP_1].poc;

    if (!temporal_mvp_asigned_flag)
    {
        dpoc_co[REFP_0] = colPic.poc - colPic.list_poc[map_refi_co[neb_addr_coll][REFP_0]]; //POC1
        dpoc_co[REFP_1] = colPic.poc - colPic.list_poc[map_refi_co[neb_addr_coll][REFP_1]]; //POC2

        for (int lidx = 0; lidx < REFP_NUM; lidx++)
        {
            s8 refidx = map_refi_co[neb_addr_coll][lidx];
            if (dpoc_co[lidx] != 0 && REFI_IS_VALID(refidx))
            {
                int ratio_tmvp = ((dpoc[lidx]) << MVP_SCALING_PRECISION) / dpoc_co[lidx];
                ver_refi[lidx] = 0; // ref idx
                s16 *mvc = colPic.map_mv[neb_addr_coll][lidx];
                scaling_mv(ratio_tmvp, mvc, mvp[lidx]);
            }
            else
            {
                mvp[lidx][MV_X] = 0;
                mvp[lidx][MV_Y] = 0;
            }
        }
    }
    else
    {
        dpoc_co[REFP_0] = 0;
        // collocated_mvp_source_list_idx = REFP_0; // specified above
        s8 refidx = map_refi_co[neb_addr_coll][collocated_mvp_source_list_idx];
        if(REFI_IS_VALID(refidx))
            dpoc_co[REFP_0] = colPic.poc - colPic.list_poc[refidx];

        if (dpoc_co[REFP_0] != 0)
        {
            ver_refi[REFP_0] = 0;
            ver_refi[REFP_1] = 0;
            s16 *mvc = colPic.map_mv[neb_addr_coll][collocated_mvp_source_list_idx]; //  collocated_mvp_source_list_idx == 0 for RA
            int ratio_tmvp = ((dpoc[REFP_0]) << MVP_SCALING_PRECISION) / dpoc_co[REFP_0];
            scaling_mv(ratio_tmvp, mvc, mvp[REFP_0]);
            ratio_tmvp = ((dpoc[REFP_1]) << MVP_SCALING_PRECISION) / dpoc_co[REFP_0];
            scaling_mv(ratio_tmvp, mvc, mvp[REFP_1]);
        }
        else
        {
            mvp[REFP_0][MV_X] = 0;
            mvp[REFP_0][MV_Y] = 0;
            mvp[REFP_1][MV_X] = 0;
            mvp[REFP_1][MV_Y] = 0;
        }
    }


    {
        int maxX = PIC_PAD_SIZE_L + (w_scu << MIN_CU_LOG2) - 1;
        int maxY = PIC_PAD_SIZE_L + (h_scu << MIN_CU_LOG2) - 1;
        int x = (c_scu % w_scu) << MIN_CU_LOG2;
        int y = (c_scu / w_scu) << MIN_CU_LOG2;
        xevdm_clip_mv_pic(x, y, maxX, maxY, mvp);
    }

    int flag = REFI_IS_VALID(ver_refi[REFP_0]) + (REFI_IS_VALID(ver_refi[REFP_1]) << 1);
    *availablePredIdx = flag; // combines flag and indication on what type of prediction is ( 0 - not available, 1 = uniL0, 2 = uniL1, 3 = Bi)
}

u8 xevdm_check_chroma_split_allowed(int luma_width, int luma_height)
{
    return (luma_width * luma_height) >= (16 * 4) ? 1 : 0;
}
u8 xevd_is_chroma_split_allowed(int w, int h, SPLIT_MODE split)
{
    switch (split)
    {
    case SPLIT_BI_VER:
        return xevdm_check_chroma_split_allowed(w >> 1, h);
    case SPLIT_BI_HOR:
        return xevdm_check_chroma_split_allowed(w, h >> 1);
    case SPLIT_TRI_VER:
        return xevdm_check_chroma_split_allowed(w >> 2, h);
    case SPLIT_TRI_HOR:
        return xevdm_check_chroma_split_allowed(w, h >> 2);
    default:
        xevd_assert(!"This check is for BTT only");
        return 0;
    }
}

enum TQC_RUN xevd_get_run(enum TQC_RUN run_list, TREE_CONS tree_cons)
{
    enum TQC_RUN ans = 0;
    if (xevd_check_luma_fn(tree_cons))
    {
        ans |= run_list & RUN_L;
    }

    if (xevd_check_chroma_fn(tree_cons))
    {
        ans |= run_list & RUN_CB;
        ans |= run_list & RUN_CR;
    }
    return ans;
}


u8 xevd_check_luma_fn(TREE_CONS tree_cons)
{
    return tree_cons.tree_type != TREE_C;
}

u8 xevd_check_chroma_fn(TREE_CONS tree_cons)
{
    return tree_cons.tree_type != TREE_L;
}

u8 xevd_check_all_fn(TREE_CONS tree_cons)
{
    return tree_cons.tree_type == TREE_LC;
}

u8 xevd_check_only_intra_fn(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eOnlyIntra;
}

u8 xevd_check_only_inter_fn(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eOnlyInter;
}

u8 xevd_check_all_preds_fn(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eAll;
}

TREE_CONS xevd_get_default_tree_cons()
{
    TREE_CONS ans;
    ans.changed = FALSE;
    ans.mode_cons = eAll;
    ans.tree_type = TREE_LC;
    return ans;
}

void xevd_set_tree_mode(TREE_CONS* dest, MODE_CONS mode)
{
    dest->mode_cons = mode;
    switch (mode)
    {
    case eOnlyIntra:
        dest->tree_type = TREE_L;
        break;
    default:
        dest->tree_type = TREE_LC;
        break;
    }
}

MODE_CONS xevdm_get_mode_cons_by_split(SPLIT_MODE split_mode, int cuw, int cuh)
{
    int small_cuw = cuw;
    int small_cuh = cuh;
    switch (split_mode)
    {
    case SPLIT_BI_HOR:
        small_cuh >>= 1;
        break;
    case SPLIT_BI_VER:
        small_cuw >>= 1;
        break;
    case SPLIT_TRI_HOR:
        small_cuh >>= 2;
        break;
    case SPLIT_TRI_VER:
        small_cuw >>= 2;
        break;
    default:
        xevd_assert(!"For BTT only");
    }
    return (small_cuh == 4 && small_cuw == 4) ? eOnlyIntra : eAll;
}

BOOL xevd_signal_mode_cons(TREE_CONS* parent, TREE_CONS* cur_split)
{
    return parent->mode_cons == eAll && cur_split->changed;
}


XEVD_PIC * xevdm_picbuf_alloc(PICBUF_ALLOCATOR * pa, int * ret, int bitdepth)
{
    return xevdm_picbuf_alloc_exp(pa->w, pa->h, pa->pad_l, pa->pad_c, ret, pa->idc, bitdepth);
}

#if HDR_MD5_CHECK
static void __imgb_cpy_plane(void *src, void *dst, int bw, int h, int s_src,
    int s_dst)
{
    int i;
    unsigned char *s, *d;

    s = (unsigned char*)src;
    d = (unsigned char*)dst;

    for (i = 0; i < h; i++)
    {
        xevd_mcpy(d, s, bw);
        s += s_src;
        d += s_dst;
    }
}

static void imgb_conv_8b_to_16b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src,
    int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for (i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_conv_16b_to_8b(XEVD_IMGB * imgb_dst, XEVD_IMGB * imgb_src,
    int shift)
{

    int i, j, k, t0, add;

    short         * s;
    unsigned char * d;

    add = 1 << (shift - 1);

    for (i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVD_CLIP(t0, 0, 255));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = d + imgb_dst->s[i];
        }
    }
}
void xevd_imgb_cpy(XEVD_IMGB * dst, XEVD_IMGB * src)
{
    int i, bd;

    if (src->cs == dst->cs)
    {
        if (src->cs == XEVD_CS_YCBCR420_10LE) bd = 2;
        else bd = 1;

        for (i = 0; i < src->np; i++)
        {
            __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i],
                src->s[i], dst->s[i]);
        }
    }
    else if (src->cs == XEVD_CS_YCBCR420 &&
        dst->cs == XEVD_CS_YCBCR420_10LE)
    {
        imgb_conv_8b_to_16b(dst, src, 2);
    }
    else if (src->cs == XEVD_CS_YCBCR420_10LE &&
        dst->cs == XEVD_CS_YCBCR420)
    {
        imgb_conv_16b_to_8b(dst, src, 2);
    }
    else
    {
        xevd_trace("ERROR: unsupported image copy\n");
        return;
    }
    for (i = 0; i < XEVD_TS_NUM; i++)
    {
        dst->ts[i] = src->ts[i];
    }
    dst->imgb_active_aps_id = src->imgb_active_aps_id;
    dst->imgb_active_pps_id = src->imgb_active_pps_id;
}
int xevdm_picbuf_check_signature(XEVD_PIC * pic, u8 signature[N_C][16], XEVD_IMGB *imgb, int compare_md5)
{
    u8 pic_sign[N_C][16] = { { 0 } };
    int ret;

    /* execute MD5 digest here */
    ret = xevd_md5_imgb(imgb, pic_sign);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

    if (compare_md5)
    {
        if (xevd_mcmp(signature, pic_sign[0], N_C * 16) != 0)
        {
            return XEVD_ERR_BAD_CRC;
        }
    }

    xevd_mcpy(pic->digest, pic_sign, N_C * 16);

    return XEVD_OK;
}
#endif
#if !HDR_MD5_CHECK
int xevdm_picbuf_check_signature(XEVD_PIC * pic, u8 signature[16], int bit_depth)
{
    u8 pic_sign[16];
    int ret;

    /* execute MD5 digest here */
    ret = xevd_picbuf_signature(pic, pic_sign);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    if (memcmp(signature, pic_sign, 16) != 0)
    {
        return XEVD_ERR_BAD_CRC;
    }

    return XEVD_OK;
}
#endif
void xevdm_set_affine_mvf(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int   w_cu;
    int   h_cu;
    int   scup;
    int   w_scu;
    int   lidx;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    int   vertex_num = mcore->affine_flag + 1;
    int   aff_scup[VER_NUM];

    scup = core->scup;
    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;
    w_scu = ctx->w_scu;

    aff_scup[0] = 0;
    aff_scup[1] = (w_cu - 1);
    aff_scup[2] = (h_cu - 1) * w_scu;
    aff_scup[3] = (w_cu - 1) + (h_cu - 1) * w_scu;

    // derive sub-block size
    int sub_w = 4, sub_h = 4;
    xevdm_derive_affine_subblock_size_bi( mcore->affine_mv, core->refi, (1 << core->log2_cuw), (1 << core->log2_cuh), &sub_w, &sub_h, vertex_num, NULL);

    int   sub_w_in_scu = PEL2SCU( sub_w );
    int   sub_h_in_scu = PEL2SCU( sub_h );
    int   half_w = sub_w >> 1;
    int   half_h = sub_h >> 1;

    for(lidx = 0; lidx < REFP_NUM; lidx++)
    {
        if(core->refi[lidx] >= 0)
        {
            s16( *ac_mv )[MV_D] = mcore->affine_mv[lidx];
            int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
            int mv_scale_hor = ac_mv[0][MV_X] << 7;
            int mv_scale_ver = ac_mv[0][MV_Y] << 7;
            int mv_y_hor = mv_scale_hor;
            int mv_y_ver = mv_scale_ver;
            int mv_scale_tmp_hor, mv_scale_tmp_ver;

            // convert to 2^(storeBit + iBit) precision
            dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);     // deltaMvHor
            dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);
            if ( vertex_num == 3 )
            {
                dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh); // deltaMvVer
                dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
            }
            else
            {
                dmv_ver_x = -dmv_hor_y;                                                // deltaMvVer
                dmv_ver_y = dmv_hor_x;
            }

            for ( int h = 0; h < h_cu; h += sub_h_in_scu )
            {
                for ( int w = 0; w < w_cu; w += sub_w_in_scu )
                {
                    if ( w == 0 && h == 0 )
                    {
                        mv_scale_tmp_hor = ac_mv[0][MV_X];
                        mv_scale_tmp_ver = ac_mv[0][MV_Y];
                    }
                    else if ( w + sub_w_in_scu == w_cu && h == 0 )
                    {
                        mv_scale_tmp_hor = ac_mv[1][MV_X];
                        mv_scale_tmp_ver = ac_mv[1][MV_Y];
                    }
                    else if ( w == 0 && h + sub_h_in_scu == h_cu && vertex_num == 3 )
                    {
                        mv_scale_tmp_hor = ac_mv[2][MV_X];
                        mv_scale_tmp_ver = ac_mv[2][MV_Y];
                    }
                    else
                    {
                        int pos_x = (w << MIN_CU_LOG2) + half_w;
                        int pos_y = (h << MIN_CU_LOG2) + half_h;

                        mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                        mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                        // 1/16 precision, 18 bits, same as MC
                        xevdm_mv_rounding_s32( mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 5, 0 );

                        mv_scale_tmp_hor = XEVD_CLIP3( -(1 << 17), (1 << 17) - 1, mv_scale_tmp_hor );
                        mv_scale_tmp_ver = XEVD_CLIP3( -(1 << 17), (1 << 17) - 1, mv_scale_tmp_ver );

                        // 1/4 precision, 16 bits for storage
                        mv_scale_tmp_hor >>= 2;
                        mv_scale_tmp_ver >>= 2;
                    }

                    // save MV for each 4x4 block
                    for ( int y = h; y < h + sub_h_in_scu; y++ )
                    {
                        for ( int x = w; x < w + sub_w_in_scu; x++ )
                        {
                            int addr_in_scu = scup + x + y * w_scu;
                            ctx->map_mv[addr_in_scu][lidx][MV_X] = (s16)mv_scale_tmp_hor;
                            ctx->map_mv[addr_in_scu][lidx][MV_Y] = (s16)mv_scale_tmp_ver;
                        }
                    }
                }
            }
        }
    }
}

void xevdm_set_dec_info(XEVD_CTX * ctx, XEVD_CORE * core)
{
    s8  (*map_refi)[REFP_NUM];
    s16 (*map_mv)[REFP_NUM][MV_D];
    s16(*map_unrefined_mv)[REFP_NUM][MV_D];
    u32 idx;
    u32  *map_scu;
    s8   *map_ipm;
    u8   *map_ats_inter;
    int   w_cu;
    int   h_cu;
    int   scup;
    int   w_scu;
    int   i, j;
    int   flag;

    u32  *map_affine;
    u32  *map_cu_mode;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    scup = core->scup;
    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;
    w_scu = ctx->w_scu;
    map_refi = ctx->map_refi + scup;
    map_scu  = ctx->map_scu + scup;
    map_mv   = ctx->map_mv + scup;
    map_unrefined_mv = mctx->map_unrefined_mv + scup;
    map_ipm  = ctx->map_ipm + scup;

    flag = (core->pred_mode == MODE_INTRA) ? 1 : 0;
    map_affine = mctx->map_affine + scup;
    map_cu_mode = ctx->map_cu_mode + scup;
    map_ats_inter = mctx->map_ats_inter + scup;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    idx = 0;

    if (xevd_check_luma(ctx, core))
    {
    for(i = 0; i < h_cu; i++)
    {
        for(j = 0; j < w_cu; j++)
        {
            if(core->pred_mode == MODE_SKIP)
            {
                MCU_SET_SF(map_scu[j]);
            }
            else
            {
                MCU_CLR_SF(map_scu[j]);
            }
            if((core->pred_mode == MODE_SKIP) || (core->pred_mode == MODE_DIR))
            {
                if(mcore->dmvr_flag)
                {
                    MCU_SET_DMVRF(map_scu[j]);
                }
                else
                {
                    MCU_CLR_DMVRF(map_scu[j]);
                }
            }
            int sub_idx = ((!!(i & 32)) << 1) | (!!(j & 32));
            if (core->is_coef_sub[Y_C][sub_idx])
            {
                MCU_SET_CBFL(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFL(map_scu[j]);
            }

            if(mcore->affine_flag)
            {
                MCU_SET_AFF(map_scu[j], mcore->affine_flag);

                MCU_SET_AFF_LOGW(map_affine[j], core->log2_cuw);
                MCU_SET_AFF_LOGH(map_affine[j], core->log2_cuh);
                MCU_SET_AFF_XOFF(map_affine[j], j);
                MCU_SET_AFF_YOFF(map_affine[j], i);
            }
            else
            {
                MCU_CLR_AFF(map_scu[j]);
            }
            if (mcore->ibc_flag)
            {
              MCU_SET_IBC(map_scu[j]);
            }
            else
            {
              MCU_CLR_IBC(map_scu[j]);
            }
            MCU_SET_LOGW(map_cu_mode[j], core->log2_cuw);
            MCU_SET_LOGH(map_cu_mode[j], core->log2_cuh);

            if(mcore->mmvd_flag)
            {
                MCU_SET_MMVDS(map_cu_mode[j]);
            }
            else
            {
                MCU_CLR_MMVDS(map_cu_mode[j]);
            }

            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_RESET_QP(map_scu[j]);
                MCU_SET_IF_SN_QP(map_scu[j], flag, ctx->slice_num, core->qp);
            }
            else
            {
                MCU_SET_IF_SN_QP(map_scu[j], flag, ctx->slice_num, ctx->tile[core->tile_num].qp);
            }

            map_refi[j][REFP_0] = core->refi[REFP_0];
            map_refi[j][REFP_1] = core->refi[REFP_1];
            map_ats_inter[j] = mcore->ats_inter_info;
            if (core->pred_mode == MODE_IBC)
            {
                map_ats_inter[j] = 0;
            }


            if(mcore->dmvr_flag)
            {
                map_mv[j][REFP_0][MV_X] = mcore->dmvr_mv[idx + j][REFP_0][MV_X];
                map_mv[j][REFP_0][MV_Y] = mcore->dmvr_mv[idx + j][REFP_0][MV_Y];
                map_mv[j][REFP_1][MV_X] = mcore->dmvr_mv[idx + j][REFP_1][MV_X];
                map_mv[j][REFP_1][MV_Y] = mcore->dmvr_mv[idx + j][REFP_1][MV_Y];

                map_unrefined_mv[j][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
                map_unrefined_mv[j][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
                map_unrefined_mv[j][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
                map_unrefined_mv[j][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];
            }
            else
            {

                map_mv[j][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
                map_mv[j][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
                map_mv[j][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
                map_mv[j][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];

                map_unrefined_mv[j][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
                map_unrefined_mv[j][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
                map_unrefined_mv[j][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
                map_unrefined_mv[j][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];

            }

            map_ipm[j] = core->ipm[0];
        }
        map_refi += w_scu;
        map_mv += w_scu;

        map_unrefined_mv += w_scu;
        idx += w_cu;
        map_scu += w_scu;
        map_ipm += w_scu;

        map_affine += w_scu;
        map_cu_mode += w_scu;
        map_ats_inter += w_scu;
    }

    if (mcore->ats_inter_info)
    {
        assert(core->is_coef_sub[Y_C][0] == core->is_coef[Y_C]);
        assert(core->is_coef_sub[U_C][0] == core->is_coef[U_C]);
        assert(core->is_coef_sub[V_C][0] == core->is_coef[V_C]);
        xevdm_set_cu_cbf_flags(core->is_coef[Y_C], mcore->ats_inter_info, core->log2_cuw, core->log2_cuh, ctx->map_scu + core->scup, ctx->w_scu);
    }

    if(mcore->affine_flag)
    {
        xevdm_set_affine_mvf(ctx, core);
    }

    map_refi = ctx->map_refi + scup;
    map_mv = ctx->map_mv + scup;

    xevd_mcpy(core->mv, map_mv, sizeof(core->mv));
    xevd_mcpy(core->refi, map_refi, sizeof(core->refi));
    }
}

void xevdm_split_tbl_init(XEVD_CTX *ctx, XEVD_SPS * sps)
{
    xevd_split_tbl[BLOCK_11][IDX_MAX] = ctx->log2_max_cuwh;
    xevd_split_tbl[BLOCK_11][IDX_MIN] = sps->log2_min_cb_size_minus2 + 2;
    xevd_split_tbl[BLOCK_12][IDX_MAX] = ctx->log2_max_cuwh;
    xevd_split_tbl[BLOCK_12][IDX_MIN] = xevd_split_tbl[BLOCK_11][IDX_MIN] + 1;
    xevd_split_tbl[BLOCK_14][IDX_MAX] = XEVD_MIN(ctx->log2_max_cuwh - sps->log2_diff_ctu_max_14_cb_size, 6);
    xevd_split_tbl[BLOCK_14][IDX_MIN] = xevd_split_tbl[BLOCK_12][IDX_MIN] + 1;
    xevd_split_tbl[BLOCK_TT][IDX_MAX] = XEVD_MIN(ctx->log2_max_cuwh - sps->log2_diff_ctu_max_tt_cb_size, 6);
    xevd_split_tbl[BLOCK_TT][IDX_MIN] = xevd_split_tbl[BLOCK_11][IDX_MIN] + sps->log2_diff_min_cb_min_tt_cb_size_minus2 + 2;
}

#if USE_DRAW_PARTITION_DEC
void cpy_pic(XEVD_PIC * pic_src, XEVD_PIC * pic_dst)
{
    int i, aw, ah, s, e, bsize;

    int a_size, p_size;
    for (i = 0; i<3; i++)
    {

        a_size = MIN_CU_SIZE >> (!!i);
        p_size = i ? pic_dst->pad_c : pic_dst->pad_l;

        aw = XEVD_ALIGN(pic_dst->w_l >> (!!i), a_size);
        ah = XEVD_ALIGN(pic_dst->h_l >> (!!i), a_size);

        s = aw + p_size + p_size;
        e = ah + p_size + p_size;

        bsize = s * ah * sizeof(pel);
        switch (i)
        {
        case 0:
            xevd_mcpy(pic_dst->y, pic_src->y, bsize); break;
        case 1:
            xevd_mcpy(pic_dst->u, pic_src->u, bsize); break;
        case 2:
            xevd_mcpy(pic_dst->v, pic_src->v, bsize); break;
        default:
            break;
        }
    }
}

int write_pic(char * fname, XEVD_PIC * pic)
{
    pel    * p;
    int      j;
    FILE   * fp;
    static int cnt = 0;

    if (cnt == 0)
        fp = fopen(fname, "wb");
    else
        fp = fopen(fname, "ab");
    cnt++;
    if (fp == NULL)
    {
        assert(!"cannot open file");
        return -1;
    }

    {
        /* Crop image supported */
        /* luma */
        p = pic->y;
        for (j = 0; j<pic->h_l; j++)
        {
            fwrite(p, pic->w_l, sizeof(pel), fp);
            p += pic->s_l;
        }

        /* chroma */
        p = pic->u;
        for (j = 0; j<pic->h_c; j++)
        {
            fwrite(p, pic->w_c, sizeof(pel), fp);
            p += pic->s_c;
        }

        p = pic->v;
        for (j = 0; j<pic->h_c; j++)
        {
            fwrite(p, pic->w_c, sizeof(pel), fp);
            p += pic->s_c;
        }
    }

    fclose(fp);
    return 0;
}

static int draw_tree(XEVD_CTX * ctx, XEVD_PIC * pic, int x, int y,
                     int cuw, int cuh, int cud, int cup, int next_split)
{
    s8      split_mode;
    int     cup_x1, cup_y1;
    int     x1, y1, lcu_num;
    int     dx, dy, cup1, cup2, cup3;

    lcu_num = (x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xevd_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, &ctx->map_split[lcu_num]);

    if (split_mode != NO_SPLIT && !(cuw == 4 && cuh == 4))
    {
        if (split_mode == SPLIT_BI_VER)
        {
            int sub_cud = cud + 1;
            int sub_cuw = cuw >> 1;
            int sub_cuh = cuh;

            x1 = x + sub_cuw;
            y1 = y + sub_cuh;

            cup_x1 = cup + (sub_cuw >> MIN_CU_LOG2);
            cup_y1 = (sub_cuh >> MIN_CU_LOG2) * (ctx->max_cuwh >> MIN_CU_LOG2);

            draw_tree(ctx, pic, x, y, sub_cuw, sub_cuh, sub_cud, cup, next_split - 1);

            if (x1 < pic->w_l)
            {
                draw_tree(ctx, pic, x1, y, sub_cuw, sub_cuh, sub_cud, cup_x1, next_split - 1);
            }
        }
        else if (split_mode == SPLIT_BI_HOR)
        {
            int sub_cud = cud + 1;
            int sub_cuw = cuw;
            int sub_cuh = cuh >> 1;

            x1 = x + sub_cuw;
            y1 = y + sub_cuh;

            cup_x1 = cup + (sub_cuw >> MIN_CU_LOG2);
            cup_y1 = (sub_cuh >> MIN_CU_LOG2) * (ctx->max_cuwh >> MIN_CU_LOG2);

            draw_tree(ctx, pic, x, y, sub_cuw, sub_cuh, sub_cud, cup, next_split - 1);

            if (y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x, y1, sub_cuw, sub_cuh, sub_cud, cup + cup_y1, next_split - 1);
            }
        }
        else if (split_mode == SPLIT_TRI_VER)
        {
            int sub_cud = cud + 2;
            int sub_cuw = cuw >> 2;
            int sub_cuh = cuh;
            int x2, cup_x2;

            draw_tree(ctx, pic, x, y, sub_cuw, sub_cuh, sub_cud, cup, next_split - 1);

            x1 = x + sub_cuw;
            cup_x1 = cup + (sub_cuw >> MIN_CU_LOG2);
            sub_cuw = cuw >> 1;
            if (x1 < pic->w_l)
            {
                draw_tree(ctx, pic, x1, y, sub_cuw, sub_cuh, sub_cud - 1, cup_x1, (next_split - 1) == 0 ? 0 : 0);
            }

            x2 = x1 + sub_cuw;
            cup_x2 = cup_x1 + (sub_cuw >> MIN_CU_LOG2);
            sub_cuw = cuw >> 2;
            if (x2 < pic->w_l)
            {
                draw_tree(ctx, pic, x2, y, sub_cuw, sub_cuh, sub_cud, cup_x2, next_split - 1);
            }
        }
        else if (split_mode == SPLIT_TRI_HOR)
        {
            int sub_cud = cud + 2;
            int sub_cuw = cuw;
            int sub_cuh = cuh >> 2;
            int y2, cup_y2;

            draw_tree(ctx, pic, x, y, sub_cuw, sub_cuh, sub_cud, cup, next_split - 1);

            y1 = y + sub_cuh;
            cup_y1 = (sub_cuh >> MIN_CU_LOG2) * (ctx->max_cuwh >> MIN_CU_LOG2);
            sub_cuh = cuh >> 1;
            if (y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x, y1, sub_cuw, sub_cuh, sub_cud - 1, cup + cup_y1, (next_split - 1) == 0 ? 0 : 0);
            }

            y2 = y1 + sub_cuh;
            cup_y2 = cup_y1 + (sub_cuh >> MIN_CU_LOG2) * (ctx->max_cuwh >> MIN_CU_LOG2);
            sub_cuh = cuh >> 2;
            if (y2 < pic->h_l)
            {
                draw_tree(ctx, pic, x, y2, sub_cuw, sub_cuh, sub_cud, cup + cup_y2, next_split - 1);
            }
        }
        else if (split_mode == SPLIT_QUAD)
        {
            cud+=2;
            cuw >>= 1;
            cuh >>= 1;
            x1 = x + cuw;
            y1 = y + cuh;
            dx = (cuw >> MIN_CU_LOG2);
            dy = (dx * (ctx->max_cuwh >> MIN_CU_LOG2));

            cup1 = cup + dx;
            cup2 = cup + dy;
            cup3 = cup + dx + dy;

            draw_tree(ctx, pic, x, y, cuw, cuh, cud, cup, next_split - 1);
            if (x1 < pic->w_l)
            {
                draw_tree(ctx, pic, x1, y, cuw, cuh, cud, cup1, next_split - 1);
            }
            if (y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x, y1, cuw, cuh, cud, cup2, next_split - 1);
            }
            if (x1 < pic->w_l && y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x1, y1, cuw, cuh, cud, cup3, next_split - 1);
            }
        }
    }
    else
    {
        int     i, s_l;
        s16 * luma;
        /* draw rectangle */
        s_l = pic->s_l;
        luma = pic->y + (y * s_l) + x;

        for (i = 0; i<cuw; i++) luma[i] = sizeof(pel) << 7;
        for (i = 0; i<cuh; i++) luma[i*s_l] = sizeof(pel) << 7;
    }

    return XEVD_OK;
}

void xevd_draw_partition(XEVD_CTX * ctx, XEVD_PIC * pic)
{
    int i, j, k, cuw, cuh, s_l;
    s16 * luma;

    XEVD_PIC * tmp;
    int * ret = NULL;
    char file_name[256];

    tmp = xevdm_picbuf_alloc_exp(ctx->w, ctx->h, pic->pad_l, pic->pad_c, ret, ctx->param.chroma_format_idc);

    cpy_pic(pic, tmp);

    /* CU partition line */
    for (i = 0; i<ctx->h_lcu; i++)
    {
        for (j = 0; j<ctx->w_lcu; j++)
        {
            draw_tree(ctx, tmp, (j << ctx->log2_max_cuwh), (i << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 255);
        }
    }

    s_l = tmp->s_l;
    luma = tmp->y;

    /* LCU boundary line */
    for (i = 0; i<ctx->h; i += ctx->max_cuwh)
    {
        for (j = 0; j<ctx->w; j += ctx->max_cuwh)
        {
            cuw = j + ctx->max_cuwh > ctx->w ? ctx->w - j : ctx->max_cuwh;
            cuh = i + ctx->max_cuwh > ctx->h ? ctx->h - i : ctx->max_cuwh;

            for (k = 0; k<cuw; k++)
            {
                luma[i*s_l + j + k] = 0;
            }

            for (k = 0; k < cuh; k++)
            {
                luma[(i + k)*s_l + j] = 0;
            }
        }
    }

    sprintf(file_name, "dec_partition_%dx%d.yuv", pic->w_l, pic->h_l);

    write_pic(file_name, tmp);
    xevd_picbuf_free(tmp);
}
#endif


void xevdm_get_mmvd_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int real_mv[MMVD_GRP_NUM * MMVD_BASE_MV_NUM * MMVD_MAX_REFINE_NUM][2][3];
    int REF_SET[REFP_NUM][XEVD_MAX_NUM_ACTIVE_REF_FRAME] = { {0,0,}, };
    int cuw, cuh;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    for (int k = 0; k < XEVD_MAX_NUM_ACTIVE_REF_FRAME; k++)
    {
        REF_SET[0][k] = ctx->refp[k][0].poc;
        REF_SET[1][k] = ctx->refp[k][1].poc;
    }

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    xevdm_get_mmvd_mvp_list(ctx->map_refi, ctx->refp[0], ctx->map_mv, ctx->w_scu, ctx->h_scu, core->scup, core->avail_cu, core->log2_cuw, core->log2_cuh, ctx->sh.slice_type, real_mv, ctx->map_scu, REF_SET, core->avail_lr
        , ctx->poc.poc_val, mctx->dpm.num_refp
        , core->history_buffer, ctx->sps->tool_admvp, &ctx->sh, ctx->log2_max_cuwh, ctx->map_tidx, mcore->mmvd_idx);

    core->mv[REFP_0][MV_X] = real_mv[mcore->mmvd_idx][0][MV_X];
    core->mv[REFP_0][MV_Y] = real_mv[mcore->mmvd_idx][0][MV_Y];
    core->refi[REFP_0] = real_mv[mcore->mmvd_idx][0][2];

    if (ctx->sh.slice_type == SLICE_B)
    {
        core->refi[REFP_1] = real_mv[mcore->mmvd_idx][1][2];
        core->mv[REFP_1][MV_X] = real_mv[mcore->mmvd_idx][1][MV_X];
        core->mv[REFP_1][MV_Y] = real_mv[mcore->mmvd_idx][1][MV_Y];
    }
    else if(ctx->sh.slice_type == SLICE_P)
    {
        core->refi[REFP_1] = REFI_INVALID;
    }

}

u8 xevd_check_luma(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_luma_fn(mcore->tree_cons);
}

u8 xevd_check_chroma(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_chroma_fn(mcore->tree_cons);
}
u8 xevd_check_all(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_all_fn(mcore->tree_cons);
}

u8 xevd_check_only_intra(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_only_intra_fn(mcore->tree_cons);
}

u8 xevd_check_only_inter(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_only_inter_fn(mcore->tree_cons);
}

u8 xevd_check_all_preds(XEVD_CTX *ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    return xevd_check_all_preds_fn(mcore->tree_cons);
}

MODE_CONS xevd_derive_mode_cons(XEVD_CTX *ctx, int scup)
{
    return ( MCU_GET_IF(ctx->map_scu[scup]) || MCU_GET_IBC(ctx->map_scu[scup]) ) ? eOnlyIntra : eOnlyInter;
}

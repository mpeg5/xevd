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

#include "xevdm_alf.h"

static void alf_derive_classification_blk(ALF_CLASSIFIER ** classifier, const pel * src_luma, const int src_stride, const AREA * blk, const int shift, int bit_depth)
{
    static const int th[16] = { 0, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4 };
    const int stride = src_stride;
    const pel * src = src_luma;
    const int max_act = 15;

    int fl = 2;
    int flP1 = fl + 1;
    int fl2 = 2 * fl;

    int main_dir, sec_dir, dir_temp_hv, dir_temp_d;

    int pix_y;
    int height = blk->height + fl2;
    int width = blk->width + fl2;
    int pos_x = blk->x;
    int pos_y = blk->y;
    int start_h = pos_y - flP1;
    int laplacian[NUM_DIRECTIONS][CLASSIFICATION_BLK_SIZE + 5][CLASSIFICATION_BLK_SIZE + 5];

    for (int i = 0; i < height; i += 2)
    {
        int y_offset = (i + 1 + start_h) * stride - flP1;
        const pel * src0 = &src[y_offset - stride];
        const pel * src1 = &src[y_offset];
        const pel * src2 = &src[y_offset + stride];
        const pel * src3 = &src[y_offset + stride * 2];

        int * y_ver = laplacian[VER][i];
        int * y_hor = laplacian[HOR][i];
        int * y_dig0 = laplacian[DIAG0][i];
        int * y_dig1 = laplacian[DIAG1][i];

        for (int j = 0; j < width; j += 2)
        {
            pix_y = j + 1 + pos_x;
            const pel * y = src1 + pix_y;
            const pel * y_down = src0 + pix_y;
            const pel * y_up = src2 + pix_y;
            const pel * y_up2 = src3 + pix_y;

            const pel y0 = y[0] << 1;
            const pel y1 = y[1] << 1;
            const pel y_up0 = y_up[0] << 1;
            const pel y_up1 = y_up[1] << 1;

            y_ver[j] = abs(y0 - y_down[0] - y_up[0]) + abs(y1 - y_down[1] - y_up[1]) + abs(y_up0 - y[0] - y_up2[0]) + abs(y_up1 - y[1] - y_up2[1]);
            y_hor[j] = abs(y0 - y[1] - y[-1]) + abs(y1 - y[2] - y[0]) + abs(y_up0 - y_up[1] - y_up[-1]) + abs(y_up1 - y_up[2] - y_up[0]);
            y_dig0[j] = abs(y0 - y_down[-1] - y_up[1]) + abs(y1 - y_down[0] - y_up[2]) + abs(y_up0 - y[-1] - y_up2[1]) + abs(y_up1 - y[0] - y_up2[2]);
            y_dig1[j] = abs(y0 - y_up[-1] - y_down[1]) + abs(y1 - y_up[0] - y_down[2]) + abs(y_up0 - y_up2[-1] - y[1]) + abs(y_up1 - y_up2[0] - y[2]);

            if (j > 4 && (j - 6) % 4 == 0)
            {
                int jM6 = j - 6;
                int jM4 = j - 4;
                int jM2 = j - 2;

                y_ver[jM6] += y_ver[jM4] + y_ver[jM2] + y_ver[j];
                y_hor[jM6] += y_hor[jM4] + y_hor[jM2] + y_hor[j];
                y_dig0[jM6] += y_dig0[jM4] + y_dig0[jM2] + y_dig0[j];
                y_dig1[jM6] += y_dig1[jM4] + y_dig1[jM2] + y_dig1[j];
            }
        }
    }

    // classification block size
    const int cls_size_y = 4;
    const int cls_size_x = 4;

    for (int i = 0; i < blk->height; i += cls_size_y)
    {
        int * y_ver = laplacian[VER][i];
        int * y_ver2 = laplacian[VER][i + 2];
        int * y_ver4 = laplacian[VER][i + 4];
        int * y_ver6 = laplacian[VER][i + 6];

        int * y_hor = laplacian[HOR][i];
        int * y_hor2 = laplacian[HOR][i + 2];
        int * y_hor4 = laplacian[HOR][i + 4];
        int * y_hor6 = laplacian[HOR][i + 6];

        int * y_dig0 = laplacian[DIAG0][i];
        int * y_dig02 = laplacian[DIAG0][i + 2];
        int * y_dig04 = laplacian[DIAG0][i + 4];
        int * y_dig06 = laplacian[DIAG0][i + 6];

        int * y_dig1 = laplacian[DIAG1][i];
        int * y_dig12 = laplacian[DIAG1][i + 2];
        int * y_dig14 = laplacian[DIAG1][i + 4];
        int * y_dig16 = laplacian[DIAG1][i + 6];

        for (int j = 0; j < blk->width; j += cls_size_x)
        {
            int sum_v = y_ver[j] + y_ver2[j] + y_ver4[j] + y_ver6[j];
            int sum_h = y_hor[j] + y_hor2[j] + y_hor4[j] + y_hor6[j];
            int sum_d0 = y_dig0[j] + y_dig02[j] + y_dig04[j] + y_dig06[j];
            int sum_d1 = y_dig1[j] + y_dig12[j] + y_dig14[j] + y_dig16[j];
            int temp_act = sum_v + sum_h;
            int activity = (pel)XEVD_CLIP3(0, max_act, temp_act >> (bit_depth - 2));
            int class_idx = th[activity];
            int hv1, hv0, d1, d0, hvd1, hvd0;

            if (sum_v > sum_h)
            {
                hv1 = sum_v;
                hv0 = sum_h;
                dir_temp_hv = 1;
            }
            else
            {
                hv1 = sum_h;
                hv0 = sum_v;
                dir_temp_hv = 3;
            }
            if (sum_d0 > sum_d1)
            {
                d1 = sum_d0;
                d0 = sum_d1;
                dir_temp_d = 0;
            }
            else
            {
                d1 = sum_d1;
                d0 = sum_d0;
                dir_temp_d = 2;
            }
            if (d1*hv0 > hv1*d0)
            {
                hvd1 = d1;
                hvd0 = d0;
                main_dir = dir_temp_d;
                sec_dir = dir_temp_hv;
            }
            else
            {
                hvd1 = hv1;
                hvd0 = hv0;
                main_dir = dir_temp_hv;
                sec_dir = dir_temp_d;
            }

            int directionStrength = 0;
            if (hvd1 > 2 * hvd0)
            {
                directionStrength = 1;
            }
            if (hvd1 * 2 > 9 * hvd0)
            {
                directionStrength = 2;
            }

            if (directionStrength)
            {
                class_idx += (((main_dir & 0x1) << 1) + directionStrength) * 5;
            }

            static const int trans_tbl[8] = { 0, 1, 0, 2, 2, 3, 1, 3 };
            int trans_idx = trans_tbl[main_dir * 2 + (sec_dir >> 1)];

            int y_offset = i + pos_y;
            int x_offset = j + pos_x;

            ALF_CLASSIFIER *cl0 = classifier[y_offset] + x_offset;
            ALF_CLASSIFIER *cl1 = classifier[y_offset + 1] + x_offset;
            ALF_CLASSIFIER *cl2 = classifier[y_offset + 2] + x_offset;
            ALF_CLASSIFIER *cl3 = classifier[y_offset + 3] + x_offset;
            cl0[0] = cl0[1] = cl0[2] = cl0[3] = cl1[0] = cl1[1] = cl1[2] = cl1[3] = cl2[0] = cl2[1] = cl2[2] = cl2[3] = cl3[0] = cl3[1] = cl3[2] = cl3[3] = ((class_idx << 2) + trans_idx) & 0xFF;
        }
    }
}

static void alf_filter_blk_7(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel* rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range)
{
    const BOOL is_chroma = FALSE;

    const int start_h = blk->y;
    const int end_h = blk->y + blk->height;
    const int start_w = blk->x;
    const int end_w = blk->x + blk->width;

    const pel* src = rec_src;
    pel* dst = rec_dst;

    const pel *img_y_pad0, *img_y_pad1, *img_y_pad2, *img_y_pad3, *img_y_pad4, *img_y_pad5, *img_y_pad6;
    const pel *img0, *img1, *img2, *img3, *img4, *img5, *img6;

    short *coef = filter_set;

    const int shift = 9;
    const int offset = 1 << (shift - 1);

    int trans_idx = 0;
    const int cls_size_y = 4;
    const int cls_size_x = 4;

    CHECK(start_h % cls_size_y, "Wrong start_h in filtering");
    CHECK(start_w % cls_size_x, "Wrong start_w in filtering");
    CHECK((end_h - start_h) % cls_size_y, "Wrong end_h in filtering");
    CHECK((end_w - start_w) % cls_size_x, "Wrong end_w in filtering");

    ALF_CLASSIFIER *alf_class = NULL;

    int dst_stride2 = dst_stride * cls_size_y;
    int src_stride2 = src_stride * cls_size_y;

    pel filter_coef[MAX_NUM_ALF_LUMA_COEFF];
    img_y_pad0 = src;
    img_y_pad1 = img_y_pad0 + src_stride;
    img_y_pad2 = img_y_pad0 - src_stride;
    img_y_pad3 = img_y_pad1 + src_stride;
    img_y_pad4 = img_y_pad2 - src_stride;
    img_y_pad5 = img_y_pad3 + src_stride;
    img_y_pad6 = img_y_pad4 - src_stride;
    pel * rec0 = dst;
    pel * rec1 = rec0 + dst_stride;

    for (int i = 0; i < end_h - start_h; i += cls_size_y)
    {
        if (!is_chroma)
        {
            alf_class = classifier[start_h + i] + start_w;
        }

        for (int j = 0; j < end_w - start_w; j += cls_size_x)
        {
            ALF_CLASSIFIER cl = alf_class[j];
            trans_idx = cl & 0x03;
            coef = filter_set + ((cl >> 2) & 0x1F) * MAX_NUM_ALF_LUMA_COEFF;

            const int l[4][MAX_NUM_ALF_LUMA_COEFF] = {
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
                { 9, 4, 10, 8, 1, 5, 11, 7, 3, 0, 2, 6, 12 },
                { 0, 3, 2, 1, 8, 7, 6, 5, 4, 9, 10, 11, 12 },
                { 9, 8, 10, 4, 3, 7, 11, 5, 1, 0, 2, 6, 12 }
            };

            for (int i = 0; i < MAX_NUM_ALF_LUMA_COEFF; i++)
            {
                filter_coef[i] = coef[l[trans_idx][i]];
            }

            for (int ii = 0; ii < cls_size_y; ii++)
            {
                img0 = img_y_pad0 + j + ii * src_stride;
                img1 = img_y_pad1 + j + ii * src_stride;
                img2 = img_y_pad2 + j + ii * src_stride;
                img3 = img_y_pad3 + j + ii * src_stride;
                img4 = img_y_pad4 + j + ii * src_stride;
                img5 = img_y_pad5 + j + ii * src_stride;
                img6 = img_y_pad6 + j + ii * src_stride;

                rec1 = rec0 + j + ii * dst_stride;

                for (int jj = 0; jj < cls_size_x; jj++)
                {
                    int sum = 0;
                    sum += filter_coef[0] * (img5[0] + img6[0]);

                    sum += filter_coef[1] * (img3[+1] + img4[-1]);
                    sum += filter_coef[2] * (img3[+0] + img4[+0]);
                    sum += filter_coef[3] * (img3[-1] + img4[+1]);

                    sum += filter_coef[4] * (img1[+2] + img2[-2]);
                    sum += filter_coef[5] * (img1[+1] + img2[-1]);
                    sum += filter_coef[6] * (img1[+0] + img2[+0]);
                    sum += filter_coef[7] * (img1[-1] + img2[+1]);
                    sum += filter_coef[8] * (img1[-2] + img2[+2]);

                    sum += filter_coef[9] * (img0[+3] + img0[-3]);
                    sum += filter_coef[10] * (img0[+2] + img0[-2]);
                    sum += filter_coef[11] * (img0[+1] + img0[-1]);
                    sum += filter_coef[12] * (img0[+0]);

                    sum = (sum + offset) >> shift;
                    rec1[jj] = clip_pel(sum, *clip_range);

                    img0++;
                    img1++;
                    img2++;
                    img3++;
                    img4++;
                    img5++;
                    img6++;
                }
            }
        }

        rec0 += dst_stride2;
        rec1 += dst_stride2;

        img_y_pad0 += src_stride2;
        img_y_pad1 += src_stride2;
        img_y_pad2 += src_stride2;
        img_y_pad3 += src_stride2;
        img_y_pad4 += src_stride2;
        img_y_pad5 += src_stride2;
        img_y_pad6 += src_stride2;
    }
}

static void alf_filter_blk_5(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel* rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range)
{
    const int start_h = blk->y;
    const int end_h = blk->y + blk->height;
    const int start_w = blk->x;
    const int end_w = blk->x + blk->width;

    const pel* src = rec_src;
    pel* dst = rec_dst;

    const pel *img_y_pad0, *img_y_pad1, *img_y_pad2, *img_y_pad3, *img_y_pad4;
    const pel *img0, *img1, *img2, *img3, *img4;

    short *coef = filter_set;

    const int shift = 9;
    const int offset = 1 << (shift - 1);

    int trans_idx = 0;
    const int cls_size_y = 1;
    const int cls_size_x = 1;

    ALF_CLASSIFIER *alf_class = NULL;

    int dst_stride2 = dst_stride * cls_size_y;
    int src_stride2 = src_stride * cls_size_y;

    pel filter_coef[MAX_NUM_ALF_LUMA_COEFF];
    img_y_pad0 = src;
    img_y_pad1 = img_y_pad0 + src_stride;
    img_y_pad2 = img_y_pad0 - src_stride;
    img_y_pad3 = img_y_pad1 + src_stride;
    img_y_pad4 = img_y_pad2 - src_stride;
    pel* rec0 = dst;
    pel* rec1 = rec0 + dst_stride;

    for (int i = 0; i < end_h - start_h; i += cls_size_y)
    {
        for (int j = 0; j < end_w - start_w; j += cls_size_x)
        {
            for (int i = 0; i < MAX_NUM_ALF_CHROMA_COEFF; i++)
            {
                filter_coef[i] = coef[i];
            }

            for (int ii = 0; ii < cls_size_y; ii++)
            {
                img0 = img_y_pad0 + j + ii * src_stride;
                img1 = img_y_pad1 + j + ii * src_stride;
                img2 = img_y_pad2 + j + ii * src_stride;
                img3 = img_y_pad3 + j + ii * src_stride;
                img4 = img_y_pad4 + j + ii * src_stride;

                rec1 = rec0 + j + ii * dst_stride;

                for (int jj = 0; jj < cls_size_x; jj++)
                {
                    int sum = 0;

                    sum += filter_coef[0] * (img3[+0] + img4[+0]);

                    sum += filter_coef[1] * (img1[+1] + img2[-1]);
                    sum += filter_coef[2] * (img1[+0] + img2[+0]);
                    sum += filter_coef[3] * (img1[-1] + img2[+1]);

                    sum += filter_coef[4] * (img0[+2] + img0[-2]);
                    sum += filter_coef[5] * (img0[+1] + img0[-1]);
                    sum += filter_coef[6] * (img0[+0]);

                    sum = (sum + offset) >> shift;
                    rec1[jj] = clip_pel(sum, *clip_range);

                    img0++;
                    img1++;
                    img2++;
                    img3++;
                    img4++;
                }
            }
        }

        rec0 += dst_stride2;
        rec1 += dst_stride2;

        img_y_pad0 += src_stride2;
        img_y_pad1 += src_stride2;
        img_y_pad2 += src_stride2;
        img_y_pad3 += src_stride2;
        img_y_pad4 += src_stride2;
    }
}

void xevd_alf_init(ADAPTIVE_LOOP_FILTER * alf, int bit_depth)
{
    alf->clip_ranges.comp[0] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.comp[1] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.comp[2] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.used = FALSE;
    alf->clip_ranges.chroma = FALSE;

    for (int compIdx = 0; compIdx < N_C; compIdx++)
    {
        alf->ctu_enable_flag[compIdx] = NULL;
    }

    alf->derive_classification_blk = alf_derive_classification_blk;
    alf->filter_5x5_blk = alf_filter_blk_5;
    alf->filter_7x7_blk = alf_filter_blk_7;
}

ADAPTIVE_LOOP_FILTER * new_alf(int bit_depth)
{
    ADAPTIVE_LOOP_FILTER * alf = (ADAPTIVE_LOOP_FILTER *)xevd_malloc(sizeof(ADAPTIVE_LOOP_FILTER));
    xevd_mset(alf, 0, sizeof(ADAPTIVE_LOOP_FILTER));
    xevd_alf_init(alf, bit_depth);
    return alf;
}

void delete_alf(ADAPTIVE_LOOP_FILTER* alf)
{
    xevd_mfree(alf);
}

void xevd_alf_init_filter_shape(void* _filter_shape, int size)
{
    ALF_FILTER_SHAPE* filter_shape = (ALF_FILTER_SHAPE*)_filter_shape;

    filter_shape->filterLength = size;
    filter_shape->num_coef = size * size / 4 + 1;
    filter_shape->filter_size = size * size / 2 + 1;

    if (size == 5)
    {
        xevd_mcpy(filter_shape->pattern, pattern5, sizeof(pattern5));
        xevd_mcpy(filter_shape->weights, weights5, sizeof(weights5));
        xevd_mcpy(filter_shape->golombIdx, golombIdx5, sizeof(golombIdx5));
        xevd_mcpy(filter_shape->pattern_to_large_filter, pattern_to_large_filter5, sizeof(pattern_to_large_filter5));
        filter_shape->filter_type = ALF_FILTER_5;
    }
    else if (size == 7)
    {
        xevd_mcpy(filter_shape->pattern, pattern7, sizeof(pattern7));
        xevd_mcpy(filter_shape->weights, weights7, sizeof(weights7));
        xevd_mcpy(filter_shape->golombIdx, golombIdx7, sizeof(golombIdx7));
        xevd_mcpy(filter_shape->pattern_to_large_filter, pattern_to_large_filter7, sizeof(pattern_to_large_filter7));
        filter_shape->filter_type = ALF_FILTER_7;
    }
    else
    {
        filter_shape->filter_type = ALF_NUM_OF_FILTER_TYPES;
        CHECK(0, "Wrong ALF filter shape");
    }
}

int xevd_alf_create(ADAPTIVE_LOOP_FILTER * alf, const int pic_width, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth, int chroma_format_idc, int bit_depth)
{
    const int input_bit_depth[N_C] = { bit_depth, bit_depth };
    int ret = XEVD_OK;
    xevd_mset(alf->alf_idx_in_scan_order, 0, sizeof(u8) * APS_MAX_NUM);
    alf->next_free_alf_idx_in_buf = 0;
    alf->first_idx_poc = INT_MAX;
    alf->last_idr_poc = INT_MAX;
    alf->curr_poc = INT_MAX;
    alf->curr_temp_layer = INT_MAX;
    alf->alf_present_idr = 0;
    alf->alf_idx_idr = INT_MAX;
    alf->ac_alf_line_buf_curr_size = 0;
    alf->last_ras_poc = INT_MAX;
    alf->pending_ras_init = FALSE;

    xevd_mcpy(alf->input_bit_depth, input_bit_depth, sizeof(alf->input_bit_depth));
    alf->pic_width = pic_width;
    alf->pic_height = pic_height;
    alf->max_cu_width = max_cu_width;
    alf->max_cu_height = max_cu_height;
    alf->max_cu_depth = max_cu_depth;
    alf->chroma_format = chroma_format_idc;

    alf->num_ctu_in_widht = (alf->pic_width / alf->max_cu_width) + ((alf->pic_width % alf->max_cu_width) ? 1 : 0);
    alf->num_ctu_in_height = (alf->pic_height / alf->max_cu_height) + ((alf->pic_height % alf->max_cu_height) ? 1 : 0);
    alf->num_ctu_in_pic = alf->num_ctu_in_height * alf->num_ctu_in_widht;

    xevd_alf_init_filter_shape(&alf->filter_shapes[CHANNEL_TYPE_LUMA][0], 5);
    xevd_alf_init_filter_shape(&alf->filter_shapes[CHANNEL_TYPE_LUMA][1], 7);
    xevd_alf_init_filter_shape(&alf->filter_shapes[CHANNEL_TYPE_CHROMA][0], 5);

    alf->temp_buf = (pel*)malloc((pic_width + (7 * alf->num_ctu_in_widht))*(pic_height + (7 * alf->num_ctu_in_height)) * sizeof(pel)); // +7 is of filter diameter //todo: check this
    if (alf->chroma_format)
    {
        alf->temp_buf1 = (pel*)malloc(((pic_width >> 1) + (7 * alf->num_ctu_in_widht))*((pic_height >> 1) + (7 * alf->num_ctu_in_height)) * sizeof(pel)); // for chroma just left for unification
        alf->temp_buf2 = (pel*)malloc(((pic_width >> 1) + (7 * alf->num_ctu_in_widht))*((pic_height >> 1) + (7 * alf->num_ctu_in_height)) * sizeof(pel));
    }
    alf->classifier_mt = (ALF_CLASSIFIER**)malloc(MAX_CU_SIZE * XEVD_MAX_TASK_CNT * sizeof(ALF_CLASSIFIER*));
    xevd_assert_rv(alf->classifier_mt, XEVD_ERR_OUT_OF_MEMORY);
    if (alf->classifier_mt)
    {
        for (int i = 0; i < MAX_CU_SIZE * XEVD_MAX_TASK_CNT; i++)
        {
            alf->classifier_mt[i] = (ALF_CLASSIFIER*)malloc(MAX_CU_SIZE * sizeof(ALF_CLASSIFIER));
            xevd_assert_rv(alf->classifier_mt, XEVD_ERR_OUT_OF_MEMORY);
            xevd_mset(alf->classifier_mt[i], 0, MAX_CU_SIZE * sizeof(ALF_CLASSIFIER));
        }
    }

    // Classification
    alf->classifier = (ALF_CLASSIFIER**)malloc(pic_height * sizeof(ALF_CLASSIFIER*));
    xevd_assert_rv(alf->classifier, XEVD_ERR_OUT_OF_MEMORY);

    for (int i = 0; i < pic_height; i++)
    {
        alf->classifier[i] = (ALF_CLASSIFIER*)malloc(pic_width * sizeof(ALF_CLASSIFIER));
        xevd_assert_rv(alf->classifier[i], XEVD_ERR_OUT_OF_MEMORY);

        xevd_mset(alf->classifier[i], 0, pic_width * sizeof(ALF_CLASSIFIER));
    }

    return XEVD_OK;
}

void xevd_alf_destroy(ADAPTIVE_LOOP_FILTER * alf)
{
    free(alf->temp_buf);
    free(alf->temp_buf1);
    free(alf->temp_buf2);

    if (alf->classifier)
    {
        for (int i = 0; i < alf->pic_height; i++)
        {
            free(alf->classifier[i]);
            alf->classifier[i] = NULL;
        }

        free(alf->classifier);
        alf->classifier = NULL;
    }
    if (alf->classifier_mt)
    {
        for (int i = 0; i < MAX_CU_SIZE * XEVD_MAX_TASK_CNT; i++)
        {
            free(alf->classifier_mt[i]);
            alf->classifier_mt[i] = NULL;
        }
        free(alf->classifier_mt);
        alf->classifier_mt = NULL;
    }
}

static void alf_copy_param(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src)
{
    xevd_mcpy(dst->enable_flag, src->enable_flag, sizeof(BOOL)*N_C);
    dst->chroma_filter_present = src->chroma_filter_present;
    xevd_mcpy(dst->luma_coef, src->luma_coef, sizeof(short)*MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF);
    xevd_mcpy(dst->chroma_coef, src->chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    xevd_mcpy(dst->filter_coef_delta_idx, src->filter_coef_delta_idx, sizeof(short)*MAX_NUM_ALF_CLASSES);
    xevd_mcpy(dst->filter_coef_flag, src->filter_coef_flag, sizeof(BOOL)*MAX_NUM_ALF_CLASSES);
    xevd_mcpy(dst->fixed_filter_idx, src->fixed_filter_idx, sizeof(int)*MAX_NUM_ALF_CLASSES);
    xevd_mcpy(dst->fixed_filter_usage_flag, src->fixed_filter_usage_flag, sizeof(u8)*MAX_NUM_ALF_CLASSES);

    dst->luma_filter_type = src->luma_filter_type;
    dst->num_luma_filters = src->num_luma_filters;
    dst->coef_delta_flag = src->coef_delta_flag;
    dst->coef_delta_pred_mode_flag = src->coef_delta_pred_mode_flag;
    dst->filter_shapes = src->filter_shapes;
    dst->chroma_ctb_present_flag = src->chroma_ctb_present_flag;
    dst->fixed_filter_pattern = src->fixed_filter_pattern;
    dst->temporal_alf_flag = src->temporal_alf_flag;
    dst->prev_idx = src->prev_idx;
    dst->prev_idx_comp[0] = src->prev_idx_comp[0];
    dst->prev_idx_comp[1] = src->prev_idx_comp[1];
    dst->t_layer = src->t_layer;

    dst->filter_poc = src->filter_poc;
    dst->min_idr_poc = src->min_idr_poc;
    dst->max_idr_poc = src->max_idr_poc;
}

static void alf_store_paramline_from_aps(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_param, u8 idx)
{
    assert(idx < APS_MAX_NUM);
    alf_copy_param(&(alf->ac_alf_line_buf[idx]), alf_param);
    alf->ac_alf_line_buf_curr_size++;
    alf->ac_alf_line_buf_curr_size = alf->ac_alf_line_buf_curr_size > APS_MAX_NUM ? APS_MAX_NUM : alf->ac_alf_line_buf_curr_size;  // Increment used ALF circular buffer size
}

void store_dec_aps_to_buffer(XEVD_CTX * ctx)
{
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    ALF_SLICE_PARAM alf_slice_param;
    XEVD_ALF_SLICE_PARAM alf_param = mctx->aps.alf_aps_param;

    //port
    alf_slice_param.filter_shapes = NULL; // pointer will be assigned in ApplyALF;
    alf_slice_param.enable_flag[Y_C] = (alf_param.enabled_flag[0]);
    alf_slice_param.enable_flag[U_C] = (alf_param.enabled_flag[1]);
    alf_slice_param.enable_flag[V_C] = (alf_param.enabled_flag[2]);
    alf_slice_param.chroma_filter_present = alf_param.chroma_filter_present;

    alf_slice_param.num_luma_filters = alf_param.num_luma_filters;
    alf_slice_param.luma_filter_type = (ALF_FILTER_TYPE)(alf_param.luma_filter_type);
    alf_slice_param.chroma_ctb_present_flag = (alf_param.chroma_ctb_present_flag);

    xevd_mcpy(alf_slice_param.filter_coef_delta_idx, alf_param.filter_coeff_delta_idx, MAX_NUM_ALF_CLASSES * sizeof(short));
    xevd_mcpy(alf_slice_param.luma_coef, alf_param.luma_coeff, sizeof(short)*MAX_NUM_ALF_CLASSES*MAX_NUM_ALF_LUMA_COEFF);
    xevd_mcpy(alf_slice_param.chroma_coef, alf_param.chroma_coeff, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    xevd_mcpy(alf_slice_param.fixed_filter_idx, alf_param.fixed_filter_idx, MAX_NUM_ALF_CLASSES * sizeof(int));
    xevd_mcpy(alf_slice_param.fixed_filter_usage_flag, alf_param.fixed_filter_usage_flag, MAX_NUM_ALF_CLASSES * sizeof(u8));
    alf_slice_param.fixed_filter_pattern = alf_param.fixed_filter_pattern;

    alf_slice_param.coef_delta_flag = (alf_param.coeff_delta_flag);
    alf_slice_param.coef_delta_pred_mode_flag = (alf_param.coeff_delta_pred_mode_flag);

    for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
    {
        alf_slice_param.filter_coef_flag[i] = (alf_param.filter_coeff_flag[i]);
    }

    alf_slice_param.prev_idx = alf_param.prev_idx;
    alf_slice_param.prev_idx_comp[0] = alf_param.prev_idx_comp[0];
    alf_slice_param.prev_idx_comp[1] = alf_param.prev_idx_comp[1];
    alf_slice_param.t_layer = alf_param.t_layer;
    alf_slice_param.temporal_alf_flag = (alf_param.temporal_alf_flag);

    const unsigned tidx = ctx->nalu.nuh_temporal_id;

    // Initialize un-used variables at the decoder side  TODO: Modify structure
    alf_slice_param.filter_poc = INT_MAX;
    alf_slice_param.min_idr_poc = INT_MAX;
    alf_slice_param.max_idr_poc = INT_MAX;

    alf_store_paramline_from_aps(mctx->alf, &(alf_slice_param), alf_slice_param.prev_idx);
}

static void alf_param_chroma(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src)
{
    xevd_mcpy(dst->chroma_coef, src->chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    dst->chroma_filter_present = src->chroma_filter_present;
    dst->chroma_ctb_present_flag = src->chroma_ctb_present_flag;
    dst->enable_flag[1] = src->enable_flag[1];
    dst->enable_flag[2] = src->enable_flag[2];

}

static void alf_load_paramline_from_aps_buffer2(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_param, u8 idxY, u8 idxUV, u8 alf_chroma_idc)
{
    alf_copy_param(alf_param, &(alf->ac_alf_line_buf[idxY]));
    assert(alf_param->enable_flag[0] == 1);
    if (alf_chroma_idc)
    {
        alf_param_chroma(alf_param, &(alf->ac_alf_line_buf[idxUV]));
        assert(alf_param->chroma_filter_present == 1);
        alf_param->enable_flag[1] = alf_chroma_idc & 1;
        alf_param->enable_flag[2] = (alf_chroma_idc >> 1) & 1;
    }
    else
    {
        alf_param->enable_flag[1] = 0;
        alf_param->enable_flag[2] = 0;
    }
}

static void alf_recon_coef(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_slice_param, int channel, const BOOL is_rdo, const BOOL is_re_do)
{
    int factor = is_rdo ? 0 : (1 << (NUM_BITS - 1));
    ALF_FILTER_TYPE filter_type = channel == CHANNEL_TYPE_LUMA ? alf_slice_param->luma_filter_type : ALF_FILTER_5;
    int num_classes = channel == CHANNEL_TYPE_LUMA ? MAX_NUM_ALF_CLASSES : 1;
    int num_coef = filter_type == ALF_FILTER_5 ? 7 : 13;
    int num_coef_minus1 = num_coef - 1;
    int num_filters = channel == CHANNEL_TYPE_LUMA ? alf_slice_param->num_luma_filters : 1;
    short* coeff = channel == CHANNEL_TYPE_LUMA ? alf_slice_param->luma_coef : alf_slice_param->chroma_coef;
    if (channel == CHANNEL_TYPE_LUMA)
    {
        if (alf_slice_param->coef_delta_pred_mode_flag)
        {
            for (int i = 1; i < num_filters; i++)
            {
                for (int j = 0; j < num_coef_minus1; j++)
                {
                    coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] += coeff[(i - 1) * MAX_NUM_ALF_LUMA_COEFF + j];
                }
            }
        }

        xevd_mset(alf->coef_final, 0, sizeof(alf->coef_final));
        int num_coef_large_minus1 = MAX_NUM_ALF_LUMA_COEFF - 1;
        for (int class_idx = 0; class_idx < num_classes; class_idx++)
        {
            int filter_idx = alf_slice_param->filter_coef_delta_idx[class_idx];
            int fixed_filter_idx = alf_slice_param->fixed_filter_idx[class_idx];
            u8  fixed_filter_usage_flag = alf_slice_param->fixed_filter_usage_flag[class_idx];
            int fixed_filter_used = fixed_filter_usage_flag;
            int fixed_filter_map_idx = fixed_filter_idx;
            if (fixed_filter_used)
            {
                fixed_filter_idx = alf_class_to_filter_mapping[class_idx][fixed_filter_map_idx];
            }

            for (int i = 0; i < num_coef_large_minus1; i++)
            {
                int cur_coef = 0;
                //fixed filter
                if (fixed_filter_usage_flag > 0)
                {
                    cur_coef = alf_fixed_filter_coef[fixed_filter_idx][i];
                }
                //add coded coeff
                if (alf->filter_shapes[CHANNEL_TYPE_LUMA][filter_type].pattern_to_large_filter[i] > 0)
                {
                    int coeff_idx = alf->filter_shapes[CHANNEL_TYPE_LUMA][filter_type].pattern_to_large_filter[i] - 1;
                    cur_coef += coeff[filter_idx * MAX_NUM_ALF_LUMA_COEFF + coeff_idx];
                }
                if (is_rdo == 0)
                    xevd_assert(cur_coef >= -(1 << 9) && cur_coef <= (1 << 9) - 1);
                alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] = cur_coef;
            }

            //last coeff
            int sum = 0;
            for (int i = 0; i < num_coef_large_minus1; i++)
            {
                sum += (alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] << 1);
            }
            alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] = factor - sum;
            if (is_rdo == 0)
                xevd_assert(alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] >= -(1 << 10) && alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] <= (1 << 10) - 1);
        }

        if (is_re_do && alf_slice_param->coef_delta_pred_mode_flag)
        {
            for (int i = num_filters - 1; i > 0; i--)
            {
                for (int j = 0; j < num_coef_minus1; j++)
                {
                    coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] = coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] - coeff[(i - 1) * MAX_NUM_ALF_LUMA_COEFF + j];
                }
            }
        }
    }
    else
    {
        for (int filter_idx = 0; filter_idx < num_filters; filter_idx++)
        {
            int sum = 0;
            for (int i = 0; i < num_coef_minus1; i++)
            {
                sum += (coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] << 1);
                if (is_rdo == 0)
                    xevd_assert(coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] >= -(1 << 9) && coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] <= (1 << 9) - 1);
            }
            coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] = factor - sum;
            if (is_rdo == 0)
                xevd_assert(coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] >= -(1 << 10) && coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] <= (1 << 10) - 1);
        }
        return;
    }
}

typedef struct _XEVD_ALF_TMP
{
    ADAPTIVE_LOOP_FILTER *alf;
    CODING_STRUCTURE    *cs;
    ALF_SLICE_PARAM      *alf_slice_param;
    int                 tile_idx;
    int                 tsk_num;
} XEVD_ALF_TMP;

static void alf_copy_and_extend_tile(pel* tmp_yuv, const int s, const pel* rec, const int s2, const int w, const int h, const int m)
{
    //copy
    for (int j = 0; j < h; j++)
    {
        xevd_mcpy(tmp_yuv + j * s, rec + j * s2, sizeof(pel) * w);
    }

    //extend
    pel * p = tmp_yuv;
    // do left and right margins
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < m; x++)
        {
            *(p - m + x) = p[0];
            p[w + x] = p[w - 1];
        }
        p += s;
    }

    // p is now the (0,height) (bottom left of image within bigger picture
    p -= (s + m);
    // p is now the (-margin, height-1)
    for (int y = 0; y < m; y++)
    {
        xevd_mcpy(p + (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }

    // pi is still (-marginX, height-1)
    p -= ((h - 1) * s);
    // pi is now (-marginX, 0)
    for (int y = 0; y < m; y++)
    {
        xevd_mcpy(p - (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }
}


static void tile_boundary_check(int* avail_left, int* avail_right, int* avail_top, int* avail_bottom, const int width, const int height, int x_pos, int y_pos, int x_l, int x_r, int y_l, int y_r)
{
    if (x_pos == x_l)
    {
        *avail_left = 0;
    }
    else
    {
        *avail_left = 1;
    }

    if (x_pos + width == x_r)
    {
        *avail_right = 0;
    }
    else
    {
        *avail_right = 1;
    }

    if (y_pos == y_l)
    {
        *avail_top = 0;
    }
    else
    {
        *avail_top = 1;
    }

    if (y_pos + height == y_r)
    {
        *avail_bottom = 0;
    }
    else
    {
        *avail_bottom = 1;
    }
}

static void alf_derive_classification(ADAPTIVE_LOOP_FILTER * alf, ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_luma_stride, const AREA * blk)
{
    int height = blk->y + blk->height;
    int width = blk->x + blk->width;

    for (int i = blk->y; i < height; i += CLASSIFICATION_BLK_SIZE)
    {
        int h = XEVD_MIN(i + CLASSIFICATION_BLK_SIZE, height) - i;

        for (int j = blk->x; j < width; j += CLASSIFICATION_BLK_SIZE)
        {
            int w = XEVD_MIN(j + CLASSIFICATION_BLK_SIZE, width) - j;
            AREA area = { j, i, w, h };
            alf_derive_classification_blk(classifier, src_luma, src_luma_stride, &area, alf->input_bit_depth[CHANNEL_TYPE_LUMA] + 4, alf->input_bit_depth[CHANNEL_TYPE_LUMA]);
        }
    }
}

int alf_process_tile(void * arg)
{
    XEVD_ALF_TMP *input = (XEVD_ALF_TMP*)(arg);
    ADAPTIVE_LOOP_FILTER *alf = input->alf;
    CODING_STRUCTURE* cs = input->cs;
    ALF_SLICE_PARAM* alf_slice_param = input->alf_slice_param;
    int tile_idx = input->tile_idx;
    int tsk_num = input->tsk_num;
    int ii, x_l, x_r, y_l, y_r, w_tile, h_tile;
    const int h = cs->pic->h_l;
    const int w = cs->pic->w_l;
    const int m = MAX_ALF_FILTER_LENGTH >> 1;
    const int s = (w + (7 * alf->num_ctu_in_widht));

    int col_bd = 0;
    u32 k = 0;
    XEVD_CTX* ctx = (XEVD_CTX*)(cs->ctx);
    ii = tile_idx;
    col_bd = 0;
    if (ii % (ctx->pps.num_tile_columns_minus1 + 1))
    {
        int temp = ii - 1;
        while (temp >= 0)
        {
            col_bd += ctx->tile[temp].w_ctb;
            if (!(temp % (ctx->pps.num_tile_columns_minus1 + 1))) break;
            temp--;
        }
    }
    else
    {
        col_bd = 0;
    }
    int x_loc = ((ctx->tile[ii].ctba_rs_first) % ctx->w_lcu);
    int y_loc = ((ctx->tile[ii].ctba_rs_first) / ctx->w_lcu);
    int ctuIdx = x_loc + y_loc * ctx->w_lcu;
    x_l = x_loc << ctx->log2_max_cuwh; //entry point lcu's x location
    y_l = y_loc << ctx->log2_max_cuwh; // entry point lcu's y location
    x_r = x_l + ((int)(ctx->tile[ii].w_ctb) << ctx->log2_max_cuwh);
    y_r = y_l + ((int)(ctx->tile[ii].h_ctb) << ctx->log2_max_cuwh);
    w_tile = x_r > ((int)ctx->w_scu << MIN_CU_LOG2) ? ((int)ctx->w_scu << MIN_CU_LOG2) - x_l : x_r - x_l;
    h_tile = y_r > ((int)ctx->h_scu << MIN_CU_LOG2) ? ((int)ctx->h_scu << MIN_CU_LOG2) - y_l : y_r - y_l;
    x_r = x_r > ((int)ctx->w_scu << MIN_CU_LOG2) ? ((int)ctx->w_scu << MIN_CU_LOG2) : x_r;
    y_r = y_r > ((int)ctx->h_scu << MIN_CU_LOG2) ? ((int)ctx->h_scu << MIN_CU_LOG2) : y_r;

    ALF_CLASSIFIER** classifier_tmp = &alf->classifier_mt[tsk_num*MAX_CU_SIZE];
    pel * recYuv = cs->pic->y;
    pel * tmpYuv = alf->temp_buf;
    tmpYuv += (y_loc * 7)* s + x_loc * 7;
    tmpYuv += s * m + m;
    const int s1 = (w >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (7 * alf->num_ctu_in_widht);
    pel * recYuv1 = cs->pic->u;
    pel * tmpYuv1 = alf->temp_buf1 + s1 * m + m + (y_loc * 7)* s1 + x_loc * 7;
    pel * recYuv2 = cs->pic->v;
    pel * tmpYuv2 = alf->temp_buf2 + s1 * m + m + (y_loc * 7)* s1 + x_loc * 7;
    pel * recLuma0_tile = tmpYuv + x_l + y_l * s;
    pel * recLuma1_tile = tmpYuv1 + (x_l >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (y_l >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * (s1);
    pel * recLuma2_tile = tmpYuv2 + (x_l >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (y_l >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * (s1);
    pel * recoYuv0_tile = recYuv + x_l + y_l * cs->pic->s_l;
    pel * recoYuv1_tile = recYuv1 + (x_l >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (y_l >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * cs->pic->s_c;
    pel * recoYuv2_tile = recYuv2 + (x_l >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (y_l >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * cs->pic->s_c;

    alf_copy_and_extend_tile(recLuma0_tile, s, recoYuv0_tile, cs->pic->s_l, w_tile, h_tile, m);
    if (ctx->sps->chroma_format_idc)
    {
        alf_copy_and_extend_tile(recLuma1_tile, s1, recoYuv1_tile, cs->pic->s_c, (w_tile >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))), (h_tile >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))), m);
        alf_copy_and_extend_tile(recLuma2_tile, s1, recoYuv2_tile, cs->pic->s_c, (w_tile >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))), (h_tile >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))), m);
    }

    int l_zero_offset = (MAX_CU_SIZE + m + m) * m + m;
    int l_stride = MAX_CU_SIZE + 2 * (MAX_ALF_FILTER_LENGTH >> 1);
    pel l_buffer[(MAX_CU_SIZE + 2 * (MAX_ALF_FILTER_LENGTH >> 1)) *(MAX_CU_SIZE + 2 * (MAX_ALF_FILTER_LENGTH >> 1))];
    pel *p_buffer = l_buffer + l_zero_offset;
    int l_zero_offset_chroma = ((MAX_CU_SIZE >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m + m) * m + m;
    int l_stride_chroma = (MAX_CU_SIZE >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + 2 * (MAX_ALF_FILTER_LENGTH >> 1);
    pel l_buffer_cb[((MAX_CU_SIZE)+2 * (MAX_ALF_FILTER_LENGTH >> 1)) *((MAX_CU_SIZE)+2 * (MAX_ALF_FILTER_LENGTH >> 1))];
    pel l_buffer_cr[((MAX_CU_SIZE)+2 * (MAX_ALF_FILTER_LENGTH >> 1)) *((MAX_CU_SIZE)+2 * (MAX_ALF_FILTER_LENGTH >> 1))];
    pel *p_buffer_cr = l_buffer_cr + l_zero_offset_chroma;
    pel *p_buffer_cb = l_buffer_cb + l_zero_offset_chroma;

    for (int yPos = y_l; yPos < y_r; yPos += ctx->max_cuwh)
    {
        for (int xPos = x_l; xPos < x_r; xPos += ctx->max_cuwh)
        {
            const int width = (xPos + ctx->max_cuwh > cs->pic->w_l) ? (cs->pic->w_l - xPos) : ctx->max_cuwh;
            const int height = (yPos + ctx->max_cuwh > cs->pic->h_l) ? (cs->pic->h_l - yPos) : ctx->max_cuwh;

            int availableL, availableR, availableT, availableB;
            availableL = availableR = availableT = availableB = 1;

            if (!(ctx->pps.loop_filter_across_tiles_enabled_flag))
            {
                tile_boundary_check(&availableL, &availableR, &availableT, &availableB, width, height, xPos, yPos, x_l, x_r, y_l, y_r);
            }
            else
            {
                tile_boundary_check(&availableL, &availableR, &availableT, &availableB, width, height, xPos, yPos,
                    0, ctx->sps->pic_width_in_luma_samples - 1, 0, ctx->sps->pic_height_in_luma_samples - 1);
            }
            for (int i = m; i < height + m; i++)
            {
                int dstPos = i * l_stride - l_zero_offset;
                int srcPos_offset = xPos + yPos * s;
                int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                xevd_mcpy(p_buffer + dstPos + m, tmpYuv + srcPos_offset + (i - m) * s, sizeof(pel) * (stride - 2 * m));
                for (int j = 0; j < m; j++)
                {
                    if (availableL)
                    {
                        p_buffer[dstPos + j] = tmpYuv[srcPos_offset + (i - m) * s - m + j];
                    }
                    else
                    {
                        p_buffer[dstPos + j] = tmpYuv[srcPos_offset + (i - m) * s + m - j];
                    }

                    if (availableR)
                    {
                        p_buffer[dstPos + j + width + m] = tmpYuv[srcPos_offset + (i - m) * s + width + j];
                    }
                    else
                    {
                        p_buffer[dstPos + j + width + m] = tmpYuv[srcPos_offset + (i - m) * s + width - j - 2];
                    }
                }
            }

            for (int i = 0; i < m; i++)
            {
                int dstPos = i * l_stride - l_zero_offset;
                int srcPos_offset = xPos + yPos * s;
                int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                if (availableT)
                    xevd_mcpy(p_buffer + dstPos, tmpYuv + srcPos_offset - (m - i) * s - m, sizeof(pel) * stride);
                else
                    xevd_mcpy(p_buffer + dstPos, p_buffer + dstPos + (2 * m - 2 * i) * l_stride, sizeof(pel) * stride);
            }

            for (int i = height + m; i < height + m + m; i++)
            {
                int dstPos = i * l_stride - l_zero_offset;
                int srcPos_offset = xPos + yPos * s;
                int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                if (availableB)
                {
                    xevd_mcpy(p_buffer + dstPos, tmpYuv + srcPos_offset + (i - m) * s - m, sizeof(pel) * stride);
                }
                else
                {
                    xevd_mcpy(p_buffer + dstPos, p_buffer + dstPos - (2 * (i - height - m) + 2) * l_stride, sizeof(pel) * stride);
                }
            }
            if (alf->ctu_enable_flag[Y_C][ctuIdx])
            {
                AREA blk = { 0, 0, width, height };
                alf_derive_classification(alf, classifier_tmp, p_buffer, l_stride, &blk);
                alf->filter_7x7_blk(classifier_tmp, recYuv + xPos + yPos * (cs->pic->s_l), cs->pic->s_l, p_buffer, l_stride, &blk, Y_C, alf->coef_final, &(alf->clip_ranges.comp[Y_C]));
            }

            if (ctx->sps->chroma_format_idc)
            {
                for (int i = m; i < ((height >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) + m); i++)
                {
                    int dstPos = i * l_stride_chroma - l_zero_offset_chroma;
                    int srcPos_offset = (xPos >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (yPos >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * s1;
                    int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m + m);
                    xevd_mcpy(p_buffer_cb + dstPos + m, tmpYuv1 + srcPos_offset + (i - m) * s1, sizeof(pel) * (stride - 2 * m));
                    xevd_mcpy(p_buffer_cr + dstPos + m, tmpYuv2 + srcPos_offset + (i - m) * s1, sizeof(pel) * (stride - 2 * m));
                    for (int j = 0; j < m; j++)
                    {
                        if (availableL)
                        {
                            p_buffer_cb[dstPos + j] = tmpYuv1[srcPos_offset + (i - m) * s1 - m + j];
                            p_buffer_cr[dstPos + j] = tmpYuv2[srcPos_offset + (i - m) * s1 - m + j];
                        }
                        else
                        {
                            p_buffer_cb[dstPos + j] = tmpYuv1[srcPos_offset + (i - m) * s1 + m - j];
                            p_buffer_cr[dstPos + j] = tmpYuv2[srcPos_offset + (i - m) * s1 + m - j];
                        }
                        if (availableR)
                        {
                            p_buffer_cb[dstPos + j + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m] = tmpYuv1[srcPos_offset + (i - m) * s1 + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + j];
                            p_buffer_cr[dstPos + j + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m] = tmpYuv2[srcPos_offset + (i - m) * s1 + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + j];
                        }
                        else
                        {
                            p_buffer_cb[dstPos + j + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m] = tmpYuv1[srcPos_offset + (i - m) * s1 + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) - j - 2];
                            p_buffer_cr[dstPos + j + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m] = tmpYuv2[srcPos_offset + (i - m) * s1 + (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) - j - 2];
                        }
                    }
                }

                for (int i = 0; i < m; i++)
                {
                    int dstPos = i * l_stride_chroma - l_zero_offset_chroma;
                    int srcPos_offset = (xPos >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (yPos >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * s1;
                    int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m + m);
                    if (availableT)
                    {
                        xevd_mcpy(p_buffer_cb + dstPos, tmpYuv1 + srcPos_offset - (m - i) * s1 - m, sizeof(pel) * stride);
                    }
                    else
                    {
                        xevd_mcpy(p_buffer_cb + dstPos, p_buffer_cb + dstPos + (2 * m - 2 * i) * l_stride_chroma, sizeof(pel) * stride);
                    }
                    if (availableT)
                    {
                        xevd_mcpy(p_buffer_cr + dstPos, tmpYuv2 + srcPos_offset - (m - i) * s1 - m, sizeof(pel) * stride);
                    }
                    else
                    {
                        xevd_mcpy(p_buffer_cr + dstPos, p_buffer_cr + dstPos + (2 * m - 2 * i) * l_stride_chroma, sizeof(pel) * stride);
                    }
                }
                for (int i = ((height >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) + m); i < ((height >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) + m + m); i++)
                {
                    int dstPos = i * l_stride_chroma - l_zero_offset_chroma;
                    int srcPos_offset = (xPos >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + (yPos >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * s1;
                    int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) + m + m);
                    if (availableB)
                    {
                        xevd_mcpy(p_buffer_cb + dstPos, tmpYuv1 + srcPos_offset + (i - m) * s1 - m, sizeof(pel) * stride);
                    }
                    else
                    {
                        xevd_mcpy(p_buffer_cb + dstPos, p_buffer_cb + dstPos - (2 * (i - (height >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) - m) + 2) * l_stride_chroma, sizeof(pel) * stride);
                    }
                    if (availableB)
                    {
                        xevd_mcpy(p_buffer_cr + dstPos, tmpYuv2 + srcPos_offset + (i - m) * s1 - m, sizeof(pel) * stride);
                    }
                    else
                    {
                        xevd_mcpy(p_buffer_cr + dstPos, p_buffer_cr + dstPos - (2 * (i - (height >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) - m) + 2) * l_stride_chroma, sizeof(pel) * stride);
                    }
                }
            }
            for (int compIdx = 1; compIdx < N_C; compIdx++)
            {
                COMPONENT_ID compID = (COMPONENT_ID)(compIdx);
                const int chromaScaleX = (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
                const int chromaScaleY = (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
                if (alf_slice_param->enable_flag[compIdx])
                {
                    xevd_assert(alf->ctu_enable_flag[compIdx][ctuIdx] == 1);
                    AREA blk = { 0, 0, width >> chromaScaleX, height >> chromaScaleY };

                    if (compIdx == 1)
                        alf->filter_5x5_blk(classifier_tmp, recYuv1 + (xPos >> chromaScaleX) + (yPos >> chromaScaleY) * (cs->pic->s_c), cs->pic->s_c, p_buffer_cb, l_stride_chroma, &blk, compID, alf_slice_param->chroma_coef, &(alf->clip_ranges.comp[compIdx]));
                    else if (compIdx == 2)
                        alf->filter_5x5_blk(classifier_tmp, recYuv2 + (xPos >> chromaScaleX) + (yPos >> chromaScaleY) * (cs->pic->s_c), cs->pic->s_c, p_buffer_cr, l_stride_chroma, &blk, compID, alf_slice_param->chroma_coef, &(alf->clip_ranges.comp[compIdx]));
                }
            }
            x_loc++;
            if (x_loc >= ctx->tile[ii].w_ctb + col_bd)
            {
                x_loc = ((ctx->tile[ii].ctba_rs_first) % ctx->w_lcu);
                y_loc++;
            }
            ctuIdx = x_loc + y_loc * ctx->w_lcu;
        }
    }
    return XEVD_OK;
}

void alf_process(ADAPTIVE_LOOP_FILTER *alf, CODING_STRUCTURE* cs, ALF_SLICE_PARAM* alf_slice_param)
{
    if (!alf_slice_param->enable_flag[Y_C] && !alf_slice_param->enable_flag[U_C] && !alf_slice_param->enable_flag[V_C])
    {
        return;
    }

    XEVD_CTX* ctx = (XEVD_CTX*)(cs->ctx);

    // set available filter shapes
    alf_slice_param->filter_shapes = &alf->filter_shapes[0];

    // set CTU enable flags
    for (int compIdx = 0; compIdx < N_C; compIdx++)
    {
        alf->ctu_enable_flag[compIdx] = alf_slice_param->alf_ctb_flag + ctx->f_lcu * compIdx;
    }

    alf_recon_coef(alf, alf_slice_param, CHANNEL_TYPE_LUMA, FALSE, TRUE);
    if (alf_slice_param->enable_flag[U_C] || alf_slice_param->enable_flag[V_C])
    {
        alf_recon_coef(alf, alf_slice_param, CHANNEL_TYPE_CHROMA, FALSE, FALSE);
    }

    int ii;
    const int h = cs->pic->h_l;
    const int w = cs->pic->w_l;
    const int m = MAX_ALF_FILTER_LENGTH >> 1;
    const int s = w + m + m;
    int col_bd = 0;
    u32 k = 0;
    ii = 0;
    int num_tiles_in_slice = ctx->w_tile * ctx->h_tile;
    XEVD_ALF_TMP alf_tmp[DEC_XEVD_MAX_TASK_CNT];
    int i;
    int(*tile_alf)(void * arg) = alf_process_tile;
    int res = 0;
    int j;
    int tile_start_num, num_tiles_proc;
    int        ret;
    ret = XEVD_OK;

    while (num_tiles_in_slice)
    {
        if (num_tiles_in_slice > ctx->tc.max_task_cnt)
        {
            tile_start_num = k;
            num_tiles_proc = ctx->tc.max_task_cnt;
        }
        else
        {
            tile_start_num = k;
            num_tiles_proc = num_tiles_in_slice;
        }
        for (j = 1; j < num_tiles_proc; j++)
        {
            alf_tmp[j].alf_slice_param = alf_slice_param;
            alf_tmp[j].cs = cs;
            alf_tmp[j].alf = alf;
            alf_tmp[j].tile_idx = tile_start_num + j - 1;

            alf_tmp[j].tsk_num = j;

            ret = ctx->tc.run(ctx->thread_pool[j], tile_alf, (void *)(&alf_tmp[j]));
            k++;
        }
        j = tile_start_num + num_tiles_proc - 1;
        i = ctx->tile_in_slice[j];
        alf_tmp[0].alf_slice_param = alf_slice_param;
        alf_tmp[0].cs = cs;
        alf_tmp[0].alf = alf;
        alf_tmp[0].tile_idx = j;
        alf_tmp[0].tsk_num = 0;

        ret = tile_alf((void *)(&alf_tmp[0]));
        k++;
        for (i = 1; i < num_tiles_proc; i++)
        {
            ret = ctx->tc.join(ctx->thread_pool[i], &res);
        }
        num_tiles_in_slice -= (num_tiles_proc);
    }
}

int call_dec_alf_process_aps(ADAPTIVE_LOOP_FILTER* alf, XEVD_CTX * ctx, XEVD_PIC * pic)
{
    CODING_STRUCTURE cs;
    cs.ctx = (void *)ctx;
    cs.pic = pic;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    ALF_SLICE_PARAM alf_slice_param;
    alf_slice_param.alf_ctb_flag = (u8 *)malloc(N_C * ctx->f_lcu * sizeof(u8));
    if (alf_slice_param.alf_ctb_flag == NULL)
        return XEVD_ERR;
    xevd_mset(alf_slice_param.alf_ctb_flag, 0, N_C * ctx->f_lcu * sizeof(u8));
    // load filter from buffer
    alf_load_paramline_from_aps_buffer2(alf, &(alf_slice_param), mctx->sh.aps_id_y, mctx->sh.aps_id_ch, mctx->sh.alf_chroma_idc);

    // load filter map buffer
    alf_slice_param.is_ctb_alf_on = mctx->sh.alf_sh_param.is_ctb_alf_on;
    xevd_mcpy(alf_slice_param.alf_ctb_flag, mctx->sh.alf_sh_param.alf_ctu_enable_flag, N_C * ctx->f_lcu * sizeof(u8));
    alf_process(alf, &cs, &alf_slice_param);
    if (alf_slice_param.alf_ctb_flag)
        free(alf_slice_param.alf_ctb_flag);

    return XEVD_OK;
}

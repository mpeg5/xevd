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
#include "xevdm_tbl.h"
#include "xevd_eco.h"

#include <math.h>
#include "xevdm_alf.h"

static u32 sbac_decode_bin_ep(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    u32 bin, t0;

    sbac->range >>= 1;

    if(sbac->value >= sbac->range)
    {
        bin = 1;
        sbac->value -= sbac->range;
    }
    else
    {
        bin = 0;
    }

    sbac->range <<= 1;
#if TRACE_HLS
    xevd_bsr_read1_trace(bs, &t0, 0);
#else
    xevd_bsr_read1(bs, &t0);
#endif
    sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;

    return bin;
}

static u32 sbac_decode_bins_ep(XEVD_BSR *bs, XEVD_SBAC *sbac, int num_bin)
{
    int bin = 0;
    u32 value = 0;

    for(bin = num_bin - 1; bin >= 0; bin--)
    {
        if(sbac_decode_bin_ep(bs, sbac))
        {
            value += (1 << bin);
        }
    }

    return value;
}

static u32 sbac_read_unary_sym(XEVD_BSR * bs, XEVD_SBAC * sbac, SBAC_CTX_MODEL * model, u32 num_ctx)
{
    u32 ctx_idx = 0;
    u32 t32u;
    u32 symbol;

    symbol = xevd_sbac_decode_bin(bs, sbac, model);

    if(symbol == 0)
    {
        return symbol;
    }

    symbol = 0;
    do
    {
        if(ctx_idx < num_ctx - 1)
        {
            ctx_idx++;
        }
        t32u = xevd_sbac_decode_bin(bs, sbac, &model[ctx_idx]);
        symbol++;
    } while(t32u);

    return symbol;
}

static u32 sbac_read_truncate_unary_sym(XEVD_BSR * bs, XEVD_SBAC * sbac, SBAC_CTX_MODEL * model, u32 num_ctx, u32 max_num)
{
    u32 ctx_idx = 0;
    u32 t32u;
    u32 symbol;

    if(max_num > 1)
    {
        for(; ctx_idx < max_num - 1; ++ctx_idx)
        {
            symbol = xevd_sbac_decode_bin(bs, sbac, model + (ctx_idx > num_ctx - 1 ? num_ctx - 1 : ctx_idx));
            if(symbol == 0)
            {
                break;
            }
        }
    }
    t32u = ctx_idx;

    return t32u;
}

static int xevdm_eco_ats_inter_info(XEVD_BSR * bs, XEVD_SBAC * sbac, int log2_cuw, int log2_cuh, u8* ats_inter_info, u8 ats_inter_avail)
{
    u8 mode_vert = (ats_inter_avail >> 0) & 0x1;
    u8 mode_hori = (ats_inter_avail >> 1) & 0x1;
    u8 mode_vert_quad = (ats_inter_avail >> 2) & 0x1;
    u8 mode_hori_quad = (ats_inter_avail >> 3) & 0x1;
    u8 num_ats_inter_mode_avail = mode_vert + mode_hori + mode_vert_quad + mode_hori_quad;

    if (num_ats_inter_mode_avail == 0)
    {
        *ats_inter_info = 0;
        return XEVD_OK;
    }
    else
    {
        u8 ats_inter_flag = 0;
        u8 ats_inter_hor = 0;
        u8 ats_inter_quad = 0;
        u8 ats_inter_pos = 0;
        int size = 1 << (log2_cuw + log2_cuh);
        u8 ctx_ats_inter = sbac->ctx.sps_cm_init_flag == 1 ? ((log2_cuw + log2_cuh >= 8) ? 0 : 1) : 0;
        u8 ctx_ats_inter_hor = sbac->ctx.sps_cm_init_flag == 1 ? ((log2_cuw == log2_cuh) ? 0 : (log2_cuw < log2_cuh ? 1 : 2)) : 0;

        XEVD_SBAC_CTX * sbac_ctx;
        sbac_ctx = &sbac->ctx;

        ats_inter_flag = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->ats_cu_inter_flag + ctx_ats_inter);
        XEVD_TRACE_STR("ats_inter_flag ");
        XEVD_TRACE_INT(ats_inter_flag);
        XEVD_TRACE_STR("\n");

        if (ats_inter_flag)
        {
            if ((mode_vert_quad || mode_hori_quad) && (mode_vert || mode_hori))
            {
                ats_inter_quad = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->ats_cu_inter_quad_flag);
                XEVD_TRACE_STR("ats_inter_quad ");
                XEVD_TRACE_INT(ats_inter_quad);
                XEVD_TRACE_STR("\n");
            }
            else
            {
                ats_inter_quad = 0;
            }

            if ((ats_inter_quad && mode_vert_quad && mode_hori_quad) || (!ats_inter_quad && mode_vert && mode_hori))
            {
                ats_inter_hor = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->ats_cu_inter_hor_flag + ctx_ats_inter_hor);
                XEVD_TRACE_STR("ats_inter_hor ");
                XEVD_TRACE_INT(ats_inter_hor);
                XEVD_TRACE_STR("\n");
            }
            else
            {
                ats_inter_hor = (ats_inter_quad && mode_hori_quad) || (!ats_inter_quad && mode_hori);
            }

            ats_inter_pos = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->ats_cu_inter_pos_flag);
            XEVD_TRACE_STR("ats_inter_pos ");
            XEVD_TRACE_INT(ats_inter_pos);
            XEVD_TRACE_STR("\n");
        }
        *ats_inter_info = get_ats_inter_info((ats_inter_quad ? 2 : 0) + (ats_inter_hor ? 1 : 0) + ats_inter_flag, ats_inter_pos);

        return XEVD_OK;
    }
}

static int xevdm_eco_cbf(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 pred_mode, u8 cbf[N_C], int b_no_cbf, int is_sub, int sub_pos, int *cbf_all, TREE_CONS tree_cons, int chroma_format_idc)
{
    XEVD_SBAC_CTX * sbac_ctx;
    sbac_ctx = &sbac->ctx;

    /* decode allcbf */
    if (pred_mode != MODE_INTRA && tree_cons.tree_type == TREE_LC)
    {
        if (b_no_cbf == 0 && sub_pos == 0)
        {
            if (xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_all) == 0)
            {
                *cbf_all = 0;
                cbf[Y_C] = cbf[U_C] = cbf[V_C] = 0;

                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("all_cbf ");
                XEVD_TRACE_INT(0);
                XEVD_TRACE_STR("\n");

                return 1;
            }
            else
            {
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("all_cbf ");
                XEVD_TRACE_INT(1);
                XEVD_TRACE_STR("\n");
            }
        }
        if(chroma_format_idc != 0)
        {
        cbf[U_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cb);
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("cbf U ");
        XEVD_TRACE_INT(cbf[U_C]);
        XEVD_TRACE_STR("\n");

        cbf[V_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cr);
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("cbf V ");
        XEVD_TRACE_INT(cbf[V_C]);
        XEVD_TRACE_STR("\n");
        }
        else
        {
            cbf[U_C] = 0;
            cbf[V_C] = 0;
        }

        if (cbf[U_C] + cbf[V_C] == 0 && !is_sub)
        {
            cbf[Y_C] = 1;
        }
        else
        {
            cbf[Y_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_luma);
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("cbf Y ");
            XEVD_TRACE_INT(cbf[Y_C]);
            XEVD_TRACE_STR("\n");
        }
    }
    else
    {
        if(xevd_check_chroma_fn(tree_cons) && chroma_format_idc != 0)
        {
            cbf[U_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cb);
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("cbf U ");
            XEVD_TRACE_INT(cbf[U_C]);
            XEVD_TRACE_STR("\n");

            cbf[V_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cr);
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("cbf V ");
            XEVD_TRACE_INT(cbf[V_C]);
            XEVD_TRACE_STR("\n");
        }
        else
        {
            cbf[U_C] = cbf[V_C] = 0;
        }
        if (xevd_check_luma_fn(tree_cons))
        {
            cbf[Y_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_luma);
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("cbf Y ");
            XEVD_TRACE_INT(cbf[Y_C]);
            XEVD_TRACE_STR("\n");
        }
        else
        {
            cbf[Y_C] = 0;
        }
    }

    return 0;
}

static int xevdm_eco_run_length_cc(XEVD_CTX * ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type)
{
    XEVD_SBAC_CTX *sbac_ctx;
    int            sign, level, prev_level, run, last_flag;
    int            t0, scan_pos_offset, num_coeff, i, coef_cnt = 0;
    const u16     *scanp;
    int            ctx_last = 0;

    sbac_ctx = &sbac->ctx;
    scanp = ctx->scan_tables->xevd_scan_tbl[COEF_SCAN_ZIGZAG][log2_w - 1][log2_h - 1];
    num_coeff = 1 << (log2_w + log2_h);
    scan_pos_offset = 0;
    prev_level = 6;

    do
    {
        t0 = sbac->ctx.sps_cm_init_flag == 1 ? ((XEVD_MIN(prev_level - 1, 5)) << 1) + (ch_type == Y_C ? 0 : 12) : (ch_type == Y_C ? 0 : 2);

        /* Run parsing */
        run = sbac_read_unary_sym(bs, sbac, sbac_ctx->run + t0, 2);
        for(i = scan_pos_offset; i < scan_pos_offset + run; i++)
        {
            coef[scanp[i]] = 0;
        }
        scan_pos_offset += run;

        /* Level parsing */
        level = sbac_read_unary_sym(bs, sbac, sbac_ctx->level + t0, 2);
        level++;
        prev_level = level;

        /* Sign parsing */
        sign = sbac_decode_bin_ep(bs, sbac);
        coef[scanp[scan_pos_offset]] = sign ? -(s16)level : (s16)level;

        coef_cnt++;

        if(scan_pos_offset >= num_coeff - 1)
        {
            break;
        }
        scan_pos_offset++;

        /* Last flag parsing */
        ctx_last = (ch_type == Y_C) ? 0 : 1;
        last_flag = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->last + ctx_last);
    } while(!last_flag);

    return XEVD_OK;
}

static int xevdm_eco_ats_intra_cu(XEVD_BSR* bs, XEVD_SBAC* sbac)
{
    u32 t0;
    t0 = sbac_decode_bin_ep(bs, sbac);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("ats intra CU ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");

    return t0;
}

static int xevdm_eco_ats_mode_h(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    u32 t0;

    t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.ats_mode);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("ats intra tuH ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");

    return t0;
}

static int xevdm_eco_ats_mode_v(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    u32 t0;

    t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.ats_mode);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("ats intra tuV ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");

    return t0;
}

static void xevdm_parse_positionLastXY(XEVD_BSR *bs, XEVD_SBAC *sbac, int* last_x, int* last_y, int width, int height, int ch_type)
{
    XEVD_SBAC_CTX *sbac_ctx = &sbac->ctx;
    SBAC_CTX_MODEL* cm_x = sbac_ctx->last_sig_coeff_x_prefix + (ch_type == Y_C ? 0 : (sbac->ctx.sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));
    SBAC_CTX_MODEL* cm_y = sbac_ctx->last_sig_coeff_y_prefix + (ch_type == Y_C ? 0 : (sbac->ctx.sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));
    int last;
    int blk_offset_x, blk_offset_y, shift_x, shift_y;
    int pos_x, pos_y;
    int i, cnt, tmp;
    if (sbac->ctx.sps_cm_init_flag == 1)
    {
        xevd_get_ctx_last_pos_xy_para(ch_type, width, height, &blk_offset_x, &blk_offset_y, &shift_x, &shift_y);
    }
    else
    {
        blk_offset_x = 0;
        blk_offset_y = 0;
        shift_x = 0;
        shift_y = 0;
    }
    // last_sig_coeff_x_prefix
    for (pos_x = 0; pos_x < g_group_idx[width - 1]; pos_x++)
    {
        last = xevd_sbac_decode_bin(bs, sbac, cm_x + blk_offset_x + (pos_x >> shift_x));
        if (!last)
        {
            break;
        }
    }

    // last_sig_coeff_y_prefix
    for (pos_y = 0; pos_y < g_group_idx[height - 1]; pos_y++)
    {
        last = xevd_sbac_decode_bin(bs, sbac, cm_y + blk_offset_y + (pos_y >> shift_y));
        if (!last)
        {
            break;
        }
    }

    // last_sig_coeff_x_suffix
    if (pos_x > 3)
    {
        tmp = 0;
        cnt = (pos_x - 2) >> 1;
        for (i = cnt - 1; i >= 0; i--)
        {
            last = sbac_decode_bin_ep(bs, sbac);
            tmp += last << i;
        }

        pos_x = g_min_in_group[pos_x] + tmp;
    }
    // last_sig_coeff_y_suffix
    if (pos_y > 3)
    {
        tmp = 0;
        cnt = (pos_y - 2) >> 1;
        for (i = cnt - 1; i >= 0; i--)
        {
            last = sbac_decode_bin_ep(bs, sbac);
            tmp += last << i;
        }
        pos_y = g_min_in_group[pos_y] + tmp;
    }

    *last_x = pos_x;
    *last_y = pos_y;
}
static int xevdm_parse_coef_remain_exgolomb(XEVD_BSR *bs, XEVD_SBAC *sbac, int rparam)
{
    int symbol = 0;
    int prefix = 0;
    int code_word = 0;

    do
    {
        prefix++;
        code_word = sbac_decode_bin_ep(bs, sbac);
    } while (code_word);

    code_word = 1 - code_word;
    prefix -= code_word;
    code_word = 0;
    if (prefix < g_go_rice_range[rparam])
    {
        code_word = sbac_decode_bins_ep(bs, sbac, rparam);
        symbol = (prefix << rparam) + code_word;
    }
    else
    {
        code_word = sbac_decode_bins_ep(bs, sbac, prefix - g_go_rice_range[rparam] + rparam);
        symbol = (((1 << (prefix - g_go_rice_range[rparam])) + g_go_rice_range[rparam] - 1) << rparam) + code_word;
    }

    return symbol;
}
static int xevdm_eco_adcc(XEVD_CTX * ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type)
{
    int width = 1 << log2_w;
    int height = 1 << log2_h;
    int offset0;
    SBAC_CTX_MODEL* cm_sig_coeff;
    SBAC_CTX_MODEL* cm_gtAB;
    int scan_type = COEF_SCAN_ZIGZAG;

    int log2_block_size = XEVD_MIN(log2_w, log2_h);
    u16 *scan;
    u16 *scan_inv;
    int scan_pos_last = -1;
    int last_x = 0, last_y = 0;
    int num_coeff;
    int ipos;
    int last_scan_set;
    int rice_param;
    int sub_set;
    int ctx_sig_coeff = 0;
    int cg_log2_size = LOG2_CG_SIZE;
    int is_last_nz = 0;
    int pos_last = 0;
    int cnt_gtA = 0;
    int cnt_gtB = 0;
    int ctx_gtA = 0;
    int ctx_gtB = 0;
    int escape_data_present_ingroup = 0;
    int blkpos, sx, sy;
    u32 sig_coeff_flag;

    // decode last position
    xevdm_parse_positionLastXY(bs, sbac, &last_x, &last_y, width, height, ch_type);
    int max_num_coef = width * height;

    scan = ctx->scan_tables->xevd_scan_tbl[COEF_SCAN_ZIGZAG][log2_w - 1][log2_h - 1];
    scan_inv = ctx->scan_tables->xevd_inv_scan_tbl[COEF_SCAN_ZIGZAG][log2_w - 1][log2_h - 1];

    int last_pos_in_raster = last_x + last_y * width;
    int last_pos_in_scan = scan_inv[last_pos_in_raster];
    num_coeff = last_pos_in_scan + 1;
    //===== code significance flag =====
    offset0 = log2_block_size <= 2 ? 0 : NUM_CTX_SIG_COEFF_LUMA_TU << (XEVD_MIN(1, (log2_block_size - 3)));
    if (sbac->ctx.sps_cm_init_flag == 1)
    {
        cm_sig_coeff = (ch_type == Y_C) ? sbac->ctx.sig_coeff_flag + offset0 : sbac->ctx.sig_coeff_flag + NUM_CTX_SIG_COEFF_LUMA;
        cm_gtAB = (ch_type == Y_C) ? sbac->ctx.coeff_abs_level_greaterAB_flag : sbac->ctx.coeff_abs_level_greaterAB_flag + NUM_CTX_GTX_LUMA;
    }
    else
    {
        cm_sig_coeff = (ch_type == Y_C) ? sbac->ctx.sig_coeff_flag : sbac->ctx.sig_coeff_flag + 1;
        cm_gtAB = (ch_type == Y_C) ? sbac->ctx.coeff_abs_level_greaterAB_flag : sbac->ctx.coeff_abs_level_greaterAB_flag + 1;
    }
    last_scan_set = (num_coeff - 1) >> cg_log2_size;
    scan_pos_last = num_coeff - 1;

    rice_param = 0;
    ipos = scan_pos_last;

    for (sub_set = last_scan_set; sub_set >= 0; sub_set--)
    {
        int num_nz = 0;
        int sub_pos = sub_set << cg_log2_size;
        int abs_coef[1 << LOG2_CG_SIZE ];
        int pos[1 << LOG2_CG_SIZE];
        int last_nz_pos_in_cg = -1;
        int first_nz_pos_in_cg = 1 << cg_log2_size;

        {
            for (; ipos >= sub_pos; ipos--)
            {
                blkpos = scan[ipos];
                sx = blkpos & (width - 1);
                sy = blkpos >> log2_w;

                // sigmap
                if (ipos == scan_pos_last)
                {
                    ctx_sig_coeff = 0;
                }
                else
                {
                    ctx_sig_coeff = sbac->ctx.sps_cm_init_flag == 1 ? xevdm_get_ctx_sig_coeff_inc(coef, blkpos, width, height, ch_type) : 0;
                }

                if (!(ipos == scan_pos_last)) // skipping signaling flag for last, we know it is non-zero
                {
                    sig_coeff_flag = xevd_sbac_decode_bin(bs, sbac, cm_sig_coeff + ctx_sig_coeff);
                }
                else
                {
                    sig_coeff_flag = 1;
                }
                coef[blkpos] = sig_coeff_flag;

                if (sig_coeff_flag)
                {
                    pos[num_nz] = blkpos;
                    num_nz++;

                    if (last_nz_pos_in_cg == -1)
                    {
                        last_nz_pos_in_cg = ipos;
                    }
                    first_nz_pos_in_cg = ipos;

                    if (is_last_nz == 0)
                    {
                        pos_last = blkpos;
                        is_last_nz = 1;
                    }
                }
            }
            if (num_nz > 0)
            {
                int i, idx;
                u32 coef_signs_group;
                int c2_idx = 0;
                escape_data_present_ingroup = 0;

                for (i = 0; i < num_nz; i++)
                {
                    abs_coef[i] = 1;
                }
                int numC1Flag = XEVD_MIN(num_nz, CAFLAG_NUMBER);
                int firstC2FlagIdx = -1;

                for (int idx = 0; idx < numC1Flag; idx++)
                {
                    if (pos[idx] != pos_last)
                    {
                        ctx_gtA = sbac->ctx.sps_cm_init_flag == 1 ? xevdm_get_ctx_gtA_inc(coef, pos[idx], width, height, ch_type) : 0;
                    }
                    u32 coeff_abs_level_greaterA_flag = xevd_sbac_decode_bin(bs, sbac, cm_gtAB + ctx_gtA);
                    coef[pos[idx]] += coeff_abs_level_greaterA_flag;
                    abs_coef[idx] = coeff_abs_level_greaterA_flag + 1;
                    if (coeff_abs_level_greaterA_flag == 1)
                    {
                        if (firstC2FlagIdx == -1)
                        {
                            firstC2FlagIdx = idx;
                        }
                        else //if a greater-than-one has been encountered already this group
                        {
                            escape_data_present_ingroup = TRUE;
                        }
                    }
                }
                if (firstC2FlagIdx != -1)
                {
                    if (pos[firstC2FlagIdx] != pos_last)
                    {
                        ctx_gtB = sbac->ctx.sps_cm_init_flag == 1 ? xevdm_get_ctx_gtB_inc(coef, pos[firstC2FlagIdx], width, height, ch_type) : 0;
                    }
                    u32 coeff_abs_level_greaterB_flag = xevd_sbac_decode_bin(bs, sbac, cm_gtAB + ctx_gtB);
                    coef[pos[firstC2FlagIdx]] += coeff_abs_level_greaterB_flag;
                    abs_coef[firstC2FlagIdx] = coeff_abs_level_greaterB_flag + 2;
                    if (coeff_abs_level_greaterB_flag != 0)
                    {
                        escape_data_present_ingroup = 1;
                    }
                }
                escape_data_present_ingroup = escape_data_present_ingroup || (num_nz > CAFLAG_NUMBER);

                int iFirstCoeff2 = 1;
                if (escape_data_present_ingroup)
                {
                    for (idx = 0; idx < num_nz; idx++)
                    {
                        int base_level = (idx < CAFLAG_NUMBER) ? (2 + iFirstCoeff2) : 1;

                        if (abs_coef[idx] >= base_level)
                        {
                            int coeff_abs_level_remaining;

                            rice_param = xevdm_get_rice_para(coef, pos[idx], width, height, base_level);
                            coeff_abs_level_remaining = xevdm_parse_coef_remain_exgolomb(bs, sbac, rice_param);
                            coef[pos[idx]] = coeff_abs_level_remaining + base_level;
                            abs_coef[idx]  = coeff_abs_level_remaining + base_level;
                        }
                        if (abs_coef[idx] >= 2)
                        {
                            iFirstCoeff2 = 0;
                        }
                    }
                }
                coef_signs_group = sbac_decode_bins_ep(bs, sbac, num_nz);
                coef_signs_group <<= 32 - num_nz;

                for (idx = 0; idx < num_nz; idx++)
                {
                    blkpos = pos[idx];
                    coef[blkpos] = abs_coef[idx];

                    int sign = (int)((coef_signs_group) >> 31);
                    coef[blkpos] = sign > 0 ? -coef[blkpos] : coef[blkpos];
                    coef_signs_group <<= 1;
                } // for non-zero coefs within cg
            } // if nnz
        }
    } // for (cg)
    return XEVD_OK;
}


static int xevdm_eco_xcoef(XEVD_CTX *ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type, u8 ats_inter_info, int is_intra, int tool_adcc)
{
    if (is_intra)
    {
        assert(ats_inter_info == 0);
    }
    xevdm_get_tu_size(ats_inter_info, log2_w, log2_h, &log2_w, &log2_h);

    if (tool_adcc)
    {
        xevdm_eco_adcc(ctx, bs, sbac, coef, log2_w, log2_h, (ch_type == Y_C ? 0 : 1));
    }
    else
    {
        xevdm_eco_run_length_cc(ctx, bs, sbac, coef, log2_w, log2_h, (ch_type == Y_C ? 0 : 1));
    }
#if TRACE_COEFFS
    int cuw = 1 << log2_w;
    int cuh = 1 << log2_h;
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("Coeff for ");
    XEVD_TRACE_INT(ch_type);
    XEVD_TRACE_STR(": ");
    for (int i = 0; i < (cuw * cuh); ++i)
    {
        if (i != 0)
            XEVD_TRACE_STR(", ");
        XEVD_TRACE_INT(coef[i]);
    }
    XEVD_TRACE_STR("\n");
#endif
    return XEVD_OK;
}

static int xevdm_eco_merge_idx(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    int idx;
    idx = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.merge_idx, NUM_CTX_MERGE_IDX, MAXM_NUM_MVP);

#if ENC_DEC_TRACE
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("merge idx ");
    XEVD_TRACE_INT(idx);
    XEVD_TRACE_STR("\n");
#endif

    return idx;
}

static int xevdm_eco_affine_mvp_idx( XEVD_BSR * bs, XEVD_SBAC * sbac )
{
#if AFF_MAX_NUM_MVP == 1
    return 0;
#else
#if ENC_DEC_TRACE
    int idx;
    idx = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.affine_mvp_idx, NUM_CTX_AFFINE_MVP_IDX, AFF_MAX_NUM_MVP);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("affine mvp idx ");
    XEVD_TRACE_INT(idx);
    XEVD_TRACE_STR("\n");

    return idx;
#else
    return sbac_read_truncate_unary_sym( bs, sbac, sbac->ctx.affine_mvp_idx, NUM_CTX_AFFINE_MVP_IDX, AFF_MAX_NUM_MVP );
#endif
#endif
}

void xevdm_eco_mmvd_data(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    int        type = mctx->sh.mmvd_group_enable_flag && !(1 << (core->log2_cuw + core->log2_cuh) <= NUM_SAMPLES_BLOCK);
    int        parse_idx = 0;
    int        temp = 0;
    int        temp_t;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    if (type == 1)
    {
        /* mmvd_group_idx */
        temp_t = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.mmvd_group_idx + 0);
        if (temp_t == 1)
        {
            temp_t += xevd_sbac_decode_bin(bs, sbac, sbac->ctx.mmvd_group_idx + 1);
        }
    }
    else
    {
        temp_t = 0;
    }

    temp = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.mmvd_merge_idx, NUM_CTX_MMVD_MERGE_IDX, MMVD_BASE_MV_NUM); /* mmvd_merge_idx */
    parse_idx = temp * MMVD_MAX_REFINE_NUM + temp_t * (MMVD_MAX_REFINE_NUM * MMVD_BASE_MV_NUM);

    temp = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.mmvd_distance_idx, NUM_CTX_MMVD_DIST_IDX, MMVD_DIST_NUM); /* mmvd_distance_idx */
    parse_idx += (temp * 4);

    /* mmvd_direction_idx */
    temp = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.mmvd_direction_idx);
    parse_idx += (temp * 2);
    temp = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.mmvd_direction_idx + 1);
    parse_idx += (temp);

    mcore->mmvd_idx = parse_idx;

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("mmvd_idx ");
    XEVD_TRACE_INT(mcore->mmvd_idx);
    XEVD_TRACE_STR("\n");
}

static int xevdm_eco_mvr_idx(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    return sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.mvr_idx, MAX_NUM_MVR, MAX_NUM_MVR);
}

int xevdm_eco_coef(XEVD_CTX * ctx, XEVD_CORE * core)
{
    u8          cbf[N_C];
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;
    int         b_no_cbf = 0;
    u8          ats_inter_avail = xevdm_check_ats_inter_info_coded(1 << core->log2_cuw, 1 << core->log2_cuh, core->pred_mode, ctx->sps->tool_ats);
    int         log2_tuw = core->log2_cuw;
    int         log2_tuh = core->log2_cuh;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    b_no_cbf |= core->pred_mode == MODE_DIR && mcore->affine_flag;
    b_no_cbf |= core->pred_mode == MODE_DIR_MMVD;
    b_no_cbf |= core->pred_mode == MODE_DIR;

    if(ctx->sps->tool_admvp == 0)
    {
        b_no_cbf = 0;
    }
    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    s16 *coef_temp[N_C];
    s16 coef_temp_buf[N_C][MAX_TR_DIM];
    int i, j, c;
    int log2_w_sub = (core->log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuw;
    int log2_h_sub = (core->log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuh;
    int loop_w = (core->log2_cuw > MAX_TR_LOG2) ? (1 << (core->log2_cuw - MAX_TR_LOG2)) : 1;
    int loop_h = (core->log2_cuh > MAX_TR_LOG2) ? (1 << (core->log2_cuh - MAX_TR_LOG2)) : 1;
    int stride = (1 << core->log2_cuw);
    int sub_stride = (1 << log2_w_sub);
    int tmp_coef[N_C] = { 0 };
    int is_sub = loop_h + loop_w > 2 ? 1 : 0;
    int cbf_all = 1;

    u8 ats_intra_cu_on = 0;
    u8 ats_mode_idx = 0;
    u8 is_intra = (core->pred_mode == MODE_INTRA) ? 1 : 0;

    xevd_mset(core->is_coef, 0, sizeof(int) * N_C);
    xevd_mset(core->is_coef_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            if (cbf_all)
            {
                int is_coded_cbf_zero = xevdm_eco_cbf(bs, sbac, core->pred_mode, cbf, b_no_cbf, is_sub, j + i, &cbf_all, mcore->tree_cons, ctx->sps->chroma_format_idc);

                if (is_coded_cbf_zero)
                {
                    core->qp = GET_QP(ctx->tile[core->tile_num].qp_prev_eco, 0);
                    core->qp_y = GET_LUMA_QP(core->qp, ctx->sps->bit_depth_luma_minus8);
                    return XEVD_OK;
                }
            }
            else
            {
                cbf[Y_C] = cbf[U_C] = cbf[V_C] = 0;
            }

            int dqp;
            int qp_i_cb, qp_i_cr;
            if(ctx->pps.cu_qp_delta_enabled_flag && (((!(ctx->sps->dquant_flag) || (core->cu_qp_delta_code == 1 && !core->cu_qp_delta_is_coded))
                && (cbf[Y_C] || cbf[U_C] || cbf[V_C])) || (core->cu_qp_delta_code == 2 && !core->cu_qp_delta_is_coded)))
            {
                dqp = xevd_eco_dqp(bs);
                core->qp = GET_QP(ctx->tile[core->tile_num].qp_prev_eco, dqp);
                core->qp_y = GET_LUMA_QP(core->qp, ctx->sps->bit_depth_luma_minus8);
                core->cu_qp_delta_is_coded = 1;
                ctx->tile[core->tile_num].qp_prev_eco = core->qp;
            }
            else
            {
                dqp = 0;
                core->qp = GET_QP(ctx->tile[core->tile_num].qp_prev_eco, dqp);
                core->qp_y = GET_LUMA_QP(core->qp, ctx->sps->bit_depth_luma_minus8);
            }
            qp_i_cb = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = xevd_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps->bit_depth_chroma_minus8;
            core->qp_v = xevd_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps->bit_depth_chroma_minus8;

            if (ctx->sps->tool_ats && cbf[Y_C] && (core->log2_cuw <= 5 && core->log2_cuh <= 5) && is_intra)
            {
                xevd_assert(!xevd_check_only_inter(ctx, core));

                ats_intra_cu_on = xevdm_eco_ats_intra_cu(bs, sbac);

                ats_mode_idx = 0;
                if (ats_intra_cu_on)
                {
                    u8 ats_intra_mode_h = xevdm_eco_ats_mode_h(bs, sbac);
                    u8 ats_intra_mode_v = xevdm_eco_ats_mode_v(bs, sbac);

                    ats_mode_idx = ((ats_intra_mode_h << 1) | ats_intra_mode_v);
                }
            }
            else
            {
                ats_intra_cu_on = 0;
                ats_mode_idx = 0;
            }
            mcore->ats_intra_cu = ats_intra_cu_on;
            mcore->ats_intra_mode_h = (ats_mode_idx >> 1);
            mcore->ats_intra_mode_v = (ats_mode_idx & 1);

            if (ats_inter_avail && (cbf[Y_C] || cbf[U_C] || cbf[V_C]))
            {
                xevd_assert(!xevd_check_only_intra(ctx, core));
                xevdm_eco_ats_inter_info(bs, sbac, core->log2_cuw, core->log2_cuh, &mcore->ats_inter_info, ats_inter_avail);
            }
            else
            {
                assert(mcore->ats_inter_info == 0);
            }

            for (c = 0; c < N_C; c++)
            {
                if (cbf[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)))));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)))) * (stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)));

                    if (is_sub)
                    {
                        if(c == 0)
                            xevd_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        else
                            xevd_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), coef_temp_buf[c], sub_stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)));
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = core->coef[c];
                    }
                    if(c == 0)
                        xevdm_eco_xcoef(ctx, bs, sbac, coef_temp[c], log2_w_sub, log2_h_sub, c, mcore->ats_inter_info, core->pred_mode == MODE_INTRA, ctx->sps->tool_adcc);
                    else
                        xevdm_eco_xcoef(ctx, bs, sbac, coef_temp[c], log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)), c, mcore->ats_inter_info, core->pred_mode == MODE_INTRA, ctx->sps->tool_adcc);

                    core->is_coef_sub[c][(j << 1) | i] = 1;
                    tmp_coef[c] += 1;

                    if (is_sub)
                    {
                        if(c == 0)
                            xevd_block_copy(coef_temp_buf[c], sub_stride, core->coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
                        else
                            xevd_block_copy(coef_temp_buf[c], sub_stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), core->coef[c] + pos_sub_x + pos_sub_y, stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)));
                    }
                }
                else
                {
                    core->is_coef_sub[c][(j << 1) | i] = 0;
                    tmp_coef[c] += 0;
                }
            }
        }
    }
    for (c = 0; c < N_C; c++)
    {
        core->is_coef[c] = tmp_coef[c] ? 1 : 0;
    }
    return XEVD_OK;
}

void xevdm_eco_sbac_reset(XEVD_BSR * bs, u8 slice_type, u8 slice_qp, int sps_cm_init_flag)
{
    int i;
    u32 t0;
    XEVD_SBAC    * sbac;
    XEVD_SBAC_CTX * sbac_ctx;
    sbac = GET_SBAC_DEC(bs);
    sbac_ctx = &sbac->ctx;
    /* Initialization of the internal variables */
    sbac->range = 16384;
    sbac->value = 0;
    for(i = 0; i < 14; i++)
    {
#if TRACE_HLS
        xevd_bsr_read1_trace(bs, &t0, 0);
#else
        xevd_bsr_read1(bs, &t0);
#endif
        sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
    }

    xevd_mset(sbac_ctx, 0x00, sizeof(XEVD_SBAC_CTX));

    sbac_ctx->sps_cm_init_flag = sps_cm_init_flag;

    /* Initialization of the context models */
    if(sps_cm_init_flag == 1)
    {
        xevd_eco_sbac_ctx_initialize(sbac_ctx->cbf_luma, (s16*)init_cbf_luma, NUM_CTX_CBF_LUMA, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->cbf_cb, (s16*)init_cbf_cb, NUM_CTX_CBF_CB, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->cbf_cr, (s16*)init_cbf_cr, NUM_CTX_CBF_CR, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->cbf_all, (s16*)init_cbf_all, NUM_CTX_CBF_ALL, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->delta_qp, (s16*)init_dqp, NUM_CTX_DELTA_QP, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->sig_coeff_flag, (s16*)init_sig_coeff_flag, NUM_CTX_SIG_COEFF_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->coeff_abs_level_greaterAB_flag, (s16*)init_coeff_abs_level_greaterAB_flag, NUM_CTX_GTX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->last_sig_coeff_x_prefix, (s16*)init_last_sig_coeff_x_prefix, NUM_CTX_LAST_SIG_COEFF, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->last_sig_coeff_y_prefix, (s16*)init_last_sig_coeff_y_prefix, NUM_CTX_LAST_SIG_COEFF, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->pred_mode, (s16*)init_pred_mode, NUM_CTX_PRED_MODE, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mode_cons, (s16*)init_mode_cons, NUM_CTX_MODE_CONS, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->direct_mode_flag, (s16*)init_direct_mode_flag, NUM_CTX_DIRECT_MODE_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->merge_mode_flag, (s16*)init_merge_mode_flag, NUM_CTX_MERGE_MODE_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->inter_dir, (s16*)init_inter_dir, NUM_CTX_INTER_PRED_IDC, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->intra_dir, (s16*)init_intra_dir, NUM_CTX_INTRA_PRED_MODE, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->intra_luma_pred_mpm_flag, (s16*)init_intra_luma_pred_mpm_flag, NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->intra_luma_pred_mpm_idx, (s16*)init_intra_luma_pred_mpm_idx, NUM_CTX_INTRA_LUMA_PRED_MPM_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->intra_chroma_pred_mode, (s16*)init_intra_chroma_pred_mode, NUM_CTX_INTRA_CHROMA_PRED_MODE, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->run, (s16*)init_run, NUM_CTX_CC_RUN, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->last, (s16*)init_last, NUM_CTX_CC_LAST, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->level, (s16*)init_level, NUM_CTX_CC_LEVEL, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mmvd_flag, (s16*)init_mmvd_flag, NUM_CTX_MMVD_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mmvd_merge_idx, (s16*)init_mmvd_merge_idx, NUM_CTX_MMVD_MERGE_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mmvd_distance_idx, (s16*)init_mmvd_distance_idx, NUM_CTX_MMVD_DIST_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mmvd_direction_idx, (s16*)init_mmvd_direction_idx, NUM_CTX_MMVD_DIRECTION_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mmvd_group_idx, (s16*)init_mmvd_group_idx, NUM_CTX_MMVD_GROUP_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->merge_idx, (s16*)init_merge_idx, NUM_CTX_MERGE_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mvp_idx, (s16*)init_mvp_idx, NUM_CTX_MVP_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->affine_mvp_idx, (s16*)init_affine_mvp_idx, NUM_CTX_AFFINE_MVP_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mvr_idx, (s16*)init_mvr_idx, NUM_CTX_AMVR_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->bi_idx, (s16*)init_bi_idx, NUM_CTX_BI_PRED_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->mvd, (s16*)init_mvd, NUM_CTX_MVD, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->refi, (s16*)init_refi, NUM_CTX_REF_IDX, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->btt_split_flag, (s16*)init_btt_split_flag, NUM_CTX_BTT_SPLIT_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->btt_split_dir, (s16*)init_btt_split_dir, NUM_CTX_BTT_SPLIT_DIR, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->btt_split_type, (s16*)init_btt_split_type, NUM_CTX_BTT_SPLIT_TYPE, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->suco_flag, (s16*)init_suco_flag, NUM_CTX_SUCO_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->alf_ctb_flag, (s16*)init_alf_ctb_flag, NUM_CTX_ALF_CTB_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->split_cu_flag, (s16*)init_split_cu_flag, NUM_CTX_SPLIT_CU_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->affine_flag, (s16*)init_affine_flag, NUM_CTX_AFFINE_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->affine_mode, (s16*)init_affine_mode, NUM_CTX_AFFINE_MODE, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->affine_mrg, (s16*)init_affine_mrg, NUM_CTX_AFFINE_MRG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->affine_mvd_flag, (s16*)init_affine_mvd_flag, NUM_CTX_AFFINE_MVD_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->skip_flag, (s16*)init_skip_flag, NUM_CTX_SKIP_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ibc_flag, (s16*)init_ibc_flag, NUM_CTX_IBC_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ats_mode, (s16*)init_ats_mode, NUM_CTX_ATS_MODE_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_flag, (s16*)init_ats_cu_inter_flag, NUM_CTX_ATS_INTER_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_quad_flag, (s16*)init_ats_cu_inter_quad_flag, NUM_CTX_ATS_INTER_QUAD_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_hor_flag, (s16*)init_ats_cu_inter_hor_flag, NUM_CTX_ATS_INTER_HOR_FLAG, slice_type, slice_qp);
        xevd_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_pos_flag, (s16*)init_ats_cu_inter_pos_flag, NUM_CTX_ATS_INTER_POS_FLAG, slice_type, slice_qp);
    }
    else
    {
        for(i = 0; i < NUM_CTX_ALF_CTB_FLAG; i++) sbac_ctx->alf_ctb_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_SPLIT_CU_FLAG; i++) sbac_ctx->split_cu_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CC_RUN; i++) sbac_ctx->run[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CC_LAST; i++) sbac_ctx->last[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CC_LEVEL; i++) sbac_ctx->level[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CBF_LUMA; i++) sbac_ctx->cbf_luma[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CBF_CB; i++) sbac_ctx->cbf_cb[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CBF_CR; i++) sbac_ctx->cbf_cr[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_CBF_ALL; i++) sbac_ctx->cbf_all[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_SIG_COEFF_FLAG; i++) sbac_ctx->sig_coeff_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_GTX; i++) sbac_ctx->coeff_abs_level_greaterAB_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_LAST_SIG_COEFF; i++) sbac_ctx->last_sig_coeff_x_prefix[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_LAST_SIG_COEFF; i++) sbac_ctx->last_sig_coeff_y_prefix[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_PRED_MODE; i++) sbac_ctx->pred_mode[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MODE_CONS; i++) sbac_ctx->mode_cons[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_DIRECT_MODE_FLAG; i++) sbac_ctx->direct_mode_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MERGE_MODE_FLAG; i++) sbac_ctx->merge_mode_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_INTER_PRED_IDC; i++) sbac_ctx->inter_dir[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_INTRA_PRED_MODE; i++) sbac_ctx->intra_dir[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG; i++) sbac_ctx->intra_luma_pred_mpm_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_INTRA_LUMA_PRED_MPM_IDX; i++) sbac_ctx->intra_luma_pred_mpm_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_INTRA_CHROMA_PRED_MODE; i++) sbac_ctx->intra_chroma_pred_mode[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MMVD_FLAG; i++) sbac_ctx->mmvd_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MMVD_MERGE_IDX; i++) sbac_ctx->mmvd_merge_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MMVD_DIST_IDX; i++) sbac_ctx->mmvd_distance_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MMVD_DIRECTION_IDX; i++) sbac_ctx->mmvd_direction_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MMVD_GROUP_IDX; i++) sbac_ctx->mmvd_group_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MERGE_IDX; i++) sbac_ctx->merge_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MVP_IDX; i++) sbac_ctx->mvp_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AMVR_IDX; i++) sbac_ctx->mvr_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_BI_PRED_IDX; i++) sbac_ctx->bi_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_MVD; i++) sbac_ctx->mvd[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_REF_IDX; i++) sbac_ctx->refi[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_BTT_SPLIT_FLAG; i++) sbac_ctx->btt_split_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_BTT_SPLIT_DIR; i++) sbac_ctx->btt_split_dir[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_BTT_SPLIT_TYPE; i++) sbac_ctx->btt_split_type[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_SUCO_FLAG; i++) sbac_ctx->suco_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_DELTA_QP; i++) sbac_ctx->delta_qp[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AFFINE_FLAG; i++) sbac_ctx->affine_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AFFINE_MODE; i++) sbac_ctx->affine_mode[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AFFINE_MRG; i++) sbac_ctx->affine_mrg[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AFFINE_MVP_IDX; i++) sbac_ctx->affine_mvp_idx[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_AFFINE_MVD_FLAG; i++) sbac_ctx->affine_mvd_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_SKIP_FLAG; i++)  sbac_ctx->skip_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_IBC_FLAG; i++) sbac_ctx->ibc_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_ATS_MODE_FLAG; i++) sbac_ctx->ats_mode[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_ATS_INTER_FLAG; i++) sbac_ctx->ats_cu_inter_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_ATS_INTER_QUAD_FLAG; i++) sbac_ctx->ats_cu_inter_quad_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_ATS_INTER_HOR_FLAG; i++) sbac_ctx->ats_cu_inter_hor_flag[i] = PROB_INIT;
        for(i = 0; i < NUM_CTX_ATS_INTER_POS_FLAG; i++) sbac_ctx->ats_cu_inter_pos_flag[i] = PROB_INIT;
    }
}

void xevdm_eco_merge_mode_flag(XEVD_CTX * ctx, XEVD_CORE * core)
{

        XEVD_SBAC *sbac;
        XEVD_BSR   *bs;
        int        merge_mode_flag = 0;

        bs = core->bs;
        sbac = GET_SBAC_DEC(bs);

        merge_mode_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.merge_mode_flag);

        if(merge_mode_flag)
        {
            core->inter_dir = PRED_DIR;
        }
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("merge_mode_flag ");
        XEVD_TRACE_INT(core->inter_dir == PRED_DIR ? PRED_DIR : 0);
        XEVD_TRACE_STR("\n");
}


void xevdm_eco_inter_pred_idc(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int tmp = 1;
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    if (xevdm_check_bi_applicability(ctx->sh.slice_type, 1 << core->log2_cuw, 1 << core->log2_cuh, ctx->sps->tool_admvp))
    {
        tmp = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.inter_dir);
    }

    if (!tmp)
    {
        core->inter_dir = PRED_BI;
    }
    else
    {
        tmp = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.inter_dir + 1);
        core->inter_dir = tmp ? PRED_L1 : mcore->ibc_flag ? PRED_IBC : PRED_L0;
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("inter dir ");
    XEVD_TRACE_INT(core->inter_dir);
    XEVD_TRACE_STR("\n");
}

s8 xevdm_eco_split_mode(XEVD_CTX * c, XEVD_BSR *bs, XEVD_SBAC *sbac, int cuw, int cuh, const int parent_split, int* same_layer_split
                     , const int node_idx, const int* parent_split_allow, int* curr_split_allow, int qt_depth, int btt_depth, int x, int y
                     , MODE_CONS mode_cons, XEVD_CORE * core)
{
    int sps_cm_init_flag = sbac->ctx.sps_cm_init_flag;
    s8 split_mode = NO_SPLIT;
    int ctx = 0;
    int split_allow[SPLIT_CHECK_NUM];
    int i;

    if(cuw < 8 && cuh < 8)
    {
        split_mode = NO_SPLIT;
        return split_mode;
    }

    if (!c->sps->sps_btt_flag)
    {
        split_mode = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.split_cu_flag); /* split_cu_flag */
        split_mode = split_mode ? SPLIT_QUAD : NO_SPLIT;
        return split_mode;
    }

    xevdm_check_split_mode(split_allow, XEVD_CONV_LOG2(cuw), XEVD_CONV_LOG2(cuh), 0, 0, 0, c->log2_max_cuwh
                          , parent_split, same_layer_split, node_idx, parent_split_allow, qt_depth, btt_depth
                          , x, y, c->w, c->h
                          , NULL, c->sps->sps_btt_flag
                          , mode_cons);

    for(i = 1; i < SPLIT_CHECK_NUM; i++)
    {
        curr_split_allow[i] = split_allow[i];
    }

    if(split_allow[SPLIT_BI_VER] || split_allow[SPLIT_BI_HOR] || split_allow[SPLIT_TRI_VER] || split_allow[SPLIT_TRI_HOR])
    {
        int log2_cuw = XEVD_CONV_LOG2(cuw);
        int log2_cuh = XEVD_CONV_LOG2(cuh);
        if(sps_cm_init_flag == 1)
        {
            int i;
            u16 x_scu = x >> MIN_CU_LOG2;
            u16 y_scu = y >> MIN_CU_LOG2;
            u16 scuw = cuw >> MIN_CU_LOG2;
            u16 w_scu = c->w >> MIN_CU_LOG2;
            u8  smaller[3] = {0, 0, 0};
            u8  avail[3] = {0, 0, 0};
            int scun[3];
            int w[3], h[3];
            int scup = x_scu + y_scu * w_scu;

            avail[0] = y_scu > 0  && (c->map_tidx[scup] == c->map_tidx[scup - w_scu]);  //up
            if (x_scu > 0)
            {
                avail[1] = c->cod_eco[scup - 1] && (c->map_tidx[scup] == c->map_tidx[scup - 1]); //left
            }
            if (x_scu + scuw < w_scu)
            {
                avail[2] = c->cod_eco[scup + scuw] && (c->map_tidx[scup] == c->map_tidx[scup + scuw]); //right
            }
            scun[0] = scup - w_scu;
            scun[1] = scup - 1;
            scun[2] = scup + scuw;
            for(i = 0; i < 3; i++)
            {
                if(avail[i])
                {
                    w[i] = 1 << MCU_GET_LOGW(c->map_cu_mode[scun[i]]);
                    h[i] = 1 << MCU_GET_LOGH(c->map_cu_mode[scun[i]]);
                    if (i == 0)
                    {
                        smaller[i] = w[i] < cuw;
                    }
                    else
                    {
                        smaller[i] = h[i] < cuh;
                    }
                }
            }
            ctx = XEVD_MIN(smaller[0] + smaller[1] + smaller[2], 2);
            ctx = ctx + 3 * xevd_tbl_split_flag_ctx[log2_cuw - 2][log2_cuh - 2];
        }
        else
        {
            ctx = 0;
        }

        int btt_split_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.btt_split_flag + ctx); /* btt_split_flag */
        if(btt_split_flag)
        {
            u8 ctx_dir = sps_cm_init_flag == 1 ? (log2_cuw - log2_cuh + 2) : 0;
            u8 ctx_typ = 0;
            u8 btt_split_dir, btt_split_type;

            if((split_allow[SPLIT_BI_VER] || split_allow[SPLIT_TRI_VER]) &&
               (split_allow[SPLIT_BI_HOR] || split_allow[SPLIT_TRI_HOR]))
            {
                btt_split_dir = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.btt_split_dir + ctx_dir); /* btt_split_dir */
            }
            else
            {
                if(split_allow[SPLIT_BI_VER] || split_allow[SPLIT_TRI_VER])
                    btt_split_dir = 1;
                else
                    btt_split_dir = 0;
            }

            if((btt_split_dir && split_allow[SPLIT_BI_VER] && split_allow[SPLIT_TRI_VER]) ||
              (!btt_split_dir && split_allow[SPLIT_BI_HOR] && split_allow[SPLIT_TRI_HOR]))
            {
                btt_split_type = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.btt_split_type + ctx_typ); /* btt_split_type */
            }
            else
            {
                if((btt_split_dir && split_allow[SPLIT_TRI_VER]) ||
                  (!btt_split_dir && split_allow[SPLIT_TRI_HOR]))
                    btt_split_type = 1;
                else
                    btt_split_type = 0;
            }

            if(btt_split_type == 0) // BT
                split_mode = btt_split_dir ? SPLIT_BI_VER : SPLIT_BI_HOR;
            else
                split_mode = btt_split_dir ? SPLIT_TRI_VER : SPLIT_TRI_HOR;
        }
    }
    return split_mode;
}

s8 xevdm_eco_suco_flag(XEVD_BSR *bs, XEVD_SBAC *sbac, XEVD_CTX *c, XEVD_CORE *core, int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, int parent_suco)
{
    s8 suco_flag = parent_suco;
    int ctx = 0;
    u8 allow_suco = c->sps->sps_suco_flag ? xevdm_check_suco_cond(cuw, cuh, split_mode, boundary, log2_max_cuwh, c->sps->log2_diff_ctu_size_max_suco_cb_size, c->sps->log2_diff_max_suco_min_suco_cb_size, c->sps->log2_min_cb_size_minus2 + 2) : 0;

    if(!allow_suco)
    {
        return suco_flag;
    }

    if(sbac->ctx.sps_cm_init_flag == 1)
    {
        ctx = XEVD_CONV_LOG2(XEVD_MAX(cuw, cuh)) - 2;
        ctx = (cuw == cuh) ? ctx * 2 : ctx * 2 + 1;
    }
    else
    {
        ctx = 0;
    }

    suco_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.suco_flag + ctx);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("suco flag ");
    XEVD_TRACE_INT(suco_flag);
    XEVD_TRACE_STR("\n");

    return suco_flag;
}


void xevdm_eco_mmvd_flag(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC   *sbac;
    XEVD_BSR     *bs;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    mcore->mmvd_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.mmvd_flag); /* mmvd_flag */

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("mmvd_flag ");
    XEVD_TRACE_INT(mcore->mmvd_flag);
    XEVD_TRACE_STR("\n");
}

void xevdm_eco_affine_flag(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC   *sbac;
    XEVD_BSR     *bs;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    mcore->affine_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.affine_flag + core->ctx_flags[CNID_AFFN_FLAG]);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("affine flag ");
    XEVD_TRACE_INT(mcore->affine_flag);
    XEVD_TRACE_STR("\n");
}

int xevdm_eco_affine_mode(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC   *sbac;
    XEVD_BSR     *bs;
    u32          t0;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.affine_mode);

#if TRACE_ADDITIONAL_FLAGS
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("affine mode ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");
#endif
    return t0;
}


int xevdm_eco_affine_mvd_flag(XEVD_CTX * ctx, XEVD_CORE * core, int refi)
{
    XEVD_SBAC   *sbac;
    XEVD_BSR     *bs;
    u32          t0;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    t0 = xevd_sbac_decode_bin(bs, sbac, &sbac->ctx.affine_mvd_flag[refi]);
    return t0;
}

void xevdm_eco_pred_mode( XEVD_CTX * ctx, XEVD_CORE * core )
{
    XEVD_BSR    *bs = core->bs;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    XEVD_SBAC  *sbac = GET_SBAC_DEC( bs );
    BOOL        pred_mode_flag = FALSE;
    TREE_CONS   tree_cons = mcore->tree_cons;
    u8*         ctx_flags = core->ctx_flags;
    MODE_CONS   pred_mode_constraint = tree_cons.mode_cons; //TODO: Tim changed place

    if (pred_mode_constraint == eAll)
    {
        pred_mode_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.pred_mode + ctx_flags[CNID_PRED_MODE]);
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("pred mode ");
        XEVD_TRACE_INT(pred_mode_flag ? MODE_INTRA : MODE_INTER);
        XEVD_TRACE_STR("\n");
    }

    BOOL isIbcAllowed = ctx->sps->ibc_flag &&
        core->log2_cuw <= ctx->sps->ibc_log_max_size && core->log2_cuh <= ctx->sps->ibc_log_max_size &&
        tree_cons.tree_type != TREE_C &&
        pred_mode_constraint != eOnlyInter &&
        !( pred_mode_constraint == eAll && pred_mode_flag );

    mcore->ibc_flag = FALSE;

    if ( isIbcAllowed )
        mcore->ibc_flag = xevd_sbac_decode_bin( bs, sbac, sbac->ctx.ibc_flag + ctx_flags[CNID_IBC_FLAG] );

    if ( mcore->ibc_flag )
        core->pred_mode = MODE_IBC;
    else if ( pred_mode_constraint == eOnlyInter )
        core->pred_mode = MODE_INTER;
    else if( pred_mode_constraint == eOnlyIntra )
        core->pred_mode = MODE_INTRA;
    else
        core->pred_mode = pred_mode_flag ? MODE_INTRA : MODE_INTER;

#if TRACE_ADDITIONAL_FLAGS
    if ( isIbcAllowed )
    {
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR( "ibc pred mode " );
        XEVD_TRACE_INT( !!core->ibc_flag );
        XEVD_TRACE_STR( "ctx " );
        XEVD_TRACE_INT( ctx_flags[CNID_IBC_FLAG] );
        XEVD_TRACE_STR( "\n" );
    }
#endif
}

MODE_CONS xevdm_eco_mode_constr( XEVD_BSR *bs, u8 ctx_num )
{
    XEVD_SBAC   *sbac;
    u32          t0;

    sbac = GET_SBAC_DEC(bs);
    t0 = xevd_sbac_decode_bin( bs, sbac, sbac->ctx.mode_cons + ctx_num );
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("mode_constr ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");
    return t0 ? eOnlyIntra : eOnlyInter;
}

int xevdm_eco_cu(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC   *sbac;
    XEVD_BSR     *bs;
    int          ret, cuw, cuh, mvp_idx[REFP_NUM] = { 0, 0 };
    int          mmvd_flag = 0;
    int          direct_idx = -1;
    int          REF_SET[3][XEVD_MAX_NUM_ACTIVE_REF_FRAME] = { {0,0,}, };
    u8           bi_idx = BI_NON;
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    core->pred_mode = MODE_INTRA;
    mcore->affine_flag = 0;
    xevd_mset(mcore->affine_bzero, 0, sizeof(int) * REFP_NUM);
    xevd_mset(mcore->affine_mvd, 0, sizeof(s16) * REFP_NUM * 3 * MV_D);
    mcore->ibc_flag = 0;
    mcore->mmvd_idx = 0;
    core->mvr_idx = 0;
    mcore->mmvd_flag = 0;
    mcore->dmvr_flag = 0;
    mcore->ats_inter_info = 0;
    core->mvp_idx[REFP_0] = 0;
    core->mvp_idx[REFP_1] = 0;
    core->inter_dir = 0;
    core->bi_idx = 0;
    xevd_mset(core->mvd, 0, sizeof(s16) * REFP_NUM * MV_D);

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);
    core->avail_lr = xevd_check_eco_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->cod_eco, ctx->map_tidx);

    if (!xevd_check_all(ctx, core))
    {
        xevd_assert(xevd_check_only_intra(ctx, core));
    }


    xevdm_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->cod_eco, ctx->map_cu_mode, core->ctx_flags, ctx->sh.slice_type, ctx->sps->tool_cm_init
                         , ctx->sps->ibc_flag, ctx->sps->ibc_log_max_size, ctx->map_tidx, 1);

    if ( !xevd_check_only_intra(ctx, core) )
    {
        /* CU skip flag */
        xevd_eco_cu_skip_flag(ctx, core); /* cu_skip_flag */
    }

    /* parse prediction info */
    if (core->pred_mode == MODE_SKIP)
    {
        if (ctx->sps->tool_mmvd)
        {
            xevdm_eco_mmvd_flag(ctx, core); /* mmvd_flag */
        }

        if (mcore->mmvd_flag)
        {
            xevdm_eco_mmvd_data(ctx, core);
        }
        else
        {
            if (ctx->sps->tool_affine && cuw >= 8 && cuh >= 8)
            {
                xevdm_eco_affine_flag(ctx, core); /* affine_flag */
            }

            if (mcore->affine_flag)
            {
                core->mvp_idx[0] = xevdm_eco_affine_mrg_idx(bs, sbac);
            }
            else
            {
                if(!ctx->sps->tool_admvp)
                {
                    core->mvp_idx[REFP_0] = xevd_eco_mvp_idx(bs, sbac);
                    if(ctx->sh.slice_type == SLICE_B)
                    {
                        core->mvp_idx[REFP_1] = xevd_eco_mvp_idx(bs, sbac);
                    }
                }
                else
                {
                    core->mvp_idx[REFP_0] = xevdm_eco_merge_idx(bs, sbac);
                    core->mvp_idx[REFP_1] = core->mvp_idx[REFP_0];
                }
            }
        }

        core->is_coef[Y_C] = core->is_coef[U_C] = core->is_coef[V_C] = 0;   //TODO: Tim why we need to duplicate code here?
        xevd_mset(core->is_coef_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
        //need to check
        if(ctx->pps.cu_qp_delta_enabled_flag)
        {
            int qp_i_cb, qp_i_cr;
            core->qp = ctx->tile[core->tile_num].qp_prev_eco;
            core->qp_y = GET_LUMA_QP(core->qp, ctx->sps->bit_depth_luma_minus8);
            qp_i_cb = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = xevd_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps->bit_depth_chroma_minus8;
            core->qp_v = xevd_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps->bit_depth_chroma_minus8;
        }
        else
        {

            int qp_i_cb, qp_i_cr;
            core->qp = ctx->sh.qp;
            core->qp_y = GET_LUMA_QP(core->qp, ctx->sps->bit_depth_luma_minus8);
            qp_i_cb = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = XEVD_CLIP3(-6 * ctx->sps->bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = xevd_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps->bit_depth_chroma_minus8;
            core->qp_v = xevd_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps->bit_depth_chroma_minus8;
        }
    }
    else
    {
        xevdm_eco_pred_mode(ctx, core);

        if (core->pred_mode == MODE_INTER)
        {
            if (ctx->sps->tool_amvr)
            {
                core->mvr_idx = xevdm_eco_mvr_idx(bs, sbac);
#if TRACE_ADDITIONAL_FLAGS
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("mvr_idx ");
                XEVD_TRACE_INT(core->mvr_idx);
                XEVD_TRACE_STR("\n");
#endif
            }

            if (ctx->sh.slice_type == SLICE_B && ctx->sps->tool_admvp == 0)
            {
                xevd_eco_direct_mode_flag(ctx, core);
            }
            else if(ctx->sps->tool_admvp && core->mvr_idx == 0)
            {
                xevdm_eco_merge_mode_flag(ctx, core);
            }

            if (core->inter_dir == PRED_DIR)
            {
                if (ctx->sps->tool_admvp != 0)
                {
                    if (ctx->sps->tool_mmvd)
                    {
                        xevdm_eco_mmvd_flag(ctx, core); /* mmvd_flag */
                    }

                    if (mcore->mmvd_flag)
                    {
                        xevdm_eco_mmvd_data(ctx, core);
                        core->inter_dir = PRED_DIR_MMVD;
                    }
                    else
                    {
                        if (ctx->sps->tool_affine && cuw >= 8 && cuh >= 8)
                        {
                            xevdm_eco_affine_flag(ctx, core); /* affine_flag */
                        }

                        if (mcore->affine_flag)
                        {
                            core->inter_dir = AFF_DIR;
                            core->mvp_idx[0] = xevdm_eco_affine_mrg_idx(bs, sbac);
                        }
                        else
                        {
                            core->mvp_idx[REFP_0] = xevdm_eco_merge_idx(bs, sbac);
                            core->mvp_idx[REFP_1] = core->mvp_idx[REFP_0];
                        }
                    }
                    core->pred_mode = MODE_DIR;
                }
            }
            else
            {
                if (ctx->sh.slice_type == SLICE_B)
                {
                xevdm_eco_inter_pred_idc(ctx, core); /* inter_pred_idc */
                }

                if (cuw >= 16 && cuh >= 16 && ctx->sps->tool_affine && core->mvr_idx == 0)
                {
                    xevdm_eco_affine_flag(ctx, core);
                }

                if (mcore->affine_flag)
                {
                    mcore->affine_flag += xevdm_eco_affine_mode(ctx, core);

                    for (int inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
                    {
                        if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
                        {
                            XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
                            core->refi[inter_dir_idx] = xevd_eco_refi(bs, sbac, mctx->dpm.num_refp[inter_dir_idx]);
                            core->mvp_idx[inter_dir_idx] = xevdm_eco_affine_mvp_idx(bs, sbac);
                            mcore->affine_bzero[inter_dir_idx] = xevdm_eco_affine_mvd_flag(ctx, core, inter_dir_idx);

                            for (int vertex = 0; vertex < mcore->affine_flag + 1; vertex++)
                            {
                                if (mcore->affine_bzero[inter_dir_idx])
                                {
                                    mcore->affine_mvd[inter_dir_idx][vertex][MV_X] = 0;
                                    mcore->affine_mvd[inter_dir_idx][vertex][MV_Y] = 0;
                                }
                                else
                                {
                                    xevd_eco_get_mvd(bs, sbac, mcore->affine_mvd[inter_dir_idx][vertex]);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (ctx->sps->tool_admvp == 1 && core->inter_dir == PRED_BI)
                    {
                        core->bi_idx = xevd_eco_bi_idx(bs, sbac) + 1;
#if TRACE_ADDITIONAL_FLAGS
                        XEVD_TRACE_COUNTER;
                        XEVD_TRACE_STR("bi_idx ");
                        XEVD_TRACE_INT(core->bi_idx-1);
                        XEVD_TRACE_STR("\n");
#endif
                    }
                    for (int inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
                    {
                        /* 0: forward, 1: backward */
                        if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
                        {
                            XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
                            if (ctx->sps->tool_admvp == 0)
                            {

                                core->refi[inter_dir_idx] = xevd_eco_refi(bs, sbac, mctx->dpm.num_refp[inter_dir_idx]);
                                core->mvp_idx[inter_dir_idx] = xevd_eco_mvp_idx(bs, sbac);
                                xevd_eco_get_mvd(bs, sbac, core->mvd[inter_dir_idx]);
                            }
                            else
                            {
                                if (core->bi_idx != BI_FL0 && core->bi_idx != BI_FL1)
                                {
                                    core->refi[inter_dir_idx] = xevd_eco_refi(bs, sbac, mctx->dpm.num_refp[inter_dir_idx]);
                                }
                                if (core->bi_idx != BI_FL0 + inter_dir_idx)
                                {
                                    xevd_eco_get_mvd(bs, sbac, core->mvd[inter_dir_idx]);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (core->pred_mode == MODE_INTRA)
        {
            if (ctx->sps->tool_eipd)
            {
                xevdm_get_mpm(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->cod_eco, ctx->map_ipm, core->scup, ctx->w_scu,
                    core->mpm, core->avail_lr, core->mpm_ext, core->pims, ctx->map_tidx);
            }
            else
            {
                xevd_get_mpm_b(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->cod_eco, ctx->map_ipm, core->scup, ctx->w_scu,
                    &core->mpm_b_list, core->avail_lr, core->mpm_ext, core->pims, ctx->map_tidx);
            }

            if (ctx->sps->tool_eipd)
            {
                if (xevd_check_luma(ctx, core))
                {
                core->ipm[0] = xevd_eco_intra_dir(bs, sbac, core->mpm, core->mpm_ext, core->pims);
                }
                else
                {
                    int luma_cup = xevd_get_luma_cup(core->x_scu, core->y_scu, PEL2SCU(cuw), PEL2SCU(cuh), ctx->w_scu);
                    if (MCU_GET_IF(ctx->map_scu[luma_cup]))
                    {
                        core->ipm[0] = ctx->map_ipm[luma_cup];
                    }
                    else
                    {
                        core->ipm[0] = IPD_DC;
                    }
                }
                if (xevd_check_chroma(ctx, core) && (ctx->sps->chroma_format_idc != 0))
                {
                    core->ipm[1] = xevd_eco_intra_dir_c(bs, sbac, core->ipm[0]);
                }
            }
            else
            {

                int luma_ipm = IPD_DC_B;
                if (xevd_check_luma(ctx, core))
                {

                core->ipm[0] = xevd_eco_intra_dir_b(bs, sbac, core->mpm_b_list, core->mpm_ext, core->pims);

                luma_ipm = core->ipm[0];
                }
                else
                {
                    int luma_cup = xevd_get_luma_cup(core->x_scu, core->y_scu, PEL2SCU(cuw), PEL2SCU(cuh), ctx->w_scu);
                    assert( MCU_GET_IF( ctx->map_scu[luma_cup] ) && "IBC is forbidden for this case (EIPD off)");

                    luma_ipm = ctx->map_ipm[luma_cup];
                }
                if(xevd_check_chroma(ctx, core))
                {
                    core->ipm[1] = luma_ipm;
                }

            }

            SET_REFI(core->refi, REFI_INVALID, REFI_INVALID);

            core->mv[REFP_0][MV_X] = core->mv[REFP_0][MV_Y] = 0;
            core->mv[REFP_1][MV_X] = core->mv[REFP_1][MV_Y] = 0;
        }
        else if (core->pred_mode == MODE_IBC)
        {
            mcore->affine_flag = 0;
            SET_REFI(core->refi, REFI_INVALID, REFI_INVALID);

            mvp_idx[REFP_0] = 0;

            xevd_eco_get_mvd(bs, sbac, core->mv[REFP_0]);

            core->mv[REFP_1][MV_X] = 0;
            core->mv[REFP_1][MV_Y] = 0;
        }
        else
        {
            xevd_assert_rv(0, XEVD_ERR_MALFORMED_BITSTREAM);
        }

        /* clear coefficient buffer */
        xevd_mset(core->coef[Y_C], 0, cuw * cuh * sizeof(s16));
        xevd_mset(core->coef[U_C], 0, (cuw >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) * (cuh >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * sizeof(s16));
        xevd_mset(core->coef[V_C], 0, (cuw >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) * (cuh >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) * sizeof(s16));

        /* parse coefficients */
        ret = xevdm_eco_coef(ctx, core);
        xevd_assert_rv(ret == XEVD_OK, ret);
    }

    return XEVD_OK;
}


int xevdm_eco_rlp(XEVD_BSR * bs, XEVD_RPL * rpl)
{
    u32 delta_poc_st, strp_entry_sign_flag = 0;
    xevd_bsr_read_ue(bs, &rpl->ref_pic_num);
    if (rpl->ref_pic_num > 0)
    {
        xevd_bsr_read_ue(bs, &delta_poc_st);
        rpl->ref_pics[0] = delta_poc_st;
        if (rpl->ref_pics[0] != 0)
        {
            xevd_bsr_read1(bs, &strp_entry_sign_flag);
            rpl->ref_pics[0] *= 1 - (strp_entry_sign_flag << 1);
        }
    }
    for (int i = 1; i < rpl->ref_pic_num; ++i)
    {
        xevd_bsr_read_ue(bs, &delta_poc_st);
        if (delta_poc_st != 0) {
            xevd_bsr_read1(bs, &strp_entry_sign_flag);
        }
        rpl->ref_pics[i] = rpl->ref_pics[i - 1] + delta_poc_st * (1 - (strp_entry_sign_flag << 1));
    }

    return XEVD_OK;
}


int xevdm_eco_sps(XEVD_BSR * bs, XEVD_SPS * sps)
{
#if TRACE_HLS
    XEVD_TRACE_STR("***********************************\n");
    XEVD_TRACE_STR("************ SPS Start ************\n");
#endif
    xevd_bsr_read_ue(bs, &sps->sps_seq_parameter_set_id);
    xevd_bsr_read(bs, &sps->profile_idc, 8);
    xevd_bsr_read(bs, &sps->level_idc, 8);
    xevd_bsr_read(bs, &sps->toolset_idc_h, 32);
    xevd_bsr_read(bs, &sps->toolset_idc_l, 32);
    xevd_bsr_read_ue(bs, &sps->chroma_format_idc);
    xevd_bsr_read_ue(bs, &sps->pic_width_in_luma_samples);
    xevd_bsr_read_ue(bs, &sps->pic_height_in_luma_samples);
    xevd_bsr_read_ue(bs, &sps->bit_depth_luma_minus8);
    xevd_bsr_read_ue(bs, &sps->bit_depth_chroma_minus8);
    xevd_bsr_read1(bs, &sps->sps_btt_flag);
    if (sps->sps_btt_flag)
    {
        xevd_bsr_read_ue(bs, &sps->log2_ctu_size_minus5);
        xevd_bsr_read_ue(bs, &sps->log2_min_cb_size_minus2);
        xevd_bsr_read_ue(bs, &sps->log2_diff_ctu_max_14_cb_size);
        xevd_bsr_read_ue(bs, &sps->log2_diff_ctu_max_tt_cb_size);
        xevd_bsr_read_ue(bs, &sps->log2_diff_min_cb_min_tt_cb_size_minus2);
    }
    xevd_bsr_read1(bs, &sps->sps_suco_flag);
    if (sps->sps_suco_flag)
    {
        xevd_bsr_read_ue(bs, &sps->log2_diff_ctu_size_max_suco_cb_size);
        xevd_bsr_read_ue(bs, &sps->log2_diff_max_suco_min_suco_cb_size);
    }

    xevd_bsr_read1(bs, &sps->tool_admvp);
    if (sps->tool_admvp)
    {
        xevd_bsr_read1(bs, &sps->tool_affine);
        xevd_bsr_read1(bs, &sps->tool_amvr);
        xevd_bsr_read1(bs, &sps->tool_dmvr);
        xevd_bsr_read1(bs, &sps->tool_mmvd);
        xevd_bsr_read1(bs, &sps->tool_hmvp);
    }

    xevd_bsr_read1(bs, &sps->tool_eipd);
    if(sps->tool_eipd)
    {
        xevd_bsr_read1(bs, &sps->ibc_flag);
        if (sps->ibc_flag)
        {
            xevd_bsr_read_ue(bs, &sps->ibc_log_max_size);
            sps->ibc_log_max_size += 2;
        }
    }

    xevd_bsr_read1(bs, &sps->tool_cm_init);
    if(sps->tool_cm_init)
    {
        xevd_bsr_read1(bs, &sps->tool_adcc);
    }

    xevd_bsr_read1(bs, &sps->tool_iqt);
    if(sps->tool_iqt)
    {
        xevd_bsr_read1(bs, &sps->tool_ats);
    }

    xevd_bsr_read1(bs, &sps->tool_addb);

    xevd_bsr_read1(bs, &sps->tool_alf);
    xevd_bsr_read1(bs, &sps->tool_htdf);
    xevd_bsr_read1(bs, &sps->tool_rpl);
    xevd_bsr_read1(bs, &sps->tool_pocs);
    xevd_bsr_read1(bs, &sps->dquant_flag);
    xevd_bsr_read1(bs, &sps->tool_dra);
    if (sps->tool_pocs)
    {
        xevd_bsr_read_ue(bs, &sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    if (!sps->tool_rpl || !sps->tool_pocs)
    {
        xevd_bsr_read_ue(bs, &sps->log2_sub_gop_length);
        if (sps->log2_sub_gop_length == 0)
        {
            xevd_bsr_read_ue(bs, &sps->log2_ref_pic_gap_length);
        }
    }


    if (!sps->tool_rpl)
    {
         xevd_bsr_read_ue(bs, &sps->max_num_ref_pics);
    }
    else
    {

        xevd_bsr_read_ue(bs, &sps->sps_max_dec_pic_buffering_minus1);
        xevd_bsr_read1(bs, &sps->long_term_ref_pics_flag);
        xevd_bsr_read1(bs, &sps->rpl1_same_as_rpl0_flag);
        xevd_bsr_read_ue(bs, &sps->num_ref_pic_lists_in_sps0);

        for (int i = 0; i < sps->num_ref_pic_lists_in_sps0; ++i)
            xevdm_eco_rlp(bs, &sps->rpls_l0[i]);

        if (!sps->rpl1_same_as_rpl0_flag)
        {
            xevd_bsr_read_ue(bs, &sps->num_ref_pic_lists_in_sps1);
            for (int i = 0; i < sps->num_ref_pic_lists_in_sps1; ++i)
                xevdm_eco_rlp(bs, &sps->rpls_l1[i]);
        }
        else
        {
            assert(!"hasn't been implemented yet");
            //TBD: Basically copy everything from sps->rpls_l0 to sps->rpls_l1
        }
    }

    xevd_bsr_read1(bs, &sps->picture_cropping_flag);
    if (sps->picture_cropping_flag)
    {
        xevd_bsr_read_ue(bs, &sps->picture_crop_left_offset);
        xevd_bsr_read_ue(bs, &sps->picture_crop_right_offset);
        xevd_bsr_read_ue(bs, &sps->picture_crop_top_offset);
        xevd_bsr_read_ue(bs, &sps->picture_crop_bottom_offset);
    }


    if (sps->chroma_format_idc != 0)
    {
        xevd_bsr_read1(bs, &sps->chroma_qp_table_struct.chroma_qp_table_present_flag);
        if (sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
        {
            xevd_bsr_read1(bs, &sps->chroma_qp_table_struct.same_qp_table_for_chroma);
            xevd_bsr_read1(bs, &sps->chroma_qp_table_struct.global_offset_flag);
            for (int i = 0; i < (sps->chroma_qp_table_struct.same_qp_table_for_chroma ? 1 : 2); i++)
            {
                xevd_bsr_read_ue(bs, &sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]);
                for (int j = 0; j <= sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]; j++)
                {
                    xevd_bsr_read(bs, &sps->chroma_qp_table_struct.delta_qp_in_val_minus1[i][j], 6);
                    xevd_bsr_read_se(bs, &sps->chroma_qp_table_struct.delta_qp_out_val[i][j]);
                }
            }
        }
    }

    xevd_bsr_read1(bs, &sps->vui_parameters_present_flag);
    if (sps->vui_parameters_present_flag)
        xevd_eco_vui(bs, &(sps->vui_parameters));
    u32 t0;
    while (!XEVD_BSR_IS_BYTE_ALIGN(bs))
    {
        xevd_bsr_read1(bs, &t0);
    }
#if TRACE_HLS
    XEVD_TRACE_STR("************ SPS End   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    return XEVD_OK;
}

int xevdm_eco_pps(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps)
{
#if TRACE_HLS
    XEVD_TRACE_STR("***********************************\n");
    XEVD_TRACE_STR("************ PPS Start ************\n");
#endif
    xevd_bsr_read_ue(bs, &pps->pps_pic_parameter_set_id);
    assert(pps->pps_pic_parameter_set_id >= 0 && pps->pps_pic_parameter_set_id < MAX_NUM_PPS);
    xevd_bsr_read_ue(bs, &pps->pps_seq_parameter_set_id);
    xevd_bsr_read_ue(bs, &pps->num_ref_idx_default_active_minus1[0]);
    xevd_bsr_read_ue(bs, &pps->num_ref_idx_default_active_minus1[1]);
    xevd_bsr_read_ue(bs, &pps->additional_lt_poc_lsb_len);
    xevd_bsr_read1(bs, &pps->rpl1_idx_present_flag);
    xevd_bsr_read1(bs, &pps->single_tile_in_pic_flag);

    if(!pps->single_tile_in_pic_flag)
    {
        xevd_bsr_read_ue(bs, &pps->num_tile_columns_minus1);
        xevd_bsr_read_ue(bs, &pps->num_tile_rows_minus1);
        xevd_bsr_read1(bs, &pps->uniform_tile_spacing_flag);
        if(!pps->uniform_tile_spacing_flag)
        {
            for(int i = 0; i < pps->num_tile_columns_minus1; ++i)
            {
                xevd_bsr_read_ue(bs, &pps->tile_column_width_minus1[i]);
            }
            for(int i = 0; i < pps->num_tile_rows_minus1; ++i)
            {
                xevd_bsr_read_ue(bs, &pps->tile_row_height_minus1[i]);
            }
        }
        xevd_bsr_read1(bs, &pps->loop_filter_across_tiles_enabled_flag);
        xevd_bsr_read_ue(bs, &pps->tile_offset_lens_minus1);
    }

    xevd_bsr_read_ue(bs, &pps->tile_id_len_minus1);
    xevd_bsr_read1(bs, &pps->explicit_tile_id_flag);
    if(pps->explicit_tile_id_flag)
    {
        for(int i = 0; i <= pps->num_tile_rows_minus1; ++i)
        {
            for(int j = 0; j <= pps->num_tile_columns_minus1; ++j)
            {
                xevd_bsr_read(bs, &pps->tile_id_val[i][j], pps->tile_id_len_minus1 + 1);
            }
        }
    }

    pps->pic_dra_enabled_flag = 0;
    xevd_bsr_read1(bs, &pps->pic_dra_enabled_flag);
    if (pps->pic_dra_enabled_flag)
    {
        xevd_assert( sps->tool_dra == 1 );
        xevd_bsr_read(bs, &pps->pic_dra_aps_id, APS_MAX_NUM_IN_BITS);
    }

    xevd_bsr_read1(bs, &pps->arbitrary_slice_present_flag);
    xevd_bsr_read1(bs, &pps->constrained_intra_pred_flag);

    xevd_bsr_read1(bs, &pps->cu_qp_delta_enabled_flag);
    if(pps->cu_qp_delta_enabled_flag)
    {
        xevd_bsr_read_ue(bs, &pps->cu_qp_delta_area);
        pps->cu_qp_delta_area += 6;
    }
    u32 t0;
    while(!XEVD_BSR_IS_BYTE_ALIGN(bs))
    {
        xevd_bsr_read1(bs, &t0);
    }
#if TRACE_HLS
    XEVD_TRACE_STR("************ PPS End   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    return XEVD_OK;
}
int xevdm_eco_aps_gen(XEVD_BSR * bs, XEVD_APS_GEN * aps, int  bit_depth)
{
#if TRACE_HLS
    XEVD_TRACE_STR("***********************************\n");
    XEVD_TRACE_STR("************ APS Start ************\n");
#endif
    u32 aps_id, aps_type_id;

    xevd_bsr_read(bs, &aps_id, APS_MAX_NUM_IN_BITS); // parse APS ID
    xevd_bsr_read(bs, &aps_type_id, APS_TYPE_ID_BITS); // parse APS Type ID

    if (aps_type_id == 0)
    {
        XEVD_APS_GEN *aps_tmp = aps;
        aps_tmp->aps_id = aps_id;
        aps_tmp->aps_type_id = aps_type_id;

        XEVD_APS local_alf_aps;
        xevdm_eco_alf_aps_param(bs, &local_alf_aps); // parse ALF filter parameter (except ALF map)
        XEVD_ALF_SLICE_PARAM * alf_data = &(local_alf_aps.alf_aps_param);
        XEVD_ALF_SLICE_PARAM * aps_data = (XEVD_ALF_SLICE_PARAM *)(aps->aps_data);
        xevd_mcpy(aps_data, alf_data, sizeof(XEVD_ALF_SLICE_PARAM));
    }
    else if (aps_type_id == 1)
    {
        XEVD_APS_GEN *aps_tmp = aps + 1;
        aps_tmp->aps_id = aps_id;
        aps_tmp->aps_type_id = aps_type_id;

        xevdm_eco_dra_aps_param(bs, aps_tmp, bit_depth); // parse ALF filter parameter (except ALF map)
    }
    else
    {
        xevd_trace("This version of XEVD doesn't support APS Type %d\n", aps->aps_type_id);
    }

    u32 aps_extension_flag, aps_extension_data_flag, t0;
    xevd_bsr_read1(bs, &aps_extension_flag);
    assert(aps_extension_flag == 0);
    if (aps_extension_flag)
    {
        while (0/*more_rbsp_data()*/)
        {
            xevd_bsr_read1(bs, &aps_extension_data_flag);
        }
    }

    while (!XEVD_BSR_IS_BYTE_ALIGN(bs))
    {
        xevd_bsr_read1(bs, &t0);
    }
#if TRACE_HLS
    XEVD_TRACE_STR("************ APS End   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    return XEVD_OK;
}

int xevdm_truncatedUnaryEqProb(XEVD_BSR * bs, const int maxSymbol)
{
    for(int k = 0; k < maxSymbol; k++)
    {
        u32 symbol;
        xevd_bsr_read1(bs, &symbol);
        if(!symbol)
        {
            return k;
        }
    }
    return maxSymbol;
}

int xevdm_alfGolombDecode(XEVD_BSR * bs, const int k, const BOOL signed_val)
{
    int numLeadingBits = -1;
    uint32_t uiSymbol = 0;
    for (; !uiSymbol; numLeadingBits++)

    {
#if TRACE_HLS
        xevd_bsr_read1_trace(bs, &uiSymbol, 0);
#else
        xevd_bsr_read1(bs, &uiSymbol); //alf_coeff_abs_prefix
#endif
    }


    int symbol = ((1 << numLeadingBits) - 1) << k;
    if (numLeadingBits + k > 0)
    {
        uint32_t bins;
        xevd_bsr_read(bs, &bins, numLeadingBits + k);
        symbol += bins;
    }

    if (signed_val && symbol != 0)
    {
#if TRACE_HLS
        xevd_bsr_read1_trace(bs, &uiSymbol, 0);
#else
        xevd_bsr_read1(bs, &uiSymbol);
#endif
        symbol = (uiSymbol) ? symbol : -symbol;
    }
    return symbol;
}

u32 xevdm_xReadTruncBinCode(XEVD_BSR * bs, const int uiMaxSymbol)
{
    u32 ruiSymbol;
    int uiThresh;
    if(uiMaxSymbol > 256)
    {
        int uiThreshVal = 1 << 8;
        uiThresh = 8;
        while(uiThreshVal <= uiMaxSymbol)
        {
            uiThresh++;
            uiThreshVal <<= 1;
        }
        uiThresh--;
    }
    else
    {
        uiThresh = tb_max[uiMaxSymbol];
    }

    int uiVal = 1 << uiThresh;
    int b = uiMaxSymbol - uiVal;
    xevd_bsr_read(bs, &ruiSymbol, uiThresh); //xReadCode( uiThresh, ruiSymbol );
    if(ruiSymbol >= uiVal - b)
    {
        u32 uiSymbol;
        xevd_bsr_read1(bs, &uiSymbol); //xReadFlag( uiSymbol );
        ruiSymbol <<= 1;
        ruiSymbol += uiSymbol;
        ruiSymbol -= (uiVal - b);
    }

    return ruiSymbol;
}

int xevdm_eco_alf_filter(XEVD_BSR * bs, XEVD_ALF_SLICE_PARAM* alf_slice_param, const BOOL isChroma)
{
    if (!isChroma)
    {
        xevd_bsr_read1(bs, &alf_slice_param->coeff_delta_flag); // "alf_coefficients_delta_flag"
        if (!alf_slice_param->coeff_delta_flag && alf_slice_param->num_luma_filters > 1)
        {
            xevd_bsr_read1(bs, &alf_slice_param->coeff_delta_pred_mode_flag); // "coeff_delta_pred_mode_flag"
        }
        else
        {
            alf_slice_param->coeff_delta_pred_mode_flag = 0;
        }
        XEVD_ALF_FILTER_SHAPE alfShape;
        xevd_alf_init_filter_shape(&alfShape, isChroma ? 5 : (alf_slice_param->luma_filter_type == ALF_FILTER_5 ? 5 : 7));

        const int maxGolombIdx = alfShape.filterType == 0 ? 2 : 3;

        int alf_luma_min_eg_order_minus1;
        int kMin;
        int kMinTab[MAX_NUM_ALF_COEFF];

        xevd_bsr_read_ue(bs, &alf_luma_min_eg_order_minus1);
        xevd_assert(alf_luma_min_eg_order_minus1 >= 0 && alf_luma_min_eg_order_minus1 <= 6);
        kMin = alf_luma_min_eg_order_minus1 + 1;

        const int numFilters = isChroma ? 1 : alf_slice_param->num_luma_filters;
        short* coeff = isChroma ? alf_slice_param->chroma_coeff : alf_slice_param->luma_coeff;

        for (int idx = 0; idx < maxGolombIdx; idx++)
        {
            u32 alf_eg_order_increase_flag;
            xevd_bsr_read1(bs, &alf_eg_order_increase_flag);
            kMinTab[idx] = kMin + alf_eg_order_increase_flag;
            kMin = kMinTab[idx];
        }
        if (alf_slice_param->coeff_delta_flag)
        {
            for (int ind = 0; ind < alf_slice_param->num_luma_filters; ++ind)
            {
                xevd_bsr_read1(bs, &alf_slice_param->filter_coeff_flag[ind]); // "filter_coefficient_flag[i]"
            }
        }
        for (int ind = 0; ind < numFilters; ++ind)
        {
            if (alf_slice_param->filter_coeff_flag[ind])
            {
                for (int i = 0; i < alfShape.numCoeff - 1; i++)
                {
                    coeff[ind * MAX_NUM_ALF_LUMA_COEFF + i] = xevdm_alfGolombDecode(bs, kMinTab[alfShape.golombIdx[i]], TRUE);
                }
            }
            else
            {
                xevd_mset(coeff + ind * MAX_NUM_ALF_LUMA_COEFF, 0, sizeof(*coeff) * alfShape.numCoeff);
                continue;
            }
        }
    }
    else
    {
        XEVD_ALF_FILTER_SHAPE alfShape;
        xevd_alf_init_filter_shape(&alfShape, isChroma ? 5 : (alf_slice_param->luma_filter_type == ALF_FILTER_5 ? 5 : 7));

        const int maxGolombIdx = alfShape.filterType == 0 ? 2 : 3;

        int alf_luma_min_eg_order_minus1;
        int kMin;
        int kMinTab[MAX_NUM_ALF_COEFF];

        xevd_bsr_read_ue(bs, &alf_luma_min_eg_order_minus1);
        xevd_assert(alf_luma_min_eg_order_minus1 >= 0 && alf_luma_min_eg_order_minus1 <= 6);
        kMin = alf_luma_min_eg_order_minus1 + 1;

        const int numFilters = isChroma ? 1 : alf_slice_param->num_luma_filters;
        short* coeff = isChroma ? alf_slice_param->chroma_coeff : alf_slice_param->luma_coeff;

        for (int idx = 0; idx < maxGolombIdx; idx++)
        {
            u32 alf_eg_order_increase_flag;
            xevd_bsr_read1(bs, &alf_eg_order_increase_flag);
            kMinTab[idx] = kMin + alf_eg_order_increase_flag;
            kMin = kMinTab[idx];
        }
        for (int ind = 0; ind < numFilters; ++ind)
        {
            for (int i = 0; i < alfShape.numCoeff - 1; i++)
            {
                coeff[ind * MAX_NUM_ALF_LUMA_COEFF + i] = xevdm_alfGolombDecode(bs, kMinTab[alfShape.golombIdx[i]], TRUE);
            }
        }
    }

    return XEVD_OK;
}
int xevdm_eco_dra_aps_param(XEVD_BSR * bs, XEVD_APS_GEN * aps, int bit_depth)
{
    int DRA_RANGE_10 = 10;
    int dra_number_ranges_minus1;
    int dra_equal_ranges_flag;
    int dra_global_offset;
    int dra_delta_range[32];
    SIG_PARAM_DRA *p_dra_param = (SIG_PARAM_DRA *)aps->aps_data;
    p_dra_param->signal_dra_flag = 1;
    xevd_bsr_read(bs, &p_dra_param->dra_descriptor1, 4);
    xevd_bsr_read(bs, &p_dra_param->dra_descriptor2, 4);
    xevd_assert(p_dra_param->dra_descriptor1 == 4);
    xevd_assert(p_dra_param->dra_descriptor2 == 9);
    int numBits = p_dra_param->dra_descriptor1 + p_dra_param->dra_descriptor2;
    xevd_assert(numBits > 0);
    xevd_bsr_read_ue(bs, &dra_number_ranges_minus1);
    xevd_assert(dra_number_ranges_minus1 >= 0 && dra_number_ranges_minus1 <= 31);
    xevd_bsr_read1(bs, &dra_equal_ranges_flag);
    xevd_bsr_read(bs, &dra_global_offset, DRA_RANGE_10);
    xevd_assert(dra_global_offset >= 1 && dra_global_offset <= XEVD_MIN(1023, (1 << bit_depth) - 1));
    if (dra_equal_ranges_flag)
    {
        xevd_bsr_read(bs, &dra_delta_range[0], DRA_RANGE_10);
    }
    else
    {
        for (int i = 0; i <= dra_number_ranges_minus1; i++)
        {
            xevd_bsr_read(bs, &dra_delta_range[i], DRA_RANGE_10);
            xevd_assert(dra_delta_range[i] >= 1 && dra_delta_range[i] <= XEVD_MIN(1023, (1 << bit_depth) - 1));
        }
    }

    for (int i = 0; i <= dra_number_ranges_minus1; i++)
    {
        xevd_bsr_read(bs, &p_dra_param->dra_scale_value[i], numBits);
        xevd_assert(p_dra_param->dra_scale_value[i] < (4 << p_dra_param->dra_descriptor2) );
    }

    xevd_bsr_read(bs, &p_dra_param->dra_cb_scale_value, numBits);
    xevd_bsr_read(bs, &p_dra_param->dra_cr_scale_value, numBits);
    xevd_assert(p_dra_param->dra_cb_scale_value < (4 << p_dra_param->dra_descriptor2));
    xevd_assert(p_dra_param->dra_cr_scale_value < (4 << p_dra_param->dra_descriptor2));
    xevd_bsr_read_ue(bs, &p_dra_param->dra_table_idx);
    xevd_assert(p_dra_param->dra_table_idx >= 0 && p_dra_param->dra_table_idx <= 58);
    p_dra_param->num_ranges = dra_number_ranges_minus1 + 1;
    p_dra_param->equal_ranges_flag = dra_equal_ranges_flag;
    p_dra_param->in_ranges[0] = dra_global_offset << XEVD_MAX(0, bit_depth - DRA_RANGE_10);

    for (int i = 1; i <= p_dra_param->num_ranges; i++)
    {
        int deltaRange = ((p_dra_param->equal_ranges_flag == 1) ? dra_delta_range[0] : dra_delta_range[i - 1]);
        p_dra_param->in_ranges[i] = p_dra_param->in_ranges[i - 1] + (deltaRange << XEVD_MAX(0, bit_depth - DRA_RANGE_10));
    }

    return XEVD_OK;
}
int xevdm_eco_alf_aps_param(XEVD_BSR * bs, XEVD_APS * aps)
{
    XEVD_ALF_SLICE_PARAM* alf_slice_param = &(aps->alf_aps_param);
    //AlfSliceParam reset
    alf_slice_param->reset_alf_buffer_flag = 0;
    alf_slice_param->store2_alf_buffer_flag = 0;
    alf_slice_param->temporal_alf_flag = 0;
    alf_slice_param->prev_idx = 0;
    alf_slice_param->prev_idx_comp[0] = 0;
    alf_slice_param->prev_idx_comp[1] = 0;
    alf_slice_param->t_layer = 0;
    alf_slice_param->is_ctb_alf_on = 0;
    xevd_mset(alf_slice_param->enabled_flag, 0, 3 * sizeof(BOOL));
    alf_slice_param->luma_filter_type = ALF_FILTER_5;
    xevd_mset(alf_slice_param->luma_coeff, 0, sizeof(short) * 325);
    xevd_mset(alf_slice_param->chroma_coeff, 0, sizeof(short) * 7);
    xevd_mset(alf_slice_param->filter_coeff_delta_idx, 0, sizeof(short)*MAX_NUM_ALF_CLASSES);
    xevd_mset(alf_slice_param->filter_coeff_flag, 1, sizeof(BOOL) * 25);
    alf_slice_param->num_luma_filters = 1;
    alf_slice_param->coeff_delta_flag = 0;
    alf_slice_param->coeff_delta_pred_mode_flag = 0;
    alf_slice_param->chroma_ctb_present_flag = 0;
    alf_slice_param->fixed_filter_pattern = 0;
    xevd_mset(alf_slice_param->fixed_filter_idx, 0, sizeof(int) * 25);
    xevd_mset(alf_slice_param->fixed_filter_usage_flag, 0, sizeof(u8) * 25);

    const int iNumFixedFilterPerClass = 16;
    int alf_luma_filter_signal_flag;
    int alf_chroma_filter_signal_flag;
    int alf_luma_num_filters_signalled_minus1;
    int alf_luma_type_flag;
    u32 alf_luma_coeff_delta_idx[MAX_NUM_ALF_CLASSES];
    int alf_luma_fixed_filter_usage_pattern;
    u32 alf_luma_fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int alf_luma_fixed_filter_set_idx[MAX_NUM_ALF_CLASSES];

    xevd_mset(alf_luma_fixed_filter_set_idx, 0, sizeof(alf_luma_fixed_filter_set_idx));
    xevd_mset(alf_luma_fixed_filter_usage_flag, 0, sizeof(alf_luma_fixed_filter_usage_flag));

    xevd_bsr_read1(bs, &alf_luma_filter_signal_flag);
    alf_slice_param->enabled_flag[0] = alf_luma_filter_signal_flag;
    xevd_bsr_read1(bs, &alf_chroma_filter_signal_flag);
    alf_slice_param->chroma_filter_present = alf_chroma_filter_signal_flag;

    if (alf_luma_filter_signal_flag)
    {
        xevd_bsr_read_ue(bs, &alf_luma_num_filters_signalled_minus1);
        xevd_assert( (alf_luma_num_filters_signalled_minus1 >= 0 ) && ( alf_luma_num_filters_signalled_minus1 <= MAX_NUM_ALF_CLASSES - 1) );
        xevd_bsr_read1(bs, &alf_luma_type_flag);
        alf_slice_param->num_luma_filters = alf_luma_num_filters_signalled_minus1 + 1;
        alf_slice_param->luma_filter_type = alf_luma_type_flag;

        if (alf_luma_num_filters_signalled_minus1 > 0)
        {
            for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
            {
                xevd_bsr_read(bs, &alf_luma_coeff_delta_idx[i], xevd_tbl_log2[alf_luma_num_filters_signalled_minus1] + 1);
                alf_slice_param->filter_coeff_delta_idx[i] = alf_luma_coeff_delta_idx[i];
            }
        }
        alf_luma_fixed_filter_usage_pattern = xevdm_alfGolombDecode(bs, 0, FALSE);
        alf_slice_param->fixed_filter_pattern = alf_luma_fixed_filter_usage_pattern;
        if (alf_luma_fixed_filter_usage_pattern == 2)
        {
            for (int classIdx = 0; classIdx < MAX_NUM_ALF_CLASSES; classIdx++)
            {
                xevd_bsr_read1(bs, &alf_luma_fixed_filter_usage_flag[classIdx]); // "fixed_filter_flag"
                alf_slice_param->fixed_filter_usage_flag[classIdx] = alf_luma_fixed_filter_usage_flag[classIdx];
            }
        }
        else if (alf_luma_fixed_filter_usage_pattern == 1)
        {
            for (int classIdx = 0; classIdx < MAX_NUM_ALF_CLASSES; classIdx++)
            {
                //on
                alf_luma_fixed_filter_usage_flag[classIdx] = 1;
                alf_slice_param->fixed_filter_usage_flag[classIdx] = 1;
            }
        }
        if (alf_luma_fixed_filter_usage_pattern > 0)
        {
            for (int classIdx = 0; classIdx < MAX_NUM_ALF_CLASSES; classIdx++)
            {
                if (alf_luma_fixed_filter_usage_flag[classIdx] > 0)
                {
                    xevd_bsr_read(bs, &alf_luma_fixed_filter_set_idx[classIdx], 4 );
                    xevd_assert(alf_luma_fixed_filter_set_idx[classIdx] >= 0 && alf_luma_fixed_filter_set_idx[classIdx] <= 15);
                    alf_slice_param->fixed_filter_idx[classIdx] = alf_luma_fixed_filter_set_idx[classIdx];
                }
            }
        }

        xevdm_eco_alf_filter(bs, &(aps->alf_aps_param), FALSE);
    }

    if (alf_chroma_filter_signal_flag)
    {
        xevdm_eco_alf_filter(bs, &(aps->alf_aps_param), TRUE);
    }

    return XEVD_OK;
}

int xevdm_eco_alf_sh_param(XEVD_BSR * bs, XEVDM_SH * sh)
{
    XEVD_ALF_SLICE_PARAM* alf_slice_param = &(sh->alf_sh_param);

    //AlfSliceParam reset
    alf_slice_param->prev_idx_comp[0] = 0;
    alf_slice_param->prev_idx_comp[1] = 0;
    alf_slice_param->reset_alf_buffer_flag = 0;
    alf_slice_param->store2_alf_buffer_flag = 0;
    alf_slice_param->temporal_alf_flag = 0;
    alf_slice_param->prev_idx = 0;
    alf_slice_param->t_layer = 0;
    alf_slice_param->is_ctb_alf_on = 0;
    xevd_mset(alf_slice_param->enabled_flag, 0, 3 * sizeof(BOOL));
    alf_slice_param->luma_filter_type = ALF_FILTER_5;
    xevd_mset(alf_slice_param->luma_coeff, 0, sizeof(short) * 325);
    xevd_mset(alf_slice_param->chroma_coeff, 0, sizeof(short) * 7);
    xevd_mset(alf_slice_param->filter_coeff_delta_idx, 0, sizeof(short)*MAX_NUM_ALF_CLASSES);
    xevd_mset(alf_slice_param->filter_coeff_flag, 1, sizeof(BOOL) * 25);
    alf_slice_param->num_luma_filters = 1;
    alf_slice_param->coeff_delta_flag = 0;
    alf_slice_param->coeff_delta_pred_mode_flag = 0;
    alf_slice_param->chroma_ctb_present_flag = 0;
    alf_slice_param->fixed_filter_pattern = 0;
    xevd_mset(alf_slice_param->fixed_filter_idx, 0, sizeof(int) * 25);
    xevd_mset(alf_slice_param->fixed_filter_usage_flag, 0, sizeof(u8) * 25);
    //decode map
    xevd_bsr_read1(bs, &alf_slice_param->is_ctb_alf_on);
    return XEVD_OK;
}

int xevdm_eco_sh(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps, XEVD_SH * sh, XEVDM_SH * msh, int nut)
{
#if TRACE_HLS
    XEVD_TRACE_STR("***********************************\n");
    XEVD_TRACE_STR("************ SH  Start ************\n");
#endif
    int num_tiles_in_slice = 0;

    xevd_bsr_read_ue(bs, &sh->slice_pic_parameter_set_id);
    assert(sh->slice_pic_parameter_set_id >= 0 && sh->slice_pic_parameter_set_id < MAX_NUM_PPS);
    if (!pps->single_tile_in_pic_flag)
    {
        xevd_bsr_read1(bs, &sh->single_tile_in_slice_flag);
        xevd_bsr_read(bs, &sh->first_tile_id, pps->tile_id_len_minus1 + 1);
    }
    else
    {
        sh->single_tile_in_slice_flag = 1;
    }

    if (!sh->single_tile_in_slice_flag)
    {
        if (pps->arbitrary_slice_present_flag)
        {
            xevd_bsr_read1(bs, &sh->arbitrary_slice_flag);
        }
        if (!sh->arbitrary_slice_flag)
        {
            xevd_bsr_read(bs, &sh->last_tile_id, pps->tile_id_len_minus1 + 1);
        }
        else
        {
            xevd_bsr_read_ue(bs, &sh->num_remaining_tiles_in_slice_minus1);
            num_tiles_in_slice = sh->num_remaining_tiles_in_slice_minus1 + 2;
            for (int i = 0; i < num_tiles_in_slice - 1; ++i)
            {
                xevd_bsr_read_ue(bs, &sh->delta_tile_id_minus1[i]);
            }
        }
    }

    xevd_bsr_read_ue(bs, &sh->slice_type);

    if (!sh->arbitrary_slice_flag)
    {
        int first_tile_in_slice, last_tile_in_slice, first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
        int w_tile, w_tile_slice, h_tile_slice, tile_cnt;

        w_tile = (pps->num_tile_columns_minus1 + 1);
        tile_cnt = (pps->num_tile_rows_minus1 + 1) * (pps->num_tile_columns_minus1 + 1);

        first_tile_in_slice = sh->first_tile_id;
        last_tile_in_slice = sh->last_tile_id;


        first_tile_col_idx = first_tile_in_slice % w_tile;
        last_tile_col_idx = last_tile_in_slice % w_tile;
        delta_tile_idx = last_tile_in_slice - first_tile_in_slice;

        if (last_tile_in_slice < first_tile_in_slice)
        {
            if (first_tile_col_idx > last_tile_col_idx)
            {
                delta_tile_idx += tile_cnt + w_tile;
            }
            else
            {
                delta_tile_idx += tile_cnt;
            }
        }
        else if (first_tile_col_idx > last_tile_col_idx)
        {
            delta_tile_idx += w_tile;
        }

        w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
        h_tile_slice = (delta_tile_idx / w_tile) + 1;
        num_tiles_in_slice = w_tile_slice * h_tile_slice;
    }
    else
    {
        num_tiles_in_slice = sh->num_remaining_tiles_in_slice_minus1 + 2;
    }

    if (nut == XEVD_NUT_IDR)
    {
        xevd_bsr_read1(bs, &sh->no_output_of_prior_pics_flag);
    }

    if (sps->tool_mmvd && ((sh->slice_type == SLICE_B)||(sh->slice_type == SLICE_P)) )
    {
        xevd_bsr_read1(bs, &msh->mmvd_group_enable_flag);
    }
    else
    {
        msh->mmvd_group_enable_flag = 0;
    }

    if (sps->tool_alf)
    {
        msh->alf_chroma_idc = 0;
        msh->alf_sh_param.enabled_flag[0] = msh->alf_sh_param.enabled_flag[1] = msh->alf_sh_param.enabled_flag[2] = 0;
        xevd_bsr_read1(bs, &msh->alf_on);
        msh->alf_sh_param.enabled_flag[0] = msh->alf_on;
        if (msh->alf_on)
        {
            xevd_bsr_read(bs, &msh->aps_id_y, 5);
            xevdm_eco_alf_sh_param(bs, msh); // parse ALF map
            xevd_bsr_read(bs, &msh->alf_chroma_idc, 2);
            msh->alf_sh_param.enabled_flag[1] = msh->alf_chroma_idc & 1;
            msh->alf_sh_param.enabled_flag[2] = (msh->alf_chroma_idc >> 1) & 1;
            if (msh->alf_chroma_idc == 1)
            {
                msh->chroma_alf_enabled_flag = 1;
                msh->chroma_alf_enabled2_flag = 0;
            }
            else if (msh->alf_chroma_idc == 2)
            {
                msh->chroma_alf_enabled_flag = 0;
                msh->chroma_alf_enabled2_flag = 1;
            }
            else if (msh->alf_chroma_idc == 3)
            {
                msh->chroma_alf_enabled_flag = 1;
                msh->chroma_alf_enabled2_flag = 1;
            }
            else
            {
                msh->chroma_alf_enabled_flag = 0;
                msh->chroma_alf_enabled2_flag = 0;
            }
            if (msh->alf_chroma_idc && (sps->chroma_format_idc == 1 || sps->chroma_format_idc == 2))
            {
                xevd_bsr_read(bs, &msh->aps_id_ch, 5);
            }
        }
        if (sps->chroma_format_idc == 3 && msh->chroma_alf_enabled_flag)
        {
            xevd_bsr_read(bs, &msh->aps_id_ch, 5);
            xevd_bsr_read1(bs, &msh->alf_chroma_map_signalled);
        }
        if (sps->chroma_format_idc == 3 && msh->chroma_alf_enabled2_flag)
        {
            xevd_bsr_read(bs, &msh->aps_id_ch2, 5);
            xevd_bsr_read1(bs, &msh->alf_chroma2_map_signalled);
        }
    }

    if (nut != XEVD_NUT_IDR)
    {
        if (sps->tool_pocs)
        {
            xevd_bsr_read(bs, &sh->poc_lsb, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        }
        if (sps->tool_rpl)
        {
            //L0 candidates signaling
            if (sps->num_ref_pic_lists_in_sps0 > 0)
            {
                xevd_bsr_read1(bs, &sh->ref_pic_list_sps_flag[0]);
            }
            else
            {
                sh->ref_pic_list_sps_flag[0] = 0;
            }

            if (sh->ref_pic_list_sps_flag[0])
            {
                if (sps->num_ref_pic_lists_in_sps0 > 1)
                {
                    xevd_bsr_read_ue(bs, &sh->rpl_l0_idx);
                    xevd_mcpy(&sh->rpl_l0, &sps->rpls_l0[sh->rpl_l0_idx], sizeof(sh->rpl_l0)); //TBD: temporal workaround, consider refactoring
                    sh->rpl_l0.poc = sh->poc_lsb;
                }
            }
            else
            {
                xevdm_eco_rlp(bs, &sh->rpl_l0);
                sh->rpl_l0.poc = sh->poc_lsb;
            }

            //L1 candidates signaling
            if (pps->rpl1_idx_present_flag)
            {
                if (sps->num_ref_pic_lists_in_sps1 > 0)
                {
                    xevd_bsr_read1(bs, &sh->ref_pic_list_sps_flag[1]);
                }
                else
                {
                    sh->ref_pic_list_sps_flag[1] = 0;
                }
            }
            else
            {
                sh->ref_pic_list_sps_flag[1] = sh->ref_pic_list_sps_flag[0];
            }

            if (sh->ref_pic_list_sps_flag[1])
            {
                if (pps->rpl1_idx_present_flag)
                {
                    if (sps->num_ref_pic_lists_in_sps1 > 1)
                    {
                        xevd_bsr_read_ue(bs, &sh->rpl_l1_idx);
                    }
                }
                else
                {
                    sh->rpl_l1_idx = sh->rpl_l0_idx;
                }

                xevd_mcpy(&sh->rpl_l1, &sps->rpls_l1[sh->rpl_l1_idx], sizeof(sh->rpl_l1)); //TBD: temporal workaround, consider refactoring
                sh->rpl_l1.poc = sh->poc_lsb;
            }
            else
            {
                xevdm_eco_rlp(bs, &sh->rpl_l1);
                sh->rpl_l1.poc = sh->poc_lsb;
            }
        }
    }

    if (sh->slice_type != SLICE_I)
    {
        xevd_bsr_read1(bs, &sh->num_ref_idx_active_override_flag);
        if (sh->num_ref_idx_active_override_flag)
        {
            u32 num_ref_idx_active_minus1;
            xevd_bsr_read_ue(bs, &num_ref_idx_active_minus1);
            sh->rpl_l0.ref_pic_active_num = num_ref_idx_active_minus1 + 1;
            if (sh->slice_type == SLICE_B)
            {
                xevd_bsr_read_ue(bs, &num_ref_idx_active_minus1);
                sh->rpl_l1.ref_pic_active_num = num_ref_idx_active_minus1 + 1;
            }
        }
        else
        {
            sh->rpl_l0.ref_pic_active_num = pps->num_ref_idx_default_active_minus1[REFP_0] + 1;
            sh->rpl_l1.ref_pic_active_num = pps->num_ref_idx_default_active_minus1[REFP_1] + 1;
        }

        if (sps->tool_admvp)
        {
            xevd_bsr_read1(bs, &sh->temporal_mvp_asigned_flag);
            if (sh->temporal_mvp_asigned_flag)
            {
                if (sh->slice_type == SLICE_B)
                {
                    xevd_bsr_read1(bs, &sh->collocated_from_list_idx);
                    xevd_bsr_read1(bs, &sh->collocated_mvp_source_list_idx);
                }
                xevd_bsr_read1(bs, &sh->collocated_from_ref_idx);
            }
        }
    }
    xevd_bsr_read1(bs, &sh->deblocking_filter_on);

    if(sh->deblocking_filter_on && sps->tool_addb)
    {

        xevd_bsr_read_se(bs, &sh->sh_deblock_alpha_offset);
        xevd_bsr_read_se(bs, &sh->sh_deblock_beta_offset);

    }

    xevd_bsr_read(bs, &sh->qp, 6);
    if (sh->qp < 0 || sh->qp > 51)
    {
        xevd_trace("malformed bitstream: slice_qp should be in the range of 0 to 51\n");
        return XEVD_ERR_MALFORMED_BITSTREAM;
    }

    xevd_bsr_read_se(bs, &sh->qp_u_offset);
    xevd_bsr_read_se(bs, &sh->qp_v_offset);
    sh->qp_u = (s8)XEVD_CLIP3(-6 * sps->bit_depth_luma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)XEVD_CLIP3(-6 * sps->bit_depth_luma_minus8, 57, sh->qp + sh->qp_v_offset);

    if (!sh->single_tile_in_slice_flag)
    {
        for (int i = 0; i < num_tiles_in_slice - 1; ++i)
        {
            xevd_bsr_read(bs, &sh->entry_point_offset_minus1[i], pps->tile_offset_lens_minus1 + 1);
        }
    }

    /* byte align */
    u32 t0;
    while(!XEVD_BSR_IS_BYTE_ALIGN(bs))
    {
        xevd_bsr_read1(bs, &t0);
        xevd_assert_rv(0 == t0, XEVD_ERR_MALFORMED_BITSTREAM);
    }
#if TRACE_HLS
    XEVD_TRACE_STR("************ SH  End   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    return XEVD_OK;
}

int xevdm_eco_affine_mrg_idx(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    int t0 = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.affine_mrg, AFF_MAX_CAND, AFF_MAX_CAND);

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("merge affine idx ");
    XEVD_TRACE_INT(t0);
    XEVD_TRACE_STR("\n");

    return  t0;
}


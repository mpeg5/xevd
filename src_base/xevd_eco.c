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
#include "xevd_tbl.h"
#include <math.h>

u32 xevd_sbac_decode_bin(XEVD_BSR * bs, XEVD_SBAC * sbac, SBAC_CTX_MODEL * model)
{
    u32 bin, lps, t0;
    u16 mps, state;

    state = (*model) >> 1;
    mps = (*model) & 1;

    lps = (state * (sbac->range)) >> 9;
    lps = lps < 437 ? 437 : lps;

    bin = mps;

    sbac->range -= lps;

#if TRACE_BIN
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("model ");
    XEVD_TRACE_INT(*model);
    XEVD_TRACE_STR("range ");
    XEVD_TRACE_INT(sbac->range);
    XEVD_TRACE_STR("lps ");
    XEVD_TRACE_INT(lps);
    XEVD_TRACE_STR("\n");
#endif

    if(sbac->value >= sbac->range)
    {
        bin = 1 - mps;
        sbac->value -= sbac->range;
        sbac->range = lps;

        state = state + ((512 - state + 16) >> 5);
        if(state > 256)
        {
            mps = 1 - mps;
            state = 512 - state;
        }
        *model = (state << 1) + mps;
    }
    else
    {
        bin = mps;
        state = state - ((state + 16) >> 5);
        *model = (state << 1) + mps;
    }

    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
#if TRACE_HLS
        xevd_bsr_read1_trace(bs, &t0, 0);
#else
        xevd_bsr_read1(bs, &t0);
#endif
        sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
    }

    return bin;
}

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

u32 xevd_sbac_decode_bin_trm(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    u32 bin, t0;

    sbac->range--;

    if(sbac->value >= sbac->range)
    {
        bin = 1;

        /*
        sbac->value -= sbac->range;
        sbac->range = 1;
        */

        while(!XEVD_BSR_IS_BYTE_ALIGN(bs))
        {
#if TRACE_HLS
            xevd_bsr_read1_trace(bs, &t0, 0);
#else
            xevd_bsr_read1(bs, &t0);
#endif
            xevd_assert_rv(t0 == 0, XEVD_ERR_MALFORMED_BITSTREAM);
        }
    }
    else
    {
        bin = 0;
        while(sbac->range < 8192)
        {
            sbac->range <<= 1;
#if TRACE_HLS
            xevd_bsr_read1_trace(bs, &t0, 0);
#else
            xevd_bsr_read1(bs, &t0);
#endif
            sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
        }
    }

    return bin;
}

static u32 sbac_read_unary_sym_ep(XEVD_BSR * bs, XEVD_SBAC * sbac, u32 max_val)
{
    u32 t32u;
    u32 symbol;
    int counter = 0;

    symbol = sbac_decode_bin_ep(bs, sbac); counter++;

    if(symbol == 0)
    {
        return symbol;
    }

    symbol = 0;
    do
    {
        if(counter == max_val) t32u = 0;
        else t32u = sbac_decode_bin_ep(bs, sbac);
        counter++;
        symbol++;
    } while(t32u);

    return symbol;
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

static int eco_cbf(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 pred_mode, u8 cbf[N_C], int b_no_cbf, int is_sub, int sub_pos
                 , int * cbf_all, int chroma_format_idc)
{
    XEVD_SBAC_CTX * sbac_ctx;
    sbac_ctx = &sbac->ctx;

    /* decode allcbf */
    if (pred_mode != MODE_INTRA)
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
            cbf[U_C] = cbf[V_C] = 0;
        }

        cbf[Y_C] = xevd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_luma);
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("cbf Y ");
        XEVD_TRACE_INT(cbf[Y_C]);
        XEVD_TRACE_STR("\n");

    }

    return 0;
}

static int xevd_eco_run_length_cc(XEVD_CTX * ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type)
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
        t0 = (ch_type == Y_C ? 0 : 2);

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

#if ENC_DEC_TRACE
    XEVD_TRACE_STR("coef luma ");
    for(scan_pos_offset = 0; scan_pos_offset < num_coeff; scan_pos_offset++)
    {
        XEVD_TRACE_INT(coef[scan_pos_offset]);
    }
    XEVD_TRACE_STR("\n");
#endif
    return XEVD_OK;
}

static int xevd_eco_xcoef(XEVD_CTX *ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type, int is_intra)
{
    xevd_eco_run_length_cc(ctx, bs, sbac, coef, log2_w, log2_h, (ch_type == Y_C ? 0 : 1));

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

int xevd_eco_refi(XEVD_BSR * bs, XEVD_SBAC * sbac, int num_refp)
{
    XEVD_SBAC_CTX * c = &sbac->ctx;
    int ref_num = 0;

    if(num_refp > 1)
    {
        if(xevd_sbac_decode_bin(bs, sbac, c->refi))
        {
            ref_num++;
            if(num_refp > 2 && xevd_sbac_decode_bin(bs, sbac, c->refi + 1))
            {
                ref_num++;
                for(; ref_num < num_refp - 1; ref_num++)
                {
                    if(!sbac_decode_bin_ep(bs, sbac))
                    {
                        break;
                    }
                }
                return ref_num;
            }
        }
    }
    return ref_num;
}

int xevd_eco_mvp_idx(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    int idx;
    idx = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.mvp_idx, 3, 4);
#if ENC_DEC_TRACE
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("mvp idx ");
    XEVD_TRACE_INT(idx);
    XEVD_TRACE_STR("\n");
#endif
    return idx;
}

int xevd_eco_bi_idx(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    u32 t0;

    t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.bi_idx);
    if(t0)
    {
        return 0;
    }
    else
    {
        t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.bi_idx + 1);
        if(t0)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }
}

int xevd_eco_dqp(XEVD_BSR * bs)
{
    XEVD_SBAC    *sbac;
    XEVD_SBAC_CTX *sbac_ctx;
    int             dqp, sign;
    sbac = GET_SBAC_DEC(bs);
    sbac_ctx = &sbac->ctx;

    dqp = sbac_read_unary_sym(bs, sbac, sbac_ctx->delta_qp, NUM_CTX_DELTA_QP);

    if (dqp > 0)
    {
        sign = sbac_decode_bin_ep(bs, sbac);
        dqp = XEVD_SIGN_SET(dqp, sign);
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("dqp ");
    XEVD_TRACE_INT(dqp);
    XEVD_TRACE_STR("\n");

    return dqp;
}

static u32 xevd_eco_abs_mvd(XEVD_BSR *bs, XEVD_SBAC *sbac, SBAC_CTX_MODEL *model)
{
    u32 val = 0;
    u32 code = 0;
    u32 len;

    code = xevd_sbac_decode_bin(bs, sbac, model); /* use one model */

    if(code == 0)
    {
        len = 0;

        while(!(code & 1))
        {
            if(len == 0)
                code = xevd_sbac_decode_bin(bs, sbac, model);
            else
                code = sbac_decode_bin_ep(bs, sbac);
            len++;
        }
        val = (1 << len) - 1;

        while(len != 0)
        {
            if(len == 0)
                code = xevd_sbac_decode_bin(bs, sbac, model);
            else
                code = sbac_decode_bin_ep(bs, sbac);
            val += (code << (--len));
        }
    }

    return val;
}

int xevd_eco_get_mvd(XEVD_BSR * bs, XEVD_SBAC * sbac, s16 mvd[MV_D])
{
    u32 sign;
    s16 t16;

    /* MV_X */
    t16 = (s16)xevd_eco_abs_mvd(bs, sbac, sbac->ctx.mvd);

    if(t16 == 0) mvd[MV_X] = 0;
    else
    {
        /* sign */
        sign = sbac_decode_bin_ep(bs, sbac);

        if(sign) mvd[MV_X] = -t16;
        else mvd[MV_X] = t16;
    }

    /* MV_Y */
    t16 = (s16)xevd_eco_abs_mvd(bs, sbac, sbac->ctx.mvd);

    if(t16 == 0)
    {
        mvd[MV_Y] = 0;
    }
    else
    {
        /* sign */
        sign = sbac_decode_bin_ep(bs, sbac);

        if(sign) mvd[MV_Y] = -t16;
        else mvd[MV_Y] = t16;
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("mvd x ");
    XEVD_TRACE_INT(mvd[MV_X]);
    XEVD_TRACE_STR("mvd y ");
    XEVD_TRACE_INT(mvd[MV_Y]);
    XEVD_TRACE_STR("\n");

    return XEVD_OK;
}

int xevd_eco_coef(XEVD_CTX * ctx, XEVD_CORE * core)
{
    u8          cbf[N_C];
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;
    int         b_no_cbf = 0;
    int         log2_tuw = core->log2_cuw;
    int         log2_tuh = core->log2_cuh;


    b_no_cbf |= core->pred_mode == MODE_DIR;
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

    u8 is_intra = (core->pred_mode == MODE_INTRA) ? 1 : 0;

    xevd_mset(core->is_coef, 0, sizeof(int) * N_C);
    xevd_mset(core->is_coef_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            if (cbf_all)
            {
                int is_coded_cbf_zero = eco_cbf(bs, sbac, core->pred_mode, cbf, b_no_cbf, is_sub, j + i, &cbf_all
                                              , ctx->sps->chroma_format_idc);
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

            if (ctx->pps.cu_qp_delta_enabled_flag && (cbf[Y_C] || cbf[U_C] || cbf[V_C]))
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

            for (c = 0; c < N_C; c++)
            {
                if (cbf[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)))));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)))) * (stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)));

                    if (is_sub)
                    {
                        if (c == 0)
                        {
                            xevd_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            xevd_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), coef_temp_buf[c], sub_stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))
                                          , log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)));
                        }

                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = core->coef[c];
                    }

                    if (c == 0)
                    {
                        xevd_eco_xcoef(ctx, bs, sbac, coef_temp[c], log2_w_sub, log2_h_sub, c, core->pred_mode == MODE_INTRA);
                    }
                    else
                    {
                        xevd_eco_xcoef(ctx, bs, sbac, coef_temp[c], log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))
                                     , c, core->pred_mode == MODE_INTRA);
                    }

                    core->is_coef_sub[c][(j << 1) | i] = 1;
                    tmp_coef[c] += 1;

                    if (is_sub)
                    {
                        if (c == 0)
                        {
                            xevd_block_copy(coef_temp_buf[c], sub_stride, core->coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            xevd_block_copy(coef_temp_buf[c], sub_stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), core->coef[c] + pos_sub_x + pos_sub_y, stride >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))
                                          , log2_w_sub - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)), log2_h_sub - (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc)));
                        }
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

void xevd_eco_sbac_reset(XEVD_BSR * bs, u8 slice_type, u8 slice_qp)
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

    /* Initialization of the context models */
    for(i = 0; i < NUM_CTX_SPLIT_CU_FLAG; i++) sbac_ctx->split_cu_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_RUN; i++) sbac_ctx->run[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_LAST; i++) sbac_ctx->last[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_LEVEL; i++) sbac_ctx->level[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_LUMA; i++) sbac_ctx->cbf_luma[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_CB; i++) sbac_ctx->cbf_cb[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_CR; i++) sbac_ctx->cbf_cr[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_ALL; i++) sbac_ctx->cbf_all[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_PRED_MODE; i++) sbac_ctx->pred_mode[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_DIRECT_MODE_FLAG; i++) sbac_ctx->direct_mode_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MERGE_MODE_FLAG; i++) sbac_ctx->merge_mode_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTER_PRED_IDC; i++) sbac_ctx->inter_dir[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTRA_PRED_MODE; i++) sbac_ctx->intra_dir[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG; i++) sbac_ctx->intra_luma_pred_mpm_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTRA_LUMA_PRED_MPM_IDX; i++) sbac_ctx->intra_luma_pred_mpm_idx[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTRA_CHROMA_PRED_MODE; i++) sbac_ctx->intra_chroma_pred_mode[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MERGE_IDX; i++) sbac_ctx->merge_idx[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MVP_IDX; i++) sbac_ctx->mvp_idx[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_BI_PRED_IDX; i++) sbac_ctx->bi_idx[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MVD; i++) sbac_ctx->mvd[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_REF_IDX; i++) sbac_ctx->refi[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_DELTA_QP; i++) sbac_ctx->delta_qp[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_SKIP_FLAG; i++)  sbac_ctx->skip_flag[i] = PROB_INIT;

}

static int intra_mode_read_trunc_binary(int max_symbol, XEVD_SBAC * sbac, XEVD_BSR *bs)
{
    int threshold = 4; /* we use 6 bits to signal the default mode */
    int val = 1 << threshold;
    int b;
    int ipm = 0;
    int t0 = 0;

    b = max_symbol - val;
    assert(b < val);
    ipm = sbac_decode_bins_ep(bs, sbac, threshold);
    if(ipm >= val - b)
    {
        t0 = sbac_decode_bins_ep(bs, sbac, 1);
        ipm <<= 1;
        ipm += t0;
        ipm -= (val - b);
    }
    return ipm;
}

int xevd_eco_intra_dir_b(XEVD_BSR * bs, XEVD_SBAC * sbac,   u8  * mpm, u8 mpm_ext[8], u8 pims[IPD_CNT])
{
    u32 t0;
    int ipm = 0;
    int i;
    t0 = sbac_read_unary_sym(bs, sbac, sbac->ctx.intra_dir, 2);
    XEVD_TRACE_COUNTER;
#if TRACE_ADDITIONAL_FLAGS
    XEVD_TRACE_STR("mpm list: ");
#endif
    for (i = 0; i< IPD_CNT_B; i++)
    {
        if (t0== mpm[i])
        {
            ipm = i;
        }
#if TRACE_ADDITIONAL_FLAGS
        XEVD_TRACE_INT(mpm[i]);
#endif
    }
    XEVD_TRACE_STR("ipm Y ");
    XEVD_TRACE_INT(ipm);
    XEVD_TRACE_STR("\n");
    return ipm;
}

int xevd_eco_intra_dir(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 mpm[2], u8 mpm_ext[8], u8 pims[IPD_CNT])
{
    int ipm = 0;
    int mpm_flag;

    mpm_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.intra_luma_pred_mpm_flag); /* intra_luma_pred_mpm_flag */

    if(mpm_flag)
    {
        int mpm_idx;
        mpm_idx = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.intra_luma_pred_mpm_idx); /* intra_luma_pred_mpm_idx */
        ipm = mpm[mpm_idx];
    }
    else
    {
        int pims_flag;
        pims_flag = sbac_decode_bin_ep(bs, sbac); /* intra_luma_pred_pims_flag */
        if(pims_flag)
        {
            int pims_idx;
            pims_idx = sbac_decode_bins_ep(bs, sbac, 3); /* intra_luma_pred_pims_idx */
            ipm = mpm_ext[pims_idx];
        }
        else
        {
            int rem_mode;
            rem_mode = intra_mode_read_trunc_binary(IPD_CNT - (INTRA_MPM_NUM + INTRA_PIMS_NUM), sbac, bs); /* intra_luma_pred_rem_mode */
            ipm = pims[(INTRA_MPM_NUM + INTRA_PIMS_NUM) + rem_mode];
        }
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("ipm Y ");
    XEVD_TRACE_INT(ipm);
    XEVD_TRACE_STR("\n");

    return ipm;
}

int xevd_eco_intra_dir_c(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 ipm_l)
{
    u32 t0;
    int ipm = 0;
    u8 chk_bypass;
#if TRACE_ADDITIONAL_FLAGS
    u8 ipm_l_saved = ipm_l;
#endif

    XEVD_IPRED_CONV_L2C_CHK(ipm_l, chk_bypass);

    t0 = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.intra_chroma_pred_mode);
    if(t0 == 0)
    {
        ipm = sbac_read_unary_sym_ep(bs, sbac, IPD_CHROMA_CNT - 1);
        ipm++;
        if(chk_bypass &&  ipm >= ipm_l) ipm++;
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("ipm UV ");
    XEVD_TRACE_INT(ipm);
#if TRACE_ADDITIONAL_FLAGS
    XEVD_TRACE_STR("ipm L ");
    XEVD_TRACE_INT(ipm_l_saved);
#endif
    XEVD_TRACE_STR("\n");

    return ipm;
}

void xevd_eco_direct_mode_flag(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;
    int        direct_mode_flag = 0;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    direct_mode_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.direct_mode_flag);

    if(direct_mode_flag)
    {
        core->inter_dir = PRED_DIR;
    }
    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("direct_mode_flag ");
    XEVD_TRACE_INT(core->inter_dir);
    XEVD_TRACE_STR("\n");
}

void xevd_eco_merge_mode_flag(XEVD_CTX * ctx, XEVD_CORE * core)
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


void xevd_eco_inter_pred_idc(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int tmp = 1;
    XEVD_SBAC *sbac;
    XEVD_BSR   *bs;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    if (xevd_check_bi_applicability(ctx->sh.slice_type, 1 << core->log2_cuw, 1 << core->log2_cuh))
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
        core->inter_dir = tmp ? PRED_L1 : PRED_L0;
    }

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("inter dir ");
    XEVD_TRACE_INT(core->inter_dir);
    XEVD_TRACE_STR("\n");
}

s8 xevd_eco_split_mode(XEVD_BSR *bs, XEVD_SBAC *sbac, int cuw, int cuh)
{
    s8 split_mode = NO_SPLIT;

    if(cuw < 8 && cuh < 8)
    {
        split_mode = NO_SPLIT;
        return split_mode;
    }

    split_mode = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.split_cu_flag); /* split_cu_flag */
    split_mode = split_mode ? SPLIT_QUAD : NO_SPLIT;
    return split_mode;
}


void xevd_eco_pred_mode( XEVD_CTX * ctx, XEVD_CORE * core )
{
    XEVD_BSR  * bs = core->bs;
    XEVD_SBAC * sbac = GET_SBAC_DEC( bs );
    BOOL        pred_mode_flag = FALSE;
    u8        * ctx_flags = core->ctx_flags;

    if (ctx->sh.slice_type != SLICE_I)
    {
        pred_mode_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.pred_mode + ctx_flags[CNID_PRED_MODE]);
        XEVD_TRACE_COUNTER;
        XEVD_TRACE_STR("pred mode ");
        XEVD_TRACE_INT(pred_mode_flag ? MODE_INTRA : MODE_INTER);
        XEVD_TRACE_STR("\n");
    }
    else
    {
        pred_mode_flag = 1;
    }

    core->pred_mode = pred_mode_flag ? MODE_INTRA : MODE_INTER;
}

void xevd_eco_cu_skip_flag(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC * sbac;
    XEVD_BSR  * bs;
    int         cu_skip_flag = 0;

    bs = core->bs;
    sbac = GET_SBAC_DEC(bs);

    cu_skip_flag = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG]); /* cu_skip_flag */

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("skip flag ");
    XEVD_TRACE_INT(cu_skip_flag);
    XEVD_TRACE_STR("ctx ");
    XEVD_TRACE_INT(core->ctx_flags[CNID_SKIP_FLAG]);
    XEVD_TRACE_STR("\n");

    if (cu_skip_flag)
    {
        core->pred_mode = MODE_SKIP;
    }
}

int xevd_eco_cu(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_SBAC * sbac;
    XEVD_BSR  * bs;
    int         ret, cuw, cuh, mvp_idx[REFP_NUM] = { 0, 0 };
    int         direct_idx = -1;
    int         REF_SET[3][XEVD_MAX_NUM_ACTIVE_REF_FRAME] = { {0,0,}, };
    u8          bi_idx = BI_NON;

    core->pred_mode = MODE_INTRA;
    core->mvr_idx = 0;
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

    if (ctx->sh.slice_type != SLICE_I)
    {
        /* CU skip flag */
        xevd_eco_cu_skip_flag(ctx, core); /* cu_skip_flag */
    }

    /* parse prediction info */
    if (core->pred_mode == MODE_SKIP)
    {
        core->mvp_idx[REFP_0] = xevd_eco_mvp_idx(bs, sbac);
        if(ctx->sh.slice_type == SLICE_B)
        {
            core->mvp_idx[REFP_1] = xevd_eco_mvp_idx(bs, sbac);
        }

        core->is_coef[Y_C] = core->is_coef[U_C] = core->is_coef[V_C] = 0;   //TODO: Tim why we need to duplicate code here?
        xevd_mset(core->is_coef_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
        if (ctx->pps.cu_qp_delta_enabled_flag)
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
        xevd_eco_pred_mode(ctx, core);

        if (core->pred_mode == MODE_INTER)
        {
            if (ctx->sh.slice_type == SLICE_B)
            {
                xevd_eco_direct_mode_flag(ctx, core);
            }

            if (core->inter_dir == PRED_DIR)
            {

            }
            else
            {
                if (ctx->sh.slice_type == SLICE_B)
                {
                    xevd_eco_inter_pred_idc(ctx, core); /* inter_pred_idc */
                }

                for (int inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
                {
                    /* 0: forward, 1: backward */
                    if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
                    {
                        core->refi[inter_dir_idx] = xevd_eco_refi(bs, sbac, ctx->dpm.num_refp[inter_dir_idx]);
                        core->mvp_idx[inter_dir_idx] = xevd_eco_mvp_idx(bs, sbac);
                        xevd_eco_get_mvd(bs, sbac, core->mvd[inter_dir_idx]);
                    }
                }
            }
        }
        else if (core->pred_mode == MODE_INTRA)
        {
            xevd_get_mpm_b(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu,ctx->cod_eco, ctx->map_ipm, core->scup, ctx->w_scu
                         , &core->mpm_b_list, core->avail_lr, core->mpm_ext, core->pims, ctx->map_tidx);

            int luma_ipm = IPD_DC_B;
            core->ipm[0] = xevd_eco_intra_dir_b(bs, sbac, core->mpm_b_list, core->mpm_ext, core->pims);
            luma_ipm = core->ipm[0];
            core->ipm[1] = luma_ipm;

            SET_REFI(core->refi, REFI_INVALID, REFI_INVALID);
            core->mv[REFP_0][MV_X] = core->mv[REFP_0][MV_Y] = 0;
            core->mv[REFP_1][MV_X] = core->mv[REFP_1][MV_Y] = 0;
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
        ret = xevd_eco_coef(ctx, core);
        xevd_assert_rv(ret == XEVD_OK, ret);
    }

    return XEVD_OK;
}

int xevd_eco_nalu(XEVD_BSR * bs, XEVD_NALU * nalu)
{
    //nalu->nal_unit_size = xevd_bsr_read(bs, 32);
    xevd_bsr_read(bs, &nalu->forbidden_zero_bit, 1);

    if (nalu->forbidden_zero_bit != 0)
    {
        xevd_trace("malformed bitstream: forbidden_zero_bit != 0\n");
        return XEVD_ERR_MALFORMED_BITSTREAM;
    }

    xevd_bsr_read(bs, &nalu->nal_unit_type_plus1, 6);
    xevd_bsr_read(bs, &nalu->nuh_temporal_id, 3);
    xevd_bsr_read(bs, &nalu->nuh_reserved_zero_5bits, 5);

    if (nalu->nuh_reserved_zero_5bits != 0)
    {
        xevd_trace("malformed bitstream: nuh_reserved_zero_5bits != 0");
        return XEVD_ERR_MALFORMED_BITSTREAM;
    }

    xevd_bsr_read(bs, &nalu->nuh_extension_flag, 1);

    if (nalu->nuh_extension_flag != 0)
    {
        xevd_trace("malformed bitstream: nuh_extension_flag != 0");
        return XEVD_ERR_MALFORMED_BITSTREAM;
    }

    return XEVD_OK;
}


int xevd_eco_hrd_parameters(XEVD_BSR * bs, XEVD_HRD * hrd) {
    xevd_bsr_read_ue(bs, &hrd->cpb_cnt_minus1);
    xevd_bsr_read(bs, &hrd->bit_rate_scale, 4);
    xevd_bsr_read(bs, &hrd->cpb_size_scale, 4);
    for (int SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++)
    {
        xevd_bsr_read_ue(bs, &hrd->bit_rate_value_minus1[SchedSelIdx]);
        xevd_bsr_read_ue(bs, &hrd->cpb_size_value_minus1[SchedSelIdx]);
        xevd_bsr_read1(bs, &hrd->cbr_flag[SchedSelIdx]);
    }
    xevd_bsr_read(bs, &hrd->initial_cpb_removal_delay_length_minus1, 5);
    xevd_bsr_read(bs, &hrd->cpb_removal_delay_length_minus1, 5);
    xevd_bsr_read(bs, &hrd->cpb_removal_delay_length_minus1, 5);
    xevd_bsr_read(bs, &hrd->time_offset_length, 5);

    return XEVD_OK;
}

int xevd_eco_vui(XEVD_BSR * bs, XEVD_VUI * vui)
{
    xevd_bsr_read1(bs, &vui->aspect_ratio_info_present_flag);
    if (vui->aspect_ratio_info_present_flag)
    {
        xevd_bsr_read(bs, &vui->aspect_ratio_idc, 8);
        if (vui->aspect_ratio_idc == EXTENDED_SAR)
        {
            xevd_bsr_read(bs, &vui->sar_width, 16);
            xevd_bsr_read(bs, &vui->sar_height, 16);
        }
    }
    xevd_bsr_read1(bs, &vui->overscan_info_present_flag);
    if (vui->overscan_info_present_flag)
    {
        xevd_bsr_read1(bs, &vui->overscan_appropriate_flag);
    }
    xevd_bsr_read1(bs, &vui->video_signal_type_present_flag);
    if (vui->video_signal_type_present_flag)
    {
        xevd_bsr_read(bs, &vui->video_format, 3);
        xevd_bsr_read1(bs, &vui->video_full_range_flag);
        xevd_bsr_read1(bs, &vui->colour_description_present_flag);
        if (vui->colour_description_present_flag)
        {
            xevd_bsr_read(bs, &vui->colour_primaries, 8);
            xevd_bsr_read(bs, &vui->transfer_characteristics, 8);
            xevd_bsr_read(bs, &vui->matrix_coefficients, 8);
        }
    }
    xevd_bsr_read1(bs, &vui->chroma_loc_info_present_flag);
    if (vui->chroma_loc_info_present_flag)
    {
        xevd_bsr_read_ue(bs, &vui->chroma_sample_loc_type_top_field);
        xevd_bsr_read_ue(bs, &vui->chroma_sample_loc_type_bottom_field);
    }
    xevd_bsr_read1(bs, &vui->neutral_chroma_indication_flag);

    xevd_bsr_read1(bs, &vui->field_seq_flag);

    xevd_bsr_read1(bs, &vui->timing_info_present_flag);
    if (vui->timing_info_present_flag)
    {
        xevd_bsr_read(bs, &vui->num_units_in_tick, 32);
        xevd_bsr_read(bs, &vui->time_scale, 32);
        xevd_bsr_read1(bs, &vui->fixed_pic_rate_flag);
    }
    xevd_bsr_read1(bs, &vui->nal_hrd_parameters_present_flag);
    if (vui->nal_hrd_parameters_present_flag)
    {
        xevd_eco_hrd_parameters(bs, &vui->hrd_parameters);
    }
    xevd_bsr_read1(bs, &vui->vcl_hrd_parameters_present_flag);
    if (vui->vcl_hrd_parameters_present_flag)
    {
        xevd_eco_hrd_parameters(bs, &vui->hrd_parameters);
    }
    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    {
        xevd_bsr_read1(bs, &vui->low_delay_hrd_flag);
    }
    xevd_bsr_read1(bs, &vui->pic_struct_present_flag);
    xevd_bsr_read1(bs, &vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        xevd_bsr_read1(bs, &vui->motion_vectors_over_pic_boundaries_flag);
        xevd_bsr_read_ue(bs, &vui->max_bytes_per_pic_denom);
        xevd_bsr_read_ue(bs, &vui->max_bits_per_mb_denom);
        xevd_bsr_read_ue(bs, &vui->log2_max_mv_length_horizontal);
        xevd_bsr_read_ue(bs, &vui->log2_max_mv_length_vertical);
        xevd_bsr_read_ue(bs, &vui->num_reorder_pics);
        xevd_bsr_read_ue(bs, &vui->max_dec_pic_buffering);
    }

    return XEVD_OK;
}

int xevd_eco_sps(XEVD_BSR * bs, XEVD_SPS * sps)
{
#if TRACE_HLS
    XEVD_TRACE_STR("***********************************\n");
    XEVD_TRACE_STR("************ SPS Start ************\n");
#endif
    xevd_bsr_read_ue(bs, &sps->sps_seq_parameter_set_id);
    xevd_bsr_read(bs, &sps->profile_idc, 8);
    xevd_assert_rv((sps->profile_idc == PROFILE_BASELINE || sps->profile_idc == PROFILE_STILL_PIC_BASELINE) , XEVD_ERR);
    xevd_bsr_read(bs, &sps->level_idc, 8);
    xevd_bsr_read(bs, &sps->toolset_idc_h, 32);
    xevd_bsr_read(bs, &sps->toolset_idc_l, 32);
    xevd_bsr_read_ue(bs, &sps->chroma_format_idc);
    xevd_bsr_read_ue(bs, &sps->pic_width_in_luma_samples);
    xevd_bsr_read_ue(bs, &sps->pic_height_in_luma_samples);
    xevd_bsr_read_ue(bs, &sps->bit_depth_luma_minus8);
    xevd_bsr_read_ue(bs, &sps->bit_depth_chroma_minus8);
    xevd_bsr_read1(bs, &sps->sps_btt_flag);
    xevd_bsr_read1(bs, &sps->sps_suco_flag);
    xevd_bsr_read1(bs, &sps->tool_admvp);
    xevd_bsr_read1(bs, &sps->tool_eipd);
    xevd_bsr_read1(bs, &sps->tool_cm_init);
    xevd_bsr_read1(bs, &sps->tool_iqt);
    xevd_bsr_read1(bs, &sps->tool_addb);
    xevd_bsr_read1(bs, &sps->tool_alf);
    xevd_bsr_read1(bs, &sps->tool_htdf);
    xevd_bsr_read1(bs, &sps->tool_rpl);
    xevd_bsr_read1(bs, &sps->tool_pocs);
    xevd_bsr_read1(bs, &sps->dquant_flag);
    xevd_bsr_read1(bs, &sps->tool_dra);

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
    {
        xevd_eco_vui(bs, &(sps->vui_parameters));
    }

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

int xevd_eco_pps(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps)
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
    xevd_bsr_read_ue(bs, &pps->tile_id_len_minus1);
    xevd_bsr_read1(bs, &pps->explicit_tile_id_flag);
    xevd_bsr_read1(bs, &pps->pic_dra_enabled_flag);
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

int xevd_eco_sh(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps, XEVD_SH * sh, int nut)
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
    }

    xevd_bsr_read1(bs, &sh->deblocking_filter_on);

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

int xevd_eco_sei(XEVD_CTX * ctx, XEVD_BSR * bs)
{
#if TRACE_HLS
    XEVD_TRACE_STR("************ SEI Start   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    u32 payload_type, payload_size;
    u32 pic_sign[N_C][16];

    /* should be aligned before adding user data */
    xevd_assert_rv(XEVD_BSR_IS_BYTE_ALIGN(bs), XEVD_ERR_UNKNOWN);

    payload_type = 0;
    u32 val = 0;

    do
    {
        xevd_bsr_read(bs, &val, 8);
        payload_type += val;
    } while (val == 0xFF);

    payload_size = 0;
    val = 0;
    do
    {
        xevd_bsr_read(bs, &val, 8);
        payload_size += val;
    } while (val == 0xFF);

    switch (payload_type)
    {
    case XEVD_USER_DATA_UNREGISTERED:
        xevd_assert(payload_size >= ISO_IEC_11578_LEN);
        u32 val;

        for (u32 i = 0; i < ISO_IEC_11578_LEN; i++)
        {
            u8 uuid_iso_iec_11578_out[16];
            xevd_bsr_read(bs, &val, 8);
            uuid_iso_iec_11578_out[i] = val;
        }

        u32 sei_resize = payload_size - ISO_IEC_11578_LEN;
        for (u32 i = 0; i < sei_resize; i++)
        {
            xevd_bsr_read(bs, &val, 8);
        }
        break;
    case XEVD_UD_PIC_SIGNATURE:
        /* read signature (HASH) from bitstream */
        for (int i = 0; i < ctx->pic[0].imgb->np; ++i)
        {
            for (u32 j = 0; j < payload_size; ++j)
            {
                xevd_bsr_read(bs, &pic_sign[i][j], 8);
                ctx->pic_sign[i][j] = pic_sign[i][j];
            }
        }
        ctx->pic_sign_exist = 1;
        break;

    default:
        xevd_assert_rv(0, XEVD_ERR_UNEXPECTED);
    }
#if TRACE_HLS
    XEVD_TRACE_STR("************ SEI End   ************\n");
    XEVD_TRACE_STR("***********************************\n");
#endif
    return XEVD_OK;
}

u32  xevd_eco_tile_end_flag(XEVD_BSR * bs, XEVD_SBAC * sbac)
{
    return xevd_sbac_decode_bin_trm(bs, sbac);
}

s32  xevd_eco_cabac_zero_word(XEVD_BSR* bs)
{
    u32 cabac_zero_word = 1;
    while ((bs->cur < bs->end) || (bs->leftbits != 0))
    {
        xevd_bsr_read(bs, &cabac_zero_word, 16);
        xevd_assert_rv(cabac_zero_word == 0, XEVD_ERR);
    }
    return XEVD_OK;
}

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


#include <math.h>
#include "xevdm_def.h"
#include "xevdm_eco.h"
#include "xevdm_df.h"
#include "xevdm_alf.h"


/* convert XEVD into XEVD_CTX */
#define XEVD_ID_TO_CTX_R(id, ctx) \
    xevd_assert_r((id)); \
    (ctx) = (XEVD_CTX *)id; \
    xevd_assert_r((ctx)->magic == XEVD_MAGIC_CODE);

/* convert XEVD into XEVD_CTX with return value if assert on */
#define XEVD_ID_TO_CTX_RV(id, ctx, ret) \
    xevd_assert_rv((id), (ret)); \
    (ctx) = (XEVD_CTX *)id; \
    xevd_assert_rv((ctx)->magic == XEVD_MAGIC_CODE, (ret));

static XEVD_CTX * ctx_alloc(void)
{
    XEVD_CTX * ctx;

    ctx = (XEVD_CTX*)xevd_malloc_fast(sizeof(XEVD_CTX));

    xevd_assert_rv(ctx != NULL, NULL);
    xevd_mset_x64a(ctx, 0, sizeof(XEVD_CTX));

    /* set default value */
    ctx->pic_cnt       = 0;

    return ctx;
}

static XEVDM_CTX * xevdm_ctx_alloc(void)
{
    XEVDM_CTX * ctx;

    ctx = (XEVDM_CTX*)xevd_malloc_fast(sizeof(XEVDM_CTX));

    xevd_assert_rv(ctx != NULL, NULL);
    xevd_mset_x64a(ctx, 0, sizeof(XEVDM_CTX));

    ctx->aps_gen_array = (XEVD_APS_GEN *)xevd_malloc(2 * sizeof(XEVD_APS_GEN));
    //xevd_assert_rv(ctx->aps_gen_array != NULL, NULL);
    xevd_mset_x64a(ctx->aps_gen_array, 0, 2 * sizeof(XEVD_APS_GEN));

    ctx->dra_array = (SIG_PARAM_DRA *)xevd_malloc(32 * sizeof(SIG_PARAM_DRA));
    //xevd_assert_rv(ctx->dra_array != NULL, NULL);
    xevd_mset_x64a(ctx->dra_array, 0, 32 * sizeof(SIG_PARAM_DRA));
    return ctx;
}
static void ctx_free(XEVD_CTX * ctx)
{
    XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
    xevd_mfree(mctx->aps_gen_array);
    xevd_mfree(mctx->dra_array);
    xevd_mfree_fast(ctx);
}

static XEVD_CORE * core_alloc(void)
{
    XEVD_CORE * core;

    core = (XEVD_CORE*)xevd_malloc_fast(sizeof(XEVD_CORE));

    xevd_assert_rv(core, NULL);
    xevd_mset_x64a(core, 0, sizeof(XEVD_CORE));

    return core;
}
static XEVDM_CORE * xevdm_core_alloc(void)
{
    XEVDM_CORE * core;

    core = (XEVDM_CORE*)xevd_malloc_fast(sizeof(XEVDM_CORE));

    xevd_assert_rv(core, NULL);
    xevd_mset_x64a(core, 0, sizeof(XEVDM_CORE));

    return core;
}
static void core_free(XEVD_CORE * core)
{
    xevd_mfree_fast(core);
}

void xevd_free_1d(void** dst, int size)
{
     xevd_mfree_fast(*dst);
}

void xevd_free_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    xevd_mfree_fast((*dst)[0]);
    xevd_mfree_fast(*dst);
}

int xevd_delete_cu_data(XEVD_CU_DATA* cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    xevd_free_1d((void**)&cu_data->qp_y, size_8b);
    xevd_free_1d((void**)&cu_data->qp_u, size_8b);
    xevd_free_1d((void**)&cu_data->qp_v, size_8b);
    xevd_free_1d((void**)&cu_data->pred_mode, size_8b);
    xevd_free_1d((void**)&cu_data->pred_mode_chroma, size_8b);

    xevd_free_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));

    xevd_free_1d((void**)&cu_data->skip_flag, size_8b);
    xevd_free_1d((void**)&cu_data->ibc_flag, size_8b);
    xevd_free_1d((void**)&cu_data->dmvr_flag, size_8b);

    xevd_free_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    xevd_free_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));

    xevd_free_1d((void**)&cu_data->mvr_idx, size_8b);
    xevd_free_1d((void**)&cu_data->bi_idx, size_8b);
    xevd_free_1d((void**)&cu_data->inter_dir, size_8b);
    xevd_free_1d((void**)&cu_data->mmvd_idx, size_16b);
    xevd_free_1d((void**)&cu_data->mmvd_flag, size_8b);

    xevd_free_1d((void**)&cu_data->ats_intra_cu, size_8b);
    xevd_free_1d((void**)&cu_data->ats_mode_h, size_8b);
    xevd_free_1d((void**)&cu_data->ats_mode_v, size_8b);

    xevd_free_1d((void**)&cu_data->ats_inter_info, size_8b);

    for (i = 0; i < N_C; i++)
    {
        xevd_free_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xevd_free_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    xevd_free_1d((void**)&cu_data->map_scu, size_32b);
    xevd_free_1d((void**)&cu_data->affine_flag, size_8b);
    xevd_free_1d((void**)&cu_data->map_affine, size_32b);
    xevd_free_1d((void**)&cu_data->map_cu_mode, size_32b);
    xevd_free_1d((void**)&cu_data->depth, size_8b);

    for (i = 0; i < N_C; i++)
    {
        xevd_free_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i) * 2)) * sizeof(s16));
        xevd_free_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i) * 2)) * sizeof(pel));
    }

    return XEVD_OK;
}

static void sequence_deinit(XEVD_CTX * ctx)
{
    XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
    xevd_mfree(ctx->map_scu);
    xevd_mfree(ctx->cod_eco);
    xevd_mfree(ctx->map_split);
    xevd_mfree(ctx->map_ipm);
    xevd_mfree(mctx->map_suco);
    xevd_mfree(mctx->map_affine);
    xevd_mfree(ctx->map_cu_mode);
    xevd_mfree(mctx->map_ats_inter);

    for (int i = 0; i < ctx->f_lcu; i++)
    {
        xevd_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }

    xevd_mfree(ctx->map_cu_data);
    xevd_mfree_fast(ctx->map_tidx);
    xevd_mfree(ctx->tile);
    xevdm_picman_deinit(&mctx->dpm);
    free((void*)ctx->sync_flag);
	  ctx->sync_flag = NULL;
    free((void*)ctx->sync_row);
	  ctx->sync_row = NULL;

    XEVDM_SH  *msh = &mctx->sh;
    if (msh->alf_sh_param.alf_ctu_enable_flag != NULL)
    {
        xevd_mfree(msh->alf_sh_param.alf_ctu_enable_flag);
    }
}

int xevd_create_cu_data(XEVD_CU_DATA *cu_data, int log2_cuw, int log2_cuh);

static int picture_init(XEVD_CTX * ctx)
{
    ctx->w_tile = (ctx->pps.num_tile_columns_minus1 + 1);
    ctx->tile_cnt = (ctx->pps.num_tile_rows_minus1 + 1) * (ctx->pps.num_tile_columns_minus1 + 1);

    if (ctx->tc.task_num_in_tile == NULL)
    {
        ctx->tc.task_num_in_tile = (int*)xevd_malloc(sizeof(int) * ctx->tile_cnt);
        xevd_assert_rv(ctx->tc.task_num_in_tile, XEVD_ERR_OUT_OF_MEMORY);
    }
    else
    {
        xevd_mfree(ctx->tc.task_num_in_tile);
        ctx->tc.task_num_in_tile = (int*)xevd_malloc(sizeof(int) * ctx->tile_cnt);
        xevd_assert_rv(ctx->tc.task_num_in_tile, XEVD_ERR_OUT_OF_MEMORY);
    }

    if (ctx->tile == NULL)
    {
        int size = sizeof(XEVD_TILE) * ctx->tile_cnt;
        ctx->tile = xevd_malloc(size);
        xevd_assert_rv(ctx->tile, XEVD_ERR_OUT_OF_MEMORY);
    }
    else
    {
        xevd_mfree(ctx->tile);
        int size = sizeof(XEVD_TILE) * ctx->tile_cnt;
        ctx->tile = xevd_malloc(size);
        xevd_assert_rv(ctx->tile, XEVD_ERR_OUT_OF_MEMORY);
    }

    if (ctx->tc.max_task_cnt > 1)
    {
        if (ctx->tile_cnt > 1)
        {
            if (ctx->tc.max_task_cnt > (int) ctx->tile_cnt)
            {
                xevd_mset(ctx->tc.task_num_in_tile, 0, sizeof(int) * ctx->tile_cnt);
                ctx->tc.tile_task_num = ctx->tile_cnt;

                for (int i = 0; i < ctx->tc.max_task_cnt; i++)
                {
                    ctx->tc.task_num_in_tile[i % ctx->tile_cnt]++;
                }
            }
            else
            {
                ctx->tc.tile_task_num = ctx->tc.max_task_cnt;
                for (u32 i = 0; i < ctx->tile_cnt; i++)
                {
                    ctx->tc.task_num_in_tile[i] = 1;
                }
            }
        }
        else
        {
            ctx->tc.tile_task_num = 1;
            ctx->tc.task_num_in_tile[0] = ctx->tc.max_task_cnt;
        }
    }
    else
    {
        ctx->tc.tile_task_num = 1;
        for (u32 i = 0; i < ctx->tile_cnt; i++)
        {
            ctx->tc.task_num_in_tile[i] = 1;
        }
    }

    return XEVD_OK;
}

static int sequence_init(XEVD_CTX * ctx, XEVD_SPS * sps)
{
    int size;
    int ret;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    if(sps->pic_width_in_luma_samples != ctx->w || sps->pic_height_in_luma_samples != ctx->h)
    {
        /* resolution was changed */
        sequence_deinit(ctx);

        ctx->w = sps->pic_width_in_luma_samples;
        ctx->h = sps->pic_height_in_luma_samples;

        if (sps->sps_btt_flag)
        {
            ctx->max_cuwh = 1 << (sps->log2_ctu_size_minus5 + 5);
            ctx->min_cuwh = 1 << (sps->log2_min_cb_size_minus2 + 2);
        }
        else
        {
            ctx->max_cuwh = 1 << 6;
            ctx->min_cuwh = 1 << 2;
        }

        ctx->log2_max_cuwh = XEVD_CONV_LOG2(ctx->max_cuwh);
        ctx->log2_min_cuwh = XEVD_CONV_LOG2(ctx->min_cuwh);
    }

    size = ctx->max_cuwh;
    ctx->w_lcu = (ctx->w + (size - 1)) / size;
    ctx->h_lcu = (ctx->h + (size - 1)) / size;
    ctx->f_lcu = ctx->w_lcu * ctx->h_lcu;
    ctx->w_scu = (ctx->w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->h_scu = (ctx->h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->f_scu = ctx->w_scu * ctx->h_scu;

    ctx->internal_codec_bit_depth = sps->bit_depth_luma_minus8 + 8;
    ctx->internal_codec_bit_depth_luma = sps->bit_depth_luma_minus8 + 8;
    ctx->internal_codec_bit_depth_chroma = sps->bit_depth_chroma_minus8 + 8;

    mctx->alf = new_alf(ctx->internal_codec_bit_depth);
    ADAPTIVE_LOOP_FILTER* alf = (ADAPTIVE_LOOP_FILTER*)(mctx->alf);
    xevd_alf_create(alf, ctx->w, ctx->h, ctx->max_cuwh, ctx->max_cuwh, 5, sps->chroma_format_idc, ctx->internal_codec_bit_depth);

    XEVDM_SH  *msh = &mctx->sh;
    if (msh->alf_sh_param.alf_ctu_enable_flag == NULL)
    {
        msh->alf_sh_param.alf_ctu_enable_flag = (u8 *)malloc(N_C * ctx->f_lcu * sizeof(u8));
        xevd_assert_gv(msh->alf_sh_param.alf_ctu_enable_flag, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(msh->alf_sh_param.alf_ctu_enable_flag, 0, N_C * ctx->f_lcu * sizeof(u8));
    }

    /* alloc SCU map */
    if(ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = (u32 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_scu, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_scu, 0, size);
    }

    if (ctx->cod_eco == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->cod_eco = (u8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->cod_eco, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->cod_eco, 0, size);
    }
    /* alloc affine SCU map */
    if (mctx->map_affine == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        mctx->map_affine = (u32 *)xevd_malloc(size);
        xevd_assert_gv(mctx->map_affine, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(mctx->map_affine, 0, size);
    }

    /* alloc cu mode SCU map */
    if(ctx->map_cu_mode == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_cu_mode = (u32 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_cu_mode, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_cu_mode, 0, size);
    }

    if (mctx->map_ats_inter == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        mctx->map_ats_inter = (u8 *)xevd_malloc(size);
        xevd_assert_gv(mctx->map_ats_inter, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(mctx->map_ats_inter, 0, size);
    }
    /* alloc map for CU split flag */
    if(ctx->map_split == NULL)
    {
        size = sizeof(s8) * ctx->f_lcu * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_LCU;
        ctx->map_split = xevd_malloc(size);
        xevd_assert_gv(ctx->map_split, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_split, 0, size);
    }

    /* alloc map for LCU suco flag */
    if(mctx->map_suco == NULL)
    {
        size = sizeof(s8) * ctx->f_lcu * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_LCU;
        mctx->map_suco = xevd_malloc(size);
        xevd_assert_gv(mctx->map_suco, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(mctx->map_suco, 0, size);
    }

    /* alloc map for intra prediction mode */
    if(ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = (s8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_ipm, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_ipm, -1, size);
    }

    /* alloc tile index map in SCU unit */
    if (ctx->map_tidx == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_tidx = (u8 *)xevd_malloc(size);
        xevd_assert_gv(ctx->map_tidx, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_tidx, 0, size);
    }

    if(ctx->map_cu_data == NULL)
    {
        size = sizeof(XEVD_CU_DATA) * ctx->f_lcu;
        ctx->map_cu_data = (XEVD_CU_DATA*)xevd_malloc_fast(size);
        xevd_assert_gv(ctx->map_cu_data, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        xevd_mset_x64a(ctx->map_cu_data, 0, size);
        for(int i = 0; i < (int)ctx->f_lcu; i++)
        {
            xevd_create_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
        }
    }

    /* initialize reference picture manager */
    ctx->pa.fn_alloc = xevdm_picbuf_alloc;
    ctx->pa.fn_free = xevdm_picbuf_free;
    ctx->pa.w = ctx->w;
    ctx->pa.h = ctx->h;
    ctx->pa.pad_l = PIC_PAD_SIZE_L;
    ctx->pa.pad_c = PIC_PAD_SIZE_C;
    ctx->ref_pic_gap_length = (int)pow(2.0, sps->log2_ref_pic_gap_length);
    ctx->pa.idc = sps->chroma_format_idc;

    ret = xevdm_picman_init(&mctx->dpm, MAXM_PB_SIZE, XEVD_MAX_NUM_REF_PICS, &ctx->pa);
    xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

    xevdm_split_tbl_init(ctx, sps);

    xevd_set_chroma_qp_tbl_loc(sps->bit_depth_luma_minus8 + 8);
    if(sps->tool_iqt == 0)
    {
        xevd_tbl_qp_chroma_adjust = xevd_tbl_qp_chroma_adjust_base;
    }
    else
    {
        xevd_tbl_qp_chroma_adjust = xevd_tbl_qp_chroma_adjust_main;
    }

    if (sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        xevd_derived_chroma_qp_mapping_tables(&(sps->chroma_qp_table_struct), sps->bit_depth_chroma_minus8 + 8);
    }
    else
    {
        xevd_mcpy(&(xevd_tbl_qp_chroma_dynamic_ext[0][6 * sps->bit_depth_chroma_minus8]), xevd_tbl_qp_chroma_adjust, MAX_QP_TABLE_SIZE * sizeof(int));
        xevd_mcpy(&(xevd_tbl_qp_chroma_dynamic_ext[1][6 * sps->bit_depth_chroma_minus8]), xevd_tbl_qp_chroma_adjust, MAX_QP_TABLE_SIZE * sizeof(int));
    }

    ctx->sync_flag = (volatile s32 *)xevd_malloc(ctx->f_lcu * sizeof(int));
    xevd_mset((void *)ctx->sync_flag, 0, ctx->f_lcu * sizeof(ctx->sync_flag[0]));

    ctx->sync_row = (volatile s32 *)xevd_malloc(ctx->h_lcu * sizeof(int));
    xevd_mset((void *)ctx->sync_row, 0, ctx->h_lcu * sizeof(ctx->sync_row[0]));

    if (sps->vui_parameters_present_flag && sps->vui_parameters.bitstream_restriction_flag)
    {
        ctx->max_coding_delay = sps->vui_parameters.num_reorder_pics;
    }
	else
    {
        ctx->max_coding_delay = sps->sps_max_dec_pic_buffering_minus1 + 1;
    }

	if (sps->vui_parameters_present_flag && sps->vui_parameters.timing_info_present_flag)
	{
		ctx->frame_rate_num = sps->vui_parameters.num_units_in_tick;
		ctx->frame_rate_den = sps->vui_parameters.time_scale;
	}

    xevd_mset_x64a(ctx->cod_eco, 0, sizeof(u8) * ctx->f_scu);
    return XEVD_OK;
ERR:
    sequence_deinit(ctx);

    return ret;
}

static void slice_deinit(XEVD_CTX * ctx)
{
}

static int slice_init(XEVD_CTX * ctx, XEVD_CORE * core, XEVD_SH * sh)
{
    core->lcu_num = 0;
    core->x_lcu = 0;
    core->y_lcu = 0;
    core->x_pel = 0;
    core->y_pel = 0;
    core->qp_y = ctx->sh.qp + 6 * ctx->sps->bit_depth_luma_minus8;
    core->qp_u = xevd_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps->bit_depth_chroma_minus8;
    core->qp_v = xevd_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps->bit_depth_chroma_minus8;

    if (ctx->tc.max_task_cnt > 1)
    {
        xevd_mset((void *)ctx->sync_flag, 0, ctx->f_lcu * sizeof(ctx->sync_flag[0]));
        xevd_mset((void *)ctx->sync_row, 0, ctx->h_lcu * sizeof(ctx->sync_row[0]));
    }

    if(ctx->sh.slice_type == SLICE_I)
    {
        ctx->last_intra_poc = ctx->poc.poc_val;
    }

    if (ctx->sps->tool_hmvp)
    {
        xevdm_hmvp_init(core);
    }
    return XEVD_OK;
}

static int xevdm_hmvp_init(XEVD_CORE * core)
{
    xevd_mset(core->history_buffer.history_mv_table, 0, ALLOWED_CHECKED_NUM * REFP_NUM * MV_D * sizeof(s16));

    for (int i = 0; i < ALLOWED_CHECKED_NUM; i++)
    {
        core->history_buffer.history_refi_table[i][REFP_0] = REFI_INVALID;
        core->history_buffer.history_refi_table[i][REFP_1] = REFI_INVALID;
    }

    core->history_buffer.currCnt = 0;
    core->history_buffer.m_maxCnt = ALLOWED_CHECKED_NUM;
    return core->history_buffer.currCnt;
}

static void make_stat(XEVD_CTX * ctx, int nalu_type, XEVD_STAT * stat)
{
    int i, j;
    stat->nalu_type = nalu_type;
    stat->stype = 0;
    stat->fnum = -1;
    if(ctx)
    {
        stat->read += XEVD_BSR_GET_READ_BYTE(&ctx->bs);
        if(nalu_type < XEVD_NUT_SPS)
        {
            stat->fnum = ctx->pic_cnt;
            stat->stype = ctx->sh.slice_type;

            /* increase decoded picture count */
            ctx->pic_cnt++;
            stat->poc = ctx->poc.poc_val;
            stat->tid = ctx->nalu.nuh_temporal_id;
            XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
            for(i = 0; i < 2; i++)
            {
                stat->refpic_num[i] = mctx->dpm.num_refp[i];
                for(j = 0; j < stat->refpic_num[i]; j++)
                {
                    stat->refpic[i][j] = ctx->refp[j][i].poc;
                }
            }
        }
    }
}

static void xevdm_itdq_main(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVDM_CORE * mcore = (XEVDM_CORE *)core;
    xevdm_sub_block_itdq(ctx, core->coef, core->log2_cuw, core->log2_cuh, core->qp_y, core->qp_u, core->qp_v, core->is_coef, core->is_coef_sub, ctx->sps->tool_iqt, core->pred_mode == MODE_IBC ? 0 : mcore->ats_intra_cu, core->pred_mode == MODE_IBC ? 0 : ((mcore->ats_intra_mode_h << 1) | mcore->ats_intra_mode_v), core->pred_mode == MODE_IBC ? 0 : mcore->ats_inter_info, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
}

static void get_nbr_yuv(int x, int y, int cuw, int cuh, XEVD_CTX * ctx, XEVD_CORE * core)
{
    int  s_rec;
    pel *rec;
    int constrained_intra_flag = core->pred_mode == MODE_INTRA && ctx->pps.constrained_intra_pred_flag;

    if (xevd_check_luma(ctx, core))
    {
        /* Y */
        s_rec = ctx->pic->s_l;
        rec = ctx->pic->y + (y * s_rec) + x;
        if (ctx->sps->tool_eipd)
        {
            xevdm_get_nbr(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, Y_C, constrained_intra_flag, ctx->map_tidx, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        }
        else
        {
            xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, Y_C, constrained_intra_flag,ctx->map_tidx, ctx->sps->bit_depth_luma_minus8+8, ctx->sps->chroma_format_idc);
        }
    }
    if (xevd_check_chroma(ctx, core) && ctx->sps->chroma_format_idc)
    {
        cuw >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        cuh >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
        x >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        y >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
        s_rec = ctx->pic->s_c;

        /* U */
        rec = ctx->pic->u + (y * s_rec) + x;
        if (ctx->sps->tool_eipd)
        {
            xevdm_get_nbr(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, U_C, constrained_intra_flag, ctx->map_tidx, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        }
        else
        {
            xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, U_C, constrained_intra_flag, ctx->map_tidx, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        }

        /* V */
        rec = ctx->pic->v + (y * s_rec) + x;
        if (ctx->sps->tool_eipd)
        {
            xevdm_get_nbr(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, V_C, constrained_intra_flag, ctx->map_tidx, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        }
        else
        {
            xevd_get_nbr_b(x, y, cuw, cuh, rec, s_rec, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, V_C, constrained_intra_flag, ctx->map_tidx, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->chroma_format_idc);
        }
    }
}

static void update_history_buffer_parse_affine(XEVD_CORE *core, int slice_type)
{
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    int i;
    if(core->history_buffer.currCnt == core->history_buffer.m_maxCnt)
    {
        for(i = 1; i < core->history_buffer.currCnt; i++)
        {
            xevd_mcpy(core->history_buffer.history_mv_table[i - 1], core->history_buffer.history_mv_table[i], REFP_NUM * MV_D * sizeof(s16));
            xevd_mcpy(core->history_buffer.history_refi_table[i - 1], core->history_buffer.history_refi_table[i], REFP_NUM * sizeof(s8));
        }
        if(mcore->affine_flag)
        {
            mcore->mv_sp[REFP_0][MV_X] = 0;
            mcore->mv_sp[REFP_0][MV_Y] = 0;
            mcore->refi_sp[REFP_0] = REFI_INVALID;
            mcore->mv_sp[REFP_1][MV_X] = 0;
            mcore->mv_sp[REFP_1][MV_Y] = 0;
            mcore->refi_sp[REFP_1] = REFI_INVALID;
            for (int lidx = 0; lidx < REFP_NUM; lidx++)
            {
                if (core->refi[lidx] >= 0)
                {
                    s16(*ac_mv)[MV_D] = mcore->affine_mv[lidx];
                    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
                    int mv_scale_hor = ac_mv[0][MV_X] << 7;
                    int mv_scale_ver = ac_mv[0][MV_Y] << 7;
                    int mv_y_hor = mv_scale_hor;
                    int mv_y_ver = mv_scale_ver;
                    int mv_scale_tmp_hor, mv_scale_tmp_ver;


                    dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);
                    dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);

                    if (mcore->affine_flag == 2)
                    {
                        dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh);
                        dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
                    }
                    else
                    {
                        dmv_ver_x = -dmv_hor_y;
                        dmv_ver_y = dmv_hor_x;
                    }
                    int pos_x = 1 << (core->log2_cuw - 1);
                    int pos_y = 1 << (core->log2_cuh - 1);

                    mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                    mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                    xevdm_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 7, 0);

                    mv_scale_tmp_hor = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_hor);
                    mv_scale_tmp_ver = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_ver);

                    mcore->mv_sp[lidx][MV_X] = mv_scale_tmp_hor;
                    mcore->mv_sp[lidx][MV_Y] = mv_scale_tmp_ver;
                    mcore->refi_sp[lidx] = core->refi[lidx];

                }
            }
            // some spatial neighbor may be unavailable
            if((slice_type == SLICE_P && REFI_IS_VALID(mcore->refi_sp[REFP_0])) ||
                (slice_type == SLICE_B && (REFI_IS_VALID(mcore->refi_sp[REFP_0]) || REFI_IS_VALID(mcore->refi_sp[REFP_1]))))
            {
                xevd_mcpy(core->history_buffer.history_mv_table[core->history_buffer.currCnt - 1], mcore->mv_sp, REFP_NUM * MV_D * sizeof(s16));
                xevd_mcpy(core->history_buffer.history_refi_table[core->history_buffer.currCnt - 1], mcore->refi_sp, REFP_NUM * sizeof(s8));
            }
        }
        else
        {
            xevd_mcpy(core->history_buffer.history_mv_table[core->history_buffer.currCnt - 1], core->mv, REFP_NUM * MV_D * sizeof(s16));
            xevd_mcpy(core->history_buffer.history_refi_table[core->history_buffer.currCnt - 1], core->refi, REFP_NUM * sizeof(s8));
        }
    }
    else
    {
        if(mcore->affine_flag)
        {
            mcore->mv_sp[REFP_0][MV_X] = 0;
            mcore->mv_sp[REFP_0][MV_Y] = 0;
            mcore->refi_sp[REFP_0] = REFI_INVALID;
            mcore->mv_sp[REFP_1][MV_X] = 0;
            mcore->mv_sp[REFP_1][MV_Y] = 0;
            mcore->refi_sp[REFP_1] = REFI_INVALID;
            for (int lidx = 0; lidx < REFP_NUM; lidx++)
            {
                if (core->refi[lidx] >= 0)
                {
                    s16(*ac_mv)[MV_D] = mcore->affine_mv[lidx];
                    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
                    int mv_scale_hor = ac_mv[0][MV_X] << 7;
                    int mv_scale_ver = ac_mv[0][MV_Y] << 7;
                    int mv_y_hor = mv_scale_hor;
                    int mv_y_ver = mv_scale_ver;
                    int mv_scale_tmp_hor, mv_scale_tmp_ver;

                    dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);
                    dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);

                    if (mcore->affine_flag == 2)
                    {
                        dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh);
                        dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
                    }
                    else
                    {
                        dmv_ver_x = -dmv_hor_y;
                        dmv_ver_y = dmv_hor_x;
                    }
                    int pos_x = 1 << (core->log2_cuw - 1);
                    int pos_y = 1 << (core->log2_cuh - 1);

                    mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                    mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                    xevdm_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 7, 0);
                    mv_scale_tmp_hor = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_hor);
                    mv_scale_tmp_ver = XEVD_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_ver);

                    mcore->mv_sp[lidx][MV_X] = mv_scale_tmp_hor;
                    mcore->mv_sp[lidx][MV_Y] = mv_scale_tmp_ver;
                    mcore->refi_sp[lidx] = core->refi[lidx];
                }
            }
            if((slice_type == SLICE_P && REFI_IS_VALID(mcore->refi_sp[REFP_0])) ||
                (slice_type == SLICE_B && (REFI_IS_VALID(mcore->refi_sp[REFP_0]) || REFI_IS_VALID(mcore->refi_sp[REFP_1]))))
            {
                xevd_mcpy(core->history_buffer.history_mv_table[core->history_buffer.currCnt], mcore->mv_sp, REFP_NUM * MV_D * sizeof(s16));
                xevd_mcpy(core->history_buffer.history_refi_table[core->history_buffer.currCnt], mcore->refi_sp, REFP_NUM * sizeof(s8));
            }
        }
        else
        {
            xevd_mcpy(core->history_buffer.history_mv_table[core->history_buffer.currCnt], core->mv, REFP_NUM * MV_D * sizeof(s16));
            xevd_mcpy(core->history_buffer.history_refi_table[core->history_buffer.currCnt], core->refi, REFP_NUM * sizeof(s8));
        }

        core->history_buffer.currCnt++;
    }
}

void xevd_get_direct_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    s8            srefi[REFP_NUM][MAXM_NUM_MVP];
    s16           smvp[REFP_NUM][MAXM_NUM_MVP][MV_D];
    u32           cuw, cuh;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    if (ctx->sps->tool_admvp == 0)
    {
        xevdm_get_motion_skip_baseline(ctx->sh.slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], cuw, cuh, ctx->w_scu, srefi, smvp, core->avail_cu);
    }
    else
    {
        xevdm_get_motion_merge_main(ctx->poc.poc_val, ctx->sh.slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], cuw, cuh, ctx->w_scu, ctx->h_scu, srefi, smvp, ctx->map_scu, core->avail_lr
            , mctx->map_unrefined_mv, core->history_buffer, mcore->ibc_flag, (XEVD_REFP(*)[2])ctx->refp[0], &ctx->sh, ctx->log2_max_cuwh, ctx->map_tidx);
    }

    core->refi[REFP_0] = srefi[REFP_0][core->mvp_idx[REFP_0]];
    core->refi[REFP_1] = srefi[REFP_1][core->mvp_idx[REFP_1]];

    core->mv[REFP_0][MV_X] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_X];
    core->mv[REFP_0][MV_Y] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_Y];

    if (ctx->sh.slice_type == SLICE_P)
    {
        core->refi[REFP_1] = REFI_INVALID;
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
    }
    else
    {
        core->mv[REFP_1][MV_X] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_X];
        core->mv[REFP_1][MV_Y] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_Y];
    }
}

void xevd_get_skip_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int REF_SET[3][XEVD_MAX_NUM_ACTIVE_REF_FRAME] = { {0,0,}, };
    int cuw, cuh, inter_dir = 0;
    s8            srefi[REFP_NUM][MAXM_NUM_MVP];
    s16           smvp[REFP_NUM][MAXM_NUM_MVP][MV_D];

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    if (ctx->sps->tool_mmvd && mcore->mmvd_flag)
    {
        xevdm_get_mmvd_motion(ctx, core);
    }
    else
    {
        if(ctx->sps->tool_admvp == 0)
        {
            xevdm_get_motion(core->scup, REFP_0, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, srefi[REFP_0], smvp[REFP_0]);

            core->refi[REFP_0] = srefi[REFP_0][core->mvp_idx[REFP_0]];

            core->mv[REFP_0][MV_X] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_X];
            core->mv[REFP_0][MV_Y] = smvp[REFP_0][core->mvp_idx[REFP_0]][MV_Y];

            if (ctx->sh.slice_type == SLICE_P)
            {
                core->refi[REFP_1] = REFI_INVALID;
                core->mv[REFP_1][MV_X] = 0;
                core->mv[REFP_1][MV_Y] = 0;
            }
            else
            {
                xevdm_get_motion(core->scup, REFP_1, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, srefi[REFP_1], smvp[REFP_1]);

                core->refi[REFP_1] = srefi[REFP_1][core->mvp_idx[REFP_1]];
                core->mv[REFP_1][MV_X] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_X];
                core->mv[REFP_1][MV_Y] = smvp[REFP_1][core->mvp_idx[REFP_1]][MV_Y];
            }
        }
        else
        {
            xevd_get_direct_motion(ctx, core);
        }
    }
}

void xevd_get_inter_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int cuw, cuh;
    s16           mvp[MAXM_NUM_MVP][MV_D];
    s8            refi[MAXM_NUM_MVP];
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    int inter_dir_idx;
    for (inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
    {
        /* 0: forward, 1: backward */
        if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
        {
            if(ctx->sps->tool_admvp == 0)
            {
                xevdm_get_motion(core->scup, inter_dir_idx, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, core->avail_cu, refi, mvp);
                core->mv[inter_dir_idx][MV_X] = mvp[core->mvp_idx[inter_dir_idx]][MV_X] + core->mvd[inter_dir_idx][MV_X];
                core->mv[inter_dir_idx][MV_Y] = mvp[core->mvp_idx[inter_dir_idx]][MV_Y] + core->mvd[inter_dir_idx][MV_Y];
            }
            else
            {
                if (core->bi_idx == BI_FL0 || core->bi_idx == BI_FL1)
                {
                    core->refi[inter_dir_idx] = xevdm_get_first_refi(core->scup, inter_dir_idx, ctx->map_refi, ctx->map_mv, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, core->mvr_idx, core->avail_lr
                        , mctx->map_unrefined_mv, core->history_buffer, ctx->sps->tool_hmvp, ctx->map_tidx);
                }

                xevdm_get_motion_from_mvr(core->mvr_idx, ctx->poc.poc_val, core->scup, inter_dir_idx, core->refi[inter_dir_idx], mctx->dpm.num_refp[inter_dir_idx], ctx->map_mv, ctx->map_refi, ctx->refp, \
                    cuw, cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, mvp, refi, ctx->map_scu, core->avail_lr, mctx->map_unrefined_mv, core->history_buffer, ctx->sps->tool_hmvp, ctx->map_tidx);

                core->mvp_idx[inter_dir_idx] = 0;

                if (core->bi_idx == BI_FL0 + inter_dir_idx)
                {
                    core->mvd[inter_dir_idx][MV_X] = core->mvd[inter_dir_idx][MV_Y] = 0;
                }

                core->mv[inter_dir_idx][MV_X] = mvp[core->mvp_idx[inter_dir_idx]][MV_X] + (core->mvd[inter_dir_idx][MV_X] << core->mvr_idx);
                core->mv[inter_dir_idx][MV_Y] = mvp[core->mvp_idx[inter_dir_idx]][MV_Y] + (core->mvd[inter_dir_idx][MV_Y] << core->mvr_idx);
            }
        }
        else
        {
            core->refi[inter_dir_idx] = REFI_INVALID;
            core->mv[inter_dir_idx][MV_X] = 0;
            core->mv[inter_dir_idx][MV_Y] = 0;
        }
    }
}

void xevd_get_affine_motion(XEVD_CTX * ctx, XEVD_CORE * core)
{
    int          cuw, cuh;
    s16          affine_mvp[MAXM_NUM_MVP][VER_NUM][MV_D];
    s8           refi[MAXM_NUM_MVP];
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    if (core->pred_mode == MODE_SKIP || core->pred_mode == MODE_DIR) // affine merge motion vector
    {
        s16 aff_mrg_mvp[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D];
        s8  aff_refi[AFF_MAX_CAND][REFP_NUM];
        int vertex_num[AFF_MAX_CAND];
        int vertex, lidx;
        int mrg_idx = core->mvp_idx[0];

        xevdm_get_affine_merge_candidate(ctx->poc.poc_val, ctx->sh.slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp, cuw, cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, aff_refi, aff_mrg_mvp, vertex_num, ctx->map_scu, mctx->map_affine
            , ctx->log2_max_cuwh, mctx->map_unrefined_mv, core->avail_lr, &ctx->sh, ctx->map_tidx);

        mcore->affine_flag = vertex_num[core->mvp_idx[0]] - 1;

        for (lidx = 0; lidx < REFP_NUM; lidx++)
        {
            if (REFI_IS_VALID(aff_refi[mrg_idx][lidx]))
            {
                core->refi[lidx] = aff_refi[mrg_idx][lidx];
                for (vertex = 0; vertex < vertex_num[mrg_idx]; vertex++)
                {
                    mcore->affine_mv[lidx][vertex][MV_X] = aff_mrg_mvp[mrg_idx][lidx][vertex][MV_X];
                    mcore->affine_mv[lidx][vertex][MV_Y] = aff_mrg_mvp[mrg_idx][lidx][vertex][MV_Y];
                }
            }
            else
            {
                core->refi[lidx] = REFI_INVALID;
                core->mv[lidx][MV_X] = 0;
                core->mv[lidx][MV_Y] = 0;
            }
        }
    }
    else if (core->pred_mode == MODE_INTER) // affine inter motion vector
    {
        int vertex;
        int vertex_num = mcore->affine_flag + 1;
        int inter_dir_idx;
        for (inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
        {
            /* 0: forward, 1: backward */
            if (((core->inter_dir + 1) >> inter_dir_idx) & 1)
            {
                xevdm_get_affine_motion_scaling(ctx->poc.poc_val, core->scup, inter_dir_idx, core->refi[inter_dir_idx], mctx->dpm.num_refp[inter_dir_idx], ctx->map_mv, ctx->map_refi, ctx->refp
                                            , cuw, cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, affine_mvp, refi, ctx->map_scu, mctx->map_affine, vertex_num, core->avail_lr, ctx->log2_max_cuwh
                                            , mctx->map_unrefined_mv, ctx->map_tidx);

                for (vertex = 0; vertex < vertex_num; vertex++)
                {
                    mcore->affine_mv[inter_dir_idx][vertex][MV_X] = affine_mvp[core->mvp_idx[inter_dir_idx]][vertex][MV_X] + mcore->affine_mvd[inter_dir_idx][vertex][MV_X];
                    mcore->affine_mv[inter_dir_idx][vertex][MV_Y] = affine_mvp[core->mvp_idx[inter_dir_idx]][vertex][MV_Y] + mcore->affine_mvd[inter_dir_idx][vertex][MV_Y];
                    if (vertex == 0)
                    {
                        affine_mvp[core->mvp_idx[inter_dir_idx]][1][MV_X] += mcore->affine_mvd[inter_dir_idx][vertex][MV_X];
                        affine_mvp[core->mvp_idx[inter_dir_idx]][1][MV_Y] += mcore->affine_mvd[inter_dir_idx][vertex][MV_Y];
                        affine_mvp[core->mvp_idx[inter_dir_idx]][2][MV_X] += mcore->affine_mvd[inter_dir_idx][vertex][MV_X];
                        affine_mvp[core->mvp_idx[inter_dir_idx]][2][MV_Y] += mcore->affine_mvd[inter_dir_idx][vertex][MV_Y];
                    }
                }
            }
            else
            {
                core->refi[inter_dir_idx] = REFI_INVALID;
                for (vertex = 0; vertex < vertex_num; vertex++)
                {
                    mcore->affine_mv[inter_dir_idx][vertex][MV_X] = 0;
                    mcore->affine_mv[inter_dir_idx][vertex][MV_Y] = 0;
                }

                core->refi[inter_dir_idx] = REFI_INVALID;
                core->mv[inter_dir_idx][MV_X] = 0;
                core->mv[inter_dir_idx][MV_Y] = 0;
            }
        }
    }
}

static int cu_init(XEVD_CTX *ctx, XEVD_CORE *core, int x, int y, int cuw, int cuh)
{
    XEVD_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    int x_in_lcu = x - (core->x_lcu << ctx->log2_max_cuwh);
    int y_in_lcu = y - (core->y_lcu << ctx->log2_max_cuwh);
    int cup = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

    core->cuw = cuw;
    core->cuh = cuh;
    core->log2_cuw = XEVD_CONV_LOG2(cuw);
    core->log2_cuh = XEVD_CONV_LOG2(cuh);
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->skip_flag = 0;
    mcore->ibc_flag = 0;
    mcore->mmvd_flag = 0;
    mcore->affine_flag = cu_data->affine_flag[cup];
    mcore->ats_inter_info = cu_data->ats_inter_info[cup];
    mcore->ats_intra_cu = cu_data->ats_intra_cu[cup];
    mcore->ats_intra_mode_h = cu_data->ats_mode_h[cup];
    mcore->ats_intra_mode_v = cu_data->ats_mode_v[cup];

    mcore->dmvr_flag = cu_data->dmvr_flag[cup];
    mcore->ibc_flag = cu_data->ibc_flag[cup];

    core->qp_y = cu_data->qp_y[cup];
    core->qp_u = cu_data->qp_u[cup];
    core->qp_v = cu_data->qp_v[cup];
    core->qp = core->qp_y - (6 * ctx->sps->bit_depth_luma_minus8);

    for (int c = Y_C; c < N_C; c++)
    {
        core->is_coef[c] = cu_data->nnz[c][cup];
        for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
        {
            core->is_coef_sub[c][sb] = cu_data->nnz_sub[c][sb][cup];
        }
    }

    core->pred_mode = xevd_check_luma(ctx, core) ? cu_data->pred_mode[cup] : cu_data->pred_mode_chroma[cup];

    if (core->pred_mode == MODE_INTRA)
    {
        core->avail_cu = xevd_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->log2_cuw, core->log2_cuh, ctx->map_scu, ctx->map_tidx);

        core->mv[REFP_0][MV_X] = 0;
        core->mv[REFP_0][MV_Y] = 0;
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
        core->refi[REFP_0] = -1;
        core->refi[REFP_1] = -1;

        if (xevd_check_luma(ctx, core))
        {
            core->ipm[0] = cu_data->ipm[0][cup];
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
        if (xevd_check_chroma(ctx, core))
        {
            core->ipm[1] = cu_data->ipm[1][cup];
        }
    }
    else if (core->pred_mode == MODE_IBC)
    {
        mcore->ibc_flag = 1;

        core->refi[REFP_0] = -1;
        core->refi[REFP_1] = -1;
        core->mvp_idx[REFP_0] = cu_data->mvp_idx[cup][REFP_0];
        core->mvp_idx[REFP_1] = 0;
        core->mv[REFP_0][MV_X] = cu_data->mv[cup][REFP_0][MV_X];
        core->mv[REFP_0][MV_Y] = cu_data->mv[cup][REFP_0][MV_Y];
        core->mv[REFP_1][MV_X] = 0;
        core->mv[REFP_1][MV_Y] = 0;
        core->mvd[REFP_0][MV_X] = cu_data->mvd[cup][REFP_0][MV_X];
        core->mvd[REFP_0][MV_Y] = cu_data->mvd[cup][REFP_0][MV_Y];

        if (!xevd_check_luma(ctx, core))
        {
            xevd_assert(0);
        }
        mcore->mmvd_flag = 0;
        mcore->affine_flag = 0;
    }
    else
    {
        xevd_assert(xevd_check_luma(ctx, core));
        core->refi[REFP_0] = cu_data->refi[cup][REFP_0];
        core->refi[REFP_1] = cu_data->refi[cup][REFP_1];
        core->mvp_idx[REFP_0] = cu_data->mvp_idx[cup][REFP_0];
        core->mvp_idx[REFP_1] = cu_data->mvp_idx[cup][REFP_1];
        core->mvr_idx = cu_data->mvr_idx[cup];
        core->bi_idx = cu_data->bi_idx[cup];
        core->inter_dir = cu_data->inter_dir[cup];
        mcore->mmvd_idx = cu_data->mmvd_idx[cup];
        mcore->mmvd_flag = cu_data->mmvd_flag[cup];
        core->skip_flag = cu_data->skip_flag[cup];

        if (mcore->dmvr_flag)
        {
            mcore->dmvr_mv[cup][REFP_0][MV_X] = cu_data->mv[cup][REFP_0][MV_X];
            mcore->dmvr_mv[cup][REFP_0][MV_Y] = cu_data->mv[cup][REFP_0][MV_Y];
            mcore->dmvr_mv[cup][REFP_1][MV_X] = cu_data->mv[cup][REFP_1][MV_X];
            mcore->dmvr_mv[cup][REFP_1][MV_Y] = cu_data->mv[cup][REFP_1][MV_Y];

            core->mv[REFP_0][MV_X] = cu_data->unrefined_mv[cup][REFP_0][MV_X];
            core->mv[REFP_0][MV_Y] = cu_data->unrefined_mv[cup][REFP_0][MV_Y];
            core->mv[REFP_1][MV_X] = cu_data->unrefined_mv[cup][REFP_1][MV_X];
            core->mv[REFP_1][MV_Y] = cu_data->unrefined_mv[cup][REFP_1][MV_Y];
        }
        else
        {
            core->mv[REFP_0][MV_X] = cu_data->mv[cup][REFP_0][MV_X];
            core->mv[REFP_0][MV_Y] = cu_data->mv[cup][REFP_0][MV_Y];
            core->mv[REFP_1][MV_X] = cu_data->mv[cup][REFP_1][MV_X];
            core->mv[REFP_1][MV_Y] = cu_data->mv[cup][REFP_1][MV_Y];
        }

        core->mvd[REFP_0][MV_X] = cu_data->mvd[cup][REFP_0][MV_X];
        core->mvd[REFP_0][MV_Y] = cu_data->mvd[cup][REFP_0][MV_Y];
        core->mvd[REFP_1][MV_X] = cu_data->mvd[cup][REFP_1][MV_X];
        core->mvd[REFP_1][MV_Y] = cu_data->mvd[cup][REFP_1][MV_Y];

        if (mcore->affine_flag)
        {
            mcore->affine_bzero[REFP_0] = cu_data->affine_bzero[cup][REFP_0];
            mcore->affine_bzero[REFP_1] = cu_data->affine_bzero[cup][REFP_1];

            mcore->affine_mvd[REFP_0][0][MV_X] = cu_data->affine_mvd[cup][REFP_0][0][MV_X];
            mcore->affine_mvd[REFP_0][0][MV_Y] = cu_data->affine_mvd[cup][REFP_0][0][MV_Y];
            mcore->affine_mvd[REFP_0][1][MV_X] = cu_data->affine_mvd[cup][REFP_0][1][MV_X];
            mcore->affine_mvd[REFP_0][1][MV_Y] = cu_data->affine_mvd[cup][REFP_0][1][MV_Y];
            mcore->affine_mvd[REFP_0][2][MV_X] = cu_data->affine_mvd[cup][REFP_0][2][MV_X];
            mcore->affine_mvd[REFP_0][2][MV_Y] = cu_data->affine_mvd[cup][REFP_0][2][MV_Y];

            mcore->affine_mvd[REFP_1][0][MV_X] = cu_data->affine_mvd[cup][REFP_1][0][MV_X];
            mcore->affine_mvd[REFP_1][0][MV_Y] = cu_data->affine_mvd[cup][REFP_1][0][MV_Y];
            mcore->affine_mvd[REFP_1][1][MV_X] = cu_data->affine_mvd[cup][REFP_1][1][MV_X];
            mcore->affine_mvd[REFP_1][1][MV_Y] = cu_data->affine_mvd[cup][REFP_1][1][MV_Y];
            mcore->affine_mvd[REFP_1][2][MV_X] = cu_data->affine_mvd[cup][REFP_1][2][MV_X];
            mcore->affine_mvd[REFP_1][2][MV_Y] = cu_data->affine_mvd[cup][REFP_1][2][MV_Y];
        }
    }

    core->avail_lr = xevd_check_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);
    mcore->dmvr_flag = 0;
    return XEVD_OK;
}

static void coef_rect_to_series(XEVD_CTX * ctx, s16 *coef_src[N_C], int x, int y, int cuw, int cuh, s16 coef_dst[N_C][MAX_CU_DIM], XEVD_CORE * core)
{
    int i, j, sidx, didx;

    sidx = (x&(ctx->max_cuwh - 1)) + ((y&(ctx->max_cuwh - 1)) << ctx->log2_max_cuwh);
    didx = 0;

    if (xevd_check_luma(ctx, core))
    {
        for (j = 0; j < cuh; j++)
        {
            for (i = 0; i < cuw; i++)
            {
                coef_dst[Y_C][didx++] = coef_src[Y_C][sidx + i];
            }
            sidx += ctx->max_cuwh;
        }

    }
    if (xevd_check_chroma(ctx, core) && ctx->sps->chroma_format_idc)
    {
        x >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        y >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));
        cuw >>= (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc));
        cuh >>= (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc));

        sidx = (x & ((ctx->max_cuwh >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))) - 1)) + ((y & ((ctx->max_cuwh >> (XEVD_GET_CHROMA_H_SHIFT(ctx->sps->chroma_format_idc))) - 1)) << (ctx->log2_max_cuwh - (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc))));

        didx = 0;

        for (j = 0; j < cuh; j++)
        {
            for (i = 0; i < cuw; i++)
            {
                coef_dst[U_C][didx] = coef_src[U_C][sidx + i];
                coef_dst[V_C][didx] = coef_src[V_C][sidx + i];
                didx++;
            }
            sidx += (ctx->max_cuwh >> (XEVD_GET_CHROMA_W_SHIFT(ctx->sps->chroma_format_idc)));

        }
    }
}


static int xevd_recon_unit(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cup, TREE_CONS_NEW tree_cons)
{
    int  cuw, cuh;
    XEVD_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO: Tim for further refactoring

    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    int chroma_format_idc = ctx->sps->chroma_format_idc;

    cu_init(ctx, core, x, y, cuw, cuh);

    core->avail_lr = xevd_check_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);

    xevdm_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->cod_eco, ctx->map_cu_mode, core->ctx_flags, ctx->sh.slice_type, ctx->sps->tool_cm_init
                         , ctx->sps->ibc_flag, ctx->sps->ibc_log_max_size, ctx->map_tidx, 0);

    /* inverse transform and dequantization */
    if(core->pred_mode != MODE_SKIP)
    {
        coef_rect_to_series(ctx, cu_data->coef, x, y, cuw, cuh, core->coef, core);
        xevdm_itdq_main(ctx, core);
    }

    /* prediction */
    if (core->pred_mode == MODE_IBC)
    {
        core->avail_cu = xevdm_get_avail_ibc(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, cuw, cuh, ctx->map_scu, ctx->map_tidx);

        xevdm_IBC_mc(x, y, log2_cuw, log2_cuh, core->mv[0], ctx->pic, core->pred[0], mcore->tree_cons, ctx->sps->chroma_format_idc);
        get_nbr_yuv(x, y, cuw, cuh, ctx, core);
    }
    else if(core->pred_mode != MODE_INTRA)
    {
        core->avail_cu = xevdm_get_avail_inter(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, cuw, cuh, ctx->map_scu, ctx->map_tidx);
        if (ctx->sps->tool_dmvr)
        {
            mcore->dmvr_enable = 0;
            if (core->pred_mode == MODE_SKIP && !mcore->mmvd_flag)
            {
                mcore->dmvr_enable = 1;
            }
            if (core->inter_dir == PRED_DIR)
            {
                mcore->dmvr_enable = 1;
            }
            if (mcore->affine_flag)
            {
                mcore->dmvr_enable = 0;
            }
        }

        if(mcore->affine_flag)
        {
            xevd_get_affine_motion(ctx, core);

            xevdm_affine_mc(x, y, ctx->w, ctx->h, cuw, cuh, core->refi, mcore->affine_mv, ctx->refp, core->pred, mcore->affine_flag + 1, core->eif_tmp_buffer, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8,  ctx->sps->chroma_format_idc);
        }
        else
        {
            if (core->pred_mode == MODE_SKIP)
            {
                xevd_get_skip_motion(ctx, core);
            }
            else
            {
                if (core->inter_dir == PRED_DIR)
                {
                    if(ctx->sps->tool_admvp == 0)
                    {
                        xevdm_get_mv_dir(ctx->refp[0], ctx->poc.poc_val, core->scup + ((1 << (core->log2_cuw - MIN_CU_LOG2)) - 1) + ((1 << (core->log2_cuh - MIN_CU_LOG2)) - 1) * ctx->w_scu, core->scup, ctx->w_scu, ctx->h_scu, core->mv
                            , ctx->sps->tool_admvp
                        );
                        core->refi[REFP_0] = 0;
                        core->refi[REFP_1] = 0;
                    }
                    else if (core->mvr_idx == 0)
                    {
                        xevd_get_direct_motion(ctx, core);
                    }
                }
                else if (core->inter_dir == PRED_DIR_MMVD)
                {
                    xevdm_get_mmvd_motion(ctx, core);
                }
                else
                {
                    xevd_get_inter_motion(ctx, core);
                }
            }
            xevdm_mc(x, y, ctx->w, ctx->h, cuw, cuh, core->refi, core->mv, ctx->refp, core->pred, ctx->poc.poc_val, mcore->dmvr_template, mcore->dmvr_ref_pred_interpolated
                   , mcore->dmvr_half_pred_interpolated, (mcore->dmvr_enable == 1) && ctx->sps->tool_dmvr, mcore->dmvr_padding_buf, &mcore->dmvr_flag, mcore->dmvr_mv
                   , ctx->sps->tool_admvp, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8+8, ctx->sps->chroma_format_idc);
        }

        xevdm_set_dec_info(ctx, core);

        if (core->pred_mode != MODE_INTRA && core->pred_mode != MODE_IBC
             && xevd_check_luma(ctx, core)
            && ctx->sps->tool_hmvp

            )
        {
            update_history_buffer_parse_affine(core, ctx->sh.slice_type);
        }

    }
    else
    {
        core->avail_cu = xevd_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->log2_cuw, core->log2_cuh, ctx->map_scu, ctx->map_tidx);
        get_nbr_yuv(x, y, cuw, cuh, ctx, core);

        if (ctx->sps->tool_eipd)
        {
            if (xevd_check_luma(ctx, core))
            {
                xevdm_ipred(core->nb[0][0] + 2, core->nb[0][1] + cuh, core->nb[0][2] + 2, core->avail_lr, core->pred[0][Y_C], core->ipm[0], cuw, cuh, ctx->sps->bit_depth_luma_minus8 + 8);
            }
            if (xevd_check_chroma(ctx, core))
            {
                xevdm_ipred_uv(core->nb[1][0] + 2, core->nb[1][1] + (cuh >> 1), core->nb[1][2] + 2, core->avail_lr, core->pred[0][U_C], core->ipm[1], core->ipm[0], cuw >> 1, cuh >> 1, ctx->sps->bit_depth_chroma_minus8 + 8);
                xevdm_ipred_uv(core->nb[2][0] + 2, core->nb[2][1] + (cuh >> 1), core->nb[2][2] + 2, core->avail_lr, core->pred[0][V_C], core->ipm[1], core->ipm[0], cuw >> 1, cuh >> 1, ctx->sps->bit_depth_chroma_minus8 + 8);
            }
        }
        else
        {
            if (xevd_check_luma(ctx, core))
            {
                xevd_ipred_b(core->nb[0][0] + 2, core->nb[0][1] + cuh, core->nb[0][2] + 2, core->avail_lr, core->pred[0][Y_C], core->ipm[0], cuw, cuh);
            }
            if (xevd_check_chroma(ctx, core))
            {
                xevd_ipred_uv_b(core->nb[1][0] + 2, core->nb[1][1] + (cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))), core->nb[1][2] + 2, core->avail_lr, core->pred[0][U_C]
                               , core->ipm[1], core->ipm[0], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
                xevd_ipred_uv_b(core->nb[2][0] + 2, core->nb[2][1] + (cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))), core->nb[2][2] + 2, core->avail_lr, core->pred[0][V_C]
                               , core->ipm[1], core->ipm[0], cuw >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc)), cuh >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc)));
            }
        }
    }

    /* reconstruction */
    xevdm_recon_yuv(x, y, cuw, cuh, core->coef, core->pred[0], core->is_coef, ctx->pic, core->pred_mode == MODE_IBC ? 0 : mcore->ats_inter_info, mcore->tree_cons, ctx->sps->bit_depth_luma_minus8 + 8 , ctx->sps->chroma_format_idc);

    if (core->pred_mode != MODE_IBC)
    {
        if (ctx->sps->tool_htdf == 1 && (core->is_coef[Y_C] || core->pred_mode == MODE_INTRA) && xevd_check_luma(ctx, core))
        {
            u16 avail_cu = xevd_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, log2_cuw, log2_cuh, ctx->map_scu, ctx->map_tidx);

            int constrained_intra_flag = core->pred_mode == MODE_INTRA && ctx->pps.constrained_intra_pred_flag;

            xevdm_htdf(ctx->pic->y + (y * ctx->pic->s_l) + x, ctx->sh.qp, cuw, cuh, ctx->pic->s_l, core->pred_mode == MODE_INTRA, ctx->pic->y + (y * ctx->pic->s_l) + x, ctx->pic->s_l, avail_cu, core->scup, ctx->w_scu, ctx->h_scu, ctx->map_scu, constrained_intra_flag, ctx->sps->bit_depth_luma_minus8 + 8);
        }
    }

    u32 *map_scu = ctx->map_scu + core->scup;
    for (int j = 0; j < cuh >> MIN_CU_LOG2; j++)
    {
        for (int i = 0; i < cuw >> MIN_CU_LOG2; i++)
        {
            MCU_SET_COD(map_scu[i]);
        }
        map_scu += ctx->w_scu;
    }

    return XEVD_OK;

}

static void copy_to_cu_data(XEVD_CTX *ctx, XEVD_CORE *core, int cud);

static int xevd_entropy_dec_unit(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int log2_cuw, int log2_cuh, TREE_CONS_NEW tree_cons, int cud)
{
    int ret, cuw, cuh;
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO: Tim for further refactoring
    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;
    core->x = x;
    core->y = y;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    int chroma_format_idc = ctx->sps->chroma_format_idc;

    XEVD_TRACE_COUNTER;
    XEVD_TRACE_STR("poc: ");
    XEVD_TRACE_INT(ctx->poc.poc_val);
    XEVD_TRACE_STR("x pos ");
    XEVD_TRACE_INT(x);
    XEVD_TRACE_STR("y pos ");
    XEVD_TRACE_INT(y);
    XEVD_TRACE_STR("width ");
    XEVD_TRACE_INT(cuw);
    XEVD_TRACE_STR("height ");
    XEVD_TRACE_INT(cuh);
    XEVD_TRACE_STR("\n");

    mcore->ats_intra_cu = mcore->ats_intra_mode_h = mcore->ats_intra_mode_v = 0;
    core->avail_lr = xevd_check_eco_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->cod_eco, ctx->map_tidx);

    xevdm_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->cod_eco, ctx->map_cu_mode, core->ctx_flags, ctx->sh.slice_type, ctx->sps->tool_cm_init
                         , ctx->sps->ibc_flag, ctx->sps->ibc_log_max_size, ctx->map_tidx, 1);

    /* parse CU info */
    ret = xevdm_eco_cu(ctx, core);
    xevd_assert_g(ret == XEVD_OK, ERR);

    copy_to_cu_data(ctx, core, cud);
    xevdm_set_dec_info(ctx, core);
    u8 *cod_eco = ctx->cod_eco + core->scup;
    xevd_mset(cod_eco, 1, (cuw >> MIN_CU_LOG2) * sizeof(u8));
    for (int i = 1; i < cuh >> MIN_CU_LOG2; i++)
    {
        xevd_mcpy(cod_eco + i*ctx->w_scu, cod_eco, (cuw >> MIN_CU_LOG2) * sizeof(u8));
    }

    return XEVD_OK;
ERR:
    return ret;
}

static void copy_to_cu_data(XEVD_CTX *ctx, XEVD_CORE *core, int cud)
{
    XEVD_CU_DATA *cu_data;
    int idx, size;
    int log2_cuw, log2_cuh;
    int x_in_lcu = core->x - (core->x_lcu << ctx->log2_max_cuwh);
    int y_in_lcu = core->y - (core->y_lcu << ctx->log2_max_cuwh);
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    log2_cuw = core->log2_cuw;
    log2_cuh = core->log2_cuh;
    core->cuw = 1 << core->log2_cuw;
    core->cuh = 1 << core->log2_cuh;
    cu_data = &ctx->map_cu_data[core->lcu_num];

    if (xevd_check_luma(ctx, core))
    {
        /* copy coef */
        size = core->cuw * sizeof(s16);
        s16* dst_coef = cu_data->coef[Y_C] + (y_in_lcu << ctx->log2_max_cuwh) + x_in_lcu;
        s16* src_coef = core->coef[Y_C];
        for (int j = 0; j < core->cuh; j++)
        {
            xevd_mcpy(dst_coef, src_coef, size);
            dst_coef += ctx->max_cuwh;
            src_coef += core->cuw;
        }

        /* copy mode info */
        idx = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

        cu_data->pred_mode[idx] = core->pred_mode;
        cu_data->skip_flag[idx] = core->skip_flag;
        cu_data->mmvd_flag[idx] = mcore->mmvd_flag;
        cu_data->nnz[Y_C][idx] = core->is_coef[Y_C];

        for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
        {
            cu_data->nnz_sub[Y_C][sb][idx] = core->is_coef_sub[Y_C][sb];
        }

        cu_data->qp_y[idx] = core->qp_y;


        if (core->pred_mode == MODE_SKIP || core->pred_mode == MODE_DIR)
        {
            cu_data->dmvr_flag[idx] = mcore->dmvr_flag;
        }
        cu_data->depth[idx] = cud;
        cu_data->ats_intra_cu[idx] = mcore->ats_intra_cu;
        cu_data->ats_mode_h[idx] = mcore->ats_intra_mode_h;
        cu_data->ats_mode_v[idx] = mcore->ats_intra_mode_v;
        cu_data->ats_inter_info[idx] = mcore->ats_inter_info;
        cu_data->affine_flag[idx] = mcore->affine_flag;

        cu_data->ibc_flag[idx] = mcore->ibc_flag;

        if (core->pred_mode == MODE_INTRA)
        {
            cu_data->ipm[0][idx] = core->ipm[0];
            cu_data->mv[idx][REFP_0][MV_X] = 0;
            cu_data->mv[idx][REFP_0][MV_Y] = 0;
            cu_data->mv[idx][REFP_1][MV_X] = 0;
            cu_data->mv[idx][REFP_1][MV_Y] = 0;
            cu_data->refi[idx][REFP_0] = -1;
            cu_data->refi[idx][REFP_1] = -1;
        }
        else if (core->pred_mode == MODE_IBC)
        {
            cu_data->refi[idx][REFP_0] = -1;
            cu_data->refi[idx][REFP_1] = -1;
            cu_data->mvp_idx[idx][REFP_0] = core->mvp_idx[REFP_0];
            cu_data->mvp_idx[idx][REFP_1] = 0;
            cu_data->mv[idx][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
            cu_data->mv[idx][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
            cu_data->mv[idx][REFP_1][MV_X] = 0;
            cu_data->mv[idx][REFP_1][MV_Y] = 0;

            cu_data->mvd[idx][REFP_0][MV_X] = core->mvd[REFP_0][MV_X];
            cu_data->mvd[idx][REFP_0][MV_Y] = core->mvd[REFP_0][MV_Y];
        }
        else
        {
            cu_data->refi[idx][REFP_0] = core->refi[REFP_0];
            cu_data->refi[idx][REFP_1] = core->refi[REFP_1];
            cu_data->mvp_idx[idx][REFP_0] = core->mvp_idx[REFP_0];
            cu_data->mvp_idx[idx][REFP_1] = core->mvp_idx[REFP_1];
            cu_data->mvr_idx[idx] = core->mvr_idx;
            cu_data->bi_idx[idx] = core->bi_idx;
            cu_data->inter_dir[idx] = core->inter_dir;
            cu_data->mmvd_idx[idx] = mcore->mmvd_idx;

            if (cu_data->dmvr_flag[idx])
            {
                cu_data->mv[idx][REFP_0][MV_X] = mcore->dmvr_mv[idx][REFP_0][MV_X];
                cu_data->mv[idx][REFP_0][MV_Y] = mcore->dmvr_mv[idx][REFP_0][MV_Y];
                cu_data->mv[idx][REFP_1][MV_X] = mcore->dmvr_mv[idx][REFP_1][MV_X];
                cu_data->mv[idx][REFP_1][MV_Y] = mcore->dmvr_mv[idx][REFP_1][MV_Y];

                cu_data->unrefined_mv[idx][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
                cu_data->unrefined_mv[idx][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
                cu_data->unrefined_mv[idx][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
                cu_data->unrefined_mv[idx][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];
            }
            else
            {
                cu_data->mv[idx][REFP_0][MV_X] = core->mv[REFP_0][MV_X];
                cu_data->mv[idx][REFP_0][MV_Y] = core->mv[REFP_0][MV_Y];
                cu_data->mv[idx][REFP_1][MV_X] = core->mv[REFP_1][MV_X];
                cu_data->mv[idx][REFP_1][MV_Y] = core->mv[REFP_1][MV_Y];
            }

            cu_data->mvd[idx][REFP_0][MV_X] = core->mvd[REFP_0][MV_X];
            cu_data->mvd[idx][REFP_0][MV_Y] = core->mvd[REFP_0][MV_Y];
            cu_data->mvd[idx][REFP_1][MV_X] = core->mvd[REFP_1][MV_X];
            cu_data->mvd[idx][REFP_1][MV_Y] = core->mvd[REFP_1][MV_Y];

            if (mcore->affine_flag)
            {
                cu_data->affine_bzero[idx][REFP_0] = mcore->affine_bzero[REFP_0];
                cu_data->affine_bzero[idx][REFP_1] = mcore->affine_bzero[REFP_1];

                cu_data->affine_mvd[idx][REFP_0][0][MV_X] = mcore->affine_mvd[REFP_0][0][MV_X];
                cu_data->affine_mvd[idx][REFP_0][0][MV_Y] = mcore->affine_mvd[REFP_0][0][MV_Y];
                cu_data->affine_mvd[idx][REFP_0][1][MV_X] = mcore->affine_mvd[REFP_0][1][MV_X];
                cu_data->affine_mvd[idx][REFP_0][1][MV_Y] = mcore->affine_mvd[REFP_0][1][MV_Y];
                cu_data->affine_mvd[idx][REFP_0][2][MV_X] = mcore->affine_mvd[REFP_0][2][MV_X];
                cu_data->affine_mvd[idx][REFP_0][2][MV_Y] = mcore->affine_mvd[REFP_0][2][MV_Y];

                cu_data->affine_mvd[idx][REFP_1][0][MV_X] = mcore->affine_mvd[REFP_1][0][MV_X];
                cu_data->affine_mvd[idx][REFP_1][0][MV_Y] = mcore->affine_mvd[REFP_1][0][MV_Y];
                cu_data->affine_mvd[idx][REFP_1][1][MV_X] = mcore->affine_mvd[REFP_1][1][MV_X];
                cu_data->affine_mvd[idx][REFP_1][1][MV_Y] = mcore->affine_mvd[REFP_1][1][MV_Y];
                cu_data->affine_mvd[idx][REFP_1][2][MV_X] = mcore->affine_mvd[REFP_1][2][MV_X];
                cu_data->affine_mvd[idx][REFP_1][2][MV_Y] = mcore->affine_mvd[REFP_1][2][MV_Y];
            }
        }
    }

    if (mcore->affine_flag)
    {
        xevdm_set_affine_mvf(ctx, core);
    }

    if (xevd_check_chroma(ctx, core))
    {
        size = (core->cuw >> 1) * sizeof(s16);
        for (int c = U_C; c <= V_C; c++)
        {
            s16* dst_coef = cu_data->coef[c] + (y_in_lcu << (ctx->log2_max_cuwh - 2)) + (x_in_lcu >> 1);
            s16* src_coef = core->coef[c];
            for (int j = 0; j < core->cuh >> 1; j++)
            {
                xevd_mcpy(dst_coef, src_coef, size);
                dst_coef += ctx->max_cuwh >> 1;
                src_coef += core->cuw >> 1;
            }
        }

        /* copy mode info */
        idx = (y_in_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2 * 2)) + (x_in_lcu >> MIN_CU_LOG2);

        cu_data->pred_mode_chroma[idx] = core->pred_mode;
        cu_data->nnz[U_C][idx] = core->is_coef[U_C];
        cu_data->nnz[V_C][idx] = core->is_coef[V_C];
        for (int c = U_C; c < N_C; c++)
        {
            for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
            {
                cu_data->nnz_sub[c][sb][idx] = core->is_coef_sub[c][sb];
            }
        }

        cu_data->qp_u[idx] = core->qp_u;
        cu_data->qp_v[idx] = core->qp_v;

        if (core->pred_mode == MODE_INTRA)
        {
            cu_data->ipm[1][idx] = core->ipm[1];
        }
    }
}

static int xevd_entropy_decode_tree(XEVD_CTX * ctx, XEVD_CORE * core, int x0, int y0, int log2_cuw, int log2_cuh, int cup, int cud, XEVD_BSR * bs, XEVD_SBAC * sbac, int next_split
                         , int parent_suco, const int parent_split, int* same_layer_split, const int node_idx, const int* parent_split_allow, int qt_depth, int btt_depth
                         , int cu_qp_delta_code
                         , MODE_CONS mode_cons)
{
    int ret;
    s8  split_mode = NO_SPLIT;
    int cuw, cuh;
    s8  suco_flag = 0;
    int bound;
    int split_mode_child[4] = {NO_SPLIT, NO_SPLIT, NO_SPLIT, NO_SPLIT};
    int split_allow[6];
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    if (cuw > ctx->min_cuwh || cuh > ctx->min_cuwh)
    {
        if(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h)
        {
            if(next_split)
            {
                split_mode = xevdm_eco_split_mode(ctx, bs, sbac, cuw, cuh, parent_split, same_layer_split, node_idx, parent_split_allow, split_allow, qt_depth, btt_depth, x0, y0, mode_cons, core);
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("x pos ");
                XEVD_TRACE_INT(core->x_pel + ((cup % (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("y pos ");
                XEVD_TRACE_INT(core->y_pel + ((cup / (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("width ");
                XEVD_TRACE_INT(cuw);
                XEVD_TRACE_STR("height ");
                XEVD_TRACE_INT(cuh);
                XEVD_TRACE_STR("depth ");
                XEVD_TRACE_INT(cud);
                XEVD_TRACE_STR("split mode ");
                XEVD_TRACE_INT(split_mode);
                XEVD_TRACE_STR("\n");
            }
            else
            {
                split_mode = NO_SPLIT;
            }
        }
        else
        {
            int boundary = 1;
            int boundary_b = boundary && (y0 + cuh > ctx->h) && !(x0 + cuw > ctx->w);
            int boundary_r = boundary && (x0 + cuw > ctx->w) && !(y0 + cuh > ctx->h);

            if (ctx->sps->sps_btt_flag)
            {
                xevdm_check_split_mode(split_allow, log2_cuw, log2_cuh, boundary, boundary_b, boundary_r, ctx->log2_max_cuwh
                    , parent_split, same_layer_split, node_idx, parent_split_allow, qt_depth, btt_depth
                    , x0, y0, ctx->w, ctx->h
                    , NULL, ctx->sps->sps_btt_flag
                    , mode_cons);

                if (split_allow[SPLIT_BI_VER])
                {
                    split_mode = SPLIT_BI_VER;
                }
                else if (split_allow[SPLIT_BI_HOR])
                {
                    split_mode = SPLIT_BI_HOR;
                }
                else
                {
                    assert(0);
                }
            }
            else
            {
                split_mode = xevdm_eco_split_mode(ctx, bs, sbac, cuw, cuh, parent_split, same_layer_split, node_idx, parent_split_allow, split_allow, qt_depth, btt_depth, x0, y0, eAll, core);
                XEVD_TRACE_COUNTER;
                XEVD_TRACE_STR("x pos ");
                XEVD_TRACE_INT(core->x_pel + ((cup % (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("y pos ");
                XEVD_TRACE_INT(core->y_pel + ((cup / (ctx->max_cuwh >> MIN_CU_LOG2) << MIN_CU_LOG2)));
                XEVD_TRACE_STR("width ");
                XEVD_TRACE_INT(cuw);
                XEVD_TRACE_STR("height ");
                XEVD_TRACE_INT(cuh);
                XEVD_TRACE_STR("depth ");
                XEVD_TRACE_INT(cud);
                XEVD_TRACE_STR("split mode ");
                XEVD_TRACE_INT(split_mode);
                XEVD_TRACE_STR("\n");
            }
        }
    }
    else
    {
        split_mode = NO_SPLIT;
    }

    if(ctx->pps.cu_qp_delta_enabled_flag && ctx->sps->dquant_flag)
    {
        if (split_mode == NO_SPLIT && (log2_cuh + log2_cuw >= ctx->pps.cu_qp_delta_area) && cu_qp_delta_code != 2)
        {
            if (log2_cuh == 7 || log2_cuw == 7)
            {
                cu_qp_delta_code = 2;
            }
            else
            {
                cu_qp_delta_code = 1;
            }
            core->cu_qp_delta_is_coded = 0;
        }
        else if ((((split_mode == SPLIT_TRI_VER || split_mode == SPLIT_TRI_HOR) && (log2_cuh + log2_cuw == ctx->pps.cu_qp_delta_area + 1)) ||
            (log2_cuh + log2_cuw == ctx->pps.cu_qp_delta_area && cu_qp_delta_code != 2)))
        {
            cu_qp_delta_code = 2;
            core->cu_qp_delta_is_coded = 0;
        }
    }

    xevd_set_split_mode(split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, core->split_mode);
    same_layer_split[node_idx] = split_mode;

    bound = !(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);

    suco_flag = xevdm_eco_suco_flag(bs, sbac, ctx, core, cuw, cuh, split_mode, bound, ctx->log2_max_cuwh, parent_suco);
    xevdm_set_suco_flag(suco_flag, cud, cup, cuw, cuh, ctx->max_cuwh, mcore->suco_flag);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;

        xevd_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct );

        MODE_CONS mode_cons_for_child = mode_cons;

        BOOL mode_constraint_changed = FALSE;

        if ( ctx->sps->sps_btt_flag && ctx->sps->tool_admvp )       // TODO: Tim create the specific variable for local dual tree ON/OFF
        {
             /* should be updated for 4:2:2 and 4:4:4 */
            mode_constraint_changed = mode_cons == eAll && ctx->sps->chroma_format_idc != 0 && !xevd_is_chroma_split_allowed(cuw, cuh, split_mode);

            if ( mode_constraint_changed )
            {
                if ( ctx->sh.slice_type == SLICE_I || xevdm_get_mode_cons_by_split(split_mode, cuw, cuh) == eOnlyIntra || ctx->sps->chroma_format_idc != 1)
                {
                    mode_cons_for_child = eOnlyIntra;
                }
                else
                {
                    //corresponds to needSignalPredModeConstraintTypeFlag equal to 1 branch in spec
                    core->x_scu = PEL2SCU(x0);
                    core->y_scu = PEL2SCU(y0);

                    xevdm_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->cod_eco, ctx->map_cu_mode, core->ctx_flags, ctx->sh.slice_type, ctx->sps->tool_cm_init
                                         , ctx->sps->ibc_flag, ctx->sps->ibc_log_max_size, ctx->map_tidx, 1);

                    mode_cons_for_child = xevdm_eco_mode_constr(core->bs, core->ctx_flags[CNID_MODE_CONS]);
                }
            }
        }

        int suco_order[SPLIT_MAX_PART_COUNT];
        xevdm_split_get_suco_order(xevd_split_is_vertical(split_mode) ? suco_flag : 0, split_mode, suco_order);
        int curr_part_num;
        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = suco_order[part_num];
            int log2_sub_cuw = split_struct.log_cuw[cur_part_num];
            int log2_sub_cuh = split_struct.log_cuh[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                ret = xevd_entropy_decode_tree(ctx, core, x_pos, y_pos, log2_sub_cuw, log2_sub_cuh, split_struct.cup[cur_part_num], split_struct.cud[cur_part_num], bs, sbac, 1
                                    , suco_flag, split_mode, split_mode_child, part_num, split_allow
                                    , INC_QT_DEPTH(qt_depth, split_mode), INC_BTT_DEPTH(btt_depth, split_mode, bound)
                    , cu_qp_delta_code
                    , mode_cons_for_child
                );
                xevd_assert_g(ret == XEVD_OK, ERR);
            }
            curr_part_num = cur_part_num;
        }

        if ( mode_constraint_changed && mode_cons_for_child == eOnlyIntra )
        {
            TREE_CONS_NEW local_tree_cons = { TREE_C, eOnlyIntra };
             ret = xevd_entropy_dec_unit(ctx, core, x0, y0, log2_cuw, log2_cuh, local_tree_cons, cud);
            xevd_assert_g(ret == XEVD_OK, ERR);
        }
    }
    else
    {
        core->cu_qp_delta_code = cu_qp_delta_code;
        TREE_TYPE tree_type = mode_cons == eOnlyIntra ? TREE_L : TREE_LC;
        assert( mode_cons != eOnlyInter || !( ctx->sps->tool_admvp && log2_cuw == 2 && log2_cuh == 2 ) );

        if (ctx->sh.slice_type == SLICE_I || (ctx->sps->tool_admvp && log2_cuw == 2 && log2_cuh == 2))
        {
            mode_cons = eOnlyIntra;
        }

        ret = xevd_entropy_dec_unit(ctx, core, x0, y0, log2_cuw, log2_cuh, ( TREE_CONS_NEW ) { tree_type, mode_cons }, cud);
        xevd_assert_g(ret == XEVD_OK, ERR);
    }
    return XEVD_OK;
ERR:
    return ret;
}

static int xevd_recon_tree(XEVD_CTX * ctx, XEVD_CORE * core, int x, int y, int cuw, int cuh, int cud, int cup, TREE_CONS_NEW tree_cons)
{
    s8  split_mode;
    int lcu_num;
    s8  suco_flag = 0;
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons };
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    lcu_num = core->lcu_num; //(x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xevd_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, &ctx->map_split[lcu_num]);
    xevdm_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, ctx->max_cuwh, &mctx->map_suco[lcu_num]);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;
        int suco_order[SPLIT_MAX_PART_COUNT];
        xevd_split_get_part_structure(split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct);

        TREE_CONS_NEW tree_constrain_for_child = tree_cons;
        BOOL mode_cons_changed = FALSE;

        if ( ctx->sps->tool_admvp && ctx->sps->sps_btt_flag )       // TODO: Tim create the specific variable for local dual tree ON/OFF
        {
            mode_cons_changed = tree_cons.mode_cons == eAll && !xevd_is_chroma_split_allowed( cuw, cuh, split_mode );

            if (mode_cons_changed)
            {
                tree_constrain_for_child.mode_cons = xevd_derive_mode_cons(ctx, PEL2SCU(x) + PEL2SCU(y) * ctx->w_scu);
                tree_constrain_for_child.tree_type = tree_constrain_for_child.mode_cons == eOnlyIntra ? TREE_L : TREE_LC;
            }
        }
        else
        {
            // In base profile we have small chroma blocks
            tree_constrain_for_child = (TREE_CONS_NEW) { TREE_LC, eAll };
            mode_cons_changed = FALSE;
        }

        xevdm_split_get_suco_order(xevd_split_is_vertical(split_mode) ? suco_flag : 0, split_mode, suco_order);
        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = suco_order[part_num];
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if (x_pos < ctx->w && y_pos < ctx->h)
            {
                xevd_recon_tree(ctx, core, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num]
                              , split_struct.cup[cur_part_num], tree_constrain_for_child);
            }
        }

        mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO:Tim could it be removed? tree_constrain_for_child?

        if ( mode_cons_changed && tree_constrain_for_child.mode_cons == eOnlyIntra )
        {
            tree_cons.mode_cons = eOnlyIntra;
            tree_cons.tree_type = TREE_C;
            split_mode = NO_SPLIT;
        }
    }

    if (split_mode == NO_SPLIT)
    {
        TREE_TYPE tree_type = tree_cons.mode_cons == eOnlyIntra ? (tree_cons.tree_type == TREE_C ? TREE_C : TREE_L) : TREE_LC;
        assert(tree_cons.mode_cons != eOnlyInter || !(ctx->sps->tool_admvp && XEVD_CONV_LOG2(cuw) == 2 && XEVD_CONV_LOG2(cuh) == 2));

        if (ctx->sh.slice_type == SLICE_I || (ctx->sps->tool_admvp && XEVD_CONV_LOG2(cuw) == 2 && XEVD_CONV_LOG2(cuh) == 2))
        {
            tree_cons.mode_cons = eOnlyIntra;
        }
        xevd_recon_unit(ctx, core, x, y, XEVD_CONV_LOG2(cuw), XEVD_CONV_LOG2(cuh), cup, tree_cons);
    }

    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO:Tim further refactor //TODO:Tim could it be removed? tree_constrain_for_child?

    return XEVD_OK;
}

static void deblock_tree(XEVD_CTX * ctx, XEVD_PIC * pic, int x, int y, int cuw, int cuh, int cud, int cup, int is_hor_edge
                       , TREE_CONS_NEW tree_cons , XEVD_CORE * core, int boundary_filtering)
{
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    s8  split_mode;
    int lcu_num;
    s8  suco_flag = 0;
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons };
    lcu_num = (x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xevd_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, &ctx->map_split[lcu_num]);
    xevdm_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, ctx->max_cuwh, &mctx->map_suco[lcu_num]);

    if(split_mode != NO_SPLIT)
    {
        XEVD_SPLIT_STRUCT split_struct;
        int suco_order[SPLIT_MAX_PART_COUNT];
        xevd_split_get_part_structure(split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_max_cuwh - MIN_CU_LOG2, &split_struct);

        TREE_CONS_NEW tree_constrain_for_child = tree_cons;

        BOOL mode_cons_changed = FALSE;

        if ( ctx->sps->tool_admvp && ctx->sps->sps_btt_flag )       // TODO: Tim create the specific variable for local dual tree ON/OFF
        {
            /* should be updated for 4:2:2 and 4:4:4 */
            mode_cons_changed = tree_cons.mode_cons == eAll && ctx->sps->chroma_format_idc != 0 && !xevd_is_chroma_split_allowed(cuw, cuh, split_mode);
            if (mode_cons_changed)
            {
                tree_constrain_for_child.mode_cons = xevd_derive_mode_cons(ctx, PEL2SCU(x) + PEL2SCU(y) * ctx->w_scu);
                tree_constrain_for_child.tree_type = tree_constrain_for_child.mode_cons == eOnlyIntra ? TREE_L : TREE_LC;
            }
        }
        else
        {
            // In base profile we have small chroma blocks
            tree_constrain_for_child = (TREE_CONS_NEW) { TREE_LC, eAll };
            mode_cons_changed = FALSE;
        }

        xevdm_split_get_suco_order(xevd_split_is_vertical(split_mode) ? suco_flag : 0, split_mode, suco_order);
        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = suco_order[part_num];
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                deblock_tree(ctx, pic, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num], split_struct.cup[cur_part_num], is_hor_edge
                            ,tree_constrain_for_child, core, boundary_filtering);
            }
        }

        mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO:Tim could it be removed? tree_constrain_for_child?

        if ( mode_cons_changed && tree_constrain_for_child.mode_cons == eOnlyIntra )
        {
            mcore->tree_cons.mode_cons = eOnlyIntra;
            mcore->tree_cons.tree_type = TREE_C;
            split_mode = NO_SPLIT;
        }
    }

    if (split_mode == NO_SPLIT)
    {
        // deblock

        if(is_hor_edge)
        {
            if (cuh > MAX_TR_SIZE)
            {
                xevdm_deblock_cu_hor(ctx, pic, x, y, cuw, cuh >> 1, ctx->map_scu, ctx->map_refi, mctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0
                                   , mcore->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps->tool_addb, mctx->map_ats_inter
                                  , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8, ctx->sps->chroma_format_idc);

                xevdm_deblock_cu_hor(ctx, pic, x, y + MAX_TR_SIZE, cuw, cuh >> 1, ctx->map_scu, ctx->map_refi,mctx->map_unrefined_mv
                                   , ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0, mcore->tree_cons, ctx->map_tidx, boundary_filtering
                                   , ctx->sps->tool_addb, mctx->map_ats_inter, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8
                                   , ctx->sps->chroma_format_idc);
            }
            else
            {
                xevdm_deblock_cu_hor(ctx, pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi,mctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0
                                   , mcore->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps->tool_addb, mctx->map_ats_inter
                                   , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8, ctx->sps->chroma_format_idc);
            }
        }
        else
        {
            if (cuw > MAX_TR_SIZE)
            {
                xevdm_deblock_cu_ver(ctx, pic, x, y, cuw >> 1, cuh, ctx->map_scu, ctx->map_refi, mctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                                   , ctx->map_cu_mode, ctx->refp, 0, mcore->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps->tool_addb
                                   , mctx->map_ats_inter, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8 , ctx->sps->chroma_format_idc);

                xevdm_deblock_cu_ver(ctx, pic, x + MAX_TR_SIZE, y, cuw >> 1, cuh, ctx->map_scu, ctx->map_refi, mctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                                   , ctx->map_cu_mode, ctx->refp, 0, mcore->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps->tool_addb, mctx->map_ats_inter
                                   , ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8, ctx->sps->chroma_format_idc);
            }
            else
            {
                xevdm_deblock_cu_ver(ctx, pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, mctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                                   , ctx->map_cu_mode, ctx->refp, 0, mcore->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps->tool_addb
                                   , mctx->map_ats_inter, ctx->sps->bit_depth_luma_minus8 + 8, ctx->sps->bit_depth_chroma_minus8 + 8 , ctx->sps->chroma_format_idc);
            }
        }
    }
    mcore->tree_cons = ( TREE_CONS ) { FALSE, tree_cons.tree_type, tree_cons.mode_cons }; //TODO:Tim further refactor //TODO:Tim could it be removed? tree_constrain_for_child?
}

int xevdm_deblock(void * arg)
{
    xevd_assert(arg != NULL);
    XEVD_CORE  * core = (XEVD_CORE *)arg;
    XEVD_CTX   * ctx = core->ctx;
    int          tile_idx;
    int          i, j;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    tile_idx = core->tile_num;
    ctx->pic->pic_deblock_alpha_offset = ctx->sh.sh_deblock_alpha_offset;
    ctx->pic->pic_deblock_beta_offset = ctx->sh.sh_deblock_beta_offset;
    ctx->pic->pic_qp_u_offset = ctx->sh.qp_u_offset;
    ctx->pic->pic_qp_v_offset = ctx->sh.qp_v_offset;

    int x_l, x_r, y_l, y_r, l_scu, r_scu, t_scu, b_scu;
    u32 k1;
    int scu_in_lcu_wh = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);

    x_l = (ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    y_l = (ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    x_r = x_l + ctx->tile[tile_idx].w_ctb;
    y_r = y_l + ctx->tile[tile_idx].h_ctb;
    l_scu = x_l * scu_in_lcu_wh;
    r_scu = XEVD_CLIP3(0, ctx->w_scu, x_r*scu_in_lcu_wh);
    t_scu = y_l * scu_in_lcu_wh;
    b_scu = XEVD_CLIP3(0, ctx->h_scu, y_r*scu_in_lcu_wh);

    for (j = t_scu; j < b_scu; j++)
    {
        for (i = l_scu; i < r_scu; i++)
        {
            k1 = i + j * ctx->w_scu;
            MCU_CLR_COD(ctx->map_scu[k1]);

            if (!MCU_GET_DMVRF(ctx->map_scu[k1]))
            {
                mctx->map_unrefined_mv[k1][REFP_0][MV_X] = ctx->map_mv[k1][REFP_0][MV_X];
                mctx->map_unrefined_mv[k1][REFP_0][MV_Y] = ctx->map_mv[k1][REFP_0][MV_Y];
                mctx->map_unrefined_mv[k1][REFP_1][MV_X] = ctx->map_mv[k1][REFP_1][MV_X];
                mctx->map_unrefined_mv[k1][REFP_1][MV_Y] = ctx->map_mv[k1][REFP_1][MV_Y];
            }
        }
    }

    /* horizontal filtering */
    for (j = y_l; j < y_r; j++)
    {
        for (i = x_l; i < x_r; i++)
        {
            deblock_tree(ctx, ctx->pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, core->deblock_is_hor
                       , (TREE_CONS_NEW) { TREE_LC, eAll }, core, ctx->pps.loop_filter_across_tiles_enabled_flag);
        }
    }

    return XEVD_OK;
}

int xevd_alf(XEVD_CTX * ctx, XEVD_PIC * pic)
{
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    ADAPTIVE_LOOP_FILTER* p = (ADAPTIVE_LOOP_FILTER*)(mctx->alf);
    int ret = call_dec_alf_process_aps(p, ctx, pic);
    return ret;
}

static void update_core_loc_param(XEVD_CTX * ctx, XEVD_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set y_scu location
    core->lcu_num = core->x_lcu + core->y_lcu*ctx->w_lcu; // Init the first lcu_num in tile
}

/* updating core location parameters for CTU parallel encoding case*/
static void update_core_loc_param1(XEVD_CTX * ctx, XEVD_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (ctx->log2_max_cuwh - MIN_CU_LOG2); // set y_scu location
}

static int mt_get_next_ctu_num(XEVD_CTX * ctx, XEVD_CORE * core, int skip_ctb_line_cnt)
{
    int sp_x_lcu = ctx->tile[core->tile_num].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[core->tile_num].ctba_rs_first / ctx->w_lcu;
    //core->lcu_num++;
    core->x_lcu++;
    if (core->x_lcu == sp_x_lcu + ctx->tile[core->tile_num].w_ctb)
    {
        core->x_lcu = sp_x_lcu;
        core->y_lcu += skip_ctb_line_cnt;
    }

    core->lcu_num = core->y_lcu * ctx->w_lcu + core->x_lcu;
    /* check to exceed height of ctb line */
    if (core->y_lcu >= sp_y_lcu + ctx->tile[core->tile_num].h_ctb)
    {
        return -1;
    }
    update_core_loc_param1(ctx, core);

    return core->lcu_num;
}

static int set_active_pps_info(XEVD_CTX * ctx)
{
    int active_pps_id = ctx->sh.slice_pic_parameter_set_id;
    xevd_mcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(XEVD_PPS) );

    return XEVD_OK;
}

static int set_tile_info(XEVD_CTX * ctx, XEVD_CORE *core, XEVD_PPS *pps)
{
    XEVD_TILE   * tile;
    int          i, j, size, x, y, w, h, w_tile, h_tile, w_lcu, h_lcu, tidx, t0;
    int          col_w[MAX_NUM_TILES_COL], row_h[MAX_NUM_TILES_ROW];
    u8         * map_tidx;
    XEVD_SH       * sh;
    u32         *  map_scu;
    //Below variable need to be handeled separately in multicore environment
    int slice_num = 0;

    sh = &(ctx->sh);
    ctx->tile_cnt = (pps->num_tile_rows_minus1 + 1) * (pps->num_tile_columns_minus1 + 1);
    ctx->w_tile = pps->num_tile_columns_minus1 + 1;
    ctx->h_tile = pps->num_tile_rows_minus1 + 1;
    w_tile = ctx->w_tile;
    h_tile = ctx->h_tile;
    w_lcu = ctx->w_lcu;
    h_lcu = ctx->h_lcu;

    xevd_mset(ctx->tile_order_slice, 0, sizeof(u16) * MAX_NUM_TILES_COL*MAX_NUM_TILES_ROW);

    if (!sh->arbitrary_slice_flag)
    {
        int first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
        int w_tile_slice, h_tile_slice;
        int tmp1, tmp2, i=0;

        first_tile_col_idx = sh->first_tile_id % w_tile;
        last_tile_col_idx = sh->last_tile_id % w_tile;
        delta_tile_idx = sh->last_tile_id - sh->first_tile_id;

        if (sh->last_tile_id < sh->first_tile_id)
        {
            if (first_tile_col_idx > last_tile_col_idx)
            {
                delta_tile_idx += ctx->tile_cnt + w_tile;
            }
            else
            {
                delta_tile_idx += ctx->tile_cnt;
            }
        }
        else if (first_tile_col_idx > last_tile_col_idx)
        {
            delta_tile_idx += w_tile;
        }

        w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
        h_tile_slice = (delta_tile_idx / w_tile) + 1; //Number of tiles in slice height
        ctx->num_tiles_in_slice = w_tile_slice * h_tile_slice;

        int st_row_slice = sh->first_tile_id / w_tile;
        int st_col_slice = sh->first_tile_id % w_tile;

        for (tmp1 = 0; tmp1 < h_tile_slice; tmp1++)
        {
            for (tmp2 = 0; tmp2 < w_tile_slice; tmp2++)
            {
                int curr_col_slice = (st_col_slice + tmp2) % w_tile;
                int curr_row_slice = (st_row_slice + tmp1) % h_tile;
                ctx->tile_in_slice[i] = curr_row_slice * w_tile + curr_col_slice;
                ctx->tile_order_slice[curr_row_slice * w_tile + curr_col_slice] = i++;
            }
        }
    }
    else
    {
        ctx->tile_in_slice[0] = sh->first_tile_id;
        ctx->num_tiles_in_slice = sh->num_remaining_tiles_in_slice_minus1 + 2;
        for (i = 1; i <= (ctx->num_tiles_in_slice -1); i++)
        {
            ctx->tile_in_slice[i] = sh->delta_tile_id_minus1[i - 1] + ctx->tile_in_slice[i - 1] + 1;
            ctx->tile_order_slice[sh->delta_tile_id_minus1[i - 1] + ctx->tile_in_slice[i - 1] + 1] = i;
        }
    }

    /* alloc tile information */
    if (slice_num == 0)
    {
        size = sizeof(XEVD_TILE) * ctx->tile_cnt;
        xevd_mset(ctx->tile, 0, size);
    }

    /* set tile information */
    if (pps->uniform_tile_spacing_flag)
    {
        for (i = 0; i<w_tile; i++)
        {
            col_w[i] = ((i + 1) * w_lcu) / w_tile - (i * w_lcu) / w_tile;
        }
        for (j = 0; j<h_tile; j++)
        {
            row_h[j] = ((j + 1) * h_lcu) / h_tile - (j * h_lcu) / h_tile;
        }
    }
    else
    {
        for (i = 0, t0 = 0; i<(w_tile - 1); i++)
        {
            col_w[i] = pps->tile_column_width_minus1[i] + 1;
            t0 += col_w[i];
        }
        col_w[i] = w_lcu - t0;

        for (i = 0, t0 = 0; i<(h_tile - 1); i++)
        {
            row_h[i] = pps->tile_row_height_minus1[i] + 1;
            t0 += row_h[i];
        }
        row_h[i] = h_lcu - t0;
    }

    /* update tile information */
    tidx = 0;
    for (y = 0; y<h_tile; y++)
    {
        for (x = 0; x<w_tile; x++)
        {
            tile = &ctx->tile[tidx];
            tile->w_ctb = col_w[x];
            tile->h_ctb = row_h[y];
            tile->f_ctb = tile->w_ctb * tile->h_ctb;
            tile->ctba_rs_first = 0;
            for (i = 0; i<x; i++)
            {
                tile->ctba_rs_first += col_w[i];
            }
            for (j = 0; j<y; j++)
            {
                tile->ctba_rs_first += w_lcu * row_h[j];
            }
            tidx++;
        }
    }

    /* set tile indices */
    for (tidx = 0; tidx<(w_tile * h_tile); tidx++)
    {
        tile = ctx->tile + tidx;

        x = PEL2SCU((tile->ctba_rs_first % w_lcu) << ctx->log2_max_cuwh);
        y = PEL2SCU((tile->ctba_rs_first / w_lcu) << ctx->log2_max_cuwh);
        t0 = PEL2SCU(tile->w_ctb << ctx->log2_max_cuwh);
        w = XEVD_MIN((ctx->w_scu - x), t0);
        t0 = PEL2SCU(tile->h_ctb << ctx->log2_max_cuwh);
        h = XEVD_MIN((ctx->h_scu - y), t0);

        map_tidx = ctx->map_tidx + x + y * ctx->w_scu;
        map_scu = ctx->map_scu + x + y * ctx->w_scu;
        for (j = 0; j<h; j++)
        {
            for (i = 0; i<w; i++)
            {
                map_tidx[i] = tidx;
                MCU_SET_SN(map_scu[i], slice_num);  //Mapping CUs to the slices
            }
            map_tidx += ctx->w_scu;
            map_scu += ctx->w_scu;
        }
        if (((tidx+1) % ctx->num_tiles_in_slice)==0)
            slice_num++;
    }

    return XEVD_OK;
}

void clear_tile_cod_map(XEVD_CTX  * ctx, XEVD_CORE * core)
{
    core->x_lcu = (ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu; // entry point lcu's x location
    core->y_lcu = (ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    update_core_loc_param(ctx, core);

    u8 * cod_eco = ctx->cod_eco + core->y_scu * ctx->w_scu + core->x_scu;
    int ex, ey;

    if (core->x_scu + (ctx->tile[core->tile_num].w_ctb << (ctx->log2_max_cuwh - MIN_CU_LOG2)) < ctx->w_scu)
    {
        ex = ctx->tile[core->tile_num].w_ctb << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    }
    else
    {
        ex = ctx->w_scu - core->x_scu;
    }

    if (core->y_scu + (ctx->tile[core->tile_num].h_ctb << (ctx->log2_max_cuwh - MIN_CU_LOG2)) < ctx->h_scu)
    {
        ey = ctx->tile[core->tile_num].h_ctb << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    }
    else
    {
        ey = ctx->h_scu - core->y_scu;
    }

    for (int sy = 0; sy < ey; sy++)
    {
        xevd_mset(cod_eco, 0, ex * sizeof(u8));
        cod_eco += ctx->w_scu;
    }
}

int xevd_tile_eco(void * arg)
{
    XEVD_CORE * core = (XEVD_CORE *)arg;
    XEVD_CTX  * ctx  = core->ctx;
    XEVD_BSR   * bs   = core->bs;
    XEVD_SBAC * sbac = core->sbac;
    XEVD_TILE  * tile;
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    int         ret;
    int         col_bd = 0;
    int         lcu_cnt_in_tile = 0;
    int         tile_idx;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    xevd_assert(arg != NULL);

    tile_idx = core->tile_num;
    col_bd = 0;
    if (tile_idx % (ctx->pps.num_tile_columns_minus1 + 1))
    {
        int temp = tile_idx - 1;
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

    xevdm_eco_sbac_reset(bs, ctx->sh.slice_type, ctx->sh.qp, ctx->sps->tool_cm_init);
    tile = &(ctx->tile[tile_idx]);
    core->x_lcu = (ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    core->y_lcu = (ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    lcu_cnt_in_tile = ctx->tile[tile_idx].f_ctb; //Total LCUs in the current tile
    update_core_loc_param(ctx, core);

    while (1) //LCU entropy decoding in a tile
    {
        int same_layer_split[4];
        int split_allow[6] = { 0, 0, 0, 0, 0, 1 };
        xevd_assert_rv(core->lcu_num < ctx->f_lcu, XEVD_ERR_UNEXPECTED);

        core->split_mode = &ctx->map_split[core->lcu_num];
        mcore->suco_flag = &mctx->map_suco[core->lcu_num];

        XEVD_ALF_SLICE_PARAM* alf_slice_param = &(mctx->sh.alf_sh_param);
        if ((alf_slice_param->is_ctb_alf_on) && (mctx->sh.alf_on))
        {
            XEVD_TRACE_COUNTER;
            XEVD_TRACE_STR("Usage of ALF: ");
            *(alf_slice_param->alf_ctu_enable_flag + core->lcu_num) = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.alf_ctb_flag);
            XEVD_TRACE_INT((int)(*(alf_slice_param->alf_ctu_enable_flag + core->lcu_num)));
            XEVD_TRACE_STR("\n");
        }
        if ((mctx->sh.alf_chroma_map_signalled) && (mctx->sh.alf_on))
        {
            *(alf_slice_param->alf_ctu_enable_flag_chroma + core->lcu_num) = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.alf_ctb_flag);
        }
        if ((mctx->sh.alf_chroma2_map_signalled) && (mctx->sh.alf_on))
        {
            *(alf_slice_param->alf_ctu_enable_flag_chroma2 + core->lcu_num) = xevd_sbac_decode_bin(bs, sbac, sbac->ctx.alf_ctb_flag);
        }
        //Recursion to do entropy decoding for the entire CTU
        ret = xevd_entropy_decode_tree(ctx, core, core->x_pel, core->y_pel, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, 0, bs, sbac, 1
            , 0, NO_SPLIT, same_layer_split, 0, split_allow, 0, 0, 0, eAll);
        xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

        lcu_cnt_in_tile--;
        if (lcu_cnt_in_tile == 0)
        {
            xevd_threadsafe_assign(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
            xevd_assert_gv(xevd_eco_tile_end_flag(bs, sbac) == 1, ret, XEVD_ERR, ERR);
            /*Decode zero bits after processing of last tile in slice*/
            if (core->tile_num == ctx->num_tiles_in_slice - 1)
            {
                ret = xevd_eco_cabac_zero_word(bs);
                xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);
            }
            break;
        }
        core->x_lcu++;
        if (core->x_lcu >= ctx->tile[tile_idx].w_ctb + col_bd)
        {
            xevd_threadsafe_assign(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
            core->x_lcu = (tile->ctba_rs_first) % ctx->w_lcu;
            core->y_lcu++;
        }
        update_core_loc_param(ctx, core);
    }

    clear_tile_cod_map(ctx, core);

    return XEVD_OK;
ERR:
    return ret;
}

int xevd_ctu_row_rec_mt(void * arg)
{
    XEVD_CORE * core = (XEVD_CORE *)arg;
    XEVD_CTX  * ctx = core->ctx;
    XEVD_TILE  * tile;
    XEVDM_CORE *mcore = (XEVDM_CORE *)core;
    int         ret;
    int         lcu_cnt_in_tile = 0;
    int         tile_idx;

    xevd_assert(arg != NULL);
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    tile_idx = core->tile_num;
    tile = &(ctx->tile[tile_idx]);
    lcu_cnt_in_tile = ctx->tile[tile_idx].f_ctb; //Total LCUs in the current tile
    update_core_loc_param1(ctx, core);

    int sp_x_lcu = ctx->tile[core->tile_num].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[core->tile_num].ctba_rs_first / ctx->w_lcu;

    //LCU decoding with in a tile
    while (ctx->tile[tile_idx].f_ctb > 0)
    {
        if (ctx->num_tiles_in_slice == 1 && ctx->tc.max_task_cnt > 2)
        {
            xevd_spinlock_wait(&ctx->sync_row[core->y_lcu], THREAD_TERMINATED);
        }
        if (core->y_lcu != sp_y_lcu && core->x_lcu < (sp_x_lcu + ctx->tile[tile_idx].w_ctb - 1))
        {
            /* up-right CTB */
            xevd_spinlock_wait(&ctx->sync_flag[core->lcu_num - ctx->w_lcu + 1], THREAD_TERMINATED);
        }

        xevd_assert_rv(core->lcu_num < ctx->f_lcu, XEVD_ERR_UNEXPECTED);

        if (ctx->sps->tool_hmvp && core->x_lcu == sp_x_lcu) //This condition will reset history buffer
        {
            ret = xevdm_hmvp_init(core);
            xevd_assert_rv(ret == XEVD_OK, ret);
        }

        ret = xevd_recon_tree(ctx, core, (core->x_lcu << ctx->log2_max_cuwh), (core->y_lcu << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh
                            , 0, 0, (TREE_CONS_NEW) { TREE_LC, eAll });
        xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

        xevd_threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
        xevd_threadsafe_decrement(ctx->sync_block, (volatile s32 *)&ctx->tile[tile_idx].f_ctb);

        if (ctx->tc.max_task_cnt > 2 && ctx->num_tiles_in_slice == 1)
        {
            core->lcu_num = mt_get_next_ctu_num(ctx, core, ctx->tc.task_num_in_tile[0] - 1);
        }
        else
        {
        core->lcu_num = mt_get_next_ctu_num(ctx, core, ctx->tc.task_num_in_tile[core->tile_num]);
        }
        if (core->lcu_num == -1)
        {
            break;
        }
    }
    return XEVD_OK;
ERR:
    return ret;
}

int xevd_tile_mt(void * arg)
{
    XEVD_CORE  * core = (XEVD_CORE *)arg;
    XEVD_CTX   * ctx = core->ctx;
    int          res, ret = XEVD_OK;
    int          thread_idx = core->thread_idx + ctx->tc.tile_task_num;
    xevd_mset((void *)ctx->sync_row, 0, ctx->tile[core->tile_num].h_ctb * sizeof(ctx->sync_row[0]));
    if (ctx->tc.max_task_cnt > 2 && ctx->num_tiles_in_slice == 1)
    {
        for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
        {
            if (thread_cnt < ctx->tile[0].h_ctb)
            {
                xevd_mcpy(ctx->core_mt[thread_idx], core, sizeof(XEVD_CORE));
                ctx->core_mt[thread_idx]->x_lcu = ((ctx->tile[0].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
                ctx->core_mt[thread_idx]->y_lcu = ((ctx->tile[0].ctba_rs_first) / ctx->w_lcu) + thread_cnt - 1; // entry point lcu's y location //MULTICORE_ENT_RECON
                ctx->core_mt[thread_idx]->lcu_num = ctx->core_mt[thread_idx]->y_lcu * ctx->w_lcu + ctx->core_mt[thread_idx]->x_lcu;
                ctx->core_mt[thread_idx]->thread_idx = thread_idx;
                ctx->tc.run(ctx->thread_pool[thread_idx], xevd_ctu_row_rec_mt, (void*)ctx->core_mt[thread_idx]);
            }
            thread_idx += ctx->tc.tile_task_num;
        }
        ret = xevd_tile_eco(arg);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        thread_idx = core->thread_idx + ctx->tc.tile_task_num;
        for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[0]; thread_cnt++)
        {
            if (thread_cnt < ctx->tile[0].h_ctb)
            {
                ctx->tc.join(ctx->thread_pool[thread_idx], &res);
                if (XEVD_FAILED(res))
                {
                    ret = res;
                }
            }
            thread_idx += ctx->tc.tile_task_num;
        }
    }
    else
    {

    xevd_tile_eco(arg);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

    for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[core->tile_num]; thread_cnt++)
    {
        if (thread_cnt < ctx->tile[core->tile_num].h_ctb)
        {
            xevd_mcpy(ctx->core_mt[thread_idx], core, sizeof(XEVD_CORE));
            ctx->core_mt[thread_idx]->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
            ctx->core_mt[thread_idx]->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) + thread_cnt; // entry point lcu's y location
            ctx->core_mt[thread_idx]->lcu_num = ctx->core_mt[thread_idx]->y_lcu * ctx->w_lcu + ctx->core_mt[thread_idx]->x_lcu;
            ctx->core_mt[thread_idx]->thread_idx = thread_idx;
            ctx->tc.run(ctx->thread_pool[thread_idx], xevd_ctu_row_rec_mt, (void*)ctx->core_mt[thread_idx]);
        }
        thread_idx += ctx->tc.tile_task_num;
    }

    core->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);
    core->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu);
    xevd_ctu_row_rec_mt(arg);

    thread_idx = core->thread_idx + ctx->tc.tile_task_num;
    for (int thread_cnt = 1; thread_cnt < ctx->tc.task_num_in_tile[core->tile_num]; thread_cnt++)
    {
        if (thread_cnt < ctx->tile[core->tile_num].h_ctb)
        {
            ctx->tc.join(ctx->thread_pool[thread_idx], &res);
            if (XEVD_FAILED(res))
            {
                ret = res;
            }
        }
        thread_idx += ctx->tc.tile_task_num;
    }
    }
    return ret;
}

int xevdm_dec_slice(XEVD_CTX * ctx, XEVD_CORE * core)
{
    XEVD_BSR   * bs;
    XEVD_SBAC  * sbac;
    XEVD_TILE  * tile;
    XEVD_CORE  * core_mt;
    XEVD_BSR     bs_temp;
    XEVD_SBAC    sbac_temp;

    int         ret;
    int         res = 0;
    int         lcu_cnt_in_tile = 0;
    int         col_bd = 0;

    bs   = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);
    xevd_mcpy(&bs_temp, bs, sizeof(XEVD_BSR));
    xevd_mcpy(&sbac_temp, sbac, sizeof(XEVD_SBAC));

    ctx->sh.qp_prev_eco = ctx->sh.qp;

    int tile_idx;
    int thread_idx;
    int num_tiles_in_slice = ctx->num_tiles_in_slice;
    int tile_start_num, num_tiles_proc;
    int tile_cnt = 0;

    tile_start_num = 0;

    while (num_tiles_in_slice)
    {
        num_tiles_proc = num_tiles_in_slice > ctx->tc.tile_task_num ? ctx->tc.tile_task_num : num_tiles_in_slice;
        core_mt = ctx->core_mt[num_tiles_proc - 1];

        for (thread_idx = num_tiles_proc - 1; thread_idx >= 0; thread_idx--)
        {
            tile_idx = ctx->tile_in_slice[tile_start_num + thread_idx];
            xevd_mcpy(ctx->core_mt[thread_idx], core, sizeof(XEVD_CORE));
            core_mt = ctx->core_mt[thread_idx];

            core_mt->ctx = ctx;
            core_mt->bs = &core_mt->ctx->bs_mt[thread_idx];
            core_mt->sbac = &core_mt->ctx->sbac_dec_mt[thread_idx];
            core_mt->tile_num = tile_idx;
            core_mt->thread_idx = thread_idx;

            ctx->tile[tile_idx].qp_prev_eco = ctx->sh.qp;
            ctx->tile[tile_idx].qp = ctx->sh.qp;

            xevd_mcpy(core_mt->bs, &bs_temp, sizeof(XEVD_BSR));
            xevd_mcpy(core_mt->sbac, &sbac_temp, sizeof(XEVD_SBAC));
            SET_SBAC_DEC(core_mt->bs, core_mt->sbac);
            tile_cnt = ctx->tile_order_slice[tile_idx];
            if (tile_cnt != 0)
            {
                int offset = 0;
                for (int i = 0; i < tile_cnt; i++)
                {
                    offset += ctx->sh.entry_point_offset_minus1[i] + 1;
                }
                int seek = offset - (core_mt->bs->leftbits >> 3);
                int leftbits = seek - (seek / 4) * 4;
                core_mt->bs->cur = core_mt->bs->cur + (seek / 4) * 4;          //bs changed according to marker in bs
                core_mt->bs->leftbits = 0;
                if (leftbits)
                {
                    u32  dummy;
                    xevd_bsr_read(core_mt->bs, &dummy, leftbits << 3);
                }
            }

            if (thread_idx > 0)
            {
                ret = ctx->tc.run(ctx->thread_pool[thread_idx], xevd_tile_mt, (void *)core_mt);
                xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);
            }
        }

        ret = xevd_tile_mt((void *)core_mt);
        xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);

        for (thread_idx = 1; thread_idx < num_tiles_proc; thread_idx++)
        {
            ret = ctx->tc.join(ctx->thread_pool[thread_idx], &res);
            xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);
        }

        for (tile_idx = tile_start_num; tile_idx < tile_start_num + num_tiles_proc; tile_idx++)
        {
            tile = &(ctx->tile[ctx->tile_in_slice[tile_idx]]);
            ctx->num_ctb -= (tile->w_ctb * tile->h_ctb);
        }
        num_tiles_in_slice -= (num_tiles_proc);
        tile_start_num += num_tiles_proc;
    }

    if (ctx->num_tiles_in_slice > 1)
    {
        xevd_mcpy(&ctx->bs, ctx->core_mt[ctx->tc.max_task_cnt-1]->bs, sizeof(XEVD_BSR));
        xevd_mcpy(&ctx->sbac_dec, ctx->core_mt[ctx->tc.max_task_cnt-1]->sbac, sizeof(XEVD_SBAC));
    }
    else if (!(num_tiles_in_slice == 1 && ctx->tc.max_task_cnt > 1))
    {
        xevd_mcpy(&ctx->bs, ctx->core_mt[0]->bs, sizeof(XEVD_BSR));
        xevd_mcpy(&ctx->sbac_dec, ctx->core_mt[0]->sbac, sizeof(XEVD_SBAC));
    }
    return XEVD_OK;

ERR:
    return ret;
}

int xevd_malloc_1d(void** dst, int size)
{
    if(*dst == NULL)
    {
        *dst = xevd_malloc_fast(size);
        xevd_assert_rv(*dst, XEVD_ERR_OUT_OF_MEMORY);

        xevd_mset(*dst, 0, size);
    }
    return XEVD_OK;
}

int xevd_malloc_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    if(*dst == NULL)
    {
        *dst = xevd_malloc_fast(size_1d * sizeof(s8*));
        xevd_assert_rv(*dst, XEVD_ERR_OUT_OF_MEMORY);

        xevd_mset(*dst, 0, size_1d * sizeof(s8*));

        (*dst)[0] = xevd_malloc_fast(size_1d * size_2d * type_size);
        xevd_assert_rv((*dst)[0], XEVD_ERR_OUT_OF_MEMORY);

        xevd_mset((*dst)[0], 0, size_1d * size_2d * type_size);

        for(i = 1; i < size_1d; i++)
        {
            (*dst)[i] = (*dst)[i - 1] + size_2d * type_size;
        }
    }
    return XEVD_OK;
}

int xevd_create_cu_data(XEVD_CU_DATA *cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;
    int ret = XEVD_OK;
    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    ret = xevd_malloc_1d((void**)&cu_data->qp_y, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->qp_u, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->qp_v, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->pred_mode, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->pred_mode_chroma, size_8b);

    ret = xevd_malloc_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    ret = xevd_malloc_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    ret = xevd_malloc_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));
    ret = xevd_malloc_1d((void**)&cu_data->skip_flag, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->ibc_flag, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->dmvr_flag, size_8b);
    ret = xevd_malloc_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    ret = xevd_malloc_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));
    ret = xevd_malloc_1d((void**)&cu_data->mvr_idx, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->bi_idx, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->inter_dir, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->mmvd_idx, size_16b);
    ret = xevd_malloc_1d((void**)&cu_data->mmvd_flag, size_8b);

    ret = xevd_malloc_1d((void**)& cu_data->ats_intra_cu, size_8b);
    ret = xevd_malloc_1d((void**)& cu_data->ats_mode_h, size_8b);
    ret = xevd_malloc_1d((void**)& cu_data->ats_mode_v, size_8b);

    ret = xevd_malloc_1d((void**)&cu_data->ats_inter_info, size_8b);

    for(i = 0; i < N_C; i++)
    {
        ret = xevd_malloc_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            ret = xevd_malloc_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    ret = xevd_malloc_1d((void**)&cu_data->map_scu, size_32b);
    ret = xevd_malloc_1d((void**)&cu_data->affine_flag, size_8b);
    ret = xevd_malloc_1d((void**)&cu_data->map_affine, size_32b);
    ret = xevd_malloc_1d((void**)&cu_data->map_cu_mode, size_32b);
    ret = xevd_malloc_1d((void**)&cu_data->depth, size_8b);

    for(i = 0; i < N_C; i++)
    {
        ret = xevd_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(s16));
        ret = xevd_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(pel));
    }

    return ret;
}

int xevdm_ready(XEVD_CTX *ctx)
{
    int ret = XEVD_OK;
    XEVD_CORE *core = NULL;
    XEVDM_CORE *mcore = NULL;
    xevd_assert(ctx);


    core = core_alloc();
    xevd_assert_gv(core != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);

    ctx->core = core;

    for (int i = 0; i < DEC_XEVD_MAX_TASK_CNT; i++)
    {
        mcore = xevdm_core_alloc();
        xevd_assert_gv(mcore != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
        core = (XEVD_CORE *)mcore;
        ctx->core_mt[i] = core;
    }
    return XEVD_OK;
ERR:
    if (core)
    {
        core_free(core);
    }

    return ret;
}

void xevdm_flush(XEVD_CTX * ctx)
{
    if(ctx->core)
    {
        core_free(ctx->core);
        ctx->core = NULL;
    }

    for(int i = 0; i < DEC_XEVD_MAX_TASK_CNT; i++)
    {
        if(ctx->core_mt[i])
        {
            core_free(ctx->core_mt[i]);
            ctx->core_mt[i] = NULL;
        }
    }
}

static int clear_map(XEVD_CTX * ctx)
{
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    /* clear maps */
    xevd_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    xevd_mset_x64a(ctx->map_cu_mode, 0, sizeof(u32) * ctx->f_scu);
    xevd_mset_x64a(mctx->map_affine, 0, sizeof(u32) * ctx->f_scu);
    xevd_mset_x64a(mctx->map_ats_inter, 0, sizeof(u8) * ctx->f_scu);

    return XEVD_OK;
}

int xevd_dec_nalu(XEVD_CTX * ctx, XEVD_BITB * bitb, XEVD_STAT * stat)
{
    XEVD_BSR  *bs = &ctx->bs;
    XEVD_SPS  *sps = &ctx->sps_array[ctx->sps_id];
    XEVD_PPS  *pps = &ctx->pps;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    XEVD_APS_GEN  *aps_array = mctx->aps_gen_array;
    XEVD_APS *aps = &mctx->aps;
    XEVD_SH   *sh = &ctx->sh;
    XEVDM_SH  *msh = &mctx->sh;
    XEVD_NALU *nalu = &ctx->nalu;
    int        ret;

    ret = XEVD_OK;
    /* set error status */
    ctx->bs_err = bitb->err;

#if !TRACE_DBF
    XEVD_TRACE_SET(1);
#endif

    /* bitstream reader initialization */
    xevd_bsr_init(bs, bitb->addr, bitb->ssize, NULL);
    SET_SBAC_DEC(bs, &ctx->sbac_dec);

    /* parse nalu header */
    ret = xevd_eco_nalu(bs, nalu);
    xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    mctx->aps_temp = -1;
    if(nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_SPS)
    {
        XEVD_SPS sps_new;
        xevd_mset(&sps_new, 0, sizeof(XEVD_SPS));
        ret = xevdm_eco_sps(bs, &sps_new);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        ctx->sps_id = sps_new.sps_seq_parameter_set_id;
        xevd_mcpy(&ctx->sps_array[ctx->sps_id], &sps_new, sizeof(XEVD_SPS));
        ctx->sps = &ctx->sps_array[ctx->sps_id];

        ret = sequence_init(ctx, ctx->sps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        ctx->sps_count++;
    }
    else if (nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_PPS)
    {
        ret = xevdm_eco_pps(bs, sps, pps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        int pps_id = pps->pps_pic_parameter_set_id;
        xevd_mcpy(&(ctx->pps_array[pps_id]), pps, sizeof(XEVD_PPS));
        ret = picture_init(ctx);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    }
    else if (nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_APS)
    {
        XEVD_ALF_SLICE_PARAM alf_control;
        alf_control.is_ctb_alf_on = 0;
        SIG_PARAM_DRA dra_control;
        dra_control.signal_dra_flag = 0;

        aps_array[0].aps_id = -1; // flag, aps not used yet
        aps_array[0].aps_data = (void*)&alf_control;
        aps_array[1].aps_id = -1; // flag, aps not used yet
        aps_array[1].aps_data = (void*)&dra_control;

        XEVD_APS_GEN *local_aps_gen = aps_array;
        ret = xevdm_eco_aps_gen(bs, local_aps_gen, ctx->sps->bit_depth_luma_minus8+8);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        if( (aps_array[0].aps_id != -1) && (aps_array[1].aps_id == -1))
        {
            XEVD_APS_GEN *local_aps = local_aps_gen;
            XEVD_APS_GEN *data_aps = aps_array;
            // store in the new buffer
            data_aps->aps_type_id = local_aps->aps_type_id;
            data_aps->aps_id = local_aps->aps_id;

            XEVD_ALF_SLICE_PARAM * alf_param_src = (XEVD_ALF_SLICE_PARAM *)(local_aps->aps_data);
            XEVD_ALF_SLICE_PARAM * alf_param_dst = (XEVD_ALF_SLICE_PARAM *)(data_aps->aps_data);

            alf_param_src->prev_idx = data_aps->aps_id;
            xevd_mcpy(alf_param_dst, alf_param_src, sizeof(XEVD_ALF_SLICE_PARAM));

            // store in the old buffer
            aps->aps_id = local_aps->aps_id;
            aps->alf_aps_param.prev_idx = local_aps->aps_id;
            alf_param_dst = &(aps->alf_aps_param);
            xevd_mcpy(alf_param_dst, alf_param_src, sizeof(XEVD_ALF_SLICE_PARAM));
            store_dec_aps_to_buffer(ctx);
        }
        else if ((aps_array[1].aps_id != -1) && (aps_array[0].aps_id == -1))
        {
            XEVD_APS_GEN *local_aps = local_aps_gen + 1;
            XEVD_APS_GEN *data_aps = aps_array + 1;
            // store in the new buffer
            data_aps->aps_type_id = local_aps->aps_type_id;
            data_aps->aps_id = local_aps->aps_id;

            SIG_PARAM_DRA * alf_param_src = (SIG_PARAM_DRA *)(local_aps->aps_data);
            SIG_PARAM_DRA * alf_param_dst = (SIG_PARAM_DRA *)(data_aps->aps_data);
            xevd_mcpy(alf_param_dst, alf_param_src, sizeof(SIG_PARAM_DRA));
        }
        else
        {
            xevd_trace("This version of XEVD doesnot support APS type\n");
        }
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
    }
    else if (nalu->nal_unit_type_plus1 - 1 < XEVD_NUT_SPS)
    {
        static u16 slice_num = 0;
        if (ctx->num_ctb == 0)
        {
            ctx->num_ctb = ctx->f_lcu;
            slice_num = 0;
        }

        if (slice_num == 0)
        {
            clear_map(ctx);
            xevd_mset(msh->alf_sh_param.alf_ctu_enable_flag, 1, N_C * ctx->f_lcu * sizeof(u8));
        }

        /* decode slice header */
        sh->num_ctb = ctx->f_lcu;

        ret = xevdm_eco_sh(bs, ctx->sps, &ctx->pps, sh, msh, ctx->nalu.nal_unit_type_plus1 - 1);

        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        set_active_pps_info(ctx);
        ret = set_tile_info(ctx, ctx->core, pps);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        if (ctx->num_ctb == 0)
        {
           ctx->num_ctb = ctx->f_lcu;
        }

        /* POC derivation process */
        if (ctx->poc.poc_val > ctx->poc.prev_pic_max_poc_val)
        {
            ctx->poc.prev_pic_max_poc_val = ctx->poc.poc_val;
        }

        if(!sps->tool_pocs) //sps_pocs_flag == 0
        {
            if (ctx->nalu.nal_unit_type_plus1 - 1 == XEVD_NUT_IDR)
            {
                sh->poc_lsb = 0;
                ctx->poc.prev_doc_offset = -1;
                ctx->poc.prev_poc_val = 0;
                ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps->log2_sub_gop_length);
                ctx->poc.poc_val = 0;
            }
            else
            {
                ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps->log2_sub_gop_length);
                xevd_poc_derivation(ctx->sps, ctx->nalu.nuh_temporal_id, &ctx->poc);
                sh->poc_lsb = ctx->poc.poc_val;
            }
        }
        else //sps_pocs_flag == 1
        {
            if (ctx->nalu.nal_unit_type_plus1 - 1 == XEVD_NUT_IDR)
            {
                sh->poc_lsb = 0;
                ctx->poc.poc_val = 0;
            }
            else
            {
                XEVD_POC * poc = &ctx->poc;
                int poc_msb, poc_lsb, max_poc_lsb, prev_poc_lsb, prev_poc_msb;

                poc_lsb = sh->poc_lsb;
                max_poc_lsb = 1<<(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
                prev_poc_lsb = poc->prev_poc_val & (max_poc_lsb - 1);
                prev_poc_msb = poc->prev_poc_val- prev_poc_lsb;
                if ((poc_lsb < prev_poc_lsb) && ((prev_poc_lsb - poc_lsb) >= (max_poc_lsb / 2)))
                    poc_msb = prev_poc_msb + max_poc_lsb;
                else if ((poc_lsb > prev_poc_lsb) && ((poc_lsb - prev_poc_lsb) > (max_poc_lsb / 2)))
                    poc_msb = prev_poc_msb - max_poc_lsb;
                else
                    poc_msb = prev_poc_msb;

                poc->poc_val = poc_msb + poc_lsb;

                if(ctx->nalu.nuh_temporal_id == 0)
                {
                    poc->prev_poc_val = poc->poc_val;
                }
            }

            ctx->slice_ref_flag = 1;
        }

        s32 pic_delay = (ctx->poc.poc_val - (s32)ctx->poc.prev_pic_max_poc_val - 1);
        if (ctx->max_coding_delay < pic_delay)
        {
            ctx->max_coding_delay = pic_delay;
        }

        ret = slice_init(ctx, ctx->core, sh);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        ctx->slice_num = slice_num;
        slice_num++;

        if (!sps->tool_rpl)
        {
            /* initialize reference pictures */
            ret = xevdm_picman_refp_init(&mctx->dpm, ctx->sps->max_num_ref_pics, sh->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
        }
        else
        {
            /* reference picture marking */
            ret = xevdm_picman_refpic_marking(&mctx->dpm, sh, ctx->poc.poc_val);
            xevd_assert_rv(ret == XEVD_OK, ret);

            /* reference picture lists construction */
            ret = xevdm_picman_refp_rpl_based_init(&mctx->dpm, sh, ctx->poc.poc_val, ctx->refp);
        }
        xevd_assert_rv(ret == XEVD_OK, ret);

        if (ctx->num_ctb == ctx->f_lcu)
        {
            /* get available frame buffer for decoded image */
            ctx->pic = xevdm_picman_get_empty_pic(&mctx->dpm, &ret, ctx->internal_codec_bit_depth);
            xevd_assert_rv(ctx->pic, ret);

            /* get available frame buffer for decoded image */
            ctx->map_refi = ctx->pic->map_refi;
            ctx->map_mv = ctx->pic->map_mv;
            mctx->map_unrefined_mv = ctx->pic->map_unrefined_mv;

            int size;
            size = sizeof(s8) * ctx->f_scu * REFP_NUM;
            xevd_mset_x64a(ctx->map_refi, -1, size);
            size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
            xevd_mset_x64a(ctx->map_mv, 0, size);
            size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
            xevd_mset_x64a(mctx->map_unrefined_mv, 0, size);

            ctx->pic->imgb->imgb_active_pps_id = ctx->pps.pps_pic_parameter_set_id;
            if (ctx->sps->tool_dra)
            {
                if (ctx->pps.pic_dra_enabled_flag == 1)
                    ctx->pic->imgb->imgb_active_aps_id = ctx->pps.pic_dra_aps_id;
                else
                    ctx->pic->imgb->imgb_active_aps_id = -1;
            }
        }

        /* decode slice layer */
        ret = ctx->fn_dec_slice(ctx, ctx->core);
        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);

        if (ctx->num_ctb == 0)
        {
            /* deblocking filter */
            if(ctx->sh.deblocking_filter_on)
            {
#if TRACE_DBF
                XEVD_TRACE_SET(1);
#endif
                u32 k = 0;
                int i, j, res;
                int num_tiles_in_pic, tile_start_num, num_tiles_proc;
                XEVD_CORE * core_mt;

                for(int is_hor_edge = 0 ; is_hor_edge <= 1 ; is_hor_edge++)
                {
                    for (u32 i = 0; i < ctx->f_scu; i++)
                    {
                        MCU_CLR_COD(ctx->map_scu[i]);
                    }

                    k = 0;
                    res = 0;
                    num_tiles_in_pic = ctx->w_tile * ctx->h_tile;
                    while (num_tiles_in_pic)
                    {
                        if(num_tiles_in_pic > ctx->tc.max_task_cnt)
                        {
                            tile_start_num = k;
                            num_tiles_proc = ctx->tc.max_task_cnt;
                        }
                        else
                        {
                            tile_start_num = k;
                            num_tiles_proc = num_tiles_in_pic;
                        }

                        for(j = 1; j < num_tiles_proc; j++)
                        {
                            i = tile_start_num + j - 1;
                            core_mt = ctx->core_mt[j];
                            core_mt->ctx = ctx;
                            core_mt->tile_num = i;
                            core_mt->filter_across_boundary = 0;
                            core_mt->deblock_is_hor = is_hor_edge;
                            ret = ctx->tc.run(ctx->thread_pool[j], ctx->fn_deblock, (void *)core_mt);
                            xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
                            k++;
                        }
                        j = tile_start_num + num_tiles_proc - 1;
                        core_mt = ctx->core_mt[0];
                        core_mt->ctx = ctx;
                        core_mt->tile_num = j;
                        core_mt->filter_across_boundary = 0;
                        core_mt->deblock_is_hor = is_hor_edge;
                        ret = ctx->fn_deblock((void *)core_mt);
                        xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
                        k++;
                        for(i = 1; i<num_tiles_proc; i++)
                        {
                            ret = ctx->tc.join(ctx->thread_pool[i], &res);
                        }
                        num_tiles_in_pic -= (num_tiles_proc);
                    }
                }
#if TRACE_DBF
                XEVD_TRACE_SET(0);
#endif
            }

            /* adaptive loop filter */
            if( mctx->sh.alf_on )
            {
                ret = mctx->fn_alf(ctx,  ctx->pic);
                xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
            }

            /* expand pixels to padding area */
            ctx->fn_picbuf_expand(ctx, ctx->pic);

            /* put decoded picture to DPB */
            ret = xevdm_picman_put_pic(&mctx->dpm, ctx->pic, ctx->nalu.nal_unit_type_plus1 - 1 == XEVD_NUT_IDR, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, 1, ctx->refp, ctx->slice_ref_flag, sps->tool_rpl, ctx->ref_pic_gap_length);
            xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
        }

        if(ctx->pic_cnt == 0) {
            ctx->ts.frame_first_dts = bitb->ts[XEVD_TS_DTS];
            ctx->ts.frame_duration_time = 0;
        } else if(ctx->pic_cnt == 1) {
            ctx->ts.frame_duration_time = bitb->ts[XEVD_TS_DTS] - ctx->ts.frame_first_dts;
        }
        ctx->pic->imgb->ts[XEVD_TS_DTS] = bitb->ts[XEVD_TS_DTS];

        int coding_delay = ctx->poc.poc_val - (int)ctx->pic_cnt; // number of frames between coding and presentation
        ctx->pic->imgb->ts[XEVD_TS_PTS] = bitb->ts[XEVD_TS_DTS] + coding_delay * ctx->ts.frame_duration_time;

        for (int i=0; i<XEVD_NDATA_NUM; i++) {
            ctx->pic->imgb->ndata[i] = bitb->ndata[i];
        }

        for (int i=0; i<XEVD_PDATA_NUM; i++) {
            ctx->pic->imgb->pdata[i] = bitb->pdata[i];
        }

        slice_deinit(ctx);
    }
    else if (nalu->nal_unit_type_plus1 - 1 == XEVD_NUT_SEI)
    {
        ret = xevd_eco_sei(ctx, bs);

        if (ctx->pic_sign_exist)
        {
            if (ctx->use_pic_sign)
            {
                int compare_md5 = 1;
                SIG_PARAM_DRA *effective_dra_control;
                if (ctx->pps.pic_dra_enabled_flag)
                {
                    assert(ctx->pic->imgb->imgb_active_aps_id == ctx->pps.pic_dra_aps_id);
                    effective_dra_control = mctx->dra_array + ctx->pps.pic_dra_aps_id;
                }
                else
                {
                    assert(ctx->pic->imgb->imgb_active_aps_id == -1);
                    effective_dra_control = NULL;
                }
                XEVD_IMGB *imgb_sig = NULL;
                imgb_sig = xevd_imgb_generate(ctx->w, ctx->h, ctx->pa.pad_l, ctx->pa.pad_c, ctx->pa.idc, ctx->internal_codec_bit_depth);
                xevd_imgb_cpy(imgb_sig, ctx->pic->imgb);  // store copy of the reconstructed picture in DPB

                if (ctx->pps.pic_dra_enabled_flag)
                {
                    DRA_CONTROL l_dra_control;
                    DRA_CONTROL *local_g_dra_control = &l_dra_control;
                    xevd_mcpy(&(local_g_dra_control->signalled_dra), effective_dra_control, sizeof(SIG_PARAM_DRA));

                    xevd_init_dra(local_g_dra_control, ctx->internal_codec_bit_depth);
                    xevd_apply_dra_chroma_plane(imgb_sig, imgb_sig, local_g_dra_control, 1, TRUE);
                    xevd_apply_dra_chroma_plane(imgb_sig, imgb_sig, local_g_dra_control, 2, TRUE);
                    xevd_apply_dra_luma_plane(imgb_sig, imgb_sig, local_g_dra_control, 0, TRUE);
                }
                ret = xevdm_picbuf_check_signature(ctx->pic, ctx->pic_sign, imgb_sig, compare_md5);
                xevd_assert_rv(XEVD_SUCCEEDED(ret), ret);
                xevd_imgb_destroy(imgb_sig);

                ctx->pic_sign_exist = 0;
            }
            else
            {
                ret = XEVD_WARN_CRC_IGNORED;
            }
        }
    }
    else
    {
        assert(!"wrong NALU type");
    }

    make_stat(ctx, nalu->nal_unit_type_plus1 - 1, stat);

    if (ctx->num_ctb > 0)
    {
        stat->fnum = -1;
    }

    return ret;
}

/* DRA frame level init and apply DRA*/
int xevd_apply_filter(XEVD_CTX *ctx, XEVD_IMGB *imgb)
{
    if (imgb) {

        XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;

        int pps_dra_id = (imgb)->imgb_active_aps_id;

        SIG_PARAM_DRA *g_dra_control = mctx->dra_array;
        DRA_CONTROL g_dra_control_effective;

        if ((pps_dra_id > -1) && (pps_dra_id < 32))
        {
            g_dra_control_effective.flag_enabled = 1;
        }
        else
        {
            g_dra_control_effective.flag_enabled = 0;
        }
        int sps_dra_enable_flag = ctx->sps->tool_dra;

        if ((sps_dra_enable_flag == 1) && (pps_dra_id >= 0))
        {
            // Assigned effective DRA controls as specified by PPS
            xevd_assert((pps_dra_id > -1) && (pps_dra_id < 32) && ((g_dra_control + pps_dra_id)->signal_dra_flag == 1));
            xevd_mcpy(&(g_dra_control_effective.signalled_dra), (g_dra_control + pps_dra_id), sizeof(SIG_PARAM_DRA));
            mctx->pps_dra_params = (void *)&(g_dra_control_effective.signalled_dra);

            if (g_dra_control_effective.flag_enabled)
            {
                xevd_init_dra(&g_dra_control_effective, ctx->internal_codec_bit_depth);

                xevd_apply_dra_chroma_plane(imgb, imgb, &g_dra_control_effective, 1, TRUE);
                xevd_apply_dra_chroma_plane(imgb, imgb, &g_dra_control_effective, 2, TRUE);
                xevd_apply_dra_luma_plane(imgb, imgb, &g_dra_control_effective, 0, TRUE);
            }
        }
    }
    return XEVD_OK;
}

int xevd_pull_frm(XEVD_CTX *ctx, XEVD_IMGB **imgb)
{
    int ret;
    XEVD_PIC *pic;

    *imgb = NULL;
    XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
    pic = xevdm_picman_out_pic(&mctx->dpm, &ret);

    if(pic)
    {
        xevd_assert_rv(pic->imgb != NULL, XEVD_ERR);

        /* increase reference count */
        pic->imgb->addref(pic->imgb);
        *imgb = pic->imgb;
        if (ctx->sps->picture_cropping_flag)
        {
            (*imgb)->crop_idx = 1;
            (*imgb)->crop_l = ctx->sps->picture_crop_left_offset;
            (*imgb)->crop_r = ctx->sps->picture_crop_right_offset;
            (*imgb)->crop_t = ctx->sps->picture_crop_top_offset;
            (*imgb)->crop_b = ctx->sps->picture_crop_bottom_offset;
        }

        if (ctx->sps->tool_dra) {
            XEVD_IMGB * imgb_dra = NULL;
            imgb_dra = xevd_imgb_generate(ctx->w, ctx->h, ctx->pa.pad_l, ctx->pa.pad_c, ctx->pa.idc, ctx->internal_codec_bit_depth);
            xevd_imgb_cpy(imgb_dra, *imgb);
            (*imgb)->release(*imgb);
            xevd_apply_filter(ctx, imgb_dra);
            *imgb = imgb_dra;
        }
    }
    return ret;
}

int xevdm_platform_init(XEVD_CTX *ctx)
{
#if ARM_NEON
    xevd_func_itrans     = xevdm_itrans_map_tbl_neon;
    xevdm_fn_itx         = &xevdm_tbl_itx_neon;
    xevdm_func_dmvr_mc_l = xevdm_tbl_dmvr_mc_l_neon;
    xevdm_func_dmvr_mc_c = xevdm_tbl_dmvr_mc_c_neon;
    xevdm_func_bl_mc_l   = xevdm_tbl_bl_mc_l_neon;
    xevd_func_mc_l       = xevd_tbl_mc_l_neon;
    xevd_func_mc_c       = xevd_tbl_mc_c_neon;
    xevd_func_average_no_clip = &xevd_average_16b_no_clip_neon;
    ctx->fn_itxb         = &xevd_tbl_itxb_neon;
    ctx->fn_dbk          = &xevd_tbl_dbk_neon;
    ctx->fn_dbk_chroma   = &xevd_tbl_dbk_chroma_neon;
#else
#if X86_SSE
    int check_cpu, support_sse, support_avx, support_avx2;

    check_cpu = xevd_check_cpu_info();
    support_sse = (check_cpu >> 1) & 1;
    support_avx = check_cpu & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        xevd_func_itrans     = xevdm_itrans_map_tbl_sse;
        xevdm_fn_itx          = &xevdm_tbl_itx_avx;
        xevdm_func_dmvr_mc_l = xevdm_tbl_dmvr_mc_l_sse;
        xevdm_func_dmvr_mc_c = xevdm_tbl_dmvr_mc_c_sse;
        xevdm_func_bl_mc_l   = xevdm_tbl_bl_mc_l_sse;
        xevd_func_mc_l       = xevd_tbl_mc_l_avx;
        xevd_func_mc_c       = xevd_tbl_mc_c_avx;
        xevd_func_average_no_clip = xevd_average_16b_no_clip_sse;
        ctx->fn_itxb         = &xevd_tbl_itxb_avx;
        ctx->fn_dbk          = &xevd_tbl_dbk_sse;
        ctx->fn_dbk_chroma   = &xevd_tbl_dbk_chroma_sse;
    }
    else if (support_sse)
    {
        xevd_func_itrans     = xevdm_itrans_map_tbl_sse;
        xevdm_fn_itx          = &xevdm_tbl_itx;
        xevdm_func_dmvr_mc_l = xevdm_tbl_dmvr_mc_l_sse;
        xevdm_func_dmvr_mc_c = xevdm_tbl_dmvr_mc_c_sse;
        xevdm_func_bl_mc_l   = xevdm_tbl_bl_mc_l_sse;
        xevd_func_mc_l       = xevd_tbl_mc_l_sse;
        xevd_func_mc_c       = xevd_tbl_mc_c_sse;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip_sse;
        ctx->fn_itxb         = &xevd_tbl_itxb_sse;
        ctx->fn_dbk          = &xevd_tbl_dbk_sse;
        ctx->fn_dbk_chroma   = &xevd_tbl_dbk_chroma_sse;
    }
    else
    {
        xevd_func_itrans     = xevdm_itrans_map_tbl;
        xevdm_fn_itx          = &xevdm_tbl_itx;
        xevdm_func_dmvr_mc_l = xevdm_tbl_dmvr_mc_l;
        xevdm_func_dmvr_mc_c = xevdm_tbl_dmvr_mc_c;
        xevdm_func_bl_mc_l   = xevdm_tbl_bl_mc_l;
        xevd_func_mc_l       = xevd_tbl_mc_l;
        xevd_func_mc_c       = xevd_tbl_mc_c;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip;
        ctx->fn_itxb         = &xevd_tbl_itxb;
        ctx->fn_dbk          = &xevd_tbl_dbk;
        ctx->fn_dbk_chroma   = &xevd_tbl_dbk_chroma;
    }
#else
    {
        xevd_func_itrans     = xevdm_itrans_map_tbl;
        xevdm_fn_itx         = &xevdm_tbl_itx;
        xevdm_func_dmvr_mc_l = xevdm_tbl_dmvr_mc_l;
        xevdm_func_dmvr_mc_c = xevdm_tbl_dmvr_mc_c;
        xevdm_func_bl_mc_l   = xevdm_tbl_bl_mc_l;
        xevd_func_mc_l       = xevd_tbl_mc_l;
        xevd_func_mc_c       = xevd_tbl_mc_c;
        xevd_func_average_no_clip = &xevd_average_16b_no_clip;
        ctx->fn_itxb         = &xevd_tbl_itxb;
        ctx->fn_dbk          = &xevd_tbl_dbk;
        ctx->fn_dbk_chroma   = &xevd_tbl_dbk_chroma;
    }
#endif
#endif
    ctx->fn_ready         = xevdm_ready;
    ctx->fn_flush         = xevdm_flush;
    ctx->fn_dec_cnk       = xevd_dec_nalu;
    ctx->fn_dec_slice     = xevdm_dec_slice;
    ctx->fn_pull          = xevd_pull_frm;
    ctx->fn_deblock       = xevdm_deblock;
    ctx->fn_picbuf_expand = xevd_picbuf_expand;
    ctx->pf               = NULL;

    return XEVD_OK;
}

void xevdm_platform_deinit(XEVD_CTX * ctx)
{
    xevd_assert(ctx->pf == NULL);

    ctx->fn_ready         = NULL;
    ctx->fn_flush         = NULL;
    ctx->fn_dec_cnk       = NULL;
    ctx->fn_dec_slice     = NULL;
    ctx->fn_pull          = NULL;
    ctx->fn_deblock       = NULL;
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;
    ADAPTIVE_LOOP_FILTER* alf = (ADAPTIVE_LOOP_FILTER*)(mctx->alf);
    if (alf != NULL)
    {
        xevd_alf_destroy(alf);
    }
    if (mctx->alf != NULL)
    {
        delete_alf(mctx->alf);
        mctx->fn_alf = NULL;
    }

    ctx->fn_picbuf_expand = NULL;
}

XEVD xevd_create(XEVD_CDSC * cdsc, int * err)
{
    XEVD_CTX *ctx = NULL;
    int ret;


#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("dec_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("dec_trace.txt", "w+");
#endif
#endif

    ctx = (XEVD_CTX *)xevdm_ctx_alloc();
    XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
    xevd_assert_gv(ctx != NULL, ret, XEVD_ERR_OUT_OF_MEMORY, ERR);
    xevd_mcpy(&ctx->cdsc, cdsc, sizeof(XEVD_CDSC));
    xevd_assert_gv(!(cdsc->threads > XEVD_MAX_TASK_CNT), ret, XEVD_ERR_THREAD_ALLOCATION, ERR);
    ret = xevd_init_thread_controller(&ctx->tc, cdsc->threads);
    xevd_assert_g(XEVD_SUCCEEDED(ret), ERR);
    //initialize the threads to NULL
    for (int i = 0; i < XEVD_MAX_TASK_CNT; i++)
    {
        ctx->thread_pool[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_block = xevd_get_synchronized_object();
    xevd_assert_gv(ctx->sync_block != NULL, ret, XEVD_ERR_UNKNOWN, ERR);

    if (ctx->tc.max_task_cnt > 1)
    {
        for (int i = 1; i < ctx->tc.max_task_cnt; i++)
        {
            ctx->thread_pool[i] = ctx->tc.create(&ctx->tc, i);
            xevd_assert_gv(ctx->thread_pool[i] != NULL, ret, XEVD_ERR_UNKNOWN, ERR);
        }
    }

    /* additional initialization for each platform, if needed */
    ret = xevdm_platform_init(ctx);
    xevd_assert_g(ret == XEVD_OK, ERR);

    ret = xevd_scan_tbl_init(ctx);
    xevd_assert_g(ret == XEVD_OK, ERR);

    if (ctx->fn_ready)
    {
        ret = ctx->fn_ready(ctx);
        xevd_assert_g(ret == XEVD_OK, ERR);
    }

    mctx->fn_alf = xevd_alf;

    XEVD_ALF_SLICE_PARAM alf_control;
    XEVD_ALF_SLICE_PARAM *aps_alf_control = &alf_control;

    DRA_CONTROL dra_control_read;
    XEVD_APS_GEN *aps_gen = mctx->aps_gen_array;

    dra_control_read.signalled_dra.signal_dra_flag = 0;
    aps_gen->aps_data = (void*)aps_alf_control;
    (aps_gen + 1)->aps_data = (void*)(&(dra_control_read.signalled_dra));
    xevd_reset_aps_gen_read_buffer(aps_gen);

    SIG_PARAM_DRA *dra_array = mctx->dra_array;
    for (int i = 0; i < 32; i++)
    {
        (dra_array + i)->signal_dra_flag = -1;
    }

    /* Set CTX variables to default value */
    ctx->magic = XEVD_MAGIC_CODE;
    ctx->id = (XEVD)ctx;
    xevdm_init_multi_tbl();
    xevd_init_multi_inv_tbl();

    return (ctx->id);
ERR:
    if (ctx)
    {
        if (ctx->sync_block)
        {
            xevd_release_synchornized_object(&ctx->sync_block);
        }
        if (ctx->tc.max_task_cnt)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 1; i < ctx->tc.max_task_cnt; i++)
            {
                if (ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc.release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            xevd_dinit_thread_controller(&ctx->tc);
        }
        if (ctx->fn_flush) ctx->fn_flush(ctx);
        xevdm_platform_deinit(ctx);
        ctx_free(ctx);
    }

    if (err) *err = ret;

    return NULL;
}

void xevd_delete(XEVD id)
{
    XEVD_CTX *ctx;
    XEVD_ID_TO_CTX_R(id, ctx);
    XEVDM_CTX *mctx = (XEVDM_CTX *)ctx;
    sequence_deinit(ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    if (ctx->sync_block)
    {
        xevd_release_synchornized_object(&ctx->sync_block);
    }

    //release the thread pool
    if (ctx->tc.max_task_cnt)
    {
        //thread controller instance is present
        //terminate the created thread
        for (int i = 1; i < ctx->tc.max_task_cnt; i++)
        {
            if (ctx->thread_pool[i])
            {
                //valid thread instance
                ctx->tc.release(&ctx->thread_pool[i]);
            }
        }
        //dinitialize the tc
        xevd_dinit_thread_controller(&ctx->tc);
    }
    if(ctx->fn_flush) ctx->fn_flush(ctx);

    /* addtional deinitialization for each platform, if needed */
    xevdm_platform_deinit(ctx);
    xevd_scan_tbl_delete(ctx);
    ctx_free(ctx);
}

int xevd_config(XEVD id, int cfg, void * buf, int * size)
{
    XEVD_CTX *ctx;
    int t0 = 0;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
    /* set config ************************************************************/
    case XEVD_CFG_SET_USE_PIC_SIGNATURE:
        ctx->use_pic_sign = (*((int *)buf)) ? 1 : 0;
        break;

    /* get config ************************************************************/
    case XEVD_CFG_GET_CODEC_BIT_DEPTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->internal_codec_bit_depth;
        break;

    case XEVD_CFG_GET_WIDTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        t0 = ctx->sps->picture_crop_left_offset + ctx->sps->picture_crop_right_offset;
        if(ctx->sps->chroma_format_idc) { t0 *= 2; /* unit is chroma */}
        *((int *)buf) = ctx->w - t0;
        break;

    case XEVD_CFG_GET_HEIGHT:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        t0 = ctx->sps->picture_crop_top_offset + ctx->sps->picture_crop_bottom_offset;
        if(ctx->sps->chroma_format_idc) { t0 *= 2; /* unit is chroma */}
        *((int *)buf) = ctx->h - t0;
        break;

    case XEVD_CFG_GET_CODED_WIDTH:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->w;
        break;

    case XEVD_CFG_GET_CODED_HEIGHT:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->h;
        break;

    case XEVD_CFG_GET_COLOR_SPACE:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = xevd_chroma_format_idc_to_imgb_cs[ctx->sps->chroma_format_idc];
        break;

    case XEVD_CFG_GET_MAX_CODING_DELAY:
        xevd_assert_rv(*size == sizeof(int), XEVD_ERR_INVALID_ARGUMENT);
        *((int *)buf) = ctx->max_coding_delay;
        break;

    default:
        xevd_assert_rv(0, XEVD_ERR_UNSUPPORTED);
    }
    return XEVD_OK;
}

int xevd_decode(XEVD id, XEVD_BITB * bitb, XEVD_STAT * stat)
{
    XEVD_CTX *ctx;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);
    XEVDM_CTX * mctx = (XEVDM_CTX *)ctx;

    SIG_PARAM_DRA *dra_control = mctx->dra_array;
    DRA_CONTROL dra_control_effective;

    XEVD_APS_GEN *aps_gen_array = mctx->aps_gen_array;

    if (ctx->sps && ctx->sps->tool_dra)
    {
        // check if new DRA APS recieved, update buffer
        if ((aps_gen_array + 1)->aps_id != -1)
        {
            xevd_add_dra_aps_to_buffer(dra_control, aps_gen_array);
        }
        // Assigned effective DRA controls as specified by PPS
        int pps_dra_id = ctx->pps.pic_dra_aps_id;
        if ((pps_dra_id > -1) && (pps_dra_id < 32))
        {
            memcpy(&(dra_control_effective.signalled_dra), dra_control + pps_dra_id, sizeof(SIG_PARAM_DRA));
            dra_control_effective.flag_enabled = 1;
            mctx->pps_dra_params = &(dra_control_effective.signalled_dra);
        }
        else
        {
            dra_control_effective.flag_enabled = 0;
            dra_control_effective.signalled_dra.signal_dra_flag = 0;
        }
    }

    xevd_assert_rv(ctx->fn_dec_cnk, XEVD_ERR_UNEXPECTED);

    return ctx->fn_dec_cnk(ctx, bitb, stat);
}

int xevd_pull(XEVD id, XEVD_IMGB ** imgb)
{
    XEVD_CTX *ctx;

    XEVD_ID_TO_CTX_RV(id, ctx, XEVD_ERR_INVALID_ARGUMENT);
    xevd_assert_rv(ctx->fn_pull, XEVD_ERR_UNKNOWN);

    return ctx->fn_pull(ctx, imgb);
}

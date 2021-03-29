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

#ifndef _XEVD_TBL_H_
#define _XEVD_TBL_H_

#include "xevd_def.h"
extern const u8   xevd_tbl_log2[257];
extern const s8   xevd_tbl_tm2[2][2];
extern const s8   xevd_tbl_tm4[4][4];
extern const s8   xevd_tbl_tm8[8][8];
extern const s8   xevd_tbl_tm16[16][16];
extern const s8   xevd_tbl_tm32[32][32];
extern const s8   xevd_tbl_tm64[64][64];
extern const s8 * xevd_tbl_tm[MAX_CU_DEPTH];
extern int        xevd_scan_sr[MAX_TR_SIZE*MAX_TR_SIZE];
extern int        xevd_inv_scan_sr[MAX_TR_SIZE*MAX_TR_SIZE];
extern u16      * xevd_inv_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];
extern u16      * xevd_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];
extern const u8   xevd_tbl_mpm[6][6][5];
extern const int  xevd_tbl_dq_scale[6];
extern const int  xevd_tbl_dq_scale_b[6];
extern const u8   xevd_tbl_df_st[4][52];
extern int        xevd_tbl_qp_chroma_adjust_base[XEVD_MAX_QP_TABLE_SIZE];
extern int      * xevd_tbl_qp_chroma_adjust;
extern int        xevd_tbl_qp_chroma_dynamic_ext[2][XEVD_MAX_QP_TABLE_SIZE_EXT];
extern int      * xevd_qp_chroma_dynamic_ext[2];
extern int      * xevd_qp_chroma_dynamic[2];

extern const s16 init_cbf_luma[2][NUM_CTX_CBF_LUMA];
extern const s16 init_cbf_cb[2][NUM_CTX_CBF_CR];
extern const s16 init_cbf_cr[2][NUM_CTX_CBF_CB];
extern const s16 init_cbf_all[2][NUM_CTX_CBF_ALL];
extern const s16 init_dqp[2][NUM_CTX_DELTA_QP];
extern const s16 init_pred_mode[2][NUM_CTX_PRED_MODE];
extern const s16 init_direct_mode_flag[2][NUM_CTX_DIRECT_MODE_FLAG];
extern const s16 init_merge_mode_flag[2][NUM_CTX_MERGE_MODE_FLAG];
extern const s16 init_inter_dir[2][NUM_CTX_INTER_PRED_IDC];
extern const s16 init_intra_dir[2][NUM_CTX_INTRA_PRED_MODE];
extern const s16 init_intra_luma_pred_mpm_flag[2][NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG];
extern const s16 init_intra_luma_pred_mpm_idx[2][NUM_CTX_INTRA_LUMA_PRED_MPM_IDX];
extern const s16 init_intra_chroma_pred_mode[2][NUM_CTX_INTRA_CHROMA_PRED_MODE];
extern const s16 init_merge_idx[2][NUM_CTX_MERGE_IDX];
extern const s16 init_mvp_idx[2][NUM_CTX_MVP_IDX];
extern const s16 init_bi_idx[2][NUM_CTX_BI_PRED_IDX];
extern const s16 init_mvd[2][NUM_CTX_MVD];
extern const s16 init_refi[2][NUM_CTX_REF_IDX];
extern const s16 init_run[2][NUM_CTX_CC_RUN];
extern const s16 init_last[2][NUM_CTX_CC_LAST];
extern const s16 init_level[2][NUM_CTX_CC_LEVEL];
extern const s16 init_split_cu_flag[2][NUM_CTX_SPLIT_CU_FLAG];
extern const s16 init_skip_flag[2][NUM_CTX_SKIP_FLAG];
#endif /* _XEVD_TBL_H_ */

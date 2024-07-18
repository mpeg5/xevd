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


extern const u8 xevd_tbl_split_flag_ctx[6][6];
extern const u8 xevd_tbl_log2[257];
extern const s8 xevd_tbl_tm2[2][2];
extern const s8 xevd_tbl_tm4[4][4];
extern const s8 xevd_tbl_tm8[8][8];
extern const s8 xevd_tbl_tm16[16][16];
extern const s8 xevd_tbl_tm32[32][32];
extern const s8 xevd_tbl_tm64[64][64];
extern const s8 * xevd_tbl_tm[MAX_CU_DEPTH];
extern int  xevd_scan_sr[MAX_TR_SIZE*MAX_TR_SIZE];
extern int  xevd_inv_scan_sr[MAX_TR_SIZE*MAX_TR_SIZE];
extern const u8 xevd_tbl_mpm[6][6][5];
extern const int xevd_tbl_dq_scale[6];
extern const int xevd_tbl_dq_scale_b[6];
extern const u8  xevd_tbl_df_st[4][52];
extern const int xevd_tbl_ipred_adi[32][4];
extern const int xevd_tbl_ipred_dxdy[IPD_CNT][2];
extern const u8  xevd_split_order[2][SPLIT_CHECK_NUM];
extern int xevd_tbl_qp_chroma_adjust_main[XEVD_MAX_QP_TABLE_SIZE];
extern int xevd_tbl_qp_chroma_adjust_base[XEVD_MAX_QP_TABLE_SIZE];
extern int * xevd_tbl_qp_chroma_adjust;
extern int xevd_tbl_qp_chroma_dynamic_ext[2][XEVD_MAX_QP_TABLE_SIZE_EXT];
extern int * xevd_qp_chroma_dynamic_ext[2];
extern int * xevd_qp_chroma_dynamic[2];
void xevd_derived_chroma_qp_mapping_tables(XEVD_CHROMA_TABLE *struct_chroma_qp, int bit_depth);
void xevd_set_chroma_qp_tbl_loc(int codec_bit_depth);

struct _XEVD_SCAN_TABLES {
    u16* xevd_inv_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];
    u16* xevd_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];
};

#endif /* _XEVD_TBL_H_ */

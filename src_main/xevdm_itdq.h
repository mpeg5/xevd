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


#ifndef _XEVDM_ITDQ_H_
#define _XEVDM_ITDQ_H_
#include "xevdm_def.h"
#include "xevd_itdq.h"

extern INV_TRANS *(*xevd_func_itrans)[5];
extern INV_TRANS *xevdm_itrans_map_tbl[16][5];

typedef void(*XEVD_ITX)(s16* coef, s16* t, int shift, int line);
extern XEVD_ITX xevdm_tbl_itx[MAX_TR_LOG2];
extern XEVD_ITX(*xevdm_fn_itx)[MAX_TR_LOG2];

#if ARM_NEON
#include "xevdm_itdq_neon.h"
#elif X86_SSE
#include "xevdm_itdq_avx.h"
#include "xevdm_itdq_sse.h"
#endif

void xevdm_itx_pb2(s16* src, s16* dst, int shift, int line);
void xevdm_itx_pb4(s16* src, s16* dst, int shift, int line);
void xevdm_itx_pb8(s16* src, s16* dst, int shift, int line);
void xevdm_itx_pb16(s16* src, s16* dst, int shift, int line);
void xevdm_itx_pb32(s16* src, s16* dst, int shift, int line);
void xevdm_itx_pb64(s16* src, s16* dst, int shift, int line);

void xevdm_itdq(XEVD_CTX * ctx, s16 *coef, int log2_w, int log2_h, int scale, int iqt_flag, u8 ats_intra_cu, u8 ats_mode, int bit_depth);
void xevdm_sub_block_itdq(XEVD_CTX * ctx, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, u8 qp_y, u8 qp_u, u8 qp_v, int flag[N_C], int nnz_sub[N_C][MAX_SUB_TB_NUM], int iqt_flag
                        , u8 ats_intra_cu, u8 ats_mode, u8 ats_inter_info, int bit_depth, int chroma_format_idc);
void xevdm_itrans_ats_intra_DST7_B4(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xevdm_itrans_ats_intra_DCT8_B4(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xevdm_init_multi_tbl();
void xevd_init_multi_inv_tbl();
#endif /* _XEVD_ITDQ_H_ */

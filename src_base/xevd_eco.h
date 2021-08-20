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

#ifndef _XEVD_ECO_H_
#define _XEVD_ECO_H_

#include "xevd_def.h"

#define GET_SBAC_DEC(bs)   ((XEVD_SBAC *)((bs)->pdata[1]))
#define SET_SBAC_DEC(bs, sbac) ((bs)->pdata[1] = (sbac))

u32  xevd_sbac_decode_bin(XEVD_BSR *bs, XEVD_SBAC *sbac, SBAC_CTX_MODEL *model);
u32  xevd_sbac_decode_bin_trm(XEVD_BSR *bs, XEVD_SBAC *sbac);
int  xevd_eco_nalu(XEVD_BSR * bs, XEVD_NALU * nalu);
int  xevd_eco_sps(XEVD_BSR * bs, XEVD_SPS * sps);
int  xevd_eco_pps(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps);
int  xevd_eco_sh(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps, XEVD_SH * sh, int nut);
int  xevd_eco_sei(XEVD_CTX * ctx, XEVD_BSR * bs);
void xevd_eco_sbac_reset(XEVD_BSR * bs, u8 slice_type, u8 slice_qp);
int  xevd_eco_cu(XEVD_CTX * ctx, XEVD_CORE * core);
s8   xevd_eco_split_mode(XEVD_BSR *bs, XEVD_SBAC * sbac, int cuw, int cuh);
u32  xevd_eco_tile_end_flag(XEVD_BSR * bs, XEVD_SBAC * sbac);
s32  xevd_eco_cabac_zero_word(XEVD_BSR* bs);
int  xevd_eco_bi_idx(XEVD_BSR * bs, XEVD_SBAC * sbac);
void xevd_eco_cu_skip_flag(XEVD_CTX * ctx, XEVD_CORE * core);
int  xevd_eco_mvp_idx(XEVD_BSR * bs, XEVD_SBAC * sbac);
void xevd_eco_direct_mode_flag(XEVD_CTX * ctx, XEVD_CORE * core);
int xevd_eco_refi(XEVD_BSR * bs, XEVD_SBAC * sbac, int num_refp);
int xevd_eco_get_mvd(XEVD_BSR * bs, XEVD_SBAC * sbac, s16 mvd[MV_D]);
int xevd_eco_intra_dir(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 mpm[2], u8 mpm_ext[8], u8 pims[IPD_CNT]);
int xevd_eco_intra_dir_c(XEVD_BSR * bs, XEVD_SBAC * sbac, u8 ipm_l);
int xevd_eco_intra_dir_b(XEVD_BSR * bs, XEVD_SBAC * sbac, u8  * mpm, u8 mpm_ext[8], u8 pims[IPD_CNT]);
int xevd_eco_vui(XEVD_BSR * bs, XEVD_VUI * vui);
int xevd_eco_dqp(XEVD_BSR * bs);
#endif /* _XEVD_ECO_H_ */

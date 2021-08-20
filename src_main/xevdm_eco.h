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


#ifndef _XEVDM_ECO_H_
#define _XEVDM_ECO_H_

#include "xevdm_def.h"


int xevdm_eco_sps(XEVD_BSR * bs, XEVD_SPS * sps);
int xevdm_eco_pps(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps);
int xevdm_eco_aps_gen(XEVD_BSR * bs, XEVD_APS_GEN * aps, int bit_depth);
int xevdm_eco_dra_aps_param(XEVD_BSR * bs, XEVD_APS_GEN * aps, int bit_depth);
int xevdm_eco_aps(XEVD_BSR * bs, XEVD_APS * aps);
int xevdm_eco_alf_aps_param(XEVD_BSR * bs, XEVD_APS * aps);
int xevdm_eco_sh(XEVD_BSR * bs, XEVD_SPS * sps, XEVD_PPS * pps, XEVD_SH * sh, XEVDM_SH * msh, int nut);
void xevdm_eco_sbac_reset(XEVD_BSR * bs, u8 slice_type, u8 slice_qp, int sps_cm_init_flag);
int xevdm_eco_cu(XEVD_CTX * ctx, XEVD_CORE * core);
s8 xevdm_eco_split_mode(XEVD_CTX * ctx, XEVD_BSR *bs, XEVD_SBAC *sbac, int cuw, int cuh, const int parent_split, int* same_layer_split, const int node_idx, const int* parent_split_allow, int* curr_split_allow, int qt_depth, int btt_depth, int x, int y, MODE_CONS mode_cons, XEVD_CORE * core);
s8 xevdm_eco_suco_flag(XEVD_BSR *bs, XEVD_SBAC *sbac, XEVD_CTX *c, XEVD_CORE *core, int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, int parent_suco);
int xevdm_eco_affine_mrg_idx(XEVD_BSR * bs, XEVD_SBAC * sbac);
MODE_CONS xevdm_eco_mode_constr( XEVD_BSR *bs, u8 ctx_num );

#endif /* _XEVD_ECO_H_ */

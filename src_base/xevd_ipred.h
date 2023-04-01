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

#ifndef _XEVD_IPRED_H_
#define _XEVD_IPRED_H_

#include "xevd_def.h"

#define XEVD_IPRED_CHK_CONV(mode)\
    ((mode) == IPD_VER || (mode) == IPD_HOR || (mode) == IPD_DC || (mode) == IPD_BI)

#define XEVD_IPRED_CONV_L2C(mode)\
    ((mode) == IPD_VER) ? IPD_VER_C : \
    ((mode) == IPD_HOR ? IPD_HOR_C : ((mode) == IPD_DC ? IPD_DC_C : IPD_BI_C))

#define XEVD_IPRED_CONV_L2C_CHK(mode, chk) \
    if(XEVD_IPRED_CHK_CONV(mode)) \
    {\
        (mode) = ((mode) == IPD_VER) ? IPD_VER_C : ((mode) == IPD_HOR ? IPD_HOR_C:\
        ((mode) == IPD_DC ? IPD_DC_C : IPD_BI_C)); \
        (chk) = 1; \
    }\
    else \
        (chk) = 0;

void xevd_get_nbr_b(int x, int y, int cuw, int cuh, pel *src, int s_src, u16 avail_cu, pel nb[N_C][N_REF][MAX_CU_SIZE * 3], int scup, u32 *map_scu
                  , int w_scu, int h_scu, int ch_type, int constrained_intra_pred, u8* map_tidx, int bit_depth, int chroma_format_idc);
void xevd_ipred_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm, int w, int h);
void xevd_ipred_uv_b(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm_c, int ipm, int w, int h);
void xevd_get_mpm_b(int x_scu, int y_scu, int cuw, int cuh, u32 * map_scu,u8 *cod_eco, s8 * map_ipm, int scup, int w_scu, u8 ** mpm, u16 avail_lr, u8 mpm_ext[8], u8 pms[IPD_CNT], u8 * map_tidx);
int  xevd_get_dc(const int numerator, const int w, const int h);
void xevd_ipred_vert(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h);
void xevd_ipred_plane(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int bit_depth);
void xevd_ipred_bi(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int bit_depth);
void ipred_ang(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth);
#endif /* _XEVD_IPRED_H_ */

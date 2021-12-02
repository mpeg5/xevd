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

#ifndef _XEVD_MC_SSE_H_
#define _XEVD_MC_SSE_H_

#ifdef X86_SSE
extern XEVD_MC_L xevd_tbl_mc_l_sse[2][2];
extern XEVD_MC_C xevd_tbl_mc_c_sse[2][2];

void xevd_average_16b_no_clip_sse(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht, int bit_depth);
void xevd_mc_c_00_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth);
void xevd_mc_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth);
#endif
void xevd_mc_filter_l_8pel_horz_clip_sse(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift);
void xevd_mc_filter_l_8pel_horz_no_clip_sse(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int offset,
    int shift);
void xevd_mc_filter_l_8pel_vert_clip_sse(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift);
void xevd_mc_filter_c_4pel_horz_sse(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last);
void xevd_mc_filter_c_4pel_vert_sse(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last);
#endif /* _XEVD_MC_H_ */

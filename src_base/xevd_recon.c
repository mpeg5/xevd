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
#include "xevd_recon.h"

#include <math.h>

void xevd_recon(s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec,int bit_depth)
{
    int i, j;
    s16 t0;

    if(is_coef == 0) /* just copy pred to rec */
    {
        for(i = 0; i < cuh; i++)
        {
            for(j = 0; j < cuw; j++)
            {
                rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, pred[i * cuw + j]);
            }
        }
#if SIMD_CLIP
        clip_simd(rec, s_rec, rec, s_rec, cuw, cuh, adapt_clip_min[adapt_clip_comp], adapt_clip_max[adapt_clip_comp]);
#endif
    }
    else  /* add b/w pred and coef and copy it into rec */
    {
        for(i = 0; i < cuh; i++)
        {
            for(j = 0; j < cuw; j++)
            {
                t0 = coef[i * cuw + j] + pred[i * cuw + j];
                rec[i * s_rec + j] = XEVD_CLIP3(0, (1 << bit_depth) - 1, t0);
            }
        }
#if SIMD_CLIP
        clip_simd(rec, s_rec, rec, s_rec, cuw, cuh, adapt_clip_min[adapt_clip_comp], adapt_clip_max[adapt_clip_comp]);
#endif
    }
}

void xevd_recon_yuv(XEVD_CTX * ctx, XEVD_CORE* core, int x, int y, int cuw, int cuh)
{
    pel * rec;
    int s_rec, off;
    XEVD_PIC * pic = ctx->pic;
    int bit_depth = ctx->sps->bit_depth_luma_minus8 + 8; 
    int chroma_format_idc = ctx->sps->chroma_format_idc;
    /* Y */
    s_rec = pic->s_l;
    rec = pic->y + (y * s_rec) + x;
    ctx->fn_recon(core->coef[Y_C], core->pred[0][Y_C], core->is_coef[Y_C], cuw, cuh, s_rec, rec, bit_depth);
    
    if (chroma_format_idc != 0)   
    {
        /* chroma */
        cuw >>= (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc));
        cuh >>= (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc));
        off = (x >> (XEVD_GET_CHROMA_W_SHIFT(chroma_format_idc))) + (y >> (XEVD_GET_CHROMA_H_SHIFT(chroma_format_idc))) * pic->s_c;

        ctx->fn_recon(core->coef[U_C], core->pred[0][U_C], core->is_coef[U_C], cuw, cuh, pic->s_c, pic->u + off,  bit_depth);
        ctx->fn_recon(core->coef[V_C], core->pred[0][V_C], core->is_coef[V_C], cuw, cuh, pic->s_c, pic->v + off,  bit_depth);
    }
}
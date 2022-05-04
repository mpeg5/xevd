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


#ifndef _XEVDM_PICMAN_H_
#define _XEVDM_PICMAN_H_

 /*Declaration for ref pic marking and ref pic list construction functions */
int xevdm_picman_refp_rpl_based_init(XEVDM_PM *pm, XEVD_SH *sh, int poc, XEVD_REFP(*refp)[REFP_NUM]);
int xevdm_picman_refpic_marking(XEVDM_PM *pm, XEVD_SH *sh, int poc);

int xevdm_picman_refp_init(XEVDM_PM *pm, int max_num_ref_pics, int slice_type, s32 poc, u8 layer_id, int last_intra, XEVD_REFP (*refp)[REFP_NUM]);

XEVD_PIC * xevdm_picman_get_empty_pic(XEVDM_PM *pm, int *err, int bitdepth);
int xevdm_picman_put_pic(XEVDM_PM *pm, XEVD_PIC *pic, int is_idr, u32 poc, u8 layer_id, int need_for_output, XEVD_REFP (*refp)[REFP_NUM], int ref_pic, int pnpf, int ref_pic_gap_length);
XEVD_PIC * xevdm_picman_out_pic(XEVDM_PM *pm, int *err);
int xevdm_picman_deinit(XEVDM_PM *pm);
int xevdm_picman_init(XEVDM_PM *pm, int max_pb_size, int max_num_ref_pics, PICBUF_ALLOCATOR *pa);

#endif /* _XEVD_PICMAN_H_ */

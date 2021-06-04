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

#include "xevdm_def.h"
#include <math.h>

void xevd_get_signalled_params_dra(DRA_CONTROL *dra_mapping,int bit_depth)
{
    dra_mapping->flag_enabled = dra_mapping->signalled_dra.signal_dra_flag;
    dra_mapping->chroma_qp_model.dra_table_idx = dra_mapping->signalled_dra.dra_table_idx;
    dra_mapping->num_ranges = dra_mapping->signalled_dra.num_ranges;
    dra_mapping->dra_descriptor2 = dra_mapping->signalled_dra.dra_descriptor2;
    dra_mapping->dra_descriptor1 = dra_mapping->signalled_dra.dra_descriptor1;

    dra_mapping->dra_cb_scale_value = dra_mapping->signalled_dra.dra_cb_scale_value;
    dra_mapping->dra_cr_scale_value = dra_mapping->signalled_dra.dra_cr_scale_value;
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales_s32[i] = dra_mapping->signalled_dra.dra_scale_value[i];
    }
    for (int i = 0; i <= dra_mapping->num_ranges; i++)
    {
        dra_mapping->in_ranges[i] = dra_mapping->signalled_dra.in_ranges[i];
    }
    dra_mapping->internal_bd = bit_depth;
}

void xevd_construct_dra(DRA_CONTROL *dra_mapping)
{
    int numFracBits = dra_mapping->dra_descriptor2;
    int NUM_MULT_BITS = SCALE_NUMFBITS + INVSCALE_NUMFBITS;
    int deltas[33];

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }

    dra_mapping->out_ranges_s32[0] = 0;
    for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
    {
        dra_mapping->out_ranges_s32[i] = dra_mapping->out_ranges_s32[i - 1] + deltas[i - 1] * dra_mapping->dra_scales_s32[i - 1];
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        int invScale2;
        int nomin = 1 << NUM_MULT_BITS;
        invScale2 = (int)((nomin + (dra_mapping->dra_scales_s32[i] >> 1)) / dra_mapping->dra_scales_s32[i]);

        int diffVal2 = dra_mapping->out_ranges_s32[i + 1] * invScale2;
        dra_mapping->inv_dra_offsets_s32[i] = ((dra_mapping->in_ranges[i + 1] << NUM_MULT_BITS) - diffVal2 + (1 << (dra_mapping->dra_descriptor2 - 1))) >> (dra_mapping->dra_descriptor2);
        dra_mapping->inv_dra_scales_s32[i] = invScale2;
    }

    for (int i = 0; i < dra_mapping->num_ranges + 1; i++)
    {
        dra_mapping->out_ranges_s32[i] = (dra_mapping->out_ranges_s32[i] + (1 << (numFracBits - 1))) >> numFracBits;
    }
    return;
}
int xevd_get_scaled_chroma_qp2(int compId, int unscaledChromaQP, int bit_depth)
{
    int qpBdOffsetC = 6 * (bit_depth - 8);

    int qp_value = XEVD_CLIP3(-qpBdOffsetC, MAX_QP_TABLE_SIZE - 1, unscaledChromaQP);
    qp_value = *(xevd_qp_chroma_dynamic[compId - 1] + qp_value);
    return qp_value;
}
int xevd_get_dra_range_idx_gen(DRA_CONTROL *dra_mapping, int sample, int *chromaRanges, int numRanges) {
    int rangeIdx = -1;
    for (int i = 0; i < numRanges; i++)
    {
        if (sample < chromaRanges[i + 1])
        {
            rangeIdx = i;
            break;
        }
    }
    if (rangeIdx == -1)
        rangeIdx = numRanges - 1;

    return XEVD_MIN(rangeIdx, numRanges - 1);
}

int xevd_correct_local_chroma_scale(DRA_CONTROL *dra_mapping, int intScaleLumaDra, int chId)
{
    int l_array[NUM_CHROMA_QP_OFFSET_LOG];
    memcpy(l_array, g_dra_chroma_qp_offset_tbl, NUM_CHROMA_QP_OFFSET_LOG * sizeof(int));
    int SCALE_OFFSET = 1 << SCALE_NUMFBITS;
    int TABLE0_SHIFT = NUM_CHROMA_QP_SCALE_EXP >> 1;
    int outChromaScale = 1;

    int localQPi;
    int Qp0, Qp1;
    int scaleDraInt = 1;
    int qpDraFrac = 0;

    if (dra_mapping->chroma_qp_model.dra_table_idx == 58)
    {
        return    scaleDraInt = (chId == 1) ? dra_mapping->dra_cb_scale_value : dra_mapping->dra_cr_scale_value;
    }
    else
    {
        scaleDraInt = (chId == 1) ? dra_mapping->dra_cb_scale_value * intScaleLumaDra : dra_mapping->dra_cr_scale_value * intScaleLumaDra;
        int localChromaQPShift1 = dra_mapping->chroma_qp_model.dra_table_idx - (xevd_get_scaled_chroma_qp2(chId, dra_mapping->chroma_qp_model.dra_table_idx
            , dra_mapping->internal_bd
        ));

        int qpDraInt = 0;
        int OutofRange = -1;
        int scaleDraInt9 = (scaleDraInt + (1 << 8)) >> 9;
        int IndexScaleQP = xevd_get_dra_range_idx_gen(dra_mapping, scaleDraInt9, l_array, NUM_CHROMA_QP_OFFSET_LOG - 1);

        int interpolationNum = scaleDraInt9 - g_dra_chroma_qp_offset_tbl[IndexScaleQP];  //deltaScale (1.2QP)  = 0.109375
        int interpolationDenom = g_dra_chroma_qp_offset_tbl[IndexScaleQP + 1] - g_dra_chroma_qp_offset_tbl[IndexScaleQP];  // DenomScale (2QP) = 0.232421875

        qpDraInt = 2 * IndexScaleQP - 60;  // index table == 0, associated QP == - 1

        if (interpolationNum == 0)
        {
            qpDraInt -= 1;
            qpDraFrac = 0;
        }
        else
        {
            qpDraFrac = SCALE_OFFSET * (interpolationNum << 1) / interpolationDenom;
            qpDraInt += qpDraFrac / SCALE_OFFSET;  // 0
            qpDraFrac = SCALE_OFFSET - (qpDraFrac % SCALE_OFFSET);
        }
        localQPi = dra_mapping->chroma_qp_model.dra_table_idx - qpDraInt;

        Qp0 = xevd_get_scaled_chroma_qp2(chId, XEVD_CLIP3(-(6 * (dra_mapping->internal_bd - 8)), 57, localQPi), dra_mapping->internal_bd);
        Qp1 = xevd_get_scaled_chroma_qp2(chId, XEVD_CLIP3(-(6 * (dra_mapping->internal_bd - 8)), 57, localQPi + 1), dra_mapping->internal_bd);


        int qpChDec = (Qp1 - Qp0) * qpDraFrac;
        int    qpDraFracAdj = qpChDec % (1 << 9);
        int qpDraIntAdj = (qpChDec >> 9);

        qpDraFracAdj = qpDraFrac - qpDraFracAdj;
        int localChromaQPShift2 = localQPi - Qp0 - qpDraIntAdj;

        int draChromaQpShift = localChromaQPShift2 - localChromaQPShift1;
        if (qpDraFracAdj < 0)
        {
            draChromaQpShift -= 1;
            qpDraFracAdj = (1 << 9) + qpDraFracAdj;
        }
        int draChromaQpShift_clipped = XEVD_CLIP3(-12, 12, draChromaQpShift);
        int draChromaScaleShift = g_dra_exp_nom_v2[draChromaQpShift_clipped + TABLE0_SHIFT];

        int draChromaScaleShiftFrac;
        if (draChromaQpShift >= 0)
            draChromaScaleShiftFrac = g_dra_exp_nom_v2[XEVD_CLIP3(-12, 12, draChromaQpShift + 1) + TABLE0_SHIFT] - g_dra_exp_nom_v2[draChromaQpShift_clipped + TABLE0_SHIFT];
        else
            draChromaScaleShiftFrac = g_dra_exp_nom_v2[draChromaQpShift_clipped + TABLE0_SHIFT] - g_dra_exp_nom_v2[XEVD_CLIP3(-12, 12, draChromaQpShift - 1) + TABLE0_SHIFT];

        outChromaScale = draChromaScaleShift + ((draChromaScaleShiftFrac * qpDraFracAdj + (1 << (SCALE_NUMFBITS - 1))) >> SCALE_NUMFBITS);
        return (scaleDraInt * outChromaScale + (1 << 17)) >> 18;
    }
}

void xevd_compensate_chroma_shift_table(DRA_CONTROL *dra_mapping) {
    for (int i = 0; i < dra_mapping->num_ranges; i++) {
        dra_mapping->chroma_dra_scales_s32[0][i] = xevd_correct_local_chroma_scale(dra_mapping, dra_mapping->dra_scales_s32[i], 1);
        dra_mapping->chroma_dra_scales_s32[1][i] = xevd_correct_local_chroma_scale(dra_mapping, dra_mapping->dra_scales_s32[i], 2);
        dra_mapping->chroma_inv_dra_scales_s32[0][i] = ((1 << 18) + (dra_mapping->chroma_dra_scales_s32[0][i] >> 1)) / dra_mapping->chroma_dra_scales_s32[0][i];
        dra_mapping->chroma_inv_dra_scales_s32[1][i] = ((1 << 18) + (dra_mapping->chroma_dra_scales_s32[1][i] >> 1)) / dra_mapping->chroma_dra_scales_s32[1][i];
    }
}
void xevd_build_dra_luma_lut(DRA_CONTROL *dra_mapping)
{
    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        int rangeIdxY = xevd_get_dra_range_idx_gen(dra_mapping, i, dra_mapping->out_ranges_s32, dra_mapping->num_ranges);
        int value = i * dra_mapping->inv_dra_scales_s32[rangeIdxY];
        value = (dra_mapping->inv_dra_offsets_s32[rangeIdxY] + value + (1 << 8)) >> 9;
        value = XEVD_CLIP(value, 0, DRA_LUT_MAXSIZE - 1);
        dra_mapping->luma_inv_scale_lut[i] = value;
    }
}

void xevd_build_dra_chroma_lut(DRA_CONTROL *dra_mapping)
{
    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        dra_mapping->int_chroma_scale_lut[0][i] = dra_mapping->int_chroma_scale_lut[1][i] = 1;
        dra_mapping->int_chroma_inv_scale_lut[0][i] = dra_mapping->int_chroma_scale_lut[1][i] = 1;
    }
    {

        int chromaMultRanges2[33 + 1];
        int chromaMultScale[33 + 1];
        int chromaMultOffset[33 + 1];
        for (int ch = 0; ch < 2; ch++)
        {

            chromaMultRanges2[0] = dra_mapping->out_ranges_s32[0];
            chromaMultScale[0] = 0;
            chromaMultOffset[0] = (int)(dra_mapping->chroma_inv_dra_scales_s32[ch][0]);
            for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
            {
                chromaMultRanges2[i] = (int)((dra_mapping->out_ranges_s32[i - 1] + dra_mapping->out_ranges_s32[i]) / 2);
            }

            for (int i = 1; i < dra_mapping->num_ranges; i++)
            {
                int deltaRange = chromaMultRanges2[i + 1] - chromaMultRanges2[i];
                chromaMultOffset[i] = dra_mapping->chroma_inv_dra_scales_s32[ch][i - 1];
                int deltaScale = dra_mapping->chroma_inv_dra_scales_s32[ch][i] - chromaMultOffset[i];
                chromaMultScale[i] = (int)(((deltaScale << dra_mapping->internal_bd) + (deltaRange >> 1)) / deltaRange);

            }
            chromaMultScale[dra_mapping->num_ranges] = 0;
            chromaMultOffset[dra_mapping->num_ranges] = dra_mapping->chroma_inv_dra_scales_s32[ch][dra_mapping->num_ranges - 1];

            for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
            {
                int rangeIdx = xevd_get_dra_range_idx_gen(dra_mapping, i, chromaMultRanges2, dra_mapping->num_ranges + 1);
                int runI = i - chromaMultRanges2[rangeIdx];

                int runS = (chromaMultScale[rangeIdx] * runI + (1 << (dra_mapping->internal_bd - 1))) >> dra_mapping->internal_bd;

                dra_mapping->int_chroma_inv_scale_lut[ch][i] = chromaMultOffset[rangeIdx] + runS;
            }
        }
    }
}
void xevd_init_dra(DRA_CONTROL *dra_mapping, int bit_depth)
{
    xevd_get_signalled_params_dra(dra_mapping, bit_depth);
    xevd_construct_dra(dra_mapping);
    xevd_compensate_chroma_shift_table(dra_mapping);
    xevd_build_dra_luma_lut(dra_mapping);
    xevd_build_dra_chroma_lut(dra_mapping);
}

/* DRA applicaton (sample processing) functions are listed below: */
void xevd_apply_dra_luma_plane(XEVD_IMGB * dst, XEVD_IMGB * src, DRA_CONTROL *dra_mapping, int plane_id, int backward_map)
{
    short* src_plane;
    short* dst_plane;
    short src_value, dst_value;
    int i, k, j;

    for (i = plane_id; i <= plane_id; i++)
    {
        src_plane = (short*)src->a[i];
        dst_plane = (short*)dst->a[i];
        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                src_value = src_plane[k];

                dst_value = dst_plane[k];
                if (backward_map == TRUE)
                    dst_value = dra_mapping->luma_inv_scale_lut[src_value];
                else
                    dst_value = dra_mapping->luma_scale_lut[src_value];
                dst_plane[k] = dst_value;
            }
            src_plane = (short*)((unsigned char *)src_plane + src->s[i]);
            dst_plane = (short*)((unsigned char *)dst_plane + dst->s[i]);
        }
    }
}
void xevd_apply_dra_chroma_plane(XEVD_IMGB * dst, XEVD_IMGB * src, DRA_CONTROL *dra_mapping, int plane_id, int backward_map)
{
    int round_offset = 1 << (DRA_INVSCALE_NUMFBITS - 1);
    int offset_value = 0;
    int int_scale = 1;
    double scale = 0;

    short* ref_plane;
    short* src_plane;
    short* dst_plane;
    short ref_value, src_value, dst_value;
    int i, k, j;
    int c_shift = (plane_id == 0) ? 0 : 1;

    for (i = plane_id; i <= plane_id; i++)
    {
        ref_plane = (short*)src->a[0]; //luma reference
        src_plane = (short*)src->a[i];
        dst_plane = (short*)dst->a[i];

        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                ref_value = ref_plane[k << c_shift];
                ref_value = (ref_value < 0) ? 0 : ref_value;
                src_value = src_plane[k];
                dst_value = dst_plane[k];
                src_value = src_value - 512;
                offset_value = src_value;
                if (backward_map == TRUE)
                    int_scale = (dra_mapping->int_chroma_inv_scale_lut[i - 1][ref_value]);
                else
                    int_scale = (dra_mapping->int_chroma_scale_lut[i - 1][ref_value]);
                if (src_value < 0)
                {
                    offset_value *= -1;
                }
                offset_value = (offset_value * int_scale + round_offset) >> DRA_INVSCALE_NUMFBITS;
                if (src_value < 0)
                {
                    offset_value *= -1;
                }
                dst_value = 512 + offset_value;

                dst_plane[k] = dst_value;
            }
            ref_plane = (short*)((unsigned char *)ref_plane + (dst->s[0] << c_shift));
            src_plane = (short*)((unsigned char *)src_plane + src->s[i]);
            dst_plane = (short*)((unsigned char *)dst_plane + dst->s[i]);
        }
    }
}

/* DRA APS buffer functions are listed below: */
void xevd_reset_aps_gen_read_buffer(XEVD_APS_GEN *tmp_aps_gen_array)
{
    tmp_aps_gen_array[0].aps_type_id = 0; // ALF
    tmp_aps_gen_array[0].aps_id = -1;
    tmp_aps_gen_array[0].signal_flag = 0;

    tmp_aps_gen_array[1].aps_type_id = 1; // DRA
    tmp_aps_gen_array[1].aps_id = -1;
    tmp_aps_gen_array[1].signal_flag = 0;
}

void xevd_add_dra_aps_to_buffer(SIG_PARAM_DRA* tmp_dra_control_array, XEVD_APS_GEN *tmp_aps_gen_array)
{
    int dra_id = (tmp_aps_gen_array + 1)->aps_id;
    assert((dra_id >-2) && (dra_id < APS_MAX_NUM));
    if (dra_id != -1)
    {
        SIG_PARAM_DRA* dra_buffer = tmp_dra_control_array + dra_id;
        SIG_PARAM_DRA* dra_src = (SIG_PARAM_DRA*)((tmp_aps_gen_array + 1)->aps_data);
        if (dra_buffer->signal_dra_flag == -1)
        {
            xevd_mcpy(dra_buffer, dra_src, sizeof(SIG_PARAM_DRA));
            (tmp_aps_gen_array + 1)->aps_id = -1;
        }
        else
        {
            xevd_trace("New DRA APS information ignored. APS ID was used earlier, new APS entity must contain identical content.\n");
        }
    }
}
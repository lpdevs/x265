/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComSlice.h"
#include "x265.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <time.h>

#if HIGH_BIT_DEPTH
const int x265_bit_depth = 10;
#else
const int x265_bit_depth = 8;
#endif

void x265_log(x265_param_t *param, int level, const char *fmt, ...)
{
    if (level > param->logLevel)
        return;
    string log_level;
    switch (level)
    {
    case X265_LOG_ERROR:
        log_level = "error";
        break;
    case X265_LOG_WARNING:
        log_level = "warning";
        break;
    case X265_LOG_INFO:
        log_level = "info";
        break;
    case X265_LOG_DEBUG:
        log_level = "debug";
        break;
    default:
        log_level = "unknown";
        break;
    }

    fprintf(stderr, "x265 [%s]: ", log_level.c_str());
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}

extern "C"
void x265_param_default(x265_param_t *param)
{
    memset(param, 0, sizeof(x265_param_t));
    param->logLevel = X265_LOG_INFO;
    param->searchMethod = X265_STAR_SEARCH;
    param->gopNumThreads = 1;
    param->searchRange = 64;
    param->bipredSearchRange = 4;
    param->internalBitDepth = 8;
    param->maxCUSize = 64;
    param->tuQTMaxInterDepth = 3;
    param->tuQTMaxIntraDepth = 3;
    param->bEnableAMP = 0;
    param->bEnableRectInter = 1;
    param->bEnableRDO = 1;
    param->qp = 32;
    param->bEnableSAO = 0;
    param->saoLcuBasedOptimization = 1;
    param->maxNumMergeCand = 5u;
    param->bEnableSignHiding = 1;
    param->bEnableStrongIntraSmoothing = 1;
    param->bEnableRDOQ = 1;
    param->bEnableRDOQTS = 1;
    param->bEnableTransformSkip = 1;
    param->bEnableTSkipFast = 1;
}

extern "C"
int x265_param_apply_profile(x265_param_t *param, const char *profile)
{
    if (!profile)
        return 0;
    if (!strcmp(profile, "main"))
    {}
    else if (!strcmp(profile, "main10"))
    {
#if HIGH_BIT_DEPTH
        param->internalBitDepth = 10;
#else
        x265_log(param, X265_LOG_WARNING, "not compiled for 16bpp. Falling back to main profile.\n");
        return -1;
#endif
    }
    else if (!strcmp(profile, "mainstillpicture"))
    {
        param->keyframeInterval = 1;
    }
    else
    {
        x265_log(param, X265_LOG_ERROR, "unknown profile <%s>\n", profile);
        return -1;
    }

    return 0;
}

static inline int _confirm(x265_param_t *param, bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    x265_log(param, X265_LOG_ERROR, "%s\n", message);
    return 1;
}

int x265_check_params(x265_param_t *param)
{
#define CONFIRM(expr, msg) check_failed |= _confirm(param, expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMaxLog2Size = maxCUDepth + 2 - 1;
    uint32_t tuQTMinLog2Size = 2; //log2(4)

#if !HIGH_BIT_DEPTH
    CONFIRM(param->internalBitDepth != 8,
            "InternalBitDepth must be 8");
#endif
    CONFIRM(param->gopNumThreads < 1 || param->gopNumThreads > 32,
            "Number of GOP threads must be between 1 and 32");
    CONFIRM(param->qp < -6 * (param->internalBitDepth - 8) || param->qp > 51,
            "QP exceeds supported range (-QpBDOffsety to 51)");
    CONFIRM(param->frameRate <= 0,
            "Frame rate must be more than 1");
    CONFIRM(param->searchMethod<0 || param->searchMethod> X265_FULL_SEARCH,
            "Search method is not supported value (0:DIA 1:HEX 2:UMH 3:HM 4:ORIG 5:FULL)");
    CONFIRM(param->searchRange < 0,
            "Search Range must be more than 0");
    CONFIRM(param->searchRange >= 32768,
            "Search Range must be less than 32768");
    CONFIRM(param->bipredSearchRange < 0,
            "Search Range must be more than 0");
    CONFIRM(param->keyframeInterval < -1,
            "Keyframe interval must be -1 (open-GOP) 0 (auto) 1 (intra-only) or greater than 1");

    CONFIRM(param->cbQpOffset < -12, "Min. Chroma Cb QP Offset is -12");
    CONFIRM(param->cbQpOffset >  12, "Max. Chroma Cb QP Offset is  12");
    CONFIRM(param->crQpOffset < -12, "Min. Chroma Cr QP Offset is -12");
    CONFIRM(param->crQpOffset >  12, "Max. Chroma Cr QP Offset is  12");

    CONFIRM((param->maxCUSize >> maxCUDepth) < 4,
            "Minimum partition width size should be larger than or equal to 8");
    CONFIRM(param->maxCUSize < 16,
            "Maximum partition width size should be larger than or equal to 16");
    CONFIRM((param->sourceWidth  % (param->maxCUSize >> (maxCUDepth - 1))) != 0,
            "Resulting coded frame width must be a multiple of the minimum CU size");
    CONFIRM((param->sourceHeight % (param->maxCUSize >> (maxCUDepth - 1))) != 0,
            "Resulting coded frame height must be a multiple of the minimum CU size");

    CONFIRM((1u << tuQTMaxLog2Size) > param->maxCUSize,
            "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");

    CONFIRM(param->tuQTMaxInterDepth < 1,
            "QuadtreeTUMaxDepthInter must be greater than or equal to 1");
    CONFIRM(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxInterDepth - 1)),
            "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CONFIRM(param->tuQTMaxIntraDepth < 1,
            "QuadtreeTUMaxDepthIntra must be greater than or equal to 1");
    CONFIRM(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxIntraDepth - 1)),
            "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");

    CONFIRM(param->maxNumMergeCand < 1, "MaxNumMergeCand must be 1 or greater.");
    CONFIRM(param->maxNumMergeCand > 5, "MaxNumMergeCand must be 5 or smaller.");

    //TODO:ChromaFmt assumes 4:2:0 below
    CONFIRM(param->sourceWidth  % TComSPS::getWinUnitX(CHROMA_420) != 0,
            "Picture width must be an integer multiple of the specified chroma subsampling");
    CONFIRM(param->sourceHeight % TComSPS::getWinUnitY(CHROMA_420) != 0,
            "Picture height must be an integer multiple of the specified chroma subsampling");

    // max CU size should be power of 2
    uint32_t ui = param->maxCUSize;
    while (ui)
    {
        ui >>= 1;
        if ((ui & 1) == 1)
            CONFIRM(ui != 1, "Width should be 2^n");
    }

    CONFIRM(param->bEnableWavefront < 0, "WaveFrontSynchro cannot be negative");

    return check_failed;
}

void x265_set_globals(x265_param_t *param)
{
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    // set max CU width & height
    g_maxCUWidth  = param->maxCUSize;
    g_maxCUHeight = param->maxCUSize;

    // compute actual CU depth with respect to config depth and max transform size
    g_addCUDepth  = 0;
    while ((param->maxCUSize >> maxCUDepth) > (1u << (tuQTMinLog2Size + g_addCUDepth)))
    {
        g_addCUDepth++;
    }

    maxCUDepth += g_addCUDepth;
    g_addCUDepth++;
    g_maxCUDepth = maxCUDepth;

    // set internal bit-depth and constants
    g_bitDepthY = param->internalBitDepth;

    // initialize partition order
    UInt* tmp = &g_zscanToRaster[0];
    initZscanToRaster(g_maxCUDepth + 1, 1, 0, tmp);
    initRasterToZscan(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);

    // initialize conversion matrix from partition index to pel
    initRasterToPelXY(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);
}

void x265_print_params(x265_param_t *param)
{
    if (param->logLevel < X265_LOG_INFO)
        return;
#if HIGH_BIT_DEPTH
    x265_log(param, X265_LOG_INFO, "Internal bit depth           : %d\n", param->internalBitDepth);
#endif
    x265_log(param, X265_LOG_INFO, "CU size                      : %d\n", param->maxCUSize);
    x265_log(param, X265_LOG_INFO, "Max RQT depth inter / intra  : %d / %d\n", param->tuQTMaxInterDepth, param->tuQTMaxIntraDepth);

    x265_log(param, X265_LOG_INFO, "ME method / range / maxmerge : %s / %d / %d\n",
             x265_motion_est_names[param->searchMethod], param->searchRange, param->maxNumMergeCand);
    x265_log(param, X265_LOG_INFO, "Keyframe Interval            : %d\n", param->keyframeInterval);
    if (param->bEnableWavefront)
    {
        x265_log(param, X265_LOG_INFO, "WaveFrontSubstreams          : %d\n", (param->sourceHeight + param->maxCUSize - 1) / param->maxCUSize);
    }
    x265_log(param, X265_LOG_INFO, "QP                           : %d\n", param->qp);
    if (param->cbQpOffset || param->crQpOffset)
    {
        x265_log(param, X265_LOG_INFO, "Cb/Cr QP Offset              : %d / %d\n", param->cbQpOffset, param->crQpOffset);
    }
    if (param->rdPenalty)
    {
        x265_log(param, X265_LOG_INFO, "RDpenalty                    : %d\n", param->rdPenalty);
    }
    x265_log(param, X265_LOG_INFO, "enabled coding tools: ");
#define TOOLOPT(FLAG, STR) if (FLAG) fprintf(stderr, "%s ", STR)
    TOOLOPT(param->bEnableRectInter, "rect");
    TOOLOPT(param->bEnableAMP, "amp");
    TOOLOPT(param->bEnableCbfFastMode, "cfm");
    TOOLOPT(param->bEnableConstrainedIntra, "cip");
    TOOLOPT(param->bEnableEarlySkip, "esd");
    if (param->bEnableRDO)
        fprintf(stderr, "rdo ");
    else
        fprintf(stderr, "no-rdo ");
    TOOLOPT(param->bEnableRDOQ, "rdoq");
    if (param->bEnableSAO)
    {
        TOOLOPT(param->bEnableSAO, "sao");
        TOOLOPT(param->saoLcuBasedOptimization, "sao-lcu");
    }
    TOOLOPT(param->bEnableSignHiding, "sign-hide");
    if (param->bEnableTransformSkip)
    {
        TOOLOPT(param->bEnableTransformSkip, "tskip");
        TOOLOPT(param->bEnableTSkipFast, "tskip-fast");
        TOOLOPT(param->bEnableRDOQTS, "rdoqts");
    }
    TOOLOPT(param->bEnableWeightedPred, "weightp");
    TOOLOPT(param->bEnableWeightedBiPred, "weightbp");
    fprintf(stderr, "\n");
    fflush(stderr);
}

int64_t x265_mdate(void)
{
#if _WIN32
    struct timeb tb;
    ftime(&tb);
    return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
#else
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}

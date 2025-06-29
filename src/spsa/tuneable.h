#pragma once

#include "search/history.h"
#include <array>
#include <cmath>
#include <cstddef>

// TODO: bring all search constants under here, and leverage uci options

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

TUNEABLE_CONSTANT float LMR_constant = 1.19;
TUNEABLE_CONSTANT float LMR_depth_coeff = -1.48;
TUNEABLE_CONSTANT float LMR_move_coeff = 1.91;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = 0.12;

inline auto Initialise_LMR_reduction()
{
    std::array<std::array<int, 64>, 64> ret = {};

    for (size_t i = 0; i < ret.size(); i++)
    {
        for (size_t j = 0; j < ret[i].size(); j++)
        {
            ret[i][j] = static_cast<int>(std::round(LMR_constant + LMR_depth_coeff * log(i + 1)
                + LMR_move_coeff * log(j + 1) + LMR_depth_move_coeff * log(i + 1) * log(j + 1)));
        }
    }

    return ret;
};

// [depth][move number]
TUNEABLE_CONSTANT std::array<std::array<int, 64>, 64> LMR_reduction = Initialise_LMR_reduction();

TUNEABLE_CONSTANT int aspiration_window_size = 12;

TUNEABLE_CONSTANT int nmp_const = 4;
TUNEABLE_CONSTANT int nmp_d = 6;
TUNEABLE_CONSTANT int nmp_s = 245;

TUNEABLE_CONSTANT int se_d = 56;
TUNEABLE_CONSTANT int se_de = 11;

TUNEABLE_CONSTANT int lmr_h = 7844;

TUNEABLE_CONSTANT int fifty_mr_scale = 288;

TUNEABLE_CONSTANT int rfp_max_d = 8;
TUNEABLE_CONSTANT int rfp_d = 93;

TUNEABLE_CONSTANT int lmp_max_d = 6;
TUNEABLE_CONSTANT int lmp_const = 3;
TUNEABLE_CONSTANT int lmp_depth = 3;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 31;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 11;

TUNEABLE_CONSTANT int see_d = 111;
TUNEABLE_CONSTANT int see_h = 168;

TUNEABLE_CONSTANT int se_max_d = 7;
TUNEABLE_CONSTANT int see_tt_d = 3;

TUNEABLE_CONSTANT int delta_c = 280;

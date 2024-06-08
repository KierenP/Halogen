#pragma once

#include "Score.h"
#include <array>
#include <cmath>
#include <cstddef>

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

TUNEABLE_CONSTANT Score aspiration_width = 14;

TUNEABLE_CONSTANT int nmp_constant = 4;
TUNEABLE_CONSTANT int nmp_depth_quotent = 6;
TUNEABLE_CONSTANT int nmp_score_max = 3;
TUNEABLE_CONSTANT int nmp_score_quotent = 252;

TUNEABLE_CONSTANT int se_sbeta_depth = 64;
TUNEABLE_CONSTANT Score se_double_margin = 10;
TUNEABLE_CONSTANT int se_multi_extend = 8;
TUNEABLE_CONSTANT int se_depth_limit = 6;
TUNEABLE_CONSTANT int se_tt_depth = 2;

TUNEABLE_CONSTANT int lmr_history = 3913;

TUNEABLE_CONSTANT Score snmp_margin = 112;
TUNEABLE_CONSTANT int snmp_depth = 8;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int lmp_constant = 7;
TUNEABLE_CONSTANT int lmp_depth_coeff = 7;
TUNEABLE_CONSTANT int lmp_depth_limit = 6;

TUNEABLE_CONSTANT Score futility_const = 31;
TUNEABLE_CONSTANT int futility_linear = 13;
TUNEABLE_CONSTANT int futility_quad = 14;
TUNEABLE_CONSTANT int futility_depth_limit = 8;

TUNEABLE_CONSTANT Score delta_margin = 222;

TUNEABLE_CONSTANT double LMR_constant = 1.18;
TUNEABLE_CONSTANT double LMR_depth_coeff = -1.57;
TUNEABLE_CONSTANT double LMR_move_coeff = 1.05;
TUNEABLE_CONSTANT double LMR_depth_move_coeff = 0.53;

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

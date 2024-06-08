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

TUNEABLE_CONSTANT Score aspiration_width = 15;

TUNEABLE_CONSTANT int nmp_score_quotent = 250;

TUNEABLE_CONSTANT Score se_double_margin = 16;

TUNEABLE_CONSTANT int lmr_history = 4096;

TUNEABLE_CONSTANT Score snmp_margin = 119;

TUNEABLE_CONSTANT int lmp_constant = 11;

TUNEABLE_CONSTANT Score futility_const = 27;
TUNEABLE_CONSTANT int futility_linear = 13;
TUNEABLE_CONSTANT int futility_quad = 16;

TUNEABLE_CONSTANT Score delta_margin = 200;

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

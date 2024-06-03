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

TUNEABLE_CONSTANT double LMR_constant = 1.05;
TUNEABLE_CONSTANT double LMR_depth_coeff = -2.06;
TUNEABLE_CONSTANT double LMR_move_coeff = 0.20;
TUNEABLE_CONSTANT double LMR_depth_move_coeff = 1.17;

TUNEABLE_CONSTANT int Null_constant = 4;
TUNEABLE_CONSTANT int Null_depth_quotent = 6;
TUNEABLE_CONSTANT int Null_beta_quotent = 250;

TUNEABLE_CONSTANT int Futility_constant = 27;
TUNEABLE_CONSTANT int Futility_linear = 13;
TUNEABLE_CONSTANT int Futility_quad = 16;
TUNEABLE_CONSTANT int Futility_depth = 8;

TUNEABLE_CONSTANT Score aspiration_window_mid_width = 15;

TUNEABLE_CONSTANT int Delta_margin = 200;

TUNEABLE_CONSTANT int SNMP_coeff = 119;
TUNEABLE_CONSTANT int SNMP_depth = 8;

TUNEABLE_CONSTANT int LMP_constant = 11;
TUNEABLE_CONSTANT int LMP_coeff = 7;
TUNEABLE_CONSTANT int LMP_depth = 6;

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

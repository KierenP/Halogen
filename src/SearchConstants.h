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

TUNEABLE_CONSTANT Score aspiration_width = 0;

TUNEABLE_CONSTANT int nmp_constant {};
TUNEABLE_CONSTANT int nmp_depth_quotent {};
TUNEABLE_CONSTANT int nmp_score_max {};
TUNEABLE_CONSTANT int nmp_score_quotent {};

TUNEABLE_CONSTANT int se_sbeta_depth {};
TUNEABLE_CONSTANT Score se_double_margin = 0;
TUNEABLE_CONSTANT int se_multi_extend {};
TUNEABLE_CONSTANT int se_depth_limit {};
TUNEABLE_CONSTANT int se_tt_depth {};

TUNEABLE_CONSTANT int lmr_history {};

TUNEABLE_CONSTANT Score snmp_margin = 0;
TUNEABLE_CONSTANT int snmp_depth {};

TUNEABLE_CONSTANT int iid_depth {};

TUNEABLE_CONSTANT int lmp_constant {};
TUNEABLE_CONSTANT int lmp_depth_coeff {};
TUNEABLE_CONSTANT int lmp_depth_limit {};

TUNEABLE_CONSTANT Score futility_const = 0;
TUNEABLE_CONSTANT int futility_linear {};
TUNEABLE_CONSTANT int futility_quad {};
TUNEABLE_CONSTANT int futility_depth_limit {};

TUNEABLE_CONSTANT Score delta_margin = 0;

TUNEABLE_CONSTANT double LMR_constant {};
TUNEABLE_CONSTANT double LMR_depth_coeff {};
TUNEABLE_CONSTANT double LMR_move_coeff {};
TUNEABLE_CONSTANT double LMR_depth_move_coeff {};

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

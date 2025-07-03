#pragma once

#include <array>
#include <cmath>
#include <cstddef>

// TODO: bring all search constants under here, and leverage uci options

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

TUNEABLE_CONSTANT float LMR_constant = 0.5965;
TUNEABLE_CONSTANT float LMR_depth_coeff = -0.6058;
TUNEABLE_CONSTANT float LMR_move_coeff = 1.554;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = 0.03265;

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

TUNEABLE_CONSTANT int nmp_const = 5;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 246;

TUNEABLE_CONSTANT int se_d = 52;
TUNEABLE_CONSTANT int se_de = 12;

TUNEABLE_CONSTANT int lmr_h = 6706;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 272;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 256;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 85;

TUNEABLE_CONSTANT int lmp_max_d = 6;
TUNEABLE_CONSTANT int lmp_const = 2;
TUNEABLE_CONSTANT int lmp_depth = 3;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 33;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 11;

TUNEABLE_CONSTANT int see_d = 114;
TUNEABLE_CONSTANT int see_h = 171;

TUNEABLE_CONSTANT int se_max_d = 7;
TUNEABLE_CONSTANT int see_tt_d = 4;

TUNEABLE_CONSTANT int delta_c = 285;

TUNEABLE_CONSTANT int eval_scale_knight = 474;
TUNEABLE_CONSTANT int eval_scale_bishop = 446;
TUNEABLE_CONSTANT int eval_scale_rook = 730;
TUNEABLE_CONSTANT int eval_scale_queen = 1223;
TUNEABLE_CONSTANT int eval_scale_const = 25774;

TUNEABLE_CONSTANT std::array see_values = { 97, 501, 529, 802, 1248, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.5313;

TUNEABLE_CONSTANT float node_tm_base = 0.5118;
TUNEABLE_CONSTANT float node_tm_scale = 2.041;
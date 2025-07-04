#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#define TUNE

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

TUNEABLE_CONSTANT float LMR_constant = 0.5813;
TUNEABLE_CONSTANT float LMR_depth_coeff = -0.5768;
TUNEABLE_CONSTANT float LMR_move_coeff = 1.631;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = 0.01007;

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
TUNEABLE_CONSTANT int nmp_s = 248;

TUNEABLE_CONSTANT int se_d = 50;
TUNEABLE_CONSTANT int se_de = 12;

TUNEABLE_CONSTANT int lmr_h = 6611;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 272;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 258;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 84;

TUNEABLE_CONSTANT int lmp_max_d = 6;
TUNEABLE_CONSTANT int lmp_const = 2;
TUNEABLE_CONSTANT int lmp_depth = 3;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 33;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 11;

TUNEABLE_CONSTANT int see_d = 114;
TUNEABLE_CONSTANT int see_h = 172;

TUNEABLE_CONSTANT int se_max_d = 6;
TUNEABLE_CONSTANT int see_tt_d = 4;

TUNEABLE_CONSTANT int delta_c = 292;

TUNEABLE_CONSTANT int eval_scale_knight = 463;
TUNEABLE_CONSTANT int eval_scale_bishop = 443;
TUNEABLE_CONSTANT int eval_scale_rook = 724;
TUNEABLE_CONSTANT int eval_scale_queen = 1198;
TUNEABLE_CONSTANT int eval_scale_const = 26050;

TUNEABLE_CONSTANT std::array see_values = { 99, 503, 530, 797, 1244, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.5313;
TUNEABLE_CONSTANT float node_tm_base = 0.5159;
TUNEABLE_CONSTANT float node_tm_scale = 2.098;
TUNEABLE_CONSTANT int blitz_tc_a = 40;
TUNEABLE_CONSTANT int blitz_tc_b = 1200;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;
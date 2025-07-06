#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

TUNEABLE_CONSTANT float LMR_constant = 0.3356;
TUNEABLE_CONSTANT float LMR_depth_coeff = -0.3232;
TUNEABLE_CONSTANT float LMR_move_coeff = 1.923;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.2143;

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

TUNEABLE_CONSTANT int aspiration_window_size = 11;

TUNEABLE_CONSTANT int nmp_const = 6;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 250;

TUNEABLE_CONSTANT int se_d = 51;
TUNEABLE_CONSTANT int se_de = 12;

TUNEABLE_CONSTANT int lmr_h = 6526;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 278;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 251;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 78;

TUNEABLE_CONSTANT int lmp_max_d = 6;
TUNEABLE_CONSTANT int lmp_const = 3;
TUNEABLE_CONSTANT int lmp_depth = 4;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 34;
TUNEABLE_CONSTANT int fp_depth = 12;
TUNEABLE_CONSTANT int fp_quad = 11;

TUNEABLE_CONSTANT int see_d = 108;
TUNEABLE_CONSTANT int see_h = 172;

TUNEABLE_CONSTANT int se_max_d = 6;
TUNEABLE_CONSTANT int see_tt_d = 4;

TUNEABLE_CONSTANT int delta_c = 296;

TUNEABLE_CONSTANT int eval_scale_knight = 497;
TUNEABLE_CONSTANT int eval_scale_bishop = 463;
TUNEABLE_CONSTANT int eval_scale_rook = 736;
TUNEABLE_CONSTANT int eval_scale_queen = 1265;
TUNEABLE_CONSTANT int eval_scale_const = 26757;

TUNEABLE_CONSTANT std::array see_values = { 108, 483, 504, 739, 1216, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.5414;
TUNEABLE_CONSTANT float node_tm_base = 0.5420;
TUNEABLE_CONSTANT float node_tm_scale = 2.283;
TUNEABLE_CONSTANT int blitz_tc_a = 47;
TUNEABLE_CONSTANT int blitz_tc_b = 985;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;
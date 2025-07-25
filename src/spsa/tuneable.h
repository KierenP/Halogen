#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

constexpr inline int LMR_SCALE = 1024;

TUNEABLE_CONSTANT float LMR_constant = -1.621;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.149;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.840;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7957;

inline auto Initialise_LMR_reduction()
{
    std::array<std::array<int, 64>, 64> ret = {};

    for (size_t i = 0; i < ret.size(); i++)
    {
        for (size_t j = 0; j < ret[i].size(); j++)
        {
            auto lmr = LMR_constant + LMR_depth_coeff * log(i + 1) + LMR_move_coeff * log(j + 1)
                + LMR_depth_move_coeff * log(i + 1) * log(j + 1);
            ret[i][j] = static_cast<int>(std::round(lmr * LMR_SCALE));
        }
    }

    return ret;
};

// [depth][move number]
TUNEABLE_CONSTANT std::array<std::array<int, 64>, 64> LMR_reduction = Initialise_LMR_reduction();

TUNEABLE_CONSTANT int aspiration_window_size = 9;

TUNEABLE_CONSTANT int nmp_const = 6;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 245;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 39;
TUNEABLE_CONSTANT int se_double = 6;
TUNEABLE_CONSTANT int se_double_pv = 456;
TUNEABLE_CONSTANT int se_double_hd = 286;
TUNEABLE_CONSTANT int se_double_quiet = 3;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1275;
TUNEABLE_CONSTANT int lmr_cut = 1188;
TUNEABLE_CONSTANT int lmr_improving = 1186;
TUNEABLE_CONSTANT int lmr_loud = 756;
TUNEABLE_CONSTANT int lmr_h = 2642;
TUNEABLE_CONSTANT int lmr_offset = 424;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 323;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 233;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 54;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 6;
TUNEABLE_CONSTANT int lmp_depth = 6;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 39;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 14;

TUNEABLE_CONSTANT int see_quiet_depth = 111;
TUNEABLE_CONSTANT int see_quiet_hist = 161;
TUNEABLE_CONSTANT int see_loud_depth = 36;
TUNEABLE_CONSTANT int see_loud_hist = 153;
TUNEABLE_CONSTANT int see_max_depth = 7;

TUNEABLE_CONSTANT int hist_prune_depth = 3388;
TUNEABLE_CONSTANT int hist_prune = 404;

TUNEABLE_CONSTANT int delta_c = 377;

TUNEABLE_CONSTANT int eval_scale_knight = 565;
TUNEABLE_CONSTANT int eval_scale_bishop = 450;
TUNEABLE_CONSTANT int eval_scale_rook = 675;
TUNEABLE_CONSTANT int eval_scale_queen = 1642;
TUNEABLE_CONSTANT int eval_scale_const = 23278;

TUNEABLE_CONSTANT std::array see_values = { 110, 471, 452, 810, 1489, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3484;
TUNEABLE_CONSTANT float node_tm_base = 0.5283;
TUNEABLE_CONSTANT float node_tm_scale = 2.352;
TUNEABLE_CONSTANT int blitz_tc_a = 48;
TUNEABLE_CONSTANT int blitz_tc_b = 535;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 11;
TUNEABLE_CONSTANT int history_bonus_depth = 3;
TUNEABLE_CONSTANT int history_bonus_quad = 72;

TUNEABLE_CONSTANT int history_penalty_const = 14;
TUNEABLE_CONSTANT int history_penalty_depth = 2;
TUNEABLE_CONSTANT int history_penalty_quad = 56;
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

TUNEABLE_CONSTANT float LMR_constant = -1.548;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.150;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.691;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7976;

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
TUNEABLE_CONSTANT int nmp_s = 243;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 41;
TUNEABLE_CONSTANT int se_double = 7;
TUNEABLE_CONSTANT int se_double_pv = 462;
TUNEABLE_CONSTANT int se_double_hd = 276;
TUNEABLE_CONSTANT int se_double_quiet = 2;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1319;
TUNEABLE_CONSTANT int lmr_cut = 1238;
TUNEABLE_CONSTANT int lmr_improving = 1176;
TUNEABLE_CONSTANT int lmr_loud = 755;
TUNEABLE_CONSTANT int lmr_h = 2638;
TUNEABLE_CONSTANT int lmr_offset = 456;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 325;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 234;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 53;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 6;
TUNEABLE_CONSTANT int lmp_depth = 6;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 38;
TUNEABLE_CONSTANT int fp_depth = 14;
TUNEABLE_CONSTANT int fp_quad = 14;

TUNEABLE_CONSTANT int see_quiet_depth = 109;
TUNEABLE_CONSTANT int see_quiet_hist = 155;
TUNEABLE_CONSTANT int see_loud_depth = 37;
TUNEABLE_CONSTANT int see_loud_hist = 158;
TUNEABLE_CONSTANT int see_max_depth = 7;

TUNEABLE_CONSTANT int hist_prune_depth = 3324;
TUNEABLE_CONSTANT int hist_prune = 344;

TUNEABLE_CONSTANT int delta_c = 367;

TUNEABLE_CONSTANT int eval_scale_knight = 564;
TUNEABLE_CONSTANT int eval_scale_bishop = 460;
TUNEABLE_CONSTANT int eval_scale_rook = 681;
TUNEABLE_CONSTANT int eval_scale_queen = 1645;
TUNEABLE_CONSTANT int eval_scale_const = 23008;

TUNEABLE_CONSTANT std::array see_values = { 112, 452, 452, 817, 1553, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2957;
TUNEABLE_CONSTANT float node_tm_base = 0.5052;
TUNEABLE_CONSTANT float node_tm_scale = 2.329;
TUNEABLE_CONSTANT int blitz_tc_a = 49;
TUNEABLE_CONSTANT int blitz_tc_b = 500;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 14;
TUNEABLE_CONSTANT int history_bonus_depth = 2;
TUNEABLE_CONSTANT int history_bonus_quad = 76;

TUNEABLE_CONSTANT int history_penalty_const = 16;
TUNEABLE_CONSTANT int history_penalty_depth = 3;
TUNEABLE_CONSTANT int history_penalty_quad = 54;
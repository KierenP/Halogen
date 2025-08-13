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

TUNEABLE_CONSTANT float LMR_constant = -1.583;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.428;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.522;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.8144;

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

TUNEABLE_CONSTANT int se_sbeta_depth = 45;
TUNEABLE_CONSTANT int se_double = 7;
TUNEABLE_CONSTANT int se_double_pv = 489;
TUNEABLE_CONSTANT int se_double_hd = 315;
TUNEABLE_CONSTANT int se_double_quiet = 1;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 3;

TUNEABLE_CONSTANT int lmr_pv = 1337;
TUNEABLE_CONSTANT int lmr_cut = 1379;
TUNEABLE_CONSTANT int lmr_improving = 1082;
TUNEABLE_CONSTANT int lmr_loud = 793;
TUNEABLE_CONSTANT int lmr_h = 2531;
TUNEABLE_CONSTANT int lmr_offset = 396;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 308;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 205;

TUNEABLE_CONSTANT int rfp_max_d = 10;
TUNEABLE_CONSTANT int rfp_const = -164;
TUNEABLE_CONSTANT int rfp_depth = 2950;
TUNEABLE_CONSTANT int rfp_quad = 29;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 396;
TUNEABLE_CONSTANT int lmp_depth = 392;
TUNEABLE_CONSTANT int lmp_quad = 15;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 2443;
TUNEABLE_CONSTANT int fp_depth = 841;
TUNEABLE_CONSTANT int fp_quad = 751;

TUNEABLE_CONSTANT int see_quiet_depth = 106;
TUNEABLE_CONSTANT int see_quiet_hist = 153;
TUNEABLE_CONSTANT int see_loud_depth = 40;
TUNEABLE_CONSTANT int see_loud_hist = 154;
TUNEABLE_CONSTANT int see_max_depth = 8;

TUNEABLE_CONSTANT int hist_prune_depth = 2256;
TUNEABLE_CONSTANT int hist_prune = 256;

TUNEABLE_CONSTANT int delta_c = 448;

TUNEABLE_CONSTANT std::array eval_scale = { 0, 573, 461, 682, 1791 };
TUNEABLE_CONSTANT int eval_scale_const = 20852;

TUNEABLE_CONSTANT std::array see_values = { 126, 493, 438, 760, 1661, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2608;
TUNEABLE_CONSTANT float node_tm_base = 0.4334;
TUNEABLE_CONSTANT float node_tm_scale = 2.484;
TUNEABLE_CONSTANT int blitz_tc_a = 46;
TUNEABLE_CONSTANT int blitz_tc_b = 337;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 840;
TUNEABLE_CONSTANT int history_bonus_depth = 51;
TUNEABLE_CONSTANT int history_bonus_quad = 80;

TUNEABLE_CONSTANT int history_penalty_const = 1232;
TUNEABLE_CONSTANT int history_penalty_depth = -10;
TUNEABLE_CONSTANT int history_penalty_quad = 40;

TUNEABLE_CONSTANT int good_loud_see = 100;
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

TUNEABLE_CONSTANT float LMR_constant = -1.864;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.414;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.508;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.8280;

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

TUNEABLE_CONSTANT int aspiration_window_size = 8;

TUNEABLE_CONSTANT int nmp_const = 5;
TUNEABLE_CONSTANT int nmp_d = 5;
TUNEABLE_CONSTANT int nmp_s = 249;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 53;
TUNEABLE_CONSTANT int se_double = 6;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1522;
TUNEABLE_CONSTANT int lmr_cut = 1473;
TUNEABLE_CONSTANT int lmr_improving = 902;
TUNEABLE_CONSTANT int lmr_loud = 818;
TUNEABLE_CONSTANT int lmr_h = 2516;
TUNEABLE_CONSTANT int lmr_offset = 502;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 319;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 210;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_const = 14;
TUNEABLE_CONSTANT int rfp_depth = 2399;
TUNEABLE_CONSTANT int rfp_quad = 12;
TUNEABLE_CONSTANT int rfp_threat = 44;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 418;
TUNEABLE_CONSTANT int lmp_depth = 397;
TUNEABLE_CONSTANT int lmp_quad = 30;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 2246;
TUNEABLE_CONSTANT int fp_depth = 958;
TUNEABLE_CONSTANT int fp_quad = 736;

TUNEABLE_CONSTANT int see_quiet_depth = 119;
TUNEABLE_CONSTANT int see_quiet_hist = 134;
TUNEABLE_CONSTANT int see_loud_depth = 39;
TUNEABLE_CONSTANT int see_loud_hist = 126;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 2553;
TUNEABLE_CONSTANT int hist_prune = 213;

TUNEABLE_CONSTANT int delta_c = 539;

TUNEABLE_CONSTANT std::array eval_scale = { -57, 656, 516, 727, 1903 };
TUNEABLE_CONSTANT int eval_scale_const = 16657;

TUNEABLE_CONSTANT std::array see_values = { 139, 472, 408, 747, 1576, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2244;
TUNEABLE_CONSTANT float node_tm_base = 0.3818;
TUNEABLE_CONSTANT float node_tm_scale = 2.333;
TUNEABLE_CONSTANT int blitz_tc_a = 49;
TUNEABLE_CONSTANT int blitz_tc_b = 318;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 714;
TUNEABLE_CONSTANT int history_bonus_depth = 539;
TUNEABLE_CONSTANT int history_bonus_quad = 104;

TUNEABLE_CONSTANT int history_penalty_const = 1483;
TUNEABLE_CONSTANT int history_penalty_depth = 187;
TUNEABLE_CONSTANT int history_penalty_quad = 41;

TUNEABLE_CONSTANT int good_loud_see = 96;
TUNEABLE_CONSTANT int good_loud_see_hist = 42;

TUNEABLE_CONSTANT int tt_replace_self_depth = 45;
TUNEABLE_CONSTANT int tt_replace_depth = 56;
TUNEABLE_CONSTANT int tt_replace_age = 236;
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
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.502;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.625;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7820;

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
TUNEABLE_CONSTANT int nmp_s = 253;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 1;

TUNEABLE_CONSTANT int se_sbeta_depth = 51;
TUNEABLE_CONSTANT int se_double = 3;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1396;
TUNEABLE_CONSTANT int lmr_cut = 1691;
TUNEABLE_CONSTANT int lmr_improving = 1015;
TUNEABLE_CONSTANT int lmr_loud = 703;
TUNEABLE_CONSTANT int lmr_h = 2284;
TUNEABLE_CONSTANT int lmr_offset = 484;
TUNEABLE_CONSTANT int lmr_shallower = 10;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 0;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 291;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 209;

TUNEABLE_CONSTANT int rfp_max_d = 8;
TUNEABLE_CONSTANT int rfp_const = -135;
TUNEABLE_CONSTANT int rfp_depth = 2076;
TUNEABLE_CONSTANT int rfp_quad = 32;
TUNEABLE_CONSTANT int rfp_threat = 56;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 408;
TUNEABLE_CONSTANT int lmp_depth = 435;
TUNEABLE_CONSTANT int lmp_quad = 15;

TUNEABLE_CONSTANT int fp_max_d = 11;
TUNEABLE_CONSTANT int fp_const = 2329;
TUNEABLE_CONSTANT int fp_depth = 929;
TUNEABLE_CONSTANT int fp_quad = 610;

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 124;
TUNEABLE_CONSTANT int see_loud_depth = 43;
TUNEABLE_CONSTANT int see_loud_hist = 134;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 1745;
TUNEABLE_CONSTANT int hist_prune = 718;

TUNEABLE_CONSTANT std::array eval_scale = { 31, 526, 522, 574, 1652 };
TUNEABLE_CONSTANT int eval_scale_const = 16896;

TUNEABLE_CONSTANT std::array see_values = { 134, 438, 480, 792, 1885, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2879;
TUNEABLE_CONSTANT float node_tm_base = 0.3657;
TUNEABLE_CONSTANT float node_tm_scale = 2.835;
TUNEABLE_CONSTANT int blitz_tc_a = 50;
TUNEABLE_CONSTANT int blitz_tc_b = 268;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 877;
TUNEABLE_CONSTANT int history_bonus_depth = -11;
TUNEABLE_CONSTANT int history_bonus_quad = 101;

TUNEABLE_CONSTANT int history_penalty_const = 1546;
TUNEABLE_CONSTANT int history_penalty_depth = -33;
TUNEABLE_CONSTANT int history_penalty_quad = 33;

TUNEABLE_CONSTANT int good_loud_see = 71;
TUNEABLE_CONSTANT int good_loud_see_hist = 55;

TUNEABLE_CONSTANT int tt_replace_self_depth = 5;
TUNEABLE_CONSTANT int tt_replace_depth = 54;
TUNEABLE_CONSTANT int tt_replace_age = 237;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 171;

TUNEABLE_CONSTANT int probcut_beta = 214;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 400;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 4;
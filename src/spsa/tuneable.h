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

TUNEABLE_CONSTANT float LMR_constant = -1.843;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.530;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.508;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7873;

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
TUNEABLE_CONSTANT int nmp_s = 256;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 1;

TUNEABLE_CONSTANT int se_sbeta_depth = 53;
TUNEABLE_CONSTANT int se_double = 5;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1421;
TUNEABLE_CONSTANT int lmr_cut = 1589;
TUNEABLE_CONSTANT int lmr_improving = 1103;
TUNEABLE_CONSTANT int lmr_loud = 747;
TUNEABLE_CONSTANT int lmr_h = 2324;
TUNEABLE_CONSTANT int lmr_offset = 509;
TUNEABLE_CONSTANT int lmr_shallower = 10;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 302;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 217;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_const = -202;
TUNEABLE_CONSTANT int rfp_depth = 2433;
TUNEABLE_CONSTANT int rfp_quad = 17;
TUNEABLE_CONSTANT int rfp_threat = 55;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 376;
TUNEABLE_CONSTANT int lmp_depth = 392;
TUNEABLE_CONSTANT int lmp_quad = 29;

TUNEABLE_CONSTANT int fp_max_d = 11;
TUNEABLE_CONSTANT int fp_const = 2354;
TUNEABLE_CONSTANT int fp_depth = 976;
TUNEABLE_CONSTANT int fp_quad = 642;

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 122;
TUNEABLE_CONSTANT int see_loud_depth = 44;
TUNEABLE_CONSTANT int see_loud_hist = 138;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 1981;
TUNEABLE_CONSTANT int hist_prune = 447;

TUNEABLE_CONSTANT std::array eval_scale = { 27, 539, 507, 620, 1650 };
TUNEABLE_CONSTANT int eval_scale_const = 17011;

TUNEABLE_CONSTANT std::array see_values = { 136, 444, 479, 832, 1851, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2378;
TUNEABLE_CONSTANT float node_tm_base = 0.3673;
TUNEABLE_CONSTANT float node_tm_scale = 2.743;
TUNEABLE_CONSTANT int blitz_tc_a = 46;
TUNEABLE_CONSTANT int blitz_tc_b = 300;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 910;
TUNEABLE_CONSTANT int history_bonus_depth = -12;
TUNEABLE_CONSTANT int history_bonus_quad = 94;

TUNEABLE_CONSTANT int history_penalty_const = 1404;
TUNEABLE_CONSTANT int history_penalty_depth = -80;
TUNEABLE_CONSTANT int history_penalty_quad = 35;

TUNEABLE_CONSTANT int good_loud_see = 80;
TUNEABLE_CONSTANT int good_loud_see_hist = 52;

TUNEABLE_CONSTANT int tt_replace_self_depth = 4;
TUNEABLE_CONSTANT int tt_replace_depth = 55;
TUNEABLE_CONSTANT int tt_replace_age = 268;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 179;

TUNEABLE_CONSTANT int probcut_beta = 213;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 400;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 4;
#pragma once

#include "utility/fraction.h"
#include <array>
#include <cmath>
#include <cstddef>

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

constexpr inline int LMR_SCALE = 1024;

TUNEABLE_CONSTANT float LMR_constant = -1.926;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.539;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.708;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7731;

inline auto Initialise_LMR_reduction()
{
    std::array<std::array<Fraction<LMR_SCALE>, 64>, 64> ret = {};

    for (size_t i = 0; i < ret.size(); i++)
    {
        for (size_t j = 0; j < ret[i].size(); j++)
        {
            auto lmr = LMR_constant + LMR_depth_coeff * log(i + 1) + LMR_move_coeff * log(j + 1)
                + LMR_depth_move_coeff * log(i + 1) * log(j + 1);
            ret[i][j] = Fraction<LMR_SCALE>::from_raw(std::round(lmr * LMR_SCALE));
        }
    }

    return ret;
};

// [depth][move number]
TUNEABLE_CONSTANT auto LMR_reduction = Initialise_LMR_reduction();

TUNEABLE_CONSTANT int aspiration_window_size = 9;

TUNEABLE_CONSTANT int nmp_const = 6;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 253;
TUNEABLE_CONSTANT int nmp_sd = 6;

TUNEABLE_CONSTANT int iid_depth = 1;

TUNEABLE_CONSTANT auto se_sbeta_depth = Fraction<64>::from_raw(49);
TUNEABLE_CONSTANT int se_double = 0;
TUNEABLE_CONSTANT int se_triple = 31;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT auto lmr_pv = Fraction<LMR_SCALE>::from_raw(1389);
TUNEABLE_CONSTANT auto lmr_cut = Fraction<LMR_SCALE>::from_raw(1790);
TUNEABLE_CONSTANT auto lmr_improving = Fraction<LMR_SCALE>::from_raw(916);
TUNEABLE_CONSTANT auto lmr_loud = Fraction<LMR_SCALE>::from_raw(722);
TUNEABLE_CONSTANT int lmr_h = 2321;
TUNEABLE_CONSTANT auto lmr_offset = Fraction<LMR_SCALE>::from_raw(517);
TUNEABLE_CONSTANT int lmr_shallower = 10;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 12;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 286;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 209;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT auto rfp_const = Fraction<64>::from_raw(-150);
TUNEABLE_CONSTANT auto rfp_depth = Fraction<64>::from_raw(1940);
TUNEABLE_CONSTANT auto rfp_quad = Fraction<64>::from_raw(19);
TUNEABLE_CONSTANT int rfp_threat = 57;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT auto lmp_const = Fraction<64>::from_raw(425);
TUNEABLE_CONSTANT auto lmp_depth = Fraction<64>::from_raw(438);
TUNEABLE_CONSTANT auto lmp_quad = Fraction<64>::from_raw(3);

TUNEABLE_CONSTANT int fp_max_d = 12;
TUNEABLE_CONSTANT auto fp_const = Fraction<64>::from_raw(2235);
TUNEABLE_CONSTANT auto fp_depth = Fraction<64>::from_raw(910);
TUNEABLE_CONSTANT auto fp_quad = Fraction<64>::from_raw(613);

TUNEABLE_CONSTANT int see_quiet_depth = 113;
TUNEABLE_CONSTANT int see_quiet_hist = 123;
TUNEABLE_CONSTANT int see_loud_depth = 44;
TUNEABLE_CONSTANT int see_loud_hist = 124;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 1536;
TUNEABLE_CONSTANT int hist_prune = 985;

TUNEABLE_CONSTANT std::array eval_scale = { 35, 525, 514, 577, 1619 };
TUNEABLE_CONSTANT int eval_scale_const = 15969;

TUNEABLE_CONSTANT std::array see_values = { 134, 470, 466, 878, 1929, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3241;
TUNEABLE_CONSTANT float node_tm_base = 0.3730;
TUNEABLE_CONSTANT float node_tm_scale = 2.799;
TUNEABLE_CONSTANT float move_stability_base = 0.5475;
TUNEABLE_CONSTANT float move_stability_scale_a = 1.079;
TUNEABLE_CONSTANT float move_stability_scale_b = 0.3154;
TUNEABLE_CONSTANT float score_stability_base = 0.6062;
TUNEABLE_CONSTANT float score_stability_range = 1.450;
TUNEABLE_CONSTANT float score_stability_scale = 0.04794;
TUNEABLE_CONSTANT float score_stability_offset = 17.87;

TUNEABLE_CONSTANT int blitz_tc_a = 50;
TUNEABLE_CONSTANT int blitz_tc_b = 271;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT auto history_bonus_const = Fraction<64>::from_raw(879);
TUNEABLE_CONSTANT auto history_bonus_depth = Fraction<64>::from_raw(-13);
TUNEABLE_CONSTANT auto history_bonus_quad = Fraction<64>::from_raw(102);

TUNEABLE_CONSTANT auto history_penalty_const = Fraction<64>::from_raw(1555);
TUNEABLE_CONSTANT auto history_penalty_depth = Fraction<64>::from_raw(-2);
TUNEABLE_CONSTANT auto history_penalty_quad = Fraction<64>::from_raw(33);

TUNEABLE_CONSTANT int tt_replace_self_depth = 5;
TUNEABLE_CONSTANT auto tt_replace_depth = Fraction<64>::from_raw(56);
TUNEABLE_CONSTANT auto tt_replace_age = Fraction<64>::from_raw(249);

TUNEABLE_CONSTANT int good_loud_see = 65;
TUNEABLE_CONSTANT int good_loud_see_hist = 55;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 163;

TUNEABLE_CONSTANT int probcut_beta = 213;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 403;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 4;
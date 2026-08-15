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

TUNEABLE_CONSTANT float LMR_constant = -1.767;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.456;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.624;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7519;

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

TUNEABLE_CONSTANT auto aspiration_window_size = Fraction<64>::from_raw(568);
TUNEABLE_CONSTANT auto aspiration_window_growth_factor = Fraction<64>::from_raw(85);

TUNEABLE_CONSTANT int razor_max_d = 3;
TUNEABLE_CONSTANT std::array<int, 5> razor_margin = { 0, 393, 586, 764, 934 };
TUNEABLE_CONSTANT int razor_full_d = 2;
TUNEABLE_CONSTANT int razor_full_margin = 173;
TUNEABLE_CONSTANT int razor_verify_d = 3;
TUNEABLE_CONSTANT int razor_trim = 1;

TUNEABLE_CONSTANT auto nmp_const = Fraction<64>::from_raw(392);
TUNEABLE_CONSTANT auto nmp_depth = Fraction<4096>::from_raw(579);
TUNEABLE_CONSTANT auto nmp_score = Fraction<16384>::from_raw(73);
TUNEABLE_CONSTANT auto nmp_score_max = Fraction<64>::from_raw(316);
TUNEABLE_CONSTANT int nmp_beta_constant_margin = 48;

TUNEABLE_CONSTANT int iid_no_tt_depth = 1;
TUNEABLE_CONSTANT int iid_no_move_depth = 6;

TUNEABLE_CONSTANT auto se_sbeta_depth = Fraction<64>::from_raw(54);
TUNEABLE_CONSTANT int se_double = 1;
TUNEABLE_CONSTANT int se_triple = 32;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 5;

TUNEABLE_CONSTANT auto lmr_pv = Fraction<LMR_SCALE>::from_raw(1414);
TUNEABLE_CONSTANT auto lmr_cut = Fraction<LMR_SCALE>::from_raw(1935);
TUNEABLE_CONSTANT auto lmr_improving = Fraction<LMR_SCALE>::from_raw(822);
TUNEABLE_CONSTANT auto lmr_loud = Fraction<LMR_SCALE>::from_raw(713);
TUNEABLE_CONSTANT auto lmr_h = Fraction<16777216>::from_raw(2245);
TUNEABLE_CONSTANT auto lmr_offset = Fraction<LMR_SCALE>::from_raw(571);
TUNEABLE_CONSTANT int lmr_shallower = 9;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 1;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 272;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 232;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT auto rfp_const = Fraction<64>::from_raw(-33);
TUNEABLE_CONSTANT auto rfp_depth = Fraction<64>::from_raw(1562);
TUNEABLE_CONSTANT auto rfp_quad = Fraction<64>::from_raw(91);
TUNEABLE_CONSTANT int rfp_threat = 58;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT auto lmp_const = Fraction<64>::from_raw(438);
TUNEABLE_CONSTANT auto lmp_depth = Fraction<64>::from_raw(371);
TUNEABLE_CONSTANT auto lmp_quad = Fraction<64>::from_raw(13);

TUNEABLE_CONSTANT int fp_max_d = 12;
TUNEABLE_CONSTANT auto fp_const = Fraction<64>::from_raw(2392);
TUNEABLE_CONSTANT auto fp_depth = Fraction<64>::from_raw(676);
TUNEABLE_CONSTANT auto fp_quad = Fraction<64>::from_raw(526);

TUNEABLE_CONSTANT int see_quiet_depth = 110;
TUNEABLE_CONSTANT int see_quiet_hist = 111;
TUNEABLE_CONSTANT int see_loud_depth = 45;
TUNEABLE_CONSTANT int see_loud_hist = 96;
TUNEABLE_CONSTANT int see_max_depth = 7;

TUNEABLE_CONSTANT int hist_prune_depth = 1624;
TUNEABLE_CONSTANT int hist_prune = 555;

TUNEABLE_CONSTANT std::array eval_scale = { 49, 559, 557, 467, 1545 };
TUNEABLE_CONSTANT int eval_scale_const = 16567;

TUNEABLE_CONSTANT std::array see_values = { 150, 454, 458, 912, 1838, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3241;
TUNEABLE_CONSTANT float node_tm_base = 0.3140;
TUNEABLE_CONSTANT float node_tm_scale = 2.809;
TUNEABLE_CONSTANT float move_stability_base = 0.5964;
TUNEABLE_CONSTANT float move_stability_scale_a = 1.094;
TUNEABLE_CONSTANT float move_stability_scale_b = 0.3075;
TUNEABLE_CONSTANT float score_stability_base = 0.6419;
TUNEABLE_CONSTANT float score_stability_range = 1.398;
TUNEABLE_CONSTANT float score_stability_scale = 0.06160;
TUNEABLE_CONSTANT float score_stability_offset = 16.43;

TUNEABLE_CONSTANT int blitz_tc_a = 43;
TUNEABLE_CONSTANT int blitz_tc_b = 255;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT auto history_bonus_const = Fraction<64>::from_raw(720);
TUNEABLE_CONSTANT auto history_bonus_depth = Fraction<64>::from_raw(-60);
TUNEABLE_CONSTANT auto history_bonus_quad = Fraction<64>::from_raw(112);

TUNEABLE_CONSTANT auto history_penalty_const = Fraction<64>::from_raw(1898);
TUNEABLE_CONSTANT auto history_penalty_depth = Fraction<64>::from_raw(-59);
TUNEABLE_CONSTANT auto history_penalty_quad = Fraction<64>::from_raw(21);

TUNEABLE_CONSTANT int tt_replace_self_depth = 6;
TUNEABLE_CONSTANT auto tt_replace_depth = Fraction<64>::from_raw(78);
TUNEABLE_CONSTANT auto tt_replace_age = Fraction<64>::from_raw(242);

TUNEABLE_CONSTANT int good_loud_see = 70;
TUNEABLE_CONSTANT int good_loud_see_hist = 53;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 161;

TUNEABLE_CONSTANT int probcut_beta = 210;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 439;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 5;

TUNEABLE_CONSTANT float smp_voting_depth = 2.361f;
TUNEABLE_CONSTANT float smp_voting_score = 0.1270f;
TUNEABLE_CONSTANT float smp_voting_const = 424.3f;

TUNEABLE_CONSTANT auto pcm_const = Fraction<64>::from_raw(393);
TUNEABLE_CONSTANT auto pcm_depth = Fraction<64>::from_raw(-32);
TUNEABLE_CONSTANT auto pcm_quad = Fraction<64>::from_raw(56);

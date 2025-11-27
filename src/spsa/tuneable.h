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

TUNEABLE_CONSTANT float LMR_constant = -1.938;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.462;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.707;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7648;

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

TUNEABLE_CONSTANT auto aspiration_window_size = Fraction<64>::from_raw(543);
TUNEABLE_CONSTANT auto aspiration_window_growth_factor = Fraction<64>::from_raw(92);

TUNEABLE_CONSTANT int razor_max_d = 3;
TUNEABLE_CONSTANT std::array<int, 5> razor_margin = { 0, 360, 600, 850, 950 };
TUNEABLE_CONSTANT int razor_full_d = 2;
TUNEABLE_CONSTANT int razor_full_margin = 150;
TUNEABLE_CONSTANT int razor_verify_d = 3;
TUNEABLE_CONSTANT int razor_trim = 1;

TUNEABLE_CONSTANT auto nmp_const = Fraction<64>::from_raw(381);
TUNEABLE_CONSTANT auto nmp_depth = Fraction<4096>::from_raw(579);
TUNEABLE_CONSTANT auto nmp_score = Fraction<16384>::from_raw(69);
TUNEABLE_CONSTANT auto nmp_score_max = Fraction<64>::from_raw(330);
TUNEABLE_CONSTANT int nmp_beta_constant_margin = 30;

TUNEABLE_CONSTANT int iid_no_tt_depth = 1;
TUNEABLE_CONSTANT int iid_no_move_depth = 6;

TUNEABLE_CONSTANT auto se_sbeta_depth = Fraction<64>::from_raw(52);
TUNEABLE_CONSTANT int se_double = 0;
TUNEABLE_CONSTANT int se_triple = 31;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 5;

TUNEABLE_CONSTANT int check_ext_max_depth = 20;

TUNEABLE_CONSTANT auto lmr_pv = Fraction<LMR_SCALE>::from_raw(1429);
TUNEABLE_CONSTANT auto lmr_cut = Fraction<LMR_SCALE>::from_raw(1820);
TUNEABLE_CONSTANT auto lmr_improving = Fraction<LMR_SCALE>::from_raw(807);
TUNEABLE_CONSTANT auto lmr_loud = Fraction<LMR_SCALE>::from_raw(716);
TUNEABLE_CONSTANT auto lmr_h = Fraction<16777216>::from_raw(2276);
TUNEABLE_CONSTANT auto lmr_offset = Fraction<LMR_SCALE>::from_raw(536);
TUNEABLE_CONSTANT int lmr_shallower = 10;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 13;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 272;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 225;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT auto rfp_const = Fraction<64>::from_raw(-15);
TUNEABLE_CONSTANT auto rfp_depth = Fraction<64>::from_raw(1524);
TUNEABLE_CONSTANT auto rfp_quad = Fraction<64>::from_raw(40);
TUNEABLE_CONSTANT int rfp_threat = 58;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT auto lmp_const = Fraction<64>::from_raw(401);
TUNEABLE_CONSTANT auto lmp_depth = Fraction<64>::from_raw(440);
TUNEABLE_CONSTANT auto lmp_quad = Fraction<64>::from_raw(8);

TUNEABLE_CONSTANT int fp_max_d = 12;
TUNEABLE_CONSTANT auto fp_const = Fraction<64>::from_raw(2344);
TUNEABLE_CONSTANT auto fp_depth = Fraction<64>::from_raw(795);
TUNEABLE_CONSTANT auto fp_quad = Fraction<64>::from_raw(461);

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 119;
TUNEABLE_CONSTANT int see_loud_depth = 46;
TUNEABLE_CONSTANT int see_loud_hist = 121;
TUNEABLE_CONSTANT int see_max_depth = 7;

TUNEABLE_CONSTANT int hist_prune_depth = 1598;
TUNEABLE_CONSTANT int hist_prune = 604;

TUNEABLE_CONSTANT std::array eval_scale = { 56, 567, 545, 525, 1663 };
TUNEABLE_CONSTANT int eval_scale_const = 15206;

TUNEABLE_CONSTANT std::array see_values = { 141, 469, 468, 979, 1894, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2956;
TUNEABLE_CONSTANT float node_tm_base = 0.3464;
TUNEABLE_CONSTANT float node_tm_scale = 2.773;
TUNEABLE_CONSTANT float move_stability_base = 0.6139;
TUNEABLE_CONSTANT float move_stability_scale_a = 1.036;
TUNEABLE_CONSTANT float move_stability_scale_b = 0.3461;
TUNEABLE_CONSTANT float score_stability_base = 0.6104;
TUNEABLE_CONSTANT float score_stability_range = 1.367;
TUNEABLE_CONSTANT float score_stability_scale = 0.05795;
TUNEABLE_CONSTANT float score_stability_offset = 16.69;

TUNEABLE_CONSTANT int blitz_tc_a = 45;
TUNEABLE_CONSTANT int blitz_tc_b = 245;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT auto history_bonus_const = Fraction<64>::from_raw(751);
TUNEABLE_CONSTANT auto history_bonus_depth = Fraction<64>::from_raw(-36);
TUNEABLE_CONSTANT auto history_bonus_quad = Fraction<64>::from_raw(110);

TUNEABLE_CONSTANT auto history_penalty_const = Fraction<64>::from_raw(1728);
TUNEABLE_CONSTANT auto history_penalty_depth = Fraction<64>::from_raw(-6);
TUNEABLE_CONSTANT auto history_penalty_quad = Fraction<64>::from_raw(26);

TUNEABLE_CONSTANT int tt_replace_self_depth = 6;
TUNEABLE_CONSTANT auto tt_replace_depth = Fraction<64>::from_raw(65);
TUNEABLE_CONSTANT auto tt_replace_age = Fraction<64>::from_raw(272);

TUNEABLE_CONSTANT int good_loud_see = 65;
TUNEABLE_CONSTANT int good_loud_see_hist = 52;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 163;

TUNEABLE_CONSTANT int probcut_beta = 214;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 446;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 5;

TUNEABLE_CONSTANT float smp_voting_depth = 2.046f;
TUNEABLE_CONSTANT float smp_voting_score = 12.72f;
TUNEABLE_CONSTANT float smp_voting_const = 276.3f;

TUNEABLE_CONSTANT auto pcm_const = Fraction<64>::from_raw(375);
TUNEABLE_CONSTANT auto pcm_depth = Fraction<64>::from_raw(-18);
TUNEABLE_CONSTANT auto pcm_quad = Fraction<64>::from_raw(55);

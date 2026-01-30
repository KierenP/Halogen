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

TUNEABLE_CONSTANT float LMR_constant = -1.805;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.427;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.665;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7520;

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

TUNEABLE_CONSTANT auto aspiration_window_size = Fraction<64>::from_raw(550);
TUNEABLE_CONSTANT auto aspiration_window_growth_factor = Fraction<64>::from_raw(86);

TUNEABLE_CONSTANT int razor_max_d = 3;
TUNEABLE_CONSTANT std::array<int, 5> razor_margin = { 0, 391, 610, 757, 1019 };
TUNEABLE_CONSTANT int razor_full_d = 2;
TUNEABLE_CONSTANT int razor_full_margin = 163;
TUNEABLE_CONSTANT int razor_verify_d = 3;
TUNEABLE_CONSTANT int razor_trim = 1;

TUNEABLE_CONSTANT auto nmp_const = Fraction<64>::from_raw(376);
TUNEABLE_CONSTANT auto nmp_depth = Fraction<4096>::from_raw(570);
TUNEABLE_CONSTANT auto nmp_score = Fraction<16384>::from_raw(73);
TUNEABLE_CONSTANT auto nmp_score_max = Fraction<64>::from_raw(306);
TUNEABLE_CONSTANT int nmp_beta_constant_margin = 35;

TUNEABLE_CONSTANT int iid_no_tt_depth = 1;
TUNEABLE_CONSTANT int iid_no_move_depth = 6;

TUNEABLE_CONSTANT auto se_sbeta_depth = Fraction<64>::from_raw(54);
TUNEABLE_CONSTANT int se_double = 0;
TUNEABLE_CONSTANT int se_triple = 32;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 5;

TUNEABLE_CONSTANT auto lmr_pv = Fraction<LMR_SCALE>::from_raw(1376);
TUNEABLE_CONSTANT auto lmr_cut = Fraction<LMR_SCALE>::from_raw(1969);
TUNEABLE_CONSTANT auto lmr_improving = Fraction<LMR_SCALE>::from_raw(881);
TUNEABLE_CONSTANT auto lmr_loud = Fraction<LMR_SCALE>::from_raw(733);
TUNEABLE_CONSTANT auto lmr_gives_check = Fraction<LMR_SCALE>::from_raw(1024);
TUNEABLE_CONSTANT auto lmr_h = Fraction<16777216>::from_raw(2232);
TUNEABLE_CONSTANT auto lmr_offset = Fraction<LMR_SCALE>::from_raw(576);
TUNEABLE_CONSTANT int lmr_shallower = 9;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 5;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 279;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 231;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT auto rfp_const = Fraction<64>::from_raw(-140);
TUNEABLE_CONSTANT auto rfp_depth = Fraction<64>::from_raw(1682);
TUNEABLE_CONSTANT auto rfp_quad = Fraction<64>::from_raw(73);
TUNEABLE_CONSTANT int rfp_threat = 61;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT auto lmp_const = Fraction<64>::from_raw(421);
TUNEABLE_CONSTANT auto lmp_depth = Fraction<64>::from_raw(411);
TUNEABLE_CONSTANT auto lmp_quad = Fraction<64>::from_raw(23);

TUNEABLE_CONSTANT int fp_max_d = 12;
TUNEABLE_CONSTANT auto fp_const = Fraction<64>::from_raw(2378);
TUNEABLE_CONSTANT auto fp_depth = Fraction<64>::from_raw(689);
TUNEABLE_CONSTANT auto fp_quad = Fraction<64>::from_raw(545);

TUNEABLE_CONSTANT int see_quiet_depth = 109;
TUNEABLE_CONSTANT int see_quiet_hist = 118;
TUNEABLE_CONSTANT int see_loud_depth = 45;
TUNEABLE_CONSTANT int see_loud_hist = 101;
TUNEABLE_CONSTANT int see_max_depth = 7;

TUNEABLE_CONSTANT int hist_prune_depth = 1650;
TUNEABLE_CONSTANT int hist_prune = 421;

TUNEABLE_CONSTANT std::array eval_scale = { 64, 576, 560, 487, 1654 };
TUNEABLE_CONSTANT int eval_scale_const = 15486;

TUNEABLE_CONSTANT std::array see_values = { 146, 462, 455, 954, 1847, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2937;
TUNEABLE_CONSTANT float node_tm_base = 0.3256;
TUNEABLE_CONSTANT float node_tm_scale = 2.736;
TUNEABLE_CONSTANT float move_stability_base = 0.6151;
TUNEABLE_CONSTANT float move_stability_scale_a = 1.018;
TUNEABLE_CONSTANT float move_stability_scale_b = 0.3062;
TUNEABLE_CONSTANT float score_stability_base = 0.6054;
TUNEABLE_CONSTANT float score_stability_range = 1.418;
TUNEABLE_CONSTANT float score_stability_scale = 0.06193;
TUNEABLE_CONSTANT float score_stability_offset = 16.01;

TUNEABLE_CONSTANT int blitz_tc_a = 42;
TUNEABLE_CONSTANT int blitz_tc_b = 248;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT auto history_bonus_const = Fraction<64>::from_raw(737);
TUNEABLE_CONSTANT auto history_bonus_depth = Fraction<64>::from_raw(-79);
TUNEABLE_CONSTANT auto history_bonus_quad = Fraction<64>::from_raw(113);

TUNEABLE_CONSTANT auto history_penalty_const = Fraction<64>::from_raw(1899);
TUNEABLE_CONSTANT auto history_penalty_depth = Fraction<64>::from_raw(-90);
TUNEABLE_CONSTANT auto history_penalty_quad = Fraction<64>::from_raw(21);

TUNEABLE_CONSTANT int tt_replace_self_depth = 6;
TUNEABLE_CONSTANT auto tt_replace_depth = Fraction<64>::from_raw(72);
TUNEABLE_CONSTANT auto tt_replace_age = Fraction<64>::from_raw(248);

TUNEABLE_CONSTANT int good_loud_see = 73;
TUNEABLE_CONSTANT int good_loud_see_hist = 51;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 162;

TUNEABLE_CONSTANT int probcut_beta = 211;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 445;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 5;

TUNEABLE_CONSTANT float smp_voting_depth = 2.126f;
TUNEABLE_CONSTANT float smp_voting_score = 1.461f;
TUNEABLE_CONSTANT float smp_voting_const = 356.7f;

TUNEABLE_CONSTANT auto pcm_const = Fraction<64>::from_raw(391);
TUNEABLE_CONSTANT auto pcm_depth = Fraction<64>::from_raw(-28);
TUNEABLE_CONSTANT auto pcm_quad = Fraction<64>::from_raw(56);

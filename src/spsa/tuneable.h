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

TUNEABLE_CONSTANT float LMR_constant = -1.930;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.495;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.809;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7865;

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

TUNEABLE_CONSTANT Fraction<64> aspiration_window_size = Fraction<64>::from_raw(576);
TUNEABLE_CONSTANT Fraction<64> aspiration_window_growth_factor = Fraction<64>::from_raw(96);

TUNEABLE_CONSTANT auto nmp_const = Fraction<64>::from_raw(512);
TUNEABLE_CONSTANT auto nmp_d = Fraction<64>::from_raw(9);
TUNEABLE_CONSTANT auto nmp_s = Fraction<4096>::from_raw(16);
TUNEABLE_CONSTANT auto nmp_sd = Fraction<4096>::from_raw(24576);

TUNEABLE_CONSTANT int iid_no_tt_depth = 1;
TUNEABLE_CONSTANT int iid_no_move_depth = 6;

TUNEABLE_CONSTANT auto se_sbeta_depth = Fraction<64>::from_raw(48);
TUNEABLE_CONSTANT int se_double = 0;
TUNEABLE_CONSTANT int se_triple = 31;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT auto lmr_pv = Fraction<LMR_SCALE>::from_raw(1410);
TUNEABLE_CONSTANT auto lmr_cut = Fraction<LMR_SCALE>::from_raw(1775);
TUNEABLE_CONSTANT auto lmr_improving = Fraction<LMR_SCALE>::from_raw(893);
TUNEABLE_CONSTANT auto lmr_loud = Fraction<LMR_SCALE>::from_raw(721);
TUNEABLE_CONSTANT int lmr_h = 2306;
TUNEABLE_CONSTANT auto lmr_offset = Fraction<LMR_SCALE>::from_raw(527);
TUNEABLE_CONSTANT int lmr_shallower = 9;

TUNEABLE_CONSTANT int lmr_hindsight_ext_depth = 3;
TUNEABLE_CONSTANT int lmr_hindsight_ext_margin = 9;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 284;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 216;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT auto rfp_const = Fraction<64>::from_raw(-57);
TUNEABLE_CONSTANT auto rfp_depth = Fraction<64>::from_raw(1760);
TUNEABLE_CONSTANT auto rfp_quad = Fraction<64>::from_raw(28);
TUNEABLE_CONSTANT int rfp_threat = 62;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT auto lmp_const = Fraction<64>::from_raw(425);
TUNEABLE_CONSTANT auto lmp_depth = Fraction<64>::from_raw(434);
TUNEABLE_CONSTANT auto lmp_quad = Fraction<64>::from_raw(1);

TUNEABLE_CONSTANT int fp_max_d = 12;
TUNEABLE_CONSTANT auto fp_const = Fraction<64>::from_raw(2273);
TUNEABLE_CONSTANT auto fp_depth = Fraction<64>::from_raw(893);
TUNEABLE_CONSTANT auto fp_quad = Fraction<64>::from_raw(543);

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 128;
TUNEABLE_CONSTANT int see_loud_depth = 44;
TUNEABLE_CONSTANT int see_loud_hist = 123;
TUNEABLE_CONSTANT int see_max_depth = 8;

TUNEABLE_CONSTANT int hist_prune_depth = 1554;
TUNEABLE_CONSTANT int hist_prune = 724;

TUNEABLE_CONSTANT std::array eval_scale = { 46, 571, 534, 557, 1648 };
TUNEABLE_CONSTANT int eval_scale_const = 15321;

TUNEABLE_CONSTANT std::array see_values = { 137, 456, 491, 990, 1929, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2914;
TUNEABLE_CONSTANT float node_tm_base = 0.3469;
TUNEABLE_CONSTANT float node_tm_scale = 2.823;
TUNEABLE_CONSTANT float move_stability_base = 0.6029;
TUNEABLE_CONSTANT float move_stability_scale_a = 1.131;
TUNEABLE_CONSTANT float move_stability_scale_b = 0.3311;
TUNEABLE_CONSTANT float score_stability_base = 0.6039;
TUNEABLE_CONSTANT float score_stability_range = 1.400;
TUNEABLE_CONSTANT float score_stability_scale = 0.05841;
TUNEABLE_CONSTANT float score_stability_offset = 16.87;

TUNEABLE_CONSTANT int blitz_tc_a = 48;
TUNEABLE_CONSTANT int blitz_tc_b = 245;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT auto history_bonus_const = Fraction<64>::from_raw(827);
TUNEABLE_CONSTANT auto history_bonus_depth = Fraction<64>::from_raw(-39);
TUNEABLE_CONSTANT auto history_bonus_quad = Fraction<64>::from_raw(109);

TUNEABLE_CONSTANT auto history_penalty_const = Fraction<64>::from_raw(1681);
TUNEABLE_CONSTANT auto history_penalty_depth = Fraction<64>::from_raw(8);
TUNEABLE_CONSTANT auto history_penalty_quad = Fraction<64>::from_raw(27);

TUNEABLE_CONSTANT int tt_replace_self_depth = 6;
TUNEABLE_CONSTANT auto tt_replace_depth = Fraction<64>::from_raw(61);
TUNEABLE_CONSTANT auto tt_replace_age = Fraction<64>::from_raw(253);

TUNEABLE_CONSTANT int good_loud_see = 59;
TUNEABLE_CONSTANT int good_loud_see_hist = 52;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 162;

TUNEABLE_CONSTANT int probcut_beta = 216;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 442;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 4;

TUNEABLE_CONSTANT float smp_voting_depth = 1.0f;
TUNEABLE_CONSTANT float smp_voting_score = 20.0f;
TUNEABLE_CONSTANT float smp_voting_const = 180.0f;

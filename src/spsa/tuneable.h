#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#define TUNE

#ifdef TUNE
#define TUNEABLE_CONSTANT inline
#else
#define TUNEABLE_CONSTANT const inline
#endif

constexpr inline int LMR_SCALE = 1024;

TUNEABLE_CONSTANT float LMR_constant = -1.871;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.525;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.529;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7794;

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

TUNEABLE_CONSTANT int se_sbeta_depth = 54;
TUNEABLE_CONSTANT int se_double = 4;
TUNEABLE_CONSTANT int se_min_depth = 5;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1426;
TUNEABLE_CONSTANT int lmr_cut = 1654;
TUNEABLE_CONSTANT int lmr_improving = 1093;
TUNEABLE_CONSTANT int lmr_loud = 721;
TUNEABLE_CONSTANT int lmr_h = 2313;
TUNEABLE_CONSTANT int lmr_offset = 515;
TUNEABLE_CONSTANT int lmr_shallower = 10;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 299;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 212;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_const = -108;
TUNEABLE_CONSTANT int rfp_depth = 2319;
TUNEABLE_CONSTANT int rfp_quad = 21;
TUNEABLE_CONSTANT int rfp_threat = 56;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 388;
TUNEABLE_CONSTANT int lmp_depth = 409;
TUNEABLE_CONSTANT int lmp_quad = 32;

TUNEABLE_CONSTANT int fp_max_d = 11;
TUNEABLE_CONSTANT int fp_const = 2310;
TUNEABLE_CONSTANT int fp_depth = 987;
TUNEABLE_CONSTANT int fp_quad = 607;

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 126;
TUNEABLE_CONSTANT int see_loud_depth = 42;
TUNEABLE_CONSTANT int see_loud_hist = 133;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 1950;
TUNEABLE_CONSTANT int hist_prune = 521;

TUNEABLE_CONSTANT std::array eval_scale = { 25, 538, 498, 602, 1658 };
TUNEABLE_CONSTANT int eval_scale_const = 16545;

TUNEABLE_CONSTANT std::array see_values = { 136, 451, 475, 836, 1878, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2437;
TUNEABLE_CONSTANT float node_tm_base = 0.3607;
TUNEABLE_CONSTANT float node_tm_scale = 2.739;
TUNEABLE_CONSTANT int blitz_tc_a = 47;
TUNEABLE_CONSTANT int blitz_tc_b = 296;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 900;
TUNEABLE_CONSTANT int history_bonus_depth = -41;
TUNEABLE_CONSTANT int history_bonus_quad = 95;

TUNEABLE_CONSTANT int history_penalty_const = 1445;
TUNEABLE_CONSTANT int history_penalty_depth = -93;
TUNEABLE_CONSTANT int history_penalty_quad = 33;

TUNEABLE_CONSTANT int good_loud_see = 77;
TUNEABLE_CONSTANT int good_loud_see_hist = 50;

TUNEABLE_CONSTANT int tt_replace_self_depth = 4;
TUNEABLE_CONSTANT int tt_replace_depth = 53;
TUNEABLE_CONSTANT int tt_replace_age = 270;

TUNEABLE_CONSTANT int qsearch_lmp = 2;
TUNEABLE_CONSTANT int qsearch_see_hist = 177;

TUNEABLE_CONSTANT int probcut_beta = 214;
TUNEABLE_CONSTANT int probcut_min_depth = 3;
TUNEABLE_CONSTANT int probcut_depth_const = 5;

TUNEABLE_CONSTANT int generalized_tt_failhigh_margin = 400;
TUNEABLE_CONSTANT int generalized_tt_failhigh_depth = 4;
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

TUNEABLE_CONSTANT float LMR_constant = -1.672;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.430;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.457;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7852;

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
TUNEABLE_CONSTANT int nmp_s = 251;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 48;
TUNEABLE_CONSTANT int se_double = 7;
TUNEABLE_CONSTANT int se_double_pv = 477;
TUNEABLE_CONSTANT int se_double_hd = 303;
TUNEABLE_CONSTANT int se_double_quiet = 0;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 4;

TUNEABLE_CONSTANT int lmr_pv = 1382;
TUNEABLE_CONSTANT int lmr_cut = 1494;
TUNEABLE_CONSTANT int lmr_improving = 1130;
TUNEABLE_CONSTANT int lmr_loud = 770;
TUNEABLE_CONSTANT int lmr_h = 2472;
TUNEABLE_CONSTANT int lmr_offset = 427;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 312;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 203;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_const = -194;
TUNEABLE_CONSTANT int rfp_depth = 2774;
TUNEABLE_CONSTANT int rfp_quad = 7;
TUNEABLE_CONSTANT int rfp_threat = 49;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 374;
TUNEABLE_CONSTANT int lmp_depth = 387;
TUNEABLE_CONSTANT int lmp_quad = 23;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 2390;
TUNEABLE_CONSTANT int fp_depth = 863;
TUNEABLE_CONSTANT int fp_quad = 729;

TUNEABLE_CONSTANT int see_quiet_depth = 110;
TUNEABLE_CONSTANT int see_quiet_hist = 151;
TUNEABLE_CONSTANT int see_loud_depth = 41;
TUNEABLE_CONSTANT int see_loud_hist = 139;
TUNEABLE_CONSTANT int see_max_depth = 8;

TUNEABLE_CONSTANT int hist_prune_depth = 2057;
TUNEABLE_CONSTANT int hist_prune = 290;

TUNEABLE_CONSTANT int delta_c = 484;

TUNEABLE_CONSTANT std::array eval_scale = { 0, 585, 495, 674, 1810 };
TUNEABLE_CONSTANT int eval_scale_const = 20310;

TUNEABLE_CONSTANT std::array see_values = { 135, 478, 428, 734, 1693, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2726;
TUNEABLE_CONSTANT float node_tm_base = 0.3899;
TUNEABLE_CONSTANT float node_tm_scale = 2.636;
TUNEABLE_CONSTANT int blitz_tc_a = 48;
TUNEABLE_CONSTANT int blitz_tc_b = 343;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 864;
TUNEABLE_CONSTANT int history_bonus_depth = 75;
TUNEABLE_CONSTANT int history_bonus_quad = 87;

TUNEABLE_CONSTANT int history_penalty_const = 1174;
TUNEABLE_CONSTANT int history_penalty_depth = -13;
TUNEABLE_CONSTANT int history_penalty_quad = 39;

TUNEABLE_CONSTANT int good_loud_see = 100;
TUNEABLE_CONSTANT int good_loud_see_hist = 51;

TUNEABLE_CONSTANT int tt_replace_self_depth = 4;
TUNEABLE_CONSTANT int tt_replace_depth = 58;
TUNEABLE_CONSTANT int tt_replace_age = 235;
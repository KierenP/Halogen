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

TUNEABLE_CONSTANT float LMR_constant = -1.389;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.445;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.411;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.7255;

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

TUNEABLE_CONSTANT int aspiration_window_size = 7;

TUNEABLE_CONSTANT int nmp_const = 5;
TUNEABLE_CONSTANT int nmp_d = 6;
TUNEABLE_CONSTANT int nmp_s = 271;
TUNEABLE_CONSTANT int nmp_sd = 4;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 42;
TUNEABLE_CONSTANT int se_double = 13;
TUNEABLE_CONSTANT int se_min_depth = 7;
TUNEABLE_CONSTANT int se_tt_depth = 6;

TUNEABLE_CONSTANT int lmr_pv = 1436;
TUNEABLE_CONSTANT int lmr_cut = 957;
TUNEABLE_CONSTANT int lmr_improving = 1130;
TUNEABLE_CONSTANT int lmr_loud = 862;
TUNEABLE_CONSTANT int lmr_h = 2719;
TUNEABLE_CONSTANT int lmr_offset = 677;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 317;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 222;

TUNEABLE_CONSTANT int rfp_max_d = 10;
TUNEABLE_CONSTANT int rfp_const = -340;
TUNEABLE_CONSTANT int rfp_depth = 1745;
TUNEABLE_CONSTANT int rfp_quad = 4;
TUNEABLE_CONSTANT int rfp_threat = 41;

TUNEABLE_CONSTANT int lmp_max_d = 8;
TUNEABLE_CONSTANT int lmp_const = 401;
TUNEABLE_CONSTANT int lmp_depth = 354;
TUNEABLE_CONSTANT int lmp_quad = 21;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 1680;
TUNEABLE_CONSTANT int fp_depth = 549;
TUNEABLE_CONSTANT int fp_quad = 807;

TUNEABLE_CONSTANT int see_quiet_depth = 122;
TUNEABLE_CONSTANT int see_quiet_hist = 112;
TUNEABLE_CONSTANT int see_loud_depth = 27;
TUNEABLE_CONSTANT int see_loud_hist = 115;
TUNEABLE_CONSTANT int see_max_depth = 9;

TUNEABLE_CONSTANT int hist_prune_depth = 2304;
TUNEABLE_CONSTANT int hist_prune = 376;

TUNEABLE_CONSTANT int delta_c = 869;

TUNEABLE_CONSTANT std::array eval_scale = { -9, 551, 561, 787, 2043 };
TUNEABLE_CONSTANT int eval_scale_const = 22097;

TUNEABLE_CONSTANT std::array see_values = { 79, 467, 555, 776, 1244, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3950;
TUNEABLE_CONSTANT float node_tm_base = 0.4289;
TUNEABLE_CONSTANT float node_tm_scale = 1.672;
TUNEABLE_CONSTANT int blitz_tc_a = 49;
TUNEABLE_CONSTANT int blitz_tc_b = 366;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 1286;
TUNEABLE_CONSTANT int history_bonus_depth = 806;
TUNEABLE_CONSTANT int history_bonus_quad = 78;

TUNEABLE_CONSTANT int history_penalty_const = 1311;
TUNEABLE_CONSTANT int history_penalty_depth = 258;
TUNEABLE_CONSTANT int history_penalty_quad = 33;

TUNEABLE_CONSTANT int good_loud_see = 140;
TUNEABLE_CONSTANT int good_loud_see_hist = 31;

TUNEABLE_CONSTANT int tt_replace_self_depth = 2;
TUNEABLE_CONSTANT int tt_replace_depth = 64;
TUNEABLE_CONSTANT int tt_replace_age = 149;
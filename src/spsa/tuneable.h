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

TUNEABLE_CONSTANT float LMR_constant = -1.551;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.388;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.564;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.8151;

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
TUNEABLE_CONSTANT int nmp_s = 244;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 45;
TUNEABLE_CONSTANT int se_double = 7;
TUNEABLE_CONSTANT int se_double_pv = 489;
TUNEABLE_CONSTANT int se_double_hd = 300;
TUNEABLE_CONSTANT int se_double_quiet = 1;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 3;

TUNEABLE_CONSTANT int lmr_pv = 1361;
TUNEABLE_CONSTANT int lmr_cut = 1335;
TUNEABLE_CONSTANT int lmr_improving = 1110;
TUNEABLE_CONSTANT int lmr_loud = 808;
TUNEABLE_CONSTANT int lmr_h = 2586;
TUNEABLE_CONSTANT int lmr_offset = 374;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 312;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 214;

TUNEABLE_CONSTANT int rfp_max_d = 10;
TUNEABLE_CONSTANT int rfp_const = -135;
TUNEABLE_CONSTANT int rfp_depth = 3097;
TUNEABLE_CONSTANT int rfp_quad = 15;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 405;
TUNEABLE_CONSTANT int lmp_depth = 403;
TUNEABLE_CONSTANT int lmp_quad = 9;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 2394;
TUNEABLE_CONSTANT int fp_depth = 841;
TUNEABLE_CONSTANT int fp_quad = 812;

TUNEABLE_CONSTANT int see_quiet_depth = 109;
TUNEABLE_CONSTANT int see_quiet_hist = 146;
TUNEABLE_CONSTANT int see_loud_depth = 37;
TUNEABLE_CONSTANT int see_loud_hist = 155;
TUNEABLE_CONSTANT int see_max_depth = 8;

TUNEABLE_CONSTANT int hist_prune_depth = 2681;
TUNEABLE_CONSTANT int hist_prune = 253;

TUNEABLE_CONSTANT int delta_c = 432;

TUNEABLE_CONSTANT std::array eval_scale = { 0, 569, 474, 706, 1697 };
TUNEABLE_CONSTANT int eval_scale_const = 21516;

TUNEABLE_CONSTANT std::array see_values = { 123, 456, 427, 806, 1632, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2562;
TUNEABLE_CONSTANT float node_tm_base = 0.4414;
TUNEABLE_CONSTANT float node_tm_scale = 2.487;
TUNEABLE_CONSTANT int blitz_tc_a = 47;
TUNEABLE_CONSTANT int blitz_tc_b = 401;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 818;
TUNEABLE_CONSTANT int history_bonus_depth = 55;
TUNEABLE_CONSTANT int history_bonus_quad = 78;

TUNEABLE_CONSTANT int history_penalty_const = 1144;
TUNEABLE_CONSTANT int history_penalty_depth = -23;
TUNEABLE_CONSTANT int history_penalty_quad = 44;
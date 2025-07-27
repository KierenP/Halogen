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

TUNEABLE_CONSTANT float LMR_constant = -1.525;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.339;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.544;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.8275;

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
TUNEABLE_CONSTANT int nmp_s = 245;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int iid_depth = 3;

TUNEABLE_CONSTANT int se_sbeta_depth = 44;
TUNEABLE_CONSTANT int se_double = 7;
TUNEABLE_CONSTANT int se_double_pv = 474;
TUNEABLE_CONSTANT int se_double_hd = 293;
TUNEABLE_CONSTANT int se_double_quiet = 1;
TUNEABLE_CONSTANT int se_min_depth = 6;
TUNEABLE_CONSTANT int se_tt_depth = 3;

TUNEABLE_CONSTANT int lmr_pv = 1335;
TUNEABLE_CONSTANT int lmr_cut = 1287;
TUNEABLE_CONSTANT int lmr_improving = 1129;
TUNEABLE_CONSTANT int lmr_loud = 778;
TUNEABLE_CONSTANT int lmr_h = 2619;
TUNEABLE_CONSTANT int lmr_offset = 387;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 314;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 227;

TUNEABLE_CONSTANT int rfp_max_d = 10;
TUNEABLE_CONSTANT int rfp_const = -36;
TUNEABLE_CONSTANT int rfp_depth = 3210;
TUNEABLE_CONSTANT int rfp_quad = 4;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 389;
TUNEABLE_CONSTANT int lmp_depth = 370;
TUNEABLE_CONSTANT int lmp_quad = 12;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 2381;
TUNEABLE_CONSTANT int fp_depth = 907;
TUNEABLE_CONSTANT int fp_quad = 828;

TUNEABLE_CONSTANT int see_quiet_depth = 112;
TUNEABLE_CONSTANT int see_quiet_hist = 146;
TUNEABLE_CONSTANT int see_loud_depth = 36;
TUNEABLE_CONSTANT int see_loud_hist = 156;
TUNEABLE_CONSTANT int see_max_depth = 8;

TUNEABLE_CONSTANT int hist_prune_depth = 2896;
TUNEABLE_CONSTANT int hist_prune = 286;

TUNEABLE_CONSTANT int delta_c = 415;

TUNEABLE_CONSTANT std::array eval_scale = { 0, 550, 465, 679, 1688 };
TUNEABLE_CONSTANT int eval_scale_const = 23150;

TUNEABLE_CONSTANT std::array see_values = { 117, 463, 442, 783, 1581, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.2709;
TUNEABLE_CONSTANT float node_tm_base = 0.4794;
TUNEABLE_CONSTANT float node_tm_scale = 2.439;
TUNEABLE_CONSTANT int blitz_tc_a = 50;
TUNEABLE_CONSTANT int blitz_tc_b = 422;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;

TUNEABLE_CONSTANT int history_bonus_const = 832;
TUNEABLE_CONSTANT int history_bonus_depth = 43;
TUNEABLE_CONSTANT int history_bonus_quad = 78;

TUNEABLE_CONSTANT int history_penalty_const = 1074;
TUNEABLE_CONSTANT int history_penalty_depth = -2;
TUNEABLE_CONSTANT int history_penalty_quad = 47;
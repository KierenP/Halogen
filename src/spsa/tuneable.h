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

TUNEABLE_CONSTANT float LMR_constant = -1.550;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.115;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.798;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.8258;

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
TUNEABLE_CONSTANT int nmp_s = 250;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int se_d = 40;
TUNEABLE_CONSTANT int se_de = 5;

TUNEABLE_CONSTANT int lmr_pv = 1252;
TUNEABLE_CONSTANT int lmr_cut = 1176;
TUNEABLE_CONSTANT int lmr_improving = 1192;
TUNEABLE_CONSTANT int lmr_loud = 755;
TUNEABLE_CONSTANT int lmr_h = 2668;
TUNEABLE_CONSTANT int lmr_offset = 369;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 310;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 248;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 51;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 3;
TUNEABLE_CONSTANT int lmp_depth = 3;

TUNEABLE_CONSTANT int fp_max_d = 9;
TUNEABLE_CONSTANT int fp_const = 39;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 13;

TUNEABLE_CONSTANT int see_d = 106;
TUNEABLE_CONSTANT int see_h = 149;

TUNEABLE_CONSTANT int se_max_d = 6;
TUNEABLE_CONSTANT int see_tt_d = 4;

TUNEABLE_CONSTANT int delta_c = 346;

TUNEABLE_CONSTANT int eval_scale_knight = 546;
TUNEABLE_CONSTANT int eval_scale_bishop = 432;
TUNEABLE_CONSTANT int eval_scale_rook = 718;
TUNEABLE_CONSTANT int eval_scale_queen = 1648;
TUNEABLE_CONSTANT int eval_scale_const = 23978;

TUNEABLE_CONSTANT std::array see_values = { 106, 481, 452, 808, 1399, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3463;
TUNEABLE_CONSTANT float node_tm_base = 0.5324;
TUNEABLE_CONSTANT float node_tm_scale = 2.223;
TUNEABLE_CONSTANT int blitz_tc_a = 248;
TUNEABLE_CONSTANT int blitz_tc_b = 619;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;
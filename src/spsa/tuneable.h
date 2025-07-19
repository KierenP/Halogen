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

TUNEABLE_CONSTANT float LMR_constant = -0.7851;
TUNEABLE_CONSTANT float LMR_depth_coeff = 1.041;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.126;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.6481;

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

TUNEABLE_CONSTANT int aspiration_window_size = 10;

TUNEABLE_CONSTANT int nmp_const = 6;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 247;
TUNEABLE_CONSTANT int nmp_sd = 5;

TUNEABLE_CONSTANT int se_d = 45;
TUNEABLE_CONSTANT int se_de = 10;

TUNEABLE_CONSTANT int lmr_pv = 566;
TUNEABLE_CONSTANT int lmr_cut = 1286;
TUNEABLE_CONSTANT int lmr_improving = 969;
TUNEABLE_CONSTANT int lmr_loud = 640;
TUNEABLE_CONSTANT int lmr_h = 2565;
TUNEABLE_CONSTANT int lmr_offset = 417;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 303;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 256;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 51;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 3;
TUNEABLE_CONSTANT int lmp_depth = 4;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 42;
TUNEABLE_CONSTANT int fp_depth = 13;
TUNEABLE_CONSTANT int fp_quad = 14;

TUNEABLE_CONSTANT int see_d = 108;
TUNEABLE_CONSTANT int see_h = 147;

TUNEABLE_CONSTANT int se_max_d = 6;
TUNEABLE_CONSTANT int see_tt_d = 3;

TUNEABLE_CONSTANT int delta_c = 334;

TUNEABLE_CONSTANT int eval_scale_knight = 534;
TUNEABLE_CONSTANT int eval_scale_bishop = 466;
TUNEABLE_CONSTANT int eval_scale_rook = 742;
TUNEABLE_CONSTANT int eval_scale_queen = 1368;
TUNEABLE_CONSTANT int eval_scale_const = 25618;

TUNEABLE_CONSTANT std::array see_values = { 108, 527, 484, 757, 1457, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.3682;
TUNEABLE_CONSTANT float node_tm_base = 0.5434;
TUNEABLE_CONSTANT float node_tm_scale = 2.021;
TUNEABLE_CONSTANT int blitz_tc_a = 45;
TUNEABLE_CONSTANT int blitz_tc_b = 608;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;
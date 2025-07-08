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

TUNEABLE_CONSTANT float LMR_constant = 0.3938;
TUNEABLE_CONSTANT float LMR_depth_coeff = -0.2255;
TUNEABLE_CONSTANT float LMR_move_coeff = 2.135;
TUNEABLE_CONSTANT float LMR_depth_move_coeff = -0.4355;

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

TUNEABLE_CONSTANT int aspiration_window_size = 11;

TUNEABLE_CONSTANT int nmp_const = 6;
TUNEABLE_CONSTANT int nmp_d = 7;
TUNEABLE_CONSTANT int nmp_s = 241;
TUNEABLE_CONSTANT int nmp_sd = 4;

TUNEABLE_CONSTANT int se_d = 48;
TUNEABLE_CONSTANT int se_de = 11;

TUNEABLE_CONSTANT int lmr_pv = 1079;
TUNEABLE_CONSTANT int lmr_cut = 1081;
TUNEABLE_CONSTANT int lmr_improving = 185;
TUNEABLE_CONSTANT int lmr_loud = 295;
TUNEABLE_CONSTANT int lmr_h = 2411;
TUNEABLE_CONSTANT int lmr_offset = 593;

TUNEABLE_CONSTANT int fifty_mr_scale_a = 288;
TUNEABLE_CONSTANT int fifty_mr_scale_b = 246;

TUNEABLE_CONSTANT int rfp_max_d = 9;
TUNEABLE_CONSTANT int rfp_d = 75;

TUNEABLE_CONSTANT int lmp_max_d = 7;
TUNEABLE_CONSTANT int lmp_const = 3;
TUNEABLE_CONSTANT int lmp_depth = 4;

TUNEABLE_CONSTANT int fp_max_d = 10;
TUNEABLE_CONSTANT int fp_const = 36;
TUNEABLE_CONSTANT int fp_depth = 12;
TUNEABLE_CONSTANT int fp_quad = 12;

TUNEABLE_CONSTANT int see_d = 111;
TUNEABLE_CONSTANT int see_h = 148;

TUNEABLE_CONSTANT int se_max_d = 6;
TUNEABLE_CONSTANT int see_tt_d = 4;

TUNEABLE_CONSTANT int delta_c = 337;

TUNEABLE_CONSTANT int eval_scale_knight = 529;
TUNEABLE_CONSTANT int eval_scale_bishop = 444;
TUNEABLE_CONSTANT int eval_scale_rook = 734;
TUNEABLE_CONSTANT int eval_scale_queen = 1267;
TUNEABLE_CONSTANT int eval_scale_const = 26846;

TUNEABLE_CONSTANT std::array see_values = { 103, 495, 512, 736, 1259, 5000 };

TUNEABLE_CONSTANT float soft_tm = 0.4777;
TUNEABLE_CONSTANT float node_tm_base = 0.5683;
TUNEABLE_CONSTANT float node_tm_scale = 2.302;
TUNEABLE_CONSTANT int blitz_tc_a = 44;
TUNEABLE_CONSTANT int blitz_tc_b = 789;
TUNEABLE_CONSTANT int sudden_death_tc = 51;
TUNEABLE_CONSTANT int repeating_tc = 96;
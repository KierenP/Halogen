#pragma once

#include "../Score.h"
#include "temporal_coherence.h"

#include <cmath>

// t ranges from 0 at the begining to 1 at the end
inline float learning_rate_schedule(float t)
{
    // cosine annealing
    static constexpr float initial_lr = 0.001;
    return initial_lr * (cos(t * M_PI) + 1.0) / 2.0;
}

// The current adjusted learning rate.
inline float lr_alpha = learning_rate_schedule(0);

// The current credit assignment discount factor
inline float td_lambda = 1;

inline PredictionQualityRecord prediction_quality_record;

// discount rate of future rewards
inline constexpr double GAMMA = 1;

inline constexpr int training_nodes = 4000;
inline constexpr double sigmoid_coeff = 2.5 / 400.0;

inline constexpr double training_time_hours = 16;

inline constexpr int max_threads = 20;

// we want to add only 5% of DFRC positions into the training
inline constexpr float opening_book_usage_pct = 0.05;

inline constexpr Score opening_cutoff = 500;
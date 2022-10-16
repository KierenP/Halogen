#pragma once

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

inline float td_lambda = 1;

inline PredictionQualityRecord prediction_quality_record;
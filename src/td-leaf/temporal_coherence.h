#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

// Based on Temporal Coherence and Prediction Decay in TD Learning (https://www.ijcai.org/Proceedings/99-1/Papers/081.pdf)

float prediction_quality(float p1, float p2);

struct PredictionQualityRecord
{
    void add_observation(unsigned int d, float p1, float p2);
    float fit_psi();
    void clear();
    void debug_print();

private:
    struct SumWithCount
    {
        float sum = 0;
        size_t count = 0;
    };

    std::array<SumWithCount, 64> q_d_sum;

    float mse(float psi);
};
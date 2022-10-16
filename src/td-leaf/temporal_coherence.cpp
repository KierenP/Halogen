#include "temporal_coherence.h"

float prediction_quality(float p1, float p2)
{
    auto f = [](float p1, float p2) -> float{
        auto r = std::abs(p1 - p2);

        // find positive solution to (1+p)y^2+(2-2p-p^2)y-(1-p)^2=0.
        auto a = (1+p1);
        auto b = (2-2*p1-p1*p1);
        auto c = -(1-p1)*(1-p1);

        auto y = (-b + std::sqrt(b*b - 4*a*c)) / (2*a);
        
        if (r <= y)
        {
            return 1-r/y;
        }
        else 
        {
            return -p1*(r-y)/(p1-y);
        }
    };

    if (p1 >= 0.5)
    {
        auto result = f(p1, p2);
        assert(-1 <= result && result <= 1);
        return result;
    }
    else 
    {
        auto result = f(1-p1, 1-p2);
        assert(-1 <= result && result <= 1);
        return result;
    }
}

void PredictionQualityRecord::add_observation(unsigned int d, float p1, float p2)
{
    if (d >= q_d_sum.size())
    {
        // throw away observations with temporal differences too far apart
        return;
    }

    q_d_sum[d].sum += prediction_quality(p1, p2);
    q_d_sum[d].count++;
}

float PredictionQualityRecord::fit_psi()
{
    // Naive approach to fit psi by choosing equally spaced values and zooming in

    float a = 0;
    float b = 1;

    while (b - a > 0.001)
    {
        float lowest_mse = std::numeric_limits<float>::max();
        int best_psi_index = 0;

        for (int i = 0; i <= 10; i++)
        {
            float x = a + ((b-a)/10.0) * i;
            float error = mse(x);

            if (error < lowest_mse)
            {
                lowest_mse = error;
                best_psi_index = i;
            }
        }

        if (best_psi_index == 0)
        {
            // special case, new window is [a, a + ((b-a)/10.0)]
            b = a + ((b-a)/10.0);
        }
        else if (best_psi_index == 10)
        {
            // special case, new window is [b - ((b-a)/10.0), b]
            a = b - ((b-a)/10.0);
        }
        else 
        {
            // window is [a + ((b-a)/10.0) * (best_psi_index - 1), a + ((b-a)/10.0) * (best_psi_index + 1)]
            auto new_a = a + ((b-a)/10.0) * (best_psi_index - 1);
            auto new_b = a + ((b-a)/10.0) * (best_psi_index + 1);
            a = new_a;
            b = new_b;
        }
    }

    return a;
}

float PredictionQualityRecord::mse(float psi)
{
    float mean_squared_error = 0;

    for (unsigned int i = 0; i < q_d_sum.size(); i++)
    {
        if (q_d_sum[i].count == 0)
        {
            continue;
        }
        
        auto psi_bar = (q_d_sum[i].sum / (float)(q_d_sum[i].count));
        auto error = (psi_bar - std::pow(psi, i));
        mean_squared_error += (error * error) * q_d_sum[i].count;
    }

    return mean_squared_error;
}

void PredictionQualityRecord::clear()
{
    q_d_sum = {};
}

void PredictionQualityRecord::debug_print()
{
    for (unsigned int i = 0; i < q_d_sum.size(); i++)
    {
        std::cout << "D: " << i << " Q_bar: " << q_d_sum[i].sum / (float)(q_d_sum[i].count) << " N: " << q_d_sum[i].count << "\n";
    }

    std::cout << "psi: " << fit_psi() << "\n";
}
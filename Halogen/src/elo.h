#pragma once

/*
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Elo {

	const double WIN = 1;
	const double DRAW = 0.5;
	const double LOSS = 0;
	const double LOGISTIC_DIVISOR = 400 / std::log(10);

	double round_places(double x, double places) {
		if (x == 0) {
			return 0;
		}

		double scale = std::pow(10.0, places);
		return std::round(x * scale) / scale;
	}

	struct Distribution {
	public:
		virtual double cdf(double x, double mean) const { return 0; };
		virtual ~Distribution() {};
	};

	struct LogisticDistribution : public Distribution {
		double base;
		double scale;

	public:
		LogisticDistribution(double initial_base, double initial_scale) :
			base(initial_base), scale(initial_scale) {};

		virtual double cdf(double x, double mean) const override {
			return 1.0 / (1.0 + std::pow(base, -((x - mean) / scale)));
		}
	};

	struct NormalDistribution : public Distribution {
		double stdev;

	public:
		NormalDistribution(double initial_stdev) :
			stdev(initial_stdev) {};

		virtual double cdf(double x, double mean) const override {
			return (1 + std::erf((x - mean) / (stdev * std::sqrt(2)))) / 2;
		}
	};

	LogisticDistribution default_distribution(10, 400);

	class Player;
	struct Match;

	struct Configuration {
		std::function<double(Player&)> calculate_k;
		Distribution& dist;

		Configuration(double initial_k, Distribution& initial_distribution = default_distribution) :
			calculate_k([initial_k](Player& p) {return initial_k; }), dist(initial_distribution) {};

		Configuration(std::function<double(Player&)> initial_calculate_k, Distribution& initial_distribution = default_distribution) :
			calculate_k(initial_calculate_k), dist(initial_distribution) {};
	};

	Configuration default_configuration(32, default_distribution);

	class Player {
		std::vector<Match> matches;

	public:
		double rating;
		Configuration config;

		Player(double initial_rating, Configuration initial_configuration = default_configuration) :
			rating(initial_rating), config(initial_configuration) {};

		double round_rating(double places) {
			return round_places(rating, places);
		}

		std::vector<Match> get_matches() {
			return matches;
		}

		// Add match WITHOUT changing rating.
		void add_match(Match& m) {
			matches.push_back(m);
		}
	};

	struct Match {
		Player& player_a;
		Player& player_b;
		// This is the result for player_a.
		double result;

		Match(Player& initial_player_a, Player& initial_player_b, double initial_result, bool apply_now = false) :
			player_a(initial_player_a), player_b(initial_player_b), result(initial_result) {
			if (apply_now) {
				apply();
			}
		};

		bool apply() {
			if (applied) {
				return false;
			}

			double player_a_delta = player_a.config.calculate_k(player_a) * (result - player_a.config.dist.cdf(player_b.rating, player_a.rating));
			double player_b_delta = player_b.config.calculate_k(player_b) * ((1 - result) - player_b.config.dist.cdf(player_a.rating, player_b.rating));
			player_a.rating += player_a_delta;
			player_b.rating += player_b_delta;
			player_a.add_match(*this);
			player_b.add_match(*this);

			applied = true;
			return true;
		}

	private:
		bool applied = false;
	};

	struct IntervalEstimate {
		double lower;
		double estimate;
		double upper;
		double p;
		bool lower_infinity = false;
		bool estimate_infinity = false;
		bool upper_infinity = false;
	};

	/* Source: Abramowitz, M. & Stegun, I. (1964).
	Handbook of Mathematical Functions with Formulas, Graphs, and Mathematical Tables.
	Section 26.2.23. */
	double homf_tail(double p) {
		if (p <= 0 || p >= 1) {
			throw std::invalid_argument("p must be in [0,1].");
		}

		double c0 = 2.515517;
		double c1 = 0.802853;
		double c2 = 0.010328;
		double d1 = 1.432788;
		double d2 = 0.189269;
		double d3 = 0.001308;

		double t = std::sqrt(std::log(1 / (p * p)));
		return t - (c0 + c1 * t + c2 * t * t) / (1 + d1 * t + d2 * t * t + d3 * t * t * t);
	}

	double quantile(double p) {
		if (p < 0 || p > 1) {
			throw std::invalid_argument("p must be in [0,1].");
		}

		if (p < 0.5) {
			return -homf_tail(p);
		}
		else {
			return homf_tail(1 - p);
		}
	}

	// x successes in n trials using the Wilson score interval (see also https://arxiv.org/pdf/1303.1288.pdf).
	IntervalEstimate binomial_estimate(double x, double n, double p = 0.95) {
		if (x < 0) {
			throw std::invalid_argument("x must be nonnegative.");
		}

		if (n <= 0) {
			throw std::invalid_argument("n must be positive.");
		}

		if (x > n) {
			throw std::invalid_argument("x must not be greater than n.");
		}

		IntervalEstimate est;
		double z = quantile(1 - (1 - p) / 2);
		double mlp = x / n;
		double mlq = 1 - mlp;
		double mid = (x + (z * z) / 2) / (n + z * z);
		double delta = (z / (n + z * z)) * std::sqrt(mlp * mlq * n + (z * z) / 4);

		est.estimate = mid;
		est.lower = mid - delta;
		est.upper = mid + delta;
		est.p = p;
		return est;
	}

	// Default parameters with the default distribution.
	double logistic_inverse_cdf(double x, double mean = 0, double scale = LOGISTIC_DIVISOR) {
		if (x <= 0 || x >= 1) {
			throw std::invalid_argument("x must be in (0,1).");
		}

		return (mean + scale * std::log(x / (1 - x)));
	}


	// Rating difference assuming the default Logistic distribution.
	IntervalEstimate estimate_rating_difference(int wins, int draws, int losses, double p = 0.95) {
		if (wins < 0 || draws < 0 || losses < 0) {
			throw std::invalid_argument("wins, draws, and losses must be nonnegative.");
		}

		// Naive solution: 1 draw = 1 win and 1 loss. This is so we can calculate based on a binomial rather than trinomial distribution.
		wins += draws;
		losses += draws;
		double n = wins + losses;

		if (n <= 0) {
			throw std::invalid_argument("The total number of games must be positive.");
		}

		IntervalEstimate est;

		if (wins / n == 0 || wins / n == 1) {
			est.estimate_infinity = true;
			return est;
		}

		est = binomial_estimate(wins, n, p);

		if (est.estimate <= 0 || est.estimate >= 1) {
			est.estimate_infinity = true;
		}
		else {
			est.estimate = logistic_inverse_cdf(est.estimate);
		}

		if (est.lower <= 0) {
			est.lower_infinity = true;
		}
		else {
			est.lower = logistic_inverse_cdf(est.lower);
		}

		if (est.upper >= 1) {
			est.upper_infinity = true;
		}
		else {
			est.upper = logistic_inverse_cdf(est.upper);
		}

		return est;
	}
}
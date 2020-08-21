#include "texel.h"

void Texel(std::vector<int*> params, std::vector<int*> PST)
{
	//Extract the PST pointers into the params list
	for (size_t piece = 0; piece < PST.size(); piece++)
	{
		for (int square = 0; square < N_SQUARES; square++)
		{
			params.push_back(&PST[piece][square]);
		}
	}

	HASH_ENABLE = false;

	std::ifstream infile("C:\\quiet-labeled.epd");

	if (!infile)
	{
		std::cout << "Cannot open position file!" << std::endl;
		return;
	}

	std::string line;
	int lineCount = 0;
	int quietCount = 0;

	std::cout << "Beginning to read in positions..." << std::endl;

	std::vector<std::pair<Position, double>> positions;
	Position position;
	SearchData data;

	while (std::getline(infile, line))
	{
		if (lineCount % 10000 == 0)
			std::cout << "Reading line: " << lineCount << " quiet positions: " << quietCount << "\r";

		Loadquietlabeled(positions, line, lineCount, position);
		
		if (EvaluatePosition(position) == TexelSearch(position, data))
			quietCount++;
		else
			positions.pop_back();

		lineCount++;
	}

	std::cout << "\nAll positions loaded successfully" << std::endl;
	std::cout << "\nEvaluating positions..." << std::endl;

	uint64_t totalScore = 0;

	for (size_t i = 0; i < positions.size(); i++)
	{
		int score = TexelSearch(positions[i].first, data);
		totalScore += score;
	}

	std::cout << "All positions evaluated successfully" << std::endl;
	std::cout << "Eval hash: " << totalScore << std::endl;
	std::cout << "\nBeginning Texel tuning..." << std::endl;

	//CalculateK(positionList, positionScore, positionResults);
	double k = 1.51;
	double step_size = 10000;
	int iteration = 1;
	int batchSize = 10;	//eg 20 means 1/20th
	double lambda = 0.0000001;

	auto rng = std::default_random_engine{};

	std::vector<double> paramiterValues;

	for (size_t i = 0; i < params.size(); i++)
	{
		paramiterValues.push_back(*params[i]);
	}

	while (true)
	{
		if (iteration % 10 == 0)
			std::shuffle(std::begin(positions), std::end(positions), rng);
		double error = CalculateError(positions, k, positions.size() / params.size() / batchSize, (positions.size() / params.size() / batchSize) * (iteration % batchSize), lambda);

		if (iteration % 10 == 0)
			PrintIteration(error, params, PST, paramiterValues, step_size, iteration);

		std::vector<double> gradient;

		for (size_t i = 0; i < params.size(); i++)
		{
			(*params[i])++;
			double error_plus_epsilon = CalculateError(positions, k, positions.size() / params.size() / batchSize, (positions.size() / params.size() / batchSize) * (iteration % batchSize), lambda);
			(*params[i])--;
			double firstDerivitivePositive = (error_plus_epsilon - error);

			(*params[i])--;
			double error_minus_epsilon = CalculateError(positions, k, positions.size() / params.size() / batchSize, (positions.size() / params.size() / batchSize) * (iteration % batchSize), lambda);
			(*params[i])++;
			double firstDerivitiveNegative = (error_minus_epsilon - error);

			double firstDerivitive;

			if (firstDerivitivePositive > 0 && firstDerivitiveNegative > 0)
				firstDerivitive = 0; //making the param higher by one made it worse and making it one lower also made it worse so we shouldn't change it.
			else 
				firstDerivitive = firstDerivitivePositive;

			gradient.push_back(firstDerivitive);
		}

		for (size_t i = 0; i < params.size(); i++)
		{
			double delta = step_size * gradient[i];
			paramiterValues[i] -= delta;
			(*params[i]) = static_cast<int>(round(paramiterValues[i]));
		}

		step_size *= 1 / (1 + 0.0000001 * iteration);
		//step_size = 100000 * exp(-0.0002 * iteration);
		iteration++;

		if (step_size < 1)
			break;
	}

	std::cout << "Texel complete" << std::endl;

	return;
}

void LoadBench4Per(std::vector<std::pair<Position, double>>& positions, std::string& line, int lineCount, Position& position)
{
	std::vector<std::string> arrayTokens;
	std::istringstream iss(line);
	arrayTokens.clear();

	do
	{
		std::string stub;
		iss >> stub;
		arrayTokens.push_back(stub);
	} while (iss);

	if (!position.InitialiseFromFen(line))
	{
		std::cout << "line " << lineCount + 1 << ": BAD FEN" << std::endl;
	}

	std::string result = arrayTokens[arrayTokens.size() - 2];

	if (result == "0-1")
	{
		positions.push_back({ position, 0 });
	}
	else if (result == "1-0")
	{
		positions.push_back({ position, 1 });
	}
	else if (result == "1/2-1/2")
	{
		positions.push_back({ position, 0.5 });
	}
	else
	{
		std::cout << "line " << lineCount + 1 << ": Could not read result" << std::endl;
	}
}

void Loadquietlabeled(std::vector<std::pair<Position, double>>& positions, std::string& line, int lineCount, Position& position)
{
	std::vector<std::string> arrayTokens;
	std::istringstream iss(line);
	arrayTokens.clear();

	do
	{
		std::string stub;
		iss >> stub;

		if (stub == "c9")
		{
			arrayTokens.push_back("0");
			arrayTokens.push_back("1");
			break;
		}

		arrayTokens.push_back(stub);
	} while (iss);

	if (!position.InitialiseFromFen(arrayTokens))
	{
		std::cout << "line " << lineCount + 1 << ": BAD FEN" << std::endl;
	}

	std::string result;
	iss >> result;

	if (result == "\"0-1\";")
	{
		positions.push_back({ position, 0 });
	}
	else if (result == "\"1-0\";")
	{
		positions.push_back({ position, 1 });
	}
	else if (result == "\"1/2-1/2\";")
	{
		positions.push_back({ position, 0.5 });
	}
	else
	{
		std::cout << "line " << lineCount + 1 << ": Could not read result" << std::endl;
	}
}

void PrintIteration(double error, std::vector<int*>& params, std::vector<int*> PST, std::vector<double> paramiterValues, double step_size, int iteration)
{
	for (size_t i = 0; i < params.size() - PST.size() * 64; i++)
	{
		std::cout << "Paramiter " << i << ": " << *(params[i]) << " exact value: " << paramiterValues[i] << std::endl;
	}
	std::cout << std::endl;

	//print PST
	for (size_t piece = 0; piece < PST.size(); piece++)
	{
		std::cout << "PST " << piece << ": " << std::endl;

		for (int i = 0; i < N_SQUARES; i++)
		{
			if (0 <= PST[piece][i] && PST[piece][i] <= 9)
				std::cout << "   " << PST[piece][i] << ",";
			else if (-9 <= PST[piece][i] && PST[piece][i] <= -1)
				std::cout << "  " << PST[piece][i] << ",";
			else if (10 <= PST[piece][i] && PST[piece][i] <= 99)
				std::cout << "  " << PST[piece][i] << ",";
			else if (-99 <= PST[piece][i] && PST[piece][i] <= -10)
				std::cout << " " << PST[piece][i] << ",";
			else if (101 <= PST[piece][i] && PST[piece][i] <= 999)
				std::cout << " " << PST[piece][i] << ",";
			else
				std::cout << "" << PST[piece][i] << ",";

			if (i % N_FILES == 7 && i > 0)
				std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	std::cout << "Error: " << error << std::endl;
	std::cout << "Step size: " << step_size << std::endl;
	std::cout << "Iteration: " << iteration << std::endl;
	std::cout << std::endl;
}

double CalculateError(std::vector<std::pair<Position, double>>& positions, double k, size_t subset, size_t start, double lambda)
{
	InitializePieceSquareTable();	//if tuning PST you need to re-load them with this

	double error = 0;
	for (size_t i = start; i < start + subset; i++)
	{
		double sigmoid = 1 / (1 + pow(10, -EvaluatePosition(positions.at(i).first) * k / 400));
		error += pow(positions.at(i).second - sigmoid, 2);
	}
	error /= subset;

	//L2 regularisation for sum of PST

	double sum;
	
	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		if (GetRank(i) != RANK_1 && GetRank(i) != RANK_8)
			sum += PawnSquareValuesMid[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		if (GetRank(i) != RANK_1 && GetRank(i) != RANK_8)
			sum += PawnSquareValuesEndGame[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += KnightSquareValues[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += BishopSquareValues[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += RookSquareValues[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += QueenSquareValues[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += KingSquareMid[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < N_SQUARES; i++)
	{
		sum += KingSquareEndGame[i];
	}
	error += lambda * sum * sum;

	//L2 regularisation for sum of Knight and Rook adjustment

	sum = 0;
	for (int i = 0; i < 9; i++)
	{
		sum += knightAdj[i];
	}
	error += lambda * sum * sum;

	sum = 0;
	for (int i = 0; i < 9; i++)
	{
		sum += rookAdj[i];
	}
	error += lambda * sum * sum;

	return error;
}

void TexelOptimise()
{

}

double CalculateK(const std::vector<Position>& positionList, const std::vector<double>& positionScore, const std::vector<double>& positionResults)
{
	double k = 0;
	double epsilon = 0.0001;

	for (int rep = 0; rep < 100; rep++)
	{
		double error_k = 0;
		double error_k_minus_epsilon = 0;
		double error_k_plus_epsilon = 0;

		for (size_t i = 0; i < positionList.size(); i++)
		{
			error_k += pow(positionResults[i] - 1 / (1 + pow(10, -positionScore[i] * k / 400)), 2);
			error_k_plus_epsilon += pow(positionResults[i] - 1 / (1 + pow(10, -positionScore[i] * (k + epsilon) / 400)), 2);
			error_k_minus_epsilon += pow(positionResults[i] - 1 / (1 + pow(10, -positionScore[i] * (k - epsilon) / 400)), 2);
		}

		error_k /= positionList.size();
		error_k_minus_epsilon /= positionList.size();
		error_k_plus_epsilon /= positionList.size();

		std::cout << "evaluation error: " << error_k << " K: " << k << std::endl;

		double firstDerivitive = (error_k_plus_epsilon - error_k) / epsilon;
		double secondDerivitive = (error_k_plus_epsilon - 2 * error_k + error_k_minus_epsilon) / (epsilon * epsilon);

		double newk = k - firstDerivitive / secondDerivitive;

		if (abs(newk - k) < epsilon)
			return k;

		k = newk;
	}

	std::cout << "Failed to converge after 100 iterations" << std::endl;
	return k;
}
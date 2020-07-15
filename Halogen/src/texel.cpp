#include "texel.h"

void Texel(std::vector<int*> params)
{
	/*
	Make sure you disable pawn hash tables and transposition tables!
	*/

	tTable.AddEntry(Move(), 1234, 0, 0, 0, EntryType::EXACT);
	if (CheckEntry(tTable.GetEntry(1234), 1234))
	{
		std::cout << "Transposition table has been left on!" << std::endl;
		return;
	}

	pawnHashTable.AddEntry(1234, 0);
	if (pawnHashTable.CheckEntry(1234))
	{
		std::cout << "Pawn hash table has been left on!" << std::endl;
		return;
	}


	std::ifstream infile("C:\\quiet-labeled.epd");
	//std::ifstream infile("C:\\OpenBench4Per.txt");

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

	for (int i = 0; i < positions.size(); i++)
	{
		int score = TexelSearch(positions[i].first, data);
		totalScore += score;
	}

	std::cout << "All positions evaluated successfully" << std::endl;
	std::cout << "Eval hash: " << totalScore << std::endl;
	std::cout << "\nBeginning Texel tuning..." << std::endl;

	//CalculateK(positionList, positionScore, positionResults);
	double k = 1.51;
	double prevError = 0;
	double step_size = 100000;
	int iteration = 1;

	auto rng = std::default_random_engine{};

	std::vector<double> paramiterValues;

	for (int i = 0; i < params.size(); i++)
	{
		paramiterValues.push_back(*params[i]);
	}

	while (true)
	{
		std::shuffle(std::begin(positions), std::end(positions), rng);

		double error = CalculateError(positions, data, k, positions.size() / params.size());
		PrintIteration(error, params, paramiterValues, step_size, iteration);

		std::vector<double> gradient;

		for (int i = 0; i < params.size(); i++)
		{
			(*params[i])++;
			double error_plus_epsilon = CalculateError(positions, data, k, positions.size() / params.size());
			(*params[i])--;
			double firstDerivitive = (error_plus_epsilon - error);

			gradient.push_back(firstDerivitive);
		}

		for (int i = 0; i < params.size(); i++)
		{
			double delta = step_size * gradient[i];
			paramiterValues[i] -= delta;
			(*params[i]) = round(paramiterValues[i]);
		}

		step_size *= 1 / (1 + 0.0001 * iteration);
		iteration++;
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

void PrintIteration(double error, std::vector<int*>& params, std::vector<double> paramiterValues, double step_size, int iteration)
{

	for (int i = 0; i < params.size(); i++)
	{
		std::cout << "Paramiter " << i << ": " << *(params[i]) << " exact value: " << paramiterValues[i] << std::endl;
	}

	std::cout << "Error: " << error << std::endl;
	std::cout << "Step size: " << step_size << std::endl;
	std::cout << "Iteration: " << iteration << std::endl;
	std::cout << std::endl;
}

double CalculateError(std::vector<std::pair<Position, double>>& positions, SearchData& data, double k, unsigned int subset)
{
	InitializePieceSquareTable();	//if tuning PST you need to re-load them with this

	double error = 0;
	for (int i = 0; i < subset; i++)
	{
		double sigmoid = 1 / (1 + pow(10, -TexelSearch(positions[i].first, data) * k / 400));
		error += pow(positions[i].second - sigmoid, 2);
	}
	error /= subset;
	return error;
}

void TexelOptimise()
{

}

double CalculateK(std::vector<Position>& positionList, std::vector<double>& positionScore, std::vector<double>& positionResults)
{
	double k = 0;
	double epsilon = 0.0001;

	for (int rep = 0; rep < 100; rep++)
	{
		double error_k = 0;
		double error_k_minus_epsilon = 0;
		double error_k_plus_epsilon = 0;
		double sigmoid = 0;

		for (int i = 0; i < positionList.size(); i++)
		{
			sigmoid = 1 / (1 + pow(10, -positionScore[i] * k / 400));
			error_k += pow(positionResults[i] - sigmoid, 2);

			sigmoid = 1 / (1 + pow(10, -positionScore[i] * (k + epsilon) / 400));
			error_k_plus_epsilon += pow(positionResults[i] - sigmoid, 2);

			sigmoid = 1 / (1 + pow(10, -positionScore[i] * (k - epsilon) / 400));
			error_k_minus_epsilon += pow(positionResults[i] - sigmoid, 2);
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
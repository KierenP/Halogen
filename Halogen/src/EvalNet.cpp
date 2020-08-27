#include "EvalNet.h"
#pragma once

#define INPUT_NEURONS 12 * 64 - 32 + 1 + 4

int pieceValueVector[N_STAGES][N_PIECE_TYPES] = { {91, 532, 568, 715, 1279, 5000},
                                                  {111, 339, 372, 638, 1301, 5000} };\

struct trainingPoint
{
    trainingPoint(std::array<double, INPUT_NEURONS> input, double gameResult);

    std::array<double, INPUT_NEURONS> inputs;
    double result;
};

struct Neuron
{
    Neuron(std::vector<double> Weight, double Bias);
    double FeedForward(std::vector<double>& input) const;
    void WriteToFile(std::ofstream& myfile);

    std::vector<double> weights;
    double bias;
};

struct HiddenLayer
{
    HiddenLayer(std::vector<double> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<double> FeedForward(std::vector<double>& input);
    void WriteToFile(std::ofstream& myfile);

    std::vector<Neuron> neurons;

    //cache for backprop after feedforward
    std::vector<double> zeta;    //weighted input
    std::vector<double> alpha;   //result after activation function

    void activation(std::vector<double>& x);
    void activationPrime(std::vector<double>& x);
};

struct Network
{
    Network(std::vector<std::vector<double>> inputs, std::vector<size_t> hiddenNeuronCount);
    double FeedForward(std::array<double, INPUT_NEURONS> inputs);
    double Backpropagate(trainingPoint data, double learnRate);
    void WriteToFile();
    void Learn();

private:
    std::vector<trainingPoint> quietlabeledDataset();
    std::vector<trainingPoint> Stockfish3PerDataset();
    size_t inputNeurons;

    //cache for backprop after feedforward (these are for the output neuron)
    double zeta;    //weighted input
    double alpha;   //result after activation function

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;
};

Network* net;

bool BlackBlockade(uint64_t wPawns, uint64_t bPawns);
bool WhiteBlockade(uint64_t wPawns, uint64_t bPawns);
Network* CreateRandom(std::vector<size_t> hiddenNeuronCount);
std::array<double, INPUT_NEURONS> GetInputLayer(const Position& position);

template<typename Numeric, typename Generator = std::mt19937>
Numeric random(Numeric from, Numeric to);

int EvaluatePositionNet(const Position& position)
{
    assert(net != nullptr);
    return net->FeedForward(GetInputLayer(position));
}

std::array<double, INPUT_NEURONS> GetInputLayer(const Position& position)
{
    std::array<double, INPUT_NEURONS> ret;

    size_t index = 0;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bb = position.GetPieceBB(i);

        for (int sq = 0; sq < N_SQUARES; sq++)
        {
            if ((i != WHITE_PAWN && i != BLACK_PAWN) || (GetRank(sq) > RANK_1 && GetRank(sq) < RANK_8))
                ret[index++] = ((bb & SquareBB[sq]) != 0);
        }
    }

    ret[index++] = (position.GetTurn());
    ret[index++] = (position.CanCastleWhiteKingside());
    ret[index++] = (position.CanCastleWhiteQueenside());
    ret[index++] = (position.CanCastleBlackKingside());
    ret[index++] = (position.CanCastleBlackQueenside());

    return ret;
}

int PieceValues(unsigned int Piece, GameStages GameStage)
{
    return pieceValueVector[GameStage][Piece % N_PIECE_TYPES];
}

void Learn()
{
    assert(net != nullptr);
    net->Learn();
}

bool InitEval(std::string file)
{
    std::ifstream stream(file);

    if (!stream.is_open())
    {
        std::cout << "info string Could not load network file: " << file << std::endl;
        std::cout << "info string random weights initialization!" << std::endl;
        net = CreateRandom({ INPUT_NEURONS, 1 });
        return false;
    }

    std::string line;

    std::vector<std::vector<double>> weights;
    std::vector<size_t> LayerNeurons;

    while (getline(stream, line))
    {
        std::istringstream iss(line);
        std::string token;

        iss >> token;

        if (token == "InputNeurons")
        {
            getline(stream, line);
            std::istringstream lineStream(line);
            lineStream >> token;
            LayerNeurons.push_back(stoull(token));
        }

        if (token == "HiddenLayerNeurons")
        {
            std::vector<double> layerWeights;

            iss >> token;
            size_t num = stoull(token);

            LayerNeurons.push_back(num);

            for (size_t i = 0; i < num; i++)
            {
                getline(stream, line);
                std::istringstream lineStream(line);

                while (lineStream >> token)
                {
                    layerWeights.push_back(stod(token));
                }
            }

            weights.push_back(layerWeights);
        }

        else if (token == "OutputLayer")
        {
            LayerNeurons.push_back(1);  //always 1 output neuron
            std::vector<double> layerWeights;
            getline(stream, line);
            std::istringstream lineStream(line);
            while (lineStream >> token)
            {
                layerWeights.push_back(stod(token));
            }
            weights.push_back(layerWeights);
        }
    }

    stream.close();
    net = new Network(weights, LayerNeurons);

    return true;
}

bool InitEval()
{
    std::string line;
    std::istringstream stream(weightsText);

    std::vector<std::vector<double>> weights;
    std::vector<size_t> LayerNeurons;

    while (getline(stream, line))
    {
        std::istringstream iss(line);
        std::string token;

        iss >> token;

        if (token == "InputNeurons")
        {
            getline(stream, line);
            std::istringstream lineStream(line);
            lineStream >> token;
            LayerNeurons.push_back(stoull(token));
        }

        if (token == "HiddenLayerNeurons")
        {
            std::vector<double> layerWeights;

            iss >> token;
            size_t num = stoull(token);

            LayerNeurons.push_back(num);

            for (size_t i = 0; i < num; i++)
            {
                getline(stream, line);
                std::istringstream lineStream(line);

                while (lineStream >> token)
                {
                    layerWeights.push_back(stod(token));
                }
            }

            weights.push_back(layerWeights);
        }

        else if (token == "OutputLayer")
        {
            LayerNeurons.push_back(1);  //always 1 output neuron
            std::vector<double> layerWeights;
            getline(stream, line);
            std::istringstream lineStream(line);
            while (lineStream >> token)
            {
                layerWeights.push_back(stod(token));
            }
            weights.push_back(layerWeights);
        }
    }

    net = new Network(weights, LayerNeurons);

    return true;
}

Neuron::Neuron(std::vector<double> Weight, double Bias)
{
    weights = Weight;
    bias = Bias;
}

double Neuron::FeedForward(std::vector<double>& input) const
{
    assert(input.size() == weights.size());
    return std::inner_product(input.begin(), input.end(), weights.begin(), 0.0);
}

void Neuron::WriteToFile(std::ofstream& myfile)
{
    for (int i = 0; i < weights.size(); i++)
    {
        myfile << weights.at(i) << " ";
    }
    myfile << bias << "\n";
}

HiddenLayer::HiddenLayer(std::vector<double> inputs, size_t NeuronCount)
{
    assert(inputs.size() % NeuronCount == 0);

    size_t WeightsPerNeuron = inputs.size() / NeuronCount;

    for (size_t i = 0; i < NeuronCount; i++)
    {
        neurons.push_back(Neuron(std::vector<double>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron - 1)));
    }
}

std::vector<double> HiddenLayer::FeedForward(std::vector<double>& input)
{
    zeta.clear();
    alpha.clear();

    for (int i = 0; i < neurons.size(); i++)
    {
        zeta.push_back(neurons.at(i).FeedForward(input));
        alpha.push_back(std::max(0.0, zeta.back()));
    }

    return alpha;
}

void HiddenLayer::WriteToFile(std::ofstream& myfile)
{
    myfile << "HiddenLayerNeurons " << neurons.size() << "\n";

    for (int i = 0; i < neurons.size(); i++)
    {
        neurons.at(i).WriteToFile(myfile);
    }
}

void HiddenLayer::activation(std::vector<double>& x)
{
    for (int i = 0; i < x.size(); i++)
    {
        x[i] = std::max(0.0, x[i]);
    }
}

void HiddenLayer::activationPrime(std::vector<double>& x)
{
    for (int i = 0; i < x.size(); i++)
    {
        x[i] = (x[i] >= 0);
    }
}

Network::Network(std::vector<std::vector<double>> inputs, std::vector<size_t> hiddenNeuronCount) : outputNeuron(std::vector<double>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    assert(inputs.size() == hiddenNeuronCount.size() - 1);

    inputNeurons = inputs.front().size() - 1;

    for (size_t i = 1; i < hiddenNeuronCount.size() - 1; i++)
    {
        hiddenLayers.push_back(HiddenLayer(inputs.at(i), hiddenNeuronCount.at(i)));
    }
}

double Network::FeedForward(std::array<double, INPUT_NEURONS> inputs)
{
    assert(inputs.size() == inputNeurons);
    std::vector<double> inputLayer = std::vector<double>(inputs.begin(), inputs.end());

    for (size_t i = 0; i < hiddenLayers.size(); i++)
    {
        inputLayer = hiddenLayers.at(i).FeedForward(inputLayer);
    }

    zeta = outputNeuron.FeedForward(inputLayer);
    alpha = 1 / (1 + exp(-0.0087 * zeta));  //-0.0087 chosen to mymic the previous evaluation function

    return zeta;
}

double Network::Backpropagate(trainingPoint data, double learnRate)
{
    FeedForward(data.inputs);
    double delta_l = (alpha - data.result) * (alpha) * (1 - alpha);

    for (int i = 0; i < outputNeuron.weights.size(); i++)
    {
        outputNeuron.weights.at(i) -= delta_l * data.inputs.at(i);
    }
    outputNeuron.bias -= delta_l;

    return 0.5 * (alpha - data.result) * (alpha - data.result);
}

void Network::WriteToFile()
{
    //from stack overflow: generates a random string
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(10, 0);
    std::generate_n(str.begin(), 10, randchar);

    std::ofstream myfile("C:\\HalogenWeights\\" + str + ".network");
    if (!myfile.is_open())
    {
        std::cout << "Could not write network to output file!";
        return;
    }

    myfile << "InputNeurons\n";
    myfile << inputNeurons << "\n";

    for (int i = 0; i < hiddenLayers.size(); i++)
    {
        hiddenLayers.at(i).WriteToFile(myfile);
    }

    myfile << "OutputLayer\n";
    for (int i = 0; i < outputNeuron.weights.size(); i++)
    {
        myfile << outputNeuron.weights.at(i) << " ";
    }
    myfile << outputNeuron.bias << "\n";

    myfile.close();

}

void Network::Learn()
{
    //std::vector<trainingPoint> data = Stockfish3PerDataset();
    std::vector<trainingPoint> data = quietlabeledDataset();

    for (int epoch = 1; epoch <= 100000; epoch++)
    {
        double error = 0;

        for (int point = 0; point < data.size(); point++)
        {
            error += net->Backpropagate(data[point], 0.1);
        }

        error /= data.size();

        std::cout << "Finished epoch: " << epoch << " MSE: " << 2 * error << std::endl;

        if (epoch % 100 == 0)
            WriteToFile();
    }

    WriteToFile();
}

std::vector<trainingPoint> Network::quietlabeledDataset()
{
    std::ifstream infile("C:\\quiet-labeled.epd");

    if (!infile)
    {
        std::cout << "Cannot open position file!" << std::endl;
        return {};
    }

    std::string line;
    int lineCount = 0;
    int quietCount = 0;

    std::cout << "Beginning to read in positions..." << std::endl;

    std::vector<trainingPoint> positions;
    Position position;

    while (std::getline(infile, line))
    {
        if (lineCount % 10000 == 0)
            std::cout << "Reading line: " << lineCount << " quiet positions: " << quietCount << "\r";

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
            positions.push_back({ GetInputLayer(position), 0 });
        }
        else if (result == "\"1-0\";")
        {
            positions.push_back({ GetInputLayer(position), 1 });
        }
        else if (result == "\"1/2-1/2\";")
        {
            positions.push_back({ GetInputLayer(position), 0.5 });
        }
        else
        {
            std::cout << "line " << lineCount + 1 << ": Could not read result" << std::endl;
        }

        quietCount++;
        lineCount++;
    }

    std::cout << "Reading line: " << lineCount << " quiet positions: " << quietCount << "\r";
    std::cout << "\nAll positions loaded successfully" << std::endl;

    return positions;
}

std::vector<trainingPoint> Network::Stockfish3PerDataset()
{
    std::ifstream infile("C:\\fish2.fens");

    if (!infile)
    {
        std::cout << "Cannot open position file!" << std::endl;
        return {};
    }

    std::string line;
    int lineCount = 0;
    int quietCount = 0;

    std::cout << "Beginning to read in positions..." << std::endl;

    std::vector<trainingPoint> positions;
    Position position;

    while (std::getline(infile, line) && lineCount < 2000000)
    {
        if (lineCount % 10000 == 0)
            std::cout << "Reading line: " << lineCount << " quiet positions: " << quietCount << "\r";

        std::vector<std::string> arrayTokens;
        std::istringstream iss(line);
        arrayTokens.clear();
        std::string result;

        do
        {
            std::string stub;
            iss >> stub;

            if (stub == "0-1" || stub == "1-0" || stub == "1/2-1/2")
            {
                result = stub;
                break;
            }

            arrayTokens.push_back(stub);
        } while (iss);

        if (!position.InitialiseFromFen(arrayTokens))
        {
            std::cout << "line " << lineCount + 1 << ": BAD FEN" << std::endl;
        }

        if (result == "0-1")
        {
            positions.push_back({ GetInputLayer(position), 0 });
        }
        else if (result == "1-0")
        {
            positions.push_back({ GetInputLayer(position), 1 });
        }
        else if (result == "1/2-1/2")
        {
            positions.push_back({ GetInputLayer(position), 0.5 });
        }
        else
        {
            std::cout << "line " << lineCount + 1 << ": Could not read result" << std::endl;
        }

        quietCount++;
        lineCount++;
    }

    std::cout << "Reading line: " << lineCount << " quiet positions: " << quietCount << "\r";
    std::cout << "\nAll positions loaded successfully" << std::endl;

    return positions;
}

Network* CreateRandom(std::vector<size_t> NeuronCount)
{
    std::vector<std::vector<double>> inputs;

    size_t prevLayerNeurons = NeuronCount[0];

    for (int layer = 1; layer < NeuronCount.size() - 1; layer++)
    {
        std::vector<double> input;
        for (int i = 0; i < (prevLayerNeurons + 1) * NeuronCount[layer]; i++)
        {
            input.push_back(random<double>(0, 1));
        }
        inputs.push_back(input);
        prevLayerNeurons = NeuronCount[layer];
    }

    //connections from last hidden to output
    std::vector<double> input;
    for (int i = 0; i < (prevLayerNeurons + 1) * NeuronCount.back(); i++)
    {
        input.push_back(random<double>(0, 1));
    }
    inputs.push_back(input);

    Network* ret = new Network(inputs, NeuronCount);
    //ret->WriteToFile();
    return ret;
}

bool DeadPosition(const Position& position)
{
    if ((position.GetPieceBB(WHITE_PAWN)) != 0) return false;
    if ((position.GetPieceBB(WHITE_ROOK)) != 0) return false;
    if ((position.GetPieceBB(WHITE_QUEEN)) != 0) return false;

    if ((position.GetPieceBB(BLACK_PAWN)) != 0) return false;
    if ((position.GetPieceBB(BLACK_ROOK)) != 0) return false;
    if ((position.GetPieceBB(BLACK_QUEEN)) != 0) return false;

    /*
    From the Chess Programming Wiki:
        According to the rules of a dead position, Article 5.2 b, when there is no possibility of checkmate for either side with any series of legal moves, the position is an immediate draw if
        - both sides have a bare king													1.
        - one side has a king and a minor piece against a bare king						2.
        - both sides have a king and a bishop, the bishops being the same color			Not covered
    */

    //We know the board must contain just knights, bishops and kings
    int WhiteBishops = GetBitCount(position.GetPieceBB(WHITE_BISHOP));
    int BlackBishops = GetBitCount(position.GetPieceBB(BLACK_BISHOP));
    int WhiteKnights = GetBitCount(position.GetPieceBB(WHITE_KNIGHT));
    int BlackKnights = GetBitCount(position.GetPieceBB(BLACK_KNIGHT));
    int WhiteMinor = WhiteBishops + WhiteKnights;
    int BlackMinor = BlackBishops + BlackKnights;

    if (WhiteMinor == 0 && BlackMinor == 0) return true;	//1
    if (WhiteMinor == 1 && BlackMinor == 0) return true;	//2
    if (WhiteMinor == 0 && BlackMinor == 1) return true;	//2

    return false;
}

bool IsBlockade(const Position& position)
{
    if (position.GetAllPieces() != (position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING) | position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN)))
        return false;

    if (position.GetTurn() == WHITE)
        return BlackBlockade(position.GetPieceBB(WHITE_PAWN), position.GetPieceBB(BLACK_PAWN));
    else
        return WhiteBlockade(position.GetPieceBB(WHITE_PAWN), position.GetPieceBB(BLACK_PAWN));
}

/*addapted from chess programming wiki code example*/
bool BlackBlockade(uint64_t wPawns, uint64_t bPawns)
{
    uint64_t fence = wPawns & (bPawns >> 8);					//blocked white pawns
    if (GetBitCount(fence) < 3)
        return false;

    fence |= ((bPawns & ~(FileBB[FILE_A])) >> 9);				//black pawn attacks
    fence |= ((bPawns & ~(FileBB[FILE_H])) >> 7);

    uint64_t flood = RankBB[RANK_1] | allBitsBelow[bitScanForward(fence)];
    uint64_t above = allBitsAbove[bitScanReverse(fence)];

    if ((wPawns & ~fence) != 0)
        return false;

    while (true) {
        uint64_t temp = flood;
        flood |= ((flood & ~(FileBB[FILE_A])) >> 1) | ((flood & ~(FileBB[FILE_H])) << 1);	//Add left and right 
        flood |= (flood << 8) | (flood >> 8);												//up and down
        flood &= ~fence;

        if (flood == temp)
            break; /* Fill has stopped, blockage? */
        if (flood & above) /* break through? */
            return false; /* yes, no blockage */
        if (flood & bPawns) {
            bPawns &= ~flood;  /* "capture" undefended black pawns */
            fence = wPawns & (bPawns >> 8);
            if (GetBitCount(fence) < 3)
                return false;

            fence |= ((bPawns & ~(FileBB[FILE_A])) >> 9);
            fence |= ((bPawns & ~(FileBB[FILE_H])) >> 7);
        }
    }

    return true;
}

bool WhiteBlockade(uint64_t wPawns, uint64_t bPawns)
{
    uint64_t fence = bPawns & (wPawns << 8);
    if (GetBitCount(fence) < 3)
        return false;

    fence |= ((wPawns & ~(FileBB[FILE_A])) << 7);
    fence |= ((wPawns & ~(FileBB[FILE_H])) << 9);

    uint64_t flood = RankBB[RANK_8] | allBitsAbove[bitScanReverse(fence)];
    uint64_t above = allBitsBelow[bitScanForward(fence)];

    if ((bPawns & ~fence) != 0)
        return false;

    while (true) {
        uint64_t temp = flood;
        flood |= ((flood & ~(FileBB[FILE_A])) >> 1) | ((flood & ~(FileBB[FILE_H])) << 1);	//Add left and right 
        flood |= (flood << 8) | (flood >> 8);												//up and down
        flood &= ~fence;

        if (flood == temp)
            break; /* Fill has stopped, blockage? */
        if (flood & above) /* break through? */
            return false; /* yes, no blockage */
        if (flood & bPawns) {
            bPawns &= ~flood;  /* "capture" undefended black pawns */
            fence = bPawns & (wPawns << 8);
            if (GetBitCount(fence) < 3)
                return false;

            fence |= ((wPawns & ~(FileBB[FILE_A])) << 7);
            fence |= ((wPawns & ~(FileBB[FILE_H])) << 9);
        }
    }
    return true;
}

template<typename Numeric, typename Generator = std::mt19937>
Numeric random(Numeric from, Numeric to)
{
    thread_local static Generator gen(std::random_device{}());

    using dist_type = typename std::conditional
        <
        std::is_integral<Numeric>::value
        , std::uniform_int_distribution<Numeric>
        , std::uniform_real_distribution<Numeric>
        >::type;

    thread_local static dist_type dist;

    return dist(gen, typename dist_type::param_type{ from, to });
}

trainingPoint::trainingPoint(std::array<double, INPUT_NEURONS> input, double gameResult)
{
    inputs = input;
    result = gameResult;
}

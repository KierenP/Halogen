#include "Network.h"


void Learn()
{
    Network net = InitNetwork();
    net.Learn();
}

Network InitNetwork(std::string file)
{
    std::ifstream stream(file);

    if (!stream.is_open())
    {
        std::cout << "info string Could not load network file: " << file << std::endl;
        std::cout << "info string random weights initialization!" << std::endl;
        return CreateRandom({ INPUT_NEURONS, 32, 1 });
    }

    std::string line;

    std::vector<std::vector<double>> weights;
    std::vector<size_t> LayerNeurons;
    weights.push_back({});

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
    return Network(weights, LayerNeurons);
}

Network InitNetwork()
{
    std::string line;
    std::istringstream stream(oss.str());

    std::vector<std::vector<double>> weights;
    std::vector<size_t> LayerNeurons;
    weights.push_back({});

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

    return Network(weights, LayerNeurons);
}

Neuron::Neuron(std::vector<double> Weight, double Bias)
{
    weights = Weight;
    bias = Bias;
}

double Neuron::FeedForward(std::vector<double>& input) const
{
    assert(input.size() == weights.size());
    return std::inner_product(input.begin(), input.end(), weights.begin(), 0.0) + bias;
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
        neurons.push_back(Neuron(std::vector<double>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1)));
    }

    zeta = std::vector<double>(NeuronCount, 0);
    alpha = std::vector<double>(NeuronCount, 0);
}

std::vector<double> HiddenLayer::FeedForward(std::vector<double>& input)
{
    for (int i = 0; i < neurons.size(); i++)
    {
        zeta[i] = neurons.at(i).FeedForward(input);
    }

    activation(zeta, alpha);
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

void HiddenLayer::activation(std::vector<double>& in, std::vector<double>& out)
{
    assert(in.size() == out.size());

    for (int i = 0; i < in.size(); i++)
    {
        out[i] = std::max(0.0, in[i]);
    }
}

std::vector<double> HiddenLayer::activationPrime(std::vector<double> x)
{
    std::vector<double> ret;
    ret.reserve(x.size());

    for (int i = 0; i < x.size(); i++)
    {
        ret.push_back(x[i] >= 0);
    }

    return ret;
}

void HiddenLayer::ApplyDelta(std::vector<std::pair<size_t, double>>& delta)
{
    assert(delta.size() == INPUT_NEURONS);
    int neuronCount = zeta.size();

    for (int index = 0; index < delta.size(); index++)
    {
        for (int neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += neurons[neuron].weights[delta[index].first] * delta[index].second;
        }
    }

    activation(zeta, alpha);
}

void HiddenLayer::ApplyInverseDelta(std::vector<std::pair<size_t, double>>& delta)
{
    assert(delta.size() == INPUT_NEURONS);
    int neuronCount = zeta.size();

    for (int index = 0; index < delta.size(); index++)
    {
        for (int neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] -= neurons[neuron].weights[delta[index].first] * delta[index].second;
        }
    }

    activation(zeta, alpha);
}

Network::Network(std::vector<std::vector<double>> inputs, std::vector<size_t> NeuronCount) : outputNeuron(std::vector<double>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    assert(inputs.size() == NeuronCount.size());

    alpha = 0;
    zeta = 0;
    inputNeurons = NeuronCount.at(0);

    for (size_t i = 1; i < NeuronCount.size() - 1; i++)
    {
        hiddenLayers.push_back(HiddenLayer(inputs.at(i), NeuronCount.at(i)));
    }
}

double Network::FeedForward(std::vector<double> inputs)
{
    assert(inputs.size() == inputNeurons);

    for (size_t i = 0; i < hiddenLayers.size(); i++)
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs);
    }

    zeta = outputNeuron.FeedForward(inputs);
    alpha = 1 / (1 + exp(-0.0087 * zeta));  //-0.0087 chosen to mymic the previous evaluation function

    return zeta;
}

double Network::Backpropagate(trainingPoint data, double learnRate)
{
    std::vector<double> inputParams(data.inputs.begin(), data.inputs.end());

    FeedForward(inputParams);
    double delta_l = 0.0087 * (alpha - data.result) * (alpha) * (1 - alpha); //0.0087 chosen to mymic the previous evaluation function

    std::vector<double> tmp = hiddenLayers.at(0).activationPrime(hiddenLayers.at(0).zeta);
    std::vector<double> tmp2;
    std::vector<double> delta_l_minus_one;

    std::transform(outputNeuron.weights.begin(), outputNeuron.weights.end(), std::back_inserter(tmp2), [&delta_l](auto& c) {return c * delta_l; });
    std::transform(tmp.begin(), tmp.end(), tmp2.begin(), std::back_inserter(delta_l_minus_one), std::multiplies<double>());

    for (int i = 0; i < delta_l_minus_one.size(); i++)
    {
        for (int weight = 0; weight < hiddenLayers.at(0).neurons.at(i).weights.size(); weight++)
        {
            hiddenLayers.at(0).neurons.at(i).weights.at(weight) -= delta_l_minus_one.at(i) * inputParams.at(weight) * learnRate;
        }
        hiddenLayers.at(0).neurons.at(i).bias -= delta_l_minus_one.at(i) * learnRate;
    }

    for (int i = 0; i < outputNeuron.weights.size(); i++)
    {
        outputNeuron.weights.at(i) -= delta_l * hiddenLayers[0].alpha.at(i) * learnRate;
    }
    outputNeuron.bias -= delta_l * learnRate;

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
        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(data), std::end(data), rng);

        double error = 0;

        for (int point = 0; point < data.size(); point++)
        {
            error += Backpropagate(data[point], 1);
        }

        error /= data.size();

        std::cout << "Finished epoch: " << epoch << " MSE: " << 2 * error << std::endl;

        if (epoch % 100 == 0)
            WriteToFile();
    }

    WriteToFile();
}

void Network::ApplyDelta(std::vector<std::pair<size_t, double>>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta);
}

void Network::ApplyInverseDelta(std::vector<std::pair<size_t, double>>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyInverseDelta(delta);
}

double Network::QuickEval()
{
    std::vector<double> inputs = hiddenLayers.at(0).alpha;

    for (size_t i = 1; i < hiddenLayers.size(); i++)    //skip first layer
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs);
    }

    zeta = outputNeuron.FeedForward(inputs);
    alpha = 1 / (1 + exp(-0.0087 * zeta));  //-0.0087 chosen to mymic the previous evaluation function

    return zeta;
}

trainingPoint::trainingPoint(std::array<bool, INPUT_NEURONS> input, double gameResult)
{
    inputs = input;
    result = gameResult;
}

Network CreateRandom(std::vector<size_t> NeuronCount)
{
    std::vector<std::vector<double>> inputs;
    inputs.push_back({});

    size_t prevLayerNeurons = NeuronCount[0];

    for (int layer = 1; layer < NeuronCount.size() - 1; layer++)
    {
        std::vector<double> input;
        for (int i = 0; i < (prevLayerNeurons + 1) * NeuronCount[layer]; i++)
        {
            if (i % prevLayerNeurons == 0 && i != 0)
                input.push_back(0);
            else
                input.push_back(1);
        }
        inputs.push_back(input);
        prevLayerNeurons = NeuronCount[layer];
    }

    //connections from last hidden to output
    std::vector<double> input;
    for (int i = 0; i < (prevLayerNeurons + 1) * NeuronCount.back(); i++)
    {
        if (i % prevLayerNeurons == 0 && i != 0)
            input.push_back(0);
        else
            input.push_back(1);
    }
    inputs.push_back(input);

    return Network(inputs, NeuronCount);
}


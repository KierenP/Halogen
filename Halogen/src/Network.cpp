#include "Network.h"

float sqrtfast(const float x);
float InvSqrt(float x);

void Learn()
{
    Network net = InitNetwork("C:\\HalogenWeights\\UcPfefCODx.network");
    net.Learn();
}

Network InitNetwork(std::string file)
{
    std::ifstream stream(file);

    if (!stream.is_open())
    {
        std::cout << "info string Could not load network file: " << file << std::endl;
        std::cout << "info string random weights initialization!" << std::endl;
        return CreateRandom({ INPUT_NEURONS, 256, 1 });
    }

    std::string line;

    std::vector<std::vector<float>> weights;
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
            std::vector<float> layerWeights;

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
            std::vector<float> layerWeights;
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

    std::vector<std::vector<float>> weights;
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
            std::vector<float> layerWeights;

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
            std::vector<float> layerWeights;
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

Neuron::Neuron(std::vector<float> Weight, float Bias) : grad(Weight.size() + 1, 0)
{
    weights = Weight;
    bias = Bias;
}

float Neuron::FeedForward(std::vector<float>& input) const
{
    assert(input.size() == weights.size());

    float ret = bias;

    for (size_t i = 0; i < input.size(); i++)
    {
        ret += std::max(0.f, input[i]) * weights[i];
    }

    return ret;
}

void Neuron::Backpropogate(float delta_l, const std::vector<float>& prev_weights, float learnRate)
{
    for (size_t weight = 0; weight < weights.size(); weight++)
    {
        float new_grad = delta_l * std::max(0.f, prev_weights.at(weight)); //ReLU activation calculated here

        grad.at(weight) += new_grad * new_grad;
        weights.at(weight) -= new_grad * learnRate *InvSqrt(grad.at(weight) + 10e-8);
    }

    float new_grad = delta_l;
    grad.at(weights.size()) += new_grad * new_grad;
    bias -= new_grad * learnRate *InvSqrt(grad.at(weights.size()) + 10e-8);
}

void Neuron::WriteToFile(std::ofstream& myfile)
{
    for (size_t i = 0; i < weights.size(); i++)
    {
        myfile << weights.at(i) << " ";
    }
    myfile << bias << "\n";
}

HiddenLayer::HiddenLayer(std::vector<float> inputs, size_t NeuronCount)
{
    assert(inputs.size() % NeuronCount == 0);

    size_t WeightsPerNeuron = inputs.size() / NeuronCount;

    for (size_t i = 0; i < NeuronCount; i++)
    {
        neurons.push_back(Neuron(std::vector<float>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1)));
    }

    for (size_t i = 0; i < WeightsPerNeuron - 1; i++)
    {
        for (size_t j = 0; j < NeuronCount; j++)
        {
            weightTranspose.push_back(neurons.at(j).weights.at(i));
        }
    }

    zeta = std::vector<float>(NeuronCount, 0);
}

std::vector<float> HiddenLayer::FeedForward(std::vector<float>& input)
{
    for (size_t i = 0; i < neurons.size(); i++)
    {
        zeta[i] = neurons.at(i).FeedForward(input);
    }

    return zeta;
}

void HiddenLayer::Backpropogate(const std::vector<float>& delta_l, const std::vector<float>& prev_weights, float learnRate)
{
    assert(delta_l.size() == neurons.size());

    for (size_t i = 0; i < delta_l.size(); i++)
    {
        neurons.at(i).Backpropogate(delta_l.at(i), prev_weights, learnRate);
    }
}

void HiddenLayer::WriteToFile(std::ofstream& myfile)
{
    myfile << "HiddenLayerNeurons " << neurons.size() << "\n";

    for (size_t i = 0; i < neurons.size(); i++)
    {
        neurons.at(i).WriteToFile(myfile);
    }
}

void HiddenLayer::activation(const std::vector<float>& in, std::vector<float>& out)
{
    assert(in.size() == out.size());

    for (size_t i = 0; i < in.size(); i++)
    {
        out[i] = std::max(0.f, in[i]);
    }
}

std::vector<float> HiddenLayer::activationPrime(std::vector<float> x)
{
    std::vector<float> ret;
    ret.reserve(x.size());

    for (size_t i = 0; i < x.size(); i++)
    {
        ret.push_back(x[i] >= 0);
    }

    return ret;
}

void HiddenLayer::ApplyDelta(std::vector<deltaPoint>& deltaVec, float forward)
{
    assert(deltaVec.size() == INPUT_NEURONS);
    size_t neuronCount = zeta.size();
    size_t deltaCount = deltaVec.size();

    for (size_t index = 0; index < deltaCount; index++)
    {
        float deltaValue = deltaVec[index].delta * forward;
        size_t weightTransposeIndex = deltaVec[index].index * neuronCount;

        for (size_t neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += weightTranspose[weightTransposeIndex + neuron] * deltaValue;
        }
    }
}

Network::Network(std::vector<std::vector<float>> inputs, std::vector<size_t> NeuronCount) : outputNeuron(std::vector<float>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
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

float Network::FeedForward(std::vector<float> inputs)
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

float Network::Backpropagate(trainingPoint data, float learnRate)
{
    std::vector<float> inputParams(data.inputs.begin(), data.inputs.end());

    FeedForward(inputParams);
    //return 0.5 * (alpha - data.result) * (alpha - data.result);   //if you just want to calculate error without training then do this

    //we choose a vector for this because delta_l will have a value for each neuron in the layer later
    std::vector<float> delta_l = { 0.0087f * (alpha - data.result) * (alpha) * (1 - alpha) }; //0.0087 chosen to mymic the previous evaluation function
    outputNeuron.Backpropogate(delta_l[0], hiddenLayers.back().zeta, learnRate);

    for (int layer = hiddenLayers.size() - 1; layer >= 0; layer--)
    {
        std::vector<float> tmp = hiddenLayers.at(layer).activationPrime(hiddenLayers.at(layer).zeta);  //1 if this neuron is on and 0 if its not
        std::vector<float> tmp2;

        for (int neuron = 0; neuron < hiddenLayers.at(layer).neurons.size(); neuron++)
        {
            if (layer == hiddenLayers.size() - 1)
            {
                tmp2.push_back(outputNeuron.weights.at(neuron) * delta_l.at(0));
            }
            else
            {
                float val = 0;

                for (int nextLayerNeuron = 0; nextLayerNeuron < hiddenLayers.at(layer + 1).neurons.size(); nextLayerNeuron++)
                {
                    val += delta_l.at(nextLayerNeuron) * hiddenLayers.at(layer + 1).neurons.at(nextLayerNeuron).weights.at(neuron);
                }

                tmp2.push_back(val);
            }
        }

        delta_l.clear();
        std::transform(tmp.begin(), tmp.end(), tmp2.begin(), std::back_inserter(delta_l), std::multiplies<float>()); //element-wise multiplication of delta_l = tmp * tmp2;

        if (layer > 0)
            hiddenLayers.at(layer).Backpropogate(delta_l, hiddenLayers.at(layer - 1).zeta, learnRate); //get the previous layers activation
        else
            hiddenLayers.at(layer).Backpropogate(delta_l, inputParams, learnRate);                      //if at first hidden layer then just use input activation
    }
    
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

    for (size_t i = 0; i < hiddenLayers.size(); i++)
    {
        hiddenLayers.at(i).WriteToFile(myfile);
    }

    myfile << "OutputLayer\n";
    for (size_t i = 0; i < outputNeuron.weights.size(); i++)
    {
        myfile << outputNeuron.weights.at(i) << " ";
    }
    myfile << outputNeuron.bias << "\n";

    myfile.close();

}

void Network::Learn()
{
    //AddExtraNullLayer(32);
    //WriteToFile();

    //std::vector<trainingPoint> data = Stockfish3PerDataset();
    std::vector<trainingPoint> data = quietlabeledDataset();

    for (int epoch = 1; epoch <= 100000; epoch++)
    {
        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(data), std::end(data), rng);

        float error = 0;

        for (size_t point = 0; point < data.size() / 100; point++)
        {
            error += Backpropagate(data[point], 0.1);
        }

        error /= data.size() / 100;

        std::cout << "Finished epoch: " << epoch << " MSE: " << 2 * error << std::endl;

        if (epoch % 100 == 0)
            WriteToFile();
    }

    WriteToFile();
}

void Network::ApplyDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta, 1);
}

void Network::ApplyInverseDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta, -1);
}

float Network::QuickEval()
{
    std::vector<float>& inputs = hiddenLayers.at(0).zeta;

    for (size_t i = 1; i < hiddenLayers.size(); i++)    //skip first layer
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs);
    }

    zeta = outputNeuron.FeedForward(inputs);
    alpha = 1 / (1 + exp(-0.0087 * zeta));  //-0.0087 chosen to mymic the previous evaluation function

    return zeta;
}

trainingPoint::trainingPoint(std::array<bool, INPUT_NEURONS> input, float gameResult)
{
    inputs = input;
    result = gameResult;
}

Network CreateRandom(std::vector<size_t> NeuronCount)
{
    std::random_device rd;
    std::mt19937 e2(rd());

    std::vector<std::vector<float>> inputs;
    inputs.push_back({});

    size_t prevLayerNeurons = NeuronCount[0];

    for (size_t layer = 1; layer < NeuronCount.size() - 1; layer++)
    {
        //see https://arxiv.org/pdf/1502.01852v1.pdf eq (10) for theoretical significance of w ~ N(0, sqrt(2/n))
        std::normal_distribution<> dist(0.0, sqrt(2.0 / static_cast<float>(NeuronCount[layer])));

        std::vector<float> input;
        for (size_t i = 0; i < (prevLayerNeurons + 1) * NeuronCount[layer]; i++)
        {
            if ((i + 1) % (prevLayerNeurons + 1) == 0)
                input.push_back(0);
            else
                input.push_back(dist(e2));
        }
        inputs.push_back(input);
        prevLayerNeurons = NeuronCount[layer];
    }

    std::normal_distribution<> dist(0.0, sqrt(2.0 / static_cast<float>(NeuronCount.back())));

    //connections from last hidden to output
    std::vector<float> input;
    for (size_t i = 0; i < (prevLayerNeurons + 1) * NeuronCount.back(); i++)
    {
        if ((i + 1) % (prevLayerNeurons + 1) == 0)
            input.push_back(0);
        else
            input.push_back(dist(e2));
    }
    inputs.push_back(input);

    return Network(inputs, NeuronCount);
}

void Network::AddExtraNullLayer(size_t neurons)
{
    std::vector<float> weights;

    for (int j = 0; j < neurons; j++)
    {
        for (int i = 0; i < hiddenLayers.back().neurons.size() + 1; i++)
        {
            if (i == j)
                weights.push_back(1);
            else
                weights.push_back(0);
        }
    }

    hiddenLayers.push_back(HiddenLayer(weights, neurons));
}

//from https://www.codeproject.com/Articles/69941/Best-Square-Root-Method-Algorithm-Function-Precisi
float sqrtfast(const float x)
{
    union
    {
        int i;
        float x;
    } u;

    u.x = x;
    u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
    return u.x;
}

//from https://math.stackexchange.com/questions/800815/fast-inverse-square-root-trick
float InvSqrt(float x)
{
    float xhalf = 0.5f * x; // store 0.5x since x will be changed
    int i = *(int*)&x; // convert to binary integer
    i = 0x5f3759df - (i >> 1); // the smart trick
    x = *(float*)&i; // convert binary back to decimal
    x = x * (1.5f - xhalf * x * x); // Newton-Raphson step
    return x;
}
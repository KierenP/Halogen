#include <iostream>
#include <string>

#include "Network.h"
#include "uci/uci.h"

const std::string version = "11.21.0";

void PrintVersion()
{
    std::cout << "Halogen " << version;

#if defined(_WIN64) or defined(__x86_64__)
    std::cout << " x64";
#endif

#if defined(USE_AVX512_VNNI)
    std::cout << " AVX512_VNNI";
#elif defined(USE_AVX512)
    std::cout << " AVX512";
#elif defined(USE_AVX2)
    std::cout << " AVX2";
#elif defined(USE_AVX)
    std::cout << " AVX";
#elif defined(USE_SSE4)
    std::cout << " SSE4";
#endif

#if defined(USE_PEXT)
    std::cout << " PEXT";
#endif

    std::cout << "\n";
}

int main(int argc, char* argv[])
{
    Uci uci { version };
    Network::Init();

    PrintVersion();
    std::string line;

    for (int i = 1; i < argc; i++) // read any command line input as a regular UCI instruction
    {
        line += argv[i];
        line += " ";
    }

    if (!line.empty())
    {
        uci.process_input(line);
        return 0;
    }

    while (getline(std::cin, line))
    {
        uci.process_input(line);
        line.clear();
    }

    return 0;
}

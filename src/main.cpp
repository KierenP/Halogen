#include <iostream>
#include <string>

#include "Network.h"
#include "uci/uci.h"

constexpr std::string_view version = "11.22.0";

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

    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);
    Network::Init();
    Uci uci { version };

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

    while (!uci.quit && getline(std::cin, line))
    {
        uci.process_input(line);
    }

    return 0;
}

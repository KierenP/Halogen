#include <iostream>
#include <string_view>

#include "Cuckoo.h"
#include "uci/uci.h"

constexpr std::string_view version = "12.35.2";

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
    PrintVersion();
    std::ios::sync_with_stdio(false);
    Uci uci { version };

    cuckoo::init();

    // read any command line input as a regular UCI instruction
    for (int i = 1; i < argc; i++)
    {
        uci.process_input(argv[i]);
    }

    if (argc != 1)
    {
        return 0;
    }

    uci.process_input_stream(std::cin);
    return 0;
}

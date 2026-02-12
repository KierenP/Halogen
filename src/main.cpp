#include <iostream>
#include <string>
#include <string_view>

#include "search/cuckoo.h"
#include "search/thread.h"
#include "test/static_exchange_evaluation_test.h"
#include "uci/uci.h"
#include "utility/arch.h"

constexpr std::string_view version = "16.1.0";

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);

    Cuckoo::init();

#ifndef NDEBUG
    static_exchange_evaluation_test();
#endif

    std::cout << fmt_version_platform_arch(version) << std::endl;

    UCI::UciOutput output;
    SearchThreadPool pool { output, 1 };
    UCI::Uci uci { version, pool, output };

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

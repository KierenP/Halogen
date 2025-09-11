#include "weights.hpp"

#include "network/arch.hpp"

#include "third-party/incbin/incbin.h"

#undef INCBIN_ALIGNMENT
#define INCBIN_ALIGNMENT 64
INCBIN(Net, EVALFILE);

[[maybe_unused]] auto verify_network_size = []
{
    if (sizeof(NetworkWeights) != gNetSize)
    {
        std::cout << "Error: embedded network is not the expected size. Expected " << sizeof(NetworkWeights)
                  << " bytes actual " << gNetSize << " bytes." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return true;
}();

NumaLocalAllocationManager<NetworkWeights> network_weights_accessor { reinterpret_cast<const NetworkWeights*>(
    gNetData) };

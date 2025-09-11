#pragma once

#include "network/arch.hpp"
#include "numa/numa.h"

extern NumaLocalAllocationManager<NetworkWeights> network_weights_accessor;
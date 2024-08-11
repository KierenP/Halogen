#pragma once

#include "../uci/uci.h"

#include <chrono>
#include <string_view>

void datagen(Uci& uci, std::string_view output_path, std::chrono::seconds duration);

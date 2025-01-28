#pragma once

#include "HyperWaveTable.hpp"
#include <string>
#include <cstdlib>

struct HyperToken* parse(std::string input, std::string& err);

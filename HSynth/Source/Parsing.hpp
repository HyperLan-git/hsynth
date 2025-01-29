#pragma once

#include "HyperWaveTable.hpp"
#include <string>
#include <cstdlib>

/**
 * Please do std::setlocale(LC_NUMERIC, "C"); before calling this
 * (so that dots are treated as decimal point)
 */
struct HyperToken* parse(std::string input, std::string& err);

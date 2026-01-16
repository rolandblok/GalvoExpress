#pragma once

#include <Arduino.h>
#include <vector>

#include "mytypes.hpp"

using std::vector;  // Now you can use 'vector' directly


bool save_pattern(const vector<patern_point>& data);
bool load_pattern(vector<patern_point>& data);
void delete_pattern();

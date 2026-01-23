#pragma once

#include <Arduino.h>
#include <vector>

#include "mytypes.hpp"

using std::vector;  // Now you can use 'vector' directly

void mystore_init();
bool mystore_save_pattern(const vector<patern_point>& data);
bool mystore_load_pattern(vector<patern_point>& data);
void mystore_delete_pattern();

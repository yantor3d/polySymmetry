/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLYSYMMETRY_UTIL_H
#define POLYSYMMETRY_UTIL_H

#include <algorithm>
#include <vector>

using namespace std;

vector<int> intersection(vector<int> &a, vector<int> &b);
bool        contains(vector<int> &items, int &value);

#endif
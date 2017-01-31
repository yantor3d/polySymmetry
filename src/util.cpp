/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "util.h"

#include <algorithm>
#include <vector>

using namespace std;

vector<int> intersection(vector<int> &a, vector<int> &b)
{
    vector<int> result(a.size() + b.size());
    vector<int>::iterator it; 

    it = set_intersection(
        a.begin(),
        a.end(),
        b.begin(),
        b.end(),
        result.begin()
    );

    result.resize(it - result.begin());

    return result;    
}

bool contains(vector<int> &items, int &value)
{
    return find(items.begin(), items.end(), value) != items.end();
}
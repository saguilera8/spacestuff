#pragma once
#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

// A 3-dimensional vector.
struct Vector3
{
    float x;
    float y;
    float z;
};

// Generate a set of legal beams to cover as many users as possible.
void solve(const Vector3 *users, const int num_users, const Vector3 *sats, const int num_sats);

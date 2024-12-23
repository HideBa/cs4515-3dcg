#pragma once
#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <span>
#include <tuple>
#include <vector>

struct Tree;

////////////////// Exercise 1 ////////////////////////////////////
std::pair<float, float> Statistics(std::span<const float> values)
{
    return { 0.0f, 0.0f };
}
//////////////////////////////////////////////////////////////////

////////////////// Exercise 2 ////////////////////////////////////
class TreeVisitor {
public:
    float visitTree(const Tree& tree, bool countOnlyEvenLevels)
    {
        return 0.0f;
    }
};
//////////////////////////////////////////////////////////////////

////////////////// Exercise 3 ////////////////////////////////////
class Complex {
public:
    Complex(float real_, float imaginary_)
    {
    }


    float real, im;
};
//////////////////////////////////////////////////////////////////

////////////////// Exercise 4 ////////////////////////////////////
float WaterLevels(std::span<const float> heights)
{
    return 0.0f;
}
//////////////////////////////////////////////////////////////////

////////////////// Exercise 5 ////////////////////////////////////
using location = std::pair<int, int>;

int Labyrinth(const std::set<std::pair<location, location>>& labyrinth, int size)
{
    return 0;
}
//////////////////////////////////////////////////////////////////

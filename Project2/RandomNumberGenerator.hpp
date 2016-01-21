// Class that generates random numbers using a distribution
// Inspiration from here: http://stackoverflow.com/questions/13445688/how-do-generate-a-random-number-in-c
// and here: http://ideone.com/EA4AYU
#include <random>
#include "Helper.h"

using namespace std;
class RandomNumberGenerator{

private:
    std::mt19937 generator;
    std::uniform_int_distribution<UINT64> distribution;

public:
    // constructor for the random number
    int high, low;
    RandomNumberGenerator::RandomNumberGenerator(int h, int l) :
        generator(std::random_device()()),
        distribution(l, h) {high = h, low = l;}

    // Generates a random number
    UINT64 createRandomNumber(){ return distribution(generator); }
};
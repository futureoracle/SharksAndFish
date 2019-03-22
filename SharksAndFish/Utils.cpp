#include"stdafx.h"
#include"Utils.h"

#include<random>

//Performs the required initialization for the util functions
void Utils::initUtils(int randomSeed)
{
	//Set the seed for the random number generator
	//Setting a constant number as the seed will allow us to replicate the results
	srand(randomSeed);
}

//Generates a random number between min and max, both inclusive; doesn't work with negative values
int Utils::getRandomNumber(int min, int max)
{
	static const double fraction = 1.0 / (1.0 + RAND_MAX);

	return min + static_cast<int>((max - min + 1) * (rand() * fraction));
}
#include"stdafx.h"
#include"Grid.h"
#include"Utils.h"

#include<iostream>

//Instantiates a grid with the given number of rows and columns
Grid::Grid(int rows, int cols)
{
	//Add 2 extra rows and columns to make space for ghost cells
	this->rows = rows + 2;
	this->cols = cols + 2;

	//Allocates memory for the two grid variables
	allocateMemoryToGridVariables();

	//Fill the grid with values
	initGrid(25, 25);
}

//Prints the contents of the current grid to the console in the form of characters
void Grid::printGridToConsole(char shark, char fish, char water)
{
	//In the for loops, the first and last row and column are excluded because they are ghost cells
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			if (currentGrid[row][col] > 0)
				std::cout << fish;
			else if (currentGrid[row][col] < 0)
				std::cout << shark;
			else
				std::cout << water;
		}
		std::cout << std::endl;
	}
}

//Prints the grid's stats, such as the count of shark and fish
void Grid::printStatsToConsole()
{
	int sharkCount = 0, fishCount = 0, waterCount = 0;

	//In the for loops, the first and last row and column are excluded because they are ghost cells
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			if (currentGrid[row][col] > 0)
				++fishCount;
			else if (currentGrid[row][col] < 0)
				++sharkCount;
			else
				++waterCount;
		}
	}

	std::cout << "Number of sharks: " << sharkCount << "\nNumber of fish: " << fishCount;
	std::cout << "\nNumber of water cells: " << waterCount << std::endl;
}

//======PRIVATE MEMBERS===========================================================================

//Allocates new memory to currentGrid and nextCalculatedGrid based on this Grid's rows and cols
void Grid::allocateMemoryToGridVariables()
{
	currentGrid = new int*[rows];
	for (int row = 0; row < rows; ++row)
		currentGrid[row] = new int[cols];

	nextCalculatedGrid = new int*[rows];
	for (int row = 0; row < rows; ++row)
		nextCalculatedGrid[row] = new int[cols];
}

//Initializes the grid randomly
void Grid::initGrid()
{
	//In the for loops, the first and last row and column are excluded because they are ghost cells
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			//because getRandomNumber cannot return negative values, if it returns 2 we count it as a shark
			currentGrid[row][col] = Utils::getRandomNumber(0, 2);
			if (currentGrid[row][col] == 2)
				currentGrid[row][col] = -1;
		}
	}
}

//Initializes the grid and tries to keep the percentage of sharks, fish, and water cells as specified in the parameters
//Both the parameters must be between 0 and 100, and their sum must not exceed 100
void Grid::initGrid(int sharkPercent, int fishPercent)
{
	int sharkUpperLimit = sharkPercent;
	int fishUpperLimit = sharkPercent + fishPercent;

	//In the for loops, the first and last row and column are excluded because they are ghost cells
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			int temp = Utils::getRandomNumber(1, 100);
			if (temp <= sharkUpperLimit)
				currentGrid[row][col] = -1;	//shark
			else if (temp <= fishUpperLimit)
				currentGrid[row][col] = 1;	//fish
			else
				currentGrid[row][col] = 0;	//water
		}
	}
}
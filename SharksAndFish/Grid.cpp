#include"stdafx.h"
#include"Grid.h"
#include"Utils.h"

#include<iostream>
#include<ctime>

//Instantiates a grid with the given number of rows and columns
Grid::Grid(int rows, int cols)
{
	//Add 2 extra rows and columns to make space for ghost cells
	this->rows = rows + 2;
	this->cols = cols + 2;

	//Allocates memory for the two grid variables
	allocateMemoryToGridVariables();

	//Fill the grid with values
	initGrid(20, 70);
}

Grid::~Grid()
{
	for (int i = 0; i < rows; ++i)
	{
		delete[] currentGrid[i];
		delete[] nextCalculatedGrid[i];
	}
	delete[] currentGrid;
	delete[] nextCalculatedGrid;
}

//Prints the contents of the current grid to the console in the form of characters
void Grid::printToConsole(char shark, char fish, char water)
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

//Runs the grid according to the rules for nIterations, and returns the time it took to complete
float Grid::runTest(int nIterations)
{
	float startTime = clock();
	for (int i = 0; i < nIterations; ++i)
	{
		calculateNextGridState();
		goToNextGridState();
	}
	return clock() - startTime;
}

//Equates the currentGrid to the nextCalculatedGrid
void Grid::goToNextGridState()
{
	//In the for loops, the first and last row and column are excluded because they are ghost cells, which are not copied
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			currentGrid[row][col] = nextCalculatedGrid[row][col];
		}
	}
}

//Evaluates the rules of the celluar automata and puts values in the nextCalculatedGrid based on them
void Grid::calculateNextGridState()
{
	updateGhostCells();

	int nSharkNeighbours, nFishNeighbours, nBreedingSharks, nBreedingFish;
	//In the for loops, the first and last row and column are excluded because they are ghost cells
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			//Get the neighbours' counts
			getNeighbourCount(row, col, nSharkNeighbours, nFishNeighbours);
			nBreedingFish = nFishNeighbours % 10;
			nBreedingSharks = nSharkNeighbours % 10;
			nFishNeighbours /= 10;
			nSharkNeighbours /= 10;

			if (currentGrid[row][col] == 0)	//cell is empty
			{
				//Breeding Rule
				if (nFishNeighbours >= 4 && nBreedingFish >= 3 && nSharkNeighbours < 4)	//fish can breed
					nextCalculatedGrid[row][col] = 1;	//spawn fish
				else if (nSharkNeighbours >= 4 && nBreedingSharks >= 3 && nFishNeighbours < 4)	//shark can spawn
					nextCalculatedGrid[row][col] = -1;	//spawn shark
			}
			else if (currentGrid[row][col] > 0)	//cell has a fish
			{
				if (nSharkNeighbours >= 5)	//shark food; fish gets eaten
					nextCalculatedGrid[row][col] = 0;
				else if (nFishNeighbours == 8)	//overpopulation; fish dies
					nextCalculatedGrid[row][col] = 0;
				else if (currentGrid[row][col] == 10)	//max age reached; fish dies
					nextCalculatedGrid[row][col] = 0;
				else	//nothing happens to the fish 
					nextCalculatedGrid[row][col] = ++currentGrid[row][col];	//increment fish's age
			}
			else	//cell has a shark
			{
				if (nSharkNeighbours >= 6 && nFishNeighbours == 0)	//starvation; shark dies
					nextCalculatedGrid[row][col] = 0;
				else if (Utils::getRandomNumber(1, 32) == 1)	//random causes; shark dies. bad luck.
					nextCalculatedGrid[row][col] = 0;
				else if (currentGrid[row][col] == -20)	//reached max age; shark dies
					nextCalculatedGrid[row][col] = 0;
				else	//nothing happens, shark survives; increment age
					nextCalculatedGrid[row][col] = --currentGrid[row][col];
			}
		}
	}
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

//Updates the ghost cells of the current grid 
void Grid::updateGhostCells()
{
	//rows
	for (int col = 1; col < cols - 1; ++col)
	{
		//top row
		currentGrid[0][col] = currentGrid[rows - 2][col];
		//bottom row
		currentGrid[rows - 1][col] = currentGrid[1][col];
	}

	//columns
	for (int row = 1; row < rows - 1; ++row)
	{
		//left column
		currentGrid[row][0] = currentGrid[row][cols - 2];
		//right column
		currentGrid[row][cols - 1] = currentGrid[row][1];
	}

	//corners
	currentGrid[0][0] = currentGrid[rows - 2][cols - 2];	//top-left ghost = actual bottom-right
	currentGrid[0][cols - 1] = currentGrid[rows - 2][1];	//top-right ghost = actual bottom-left
	currentGrid[rows - 1][0] = currentGrid[1][cols - 2];	//bottom-left ghost = actual top-right
	currentGrid[rows - 1][cols - 1] = currentGrid[1][1];	//bottom-right ghost = actual top-left
}

//Calculates the number of neighbours of each type a cell has, and returns them through the arguments
//Both of the arguments will be stored in the form 10 * number + number that can breed
//eg - If there are 5 fish neighbours out of which 3 are of breeding age, the value of outFishCount will be stored as
//10 * 5 + 3 = 53. This implies that the first digit will always be equal to or greater than the second.
void Grid::getNeighbourCount(int row, int col, int &outSharkCount, int &outFishCount)
{
	outFishCount = 0;
	outSharkCount = 0;
	int breedingFishCount = 0, breedingSharkCount = 0;

	int currentValue;
	//check cells around the [row][col] cell (including the cell itself)
	for (int iRow = row - 1; iRow <= row + 1; ++iRow)
	{
		for (int iCol = col - 1; iCol <= col + 1; ++iCol)
		{
			currentValue = currentGrid[iRow][iCol];
			if (currentValue > 0)	//fish
			{
				++outFishCount;
				if (currentValue >= 2)	//breeding age
					++breedingFishCount;
			}
			else if (currentValue < 0)	//shark
			{
				++outSharkCount;
				if (currentValue <= -3)	//breeding age
					++breedingSharkCount;
			}
		}
	}

	//Eliminate the extra value coming from the cell itself
	if (currentGrid[row][col] > 0)	//fish
	{
		--outFishCount;
		if (currentGrid[row][col] >= 2)	//breeding age
			--breedingFishCount;
	}
	else if (currentGrid[row][col] < 0)	//shark
	{
		--outSharkCount;
		if (currentGrid[row][col] <= -3)	//breeding age
			--breedingSharkCount;
	}

	outFishCount = (outFishCount * 10) + breedingFishCount;
	outSharkCount = (outSharkCount * 10) + breedingSharkCount;
}
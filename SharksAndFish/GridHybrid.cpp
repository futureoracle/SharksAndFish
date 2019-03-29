#include"stdafx.h"
#include"GridHybrid.h"
#include"Utils.h"

#include<iostream>
#include<ctime>
#include<mpi.h>
#include<opencv2\opencv.hpp>

//NOTE: The terms 'machine(s)' and 'process(ess)' have been used interchaneably throughout the comments of this file.

#define N_THREADS 2

//Evaluates the rules of the celluar automata and puts values in the nextCalculatedGrid based on them
void GridHybrid::calculateNextGridState()
{
	updateGhostCells();

	int nSharkNeighbours, nFishNeighbours, nBreedingSharks, nBreedingFish;

#pragma omp parallel num_threads(N_THREADS) private(nSharkNeighbours, nFishNeighbours, nBreedingSharks, nBreedingFish)
#pragma omp for schedule(guided)
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

//Runs the grid according to the rules for nIterations, and returns the time it took to complete
float GridHybrid::runTest(int nIterations)
{
	float startTime = clock();
	for (int i = 0; i < nIterations; ++i)
	{
		calculateNextGridState();
		goToNextGridState();
		//To make sure all the machines are at the same stage
		MPI_Barrier(MPI_COMM_WORLD);
	}
	stitchGrid();
	return clock() - startTime;
}
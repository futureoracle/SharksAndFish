#include"stdafx.h"
#include"GridOMP.h"
#include"Utils.h"

#include<iostream>
#include<ctime>
#include<opencv2\opencv.hpp>
#include<omp.h>

#define N_THREADS 12


//Evaluates the rules of the celluar automata and puts values in the nextCalculatedGrid based on them
void GridOMP::calculateNextGridState()
{
	updateGhostCells();

	int nSharkNeighbours, nFishNeighbours, nBreedingSharks, nBreedingFish;
	//In the for loops, the first and last row and column are excluded because they are ghost cells
#pragma omp parallel num_threads(N_THREADS) private(nSharkNeighbours, nFishNeighbours, nBreedingSharks, nBreedingFish)
#pragma omp for schedule(guided)
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
				else if (Utils::getRandomNumber(1, 53) == 1)	//random causes; shark dies. bad luck.
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
float GridOMP::runTest(int nIterations)
{
	float startTime = clock();
	for (int i = 0; i < nIterations; ++i)
	{
		calculateNextGridState();
		goToNextGridState();
	}
	return clock() - startTime;
}

//Shows the grid as an image using OpenCV (displays the image in a new window)
void GridOMP::showGridAsImage(std::string additionalInfo)
{
	using namespace cv;

	Vec3b waterColour = Vec3b(255, 153, 153);	//light blue
	Vec3b fishColour = Vec3b(102, 0, 204);		//maroon
	Vec3b sharkColour = Vec3b(51, 255, 255);	//yellow

												//Create the image (pixels will be empty)
	Mat gridImage = Mat(rows - 2, cols - 2, CV_8UC3);

#pragma omp parallel num_threads(N_THREADS)
#pragma omp for schedule(guided)
	//Assign a colour to each pixel depending on what the corresponding cell contains
	for (int row = 1; row < rows - 1; ++row)
	{
		for (int col = 1; col < cols - 1; ++col)
		{
			if (currentGrid[row][col] > 0)			//fish
				gridImage.at<Vec3b>(row - 1, col - 1) = fishColour;
			else if (currentGrid[row][col] == 0)	//empty
				gridImage.at<Vec3b>(row - 1, col - 1) = waterColour;
			else									//shark
				gridImage.at<Vec3b>(row - 1, col - 1) = sharkColour;
		}
	}

	cv::imshow("Sharks and Fish" + std::string(" ") + additionalInfo, gridImage);
	cv::waitKey(0);
}
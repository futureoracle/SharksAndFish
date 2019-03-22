#include"stdafx.h"
#include"GridMPI.h"
#include"Utils.h"

#include<iostream>
#include<ctime>
#include<mpi.h>
#include<opencv2\opencv.hpp>

//NOTE: The terms 'machine(s)' and 'process(ess)' have been used interchaneably throughout the comments of this file.


//The number of machines / processes that the program is to be run on. This need to be the same as the number in the .bat file.
constexpr int nMachines = 4;

//Instantiates a grid with the given number of rows and columns
GridMPI::GridMPI(int rows, int cols)
{
	//To prevent the code from breaking ;-)
	if (nMachines > rows)
	{
		std::cout << "Number of processes is greater than number of rows!\n"
			<< "Increase the number of rows or decrease the number of machines!" << std::endl;
		return;
	}

	rowsPerMachine = new int[nMachines];

	//Get this process' rank
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//Utils::initUtils((rank * 167 + rank * 5) / 57 - rank);

	//Store the total number of rows; will be useful later when combining the small grids
	totalRows = rows;

	//Calculate the number of rows for each process
	for (int i = 0; i < nMachines; ++i)
		rowsPerMachine[i] = rows / nMachines;
	//Assign remaining rows to the machines in order
	int remainingRows = rows % nMachines;
	for (int i = 0; remainingRows > 0; ++i)
	{
		++rowsPerMachine[i];
		--remainingRows;
	}
	
	//std::cout << rank << " rows " << rowsPerMachine[rank] << "\n";

	//All machines calculate the above
	//Now we let machine 0 create the grid for us, and distribute it to the other machines:
	if (rank == 0)
	{
		//Add 2 extra rows and columns to make space for ghost cells
		this->rows = rows + 2;
		this->cols = cols + 2;

		//Allocates memory for the two grid variables
		allocateMemoryToGridVariables();

		//Fill the grid with values
		initGrid(15, 60);

		//showGridAsImage("Initial Grid");

		//Send rows to other machines (depending on their rowsPerMachine count)
		int row = rowsPerMachine[0] + 1;	//the row to send; +1 to account for the first ghost row
		for (int machine = 1; machine < nMachines; ++machine)
		{
			int maxRow = row + rowsPerMachine[machine];
			int id = 1;
			for (row = row; row < maxRow; ++row)
			{
				MPI_Send(currentGrid[row], this->cols, MPI_INT, machine, id++, MPI_COMM_WORLD);
			}
		}

		//Shave off the currentGrid so that it only has the rows required by machine 0
		for (int r = rowsPerMachine[0] + 2; r < this->rows; ++r)
		{
			delete[] currentGrid[r];
			delete[] nextCalculatedGrid[r];
		}
		this->rows = rowsPerMachine[0] + 2;
	}

	//All the other machines:
	else
	{
		//Create the grids (+2 for ghost cells)
		this->rows = rowsPerMachine[rank] + 2;
		this->cols = cols + 2;

		allocateMemoryToGridVariables();

		MPI_Status status;

		//Recieve the rows:
		for (int row = 1; row < this->rows - 1; ++row)
		{
			MPI_Recv(currentGrid[row], this->cols, MPI_INT, 0, row, MPI_COMM_WORLD, &status);
		}
	}

	//Wait for all processes to reach this point
	MPI_Barrier(MPI_COMM_WORLD);
}

GridMPI::~GridMPI()
{
	for (int i = 0; i < rows; ++i)
	{
		delete[] currentGrid[i];
		delete[] nextCalculatedGrid[i];
	}
	delete[] currentGrid;
	delete[] nextCalculatedGrid;

	delete[] rowsPerMachine;
}

//Prints the contents of the current grid to the console in the form of characters
void GridMPI::printToConsole(char shark, char fish, char water)
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
void GridMPI::printStatsToConsole()
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
float GridMPI::runTest(int nIterations)
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

//Equates the currentGrid to the nextCalculatedGrid
void GridMPI::goToNextGridState()
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
void GridMPI::calculateNextGridState()
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

//Shows the grid as an image using OpenCV (displays the image in a new window)
void GridMPI::showGridAsImage(std::string additionalInfo)
{
	using namespace cv;

	Vec3b waterColour = Vec3b(255, 153, 153);	//light blue
	Vec3b fishColour = Vec3b(102, 0, 204);		//maroon
	Vec3b sharkColour = Vec3b(51, 255, 255);	//yellow

												//Create the image (pixels will be empty)
	Mat gridImage = Mat(rows - 2, cols - 2, CV_8UC3);

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

//======PRIVATE MEMBERS===========================================================================

//Allocates new memory to currentGrid and nextCalculatedGrid based on this Grid's rows and cols
void GridMPI::allocateMemoryToGridVariables()
{
	currentGrid = new int*[rows];
	for (int row = 0; row < rows; ++row)
		currentGrid[row] = new int[cols];

	nextCalculatedGrid = new int*[rows];
	for (int row = 0; row < rows; ++row)
		nextCalculatedGrid[row] = new int[cols];
}

//Initializes the grid randomly
void GridMPI::initGrid()
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
void GridMPI::initGrid(int sharkPercent, int fishPercent)
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
//Since the grid is divided among processes row-wise, each process will need to get the ghost cells from its
//neighbouring processes (eg - process 0 will need to get a row each from process 1 and process n - 1)
void GridMPI::updateGhostCells()
{
	//The previous and next processes' rank
	static int prevRank = (rank + nMachines - 1) % nMachines;
	static int nextRank = (rank + nMachines + 1) % nMachines;

	//Decide tags for upper and lower rows for sending - invert for receiving
	static const int upTag = 0;	//while receiving into the upper ghost cell row, tag will be 1
	static const int downTag = 1;	//while receiving into the lower ghost cell row, tag will be -1

	static MPI_Status status;

	//rows - we get the rows first, inclusing the two cells (first and last) from the ghost cols
	//these will contain junk values, but we will update them later
	
	//Send this process' actual upper row to the previous process
	MPI_Send(currentGrid[1], cols, MPI_INT, prevRank, upTag, MPI_COMM_WORLD);
	//Recieve the bottom ghost row from the next process
	MPI_Recv(currentGrid[rows - 1], cols, MPI_INT, nextRank, upTag, MPI_COMM_WORLD, &status);

	//Send this process' actual lower row to the next process
	MPI_Send(currentGrid[rows - 1], cols, MPI_INT, nextRank, downTag, MPI_COMM_WORLD);
	//Recieve the top ghost row from the previous process
	MPI_Recv(currentGrid[0], cols, MPI_INT, prevRank, downTag, MPI_COMM_WORLD, &status);

	//The receives are blocking receives, so no process will pass past this point without having received the ghost rows

	//columns - calculating these only requires the current grid
	for (int row = 1; row < rows - 1; ++row)
	{
		//left column
		currentGrid[row][0] = currentGrid[row][cols - 2];
		//right column
		currentGrid[row][cols - 1] = currentGrid[row][1];
	}

	//corners
	//The only true "corners" are the top ghost corners for the first process and the bottom ghost corners for the last process
	//For all other "corners", they are just copies of the actual cell in the opposite column (exactly like we calculated in the column loop above)
	/*Tags used for sending are:
	Top left = 3
	Top right = 4
	Bottom left = 5
	Bottom right = 6*/
	if (rank == 0)
	{
		//send the actual top corners to the last process
		MPI_Send(&currentGrid[1][1], 1, MPI_INT, nMachines - 1, 3, MPI_COMM_WORLD);			//top-left
		MPI_Send(&currentGrid[1][cols - 2], 1, MPI_INT, nMachines - 1, 4, MPI_COMM_WORLD);	//top-right

		//Receive the ghost corners from the last process
		MPI_Recv(&currentGrid[0][0], 1, MPI_INT, nMachines - 1, 6, MPI_COMM_WORLD, &status);		//top-left
		MPI_Recv(&currentGrid[0][cols - 1], 1, MPI_INT, nMachines - 1, 5, MPI_COMM_WORLD, &status);	//top-right
	}
	else if (rank == nMachines - 1)
	{
		//send the actual bottom corners to the first process
		MPI_Send(&currentGrid[rows - 2][1], 1, MPI_INT, 0, 5, MPI_COMM_WORLD);			//bottom-left
		MPI_Send(&currentGrid[rows - 2][cols - 2], 1, MPI_INT, 0, 6, MPI_COMM_WORLD);	//bottom-right

		//Receive the ghost corners from the first process
		MPI_Recv(&currentGrid[rows - 1][0], 1, MPI_INT, 0, 4, MPI_COMM_WORLD, &status);		//bottom-left
		MPI_Recv(&currentGrid[rows - 1][cols - 1], 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &status);		//bottom-right
	}

	//Calculate the top ghost corners as copies of the opposite column's cells
	if (rank != 0)
	{
		//top-left
		currentGrid[0][0] = currentGrid[0][cols - 2];
		//top-right
		currentGrid[0][cols - 1] = currentGrid[0][1];
	}
	//Calculate the bottom ghost corners as copies of the opposite column's cells
	if (rank != nMachines - 1)
	{
		//bottom-left
		currentGrid[rows - 1][0] = currentGrid[rows - 1][cols - 2];
		//Bottom-right
		currentGrid[rows - 1][cols - 1] = currentGrid[rows - 1][1];
	}
}

//Calculates the number of neighbours of each type a cell has, and returns them through the arguments
//Both of the arguments will be stored in the form 10 * number + number that can breed
//eg - If there are 5 fish neighbours out of which 3 are of breeding age, the value of outFishCount will be stored as
//10 * 5 + 3 = 53. This implies that the first digit will always be equal to or greater than the second.
void GridMPI::getNeighbourCount(int row, int col, int &outSharkCount, int &outFishCount)
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

//Collects the different grids from all the processes and combines them
//Prints the collected grid
void GridMPI::stitchGrid()
{
	if (rank == 0)
	{
		int **completeGrid = new int*[totalRows + 2];
		//Copy the process' rows into the grid
		for (int row = 1; row < rows + 1; ++row)
		{
			completeGrid[row] = new int[cols];
			for (int col = 0; col < cols; ++col)
				completeGrid[row][col] = currentGrid[row + 1][col];
		}
		for (int row = rows + 1; row < totalRows + 2; ++row)
		{
			completeGrid[row] = new int[cols];
		}

		MPI_Status status;

		//Allocate space to other rows; receive them from other machines
		//The rows are received along with the ghost cells at the left and right, but these
		//Will be ignored while printing
		int row = rowsPerMachine[0] + 1;	//the row to receive; +1 to account for the ghost row
		for (int machine = 1; machine < nMachines; ++machine)
		{
			int maxRow = row + rowsPerMachine[machine];
			int id = 1;
			for (row = row; row < maxRow; ++row)
			{
				MPI_Recv(completeGrid[row], cols, MPI_INT, machine, id++, MPI_COMM_WORLD, &status);
			}
		}

		//deallocate memory to previous (smaller) grid on machine 0
		for (int i = 0; i < rows; ++i)
		{
			delete[] currentGrid[i];
			delete[] nextCalculatedGrid[i];
		}
		delete[] currentGrid;
		delete[] nextCalculatedGrid;
		//Just to be safe
		nextCalculatedGrid = nullptr;

		//make the whole grid the current grid
		currentGrid = completeGrid;
		rows = totalRows + 2;

		//display
		//showGridAsImage("Final Grid");
	}
	else
	{
		//send all the rows to machine 0
		for (int row = 1; row < rows - 1; ++row)
		{
			MPI_Send(currentGrid[row], cols, MPI_INT, 0, row, MPI_COMM_WORLD);
		}
	}
}
#pragma once

/*Represents a 2D grid made up of cells, each being able to contain a shark, a fish, or water.
These are represented by integers:
> 0 = fish
< 0 = shark
==0 = water
For sharks and fish, the absolute value of the integer corresponds to their age.
eg- A cell with value -5 contains a 5-year-old shark.*/
class Grid
{
public:
	Grid(int rows, int cols);
	void printGridToConsole(char shark = 'X', char fish = 'f', char water = '.');
	void printStatsToConsole();

private:
	int **currentGrid, **nextCalculatedGrid;
	int rows, cols;

	void allocateMemoryToGridVariables();
	void initGrid();
	void initGrid(int sharkPercent, int fishPercent);

	int getRandomNumber(int min, int max);
};
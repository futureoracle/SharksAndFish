#include "stdafx.h"
#include"Grid.h"
#include"Utils.h"

#include<iostream>
#include<stdlib.h>


int main()
{
	//Initialize utilities before everything else
	Utils::initUtils();

	Grid myGrid = Grid(800, 800);

	//For running the test and recording the time it takes to run:
	//std::cout << myGrid.runTest(1000) / static_cast<float>(1000) << std::endl;
	//myGrid.showGridAsImage();

	//For going through and displaying it step by step:
	while (true)
	{
		//myGrid.printToConsole('X', 'F', ' ');
		myGrid.showGridAsImage();
		myGrid.calculateNextGridState();
		myGrid.goToNextGridState();
		//system("pause");	
	}

	
	myGrid.printStatsToConsole();
	
	system("pause");

    return 0;
}


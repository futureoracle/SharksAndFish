#include "stdafx.h"
#include"Grid.h"

#include<stdlib.h>

int main()
{
	Grid myGrid = Grid(100, 100);
	//myGrid.printGridToConsole();
	myGrid.printStatsToConsole();
	
	system("pause");

    return 0;
}


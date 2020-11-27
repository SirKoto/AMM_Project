

#include "Model.h"
#include "GreedyModel.h"
#include <iostream>

int main(int argc, char* argv[]) {

	Model modelData;

	//std::string fileName = "data/project.2.dat";
	std::string fileName = "data/output.txt";

	bool read = modelData.readFromFile(fileName);
	if (!read)
	{
		std::cout << "Cannot read file " << fileName << std::endl;
	}
	else
	{
		std::cout << "Model file loaded " << fileName << std::endl;
	}

	GreedyModel pMod(modelData);

	pMod.runGreedy();


	std::cout << pMod;

	return 0;
}


#include "Model.h"
#include "GreedyModel.h"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {

	Model modelData;

	//std::string fileName = "data/project.2.dat";
	std::string fileName = "data/output.txt";

	bool read = modelData.readFromFile(fileName);
	if (!read)
	{
		std::cout << "Cannot read file " << fileName << std::endl;
		exit(1);
	}
	else
	{
		std::cout << "Model file loaded " << fileName << std::endl;
	}


	auto start = std::chrono::steady_clock::now();
	GreedyModel pMod(modelData);

	pMod.runGreedy();
	auto end = std::chrono::steady_clock::now();
	auto diff = end - start;
	std::cout << pMod;
	std::cout << std::chrono::duration <double>(diff).count() << " seconds for greedy execution" << std::endl;
	return 0;
}
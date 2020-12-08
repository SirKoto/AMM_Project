

#include "Model.h"
#include "GreedyModel.h"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {

	Model modelData;

	//std::string fileName = "data/project.2.dat";
	std::string fileName = "data/output.txt";
	auto start = std::chrono::steady_clock::now();

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


	GreedyModel pMod(modelData);

	pMod.runGreedy();
	auto end = std::chrono::steady_clock::now();
	auto diff = end - start;
	std::cout << pMod;
	std::cout << std::chrono::duration<double>(diff).count() << " seconds for greedy execution" << std::endl;
	float costGreedy = pMod.getCentersCost();
	start = std::chrono::steady_clock::now();
	pMod.runParallelLocalSearch();
	end = std::chrono::steady_clock::now();
	float costParallel = pMod.getCentersCost();
	diff = end - start;
	std::cout << pMod;
	std::cout << std::chrono::duration <double>(diff).count() << " seconds for local search execution" << std::endl;

	std::cout << costGreedy <<" as cost for Greedy and  " << costParallel << " as cost for localSearch "  << std::endl;

	float cost = std::numeric_limits<float>::infinity();

	start = std::chrono::steady_clock::now();
	for (int i = 0; i < 100; ++i) {
		pMod.purge();
		pMod.GRASPConstructivePhase(0.2);
		pMod.runParallelLocalSearch();
		if (pMod.isSolution() && cost > pMod.getCentersCost()) cost = pMod.getCentersCost();
	}
	end = std::chrono::steady_clock::now();
	diff = end - start;
	std::cout << std::chrono::duration <double>(diff).count() << " seconds for GRASP execution with optimal cost "<< cost << std::endl;

	return 0;
}
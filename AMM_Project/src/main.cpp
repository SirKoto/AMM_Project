

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
	srand(0);
	start = std::chrono::steady_clock::now();
	auto current = std::chrono::steady_clock::now();
	uint64_t iterations = 0;
	while (std::chrono::duration <double>(current-start).count()<=600) {
		iterations++;
		pMod.purge();
		pMod.GRASPConstructivePhase(0.2);
		uint32_t costAux = pMod.getCentersCost();
		pMod.runParallelLocalSearch();
		/*
		if (pMod.isSolution()) {
			std::cout << "solution is feasible with ";
		}
		else {
			std::cout << "solution is infeasible with ";
		}
		std::cout << costAux << " cost for constructive phase " << pMod.getCentersCost() <<" cost after local search" <<std::endl;
		*/
		current = std::chrono::steady_clock::now();
		if (pMod.isSolution() && cost > pMod.getCentersCost()) cost = pMod.getCentersCost();
	}
	end = std::chrono::steady_clock::now();
	diff = end - start;
	std::cout << std::chrono::duration <double>(diff).count() << " seconds for "<< iterations <<" iterations of GRASP execution with optimal cost "<< cost << std::endl;

	return 0;
}
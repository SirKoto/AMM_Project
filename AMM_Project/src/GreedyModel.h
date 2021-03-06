#pragma once

#include <numeric>
#include <iostream>

#include "IModel.h"


class GreedyModel : public IModel
{
public:

	GreedyModel(const Model& model);

	void runGreedy();
	void runParallelLocalSearch();
	void GRASPConstructivePhase(float alpha);
	void purge();

protected:

	typedef struct Candidate
	{
		float fit;
		std::vector<char> assigns;
		uint32_t type;
		uint32_t loc;

	} Candidate;


	typedef struct Swap
	{
		uint32_t city;

		uint32_t location;
		bool primarySwap;
		float fit;

	} Swap;

	
	// array of num cities * num locations, 
	std::vector<uint32_t> mSortedCities;

	Candidate tryAddGreedy(const uint32_t l, const uint32_t t, std::vector<char>& assignments) const;

	const uint32_t* getCitiesSorted(const uint32_t l) const;


	// Returns location and type
	Candidate findBestAddition(std::vector<Candidate> &bestCandidates, uint32_t perThread, int processor_count) const;

	void applyAction(const Candidate& bestAction);

	void applySwap(Swap swap);

	void trimLocations();

	Swap findBestSwap(std::vector<Swap> bestSwaps, uint32_t perThread, int processor_count);

	double getUsefulLoad(std::vector<float> centerServing);

	Candidate findCandidateGRASP(std::vector<Candidate> &candidates, std::vector<Candidate> &RCL,std::vector<std::vector<char>> &assignments, uint32_t perThread, int processor_count, float alpha) const;

};


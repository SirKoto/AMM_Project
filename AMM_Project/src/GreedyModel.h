#pragma once

#include <numeric>
#include <iostream>

#include "IModel.h"


class GreedyModel : public IModel
{
public:

	GreedyModel(const Model& model);

	void runGreedy();

protected:

	typedef struct Candidate
	{
		float fit;
		std::vector<char> assigns;
		uint32_t type;
		uint32_t loc;

	} Candidate;
	
	// array of num cities * num locations, 
	std::vector<uint32_t> mSortedCities;

	Candidate tryAddGreedy(const uint32_t l, const uint32_t t) const;

	const uint32_t* getCitiesSorted(const uint32_t l) const;


	// Returns location and type
	Candidate findBestAddition() const;

	void applyAction(Candidate bestAction);
};


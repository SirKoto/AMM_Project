#pragma once

#include <numeric>
#include <iostream>

#include "ProcessedModel.h"


class GreedyModel : public ProcessedModel
{
public:

	GreedyModel(const Model& model);

	void runGreedy();

protected:


	// array of num cities * num locations, 
	std::vector<uint32_t> mSortedCities;

	float tryAddGreedy(const uint32_t l, const uint32_t t) const;

	const uint32_t* getCitiesSorted(const uint32_t l) const;


	// Returns location and type
	std::pair<uint32_t, uint32_t> findBestAddition() const;

	void applyAction(const uint32_t l, const uint32_t t);
};


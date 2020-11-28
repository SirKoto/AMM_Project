#pragma once

#include <numeric>
#include <iostream>

#include "ProcessedModel.h"


class GreedyModel : public ProcessedModel
{
public:

	GreedyModel(const Model& model);

	float getCentersCost() const;

	bool isSolution() const;

	void runGreedy();

protected:

	typedef struct Candidate
	{
		float fit;
		std::vector<char> assigns;
		uint32_t type;
		uint32_t loc;

	} Candidate;

	static constexpr uint32_t NOT_ASSIGNED = std::numeric_limits<uint32_t>::max();
	
	std::vector<uint32_t> mLocationTypeAssignment;

	std::vector<std::pair<uint32_t, uint32_t>> mCityCenterAssignment;

	// array of num cities * num locations, 
	std::vector<uint32_t> mSortedCities;

	bool locationIsBlocked(const uint32_t l) const;

	Candidate tryAddGreedy(const uint32_t l, const uint32_t t) const;

	const uint32_t* getCitiesSorted(const uint32_t l) const;


	// Returns location and type
	Candidate findBestAddition() const;

	void applyAction(Candidate bestAction);

	friend std::ostream& operator<<(std::ostream& os, const GreedyModel& dt);




};


#pragma once

#include "Model.h"
#include <vector>

class ProcessedModel
{
public:
	ProcessedModel(const Model& model);

protected:

	Model mBaseModel;

	std::vector<bool> mCompatibleLocations;
	std::vector<bool> mCompatibleCityLocationType;
	const uint32_t mNumLocations;
	const uint32_t mNumTypes;
	const uint32_t mNumCities;

	inline bool isLocationPairCompatible(const uint32_t& l1, const uint32_t& l2) const;

	// c < mNumCities, l < mNumLocations, t < mNumTypes, isSecondary {0,1}
	inline bool isCityLocationTypeCompatible(const uint32_t& c, const uint32_t& l, const uint32_t& t, const uint32_t& isSecondary) const;

};


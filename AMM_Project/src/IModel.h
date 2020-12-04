#pragma once

#include "Model.h"
#include <vector>

class IModel
{
public:
	IModel(const Model& model);
	IModel(const IModel* model);

	float getCentersCost() const;

	// Every city with different center assignment
	bool isSolutionFast() const;

	// isSolutionFast() && population constraints
	bool isSolutionPop() const;

	bool isSolution() const;

protected:

	Model mBaseModel;

	std::vector<bool> mCompatibleLocations;
	std::vector<bool> mCompatibleCityLocationType;

	const uint32_t mNumLocations;
	const uint32_t mNumTypes;
	const uint32_t mNumCities;


	static constexpr uint32_t NOT_ASSIGNED = std::numeric_limits<uint32_t>::max();

	std::vector<uint32_t> mLocationTypeAssignment;

	std::vector<std::pair<uint32_t, uint32_t>> mCityCenterAssignment;

	bool isLocationPairCompatible(const uint32_t& l1, const uint32_t& l2) const;

	bool areAllLocationsCompatible() const;

	bool isLocationPairCompatibleSIMD(const uint32_t& l1, const uint32_t& l2) const;

	bool locationIsBlocked(const uint32_t l) const;


	// c < mNumCities, l < mNumLocations, t < mNumTypes, isSecondary {0,1}
	bool isCityLocationTypeCompatible(const uint32_t& c, const uint32_t& l, const uint32_t& t, const uint32_t& isSecondary) const;

	friend std::ostream& operator<<(std::ostream& os, const IModel& dt);

	friend class LocalSearchModel;
};


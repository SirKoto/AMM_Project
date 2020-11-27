#include "GreedyModel.h"

#include <algorithm>

GreedyModel::GreedyModel(const Model& model) : ProcessedModel(model)
{
	mLocationTypeAssignment.resize(mNumLocations, NOT_ASSIGNED);
	mCityCenterAssignment.resize(mNumCities, { NOT_ASSIGNED, NOT_ASSIGNED });
	
	mSortedCities.resize(mNumLocations * mNumCities);
	uint32_t* pl = mSortedCities.data();
	for (uint32_t l = 0; l < mNumLocations; ++l) {
		for (uint32_t c = 0; c < mNumCities; ++c) {
			(*pl) = c;
			pl += 1;
		}
	}

	// internally sort
	for (uint32_t l = 0; l < mNumLocations; ++l) {
		uint32_t* pl = mSortedCities.data() + l * mNumCities;
		std::sort(pl, pl + mNumCities,
			[&](const uint32_t& c1, const uint32_t& c2) -> bool {
				
				return mBaseModel.getCities()[c1].cityPos.sqDist(mBaseModel.getLocations()[l]) <
					mBaseModel.getCities()[c2].cityPos.sqDist(mBaseModel.getLocations()[l]);
			});
	}
}

float GreedyModel::getCentersCost() const
{
	float sum = 0.0f;
	for (const uint32_t& type : mLocationTypeAssignment) {
		if (type != NOT_ASSIGNED) {
			sum += mBaseModel.getCenterTypes()[type].cost;
		}
	}
	return sum;
}

bool GreedyModel::isSolution() const
{
	for (const std::pair<uint32_t, uint32_t > &city : mCityCenterAssignment) {
		if (city.first == NOT_ASSIGNED || city.second == NOT_ASSIGNED) {
			return false;
		}
	}
	return true;
}

bool GreedyModel::locationIsBlocked(const uint32_t l) const
{
	for (uint32_t l2 = 0; l2 < mNumLocations; ++l2) {
		if (mLocationTypeAssignment[l] != NOT_ASSIGNED && !isLocationPairCompatible(l, l2)) {
			return true;
		}
	}
	return false;
}

float GreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		return std::numeric_limits<float>::infinity();
	}

	uint32_t num = 0;
	float pop = 0.0f;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);
	for (uint32_t c = 0; c < mNumCities; ++c) {
		if ((mCityCenterAssignment[*ptr].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) ||
			(mCityCenterAssignment[*ptr].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1))) {

			float newPop = pop + mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			pop = newPop;
		}
	}

	return mBaseModel.getCenterTypes()[t].cost / pop;
}

const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

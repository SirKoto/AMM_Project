#include "ProcessedModel.h"

ProcessedModel::ProcessedModel(const Model& model) :
	mBaseModel(model),
	mNumLocations(static_cast<uint32_t>(model.getLocations().size())),
	mNumTypes(static_cast<uint32_t>(model.getCenterTypes().size())),
	mNumCities(static_cast<uint32_t>(model.getCities().size()))
{
	mCompatibleLocations.resize(mNumLocations * mNumLocations);
	// compute location compatibility
	for (uint32_t l1 = 0; l1 < mNumLocations; ++l1) {
		for (uint32_t l2 = 0; l2 < mNumLocations; ++l2) {
			bool res = true;
			if (l1 != l2) {
				res = model.getLocations()[l1].sqDist(model.getLocations()[l2]) >= model.getMinDistanceBetweenCenters();
			}

			mCompatibleLocations[(l1 * mNumLocations + l2)] = res;
			mCompatibleLocations[(l2 * mNumLocations + l1)] = res;
		}
	}

	// compute city can be assigned to center in location of type
	mCompatibleCityLocationType.resize(2 * mNumCities * mNumLocations * mNumTypes);
	for (uint32_t c = 0; c < mNumCities; ++c) {
		for (uint32_t l = 0; l < mNumLocations; ++l) {
			for (uint32_t t = 0; t < mNumTypes; ++t) {
				uint32_t idx = (((c * mNumLocations + l) * mNumTypes + t) * 2);
				mCompatibleCityLocationType[idx] = 
					model.getCities()[c].cityPos.dist(model.getLocations()[l]) <= 
					model.getCenterTypes()[t].serveDist;
				mCompatibleCityLocationType[idx + 1] = 
					model.getCities()[c].cityPos.dist(model.getLocations()[l]) <= 
					3 * model.getCenterTypes()[t].serveDist;
			}
		}
	}
}

bool ProcessedModel::isLocationPairCompatible(const uint32_t& l1, const uint32_t& l2) const
{

	return mCompatibleLocations[(l1 * mNumLocations + l2)];
}

bool ProcessedModel::isCityLocationTypeCompatible(
	const uint32_t& c,
	const uint32_t& l,
	const uint32_t& t, 
	const uint32_t& isSecondary) const
{
	return mCompatibleCityLocationType[(((c * mNumLocations + l) * mNumTypes + t) * 2 + isSecondary)];
}

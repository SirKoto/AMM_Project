#include "BasicGreedyModel.h"

#include <algorithm>
#include <map>

BasicGreedyModel::BasicGreedyModel(const Model& model) : IModel(model)
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

void BasicGreedyModel::runGreedy()
{
	auto numToAssign = [&]() -> float
	{
		uint32_t x = 0;
		for (const std::pair<uint32_t, uint32_t >& p : mCityCenterAssignment) {
			x += p.first != NOT_ASSIGNED ? 1 : 0;
			x += p.second != NOT_ASSIGNED ? 1 : 0;
		}
		return static_cast<float>(x) / static_cast<float>(2*mCityCenterAssignment.size());
	};

	float actual = numToAssign();
	while (!isSolution()) {
		std::pair<uint32_t, uint32_t> bestAction = findBestAddition();

		if (bestAction.first == NOT_ASSIGNED) {
			break;
		}

		applyAction(bestAction.first, bestAction.second);

		float n = numToAssign();
		if (n > actual + 0.01f) {
			actual = n;
			std::cout << actual * 100.0f <<"%" << std::endl;
		}
	}
}


float BasicGreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		return std::numeric_limits<float>::infinity();
	}

	uint32_t num = 0;
	float pop = 0.0f;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);
	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {

			float newPop = pop + mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			pop = newPop;
		}
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) {
			float newPop = pop + 0.1f * mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			pop = newPop;
		}
	}

	return mBaseModel.getCenterTypes()[t].cost / pop;
}

const uint32_t* BasicGreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

std::pair<uint32_t, uint32_t> BasicGreedyModel::findBestAddition() const
{
	std::pair<uint32_t, uint32_t> res = {NOT_ASSIGNED, NOT_ASSIGNED};
	float actualFitness = std::numeric_limits<float>::infinity();

	for (uint32_t l = 0; l < mNumLocations; ++l) {
		for (uint32_t t = 0; t < mNumTypes; ++t) {
			float fit = tryAddGreedy(l, t);

			if (fit < actualFitness) {
				actualFitness = fit;
				res = { l, t };
			}
		}
	}


	return res;
}

void BasicGreedyModel::applyAction(const uint32_t l, const uint32_t t)
{
	float pop = 0.0f;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;

	const uint32_t* ptr = getCitiesSorted(l);
	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {

			float newPop = pop + mBaseModel.getCities()[c].population;
			if (newPop <= maxPop) {
				pop = newPop;
				mCityCenterAssignment[c].first = l;
			}
		}
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) {
			float newPop = pop + 0.1f * mBaseModel.getCities()[c].population;
			if (newPop <= maxPop) {
				pop = newPop;
				mCityCenterAssignment[c].second = l;
			}
			
		}
	}

	mLocationTypeAssignment[l] = t;
}


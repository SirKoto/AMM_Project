#include "GreedyModel.h"

#include <algorithm>
#include <map>

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

void GreedyModel::runGreedy()
{
	while (!isSolution()) {
		std::pair<uint32_t, uint32_t> bestAction = findBestAddition();

		if (bestAction.first == NOT_ASSIGNED) {
			break;
		}

		applyAction(bestAction.first, bestAction.second);
	}
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

const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

std::pair<uint32_t, uint32_t> GreedyModel::findBestAddition() const
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

void GreedyModel::applyAction(const uint32_t l, const uint32_t t)
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

std::ostream& operator<<(std::ostream& os, const GreedyModel& dt)
{
	std::map<uint32_t, float> centerServing;
	for (uint32_t i = 0; i < dt.mNumLocations; ++i) {
		centerServing.insert({ i, 0.0f });
	}

	os << "Resulting cost: " << dt.getCentersCost() << "\n";
	os << "Is a solution?: " << dt.isSolution() << "\n";
	os << "\nCities assigned to:\n";
	for (uint32_t i = 0; i < dt.mCityCenterAssignment.size(); ++i) {
		os << "City " << i << " first: " << dt.mCityCenterAssignment[i].first << " second: " << dt.mCityCenterAssignment[i].second << "\n";

		centerServing.at(dt.mCityCenterAssignment[i].first) += dt.mBaseModel.getCities()[i].population;
		centerServing.at(dt.mCityCenterAssignment[i].second) += 0.1f * dt.mBaseModel.getCities()[i].population;

	}
	os << "\nlocations assigned:\n";
	for (uint32_t i = 0; i < dt.mLocationTypeAssignment.size(); ++i) {
		if (dt.mLocationTypeAssignment[i] != dt.NOT_ASSIGNED) {
			os << "Location " << i << " assigned with center type " << dt.mLocationTypeAssignment[i] << "\n";
			os << "\tServing to " << centerServing.at(i) << "/" << dt.mBaseModel.getCenterTypes()[dt.mLocationTypeAssignment[i]].maxPop << " population\n";
		}
	}


	return os;
}

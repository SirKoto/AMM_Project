#include "IModel.h"

#include <map>
#include <iostream>

IModel::IModel(const Model& model) :
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
				res = model.getLocations()[l1].dist(model.getLocations()[l2]) >= model.getMinDistanceBetweenCenters();
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

float IModel::getCentersCost() const
{
	float sum = 0.0f;
	for (const uint32_t& type : mLocationTypeAssignment) {
		if (type != NOT_ASSIGNED) {
			sum += mBaseModel.getCenterTypes()[type].cost;
		}
	}
	return sum;
}

bool IModel::isSolution() const
{
	for (const std::pair<uint32_t, uint32_t >& city : mCityCenterAssignment) {
		if (city.first == NOT_ASSIGNED || city.second == NOT_ASSIGNED) {
			return false;
		}
	}
	return true;
}

bool IModel::isLocationPairCompatible(const uint32_t& l1, const uint32_t& l2) const
{

	return mCompatibleLocations[(l1 * mNumLocations + l2)];
}

bool IModel::isCityLocationTypeCompatible(
	const uint32_t& c,
	const uint32_t& l,
	const uint32_t& t, 
	const uint32_t& isSecondary) const
{
	return mCompatibleCityLocationType[(((c * mNumLocations + l) * mNumTypes + t) * 2 + isSecondary)];
}

bool IModel::locationIsBlocked(const uint32_t l) const
{
	for (uint32_t l2 = 0; l2 < mNumLocations; ++l2) {
		if (mLocationTypeAssignment[l] != NOT_ASSIGNED && !isLocationPairCompatible(l, l2)) {
			return true;
		}
	}
	return false;
}

std::ostream& operator<<(std::ostream& os, const IModel& dt)
{
	std::map<uint32_t, float> centerServing;
	for (uint32_t i = 0; i < dt.mNumLocations; ++i) {
		centerServing.insert({ i, 0.0f });
	}

	os << "\nCities assigned to:\n";
	for (uint32_t i = 0; i < dt.mCityCenterAssignment.size(); ++i) {
		os << "City " << i << " first: " << static_cast<int>(dt.mCityCenterAssignment[i].first != dt.NOT_ASSIGNED ? dt.mCityCenterAssignment[i].first : -1)
			<< " second: " << static_cast<int>(dt.mCityCenterAssignment[i].second != dt.NOT_ASSIGNED ? dt.mCityCenterAssignment[i].second : -1) << "\n";

		if (centerServing.count(dt.mCityCenterAssignment[i].first)) {
			centerServing.at(dt.mCityCenterAssignment[i].first) += dt.mBaseModel.getCities()[i].population;
		}
		if (centerServing.count(dt.mCityCenterAssignment[i].second)) {
			centerServing.at(dt.mCityCenterAssignment[i].second) += 0.1f * dt.mBaseModel.getCities()[i].population;
		}

	}
	os << "\nlocations assigned:\n";
	for (uint32_t i = 0; i < dt.mLocationTypeAssignment.size(); ++i) {
		if (dt.mLocationTypeAssignment[i] != dt.NOT_ASSIGNED) {
			os << "Location " << i << " assigned with center type " << dt.mLocationTypeAssignment[i] << "\n";
			os << "\tServing to " << centerServing.at(i) << "/" << dt.mBaseModel.getCenterTypes()[dt.mLocationTypeAssignment[i]].maxPop << " population\n";
		}
	}

	os << "\nResulting cost: " << dt.getCentersCost() << "\n";
	os << "Is a solution?: " << dt.isSolution() << "\n";
	if (!dt.isSolution()) {
		uint32_t i = 0;
		for (const std::pair<uint32_t, uint32_t >& city : dt.mCityCenterAssignment) {
			if (city.first == dt.NOT_ASSIGNED || city.second == dt.NOT_ASSIGNED) {
				os << "City " << i << " is missing " << (city.first == dt.NOT_ASSIGNED ? "first " : "") << (city.second == dt.NOT_ASSIGNED ? "second" : "") << "\n";

			}
			++i;
		}
	}


	return os;
}
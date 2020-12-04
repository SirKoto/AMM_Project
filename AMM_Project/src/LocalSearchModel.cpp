#include "LocalSearchModel.h"
#undef NDEBUG
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>

IModel LocalSearchModel::run(const IModel* model)
{
	LocalSearchModel lM(model);

	bool improvement = true;
	OperationCenters bestOp;
	float bestH = std::numeric_limits<float>::infinity();

	auto updateH = [&](const OperationCenters& op, const float& h)
	{
		if (h < bestH)
		{
			bestH = h;
			bestOp = op;
			improvement = true;
		}
	};

	// first update to set up everything
	lM.generalUpdate();
	std::vector<std::pair<uint32_t, uint32_t>> cityCenterAssignmentCopy(lM.mCityCenterAssignment.size()); 

	bestH = lM.mGenericHeuristic;

	uint32_t it = 0;

	OperationCenters op, opComp;
	while (improvement && it++ < 1000) {

		// store copy of the system at this moment
		cityCenterAssignmentCopy = lM.mCityCenterAssignment;

		improvement = false;
		op.op = OperationCenters::Op::eSet;
		for (uint32_t l = 0; l < lM.mNumLocations; ++l) {
			op.location = l;
			for(uint32_t t = 0; t < lM.mNumTypes; ++t) {
				op.type = t;
				opComp = lM.doLocationsOp(op);
				lM.generalUpdate();
				updateH(op, lM.mGenericHeuristic);
				lM.doLocationsOp(opComp);
				lM.mCityCenterAssignment = cityCenterAssignmentCopy; // recover original assignment
			}
			op.type = NOT_ASSIGNED;
			opComp = lM.doLocationsOp(op);
			lM.generalUpdate();
			updateH(op, lM.mGenericHeuristic);
			lM.doLocationsOp(opComp);
			lM.mCityCenterAssignment = cityCenterAssignmentCopy; // recover original assignment
		}
		op.op = OperationCenters::Op::eSwap;
		for (uint32_t l = 0; l < lM.mNumLocations; ++l) {
			op.location = l;
			for (uint32_t l2 = l + 1; l2 < lM.mNumLocations; ++l2) {
				op.location2 = l2;
				lM.doLocationsOp(op);
				lM.generalUpdate();
				updateH(op, lM.mGenericHeuristic);
				lM.doLocationsOp(op);
				lM.mCityCenterAssignment = cityCenterAssignmentCopy; // recover original assignment
			}
		}

		if (improvement) {
			lM.doLocationsOp(bestOp);
			lM.generalUpdate();
			std::cout << "\rH: " << lM.mGenericHeuristic << std::flush;
		}

	}

	std::cout << "\nNum iterations " << it << std::endl;
	return IModel(&lM);
}

LocalSearchModel::LocalSearchModel(const Model& model) :
	IModel(model), 
	mCityHeuristicBuffer(model.getCities().size(), 0.0f),
	mLocationAssignedPopulation(model.getLocations().size(), 0),
	mLocationHeuristicBuffer(model.getLocations().size(), 0.0f)
{

	for (std::pair<uint32_t, uint32_t>& it : mCityCenterAssignment)
	{
		it.first = NOT_ASSIGNED;
		it.second = NOT_ASSIGNED;
	}
}

LocalSearchModel::LocalSearchModel(const IModel* model) : 
	IModel(model),
	mCityHeuristicBuffer(model->mNumCities, 0.0f),
	mLocationAssignedPopulation(model->mNumLocations, 0),
	mLocationHeuristicBuffer(model->mNumLocations, 0.0f)
{

	for (std::pair<uint32_t, uint32_t>& it : mCityCenterAssignment)
	{
		it.first = NOT_ASSIGNED;
		it.second = NOT_ASSIGNED;
	}
}

void LocalSearchModel::resetCenters()
{
	std::memset(mCityHeuristicBuffer.data(), 0, mNumCities * sizeof(float));
	std::memset(mLocationHeuristicBuffer.data(), 0, mNumLocations  * sizeof(float));
	std::memset(mLocationAssignedPopulation.data(), 0, mNumLocations * sizeof(uint32_t));

	mActualAssignmentHeuristic = 0.0f;
	
	uint32_t c = 0;
	for (std::pair<uint32_t, uint32_t>& it : mCityCenterAssignment)
	{
		//it.first = NOT_ASSIGNED;
		//it.second = NOT_ASSIGNED;
		// Update centers and delete if location is empty
		if (it.first != NOT_ASSIGNED) {
			if (mLocationTypeAssignment[it.first] != NOT_ASSIGNED) {
				mLocationAssignedPopulation[it.first] += 10 * mBaseModel.getCities()[c].population;
			}
			else {
				it.first = NOT_ASSIGNED;
			}
		}
		if (it.second != NOT_ASSIGNED) {
			if (mLocationTypeAssignment[it.second] != NOT_ASSIGNED) {
				mLocationAssignedPopulation[it.second] += mBaseModel.getCities()[c].population;
			}
			else {
				it.second = NOT_ASSIGNED;
			}
		}

		c += 1;
	}
}

void LocalSearchModel::generalUpdate()
{
	resetCenters();
	bool res = localSearchAssignments();
	mGenericHeuristic = 0.0f;
	if (!res) {
		mGenericHeuristic = 10000;
	}

	if (!areAllLocationsCompatible()) {
		uint32_t count = 0;
		for (uint32_t l1 = 0; l1 < mNumLocations; ++l1) {
			for (uint32_t l2 = l1 + 1; l2 < mNumLocations; ++l2) {
				if (mLocationTypeAssignment[l1] != NOT_ASSIGNED &&
					mLocationTypeAssignment[l2] != NOT_ASSIGNED &&
					!isLocationPairCompatible(l1, l2)) {
					count += 1;
				}
			}
		}
		mGenericHeuristic += 1000 * count;
	}

	mGenericHeuristic += getCentersCost();
}

LocalSearchModel::OperationCenters LocalSearchModel::doLocationsOp(OperationCenters op)
{
	switch (op.op)
	{
	case OperationCenters::Op::eSet:
		assert(op.location != NOT_ASSIGNED);
		std::swap(mLocationTypeAssignment[op.location], op.type);
		break;
	case OperationCenters::Op::eSwap:
		assert(op.location != NOT_ASSIGNED);
		assert(op.location2 != NOT_ASSIGNED);
		std::swap(mLocationTypeAssignment[op.location], mLocationTypeAssignment[op.location2]);
		break;
	default:
		assert(false);
		break;
	}

	return op;
}

void LocalSearchModel::doFixedCentersOp(OperationAssignments& op)
{
	switch (op.op)
	{
	case OperationAssignments::Op::eSetPrimary:
		// Remove the population and add annew
		if (mCityCenterAssignment[op.city].first != NOT_ASSIGNED) {
			mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= 10 * mBaseModel.getCities()[op.city].population;
			assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= 0);
		}
		if (op.location != NOT_ASSIGNED) {
			mLocationAssignedPopulation[op.location] += 10 * mBaseModel.getCities()[op.city].population;
		}
		std::swap(mCityCenterAssignment[op.city].first, op.location);
		updateHeuristicCity(op.city);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(op.location);
		break;
	case OperationAssignments::Op::eSetSecondary:
		// Remove the population and add annew 10%
		if (mCityCenterAssignment[op.city].second != NOT_ASSIGNED) {
			mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] -= mBaseModel.getCities()[op.city].population;
			assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] >= 0);
		}
		if (op.location != NOT_ASSIGNED) {
			mLocationAssignedPopulation[op.location] += mBaseModel.getCities()[op.city].population;
		}
		std::swap(mCityCenterAssignment[op.city].second, op.location);
		updateHeuristicCity(op.city);
		updateHeuristicLocation(mCityCenterAssignment[op.city].second);
		updateHeuristicLocation(op.location);
		break;
	case OperationAssignments::Op::eSwapPP:
		// Only do swap when the two cities are assigned to a center
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= 10 * mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] -= 10 * mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] += 10 * mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] += 10 * mBaseModel.getCities()[op.city2].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= 0);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] >= 0);
		std::swap(mCityCenterAssignment[op.city].first, mCityCenterAssignment[op.city2].first);
		updateHeuristicCity(op.city);
		updateHeuristicCity(op.city2);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(mCityCenterAssignment[op.city2].first);
		break;
	case OperationAssignments::Op::eSwapPS:
		assert(mCityCenterAssignment[op.city].first != NOT_ASSIGNED);
		assert(mCityCenterAssignment[op.city2].second != NOT_ASSIGNED);

		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= 10 * mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] -= mBaseModel.getCities()[op.city2].population;
		std::swap(mCityCenterAssignment[op.city].first, mCityCenterAssignment[op.city2].second);
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] += mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] += 10 * mBaseModel.getCities()[op.city].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= 0);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] >= 0);
		updateHeuristicCity(op.city);
		updateHeuristicCity(op.city2);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(mCityCenterAssignment[op.city2].second);
		break; 
	case OperationAssignments::Op::eSwapSS:
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] -= mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] -= mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] += mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] += mBaseModel.getCities()[op.city2].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] >= 0);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] >= 0);
		std::swap(mCityCenterAssignment[op.city].second, mCityCenterAssignment[op.city2].second);
		updateHeuristicCity(op.city);
		updateHeuristicCity(op.city2);
		updateHeuristicLocation(mCityCenterAssignment[op.city].second);
		updateHeuristicLocation(mCityCenterAssignment[op.city2].second);
		break;
	default:
		assert(false);
		break;
	}
}

void LocalSearchModel::updateHeuristicCity(uint32_t city)
{

	if (city == NOT_ASSIGNED) {
		return;
	}
	//assert(mActualAssignmentHeuristic - mCityHeuristicBuffer[city] >= 0.0f);

	mActualAssignmentHeuristic -= mCityHeuristicBuffer[city];

#define SQ(x, k) (x)*(x)*(k)

	const uint32_t pop = 10 * this->mBaseModel.getCities()[city].population;
	float newH = 0.0f;

	if (mCityCenterAssignment[city].first == NOT_ASSIGNED) {
		newH += SQ(pop + 2, 300);
	}
	else {
		// if not compatible assignment because of distance
		uint32_t l = mCityCenterAssignment[city].first;
		uint32_t t = mLocationTypeAssignment[l];
		if (!isCityLocationTypeCompatible(city, l, t, false)) {
			newH += mBaseModel.getLocations()[l].sqDist(mBaseModel.getCities()[city].cityPos);
		}
	}

	if (mCityCenterAssignment[city].second == NOT_ASSIGNED || 
		mCityCenterAssignment[city].first == mCityCenterAssignment[city].second) {
		newH += SQ(pop + 2, 100);
	}
	else {
		// if not compatible assignment because of distance
		uint32_t l = mCityCenterAssignment[city].second;
		uint32_t t = mLocationTypeAssignment[l];
		if (!isCityLocationTypeCompatible(city, l, t, true)) {
			newH += mBaseModel.getLocations()[l].sqDist(mBaseModel.getCities()[city].cityPos);
		}
		
	}

	assert(newH >= 0.0f);
	//assert(mActualAssignmentHeuristic + newH >= 0.0f);

	mCityHeuristicBuffer[city] = newH;
	mActualAssignmentHeuristic += newH;


#undef SQ
}

void LocalSearchModel::updateHeuristicLocation(uint32_t l)
{
	if (l == NOT_ASSIGNED) {
		return;
	}

	uint32_t t = mLocationTypeAssignment[l];


	mActualAssignmentHeuristic -= mLocationHeuristicBuffer[l];
	float newH = 0.0f;

#define SQ(x, k) (x)*(x)*(k)

	if (t != NOT_ASSIGNED) {
		if (mLocationAssignedPopulation[l] > 10 * mBaseModel.getCenterTypes()[t].maxPop) {
			const uint32_t m = std::max(3u, mLocationAssignedPopulation[l] - mBaseModel.getCenterTypes()[t].maxPop);
			newH += SQ(m, 2);
		}
		else {
			newH += mLocationAssignedPopulation[l] / mBaseModel.getCenterTypes()[t].maxPop;
		}
	}

	mLocationHeuristicBuffer[l] = newH;
	mActualAssignmentHeuristic += newH;
#undef SQ
	assert(newH >= 0.0f);
	//assert(mActualAssignmentHeuristic >= 0.0f);

}

bool LocalSearchModel::isCityCenterAssignmentFeasible() const
{
	// check distances and populations of centers
	for (uint32_t c = 0; c < mNumCities; ++c) {
		uint32_t l = mCityCenterAssignment[c].first;
		uint32_t t;
		if (l != NOT_ASSIGNED) {

			t = mLocationTypeAssignment[l];
			if (!isCityLocationTypeCompatible(c, l, t, false) ||
				mLocationAssignedPopulation[l] > 10 * mBaseModel.getCenterTypes()[t].maxPop) {
				return false;
			}
		}
		else {
			return false;
		}

		l = mCityCenterAssignment[c].second;
		if (l != NOT_ASSIGNED) {
			t = mLocationTypeAssignment[l];
			if (!isCityLocationTypeCompatible(c, l, t, true) ||
				mLocationAssignedPopulation[l] > 10 * mBaseModel.getCenterTypes()[t].maxPop) {
				return false;
			}
		}
		else {
			return false;
		}
	}

	return true;
}

bool LocalSearchModel::assertCityPopulation() const
{
	std::vector<uint32_t> centerServing(mNumLocations, 0);

	for (uint32_t i = 0; i < mCityCenterAssignment.size(); ++i) {

		if (mCityCenterAssignment[i].first != NOT_ASSIGNED) {
			centerServing[mCityCenterAssignment[i].first] += 10 * mBaseModel.getCities()[i].population;
		}
		if (mCityCenterAssignment[i].second != NOT_ASSIGNED) {
			centerServing[mCityCenterAssignment[i].second] += mBaseModel.getCities()[i].population;
		}

	}

	for (uint32_t i = 0; i < mNumLocations; ++i) {
		assert(centerServing[i] == mLocationAssignedPopulation[i]);
	}


	return true;
}

bool LocalSearchModel::localSearchAssignments()
{
	OperationAssignments bestOp;

	for (uint32_t c = 0; c < mNumCities; ++c) {
		updateHeuristicCity(c);
	}
	for (uint32_t l = 0; l < mNumLocations; ++l) {
		if (mLocationTypeAssignment[l] != NOT_ASSIGNED) {
			updateHeuristicLocation(l);
		}
	}

	constexpr int32_t limIt = 10000;
	int32_t maxIt = limIt;
	bool improvement = true;		
	float bestH = std::numeric_limits<float>::infinity();
	bool correctResult;
	while (!(correctResult = isCityCenterAssignmentFeasible()) &&
		maxIt-- && 
		improvement)
	{
		assertCityPopulation();

		OperationAssignments op{};
		OperationAssignments opCopy;
		improvement = false;

		auto updateOp = [&]() {
			if (mActualAssignmentHeuristic < bestH) {
				bestOp = op;
				bestH = mActualAssignmentHeuristic;
				improvement = true;
			}
		};

		// try set new primary and secondary locations
		for (uint32_t c = 0; c < mNumCities; ++c) {
			for (uint32_t l = 0; l < mNumLocations; ++l) {
				// only if center is set
				if (mLocationTypeAssignment[l] != NOT_ASSIGNED) {
					op.city = c;
					op.location = l;
					if (true || mCityCenterAssignment[c].first == NOT_ASSIGNED) {
						op.op = OperationAssignments::Op::eSetPrimary;
						opCopy = op;
						float ac = mActualAssignmentHeuristic;
						doFixedCentersOp(opCopy);
						updateOp();
						doFixedCentersOp(opCopy);
					}
					assert(op.location == l);
					assert(op.city == c);
					if (true || mCityCenterAssignment[c].second == NOT_ASSIGNED) {
						op.op = OperationAssignments::Op::eSetSecondary;
						opCopy = op;
						float ac = mActualAssignmentHeuristic;
						doFixedCentersOp(opCopy);
						updateOp();
						doFixedCentersOp(opCopy);
					}
					assert(op.location == l);
					assert(op.city == c);
				}
			}

			// Swapping
			for (uint32_t c2 = c + 1; c2 < mNumCities; ++c2) {
				if (mCityCenterAssignment[c].first != NOT_ASSIGNED &&
					mCityCenterAssignment[c2].first != NOT_ASSIGNED) {
					op.op = OperationAssignments::Op::eSwapPP;
					op.city = c;
					op.city2 = c2;
					doFixedCentersOp(op);
					updateOp();
					doFixedCentersOp(op);
				}
				if (mCityCenterAssignment[c].second != NOT_ASSIGNED &&
					mCityCenterAssignment[c2].second != NOT_ASSIGNED) {
					op.op = OperationAssignments::Op::eSwapSS;
					op.city = c;
					op.city2 = c2;
					doFixedCentersOp(op);
					updateOp();
					doFixedCentersOp(op);
				}

				if (mCityCenterAssignment[c].first != NOT_ASSIGNED &&
					mCityCenterAssignment[c2].second != NOT_ASSIGNED) {
					op.op = OperationAssignments::Op::eSwapPS;
					op.city = c;
					op.city2 = c2;
					doFixedCentersOp(op);
					updateOp();
					doFixedCentersOp(op);
				}

				if (mCityCenterAssignment[c].second != NOT_ASSIGNED &&
					mCityCenterAssignment[c2].first != NOT_ASSIGNED) {
					op.op = OperationAssignments::Op::eSwapPS;
					op.city = c2;
					op.city2 = c;
					doFixedCentersOp(op);
					updateOp();
					doFixedCentersOp(op);
				}
			}
		}


		if (improvement) {
			doFixedCentersOp(bestOp);
			//updateHeuristicCity(bestOp.city);
			if (bestOp.op != OperationAssignments::Op::eSetPrimary && 
				bestOp.op != OperationAssignments::Op::eSetSecondary) {
				//updateHeuristicCity(bestOp.city2);
			}
			mActualAssignmentHeuristic = bestH;
		}

	}

	//std::cout << "Num it: " << limIt - maxIt << ", still improving: " << improvement << ", sat: " << isCityCenterAssignmentFeasible() << std::endl;
	return correctResult;
#undef updateOP
}

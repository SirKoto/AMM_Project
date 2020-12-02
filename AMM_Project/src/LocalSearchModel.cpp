#include "LocalSearchModel.h"
#undef NDEBUG
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>

LocalSearchModel::LocalSearchModel(const Model& model) : 
	IModel(model), 
	mCityHeuristicBuffer(model.getCities().size(), 0.0f),
	mLocationAssignedPopulation(model.getLocations().size(), 0.0f),
	mLocationHeuristicBuffer(model.getLocations().size(), 0.0f)
{
	this->mMaxPop = 0.0f;
	for (const City& c : model.getCities())
	{
		mMaxPop = std::max(mMaxPop, c.population);
	}

	for (std::pair<uint32_t, uint32_t>& it : mCityCenterAssignment)
	{
		it.first = NOT_ASSIGNED;
		it.second = NOT_ASSIGNED;
	}
}

LocalSearchModel::LocalSearchModel(const IModel* model) : 
	IModel(model),
	mCityHeuristicBuffer(model->mNumCities, 0.0f),
	mLocationAssignedPopulation(model->mNumLocations, 0.0f),
	mLocationHeuristicBuffer(model->mNumLocations, 0.0f)
{
	this->mMaxPop = 0.0f;
	for (const City& c : model->mBaseModel.getCities())
	{
		mMaxPop = std::max(mMaxPop, c.population);
	}

	for (std::pair<uint32_t, uint32_t>& it : mCityCenterAssignment)
	{
		it.first = NOT_ASSIGNED;
		it.second = NOT_ASSIGNED;
	}
}

void LocalSearchModel::doFixedCentersOp(OperationAssignments& op)
{
	switch (op.op)
	{
	case OperationAssignments::Op::eSetPrimary:
		// Remove the population and add annew
		if (mCityCenterAssignment[op.city].first != NOT_ASSIGNED) {
			mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= mBaseModel.getCities()[op.city].population;
			assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= -1e-3f);
		}
		if (op.location != NOT_ASSIGNED) {
			mLocationAssignedPopulation[op.location] += mBaseModel.getCities()[op.city].population;
		}
		std::swap(mCityCenterAssignment[op.city].first, op.location);
		updateHeuristicCity(op.city);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(op.location);
		break;
	case OperationAssignments::Op::eSetSecondary:
		// Remove the population and add annew 10%
		if (mCityCenterAssignment[op.city].second != NOT_ASSIGNED) {
			mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] -= 0.1f * mBaseModel.getCities()[op.city].population;
			assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] >= -1e-3f);
		}
		if (op.location != NOT_ASSIGNED) {
			mLocationAssignedPopulation[op.location] += 0.1f * mBaseModel.getCities()[op.city].population;
		}
		std::swap(mCityCenterAssignment[op.city].second, op.location);
		updateHeuristicCity(op.city);
		updateHeuristicLocation(mCityCenterAssignment[op.city].second);
		updateHeuristicLocation(op.location);
		break;
	case OperationAssignments::Op::eSwapPP:
		// Only do swap when the two cities are assigned to a center
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] -= mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] += mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] += mBaseModel.getCities()[op.city2].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= -1e-3f);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].first] >= -1e-3f);
		std::swap(mCityCenterAssignment[op.city].first, mCityCenterAssignment[op.city2].first);
		updateHeuristicCity(op.city);
		updateHeuristicCity(op.city2);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(mCityCenterAssignment[op.city2].first);
		break;
	case OperationAssignments::Op::eSwapPS:
		assert(mCityCenterAssignment[op.city].first != NOT_ASSIGNED);
		assert(mCityCenterAssignment[op.city2].second != NOT_ASSIGNED);

		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] -= mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] -= 0.1f * mBaseModel.getCities()[op.city2].population;
		std::swap(mCityCenterAssignment[op.city].first, mCityCenterAssignment[op.city2].second);
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] += 0.1f * mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] += mBaseModel.getCities()[op.city].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].first] >= -1e-3f);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] >= -1e-3f);
		updateHeuristicCity(op.city);
		updateHeuristicCity(op.city2);
		updateHeuristicLocation(mCityCenterAssignment[op.city].first);
		updateHeuristicLocation(mCityCenterAssignment[op.city2].second);
		break; 
	case OperationAssignments::Op::eSwapSS:
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] -= 0.1f * mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] -= 0.1f * mBaseModel.getCities()[op.city2].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] += 0.1f * mBaseModel.getCities()[op.city].population;
		mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] += 0.1f * mBaseModel.getCities()[op.city2].population;
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city].second] >= -1e-3f);
		assert(mLocationAssignedPopulation[mCityCenterAssignment[op.city2].second] >= -1e-3f);
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

	mActualAssignmentHeuristic -= mCityHeuristicBuffer[city];

#define SQ(x, k) (x)*(x)*(k)

	const float pop = this->mBaseModel.getCities()[city].population;
	double newH = 0.0f;

	if (mCityCenterAssignment[city].first == NOT_ASSIGNED) {
		newH += SQ(mMaxPop - pop + 2, 300);
	}
	else if(false){
		// if not compatible assignment because of distance
		uint32_t l = mCityCenterAssignment[city].first;
		uint32_t t = mLocationTypeAssignment[l];
		if (!isCityLocationTypeCompatible(city, l, t, false)) {
			newH += mBaseModel.getLocations()[l].sqDist(mBaseModel.getCities()[city].cityPos);
		}
	}

	if (mCityCenterAssignment[city].second == NOT_ASSIGNED) {
		newH += SQ(mMaxPop - pop + 2, 100);
	}
	else if(false){
		// if not compatible assignment because of distance
		uint32_t l = mCityCenterAssignment[city].second;
		uint32_t t = mLocationTypeAssignment[l];
		if (!isCityLocationTypeCompatible(city, l, t, true)) {
			newH += mBaseModel.getLocations()[l].sqDist(mBaseModel.getCities()[city].cityPos);
		}
		
	}

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
	double newH = 0.0f;

#define SQ(x, k) (x)*(x)*(k)

	if (t != NOT_ASSIGNED) {
		if (mLocationAssignedPopulation[l] > mBaseModel.getCenterTypes()[t].maxPop) {
			newH += SQ(mLocationAssignedPopulation[l] / mBaseModel.getCenterTypes()[t].maxPop * 10.0f, 1.0f);
		}
		else {
			newH += mLocationAssignedPopulation[l] / mBaseModel.getCenterTypes()[t].maxPop * 1.5f;
		}
	}

	mLocationHeuristicBuffer[l] = newH;
	mActualAssignmentHeuristic += newH;
#undef SQ

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
				mLocationAssignedPopulation[l] > mBaseModel.getCenterTypes()[t].maxPop +1e-4) {
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
				mLocationAssignedPopulation[l] > mBaseModel.getCenterTypes()[t].maxPop + 1e-4) {
				return false;
			}
		}
		else {
			return false;
		}
	}

	return true;
}

bool LocalSearchModel::assertCityPopulation()
{
	std::map<uint32_t, float> centerServing;
	for (uint32_t i = 0; i < mNumLocations; ++i) {
		centerServing.insert({ i, 0.0f });
	}

	for (uint32_t i = 0; i < mCityCenterAssignment.size(); ++i) {

		if (centerServing.count(mCityCenterAssignment[i].first)) {
			centerServing.at(mCityCenterAssignment[i].first) += mBaseModel.getCities()[i].population;
		}
		if (centerServing.count(mCityCenterAssignment[i].second)) {
			centerServing.at(mCityCenterAssignment[i].second) += 0.1f * mBaseModel.getCities()[i].population;
		}

	}

	for (uint32_t i = 0; i < mNumLocations; ++i) {
		assert(std::abs(centerServing.at(i) - mLocationAssignedPopulation[i]) < 1e-3f);
		assert(mLocationAssignedPopulation[i] >= -1e-3f);
		mLocationAssignedPopulation[i] = centerServing.at(i);
	}


	return true;
}

void LocalSearchModel::localSearchAssignments()
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
	double bestH = std::numeric_limits<float>::infinity();

	while (!isCityCenterAssignmentFeasible() && 
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
						double ac = mActualAssignmentHeuristic;
						doFixedCentersOp(opCopy);
						updateOp();
						doFixedCentersOp(opCopy);
						assert(std::abs(ac - mActualAssignmentHeuristic) <= 1e-3);
					}
					assert(op.location == l);
					assert(op.city == c);
					if (true || mCityCenterAssignment[c].second == NOT_ASSIGNED) {
						op.op = OperationAssignments::Op::eSetSecondary;
						opCopy = op;
						double ac = mActualAssignmentHeuristic;
						doFixedCentersOp(opCopy);
						updateOp();
						doFixedCentersOp(opCopy);
						assert(std::abs(ac - mActualAssignmentHeuristic) <= 1e-3);
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
			if (mActualAssignmentHeuristic == 400.0f) {
				std::cout << "uwu" << std::endl;
			}
			doFixedCentersOp(bestOp);
			//updateHeuristicCity(bestOp.city);
			if (bestOp.op != OperationAssignments::Op::eSetPrimary && 
				bestOp.op != OperationAssignments::Op::eSetSecondary) {
				//updateHeuristicCity(bestOp.city2);
			}
			assert(std::abs(bestH - mActualAssignmentHeuristic) <= 1e-3);
			mActualAssignmentHeuristic = bestH;
		}

	}

	std::cout << "Num it: " << limIt - maxIt << ", still improving: " << improvement << ", sat: " << isCityCenterAssignmentFeasible() << std::endl;
#undef updateOP
}

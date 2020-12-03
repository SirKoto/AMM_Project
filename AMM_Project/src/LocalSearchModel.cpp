#include "LocalSearchModel.h"
#undef NDEBUG
#include <cassert>
#include <algorithm>
#include <iostream>
#include <map>

LocalSearchModel::LocalSearchModel(const Model& model) : 
	IModel(model), 
	mCityHeuristicBuffer(model.getCities().size(), 0.0f),
	mLocationAssignedPopulation(model.getLocations().size(), 0),
	mLocationHeuristicBuffer(model.getLocations().size(), 0.0f)
{
	this->mMaxPop10 = 0;
	for (const City& c : model.getCities())
	{
		mMaxPop10 = std::max(mMaxPop10, 10 * c.population);
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
	mLocationAssignedPopulation(model->mNumLocations, 0),
	mLocationHeuristicBuffer(model->mNumLocations, 0.0f)
{
	this->mMaxPop10 = 0;
	for (const City& c : model->mBaseModel.getCities())
	{
		mMaxPop10 = std::max(mMaxPop10, 10 * c.population);
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

	mActualAssignmentHeuristic -= mCityHeuristicBuffer[city];

#define SQ(x, k) (x)*(x)*(k)

	const uint32_t pop = 10 * this->mBaseModel.getCities()[city].population;
	float newH = 0.0f;

	if (mCityCenterAssignment[city].first == NOT_ASSIGNED) {
		newH += SQ(mMaxPop10 - pop + 2, 300);
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
		newH += SQ(mMaxPop10 - pop + 2, 100);
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
	float newH = 0.0f;

#define SQ(x, k) (x)*(x)*(k)

	if (t != NOT_ASSIGNED) {
		if (mLocationAssignedPopulation[l] > 10 * mBaseModel.getCenterTypes()[t].maxPop) {
			newH += SQ(mLocationAssignedPopulation[l] / mBaseModel.getCenterTypes()[t].maxPop, 1.0f);
		}
		else {
			newH += mLocationAssignedPopulation[l] / mBaseModel.getCenterTypes()[t].maxPop / 10;
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

bool LocalSearchModel::assertCityPopulation()
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
	float bestH = std::numeric_limits<float>::infinity();

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
						float ac = mActualAssignmentHeuristic;
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
						float ac = mActualAssignmentHeuristic;
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

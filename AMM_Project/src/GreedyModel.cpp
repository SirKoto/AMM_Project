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
		Candidate bestAction = findBestAddition();

		/*if (bestAction.first == NOT_ASSIGNED) {
			break;
		}*/

		applyAction(bestAction);

		float n = numToAssign();
		if (n > actual + 0.01f) {
			actual = n;
			std::cout << actual * 100.0f <<"%" << std::endl;
		}
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

GreedyModel::Candidate GreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		Candidate infeasible;
		infeasible.fit=-1;
		return infeasible;
	}

	uint32_t num = 0;
	float pop = 0.0f;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);

	Candidate bestCandidate;
	std::vector<char> assignments;
	// First pass, primary heavy
	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {
			float newPop = pop + mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			assignments.push_back(1);
			pop = newPop;
		}
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) {
			float newPop = pop + 0.1f * mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			assignments.push_back(2);
			pop = newPop;
		}
		else {
			assignments.push_back(0);
		}
	}
	bestCandidate.fit=pop/mBaseModel.getCenterTypes()[t].cost;
	bestCandidate.assigns=assignments;
	bestCandidate.type=t;
	bestCandidate.loc=l;
	// Second pass, secondary heavy
	assignments.clear();	
	float totalSecondaryPop=0;
	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)){
			float newPop=totalSecondaryPop+mBaseModel.getCities()[c].population*0.1;
			if (newPop>maxPop) {
				break;	
			}
			assignments.push_back(2);
			totalSecondaryPop+=mBaseModel.getCities()[c].population*0.1;
		}
		else assignments.push_back(0);
	}
	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {
			float newPop = totalSecondaryPop + mBaseModel.getCities()[c].population*0.9;
			totalSecondaryPop-=mBaseModel.getCities()[c].population*0.1;
			if (newPop > maxPop) {
				break;
			}
			assignments[ci]=1;
			float fit=newPop/mBaseModel.getCenterTypes()[t].cost;
			if (fit>bestCandidate.fit) {
				bestCandidate.fit=fit;
				bestCandidate.assigns=assignments;
				bestCandidate.type=t;
				bestCandidate.loc=l;
			}
		}
	}
	return bestCandidate;
}

const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

GreedyModel::Candidate GreedyModel::findBestAddition() const
{
	Candidate res ;
	float actualFitness = 0;

	for (uint32_t l = 0; l < mNumLocations; ++l) {
		for (uint32_t t = 0; t < mNumTypes; ++t) {
			Candidate candidate = tryAddGreedy(l, t);

			if (candidate.fit > actualFitness) {
				actualFitness = candidate.fit;
				res=candidate;
			}
		}
	}

	return res;
}

void GreedyModel::applyAction(Candidate bestActions)
{
	/*
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
	}*/
	const uint32_t* ptr = getCitiesSorted(bestActions.loc);
	for (uint32_t ci=0;ci<bestActions.assigns.size();++ci) {
		uint32_t c = *(ptr + ci);
		if (bestActions.assigns[ci]==1) {
			mCityCenterAssignment[c].first = bestActions.loc;
		}
		else if (bestActions.assigns[ci]==2) {
			mCityCenterAssignment[c].second = bestActions.loc;
		}
	}

	mLocationTypeAssignment[bestActions.loc] = bestActions.type;
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


	return os;
}

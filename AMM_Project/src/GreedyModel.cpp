#include "GreedyModel.h"

#include <algorithm>
#include <map>
#include <thread>
#include <omp.h>

GreedyModel::GreedyModel(const Model& model) : IModel(model)
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

	const int processor_count = std::thread::hardware_concurrency();
	int perThread = mNumLocations / processor_count;
	if (perThread < 1) perThread = 1;
	std::vector<Candidate> bestCandidates(processor_count);

	while (!isSolution()) {
		Candidate bestAction = findBestAddition(bestCandidates,perThread,processor_count);
		if (bestAction.fit<0) break;
		applyAction(bestAction);
		float n = numToAssign();
		if (n > actual + 0.01f) {
			actual = n;
			std::cout << actual * 100.0f <<"%" << std::endl;
		}
	}
}

GreedyModel::Candidate GreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t, std::vector<char> assignments) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		Candidate infeasible;
		infeasible.fit = -1;
		return infeasible;
	}
	std::fill(assignments.begin(), assignments.end(), 0);
	uint32_t num = 0;
	float pop = 0.0f;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);

	Candidate bestCandidate;
	uint32_t ci = 0;
	for (ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {
			float newPop = pop + mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			assignments[ci]=1;
			pop = newPop;
		}
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) {
			float newPop = pop + 0.1f * mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			assignments[ci]=2;
			pop = newPop;
		}
		else {
			assignments[ci]=0;
		}
	}
	if (ci < mNumCities) assignments[ci + (uint32_t) 1] = 3;
	bestCandidate.fit = pop / mBaseModel.getCenterTypes()[t].cost;
	bestCandidate.assigns = assignments;
	bestCandidate.type = t;
	bestCandidate.loc = l;

	return bestCandidate;
}
const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

GreedyModel::Candidate GreedyModel::findBestAddition(std::vector<Candidate> bestCandidates,int perThread, int processor_count) const
{
	    

    #pragma omp parallel num_threads(processor_count)
    {
        const int c = omp_get_thread_num(); 
        Candidate bestCandidate;
        bestCandidate.fit=-1.0;
		std::vector<char> assignments(mNumCities);
        for (uint32_t l = c*perThread ; l < (c+1)*perThread && l < mNumLocations; ++l) {
            for (uint32_t t = 0; t < mNumTypes; ++t) {
                Candidate currentCandidate = tryAddGreedy(l, t, assignments);
                if (currentCandidate.fit>bestCandidate.fit) {
                    bestCandidate=currentCandidate;
                }
            }
        }
		bestCandidates[omp_get_thread_num()] = bestCandidate;
    }

	float bestFit = -1.0;
	int bestPos = 0;
	for (int i = 0; i < processor_count; ++i) {
		if (bestCandidates[i].fit > bestFit) {
			bestFit = bestCandidates[i].fit;
			bestPos = i;
		}
	}
    return bestCandidates[bestPos];
}


void GreedyModel::applyAction(Candidate bestActions)
{
	mLocationTypeAssignment[bestActions.loc] = bestActions.type;
	const uint32_t* ptr = getCitiesSorted(bestActions.loc);
	for (uint32_t ci=0;ci<mNumCities;++ci) {
		uint32_t c = *(ptr + ci);
		if (bestActions.assigns[ci] == 1) {
			mCityCenterAssignment[c].first = bestActions.loc;
		}
		else if (bestActions.assigns[ci] == 2) {
			mCityCenterAssignment[c].second = bestActions.loc;
		}
		else if (bestActions.assigns[ci] == 3) return;
	}
}


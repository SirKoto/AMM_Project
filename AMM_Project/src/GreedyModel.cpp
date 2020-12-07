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

	while (!isSolutionFast()) {
		Candidate bestAction = findBestAddition(bestCandidates, perThread, processor_count);
		if (bestAction.fit==-std::numeric_limits<float>::infinity()) break;
		applyAction(bestAction);
		float n = numToAssign();
		if (n > actual + 0.01f) {
			actual = n;
			std::cout << actual * 100.0f <<"%" << std::endl;
		}
	}
}

GreedyModel::Candidate GreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t, std::vector<char>& assignments) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		Candidate infeasible;
		infeasible.fit = -std::numeric_limits<float>::infinity();
		return infeasible;
	}
	std::fill(assignments.begin(), assignments.end(), 0);
	uint32_t num = 0;
	uint32_t pop = 0;
	const uint32_t maxPop = 10 * mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);

	Candidate bestCandidate;
	uint32_t ci = 0;
	for (ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) {
			uint32_t newPop = pop + 10 * mBaseModel.getCities()[c].population;
			if (newPop > maxPop) {
				break;
			}
			assignments[ci]=1;
			pop = newPop;
		}
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) {
			uint32_t newPop = pop + mBaseModel.getCities()[c].population;
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
	if (ci < mNumCities - 1) assignments[ci + 1] = 3;
	int freeCities = 0;
	for (int it = ci; it < mNumCities; ++it) {
		uint32_t c = *(ptr + it);
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) freeCities += 4;
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) freeCities += 2;
	}
	bestCandidate.fit = (static_cast<float>(pop) * 0.1f / mBaseModel.getCenterTypes()[t].cost) - freeCities;
	bestCandidate.assigns = assignments;
	bestCandidate.type = t;
	bestCandidate.loc = l;

	return bestCandidate;
}
const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

GreedyModel::Candidate GreedyModel::findBestAddition(std::vector<Candidate> bestCandidates, uint32_t perThread, int processor_count) const
{
    #pragma omp parallel num_threads(processor_count)
    {
        const int c = omp_get_thread_num(); 
        Candidate bestCandidate;
        bestCandidate.fit= -std::numeric_limits<float>::infinity();
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

	float bestFit = -std::numeric_limits<float>::infinity();
	int bestPos = 0;
	for (int i = 0; i < processor_count; ++i) {
		if (bestCandidates[i].fit > bestFit) {
			bestFit = bestCandidates[i].fit;
			bestPos = i;
		}
	}
    return std::move(bestCandidates[bestPos]);
}


void GreedyModel::applyAction(const Candidate& bestActions)
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

void GreedyModel::runParallelLocalSearch()
{
	const int processor_count = std::thread::hardware_concurrency();
	int perThread = mNumLocations / processor_count;
	if (perThread < 1) perThread = 1;
	std::vector<Swap> bestSwaps(processor_count);
	int iter = 10000;
	bool improvement = true;
	while (iter--) {
		Swap bestSwap = findBestSwap(bestSwaps, perThread, processor_count);
		if (bestSwap.fit <= 0) {
			improvement = false;
			break;
		}
		applySwap(bestSwap);
		trimLocations();
	}
}


void GreedyModel::trimLocations() {
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
	for (uint32_t cl = 0; cl < mNumLocations; ++cl) {
		uint32_t type = mLocationTypeAssignment[cl];
		uint32_t bestType = type;
		if (type != NOT_ASSIGNED) {
			float bestCost = mBaseModel.getCenterTypes()[type].cost;
			for (uint32_t t = 0; t < mNumTypes; ++t) {
				if (type != t) {
					if (mBaseModel.getCenterTypes()[t].cost >= centerServing.at(cl)) {
						if (bestCost > mBaseModel.getCenterTypes()[t].cost) {
							bestCost = mBaseModel.getCenterTypes()[t].cost;
							bestType = t;
						}
					}
				}
			}
		}
		mLocationTypeAssignment[cl] = bestType;
	}
}

void GreedyModel::applySwap(Swap bestSwap) {
	if (bestSwap.primarySwap) {
		mCityCenterAssignment[bestSwap.city].first = bestSwap.location;
	}
	else if (!bestSwap.primarySwap){
		mCityCenterAssignment[bestSwap.city].second = bestSwap.location;
	}

}

float getLoad(std::map<uint32_t, float> centerServing, uint32_t location, uint32_t maxPop) {
	return centerServing.at(location) / maxPop;
}

GreedyModel::Swap GreedyModel::findBestSwap(std::vector<Swap> bestSwaps, uint32_t perThread, int processor_count)
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
	#pragma omp parallel num_threads(processor_count)
	{

		float reducedLoad = 0;
		const int c = omp_get_thread_num();
		Swap bestSwap;
		bestSwap.fit = 0;
		std::vector<City> cities = mBaseModel.getCities();
		for (uint32_t ci = c * perThread; ci < (c + 1) * perThread && ci < mNumCities; ++ci) {
			City current = cities[ci];
			uint32_t locationPrimary = mCityCenterAssignment[ci].first;
			uint32_t locationSecondary = mCityCenterAssignment[ci].second;
			for (uint32_t cl = 0; cl < mNumLocations; ++cl) {
				if (mLocationTypeAssignment[cl]!=NOT_ASSIGNED) {
					uint32_t destiny = centerServing.at(cl);
					if (locationSecondary == NOT_ASSIGNED) {
						if ((isCityLocationTypeCompatible(c, cl, mLocationTypeAssignment[cl], 1)) && ((destiny + current.population*0.1 ) < mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop)) {
							Swap aux;
							aux.location = cl;
							aux.city = ci;
							aux.fit = std::numeric_limits<float>::infinity();
							aux.primarySwap = false;
							bestSwap = aux;
						}
					}
					else if (locationPrimary == NOT_ASSIGNED) {
						if ((isCityLocationTypeCompatible(c, cl, mLocationTypeAssignment[cl], 0)) && ((destiny + current.population) < mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop)) {
							Swap aux;
							aux.location = cl;
							aux.city = ci;
							aux.fit = std::numeric_limits<float>::infinity();
							aux.primarySwap = true;
							bestSwap = aux;
						}
					}
					else
					{
						if (isCityLocationTypeCompatible(c, cl, mLocationTypeAssignment[cl], 0)) {
							uint32_t destiny = centerServing.at(cl);
							if (((destiny + current.population) < mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop)) {
								uint32_t origin = centerServing.at(locationPrimary);
								float newLoadOrigin = (origin - current.population) / mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationPrimary]].maxPop;
								float reducedLoad = origin - newLoadOrigin;
								if (bestSwap.fit < reducedLoad) {
									Swap aux;
									aux.location = cl;
									aux.city = ci;
									aux.fit = reducedLoad;
									aux.primarySwap = true;
									bestSwap = aux;
								}
							}
						}
						if (isCityLocationTypeCompatible(c, cl, mLocationTypeAssignment[cl], 1)) {
							uint32_t destiny = centerServing.at(cl);
							if (( (destiny + current.population*0.1 ) < mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop)) {
								uint32_t origin = centerServing.at(locationSecondary);
								float newLoadOrigin = (origin - current.population) / mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationSecondary]].maxPop;
								float reducedLoad = origin - newLoadOrigin;
								if (bestSwap.fit < reducedLoad) {
									Swap aux;
									aux.location = cl;
									aux.city = ci;
									aux.fit = reducedLoad;
									aux.primarySwap = false;
									bestSwap = aux;
								}
							}
						}
					}
				}
			}
		}
		bestSwaps[omp_get_thread_num()] = bestSwap;
	}
	float bestFit = -std::numeric_limits<float>::infinity();
	int bestPos = 0;
	for (int i = 0; i < processor_count; ++i) {
		if (bestSwaps[i].fit > bestFit) {
			bestFit = bestSwaps[i].fit;
			bestPos = i;
		}
	}
	return std::move(bestSwaps[bestPos]);
}

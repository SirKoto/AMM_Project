#include "GreedyModel.h"

#include <algorithm>
#include <map>
#include <thread>
#include <omp.h>

const double EulerConstant = std::exp(1.0);
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
	int perThread = static_cast<int>((std::ceil(static_cast<float>(mNumLocations) / processor_count)));
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
	trimLocations();
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
		if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)) freeCities += 2;
		else if (mCityCenterAssignment[c].second == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 1)) freeCities += 1;
	}
	bestCandidate.fit = (static_cast<float>(pop) * 0.1f / mBaseModel.getCenterTypes()[t].cost) - freeCities/5;
	bestCandidate.assigns = std::vector<char> (assignments);
	bestCandidate.type = t;
	bestCandidate.loc = l;

	return bestCandidate;
}
const uint32_t* GreedyModel::getCitiesSorted(const uint32_t l) const
{
	return mSortedCities.data() + l * mNumCities;
}

GreedyModel::Candidate GreedyModel::findBestAddition(std::vector<Candidate> &bestCandidates, uint32_t perThread, int processor_count) const
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
	const uint32_t* ptr = getCitiesSorted(bestActions.loc);
	for (uint32_t ci=0;ci<mNumCities;++ci) {
		uint32_t c = *(ptr + ci);
		if (bestActions.assigns[ci] == 1) {
			mCityCenterAssignment[c].first = bestActions.loc;
		}
		else if (bestActions.assigns[ci] == 2) {
			mCityCenterAssignment[c].second = bestActions.loc;
		}
		else if (bestActions.assigns[ci] == 3) {
			break;
		}
	}
	mLocationTypeAssignment[bestActions.loc] = bestActions.type;
}

void GreedyModel::runParallelLocalSearch()
{
	const int processor_count = std::thread::hardware_concurrency();
	int perThread = static_cast<int>((std::ceil(static_cast<float>(mNumLocations) / processor_count)));
	if (perThread < 1) perThread = 1;
	std::vector<Swap> bestSwaps(processor_count);
	int iter = 10000;
	uint32_t noImprovement = 0;
	float oldFit = 0;
	while (iter-- && noImprovement<5) {
		Swap bestSwap = findBestSwap(bestSwaps, perThread, processor_count);
		if (bestSwap.fit <= 0) break;
		else if (oldFit >= bestSwap.fit) noImprovement++;
		else noImprovement = 0;
		oldFit = bestSwap.fit;
		applySwap(bestSwap);
	}
	trimLocations();
}

void GreedyModel::trimLocations() {
	std::vector<float> centerServing(mNumCities, 0);
	std::vector<float> maxDistLoc(mNumLocations, 0);
	std::vector<float> maxDistLocSec(mNumLocations, 0);
	for (uint32_t i = 0; i < mCityCenterAssignment.size(); ++i) {
		if (mCityCenterAssignment[i].first != NOT_ASSIGNED) {
			vec2 position=mBaseModel.getCities()[i].cityPos;
			float dist = mBaseModel.getLocations()[mCityCenterAssignment[i].first].sqDist(position);
			if (dist > maxDistLoc[mCityCenterAssignment[i].first]) maxDistLoc[mCityCenterAssignment[i].first] = dist;
			centerServing[mCityCenterAssignment[i].first] += mBaseModel.getCities()[i].population * 10;
		}
		if (mCityCenterAssignment[i].second != NOT_ASSIGNED) {
			vec2 position = mBaseModel.getCities()[i].cityPos;
			float dist = mBaseModel.getLocations()[mCityCenterAssignment[i].second].sqDist(position);
			if (dist > maxDistLocSec[mCityCenterAssignment[i].second]) maxDistLocSec[mCityCenterAssignment[i].second] = dist;
			centerServing[mCityCenterAssignment[i].second] += mBaseModel.getCities()[i].population;
		}

	}
	for (uint32_t cl = 0; cl < mNumLocations; ++cl) {
		uint32_t type = mLocationTypeAssignment[cl];
		uint32_t bestType = type;
		if (type != NOT_ASSIGNED) {
			float bestCost = mBaseModel.getCenterTypes()[type].cost;
			if (centerServing[cl] == 0.0f) {
				bestType = NOT_ASSIGNED;
			}
			else {
				for (uint32_t t = 0; t < mNumTypes; ++t) {
					if (type != t) {
						if (mBaseModel.getCenterTypes()[t].maxPop >= centerServing[cl] && mBaseModel.getCenterTypes()[t].serveDist>=maxDistLoc[cl] && mBaseModel.getCenterTypes()[t].serveDist>=3* maxDistLocSec[cl]) {
							if (bestCost > mBaseModel.getCenterTypes()[t].cost) {
								bestCost = mBaseModel.getCenterTypes()[t].cost;
								bestType = t;
							}
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

float getLoad(std::vector<float> centerServing, uint32_t location, uint32_t maxPop) {
	return centerServing[location] / maxPop;
}


double GreedyModel::getUsefulLoad(std::vector<float> centerServing) {
	double usefulLoad = 0;
	uint32_t loc = 0;
	for (uint32_t l = 0; l < mNumLocations; ++l) {
		if (centerServing[l] != 0) {
			uint32_t type = mLocationTypeAssignment[l];
			double load = static_cast<double> (centerServing[l]) / static_cast<double> (mBaseModel.getCenterTypes()[type].maxPop*10);
			double expLoad = exp(load);
			usefulLoad += expLoad;
		}
	}
	return usefulLoad;
}

GreedyModel::Swap GreedyModel::findBestSwap(std::vector<Swap> bestSwaps, uint32_t perThread, int processor_count)
{

	std::vector<float> centerServing(mNumCities,0);
	for (uint32_t i = 0; i < mCityCenterAssignment.size(); ++i) {
		if (mCityCenterAssignment[i].first!=NOT_ASSIGNED) {
			centerServing[mCityCenterAssignment[i].first] += mBaseModel.getCities()[i].population*10;
		}
		if (mCityCenterAssignment[i].second != NOT_ASSIGNED) {
			centerServing[mCityCenterAssignment[i].second] += mBaseModel.getCities()[i].population;
		}
	}
	#pragma omp parallel num_threads(processor_count)
	{

		const int c = omp_get_thread_num();
		Swap bestSwap;
		bestSwap.fit = 0;
		std::vector<City> cities = mBaseModel.getCities();
		for (uint32_t ci = c * perThread; ci < (c + 1) * perThread && ci < mNumCities; ++ci) {
			City current = cities[ci];
			uint32_t locationPrimary = mCityCenterAssignment[ci].first;
			uint32_t locationSecondary = mCityCenterAssignment[ci].second;
			double usefulLoad = getUsefulLoad(centerServing);
			for (uint32_t cl = 0; cl < mNumLocations; ++cl) {
				if (mLocationTypeAssignment[cl]!=NOT_ASSIGNED) {
					float destiny = centerServing.at(cl);
					if (locationSecondary == NOT_ASSIGNED &&  (cl!=locationPrimary)) {
						if ((isCityLocationTypeCompatible(ci, cl, mLocationTypeAssignment[cl], 1)) && ((destiny + current.population) <= mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10)) {
							Swap aux;
							aux.location = cl;
							aux.city = ci;
							double newLoadDestiny = (destiny + current.population * 10) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
							double oldLoadDestiny = destiny / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
							newLoadDestiny = exp(newLoadDestiny);
							oldLoadDestiny = exp(oldLoadDestiny);
							double newUsefulLoad = usefulLoad - oldLoadDestiny + newLoadDestiny;
							aux.fit = newUsefulLoad +EulerConstant*mNumLocations;
							aux.primarySwap = false;
							bestSwap = aux;
						}
					}
					if (locationPrimary == NOT_ASSIGNED && (locationSecondary!=cl)) {
						if ((isCityLocationTypeCompatible(ci, cl, mLocationTypeAssignment[cl], 0)) && ((destiny + current.population*10) <= mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10)) {
							Swap aux;
							aux.location = cl;
							aux.city = ci;
							double newLoadDestiny = (destiny + current.population) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
							double oldLoadDestiny = destiny / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
							newLoadDestiny = exp(newLoadDestiny);
							oldLoadDestiny = exp(oldLoadDestiny);
							double newUsefulLoad = usefulLoad - oldLoadDestiny + newLoadDestiny;
							aux.fit = newUsefulLoad/2 + EulerConstant * mNumLocations;
							aux.primarySwap = true;
							bestSwap = aux;
						}
					}
					if (locationPrimary != NOT_ASSIGNED && isCityLocationTypeCompatible(ci, cl, mLocationTypeAssignment[cl], 0) && (cl != locationSecondary)) {
							float destiny = centerServing.at(cl);
							float origin = centerServing.at(locationPrimary);
							if (((destiny + current.population*10) <= mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10)) {
								double newLoadDestiny = (destiny + current.population*10) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
								double newLoadOrigin = (origin - current.population*10) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationPrimary]].maxPop * 10);
								double oldLoadOrigin = origin / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationPrimary]].maxPop * 10);
								double oldLoadDestiny = destiny / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop * 10);
								newLoadDestiny = exp(newLoadDestiny);
								newLoadOrigin = exp(newLoadOrigin);
								oldLoadOrigin = exp(oldLoadOrigin);
								oldLoadDestiny = exp(oldLoadDestiny);

								double loadChange = -((oldLoadOrigin) + (oldLoadDestiny)) + ((newLoadDestiny) + (newLoadOrigin));
								double newUsefulLoad = usefulLoad + loadChange;
								if (bestSwap.fit < newUsefulLoad) {
									Swap aux;
									aux.location = cl;
									aux.city = ci;
									aux.fit = newUsefulLoad;
									aux.primarySwap = true;
									bestSwap = aux;
								}
							}
						}
					if (locationSecondary != NOT_ASSIGNED && isCityLocationTypeCompatible(ci, cl, mLocationTypeAssignment[cl], 1) && (cl != locationPrimary)) {
							float destiny = centerServing.at(cl);
							float origin = centerServing.at(locationSecondary);
							if (( (destiny + current.population ) <= mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10)) {
								double newLoadDestiny = (destiny + current.population) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10);
								double newLoadOrigin = (origin - current.population) / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationSecondary]].maxPop*10);
								double oldLoadOrigin = origin / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[locationSecondary]].maxPop*10);
								double oldLoadDestiny= destiny / (mBaseModel.getCenterTypes()[mLocationTypeAssignment[cl]].maxPop*10);
								newLoadDestiny = exp(newLoadDestiny);
								newLoadOrigin = exp(newLoadOrigin);
								oldLoadOrigin = exp(oldLoadOrigin);
								oldLoadDestiny = exp(oldLoadDestiny);
								double loadChange = -((oldLoadOrigin) + (oldLoadDestiny)) + ((newLoadDestiny) + (newLoadOrigin));
								double newUsefulLoad = usefulLoad + loadChange;
								if (bestSwap.fit < newUsefulLoad) {
									Swap aux;
									aux.location = cl;
									aux.city = ci;
									aux.fit = newUsefulLoad;
									aux.primarySwap = false;
									bestSwap = aux;
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
		//std::cout << bestSwaps[i].fit << std::endl;
		if (bestSwaps[i].fit > bestFit) {
			bestFit = bestSwaps[i].fit;
			bestPos = i;
		}
	}
	return std::move(bestSwaps[bestPos]);
}
void GreedyModel::GRASPConstructivePhase(float alpha) {
	std::vector<Candidate> candidateList(mNumTypes * mNumLocations);
	std::vector<Candidate> RCL(mNumTypes * mNumLocations);
	const int processor_count = std::thread::hardware_concurrency();
	int perThread = static_cast<int>((std::ceil(static_cast<float>(mNumLocations) / processor_count)));
	if (perThread < 1) perThread = 1;
	std::vector<std::vector<char>> assignments(processor_count,std::vector<char>(mNumCities));
	while (!isSolutionFast()) {
		Candidate GRASPCandidate = findCandidateGRASP(candidateList, RCL, assignments,perThread, processor_count, alpha);
		if (GRASPCandidate.fit == -std::numeric_limits<float>::infinity()) break;
		applyAction(GRASPCandidate);
	}	
}

void GreedyModel::purge() {
	for (uint32_t l = 0; l < mNumLocations; ++l) {
		mLocationTypeAssignment[l] = NOT_ASSIGNED;
	}
	for (uint32_t c = 0; c < mNumCities; ++c) {
		mCityCenterAssignment[c].first = NOT_ASSIGNED;
		mCityCenterAssignment[c].second = NOT_ASSIGNED;
	}

}

GreedyModel::Candidate GreedyModel::findCandidateGRASP(std::vector<GreedyModel::Candidate> &candidates, std::vector<Candidate> &RCL, std::vector<std::vector<char>> &assignments , uint32_t perThread, int processor_count, float alpha) const
{

std::vector<float> bestFits(processor_count);
std::vector<float> worstFits(processor_count);

#pragma omp parallel num_threads(processor_count)
	{
		const int c = omp_get_thread_num();

		float bestFit= -std::numeric_limits<float>::infinity();
		float worstFit= std::numeric_limits<float>::infinity();

		for (uint32_t l = c * perThread; l < (c + 1) * perThread && l < mNumLocations; ++l) {
			for (uint32_t t = 0; t < mNumTypes; ++t) {
				Candidate currentCandidate = tryAddGreedy(l, t, assignments[c]);
				candidates[l * mNumTypes + t] = currentCandidate;
				if (currentCandidate.fit > bestFit) {
					bestFit = currentCandidate.fit;
				}
				if (currentCandidate.fit != -std::numeric_limits<float>::infinity() && currentCandidate.fit < worstFit) {
					worstFit = currentCandidate.fit;
				}
			}
		}
		bestFits[c] = bestFit;
		worstFits[c] = worstFit;
	}
	float bestFit = -std::numeric_limits<float>::infinity();
	float worstFit = std::numeric_limits<float>::infinity();
	for (int i = 0; i < processor_count; ++i) {
		if (bestFits[i] > bestFit) {
			bestFit = bestFits[i];
		}
		if (worstFits[i] < worstFit) {
			worstFit = worstFits[i];
		}
	}
	if (bestFit == -std::numeric_limits<float>::infinity()) {
		Candidate infeasible;
		infeasible.fit = bestFit;
		return infeasible;
	}
	float cutoff = bestFit-((bestFit-worstFit) * alpha);
	uint32_t iter = 0;
	for (uint32_t i = 0; i < candidates.size(); ++i) {
		if (candidates[i].fit >= cutoff) {
			RCL[iter] = candidates[i];
			iter++;
		}
	}
	uint32_t randElec = rand() % (iter);
	return std::move(RCL[randElec]);
}

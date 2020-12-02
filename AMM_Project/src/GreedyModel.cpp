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
	while (!isSolution()) {
		Candidate bestAction = findBestAddition();

		/*if (bestAction.first == NOT_ASSIGNED) {
			break;
		}*/
		if (bestAction.fit<0) break;
		applyAction(bestAction);

		float n = numToAssign();
		if (n > actual + 0.01f) {
			actual = n;
			std::cout << actual * 100.0f <<"%" << std::endl;
		}
	}
}

GreedyModel::Candidate GreedyModel::tryAddGreedy(const uint32_t l, const uint32_t t) const
{
	if (mLocationTypeAssignment[l] != NOT_ASSIGNED || locationIsBlocked(l)) {
		Candidate infeasible;
		infeasible.fit=-1;
		return infeasible;
	}

	uint32_t num = 0;
	const float maxPop = mBaseModel.getCenterTypes()[t].maxPop;
	const uint32_t* ptr = getCitiesSorted(l);

	Candidate bestCandidate;
	bool fullMix=false;
	bool fullSecondary=false;
	bool fullPrimary=false;

	std::vector<char> assignmentsMix;
	std::vector<char> assignmentsSecondary;
	std::vector<char> assignmentsPrimary;


	float newPopPrimary=0;
	float newPopMix=0;
	float newPopSecondary=0;

	float popMix=0;
	float popSecondary=0;
	float popPrimary=0;

	for (uint32_t ci = 0; ci < mNumCities; ++ci) {
		uint32_t c = *(ptr + ci);
		if (isCityLocationTypeCompatible(c, l, t, 1)) {
			bool assignedPrimary=false;
			bool assignedSecondary=false;
			float population=mBaseModel.getCities()[c].population;
			if (mCityCenterAssignment[c].first == NOT_ASSIGNED && isCityLocationTypeCompatible(c, l, t, 0)){
				if (!fullPrimary) {
					newPopPrimary+=mBaseModel.getCities()[c].population;
				}
				if (!fullMix)  {
					newPopMix+=mBaseModel.getCities()[c].population;
				}
				assignedPrimary=true;
			}
			if (mCityCenterAssignment[c].second == NOT_ASSIGNED) {
				if (!fullSecondary) {
					newPopSecondary+=mBaseModel.getCities()[c].population*0.1;
				}
				if (!assignedPrimary && !fullMix) {
					newPopMix+=mBaseModel.getCities()[c].population*0.1;
				}
				assignedSecondary=true;
			}
			if (!fullMix && newPopMix>maxPop) fullMix=true;
			if (!fullPrimary && newPopPrimary>maxPop) fullPrimary=true;
			if (!fullSecondary && newPopSecondary>maxPop) fullSecondary=true;

			if (fullMix && fullPrimary && fullSecondary) break;

			popMix=newPopMix;
			popSecondary=newPopSecondary;
			popPrimary=newPopPrimary;
			if (!fullMix) {
				if (assignedPrimary) assignmentsMix.push_back(1);
				else if (assignedSecondary) assignmentsMix.push_back(2);
				else assignmentsMix.push_back(0);
			}
			if (!fullPrimary) {
				if (assignedPrimary) assignmentsPrimary.push_back(1);
				else assignmentsPrimary.push_back(0);
			}
			if (!fullSecondary) {
				if (assignedSecondary) assignmentsSecondary.push_back(2);
				else assignmentsSecondary.push_back(0);
			}
		}
		else {
			assignmentsMix.push_back(0);
			assignmentsSecondary.push_back(0);
			assignmentsPrimary.push_back(0);
		}

	}
	bestCandidate.type=t;
	bestCandidate.loc=l;
	if (popMix>=popSecondary && popMix>=popPrimary) {
		bestCandidate.fit=popMix/mBaseModel.getCenterTypes()[t].cost;
		bestCandidate.assigns=assignmentsMix;
	}
	else if (popPrimary>=popSecondary && popPrimary>=popMix) {
		bestCandidate.fit=popPrimary/mBaseModel.getCenterTypes()[t].cost;
		bestCandidate.assigns=assignmentsPrimary;
	}
	else {
		bestCandidate.fit=popSecondary/mBaseModel.getCenterTypes()[t].cost;
		bestCandidate.assigns=assignmentsSecondary;
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
    float actualFitness = -1.0;
    res.fit=-1.0;
	    
	const int processor_count = std::thread::hardware_concurrency();
	//omp_set_num_threads(4);

    int perThread=mNumLocations/processor_count;
    if (perThread<1) perThread=1;
    #pragma omp parallel shared(res) num_threads(processor_count)
    {
        const int c = omp_get_thread_num(); 
        Candidate bestCandidate;
        bestCandidate.fit=-1.0;
        for (int l = c*perThread ; l < (c+1)*perThread && l < mNumLocations; ++l) {
            for (uint32_t t = 0; t < mNumTypes; ++t) {
                Candidate currentCandidate = tryAddGreedy(l, t);
                if (currentCandidate.fit>bestCandidate.fit) {
                    bestCandidate=currentCandidate;
                }
            }
        }
        if (bestCandidate.fit > res.fit) {
            #pragma omp critical
            {
                if (bestCandidate.fit > res.fit) {
                    res = bestCandidate;
                }
            }
        }
    }

    return res;
}


void GreedyModel::applyAction(Candidate bestActions)
{

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


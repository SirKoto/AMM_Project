#pragma once
#include "IModel.h"

class LocalSearchModel :
    public IModel
{
public:
    LocalSearchModel(const Model& model);
    LocalSearchModel(const IModel* model);


    void localSearchAssignments();

   
protected:

    struct OperationAssignments
    {
        enum class Op
        {
            eSetPrimary,
            eSetSecondary,
            eSwapPP,
            eSwapPS,
            eSwapSS,
            eNope
        };
        Op op = Op::eNope;

        uint32_t city = 0;

        union {
            uint32_t location = 0;
            uint32_t city2;
        };

    };


    std::vector<float> mCityHeuristicBuffer;
    std::vector<float> mLocationAssignedPopulation;

    float mMaxPop;

    float mActualAssignmentHeuristic = 0.0f;

    // Modifies op, so that it is the oposite operation
    void doFixedCentersOp(OperationAssignments& op);

    void updateHeuristicCity(uint32_t city);

    bool isCityCenterAssignmentFeasible() const;

    bool assertCityPopulation();

};


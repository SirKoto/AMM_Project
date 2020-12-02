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


    std::vector<double> mCityHeuristicBuffer;
    std::vector<float> mLocationAssignedPopulation;
    std::vector<double> mLocationHeuristicBuffer;

    float mMaxPop;

    double mActualAssignmentHeuristic = 0.0f;

    // Modifies op, so that it is the oposite operation
    void doFixedCentersOp(OperationAssignments& op);

    void updateHeuristicCity(uint32_t city);
    void updateHeuristicLocation(uint32_t location);

    bool isCityCenterAssignmentFeasible() const;

    bool assertCityPopulation();

};


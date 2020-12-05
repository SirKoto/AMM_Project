#pragma once
#include "IModel.h"

class LocalSearchModel :
    public IModel
{
public:

    static IModel run(const IModel* model);
    static IModel runParallel(const IModel* model);



protected:

    LocalSearchModel(const Model& model);
    LocalSearchModel(const IModel* model);

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

    struct OperationCenters
    {
        enum class Op
        {
            eSet,
            eSwap,
            eNope
        };
        Op op = Op::eNope;

        uint32_t location = 0;

        union
        {
            uint32_t location2 = 0;
            uint32_t type;
        };
    };


    std::vector<float> mCityHeuristicBuffer;
    std::vector<uint32_t> mLocationAssignedPopulation;
    std::vector<float> mLocationHeuristicBuffer;

    float mActualAssignmentHeuristic = 0.0f;

    float mGenericHeuristic = 0.0f;

    void generalUpdate();

    // returns oposite operation
    OperationCenters doLocationsOp(OperationCenters op);

    // Modifies op, so that it is the oposite operation
    void doFixedCentersOp(OperationAssignments& op);

    void updateHeuristicCity(uint32_t city);
    void updateHeuristicLocation(uint32_t location);

    bool isCityCenterAssignmentFeasible() const;

    bool assertCityPopulation() const;

    void resetCenters();

    // returns true if correct assignment reached
    bool localSearchAssignments();
};


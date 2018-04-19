//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

public:
    std::vector<std::vector<uint32_t>>  uniqueIN;
    std::vector<std::vector<uint32_t>>  uniqueOUT;
    std::vector<uint32_t> totalAmount;
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;
    std::vector<int> OptimalJoinOrdering(cardStat left, cardStat right);
    cardStat calculateCardStat(std::string sub_query);
    cardStat estimate(RPQTree *q);

    void prepare() override ;
    std::vector<int> estimatePath(RPQTree *q);
    cardStat estimateBestJoin(cardStat left, cardStat right);

};


#endif //QS_SIMPLEESTIMATOR_H

//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H


#include <memory>
#include <cmath>
#include "SimpleGraph.h"
#include "RPQTree.h"
#include "Evaluator.h"
#include "Graph.h"

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

public:
    std::vector<std::string> queryarray;
    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEvaluator() = default;

    void prepare() override ;
    cardStat evaluate(RPQTree *query) override ;
    std::vector<std::pair<uint32_t,uint32_t>> evaluateFaster(RPQTree *query);
    int minimalLengthQuery(RPQTree *query, int length);
    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<SimpleGraph> evaluate_aux(RPQTree *q);
    static std::shared_ptr<SimpleGraph> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g, std::vector<std::vector<uint32_t>>  uniqueIN, std::vector<std::vector<uint32_t>>  uniqueOUT);
    static std::shared_ptr<SimpleGraph> join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right);

    //static cardStat computeStats(std::shared_ptr<SimpleGraph> &g);
    static cardStat computeStats(std::vector<std::pair<uint32_t,uint32_t>> pairs);

};


#endif //QS_SIMPLEEVALUATOR_H

//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;
    uint32_t sampleSize;
    std::vector<uint32_t> nr_edges_in;
    std::vector<uint32_t> nr_edges_out;
    std::vector<uint32_t> nr_label_occurences;
    std::vector<uint32_t> array;
    std::vector<std::vector<std::tuple<int, int>>> tabels;
    std::vector<std::vector<uint32_t>>  uniqueIN;
    std::vector<std::vector<uint32_t>>  uniqueOUT;
    std::vector<std::vector<uint32_t>>  AttributeCountIN;
    std::vector<std::vector<uint32_t>>  AttributeCountOUT;
    std::vector<uint32_t> thresholdsIN;
    std::vector<uint32_t> thresholdsOUT;
    std::vector<int> joinAttributes;

public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    std::vector<int> estimatePath(RPQTree *q);
    cardStat estimate(RPQTree *q) override ;

};


#endif //QS_SIMPLEESTIMATOR_H

//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
    totalAmount.resize(graph.get()->getNoLabels());
    uniqueIN.resize(graph.get()->getNoLabels());
    uniqueOUT.resize(graph.get()->getNoLabels());
}

void SimpleEstimator::prepare() {

    // do your prep here
    for(int k =0; k< totalAmount.size(); k++){
        totalAmount[k] = 0;
    }

    for(int label = 0; label < graph->getNoLabels(); label++) {
        for(int i = 0; i < graph->edge_pairs[label].size(); i++) {
            totalAmount[label] += 1;
            //if(!(std::find(std::begin(uniqueIN[label]), std::end(uniqueIN[label]), graph->edge_pairs[label][i].first) != std::end(uniqueIN[label]))) {
                uniqueIN[label].push_back(graph->edge_pairs[label][i].first);
            //}
            //if(!(std::find(std::begin(uniqueOUT[label]), std::end(uniqueOUT[label]), graph->edge_pairs[label][i].second) != std::end(uniqueOUT[label]))) {
                uniqueOUT[label].push_back(graph->edge_pairs[label][i].second);
            //}
        }
    }
}
std::vector<int> SimpleEstimator::estimatePath(RPQTree *q) {
    if(q->isLeaf()){
        std::smatch matches;
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");
        if(std::regex_search(q->data, matches, directLabel)) {
            auto label = (uint32_t) std::stoul(matches[1]);
            std::vector<int> test {static_cast<int>(uniqueIN[label].size()), static_cast<int>(uniqueOUT[label].size()), totalAmount[label], label, 1};
            return test;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            auto label = (uint32_t) std::stoul(matches[1]);
            std::vector<int> test {static_cast<int>(uniqueOUT[label].size()), static_cast<int>(uniqueIN[label].size()), totalAmount[label], label, 0};
            return test;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            std::vector<int> test;
            return test;
        }
    }
    if(q->isConcat()) {
        auto left = estimatePath(q->left);
        auto right = estimatePath(q->right);
        std::vector<int> test {left[0], right[1], (int) std::min((left[2] * (((double)right[2])/((double)right[0]))), right[2] * ((double)left[2]/(double)left[1]))};
        return test;
    }
    return {};
}

std::vector<int> SimpleEstimator::OptimalJoinOrdering(cardStat left, cardStat right) {
    std::vector<int> test {left.noOut, right.noIn, (int) std::min((left.noPaths * (((double)right.noPaths)/((double)right.noOut))),
                                                                  right.noPaths * ((double)left.noPaths/(double)left.noIn))};
    return test;
}


cardStat SimpleEstimator::calculateCardStat(std::string sub_query) {
    cardStat sub_query_new;
    std::smatch matches;
    std::regex directLabel (R"((\d+)\+)");
    std::regex inverseLabel (R"((\d+)\-)");
    if(std::regex_search(sub_query, matches, directLabel)) {
        auto label = (uint32_t) std::stoul(matches[1]);
        sub_query_new = {static_cast<int>(uniqueIN[label].size()),  totalAmount[label], static_cast<int>(uniqueOUT[label].size()),};
    } else if(std::regex_search(sub_query, matches, inverseLabel)) {
        auto label = (uint32_t) std::stoul(matches[1]);
        sub_query_new =  {static_cast<int>(uniqueOUT[label].size()), totalAmount[label], static_cast<int>(uniqueIN[label].size())};
    }
    return sub_query_new;
}

cardStat SimpleEstimator::estimateBestJoin(cardStat left, cardStat right) {
    std::vector<int> joinStats = OptimalJoinOrdering(left, right);

    joinStats[0] = std::min(joinStats[0], joinStats[2]);
    joinStats[1] = std::min(joinStats[1], joinStats[2]);
    return cardStat {joinStats[0],joinStats[2],joinStats[1]};
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    std::vector<int> testvar = estimatePath(q);
    testvar[0] = std::min(testvar[0], testvar[2]);
    testvar[1] = std::min(testvar[1], testvar[2]);
    return cardStat {testvar[0],testvar[2],testvar[1]};
}
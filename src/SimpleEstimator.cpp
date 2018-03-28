//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g){

    // works only with SimpleGraph
    graph = g;
    nr_label_occurences.resize(graph.get()->getNoLabels());
    array.resize(graph.get()->getNoLabels());
    tabels.resize(graph.get()->getNoLabels());
    uniqueIN.resize(graph.get()->getNoLabels());
    uniqueOUT.resize(graph.get()->getNoLabels());
    AttributeCountIN.resize(graph.get()->getNoLabels());
    AttributeCountOUT.resize(graph.get()->getNoLabels());
    thresholdsIN.resize(graph.get()->getNoLabels());
    thresholdsOUT.resize(graph.get()->getNoLabels());
    sampleSize = ((graph.get()->getNoEdges() * 2) / graph->getNoLabels()) / 5;
}

void SimpleEstimator::prepare() {

    // do your prep here
    for(int k =0; k< array.size(); k++){
        array[k] = 0;
    }

    for(uint32_t source = 0; source < graph->getNoVertices(); source++) {
        for (auto labelTarget : graph->adj[source]) {
            auto label = labelTarget.first;
            auto target = labelTarget.second;
            tabels[label].emplace_back(std::make_tuple(source, target));
            array[label] += 1;
            AttributeCount[label].push_back({});
            AttributeCount[label].push_back();

            if(!(std::find(std::begin(uniqueIN[label]), std::end(uniqueIN[label]), source) != std::end(uniqueIN[label]))) {
                uniqueIN[label].push_back(source);
            }
            if(!(std::find(std::begin(uniqueOUT[label]), std::end(uniqueOUT[label]), target) != std::end(uniqueOUT[label]))) {
                uniqueOUT[label].push_back(target);
            }
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
            std::vector<int> test {static_cast<int>(uniqueIN[label].size()), static_cast<int>(uniqueOUT[label].size()), array[label], label, 1};
            return test;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            auto label = (uint32_t) std::stoul(matches[1]);
            std::vector<int> test {static_cast<int>(uniqueOUT[label].size()), static_cast<int>(uniqueIN[label].size()), array[label], label, 0};
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

cardStat SimpleEstimator::estimate(RPQTree *q) {
    std::vector<int> testvar = estimatePath(q);
    testvar[0] = std::min(testvar[0], testvar[2]);
    testvar[1] = std::min(testvar[1], testvar[2]);
    return cardStat {testvar[0],testvar[2],testvar[1]};
}
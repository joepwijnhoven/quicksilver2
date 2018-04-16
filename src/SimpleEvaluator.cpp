//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();


    // prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::vector<std::pair<uint32_t,uint32_t>> pairs) {
    cardStat stats {};
    std::vector<uint32_t> uniquein;
    std::vector<uint32_t> uniqueout;
    for(int i =0; i < pairs.size(); i++) {
        uniquein.push_back(pairs[i].first);
        uniqueout.push_back(pairs[i].second);
    }

    sort(uniquein.begin(), uniquein.end());
    uniquein.erase(unique(uniquein.begin(), uniquein.end()), uniquein.end());
    sort(uniqueout.begin(), uniqueout.end());
    uniqueout.erase(unique(uniqueout.begin(), uniqueout.end()), uniqueout.end());

    stats.noPaths = pairs.size();
    stats.noIn = uniqueout.size();
    stats.noOut = uniquein.size();
    return stats;
}

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::evaluateFaster(RPQTree *query) {
    if(query->isLeaf()) {
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;
        uint32_t label;

        if(std::regex_search(query->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            return graph->edge_pairs[label];
        } else if(std::regex_search(query->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            return graph->edge_pairs_reverse[label];
        } else {
            return {};
        }
    } else {
        auto left = evaluateFaster(query->left);
        auto right = evaluateFaster(query->right);
        std::vector<std::pair<uint32_t,uint32_t>> join;

        std::vector<std::string> array;

        int left_key = 0;
        int right_key = 0;

        while(left_key != left.size() && right_key != right.size()) {
            if (left[left_key].second == right[right_key].first) {
                if (!(std::find(std::begin(array), std::end(array),
                                std::to_string(left[left_key].first) + "-" + std::to_string(right[left_key].second)) !=
                      std::end(array))) {
                    array.emplace_back(std::to_string(left[left_key].first) + "-" + std::to_string(right[left_key].second));
                    join.emplace_back(std::make_pair(left[left_key].first, right[right_key].second));
                }
                left_key++;
                right_key++;
            } else if(left[left_key].second < right[right_key].first) {
                left_key++;
            }  else {
                right_key++;
            }
        }
        return join;
    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    auto joins = evaluateFaster(query);
    return SimpleEvaluator::computeStats(joins);
}
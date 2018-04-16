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

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::evaluateFaster(std::vector<std::string> query) {
    if(query.size() == 1) {
        return edges(query[0], true);
    }
    std::string q = query[0];
    auto left = edges(query[0], false);
    for(int i = 1; i < query.size(); i++) {
//        if(GetFromCache(q + query[i]).size() != 0) {
//            left = GetFromCache(q + query[i]);
//        } else {
            left = join(left, edges(query[i], true));
            std::sort(left.begin(),left.end());
//            InsertIntoCache(q + query[i], left);
//        }

    }
    return left;
}

void SimpleEvaluator::InsertIntoCache(std::string query, std::vector<std::pair<uint32_t,uint32_t>> pairs) {
    querycache.push_back(std::make_pair(query, pairs));
}

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::GetFromCache(std::string query){
    for(int i = 0; i < querycache.size(); i++) {
        if(querycache[i].first == query){
            return querycache[i].second;
        }
    }
    return std::vector<std::pair<uint32_t,uint32_t>>();
}

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::edges(std::string sub_query, bool right) {
    std::regex directLabel(R"((\d+)\+)");
    std::regex inverseLabel(R"((\d+)\-)");
    std::smatch matches;

    if (std::regex_search(sub_query, matches, directLabel)) {
        auto label = (uint32_t) std::stoul(matches[1]);
        if (right) {
            return graph->edge_pairs[label];
        } else {
            return graph->edge_pairs_reverse[label];
        }
    } else if (std::regex_search(sub_query, matches, inverseLabel)) {
        auto label = (uint32_t) std::stoul(matches[1]);
        if (right) {
            return graph->edge_pairs_reverse[label];
        } else {
            return graph->edge_pairs[label];
        }
    }
}

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::join(std::vector<std::pair<uint32_t,uint32_t>> left, std::vector<std::pair<uint32_t,uint32_t>> right) {
    std::vector<std::pair<uint32_t,uint32_t>> join;

    std::vector<std::string> array;

    int left_key = 0;
    int right_key = 0;
    int right_key_next;
    int left_key_next;

    while(left_key != left.size() && right_key != right.size()){
        if(left[left_key].first == right[right_key].first) {
            right_key_next = right_key;
            while(right_key_next != right.size() && (left[left_key].first == right[right_key_next].first)) {
                if (!(std::find(std::begin(array), std::end(array), std::to_string(left[left_key].second) + "-" + std::to_string(right[right_key_next].second)) != std::end(array))) {
                    array.emplace_back(std::to_string(left[left_key].second) + "-" + std::to_string(right[right_key_next].second));
                    join.emplace_back(std::make_pair(left[left_key].second, right[right_key_next].second));
                }
                right_key_next++;
            }
            left_key++;
        } else if(left[left_key].first < right[right_key].first) {
            left_key++;
        } else {
            right_key++;
        }
    }
    return join;
}

std::vector<std::string> SimpleEvaluator::TreeToString(RPQTree *query) {
    if(query->isLeaf()) {
        std::vector<std::string> array;
        array.push_back(query->data);
        return array;
    } else {
        auto left = TreeToString(query->left);
        auto right = TreeToString(query->right);
        std::vector<std::string> results;
        results.reserve(left.size() + right.size());
        results.insert(results.end(), left.begin(), left.end());
        results.insert(results.end(), right.begin(), right.end());
        return results;
    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    auto q = TreeToString(query);
    auto joins = evaluateFaster(q);
    return SimpleEvaluator::computeStats(joins);
}
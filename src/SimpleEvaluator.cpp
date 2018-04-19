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



std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::evaluateFaster(std::vector<std::string> query, std::vector<std::pair<int, int>> bestjoinorder) {
    if(query.size() == 1) {
        return edges(query[0], true);
    }

    std::vector<std::pair<uint32_t,uint32_t>> prev;
    std::vector<std::pair<uint32_t,uint32_t>> left;
    std::vector<std::pair<uint32_t,uint32_t>> right;

    for(int i =0; i < bestjoinorder.size(); i++) {
        auto j = bestjoinorder[i];

        if(j.first == -1) {
            left = prev;
            right = edges(query[j.second], true);
        }
        else
        {
            left = edges(query[j.first], false);
            if(j.second == -1) {
                right = prev;
            }
            else {
                right = edges(query[j.second], true);
            }
        }

        if(i < bestjoinorder.size() - 1) {
            auto nextjoin = bestjoinorder[i+1];
            if(nextjoin.first == -1) {
                prev = join(left, right, false);
            } else {
                prev = join(left, right, true);
            }
        } else {
            prev = join(left, right, true);
        }
        std::sort(prev.begin(),prev.end());
    }

//    std::string q = query[0];
//    if(GetFromCache(q + query[i]).size() != 0) {
//        left = GetFromCache(q + query[i]);
//    } else {
//
//        std::sort(left.begin(), left.end(), [](auto &l, auto &r) {
//            return l.second < r.second;
//        });
//        InsertIntoCache(q + query[i], left);
//    }
//    q = q + query[i]
    return prev;
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

std::vector<std::pair<int, int>> SimpleEvaluator::OptimalJoinOrdering(std::vector<std::string> query) {
    std::map<std::string, cardStat> sub_solutions;
    std::string left_subplan;
    std::string right_subplan;
    std::string total_query;
    cardStat total;
    cardStat left;
    cardStat right;

    for(int i = 2; i <= query.size(); i++) { // size of plan
        for(int j = 0; j <= query.size() - i; j++) { //position of
            cardStat best_sub_solution = {1, std::numeric_limits<int>::max(), 1};
            for(int k = j; k < j + i - 1; k++) {
                left_subplan.clear();
                right_subplan.clear();
                for(int m = j; m <= k; m++) {
                    left_subplan += query[m];
                }
                for(int m = k + 1; m < j + i; m++) {
                    right_subplan += query[m];
                }
                total_query = left_subplan + right_subplan;
                if(left_subplan.size() == 2 && right_subplan.size() == 2) {
                    left = est->calculateCardStat(left_subplan);
                    right = est->calculateCardStat(right_subplan);
                    total = est->estimateBestJoin(left, right);
                } else if(left_subplan.size() == 2) {
                    left = est->calculateCardStat(left_subplan);
                    total = est->estimateBestJoin(left, sub_solutions[right_subplan]);
                } else if(right_subplan.size() == 2) {
                    right = est->calculateCardStat(right_subplan);
                    total = est->estimateBestJoin(sub_solutions[left_subplan], right);
                } else if (left_subplan.size() != 2 && right_subplan.size() != 2) {
                    total = est->estimateBestJoin(sub_solutions[left_subplan], sub_solutions[right_subplan]);
                }
                if (total.noPaths < best_sub_solution.noPaths) {
                    best_sub_solution = total;
                }
            }
            sub_solutions[total_query] = best_sub_solution;
        }
    }
    std::vector<std::pair<int, int>> optimal_join_ordering;
    return optimal_join_ordering;



}

/**
 * Function for returning edge_pairs of a label
 * @param sub_query, label string
 * @param right, if it must be joined with a right label we return the reverse edge list.
 * @return
 */
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

/**
 * Function for joining two labels
 * @param left, table on the left side
 * @param right, table on the right side
 * @param isRight, If result is a left table, we return the reverse edge list
 * @return
 */
std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::join(std::vector<std::pair<uint32_t,uint32_t>> left, std::vector<std::pair<uint32_t,uint32_t>> right, bool isRight) {
    std::vector<std::pair<uint32_t,uint32_t>> join;

    std::vector<std::string> array;

    int left_key = 0;
    int right_key = 0;
    int right_key_next;

    while(left_key != left.size() && right_key != right.size()){
        if(left[left_key].first == right[right_key].first) {
            right_key_next = right_key;
            while(right_key_next != right.size() && (left[left_key].first == right[right_key_next].first)) {
                if (!(std::find(std::begin(array), std::end(array), std::to_string(left[left_key].second) + "-" + std::to_string(right[right_key_next].second)) != std::end(array))) {
                    array.emplace_back(std::to_string(left[left_key].second) + "-" + std::to_string(right[right_key_next].second));
                    if(isRight) {
                        join.emplace_back(std::make_pair(left[left_key].second, right[right_key_next].second));
                    } else {
                        join.emplace_back(std::make_pair(right[right_key_next].second, left[left_key].second));
                    }
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
    std::vector<std::pair<int, int>> bestjoinorder = std::vector<std::pair<int, int>>();
    bestjoinorder.push_back(std::make_pair(0,1));
    bestjoinorder.push_back(std::make_pair(-1,2));
    auto joins = evaluateFaster(q, bestjoinorder);
    //auto sub_solutions = OptimalJoinOrdering(q);
    auto joins = evaluateFaster(q);
    return SimpleEvaluator::computeStats(joins);
}
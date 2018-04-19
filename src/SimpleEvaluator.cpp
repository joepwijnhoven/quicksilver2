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

    std::sort(uniquein.begin(), uniquein.end());
    uniquein.erase(unique(uniquein.begin(), uniquein.end()), uniquein.end());
    std::sort(uniqueout.begin(), uniqueout.end());
    uniqueout.erase(unique(uniqueout.begin(), uniqueout.end()), uniqueout.end());

    stats.noPaths = pairs.size();
    stats.noIn = uniqueout.size();
    stats.noOut = uniquein.size();
    return stats;
}

bool SimpleEvaluator::singleQuery(std::string sub_query) {
    int count = 0;
    for(int i = 0; i < sub_query.size(); i++) {
        if(sub_query[i] == '+' || sub_query[i] == '-') {
            count++;
        }
    }
    if(count == 1) {
        return true;
    } else {
        return false;
    }
}

std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::evaluateFaster(std::vector<std::string> query, std::vector<std::pair<std::string, std::string>> bestjoinorder) {
    if(query.size() == 1) {
        return edges(query[0], true);
    }

    std::map<std::string, std::vector<std::vector<std::pair<uint32_t,uint32_t>>>> prev_solutions;
    std::vector<std::pair<uint32_t,uint32_t>> final_join;
    std::vector<std::pair<uint32_t,uint32_t>> left;
    std::vector<std::pair<uint32_t,uint32_t>> right;

    for(int i =0; i < bestjoinorder.size(); i++) {
        auto j = bestjoinorder[i];
        // create a join from base labels if needed else retrieve it from previous solutions.
        if(singleQuery(j.first)) {
            left = edges(j.first, false);
        } else {
            left = prev_solutions[j.first][0];
            // left solution needs to be reversed and sorted
            for(int i = 0; i < left.size(); i++) {
                uint32_t from = left[i].first;
                left[i].first = left[i].second;
                left[i].second = from;
            }
            std::sort(left.begin(),left.end());
        }
        if(singleQuery(j.second)) {
            right = edges(j.second, true);
        } else {
            right = prev_solutions[j.second][0];
            // solution needs to be sorted
            std::sort(right.begin(),right.end());
        }
        // if we arent yet at the final join
        if(i < bestjoinorder.size() - 1) {
            std::string total_query = j.first+j.second;
            prev_solutions[total_query].emplace_back(join(left, right));
        } else {
            final_join = join(left, right);
        }
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
    return final_join;
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

void SimpleEvaluator::OptimalJoinOrdering(std::vector<std::string> query) {
    std::map<std::string, cardStat> sub_solutions;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> final_solution;
    std::string left_subplan;
    std::string right_subplan;
    std::string best_left_subplan;
    std::string best_right_subplan;
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
                if(singleQuery(left_subplan) && singleQuery(right_subplan)) {
                    left = est->calculateCardStat(left_subplan);
                    right = est->calculateCardStat(right_subplan);
                    total = est->estimateBestJoin(left, right);
                } else if(singleQuery(left_subplan)) {
                    left = est->calculateCardStat(left_subplan);
                    total = est->estimateBestJoin(left, sub_solutions[right_subplan]);
                } else if(singleQuery(right_subplan)) {
                    right = est->calculateCardStat(right_subplan);
                    total = est->estimateBestJoin(sub_solutions[left_subplan], right);
                } else {
                    total = est->estimateBestJoin(sub_solutions[left_subplan], sub_solutions[right_subplan]);
                }
                if (total.noPaths < best_sub_solution.noPaths) {
                    best_sub_solution = total;
                    best_left_subplan = left_subplan;
                    best_right_subplan = right_subplan;
                }
            }
            sub_solutions[total_query] = best_sub_solution;
            final_solution[total_query].push_back(std::make_pair(best_left_subplan, best_right_subplan));
        }
    }
    create_join_ordering(total_query, final_solution);
}
/**
 * recursively step through our solution matrix from the algorithm above to create a vector of pairs that
 * can be used by our join algorithm to create a solution
 * @param sub_query sub query needing to be split up
 * @param final_solution matrix that holds all the solutions
 * @param optimal_join_ordering the resulting vector of pairs that can be used by our join
 */
void SimpleEvaluator::create_join_ordering(std::string sub_query, std::map<std::string, std::vector<std::pair<std::string, std::string>>> final_solution) {
    std::string left_plan = final_solution[sub_query][0].first;
    std::string right_plan = final_solution[sub_query][0].second;
    if(!singleQuery(left_plan)) {
        create_join_ordering(left_plan, final_solution);
    }
    if(!singleQuery(right_plan)) {
        create_join_ordering(right_plan, final_solution);
    }
    optimal_join_ordering.emplace_back(std::make_pair(left_plan, right_plan));
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
 * @return
 */
std::vector<std::pair<uint32_t,uint32_t>> SimpleEvaluator::join(std::vector<std::pair<uint32_t,uint32_t>> left, std::vector<std::pair<uint32_t,uint32_t>> right) {
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
    optimal_join_ordering.clear();
    OptimalJoinOrdering(q);
//    bestjoinorder.push_back(std::make_pair(0,1));
//    bestjoinorder.push_back(std::make_pair(-1,2));
    auto joins = evaluateFaster(q, optimal_join_ordering);
    return SimpleEvaluator::computeStats(joins);
}
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

cardStat SimpleEvaluator::computeStats(std::shared_ptr<SimpleGraph> &g) {

    cardStat stats {};

    for(int source = 0; source < g->getNoVertices(); source++) {
        if(!g->adj[source].empty()) stats.noOut++;
    }

    stats.noPaths = g->getNoDistinctEdges();

    for(int target = 0; target < g->getNoVertices(); target++) {
        if(!g->reverse_adj[target].empty()) stats.noIn++;
    }

    return stats;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in, std::vector<std::vector<uint32_t>>  uniqueIN, std::vector<std::vector<uint32_t>>  uniqueOUT) {

    auto out = std::make_shared<SimpleGraph>(in->getNoVertices());
    out->setNoLabels(in->getNoLabels());

    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < uniqueIN[projectLabel].size(); source++) {
            for (auto labelTarget : in->adj[uniqueIN[projectLabel][source]]) {
                if (projectLabel < labelTarget.first)
                    break;
                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(uniqueIN[projectLabel][source], target, label);
            }
        }
    } else {
        // going backward
        for(uint32_t source = 0; source < uniqueOUT[projectLabel].size(); source++) {
            for (auto labelTarget : in->reverse_adj[uniqueOUT[projectLabel][source]]) {
                if (projectLabel < labelTarget.first)
                    break;
                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(uniqueOUT[projectLabel][source], target, label);
            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::join(std::shared_ptr<SimpleGraph> &left, std::shared_ptr<SimpleGraph> &right) {

    auto out = std::make_shared<SimpleGraph>(left->getNoVertices());
    out->setNoLabels(1);

    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) {
        for (auto labelTarget : left->adj[leftSource]) {

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }

    return out;
}

std::shared_ptr<SimpleGraph> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEvaluator::project(label, inverse, graph, est->uniqueIN, est->uniqueOUT);
    }

    if(q->isConcat()) {

        // evaluate the children
        auto leftGraph = SimpleEvaluator::evaluate_aux(q->left);
        auto rightGraph = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftGraph, rightGraph);

    }

    return nullptr;
}

int SimpleEvaluator::minimalLengthQuery(RPQTree *query, int length) {
    if(query->isLeaf()) {
        queryarray.push_back(query->data);
        return length;
    } else {
        if(length <= 3) {
            auto left = minimalLengthQuery(query->left, length++);
            auto right = minimalLengthQuery(query->left, length++);
            return left + right;
        }
        return 100;
    }
}

cardStat SimpleEvaluator::evaluateFaster(std::vector<std::string> queryarray){
    std::regex directLabel (R"((\d+)\+)");
    std::regex inverseLabel (R"((\d+)\-)");

    std::smatch matches;
    uint32_t label;
    bool inverse;

    if(std::regex_search(queryarray[0], matches, directLabel)) {
        label = (uint32_t) std::stoul(matches[1]);
        inverse = false;
    } else if(std::regex_search(queryarray[0], matches, inverseLabel)) {
        label = (uint32_t) std::stoul(matches[1]);
        inverse = true;
    }
//
//    auto paths = est->array.size();
//    auto ingoing = est->uniqueIN
//    for(int i=1; i < queryarray.size(); i++) {
//        label queryarray[i]
//    }
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
//    if(minimalLengthQuery(query, 1) <= 3) {
//        evaluateFaster(queryarray);
//        //return SimpleEvaluator::computeStats(queryarray);
//    } else {
        auto res = evaluate_aux(query);
        return SimpleEvaluator::computeStats(res);
//    }

}
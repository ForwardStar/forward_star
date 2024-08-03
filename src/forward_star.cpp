#include "forward_star.h"

bool ChainedForwardStar::InsertEdge(uint64_t src, uint64_t des, double weight) {
    if (vertex_index.find(src) == vertex_index.end()) {
        auto tmp = new DummyNode{src, false, nullptr};
        vertex_index[src] = tmp;
        dummy_nodes.push_back(tmp);
    }
    if (vertex_index.find(des) == vertex_index.end()) {
        auto tmp = new DummyNode{des, false, nullptr};
        vertex_index[des] = tmp;
        dummy_nodes.push_back(tmp);
    }

    auto& tmp = vertex_index[src];
    tmp->next = new WeightedEdge{weight, vertex_index[des], tmp->next};
}

void ChainedForwardStar::BFS(uint64_t src) {
    for (auto e : dummy_nodes) {
        e->obsolete = false;
    }
    std::queue<DummyNode*> Q;
    auto& tmp = vertex_index[src];
    tmp->obsolete = true;
    Q.push(tmp);
    while (!Q.empty()) {
        auto u = Q.front();
        Q.pop();
        auto e = u->next;
        while (e) {
            if (!e->forward->obsolete) {
                e->forward->obsolete = true;
                Q.push(e->forward);
            }
            e = e->next;
        }
    }
}

ChainedForwardStar::~ChainedForwardStar() {
    for (auto u : vertex_index) {
        auto e = u.second->next;
        while (e) {
            auto e_next = e->next;
            delete e;
            e = e_next;
        }
    }
    for (auto u : dummy_nodes) {
        delete u;
    }
}

bool ArrayForwardStar::InsertEdge(uint64_t src, uint64_t des, double weight) {
    if (vertex_index.find(src) == vertex_index.end()) {
        auto tmp = new DummyNode{src, false, std::vector<WeightedEdge>()};
        vertex_index[src] = tmp;
        dummy_nodes.push_back(tmp);
    }
    if (vertex_index.find(des) == vertex_index.end()) {
        auto tmp = new DummyNode{des, false, std::vector<WeightedEdge>()};
        vertex_index[des] = tmp;
        dummy_nodes.push_back(tmp);
    }

    vertex_index[src]->next.push_back(WeightedEdge{weight, vertex_index[des]});
}

void ArrayForwardStar::BFS(uint64_t src) {
    for (auto e : dummy_nodes) {
        e->obsolete = false;
    }
    std::queue<DummyNode*> Q;
    auto& tmp = vertex_index[src];
    tmp->obsolete = true;
    Q.push(tmp);
    while (!Q.empty()) {
        auto u = Q.front();
        Q.pop();
        for (auto e : u->next) {
            if (!e.forward->obsolete) {
                e.forward->obsolete = true;
                Q.push(e.forward);
            }
        }
    }
}

ArrayForwardStar::~ArrayForwardStar() {
    for (auto u : dummy_nodes) {
        delete u;
    }
}
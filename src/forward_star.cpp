/*
 * Copyright (C) 2025 Haoxuan Xie
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "forward_star.h"

bool ForwardStar::Insert(DummyNode* src, DummyNode* des, double weight) {
    src->next.emplace_back(weight, des->idx);
    return true;
}

bool ForwardStar::InsertEdge(NodeID src, NodeID des, double weight) {
    DummyNode* src_ptr = vertex_index->RetrieveVertex(src, true);
    DummyNode* des_ptr = vertex_index->RetrieveVertex(des, true);
    src_ptr->deg.fetch_add(1);
    if (enable_query) degree[src_ptr->idx].fetch_add(1);
    Insert(src_ptr, des_ptr, weight);
    return true;
}

bool ForwardStar::UpdateEdge(NodeID src, NodeID des, double weight) {
    DummyNode* src_ptr = vertex_index->RetrieveVertex(src);
    if (!src_ptr) {
        return false;
    }
    DummyNode* des_ptr = vertex_index->RetrieveVertex(des);
    if (!des_ptr) {
        return false;
    }
    Insert(src_ptr, des_ptr, weight);
    return true;
}

bool ForwardStar::DeleteEdge(NodeID src, NodeID des) {
    DummyNode* src_ptr = vertex_index->RetrieveVertex(src);
    if (!src_ptr) {
        return false;
    }
    DummyNode* des_ptr = vertex_index->RetrieveVertex(des);
    if (!des_ptr) {
        return false;
    }
    src_ptr->deg.fetch_sub(1);
    if (enable_query) degree[src_ptr->idx].fetch_sub(1);
    Insert(src_ptr, des_ptr, 0);
    return true;
}

bool ForwardStar::GetNeighbours(NodeID src, std::vector<WeightedEdge> &neighbours, int timestamp) {
    DummyNode* src_ptr = vertex_index->RetrieveVertex(src);
    if (!src_ptr) {
        return false;
    }
    return GetNeighbours(src_ptr, neighbours, timestamp);
}

bool ForwardStar::GetNeighbours(DummyNode* src, std::vector<WeightedEdge> &neighbours, int timestamp) {
    if (src) {
        int num = 0, k = 0;
        int thread_id = omp_get_thread_num(), cnt = timestamp == -1 ? src->next.size() : timestamp, deg = src->deg;
        neighbours.resize(deg);
        for (int i = cnt - 1; i >= 0; i--) {
            auto e = src->next[i];
            if (!bitmap[thread_id]->get_bit(e.idx)) {
                if (e.weight != 0) { // Insert or Update
                    // Have not found a previous log for this edge, thus this edge is the latest
                    neighbours[num++] = e;
                }
                bitmap[thread_id]->set_bit(e.idx);
            }
            if (deg - num == i) {
                // Edge num = log num, all previous logs are materialized
                for (int j = i - 1; j >= 0; j--) {
                    neighbours[num++] = src->next[j];
                }
                k = i;
                break;
            }
        }
        for (int i = k; i < cnt; i++) {
            bitmap[thread_id]->clear_bit(src->next[i].idx);
        }
    }
    else {
        return false;
    }

    return true;
}

std::vector<DummyNode*> ForwardStar::BFS(NodeID src) {
    std::queue<DummyNode*> Q;
    AtomicBitmap vis(vertex_index->cnt);
    vis.reset();
    auto src_ptr = vertex_index->RetrieveVertex(src);
    vis.set_bit(src_ptr->idx);
    Q.push(src_ptr);
    std::vector<DummyNode*> res;
    while (!Q.empty()) {
        auto u = Q.front();
        Q.pop();
        res.push_back(u);
        std::vector<WeightedEdge> neighbours;
        GetNeighbours(u, neighbours);
        for (auto e : neighbours) {
            if (!vis.get_bit(e.idx)) {
                vis.set_bit(e.idx);
                Q.push(vertex_index->dummy_nodes[e.idx]);
            }
        }
    }
    return res;
}

std::vector<double> ForwardStar::SSSP(NodeID src) {
    std::vector<double> dist;
    dist.assign(vertex_index->cnt, 1e9);
    auto u = vertex_index->RetrieveVertex(src);
    std::priority_queue<std::pair<double, DummyNode*>> Q;
    dist[u->idx] = 0;
    Q.emplace(0, u);
    while (!Q.empty()) {
        auto v = Q.top().second;
        Q.pop();
        std::vector<WeightedEdge> neighbours;
        GetNeighbours(v, neighbours);
        for (auto e : neighbours) {
            auto w = e.idx;
            if (dist[v->idx] + e.weight < dist[w]) {
                dist[w] = dist[v->idx] + e.weight;
                Q.emplace(-dist[w], vertex_index->dummy_nodes[w]);
            }
        }
    }
    return dist;
}

ForwardStar::ForwardStar(int d, std::vector<int> _num_children, bool _enable_query) {
    enable_query = _enable_query;
    if (enable_query) {
        degree = (std::atomic<int>*)calloc(CAP_DUMMY_NODES, sizeof(int));
        bitmap = new AtomicBitmap*[max_number_of_threads];
        for (int i = 0; i < max_number_of_threads; i++) bitmap[i] = new AtomicBitmap(CAP_DUMMY_NODES), bitmap[i]->reset();
    }
    vertex_index = new Trie(d, _num_children);
}

ForwardStar::~ForwardStar() {
    if (bitmap) {
        for (int i = 0; i < max_number_of_threads; i++) delete bitmap[i];
        delete [] bitmap;
    }
    if (degree) free(degree);
    delete vertex_index;
}
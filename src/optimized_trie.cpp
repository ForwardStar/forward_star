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
 #include "optimized_trie.h"

 DummyNode* SORT::InsertVertex(SORTNode* current, NodeID id, int d) {
     for (int i = d; i < depth; i++) {
         int num_now = sum_bits[depth - 1] - (i > 0 ? sum_bits[i - 1] : 0);
         uint64_t idx = ((id & ((1ull << num_now) - 1)) >> (sum_bits[depth - 1] - sum_bits[i]));
         if (i < depth - 1) {
             if (!current->children[idx]) {
                 current->mtx->set_bit_atomic(idx);
                 if (!current->children[idx]) {
                     auto tmp = new SORTNode();
                     int sz = (1 << num_bits[i + 1]);
                     tmp->mtx = new AtomicBitmap(sz);
                     tmp->mtx->reset();
                     tmp->children = new uint64_t[sz];
                     std::memset(tmp->children, 0, sizeof(tmp->children) * sz);
                     current->children[idx] = (uint64_t)tmp;
                 }
                 current->mtx->clear_bit(idx);
             }
         }
         else {
             auto tmp = (DummyNode*)current->children[idx];
             if (!tmp || tmp->idx == -1) {
                 current->mtx->set_bit_atomic(idx);
                 tmp = (DummyNode*)current->children[idx];
                 if (!tmp) {
                     int i = cnt.fetch_add(1);
                     tmp = &(*vertex_table.grow_by(1));
                     tmp->idx = i;
                     current->children[idx] = (uint64_t)tmp;
                 }
                 tmp->node = id;
                 current->mtx->clear_bit(idx);
             }
             return tmp;
         }
         current = (SORTNode*)current->children[idx];
     }
     return nullptr;
 }
 
 DummyNode* SORT::RetrieveVertex(NodeID id, bool insert_mode) {
     SORTNode* current = root;
     for (int i = 0; i < depth; i++) {
         int num_now = sum_bits[depth - 1] - (i > 0 ? sum_bits[i - 1] : 0);
         uint64_t idx = ((id & ((1ull << num_now) - 1)) >> (sum_bits[depth - 1] - sum_bits[i]));
         if (i < depth - 1) {
             if (!current->children[idx]) {
                 if (insert_mode) {
                     return InsertVertex(current, id, i);
                 }
                 else {
                     return nullptr;
                 }
             }
         }
         else {
             auto tmp = (DummyNode*)current->children[idx];
             if (insert_mode && (!tmp || tmp->node == -1)) {
                 return InsertVertex(current, id, i);
             }
             if (!tmp || tmp->node == -1) {
                 return nullptr;
             }
             return tmp;
         }
         current = (SORTNode*)current->children[idx];
     }
     return nullptr;
 }
 
 bool SORT::DeleteVertex(NodeID id) {
     DummyNode* tmp = RetrieveVertex(id);
     if (tmp == nullptr) {
         return false;
     }
     tmp->idx = -1;
     return true;
 }
 
 long long SORT::size() {
     long long sz = 0;
     std::queue<std::pair<SORTNode*, int>> Q;
     Q.emplace(root, 0);
     while (!Q.empty()) {
         SORTNode* u = Q.front().first;
         int d = Q.front().second;
         Q.pop();
         if (d < depth) {
             sz += (1 << num_bits[d]);
             if (d < depth - 1) {
                 for (int i = 0; i < (1 << num_bits[d]); i++) {
                     if (u->children[i]) {
                         Q.emplace((SORTNode*)u->children[i], d + 1);
                     }
                 }
             }
         }
     }
     return sz;
 }
 
 SORT::SORT(int d, int _num_bits[]) {
     depth = d;
     num_bits.resize(d), sum_bits.resize(d);
     for (int i = 0; i < d; i++) {
         num_bits[i] = _num_bits[i];
         sum_bits[i] = (i > 0 ? sum_bits[i - 1] : 0) + num_bits[i];
     }
     root = new SORTNode();
     int sz = (1 << num_bits[0]);
     root->children = new uint64_t[sz];
     std::memset(root->children, 0, sizeof(root->children));
     root->mtx = new AtomicBitmap(sz);
 }
 
 SORT::SORT(int d, std::vector<int> _num_bits) {
     depth = d;
     num_bits.resize(d), sum_bits.resize(d);
     for (int i = 0; i < d; i++) {
         num_bits[i] = _num_bits[i];
         sum_bits[i] = (i > 0 ? sum_bits[i - 1] : 0) + num_bits[i];
     }
     root = new SORTNode();
     int sz = (1 << num_bits[0]);
     root->children = new uint64_t[sz];
     std::memset(root->children, 0, sizeof(root->children) * sz);
     root->mtx = new AtomicBitmap(sz);
 }
 
 SORT::~SORT() {
     std::vector<uint64_t> ptrs;
     std::queue<std::pair<SORTNode*, int>> Q;
     ptrs.emplace_back((uint64_t)root);
     Q.emplace(root, 0);
     while (!Q.empty()) {
         SORTNode* u = Q.front().first;
         int d = Q.front().second;
         Q.pop();
         if (d < depth) {
             if (d < depth - 1) {
                 for (int i = 0; i < (1 << num_bits[d]); i++) {
                     if (u->children[i]) {
                         ptrs.emplace_back(u->children[i]);
                         Q.emplace((SORTNode*)u->children[i], d + 1);
                     }
                 }
             }
         }
     }
     for (auto u : ptrs) {
        delete (SORTNode*)u;
     }
 }
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
#ifndef TRIE
#define TRIE

#include "headers.h"
#define CAP_DUMMY_NODES 50000000

// Revised from GAPBS: https://github.com/sbeamer/gapbs
class AtomicBitmap {
 public:
  explicit AtomicBitmap(size_t size) {
    size_t num_words = (size + kBitsPerWord - 1) / kBitsPerWord;
    start_ = new std::atomic<uint8_t>[num_words];
    end_ = start_ + num_words;
  }

  ~AtomicBitmap() {
    delete [] start_;
  }

  void reset() {
    std::fill(start_, end_, 0);
  }

  void clear_bit(size_t pos) {
    start_[word_offset(pos)].fetch_and(~((uint8_t) 1l << bit_offset(pos)));
  }

  void set_bit(size_t pos) {
    start_[word_offset(pos)].fetch_or((uint8_t) 1l << bit_offset(pos));
  }

  void set_bit_atomic(size_t pos) {
    size_t offset_w = word_offset(pos), offset_b = bit_offset(pos);
    while (start_[offset_w].fetch_or((uint8_t) 1l << offset_b) & ((uint8_t) 1l << offset_b)) {}
  }

  bool get_bit(size_t pos) const {
    return (start_[word_offset(pos)] >> bit_offset(pos)) & 1l;
  }

 private:
  std::atomic<uint8_t> *start_ = nullptr;
  std::atomic<uint8_t> *end_ = nullptr;

  static const size_t kBitsPerWord = 8;
  static size_t word_offset(size_t n) { return n / kBitsPerWord; }
  static size_t bit_offset(size_t n) { return n & (kBitsPerWord - 1); }
};

typedef struct _weighted_edge;
typedef struct _dummy_node;

/* WeightedEdge:
   - Represents a directed weighted edge;
   - e.weight: when it is 0, this edge is a delete log; otherwise it represents the weight of the edge;
   - e.idx: the offset (logical ID) of destination's vertex; vertex_table[e.idx] returns the DummyNode of the vertex.
*/
typedef struct _weighted_edge {
    float weight = 0;
    int idx = -1; 
} WeightedEdge;

/* DummyNode:
   - Stores the information of a vertex;
   - N.node: the vertex ID of this DummyNode;
   - N.idx: the offset (logical ID) of this vertex;
   - N.del_time: the deletion time of this vertex;
   - N.next: the edge array pointer;
   - N.deg: the degree of the vertex (stored for analytical tasks);
   Note that we do not store ``Size`` since it can be retrieved by next.size(); N.idx is stored for practical implementation but can be removed.
*/
typedef struct _dummy_node {
    NodeID node = -1;
    int idx = -1, del_time = 0;
    tbb::concurrent_vector<WeightedEdge> next;
    std::atomic<int> deg;
} DummyNode;

class SORT {
    public:
        typedef struct _sort_node {
            uint64_t* children = nullptr;
            AtomicBitmap* mtx = nullptr;

            ~_sort_node() {
              if (mtx) delete mtx;
              if (children) delete [] children;
            }
        } SORTNode;

        SORTNode* root = nullptr;
        tbb::concurrent_vector<DummyNode> vertex_table;
        std::vector<int> num_bits, sum_bits;
        int depth = 0, space = 0, cap = CAP_DUMMY_NODES;
        std::atomic<int> cnt;

        /*  InsertVertex(): insert a vertex into SORT;
            current: using a recursive algorithm, current represents currently traversed tree node (initially be root);
            id: the vertex ID to be inserted;
            d: the layer of currently traversed tree node. */
        inline DummyNode* InsertVertex(SORTNode* current, NodeID id, int d);
        /*  RetrieveVertex(): retrieve a vertex from SORT;
            id: the vertex ID to be retrieved;
            insert_mode: if set to true, insert the vertex if not found. */
        DummyNode* RetrieveVertex(NodeID id, bool insert_mode=false);
        /*  DeleteVertex(): delete a vertex from SORT;
            id: the vertex ID to be deleted. */
        bool DeleteVertex(NodeID id);

        long long size();

        SORT() {}
        SORT(int d, int _num_bits[]);
        SORT(int d, std::vector<int> _num_bits);
        ~SORT();
};

#endif
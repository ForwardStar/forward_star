#include "headers.h"

struct DummyNode {
    uint64_t node; // this dummy node corresponds to which vertex
    std::atomic<uint8_t> flag[32];
};

class Trie {
    private:
        struct TrieNode {
            bool is_internal;
        };

        typedef struct _internal_node : TrieNode {
            std::vector<TrieNode*> children;
            std::mutex mtx;
        } InternalNode;

        typedef struct _leaf_node : TrieNode {
            DummyNode* head;
            std::mutex mtx;
        } LeafNode;

    public:
        InternalNode* root;
        std::vector<int> num_children;
        int sum_children = 0;
        int depth = 0;
        int space = 0;

        void InsertVertex(uint64_t id, DummyNode* node, int level=0);

        DummyNode* RetrieveVertex(uint64_t id, bool lock=false, int level=0);

        long long size();

        Trie() {}
        Trie(int d, int _num_children[]);
        Trie(int d, std::vector<int> _num_children);
        ~Trie();
};
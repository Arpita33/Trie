#include <iostream>
#include <vector>
#include <cstdint> // For uint8_t
#include <cstring> // For memcpy
#include <memory>
#include <optional>

#include "lib/rocksdb/table/block_based/block.h"

using optional = std::optional;


/**
 * To save space and reduce pointer chasing, we use several different types of nodes that address different common
 * patterns in a trie. It is common for a trie to have one or a couple of top levels which have many children, and where
 * it is important to make decisions with as few if-then-else branches as possible (served by the Split type), another
 * one or two levels of nodes with a small number of children, where it is most important to save space as the number of
 * these nodes is high (served by the Sparse type), and a lot of sequences of single-child nodes containing the trailing
 * bytes of the key or of some common key prefix (served by the Chain type). Most of the payload/content of the trie
 * resides at the leaves, where it makes sense to avoid taking any space for a node (the Leaf type), but we must also
 * allow the possibility for values to be present in intermediate nodes — because this is rare, we support it with a
 * special Prefix type instead of reserving a space for payload in all other node types.
 */
enum class NodeType : uint8_t {
    Leaf = 0,
    Split,
    Chain,
    Sparse,
    Prefix,
};

struct BlockPointer {
    uint32_t ptr: 27;
    uint8_t type: 5;

    [[nodiscard]] bool pointsToContent() const {
        return ptr < 0;
    }

    [[nodiscard]] uint32_t contentPtr() const {
        return ~ptr;
    }

    [[nodiscard]] bool hasChild() const {
        return ptr > 0 && type != 0;
    }
    uint8_t chainCount() {
        // The difference between 0x1C and the pointer offset specifies the number of characters in the chain.
        return 0x1C - type;
    }

    [[nodiscard]] NodeType nodeType() {
        if (type <= 0x1B) return NodeType::Chain;
        return NodeType::Leaf;
    }

    BlockPointer(uint32_t pointer, uint8_t nodeType) : ptr(pointer), type(nodeType) {
    }
};

/** Pointer value NONE (0) is used to specify "no child". */
const auto NONE = BlockPointer{
    0, 0,
};


union Node {
    struct {
        uint8_t _unused[16];
        uint32_t midcell_00;
        uint32_t midcell_01;
        uint32_t midcell_10;
        uint32_t midcell_11;
    } splitNode;

    struct {
        BlockPointer children[6];
        uint8_t characters[6];
        uint8_t order_word: 2;
    } sparseNode;

    struct {
        uint8_t transitions[28];
        BlockPointer child_pointer;
    } chainNode;

    struct {
    } prefixNode;
};
#include <cassert>
using namespace std;

int allocatedPos = 0;
const int BLOCK_SIZE = 32;
const int LAST_POINTER_OFFSET = BLOCK_SIZE - 4;
const int CHAIN_MIN_OFFSET = 0;
const int CHAIN_MAX_OFFSET = BLOCK_SIZE - 5; //27

const int SPARSE_OFFSET = BLOCK_SIZE - 2; //=30
const int SPARSE_CHILD_COUNT = 6;

const int SPLIT_OFFSET = BLOCK_SIZE - 4; //=28
enum class InsertError {
    OutOfMemory
};

// Questions
// - Trie nodes are stored in a contiguous block of memory. Do we ever do deletes? How does that work.
// - What is the `contentArray` and how big should it be?
// - What is the initial node type
// - in the contiguous block of memory, how can we tell what type of node something is
// - what are the conditions that a node transition happens
class Trie {
public:
    constexpr uint32_t BLOCK_SIZE = 8 * sizeof(uint32_t); // 32
    constexpr uint32_t LAST_POINTER_OFFSET = BLOCK_SIZE - 4;
    constexpr uint32_t CHAIN_MIN_OFFSET = 0;
    constexpr uint32_t CHAIN_MAX_OFFSET = BLOCK_SIZE - 5; // 27

    constexpr uint32_t SPARSE_OFFSET = BLOCK_SIZE - 2; // 30
    constexpr uint32_t SPLIT_OFFSET = BLOCK_SIZE - 4; // 28

    explicit Trie(uint32_t count) : bucket_(new Bucket()) {
        bucket_->reserve(count);
    }

    [[nodiscard]] std::optional<uint8_t*> Get();

    /**
     * Insert
     */
    [[nodiscard]] std::optional<InsertError> Insert(uint8_t *key) {
        return std::nullopt;
    }

private:
    using Bucket = std::vector<uint8_t>;
    std::shared_ptr<Bucket> bucket_;
    uint32_t rootIdx_;
};

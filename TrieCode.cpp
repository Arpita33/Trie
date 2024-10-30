#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint> // For uint8_t
#include <cstring> // For memcpy
#include <cassert>
#include<cmath>
#define INT unit32_t
#define BYTE uint8_t

using namespace std;

int allocatedPos = 0;
const int BLOCK_SIZE = 32;
const int LAST_POINTER_OFFSET = BLOCK_SIZE - 4;
const int CHAIN_MIN_OFFSET = 0;
const int CHAIN_MAX_OFFSET = BLOCK_SIZE - 5; //27

const int SPARSE_OFFSET = BLOCK_SIZE - 2; //=30
const int SPARSE_CHILD_COUNT = 6;
const int SPARSE_BYTES_OFFSET = -6;
const int SPLIT_OFFSET = BLOCK_SIZE - 4; //=28

const int size_of_byte_array = 2048*2;
uint8_t byteArray[size_of_byte_array];
int contentArray[10000];
const int NONE = 0;
int contentElements = 0;
int root = 0;

int offset(int pos)
{
    return pos & (BLOCK_SIZE - 1); // returns the least significant 5 offset bits
}
bool isNull(long node)
{
    return node == NONE;
}
bool isLeaf(long node)
{
    return node < NONE;
}
bool isNullOrLeaf(long node)//talk about this tomorrow
{
    //int32_t signed_value = -2;
    //uint32_t unsigned_value = static_cast<uint32_t>(signed_value);
    //return node == NONE || node >=unsigned_value;
    return node <= NONE;
}
bool isExpandableChain(int newChild)
{
    int copy = newChild;
    int newOffset = offset(copy);
    //return newChild > 0 && newChild - 1 > NONE && newOffset > CHAIN_MIN_OFFSET && newOffset <= CHAIN_MAX_OFFSET;
    //std::cout << "newoffset" << newOffset<< std::endl;
    bool ret =  newChild > 0 && newChild - 1 > NONE && newOffset > CHAIN_MIN_OFFSET && newOffset <= CHAIN_MAX_OFFSET;
    //bool ret = newOffset > CHAIN_MIN_OFFSET && newOffset <= CHAIN_MAX_OFFSET;
    //cout<<newChild<<" Return from isexpandable: "<<ret<<endl;
    return ret;
}
int createNewChainNode(uint8_t transitionByte, int childIndex)
{
    int v = allocatedPos;

    //check if space exhausted in current byte buffer, then allocate more space, --- left
    // otherwise place transition byte at v+CHAIN_MAX_OFFSET
    // and pointer from v+CHAIN_MAX_OFFSET+1 to v+CHAIN_MAX_OFFSET+5

    int pos = v+CHAIN_MAX_OFFSET;

    // Ensure there's enough space in the byte array to store both the byte and the integer
    //sizeof(int32_t) = 4
    if (sizeof(byteArray) >= pos + 1 + sizeof(int32_t)) {
        // Store the byte value at index 27
        byteArray[pos] = transitionByte;

        // Store the integer pointer starting at index 28
        std::memcpy(&byteArray[pos+1], &childIndex, sizeof(int32_t));
    } else {
        std::cerr << "Error: Not enough space in the byte array to store both the byte and the integer." << std::endl;
        return 1; // Exit with an error code
    }


    allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

   
    return pos; //return address of the chain node

}

int expandOrCreateChainNode(uint8_t transitionByte, int existingNode)
{
    //cout<<"Inside expand or create: "<<existingNode<<endl;
    //cout<<offset(existingNode)<<endl;
        if (isExpandableChain(existingNode))
        {
            //printf("New Child Expandable. \n");
            // attach as a new character in child node
            int newNode = existingNode - 1;//newNode is the pointer here
            //int index = newChild -1; // index calculated from pointers as array holds contiguous memory addresses
            byteArray[newNode] = transitionByte; 
            //should return pointer to the new index
            //std::cout<< "Offset: " <<offset(newNode)<<std::endl;
            return newNode;
        }
        //printf("Creating a new chain node. \n");
        int ret = createNewChainNode(transitionByte, existingNode);
        //printf("new chain node: %d \n",ret);
        //cout<<"new chain node: "<<ret<<endl;
        return ret;
}
int createEmptySplitNode()
{
    int v = allocatedPos;
    int pos = v+SPLIT_OFFSET;

    // std::memcpy(&byteArray[v], &child1, sizeof(int32_t));
    // std::memcpy(&byteArray[v+4], &child2, sizeof(int32_t));
    // std::memcpy(&byteArray[v+8], &byte1, sizeof(uint8_t));
    // std::memcpy(&byteArray[v+9], &byte2, sizeof(uint8_t));
    // std::memcpy(&byteArray[pos], &orderWord, sizeof(short));

    allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

    //uint32_t SplitNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
    //return SplitNodeOffset; //return address of the Split node
    return pos;
}

/**
 * Given a transition, returns the corresponding index (within the node block) of the pointer to the mid block of
 * a split node.
 */
int splitNodeMidIndex(int trans)
{
    // first 2 bits of the 2-3-3 split
    return (trans >> 6) & 0x3;
}

/**
 * Given a transition, returns the corresponding index (within the mid block) of the pointer to the tail block of
 * a split node.
 */
int splitNodeTailIndex(int trans)
{
    // second 3 bits of the 2-3-3 split
    return (trans >> 3) & 0x7;
}

/**
 * Given a transition, returns the corresponding index (within the tail block) of the pointer to the child of
 * a split node.
 */
int splitNodeChildIndex(int trans)
{
    // third 3 bits of the 2-3-3 split
    return trans & 0x7;
}

void attachChildToSplit(int node, uint8_t trans, int newChild)
{
    //cout<<"transitionByte here:" << static_cast<char>(trans) <<endl;
    int lead_bits = splitNodeMidIndex(trans);
    int lead_position = 16+lead_bits*4;
    int lead_start_index = node-SPLIT_OFFSET+lead_position;
    int32_t midNode;
    std::memcpy(&midNode, &byteArray[lead_start_index], sizeof(int32_t));
    if(midNode == NONE)
    {
        //printf("No entry for mid and tail nodes\n");
        //no entry for leading 2 bits
        midNode = createEmptySplitNode();
        int mid_bits = splitNodeTailIndex(trans);
        int mid_position = mid_bits*4;
        int mid_start_index = midNode-SPLIT_OFFSET+mid_position;
        int tailNode = createEmptySplitNode();
        int tail_bits = splitNodeChildIndex(trans);
        int tail_position = tail_bits*4;
        int tail_start_index = tailNode-SPLIT_OFFSET+tail_position;
        //sizeof(int) = 4
        std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(int32_t)); //link child to tail
        std::memcpy(&byteArray[mid_start_index], &tailNode, sizeof(int32_t)); //link tail to mid
        std::memcpy(&byteArray[lead_start_index], &midNode, sizeof(int32_t)); //link mid to lead
        return;
    }
    int mid_bits = splitNodeTailIndex(trans);
    int mid_position = mid_bits*4;
    int mid_start_index = midNode-SPLIT_OFFSET+mid_position;
    int32_t tailNode;
    std::memcpy(&tailNode, &byteArray[mid_start_index], sizeof(int32_t));
    if(tailNode == NONE)
    {
        //printf("No entry for tail node\n");
        tailNode = createEmptySplitNode();
        int tail_bits = splitNodeChildIndex(trans);
        int tail_position = tail_bits*4;
        int tail_start_index = tailNode-SPLIT_OFFSET+tail_position;
        std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(int32_t)); //link child to tail
        std::memcpy(&byteArray[mid_start_index], &tailNode, sizeof(int32_t)); //link tail to mid
        return;
    }
    //printf("New child added to tail\n");
    int tail_bits = splitNodeChildIndex(trans);
    int tail_position = tail_bits*4;
    int tail_start_index = tailNode-SPLIT_OFFSET+tail_position;
    std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(int32_t)); //link child to tail

}
/**
 * Insert the given newIndex in the base-6 encoded order word in the correct position with respect to the ordering.
 *
 * E.g.
 *   - insertOrderWord(120, 3, 0) must return 1203 (decimal 48*6 + 3)
 *   - insertOrderWord(120, 3, 1, ptr) must return 1230 (decimal 8*36 + 3*6 + 0)
 *   - insertOrderWord(120, 3, 2, ptr) must return 1320 (decimal 1*216 + 3*36 + 12)
 *   - insertOrderWord(120, 3, 3, ptr) must return 3120 (decimal 3*216 + 48)
 */
int insertInOrderWord(int order, int newIndex, int smallerCount)
{
    int r = 1;
    for (int i = 0; i < smallerCount; ++i)
        r *= 6;
    int head = order / r;
    int tail = order % r;
    // insert newIndex after the ones we have passed (order % r) and before the remaining (order / r)
    return tail + (head * 6 + newIndex) * r;
}
int CreateSparseNode(uint8_t byte1, int child1, uint8_t byte2, int child2)
{
    //left to test
        //assert byte1 != byte2 : "Attempted to create a sparse node with two of the same transition";
        if (byte1 > byte2)
        {
            // swap them so the smaller is byte1, i.e. there's always something bigger than child 0 so 0 never is
            // at the end of the order
            uint8_t t = byte1; byte1 = byte2; byte2 = t;
            int temp = child1; child1 = child2; child2 = temp;
        }
        //cout<<"Inside create sparse node: "<<child1<<" "<<child2<<endl;
        
        int v = allocatedPos;
        int pos = v+SPARSE_OFFSET;
        short orderWord = (short)(1*6+0);

        std::memcpy(&byteArray[v], &child1, sizeof(int32_t));

        std::memcpy(&byteArray[v+4], &child2, sizeof(int32_t));

        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4], &byte1, sizeof(uint8_t));
        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4+1], &byte2, sizeof(uint8_t));
        std::memcpy(&byteArray[pos], &orderWord, sizeof(short));

        allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

        //uint32_t SparseNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
        //return SparseNodeOffset; //return address of the Sparse node
        return pos;
}
//Sparse node:
//pointers
//characters
//orderword
int attachChildToSparse(int node, uint8_t transitionByte, int newChild)
{
    int index;
    int smallerCount = 0;
    
    //uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    // first check if this is an update and modify in-place if so
    for (index = 0; index < SPARSE_CHILD_COUNT; ++index)
    {
        int check_ptr_index = node - SPARSE_OFFSET + (index*4);
        int child_idx;
        std::memcpy(&child_idx, &byteArray[check_ptr_index], sizeof(int32_t));
        if(child_idx == NONE) break; // first free position -- not sure if this should be here or it should be in a separate for loop
        // check if the transition matches an existing transition
        // if so update child pointer
        
        int check_byte_index = node - SPARSE_CHILD_COUNT + index;
        uint8_t existingByte;
        std::memcpy(&existingByte, &byteArray[check_byte_index], sizeof(uint8_t));
        if(transitionByte == existingByte)
        {
            std::memcpy(&byteArray[check_ptr_index], &newChild, sizeof(int32_t)); //update child
            return node;
        }
        else if (existingByte < transitionByte)
            ++smallerCount;

    }
    int existingchildCount = index;
    if (existingchildCount == SPARSE_CHILD_COUNT)
    {
            // Node is full. Switch to split
            int split = createEmptySplitNode();
            //cout<<"Node: "<<node<<" "<<offset(node)<<" "<<static_cast<char>(byteArray[node-1])<<endl;
            for (int i = 0; i < SPARSE_CHILD_COUNT; ++i)
            {
                uint8_t byte;
                int byte_index = node - SPARSE_CHILD_COUNT + i;
                std::memcpy(&byte, &byteArray[byte_index], sizeof(uint8_t));
                //cout<<"byte from sparse:" << static_cast<char>(byte) <<endl;

                int32_t pointer;
                int ptr_index = node - SPARSE_OFFSET + i*4;
                //<<ptr_index<<endl;
                std::memcpy(&pointer, &byteArray[ptr_index], sizeof(int32_t));

                attachChildToSplit(split,byte,pointer);
            }
            attachChildToSplit(split,transitionByte,newChild);
            return split;

    }

    //put Byte at node - sparse_children_count+childcount
    int byte_index = node - SPARSE_CHILD_COUNT + existingchildCount;
    std::memcpy(&byteArray[byte_index], &transitionByte, sizeof(uint8_t));
    //Create updated Order Word
    short old_order;
    int order_idx = node;
    std::memcpy(&old_order, &byteArray[order_idx], sizeof(short));
    short newOrder = insertInOrderWord(old_order, existingchildCount, smallerCount);

    //Write the address
    int ptr_index = node - SPARSE_OFFSET + existingchildCount*4;
    std::memcpy(&byteArray[ptr_index], &newChild, sizeof(int32_t));
    //Write the OrderWord at SPARSE_OFFSET
    std::memcpy(&byteArray[order_idx], &newOrder, sizeof(short));
    return node;
    
}
int getSparseChild(int node, uint8_t transition)
{
    //uint32_t ArrayStart = reinterpret_cast<uintptr_t>(&byteArray[0]);
    for (int i = 0; i < SPARSE_CHILD_COUNT; ++i)
    {
        uint8_t byte;
        int bIdx = node + SPARSE_BYTES_OFFSET + i;
        std::memcpy( &byte, &byteArray[bIdx], sizeof(uint8_t));
        if(byte==transition)
        {
            int ind = node - SPARSE_OFFSET + i*4;
            int32_t childptr;
            std::memcpy(&childptr, &byteArray[ind], sizeof(int32_t));
            return childptr;
        }
    }
    return NONE;
}
int getSplitChild(int node, uint8_t transition)
{
    int i_mid = splitNodeMidIndex(transition);
    int i_tail = splitNodeTailIndex(transition);
    int i_child = splitNodeChildIndex(transition);

    //uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    int split_index_mid = node - 12 + i_mid*4; // 12 = 3*4
    int32_t midNode;
    std::memcpy(&midNode, &byteArray[split_index_mid], sizeof(int32_t));
    if (isNull(midNode))
    {
        return NONE;
    }

    int split_index_tail = midNode - SPLIT_OFFSET + i_tail*4;
    int32_t tailNode;
    std::memcpy(&tailNode, &byteArray[split_index_tail], sizeof(int32_t));
    if (isNull(tailNode))
    {
        return NONE;
    }

    int split_index_child = tailNode - SPLIT_OFFSET + i_child*4;
    int32_t childNode;
    std::memcpy(&childNode, &byteArray[split_index_child], sizeof(int32_t));
    return childNode;

}
int getChild(int node, uint8_t transition)
{
    int off = offset(node);
    //uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    //int byte_index = node - arrayStartAddress;
    //cout<<"Inside get child: "<<off<<endl;
    switch (off)
    {
        case SPARSE_OFFSET:
            
            return getSparseChild(node, transition);
        case SPLIT_OFFSET:
            return getSplitChild(node, transition);
        case CHAIN_MAX_OFFSET:
            uint8_t existing_byte;
            existing_byte = byteArray[node];
            //cout<<"inside chain max\n";
            //std::memcpy(&existing_byte, &byteArray[node], sizeof(uint8_t));
            //if (trans != getUnsignedByte(node))
            if(existing_byte!=transition)
                return NONE;
            //return getInt(node + 1);
            int32_t childptr;
            std::memcpy(&childptr, &byteArray[node+1], sizeof(int32_t));
            return childptr;
        default:
            uint8_t existing_byte2 = byteArray[node];
            //int byte_index2 = node - arrayStartAddress;
            //cout<<"here: "<<off<<endl;
            //cout << static_cast<char>(byteArray[node])<<endl;

            //std::memcpy(&existing_byte2, &byteArray[node], sizeof(uint8_t)); //check if off is correct here, need to change
            if (transition != existing_byte2) //if middle transition in chain node does not match
                return NONE;
            
            int next = node+1;
            // if (byteArray[next] == static_cast<uint8_t>(0))
            // {
            //     // int32_t childptr;
            //     // cout << node - offset(node) + CHAIN_MAX_OFFSET<<endl;
            //     // std::memcpy(&childptr, &byteArray[node - offset(node) + CHAIN_MAX_OFFSET], sizeof(int32_t));
            //     // return childptr;
            //     cout << "Next point is null in chain" <<endl;
            // }
            return node + 1; //return next transition in chain node until offset == CHAIN_MAX_OFFSET
    }
}

int getChildforPut(int node, uint8_t transition)
{
    int off = offset(node);
    //uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    //int byte_index = node - arrayStartAddress;
    //cout<<"Inside get child: "<<off<<endl;
    switch (off)
    {
        case SPARSE_OFFSET:
            
            return getSparseChild(node, transition);
        case SPLIT_OFFSET:
            return getSplitChild(node, transition);
        case CHAIN_MAX_OFFSET:
            uint8_t existing_byte;
            existing_byte = byteArray[node];
            //cout<<"inside chain max\n";
            //std::memcpy(&existing_byte, &byteArray[node], sizeof(uint8_t));
            //if (trans != getUnsignedByte(node))
            if(existing_byte!=transition)
                return NONE;
            //return getInt(node + 1);
            int32_t childptr;
            std::memcpy(&childptr, &byteArray[node+1], sizeof(int32_t));
            return childptr;
        default:
            uint8_t existing_byte2 = byteArray[node];
            //int byte_index2 = node - arrayStartAddress;
            //cout<<"here: "<<off<<endl;
            //cout << static_cast<char>(byteArray[node])<<endl;

            //std::memcpy(&existing_byte2, &byteArray[node], sizeof(uint8_t)); //check if off is correct here, need to change
            if (transition != existing_byte2) //if middle transition in chain node does not match
                return NONE;
            
            int next = node+1;
            if (byteArray[next] == static_cast<uint8_t>(0))
            {
                //cout << "Next point is null in chain" <<endl;
                int32_t childptr;
                //cout << node - offset(node) + CHAIN_MAX_OFFSET<<endl;
                std::memcpy(&childptr, &byteArray[node - offset(node) + CHAIN_MAX_OFFSET + 1], sizeof(int32_t));
                return childptr;
                
            }
            return node + 1; //return next transition in chain node until offset == CHAIN_MAX_OFFSET
    }
}


int putRecursive2(int node, const std::string& key, int value)
{
    //cannot insert same key twice - update not supported yet
    if (key.empty()) {
        //printf("Putting Value in Content Array from putRecursive2\n");
        //end of key reached
        //put the content here
        int idx = contentElements+1;
        contentArray[idx] = value;
        ++contentElements;
        //printf("idx=%d,~idx=%d\n",idx,~idx);
        return ~idx;
    }
    //get character
    char c = key[0];
    //printf("%c\n",c);

    uint8_t transitionbyte= static_cast<uint8_t>(c);

    int child = getChildforPut(node, transitionbyte);

    int newChild = putRecursive2(child, key.substr(1), value);
    int off_node = offset(node);
    //int off_child = offset(child);
    int off_nC = offset(newChild);

    if(isNull(node))
    {
        
        int ret = expandOrCreateChainNode(transitionbyte, newChild);
        return ret;
    }


    if(off_node>=CHAIN_MIN_OFFSET && off_node<=CHAIN_MAX_OFFSET)
    {
            uint8_t existing_byte = byteArray[node];
            int diff = CHAIN_MAX_OFFSET - off_node;
            int chain_max = node+diff;

        if(existing_byte == transitionbyte)
        {

            //if(offset(newChild) == SPARSE_OFFSET) // or newChild!=child -- check if this falls in any case
            if(newChild!=child)
            {

                //covers update too
                std::memcpy(&byteArray[chain_max+1], &newChild, sizeof(int32_t));
            }
            
            return node;


        }

        //before this create a new chain node for the first part

        int childIdx;
        std::memcpy(&childIdx,&byteArray[chain_max+1],sizeof(int32_t));

    //check if child is chain or sparse --- act accordingly

        if (!isNullOrLeaf(childIdx) && offset(childIdx) == SPARSE_OFFSET)
        {
            if(isNull(existing_byte)) 
            {
                // add to existing sparse child
                //"Adding child to sparse"<<endl;
                return attachChildToSparse(childIdx, transitionbyte, newChild);
            }
            //"Creating sparse to link to sparse"<<endl;
            //child is sparse
            //parent is chain
            //if the character exists, convert that into sparse node
            if(isNull(byteArray[node+1])){ //nulll after node -- no more chain
            // byteArray[chain_max] = static_cast<uint8_t>(0);
            // for(int i = chain_max-1; i>=node+1;i--)
            // {
            //     cout<<i<<endl;
            //     cout<<static_cast<char>(byteArray[i])<<endl;
            //     byteArray[i] = static_cast<uint8_t>(0);
            // }
            byteArray[node] = static_cast<uint8_t>(0);
            int ret = CreateSparseNode(existing_byte, childIdx, transitionbyte, newChild);
            return ret;
            }


        }    
        if (!isNullOrLeaf(childIdx) && offset(childIdx) == SPLIT_OFFSET)
        {
            if(isNull(existing_byte)) 
            {
                // add to existing sparse child
                attachChildToSplit(childIdx, transitionbyte, newChild);
                return childIdx;
            }
            //"Creating sparse to link to split"
            //child is split
            //parent is chain
            //create sparse to link two chains and then link the child to sparse node
            //if the character exists, convert that into sparse node
            if(isNull(byteArray[node+1])){ 
            // byteArray[chain_max] = static_cast<uint8_t>(0);
            // for(int i = chain_max-1; i>=node+1;i--)
            // {
            //     byteArray[i] = static_cast<uint8_t>(0);
            // }
            byteArray[node] = static_cast<uint8_t>(0);
            int ret = CreateSparseNode(existing_byte, childIdx, transitionbyte, newChild);
            return ret;
            }
        }  
        if(off_node == CHAIN_MAX_OFFSET)
        {
            int childIdx;
            std::memcpy(&childIdx,&byteArray[chain_max+1],sizeof(int32_t));
            byteArray[node] = static_cast<uint8_t>(0);
            int ret = CreateSparseNode(existing_byte, childIdx, transitionbyte, newChild);
            return ret;
        }

        int newChainNode = createNewChainNode(byteArray[chain_max], childIdx);
        byteArray[chain_max] = static_cast<uint8_t>(0);
        for(int i = chain_max-1; i>=node+1;i--)
        {
            newChainNode--;
            byteArray[newChainNode]=byteArray[i];
            byteArray[i] = static_cast<uint8_t>(0);

        }
        byteArray[node] = static_cast<uint8_t>(0);

        int ret = CreateSparseNode(existing_byte, newChainNode, transitionbyte, newChild);

        return ret;
    }
    else if(off_node == SPARSE_OFFSET)
    {
        return attachChildToSparse(node, transitionbyte, newChild);
    }
    else if(off_node == SPLIT_OFFSET)
    {
        attachChildToSplit(node, transitionbyte, newChild);
        return node;
    }

    return -100000;
}
int getValue(int node, string key)
{    
    if (key.empty()) {
        //printf("Get Value from Content Array\n");
        int idx= ~node;
        return contentArray[idx];
        //return 0;
    }
    //get character
    char c = key[0];
    int child = getChild(node, c);


    if(child == NONE)//for sparse child, there won't be any branch with that transition, so we need to recurse again on the child
    {
        int off_node = offset(node);
        if(off_node>=CHAIN_MIN_OFFSET && off_node<=CHAIN_MAX_OFFSET)
        {
            int child_start = node + CHAIN_MAX_OFFSET - off_node+1;
            int child2;
            std::memcpy(&child2, &byteArray[child_start],  sizeof(int32_t));
            if(child2!=NONE)
            {
                return getValue(child2,key);
            }
        }
    }
    
    //cout<<"Child: "<<child<<endl;

    return getValue(child,key.substr(1));
}


int getValue2(int node, string key)
{    
    if (key.empty()) {
        //printf("Get Value from Content Array\n");
        int idx= ~node;
        //cout<<"Node: "<< node<<endl;
        return contentArray[idx];
        //return 0;
    }
    //get character
    char c = key[0];
    //printf("%c\n",c);
    //cout<<"Node: "<<node<<endl;
    int child = getChildforPut(node, c);

    return getValue2(child,key.substr(1));
}

std::pair<std::string, std::string> splitBySpace(const std::string& str) {
    std::istringstream iss(str); // Convert the string to a stream
    std::string first, second;

    // Extract the first two words from the stream
    if (iss >> first && iss >> second) {
        return std::make_pair(first, second); // Return the pair of words
    } else {
        return std::make_pair(first, ""); // Return the first word and empty string if no second word is found
    }
}

int main() {
    // Open the file workload.txt
    std::ifstream file("workload.txt");

    // Check if the file is open
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file workload.txt" << std::endl;
        return 1; // Return error code
    }

    std::string line;

    // First pass: Read the file line by line
    std::cout << "First pass:" << std::endl;
    while (std::getline(file, line)) {
        //std::cout << line << std::endl;
        std::pair<std::string, std::string> result = splitBySpace(line);
        //std::cout << "Key: " << result.first << " Value : " << result.second << std::endl;
        int value = std::stoi(result.second);
        root = putRecursive2(root,result.first, value);
        cout<<"After putting "<<result.first<<endl;
        int ret_val = getValue(root,result.first);
        cout<<"Value = "<<ret_val<<endl;

    }
    //root = putRecursive2(root, "abca", 284623);
    cout<<root<<endl;
    cout<<offset(root)<<endl;
    
    // Clear the error state (eof) and return to the beginning of the file
    file.clear(); // Clear any errors (such as end-of-file)
    file.seekg(0, std::ios::beg); // Go back to the beginning of the file

    // Second pass: Read the file line by line again
    std::cout << "\nSecond pass:" << std::endl;
    while (std::getline(file, line)) {
        //std::cout << line << std::endl;
        std::pair<std::string, std::string> result = splitBySpace(line);
        //std::cout << "Key: " << result.first << " Value : " << result.second << std::endl;
        int v = getValue2(root, result.first);
        cout<<result.first<<" Value : "<<v<<endl;
    }
    // Close the file
    file.close();

    return 0; // Success
}

//read - insert - print - then print all keys together
/*try 120
trie 145
tea 175
tire 50
tirk 65
terrain 200
extreme 75*/

/*
test 10
tee 20
trip 30
tea 40
trim 50
*/

/*
batch 10
bath 20
bate 30
baton 40
bats 50
batter 60
batman 70
*/

/*
abc 10
bcd 20
cde 30
def 40
efg 50
fgh 60
ghi 70
*/
/*
abca 100
abcb 200
abcc 300
abcd 400
abce 500
abcf 600
abcg 700
abci 800
abcj 900
abck 950
abcy 650*/

/*
abca 100
abcb 200
abcc 300
abcd 400
abce 500
abcf 600
abcg 700
abci 800
abcj 900
abck 950
abcy 650
abczksf 34
abcudlf 78
*/

/*
batch 10
bath 20
bate 30
baton 40
bats 50
batter 60
batman 70
abcfa 25
*/

/*
batch 10
bats 20
baton 30
catch 40
cater 50
cattle 60
rates 70
ratify 80
rattle 90
*/
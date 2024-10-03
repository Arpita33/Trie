#include <iostream>
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
    cout<<"Inside expand or create: "<<existingNode<<endl;
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
        cout<<"new chain node: "<<ret<<endl;
        return ret;
}
int createEmptySplitNode()
{
    int v = allocatedPos;
    int pos = v+SPLIT_OFFSET;


    allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

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
    int lead_bits = splitNodeMidIndex(trans);
    int lead_position = 16+lead_bits*4;
    int lead_start_index = node-SPLIT_OFFSET+lead_position;
    int32_t midNode;
    std::memcpy(&midNode, &byteArray[lead_start_index], sizeof(int32_t));
    if(midNode == NONE)
    {
        printf("No entry for mid and tail nodes\n");
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
        printf("No entry for tail node\n");
        tailNode = createEmptySplitNode();
        int tail_bits = splitNodeChildIndex(trans);
        int tail_position = tail_bits*4;
        int tail_start_index = tailNode-SPLIT_OFFSET+tail_position;
        std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(int32_t)); //link child to tail
        std::memcpy(&byteArray[mid_start_index], &tailNode, sizeof(int32_t)); //link tail to mid
        return;
    }
    printf("New child added to tail\n");
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
        cout<<"Inside create sparse node: "<<child1<<" "<<child2<<endl;
        
        int v = allocatedPos;
        int pos = v+SPARSE_OFFSET;
        short orderWord = (short)(1*6+0);
        //cout<<"v:"<<v<<endl;
        std::memcpy(&byteArray[v], &child1, sizeof(int32_t));
        //cout<<byteArray[96]<<endl;
        std::memcpy(&byteArray[v+4], &child2, sizeof(int32_t));
        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4], &byte1, sizeof(uint8_t));
        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4+1], &byte2, sizeof(uint8_t));
        std::memcpy(&byteArray[pos], &orderWord, sizeof(short));

        allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

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
            for (int i = 0; i < SPARSE_CHILD_COUNT; ++i)
            {
                uint8_t byte;
                int byte_index = node - SPARSE_CHILD_COUNT + index;
                std::memcpy(&byte, &byteArray[byte_index], sizeof(uint8_t));

                int32_t pointer;
                int ptr_index = node - SPARSE_OFFSET + index*4;
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
            if(existing_byte!=transition)
                return NONE;
            //return getInt(node + 1);
            int32_t childptr;
            std::memcpy(&childptr, &byteArray[node+1], sizeof(int32_t));
            return childptr;
        default:
            uint8_t existing_byte2 = byteArray[node];
            if (transition != existing_byte2) //if middle transition in chain node does not match
                return NONE;
            return node + 1; //return next transition in chain node until offset == CHAIN_MAX_OFFSET
    }
}

int putRecursive2(int node, const std::string& key, int value)
{
    //cannot insert same key twice - update not supported yet
    if (key.empty()) {
        printf("Putting Value in Content Array from putRecursive2\n");
        //end of key reached
        //put the content here
        int idx = contentElements+1;
        contentArray[idx] = value;
        ++contentElements;
        printf("idx=%d,~idx=%d\n",idx,~idx);
        return ~idx;
    }
    //get character
    char c = key[0];
    printf("%c\n",c);

    uint8_t transitionbyte= static_cast<uint8_t>(c);
    //cout<<"before get child"<<endl;
    int child = getChild(node, transitionbyte);
    //cout<<"After getchild" <<endl;
    int newChild = putRecursive2(child, key.substr(1), value);
    if(isNullOrLeaf(newChild)) // if child is null too -- check
    {
        
        int32_t ret = expandOrCreateChainNode(transitionbyte, newChild);
        cout<< "Inside is null or leaf: "<<ret << " "<< offset(ret)<<endl;
        //cout<<"Transition Byte:"<< static_cast<uint8_t>(transitionbyte)<<endl;
        return ret;
    }

    if(isNull(node))
    {
        
        int ret = expandOrCreateChainNode(transitionbyte, newChild);
        //cout<<"Inside is null Node"<< static_cast<char>(byteArray[ret])<<endl;
        return ret;
    }
    int off_node = offset(node);
    //int off_child = offset(child);
    int off_nC = offset(newChild);
    //uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    //cout << "Off Node: "<<off_node<<endl;
    //printf("Character here: %c\n",c);
    //cout<<"byte at node: "<<static_cast<char>(byteArray[node])<<endl;
    //cout<<isNull(byteArray[node])<<endl;
    //cout<<"node value: "<<node<<endl;
    if(off_node>=CHAIN_MIN_OFFSET && off_node<=CHAIN_MAX_OFFSET)
    {
            uint8_t existing_byte = byteArray[node];
            cout<<"Is null existing byte: "<<isNull(byteArray[node])<<endl;
            int diff = CHAIN_MAX_OFFSET - off_node;
            int chain_max = node+diff;
            //cout<<"Node: "<<node<<endl;
            //cout<<"Existing Byte:"<< static_cast<uint8_t>(existing_byte)<<endl;
            //cout<<"Transition Byte:"<< static_cast<uint8_t>(transitionbyte)<<endl;
            //cout<<"Child: "<<child<<endl;
            //cout<<"New Child: "<<newChild << "off newchild: "<<offset(newChild)<<endl;

        if(existing_byte == transitionbyte)
        {
            //for update - just change the child pointer ---left
            //suppose ka, ke -- here k, new k
            //copy the previous branch into a new chain then, I mean break the previous chain into two parts
            // so you have to update the child pointer to a sparse node with the two different children
            // and then return the node
            //cout<<"Child 1:"<< static_cast<uint8_t>(byteArray[child])<<endl;
            //cout<<"Child 2:"<< static_cast<uint8_t>(byteArray[newChild])<<endl;

            cout<<"here: existing = transition byte"<<endl;
            if(offset(newChild) == SPARSE_OFFSET) // or newChild!=child -- check if this falls in any case
            {
                std::memcpy(&byteArray[chain_max+1], &newChild, sizeof(int32_t));
            }
            
            return node;


        }

        //before this create a new chain node for the first part
        //cout << "here: "<< static_cast<char>(byteArray[node])<<static_cast<uint8_t>(transitionbyte)<<endl;
        //cout<<newChild<<endl;
        int childIdx;
        std::memcpy(&childIdx,&byteArray[chain_max+1],sizeof(int32_t));

    //check if child is chain or sparse --- act accordingly
        cout<< "Type of child: "<<offset(childIdx)<<" "<<childIdx<<endl;
        if (offset(childIdx) == SPARSE_OFFSET)
        {
            if(isNull(existing_byte)) 
            {
                // no character starting from that point in chain, 
                //check if character exists in sparse child - one extra level added
                // int numchild=6;
                // for(int i = childIdx+SPARSE_BYTES_OFFSET; i<childIdx; i++)
                // {
                //     uint8_t byteInSparse = byteArray[i];
                //     if(transitionbyte == byteInSparse)
                //     {
                //         int childOfSparse;
                //         std::memcpy(&childOfSparse,&byteArray[childIdx+SPARSE_BYTES_OFFSET-(numchild*4)],sizeof(int32_t));
                //         return putRecursive2(childOfSparse,key.substr(1),value);
                //     }
                //     numchild--;
                // }

                // add to existing sparse child
                cout<<"Adding child to sparse"<<endl;
                return attachChildToSparse(childIdx, transitionbyte, newChild);
            }
            cout<<"Creating sparse to link to sparse"<<endl;
            //if the character exists, convert that into sparse node
            byteArray[chain_max] = static_cast<uint8_t>(0);
            for(int i = chain_max-1; i>=node+1;i--)
            {
                byteArray[i] = static_cast<uint8_t>(0);
            }
            byteArray[node] = static_cast<uint8_t>(0);
//for tire new child not need to be created -- add condition for adding child to sparse
            int ret = CreateSparseNode(existing_byte, childIdx, transitionbyte, newChild);
            return ret;
        }    

        int newChainNode = createNewChainNode(byteArray[chain_max], childIdx);
        byteArray[chain_max] = static_cast<uint8_t>(0);
        for(int i = chain_max-1; i>=node+1;i--)
        {
            //cout<<static_cast<uint8_t>(byteArray[i])<<endl;
            newChainNode--;
            byteArray[newChainNode]=byteArray[i];
            byteArray[i] = static_cast<uint8_t>(0);
            //cout<<static_cast<uint8_t>(byteArray[newChainNode])<<endl;
        }
        byteArray[node] = static_cast<uint8_t>(0);
        
        //cout<<"creating new sparse node: "<<static_cast<uint8_t>(existing_byte)<<" "<<static_cast<uint8_t>(transitionbyte)<<endl;
        //cout<<newChainNode<<endl;
        //cout<<newChild<<endl;
        //int ret =CreateSparseNode(existing_byte, node+1, transitionbyte, newChild);//check if node/child here

        int ret = CreateSparseNode(existing_byte, newChainNode, transitionbyte, newChild);
        //cout << static_cast<char>(byteArray[ret+SPARSE_BYTES_OFFSET])<<endl;
        //cout << static_cast<char>(byteArray[ret+SPARSE_BYTES_OFFSET+1])<<endl;
        return ret;
    }
    else if(off_node == SPARSE_OFFSET)
    {
        cout<<"attaching child to sparse"<<endl;
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
        printf("Get Value from Content Array\n");
        int idx= ~node;
        //cout<<"Node: "<< node<<endl;
        return contentArray[idx];
        //return 0;
    }
    //get character
    char c = key[0];
    //printf("%c\n",c);
    //cout<<"Node: "<<node<<endl;
    int child = getChild(node, c);
    //cout<<child<<" Child type: "<<offset(child)<<endl;

    if(child == NONE)//for sparse child, there won't be any branch with that transition, so we need to recurse again on the child
    {
        int off_node = offset(node);
        //cout<<"Child is none: "<<off_node<<endl;
        if(off_node>=CHAIN_MIN_OFFSET && off_node<=CHAIN_MAX_OFFSET)
        {
            int child_start = node + CHAIN_MAX_OFFSET - off_node+1;
            int child2;
            std::memcpy(&child2, &byteArray[child_start],  sizeof(int32_t));
            if(child2!=NONE)
            {
                // if(offset(child2) == SPARSE_OFFSET)
                // {
                //     cout<<"child is sparse"<<endl;
                //     //child2 = getChild(child2,c);
                // }
                //cout<<"In here, Child: "<<offset(child2)<<endl;
                //cout<<"key at this point: "<<key<<endl;
                return getValue(child2,key);
            }
        }
    }
    
    //cout<<"Child: "<<child<<endl;

    return getValue(child,key.substr(1));
   // return getValue(0,key.substr(1));
}

int main()
{
    //int off = offset(BLOCK_SIZE*4+SPLIT_OFFSET);
    //printf("offset: %d\n",off);
    // char c = 'A'; // ASCII value of 'A' is 65
    // uint8_t byte1 = static_cast<uint8_t>(c);
    // int idx = createNewChainNode(byte1, 10);
    // cout<<offset(idx)<<endl;
    // c = 'B'; // ASCII value of 'A' is 65
    // byte1 = static_cast<uint8_t>(c);
    // idx = expandOrCreateChainNode(byte1, idx);
    // cout<<offset(idx)<<endl;

    int root = 0;
    root = putRecursive2(root, "try", 120);
    cout<<"root: "<<root<<endl;
    int v = getValue(root, "try");
    cout<<"Value : "<<v<<endl;
    //cout<<"root: "<<root<<endl;

    //root = putRecursive2(root, "tea", 145);
    root = putRecursive2(root, "trie", 145);
    // v = getValue(root, "trie");
    // cout<<"Value : "<<v<<endl;

    root = putRecursive2(root, "tea", 175);
    v = getValue(root, "tea");
    cout<<"tea Value : "<<v<<endl;

    root = putRecursive2(root, "tire", 50);
    cout<<"root: "<<root<<endl;
    v = getValue(root, "tire");
    cout<<"tire Value : "<<v<<endl;

    // v = getValue(root, "trie");
    // cout<<"trie Value : "<<v<<endl;

    v = getValue(root, "try");
    cout<<"try Value : "<<v<<endl;

    v = getValue(root, "trie");
    cout<<"trie Value : "<<v<<endl;

    v = getValue(root, "tea");
    cout<<"tea Value : "<<v<<endl;

    //root = putRecursive2(root, "tire", 120);//error when 3 are inserted
    cout<<"------"<<endl;
    cout<<"root: "<<root<<endl;

    root = putRecursive2(root, "terrain", 200);
    cout<<"root: "<<root<<endl;
    v = getValue(root, "terrain");
    cout<<"terrain Value : "<<v<<endl;

    v = getValue(root, "tea");
    cout<<"tea Value : "<<v<<endl;

    // cout<<static_cast<char>(byteArray[root])<<endl;
    // int child;
    // std::memcpy(&child, &byteArray[root+CHAIN_MAX_OFFSET - offset(root)+1],  sizeof(int32_t));
    // cout<<child<<endl;
    // cout<<static_cast<char>(byteArray[child-SPARSE_CHILD_COUNT])<<endl;
    // cout<<static_cast<char>(byteArray[child-SPARSE_CHILD_COUNT+1])<<endl;
    // std::memcpy(&child, &byteArray[child-SPARSE_OFFSET+4],  sizeof(int32_t));
    // cout<<child<<endl;
    // cout<<byteArray[child]<<endl;
    
/*
    cout<<static_cast<char>(byteArray[root])<<endl;
    int child;
    std::memcpy(&child, &byteArray[root+CHAIN_MAX_OFFSET - offset(root)+1],  sizeof(int32_t));
    //cout<<child<<" "<<offset(child)<<endl;
    cout<<static_cast<char>(byteArray[child+SPARSE_BYTES_OFFSET])<<endl;
    
    int child2, nc;
    std::memcpy(&child2, &byteArray[child-SPARSE_OFFSET],  sizeof(int32_t));
    //cout<<child<<" "<<child2<<" "<<byteArray[child2]<<endl;
    for(int i=child2;i<=child2-offset(child2)+CHAIN_MAX_OFFSET;i++)
    {
        cout<<static_cast<char>(byteArray[i])<<endl;
    }
    std::memcpy(&nc, &byteArray[child2-offset(child2)+CHAIN_MAX_OFFSET + 1],  sizeof(int32_t));
    //cout<<child2-offset(child2)+CHAIN_MAX_OFFSET<<endl;
    cout<<contentArray[~nc]<<endl;
    cout<<static_cast<char>(byteArray[child+SPARSE_BYTES_OFFSET+1])<<endl;
    int child3;
    std::memcpy(&child3, &byteArray[child-SPARSE_OFFSET+4],  sizeof(int32_t));
    for(int i=child3;i<=child3-offset(child3)+CHAIN_MAX_OFFSET;i++)
    {
        cout<<static_cast<char>(byteArray[i])<<endl;
    }
    cout<<child3-offset(child3)+CHAIN_MAX_OFFSET<<endl;
    std::memcpy(&nc, &byteArray[child3-offset(child3)+CHAIN_MAX_OFFSET+1], sizeof(int32_t));
    cout<<contentArray[~nc]<<endl;
    //cout<<child<<" "<<child3<<" "<<byteArray[child3]<<endl;
*/




//for printing from sparse root:

/*    cout<<static_cast<char>(byteArray[root+SPARSE_BYTES_OFFSET])<<endl;
    int child2, child3;
    std::memcpy(&child2, &byteArray[root-SPARSE_OFFSET],  sizeof(int32_t));
    //cout<<child2<<" "<<byteArray[child2]<<endl;
    int c_off = offset(child2);
    int chain_child = child2 - c_off + CHAIN_MAX_OFFSET + 1;
    for(int i=child2;i<chain_child;i++)
    {
        cout<<static_cast<char>(byteArray[i])<<endl;
    }
    //cout<<c_off<<" "<<offset(chain_child)<<endl;
    std::memcpy(&child3, &byteArray[chain_child],  sizeof(int32_t));
    cout<<child3<<" "<<contentArray[~child3]<<endl;

    cout<<static_cast<char>(byteArray[root+SPARSE_BYTES_OFFSET+1])<<endl;
    std::memcpy(&child2, &byteArray[root-SPARSE_OFFSET+4],  sizeof(int32_t));
    //cout<<child2<<" "<<byteArray[child2]<<endl;
    c_off = offset(child2);
    chain_child = child2 - c_off + CHAIN_MAX_OFFSET + 1;
    for(int i=child2;i<chain_child;i++)
    {
        cout<<static_cast<char>(byteArray[i])<<endl;
    }
    //cout<<c_off<<" "<<offset(chain_child)<<endl;
    std::memcpy(&child3, &byteArray[chain_child],  sizeof(int32_t));
    cout<<child3<<" "<<contentArray[~child3]<<endl;
 
*/

//workload file -> input to function
//printing finalize

}
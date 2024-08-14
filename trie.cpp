#include <iostream>
#include <vector>
#include <cstdint> // For uint8_t
#include <cstring> // For memcpy
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

const int size_of_byte_array = 2048;
uint8_t byteArray[size_of_byte_array];
int contentArray[10];



/**
 * Pointer offset for a node pointer.
 */
int offset(int pos)
{
    return pos & (BLOCK_SIZE - 1); // returns the least significant 5 offset bits
}
bool isExpandableChain(int newChild)
{
    int newOffset = offset(newChild);
    //return newChild > 0 && newChild - 1 > NONE && newOffset > CHAIN_MIN_OFFSET && newOffset <= CHAIN_MAX_OFFSET;
    //what should we set NONE here to?
    //std::cout << "newoffset" << newOffset<< std::endl;
    return newOffset > CHAIN_MIN_OFFSET && newOffset <= CHAIN_MAX_OFFSET;
}
int createNewChainNode(uint8_t transitionByte, int childPointer)
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
        std::memcpy(&byteArray[pos+1], &childPointer, sizeof(int32_t));
    } else {
        std::cerr << "Error: Not enough space in the byte array to store both the byte and the integer." << std::endl;
        return 1; // Exit with an error code
    }


    allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

    //return allocatedPos;
    uint32_t ChainNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
    return ChainNodeOffset; //return address of the chain node

}

int expandOrCreateChainNode(uint8_t transitionByte, int newChild)
{
        if (isExpandableChain(newChild))
        {
            // attach as a new character in child node
            //int newNode = newChild - 1;//newNode is the pointer here
            uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
            //uintptr_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
            //uintptr_t elementAddress = reinterpret_cast<uintptr_t>(newNode);
            int index = newChild - arrayStartAddress -1; // index calculated from pointers as array holds contiguous memory addresses
            //std::cout << "here" << index << std::endl;
            // *newNode = valueToWrite;
            byteArray[index] = transitionByte; 
            //return newNode;
            //should return pointer to the new index
            std::cout<< "Offset: " <<offset(newChild-1)<<std::endl;
            return newChild-1;
        }

        return createNewChainNode(transitionByte, newChild);
    
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
        int v = allocatedPos;
        int pos = v+SPARSE_OFFSET;
        short orderWord = (short)(1*6+0);
        std::memcpy(&byteArray[v], &child1, sizeof(int32_t));
        std::memcpy(&byteArray[v+4], &child2, sizeof(int32_t));
        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4], &byte1, sizeof(uint8_t));
        std::memcpy(&byteArray[v+SPARSE_CHILD_COUNT*4+1], &byte2, sizeof(uint8_t));
        std::memcpy(&byteArray[pos], &orderWord, sizeof(short));

        allocatedPos+=BLOCK_SIZE; // advance the allocatedPos to the next block

        uint32_t SparseNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
        return SparseNodeOffset; //return address of the Sparse node
}
int attachChildToSparse(int32_t node, uint8_t transitionByte, int newChild);
void printSparseNode(int node)
{
    //Sparse node:
    //pointers
    //characters
    //orderword
    uint32_t ArrayStart = reinterpret_cast<uintptr_t>(&byteArray[0]);
    int orderwordIdx = node - ArrayStart;
    short orderWord;
    std::memcpy( &orderWord, &byteArray[orderwordIdx], sizeof(short));
    printf("Order word: %d\n", orderWord);
    uint8_t byte1, byte2;
    int bIdx1 = node - SPARSE_OFFSET + SPARSE_CHILD_COUNT*4 -ArrayStart;
    int bIdx2 = bIdx1+1;
    std::memcpy( &byte1, &byteArray[bIdx1], sizeof(uint8_t));
    std::memcpy( &byte2, &byteArray[bIdx2], sizeof(uint8_t));
    printf("%c, %c\n", byte1, byte2);
    
    int ind = node -SPARSE_OFFSET - ArrayStart;
    uint32_t childptr1, childptr2;
    std::memcpy(&childptr1, &byteArray[ind], sizeof(uint32_t));
    std::memcpy(&childptr2, &byteArray[ind+4], sizeof(uint32_t));

    uint32_t contentStartPtr = reinterpret_cast<uintptr_t>(&contentArray[0]);
    int child_index = (childptr1 - contentStartPtr)/sizeof(int32_t);
    printf("%d\n",child_index);
    printf("%d\n",contentArray[child_index]);
    
    child_index = (childptr2 - contentStartPtr)/sizeof(int32_t);
    printf("%d\n",child_index);
    printf("%d\n",contentArray[child_index]);

    char c = 'D'; // ASCII value of 'A' is 65
    uint8_t byte3 = static_cast<uint8_t>(c);
    uint32_t child3 = reinterpret_cast<uintptr_t>(&contentArray[0]);
    attachChildToSparse(node,byte3,child3);
        
    std::memcpy( &orderWord, &byteArray[orderwordIdx], sizeof(short));
    printf("Order word: %d\n", orderWord);
    std::memcpy( &byte2, &byteArray[bIdx2+1], sizeof(uint8_t));
    printf("%c\n",  byte2);
    std::memcpy(&childptr2, &byteArray[ind+8], sizeof(uint32_t));
    child_index = (childptr2 - contentStartPtr)/sizeof(int32_t);
    printf("%d\n",child_index);
    printf("%d\n",contentArray[child_index]);

}

void printBinary(int number) {
    // We will print 32 bits (for a 32-bit integer)
    for (int i = 31; i >= 0; i--) {
        int bit = (number >> i) & 1; // Right shift and mask to get the ith bit
        std::cout << bit;
    }
    std::cout << std::endl;
}
/**
 * Given a transition, returns the corresponding index (within the node block) of the pointer to the mid block of
 * a split node.
 */
int splitNodeMidIndex(int trans)
{
    //printBinary(trans);
    // first 2 bits of the 2-3-3 split
    //int r = (trans >> 6) & 0x3;
    //std:: cout<< r << std::endl;
    return (trans >> 6) & 0x3;
}

/**
 * Given a transition, returns the corresponding index (within the mid block) of the pointer to the tail block of
 * a split node.
 */
int splitNodeTailIndex(int trans)
{
    // printBinary(trans);
    // int r = (trans >> 3) & 0x7;
    // std:: cout<< r << std::endl;
    // second 3 bits of the 2-3-3 split
    return (trans >> 3) & 0x7;
}

/**
 * Given a transition, returns the corresponding index (within the tail block) of the pointer to the child of
 * a split node.
 */
int splitNodeChildIndex(int trans)
{
    // printBinary(trans);
    // int r = (trans) & 0x7;
    // std:: cout<< r << std::endl;
    // third 3 bits of the 2-3-3 split
    return trans & 0x7;
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

    uint32_t SplitNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
    return SplitNodeOffset; //return address of the Split node
}

void attachChildToSplit(int node, uint8_t trans, int newChild)
{
    int lead_bits = splitNodeMidIndex(trans);
    int lead_position = 16+lead_bits*4;
    uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    int lead_start_index = node-SPLIT_OFFSET+lead_position - arrayStartAddress;
    uint32_t midNode;
    std::memcpy(&midNode, &byteArray[lead_start_index], sizeof(int32_t));
    if(midNode == NULL)
    {
        printf("No entry for mid and tail nodes\n");
        //no entry for leading 2 bits
        midNode = createEmptySplitNode();
        int mid_bits = splitNodeTailIndex(trans);
        int mid_position = mid_bits*4;
        int mid_start_index = midNode-SPLIT_OFFSET+mid_position - arrayStartAddress;
        int tailNode = createEmptySplitNode();
        int tail_bits = splitNodeChildIndex(trans);
        int tail_position = tail_bits*4;
        int tail_start_index = tailNode-SPLIT_OFFSET+tail_position - arrayStartAddress;
        //sizeof(int) = 4
        std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(int32_t)); //link child to tail
        std::memcpy(&byteArray[mid_start_index], &tailNode, sizeof(int32_t)); //link tail to mid
        std::memcpy(&byteArray[lead_start_index], &midNode, sizeof(int32_t)); //link mid to lead
        return;
    }
    int mid_bits = splitNodeTailIndex(trans);
    int mid_position = mid_bits*4;
    int mid_start_index = midNode-SPLIT_OFFSET+mid_position - arrayStartAddress;
    uint32_t tailNode;
    std::memcpy(&tailNode, &byteArray[mid_start_index], sizeof(int32_t));
    if(tailNode == NULL)
    {
        printf("No entry for tail node\n");
        tailNode = createEmptySplitNode();
        int tail_bits = splitNodeChildIndex(trans);
        int tail_position = tail_bits*4;
        int tail_start_index = tailNode-SPLIT_OFFSET+tail_position - arrayStartAddress;
        std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(uint32_t)); //link child to tail
        std::memcpy(&byteArray[mid_start_index], &tailNode, sizeof(uint32_t)); //link tail to mid
        return;
    }
    printf("New child added to tail\n");
    int tail_bits = splitNodeChildIndex(trans);
    int tail_position = tail_bits*4;
    int tail_start_index = tailNode-SPLIT_OFFSET+tail_position - arrayStartAddress;
    std::memcpy(&byteArray[tail_start_index], &newChild, sizeof(uint32_t)); //link child to tail

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
//Sparse node:
//pointers
//characters
//orderword
int attachChildToSparse(int32_t node, uint8_t transitionByte, int newChild)
{
    int index;
    int smallerCount = 0;
    uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    // first check if this is an update and modify in-place if so
    for (index = 0; index < SPARSE_CHILD_COUNT; ++index)
    {
        int check_ptr_index = node - SPARSE_OFFSET + (index*4) - arrayStartAddress;
        uint32_t childptr;
        std::memcpy(&childptr, &byteArray[check_ptr_index], sizeof(uint32_t));
        if(childptr == NULL) break; // first free position -- not sure if this should be here or it should be in a separate for loop
        // check if the transition matches an existing transition
        // if so update child pointer
        
        int check_byte_index = node - SPARSE_CHILD_COUNT + index - arrayStartAddress;
        uint8_t existingByte;
        std::memcpy(&existingByte, &byteArray[check_byte_index], sizeof(uint8_t));
        if(transitionByte == existingByte)
        {
            std::memcpy(&byteArray[check_ptr_index], &newChild, sizeof(uint32_t));
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
                int byte_index = node - SPARSE_CHILD_COUNT + index - arrayStartAddress;
                std::memcpy(&byte, &byteArray[byte_index], sizeof(uint8_t));

                int32_t pointer;
                int ptr_index = node - SPARSE_OFFSET + index*4 - arrayStartAddress;
                std::memcpy(&pointer, &byteArray[ptr_index], sizeof(int32_t));

                attachChildToSplit(split,byte,pointer);
            }
            attachChildToSplit(split,transitionByte,newChild);
            return split;

    }

    //put Byte at node - sparse_children_count+childcount
    int byte_index = node - SPARSE_CHILD_COUNT + existingchildCount - arrayStartAddress;
    std::memcpy(&byteArray[byte_index], &transitionByte, sizeof(uint8_t));
    //Create updated Order Word
    short old_order;
    int order_idx = node-arrayStartAddress;
    std::memcpy(&old_order, &byteArray[order_idx], sizeof(short));
    short newOrder = insertInOrderWord(old_order, existingchildCount, smallerCount);

    //Write the address
    int ptr_index = node - SPARSE_OFFSET + existingchildCount*4 - arrayStartAddress;
    std::memcpy(&byteArray[ptr_index], &newChild, sizeof(int32_t));
    //Write the OrderWord at SPARSE_OFFSET
    std::memcpy(&byteArray[order_idx], &newOrder, sizeof(short));
    return node;

    
}
int attachChildToChain(int node, uint8_t transitionByte, int newChild) 
{
    int existingByte;
    uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);
    int existingByteIndex = node - arrayStartAddress;
    std::memcpy(&existingByte, &byteArray[existingByteIndex], sizeof(uint8_t));
    if (transitionByte == existingByte)
    {
        // This will only be called if new child is different from old, and the update is not on the final child
        // where we can change it in place (see attachChild). We must always create something new.
        // If the child is a chain, we can expand it (since it's a different value, its branch must be new and
        // nothing can already reside in the rest of the block).
        return expandOrCreateChainNode(transitionByte, newChild);
    }

    // The new transition is different, so we no longer have only one transition. Change type.
    int existingChild = node + 1;
    int existingChildIndex = existingChild - arrayStartAddress;
    if (offset(existingChild) == LAST_POINTER_OFFSET)
    {
        //existingChild = getInt(existingChild);
        //if at the last byte of chain node, pass the child pointer (e.g. leaf node ptr)
        //to the Sparse Node 
        std::memcpy(&existingChild, &byteArray[existingChildIndex], sizeof(int32_t));
    }
    return CreateSparseNode(existingByte, existingChild, transitionByte, newChild);
}

/**
 * Attach a child to the given non-content node. This may be an update for an existing branch, or a new child for
 * the node. An update _is_ required (i.e. this is only called when the newChild pointer is not the same as the
 * existing value).
 */
int attachChild(int node, uint8_t trans, int newChild)
{
    //assert !isLeaf(node) : "attachChild cannot be used on content nodes.";

    switch (offset(node))
    {
        //case PREFIX_OFFSET:
         //   assert false : "attachChild cannot be used on content nodes.";
        case SPARSE_OFFSET:
            return attachChildToSparse(node, trans, newChild);
        case SPLIT_OFFSET:
            attachChildToSplit(node, trans, newChild);
            return node;
        case LAST_POINTER_OFFSET - 1:
            // If this is the last character in a Chain block, we can modify the child in-place
            // if (trans == getUnsignedByte(node))
            // {
            //     putIntVolatile(node + 1, newChild);
            //     return node;
            // }
            // else pass through
        default:
            return attachChildToChain(node, trans, newChild);
    }
}

int getContentFromSplit(int split, uint8_t u8Value)
{
    int i_mid = splitNodeMidIndex(u8Value);
    int i_tail = splitNodeTailIndex(u8Value);
    int i_child = splitNodeChildIndex(u8Value);
    //uintptr_t childpointer = reinterpret_cast<uintptr_t>(&contentArray[0]);
    // int32_t childpointer = reinterpret_cast<int32_t>(childpointer);
    uint32_t contentStartPtr = reinterpret_cast<uintptr_t>(&contentArray[0]);

    uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(byteArray);

    int split_index_mid = split - arrayStartAddress - 12 + i_mid*4;
    uint32_t midNode;
    std::memcpy(&midNode, &byteArray[split_index_mid], sizeof(uint32_t));

    int split_index_tail = midNode - arrayStartAddress - SPLIT_OFFSET + i_tail*4;
    uint32_t tailNode;
    std::memcpy(&tailNode, &byteArray[split_index_tail], sizeof(uint32_t));

    int split_index_child = tailNode - arrayStartAddress - SPLIT_OFFSET + i_child*4;
    uint32_t childNode;
    std::memcpy(&childNode, &byteArray[split_index_child], sizeof(uint32_t));

    //uint32_t contentStartPtr = reinterpret_cast<uintptr_t>(&contentArray[0]);
    int child_index = (childNode - contentStartPtr)/sizeof(int32_t);
    printf("%d\n",child_index);
    printf("%d\n",contentArray[child_index]);
}



    /*testing chain node:
    contentArray[0]=12345;
    // Define a character
    char c = 'R'; // ASCII value of 'A' is 65

    // Cast the character to uint8_t
    uint8_t u8Value = static_cast<uint8_t>(c);
    // uintptr_t childpointer = reinterpret_cast<uintptr_t>(&contentArray[0]);
    // int32_t childpointer = reinterpret_cast<int32_t>(childpointer);
    uint32_t truncatedPointer = (reinterpret_cast<uintptr_t>(&contentArray[0]));

    uint32_t ChainNodeOffset = createNewChainNode(c,truncatedPointer);
    std::cout<<allocatedPos<<std::endl;
    std::cout<<static_cast<char>(byteArray[CHAIN_MAX_OFFSET])<<std::endl;
    std::cout<<sizeof(truncatedPointer)<<endl;
        for (size_t i = CHAIN_MAX_OFFSET+1; i < CHAIN_MAX_OFFSET+1+sizeof(truncatedPointer); ++i) {
        std::cout << static_cast<int>(byteArray[i]) << " ";
    }

        // Retrieve the integer from the array starting at index 27
    uint32_t retrievedPointer;
    std::memcpy(&retrievedPointer, &byteArray[CHAIN_MAX_OFFSET+1], sizeof(uint32_t));

    // Print the retrieved value to verify correctness
    std::cout << "\nRetrieved pointer: " << retrievedPointer << " "<<sizeof(uint32_t)<<std::endl;

    //uintptr_t is 8 bytes, we cast it to uint32_t which is of 4 bytes
    uint32_t arrayStartAddress = reinterpret_cast<uintptr_t>(contentArray);
    //uintptr_t elementAddress = reinterpret_cast<uintptr_t>(newNode);
    int index = retrievedPointer - arrayStartAddress; // index calculated from pointers as array holds contiguous memory addresses
    std::cout<<arrayStartAddress<<std::endl;
    std::cout<<index<<" "<<contentArray[index]<<std::endl;


    //uint32_t ChainNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[CHAIN_MAX_OFFSET]);
    std::cout<< "Offset: " <<offset(ChainNodeOffset)<<std::endl;
    uint32_t ChainNodeOffset2 = expandOrCreateNewChain(static_cast<uint8_t>('A'),ChainNodeOffset);
    
    //uint32_t ChainNodeOffset2 = reinterpret_cast<uintptr_t>(&byteArray[CHAIN_MAX_OFFSET-1]);
    expandOrCreateNewChain(static_cast<uint8_t>('C'),ChainNodeOffset2);

    std::cout<<static_cast<char>(byteArray[CHAIN_MAX_OFFSET-2])<<std::endl;
    std::cout<<static_cast<char>(byteArray[CHAIN_MAX_OFFSET-1])<<std::endl;
    std::cout<<static_cast<char>(byteArray[CHAIN_MAX_OFFSET])<<std::endl;

    std::cout<<sizeof((short) (1 * 6 + 0))<<std::endl;
    std::cout<<sizeof(int32_t)<<std::endl;
    std::cout<<sizeof(uint8_t)<<std::endl;*/



    //std::cout<<sizeof(int)<<std::endl;

    // uint32_t childpointer = reinterpret_cast<uintptr_t>(&byteArray[0]);
    // if(childpointer!=0)
    // {
    //     std::cout<<childpointer<<" "<<sizeof(childpointer)<<std::endl;
    // }



    //testing split node
    // contentArray[0]=12345;
    // contentArray[1]=9999;
    // contentArray[2]=1011347;
    // // Define a character
    // char c = 'A'; // ASCII value of 'A' is 65
    // uint8_t u8Value = static_cast<uint8_t>(c);
    // printBinary(u8Value);
    // int split = createEmptySplitNode();
    // int off = offset(split);
    // //printf("offset: %d\n",off);
    // uint32_t childPointer = reinterpret_cast<uintptr_t>(&contentArray[1]);
    // attachChildToSplit(split,u8Value,childPointer);
    // getContentFromSplit(split,u8Value);

    // c = 'Z'; // ASCII value of 'A' is 65
    // u8Value = static_cast<uint8_t>(c);
    // printBinary(u8Value);
    // childPointer = reinterpret_cast<uintptr_t>(&contentArray[2]);
    // attachChildToSplit(split,u8Value,childPointer);
    // getContentFromSplit(split,u8Value);
int main()
{

    //testing Create Sparse Node
    contentArray[0]=100;
    contentArray[1]=200;
    contentArray[2]=300;
    // Define a character
    char c = 'A'; // ASCII value of 'A' is 65
    uint8_t byte1 = static_cast<uint8_t>(c);
    uint32_t child1 = reinterpret_cast<uintptr_t>(&contentArray[1]);
    c = 'B'; // ASCII value of 'A' is 65
    uint8_t byte2 = static_cast<uint8_t>(c);
    uint32_t child2 = reinterpret_cast<uintptr_t>(&contentArray[2]);
    int sparse = CreateSparseNode(byte1, child1, byte2, child2);
    printSparseNode(sparse);


}
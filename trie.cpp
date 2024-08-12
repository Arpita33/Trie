#include <iostream>
#include <vector>
#include <cstdint> // For uint8_t
#include <cstring> // For memcpy
using namespace std;

int allocatedPos = 0;
int BLOCK_SIZE = 32;
int LAST_POINTER_OFFSET = BLOCK_SIZE - 4;
int CHAIN_MIN_OFFSET = 0;
int CHAIN_MAX_OFFSET = BLOCK_SIZE - 5; //27

int SPARSE_OFFSET = BLOCK_SIZE - 2; //=30
int SPLIT_OFFSET = BLOCK_SIZE - 4; //=28

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

int expandOrCreateNewChain(uint8_t transitionByte, int newChild)
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
        std::memcpy(&byteArray[v+8], &byte1, sizeof(uint8_t));
        std::memcpy(&byteArray[v+9], &byte2, sizeof(uint8_t));
        std::memcpy(&byteArray[pos], &orderWord, sizeof(short));



        uint32_t SparseNodeOffset = reinterpret_cast<uintptr_t>(&byteArray[pos]);
        return SparseNodeOffset; //return address of the Sparse node
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
    printBinary(trans);
    // first 2 bits of the 2-3-3 split
    int r = (trans >> 6) & 0x3;
    std:: cout<< r << std::endl;
    return (trans >> 6) & 0x3;
}

/**
 * Given a transition, returns the corresponding index (within the mid block) of the pointer to the tail block of
 * a split node.
 */
int splitNodeTailIndex(int trans)
{
    printBinary(trans);
    int r = (trans >> 3) & 0x7;
    std:: cout<< r << std::endl;
    // second 3 bits of the 2-3-3 split
    return (trans >> 3) & 0x7;
}

/**
 * Given a transition, returns the corresponding index (within the tail block) of the pointer to the child of
 * a split node.
 */
int splitNodeChildIndex(int trans)
{
    printBinary(trans);
    int r = (trans) & 0x7;
    std:: cout<< r << std::endl;
    // third 3 bits of the 2-3-3 split
    return trans & 0x7;
}

int createSplitNode()
{

}

int main()
{
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

    //testing split:
        // Define a character
    char c = '~'; // ASCII value of 'A' is 65

    // Cast the character to uint8_t
    uint8_t val = static_cast<uint8_t>(c);
    splitNodeMidIndex(255);
    splitNodeTailIndex(255);
    splitNodeChildIndex(255);
}
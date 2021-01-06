#include <cstdint>
#include <iostream>
//#include <bit> // this is only C++20
using namespace std;
/*
  Based on a concept suggested by Pridhvi: instead of hardcoding 3 levels of
  Trie, let's do one layer at a time. This way any letters that are popular
  enough can be split into another level. This saves bytes because every letter
  removed from a large hash table removes hundreds to thousands of occurrences.
  This way, we can keep the size of hash tables below 200-300 which should also
  reduce collisions.

  The penalty is that since it is slightly more complicated, the lookup is
  slightly slower. This may be somewhat compensated by the fact that the data
  should be smaller.

  Just implement the top nodes of a trie. Nodes have a binary flag and may
  either point to hash tables using an integer offset or another trie node. The
  top level has k choices (currently considering just 26) expressed as a single
  offset and a bitmap. The location of the next node is at offset+bit number

  There is a flag in each node determining whether it is a trie or hashmap node.

  Note: it is hell on earth creating this thing. You have to create the nodes
  with all children in parallel, not one at a time because the offset of a
  particular child depends on all the bits. But for a fixed dictionary, this
  could be very good.

  One way to make it better for dictionaries that do change is to preallocate
  hashmaps for combinations that might reasonably be expected but are not yet
  put into the dictionary. For example:

  qu is in the dictionary, but no other words are. But if anticipating other
  bigrams starting with q, put in all the children and have empty hash tables
  for those at a minimal cost. So for Arabic words like qaba this could be
  preallocated.

*/

class TrieHash2 {
 private:
  struct Node {
    uint8_t
        trieNode : 1;    // if true, this is a trie node, if false it is a hash
    uint8_t isWord : 1;  // if true, then the letters to get to this point
                         // constitute a valid isWord
    // note: it still might be worth handling all short words separately

    uint16_t offset;  // relative pointer to child node (either trienode or hash
                      // table)
    union {           // what, no anonymous unions in c++ anymore? WTF?
      uint32_t next;  // bit vector for up to 32 next letters ie a, b, c, ...
                      // position = offset + count of 1 bits in vector
    };
    Node(uint16_t offset) : trieNode(1), isWord(0), offset(offset), next(0) {}
    uint16_t add(uint8_t c) {  // trie method only
      if (c < 'a' || c > 'z') return 0;
      uint8_t delta = c - 'a';
      next |= (1 << delta);
      return offset +
             __builtin_popcount(
                 next &
                 (0xFFFFFFFF >> (32 - delta)));  // TODO: better way to do this?
    }
  };
  Node root;

 public:
  TrieHash2() : root(0) {}
};

int main() { TrieHash2 t; }

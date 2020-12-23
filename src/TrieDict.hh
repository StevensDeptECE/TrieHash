//#pragma once
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

class TrieHashDict
{
private:
//  static constexpr int scale = 8; // we are using 32-bit offsets in each hash table, offset by 16 bits within each word
  struct Info
  {
    uint32_t numWords;    // the number of words stored in text, and in the data structure
    uint32_t numHashMaps; // how many hashmaps are present, ie how many trigrams
    uint32_t nodeSize;    // the number of nodes used
    uint32_t textSize;    // the number of bytes of text needed to store
                          // note this is without the first 3 letters
  };
  Info info;
  Info *pInfo; // pointer that owns the memory
  char *text;  // the text of the hash maps in a single huge block.
  // No leading chars because the trie manages those
  // each word ends with the high bit set

  class HashMap;
  class HashMapNode;
  //TODO: how to handle 1 and 2 letter words
  HashMap *hashmaps;
  HashMapNode *nodes;
  int32_t lastHashMap;
  uint32_t startIndexOfCurrentHashMap;
  uint32_t wordsInCurrentHashMap;
  HashMapNode *temp;
  constexpr static uint32_t FIRST_3 = 26 * 26 * 26;
  uint32_t whichHash(const char w[]) {
    return ((w[0] - 'a') * 26 + (w[1] - 'a')) * 26 + w[2] - 'a';
  }
  uint32_t nodeCapacity;
public:
  TrieHashDict() {
		info.numHashMaps = FIRST_3;
		info.numWords = 213000;
		info.nodeSize = 0;
		info.nodeSize = 0;
    uint32_t textSize = info.numWords * 6;
    nodeCapacity = info.numWords * 2;
    uint32_t hashMapNodeSize = info.nodeSize * sizeof(HashMapNode);
    text = new char[sizeof(Info) + textSize + hashMapOffset +
                    nodeCapacity * sizeof(HashMapNode)];
    pInfo = (Info *)text;
    text += sizeof(Info);
    hashmaps = (HashMap *)(text + textSize); //TODO: call constructors
                                             //new HashMap[26*26*26];

    nodes = (HashMapNode *)(text + textSize + hashMapOffset); //new HashMapNode[nodeSize];
    temp = nodes + info.numWords;
    //TODO: initialize the arrays of hashmaps and hashmapnodes
    lastHashMap = -1;
    info.numWords = 1;
    info.numHashMaps = 0;
    info.nodeSize = 0;
    info.textSize = 1;
    startIndexOfCurrentHashMap = 0;
    wordsInCurrentHashMap = 0;
  }
  // fast load the TrieHashDict in binary
  TrieHashDict(const char filename[])
  {
    int fh = open(filename, O_RDONLY);
    struct stat st;
    fstat(fh, &st);
    uint32_t len = st.st_size;
    pInfo = (Info *)new char[len];
    int bytesRead = read(fh, (char *)pInfo, len);
    if (bytesRead < len)
    {
      throw "Could not read entire file";
    }
    info = *pInfo;
    text = (char *)(pInfo + 1);
    hashmaps = (HashMap *)(text + info.textSize);
    nodes = (HashMapNode *)(text + info.textSize + hashMapOffset);
  }
  ~TrieHashDict()
  {
    delete[] pInfo;
  }
  TrieHashDict(const TrieHashDict &orig) = delete;
  TrieHashDict &operator=(const TrieHashDict &orig) = delete;

  void save(const char filename[])
  {
    int fh = open(filename, O_WRONLY);
    uint32_t len = sizeof(Info) + info.textSize +
                   hashMapOffset +
                   info.nodeSize * sizeof(HashMapNode);
    //TODO: Make sure the bytes are right. currently preallocated capacity
    // crunch the data structure before writing
    write(fh, pInfo, len);
    close(fh);
  }
  void checkGrow(uint32_t requested)
  {
    // compare info.nodeSize > nodeCapacity
    //TODO: CHeck if we need to get more Nodes
  }
  void add(const char word[], uint32_t len)
  {
      if (len <= 2){
        return; // go into trie and set flag
      }
      if (word[0] < 'a' || word[0] > 'z' ||
          word[1] < 'a' || word[1] > 'z' ||
          word[2] < 'a' || word[2] > 'z')
      {
        throw "bad char";
      }
    // ax^2 + bx + c   a*x*x + b*x + c  HORNER's FORM = (a*x+b)*x + c
    int which = whichHash(word);
    if (which != lastHashMap)
    {
      lastHashMap = which;
      wordsInCurrentHashMap = 0;
      startIndexOfCurrentHashMap = info.nodeSize;

      hashmaps[which].base = info.textSize-1;
      hashmaps[which].baseid = info.nodeSize;
      hashmaps[which].size = 1; // power of 2 -1
			info.nodeSize = hashmaps[which].baseid + hashmaps[which].size+1;
    }
    hashmaps[which].add(*this, word + 3, len - 3);
  }

  bool get(const char word[], uint32_t len, uint32_t &id)
  {
    int which = whichHash(word);
    return hashmaps[which].get(word + 3, len - 3, id);
  }

  uint32_t *get(const char word[], uint32_t len)
  {
    int which = whichHash(word);
    return nullptr; // TODO: IMPLEMENT
  }

private:
  void addWord(uint32_t base, uint32_t baseId, uint32_t hashVal,
               const char letters[], uint32_t len)
  {
    nodes[hashVal].offset = info.textSize - base; // offset to word in text;
    nodes[hashVal].relid = info.nodeSize - baseId;

    for (int i = 0; i < len; i++)
      text[info.textSize++] = letters[i];
    text[info.textSize++] = '\0'; // TODO: get rid of this
    //info.nodeSize++;              //TODO: are nodeSize and numWords the same?
    info.numWords++;
    wordsInCurrentHashMap++;
  }

  class HashMapNode
  {
  public:
    // 0 = null
    uint16_t offset; // where the text is relative to the base for this hash map
    uint16_t relid;  // id number relative to baseid in hashmap
  };
  class HashMap
  {
  public:
    uint32_t base;   // offset into giant string of all words,
                     //each node offset relative to this
    uint32_t baseid; // all ids in this hash map are relative to this number
    uint16_t size;   // size of the table
    HashMap() : base(0), baseid(0), size(64) {}
    HashMap(uint16_t base, uint32_t baseid, uint32_t size)
        : base(base), baseid(baseid), size(size)
    {
    }
    void grow(TrieHashDict &t)
    {
      t.checkGrow(size);
      HashMapNode *temp = t.temp;
      uint32_t oldSize = size;
      size = ((size + 1) << 1) - 1; // 2 to n - 1

    	uint32_t activeNodes = 0;
      for (uint32_t i = t.startIndexOfCurrentHashMap; i <= t.startIndexOfCurrentHashMap + oldSize; i++)
      {
				if (t.nodes[i].offset != 0) {
	        temp[activeNodes++] = t.nodes[i];  // copy each node for safekeeping
  	      t.nodes[i].offset = 0; // zero the offset so each looks empty
				}
      }

      // reinsert each node into the double-sized hashmap
      for (uint32_t i = 0; i < activeNodes; i++)
      {
        const char *p = t.text + base + temp[i].offset;
        uint32_t len = strlen(p);  // TODO: inefficient
        uint32_t h = baseid + hash(p, len); // calculate new hash location
        while (t.nodes[h].offset != 0)
        {
          h++;                       // linear probe until collision resolved
          if (h > base + size) // if end of table, jump back to start
            h = base;
        }
        t.nodes[h].offset = temp[i].offset; // reinsert node in new location
        t.nodes[h].relid = temp[i].relid;
      }
			t.info.nodeSize = baseid + size + 1;
    }
    void add(TrieHashDict &t, const char word[], uint32_t len)
    {
      uint32_t h = baseid + hash(word, len);
      while (t.nodes[h].offset != 0)
      {
        h++;
        if (h > baseid + size)
          h = baseid;
      }
      t.addWord(base, baseid, h, word, len);
      if (t.wordsInCurrentHashMap * 2 > (size+1))
      {
        grow(t);
      }

      //  0             256             512             792
      //  hm1 64
    }
    bool get(const char word[], uint32_t len, uint32_t &id)
    {
      uint32_t h = hash(word, len); // linear probing. should be 50% empties
      return false;                 //TODO: complete
    }

    uint32_t *get(const char word[], uint32_t len)
    {
      return nullptr; //TODO: complete
    }

    // abc != cba   abc != bbb
    uint32_t hash(const char letters[], uint32_t len)
    {
      if (len == 0)
        return 0;
      uint32_t sum = len;
      for (len--; len > 0; len--)
        sum = ((sum << 11) | (sum >> 21)) ^
              ((sum << 7) | (sum >> 25)) + letters[len];
      sum = ((sum << 11) | (sum >> 21)) ^
            ((sum << 7) | (sum >> 25)) + letters[0];
      return sum & size; // size must be power of 2 - 1
    }
  };
  constexpr static uint32_t hashMapOffset = FIRST_3 * sizeof(HashMap);
};

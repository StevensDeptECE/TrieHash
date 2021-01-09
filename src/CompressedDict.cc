/*
  A compressed Dictionary stores words to be used in a document or
documents. For a universal dictionary, compression isn't as important because it
is amortized across many documents, but all the words not found in this
        dictionary must be defined in each document.

        For a document composed solely of words in a dictionary, compression can
be very high. The larger the word set, the more bits in each word, but given
        that there are far fewer legal words than sequences of letters, the
        compression is high. Also, parametric compression lets us compress
common sequences of common words, even when containing uncommon words.

        However, most documents contain words that are not stored in any
dictionary. These may be neologisms, typos, names that are uncommon and hence
not deemed worthy. For efficient representation of documents, it is necessary to
encode these words as well.

        Compressing these is difficult. If they are stored as individual
        bytes and built dynamically as in lzw, then the first time they are
        used, they cannot be compressed much. And switching between looking
        up words and embedding individual letters requires escape codes
        which are also hugely wasteful of space.

        Instead, a compression mechanism for English to be discussed in the
future requires a dictionary to be defined at the top of the document containing
all words in the document that are not in the main dictionary. Because the
dictionary can be stored in sorted order, it can be more efficiently encoded.
All letters starting with a for example, can be stored without the a.

        Compressed format:
        startchar endchar bitvector of which characters are used in the
dictionary then for each character in this top list, bit for isWord (if true,
then it is a word) a 16-bit offset for where the children of this node start,
and a bitvector of letters used.

        There should be more than one bit vector format.  For English we
        need 26 letters. But for byte sequences, as well as other languages
        there could be a part of the dictionary that includes a wider
        character set. Just because there is does not mean that every node
        should include this larger bit vector.

        Example of compressed format:
Full dictionary
  az 11111111111111111111111111   a-z are all present
  1 1 11111111111111111111111111  there are words aa, ab, ac... az
        1 0 10011111111111111111111110  there are words ba, NOT bb, NOT bc, ...
...
  0 1 12                          aa is a word,  the 7-bit number following
indicates there are 12 words

0 1 100   instead of a node with 100 words, replace by:
1 1 10000010000000000011000000 	(28 bits)
0 1 25
0 1 26
0 1 24
0 1 25


with nodesize = 7 bits, max = 128
split to 6 bits max = 64, each node split costs 8 bytes overhead
split to 5 bits max = 32


        words are inserted using arithmetic encoding
        example: aa, aal, aalii, aam, aani, aardvark, aardwolf, aaronic,
aaronical, aaronite, aaronitic, aaru

        l END lii END m END ni END rdvark END rdwolf END ronic END ronical END
ronite END ronitic END ru END

bdeh END bua END c END ca END cate END cay END cinate END cination END

        57 tokens = 57/13 = 4+1 = 5*8 = 40 bytes




        suppose a document has the following words not in the dictionary:
        argentia desmos firas futa fuzzball
        af 100101   (dictionary a-f, only  adf used)
        0 00000000000000000100000000   (a is not a word, that's in the main
dictionary, only next letter is r) 0 00001000000000000000000000   (d is not a
word, next letter is e) 0 00000000100000000000100000   (f is not a word, next
letters i and u)

  0 0 gentia                     (0=ar is not a word, 0=trie node
        1= leaf

        Letters use arithmetic encoding. There are 26 codes for letters of
        the English alphabet plus two additional tokens 26 and 27 mean end
        of word, and the different tokens denote two different pools of
        words.
        13 letters and tokens fit in each 64 bit word with just over 1 bit to
spare for future expansion
*/

#include <cstring>
#include <fstream>
#include <iostream>
//#include <string>
#include <vector>

#include "Bitstream.hh"
using namespace std;

class CompressedDict1 {
 private:
  /*
    TODO: for convenience we need plenty of state in here while recursively
    creating the dictionary. This class is needed while compressing, but the
    dictionary retained in memory should not have this state.
  */
  uint64_t *bitMem;
  BitIterator bits;
  const char *dict;
  uint32_t dictLen;
  vector<uint64_t> compressedWords;
  uint64_t current;
  uint64_t power;

  static constexpr uint32_t hashSizeBits = 7;
  static constexpr uint32_t maxNodeSize = 1 << hashSizeBits;
  static constexpr uint64_t base = 27;
  static constexpr uint64_t base2 = base * base;
  static constexpr uint64_t base4 = base2 * base2;
  static constexpr uint64_t base8 = base4 * base4;

  static constexpr uint64_t baseto13 = base8 * base4 * base;
  static constexpr uint8_t END = base - 1;
  static constexpr uint8_t END2 = base - 2;
  /*
1* ('a'-'a') = 0
   27*('b'-'a') = 27
   27*27 *('d'-'a') = 3*27*27=2187

*/
  inline void writeOneChar(uint8_t code) {
    if (power < baseto13) {
      current += code * power;
      power *= base;
    } else {
      compressedWords.push_back(current);
      power = 1;
      current = code;
    }
  }

  void writeOneWord(uint32_t &dictIndex, uint32_t prefixLen) {
    for (uint32_t i = 0; i < prefixLen; i++)
      if (dict[dictIndex] < 'a' || dict[dictIndex] > 'z') {
        cerr << "Error: prefix letters not within alphabet" << endl;
        return;
      }
    dictIndex += prefixLen;
    uint8_t c;
    while ((c = dict[dictIndex++]) >= 'a' && c <= 'z') {
      // write each letter of the word in base 27 or 28, 13 characters per 64
      // bit word
      writeOneChar(c - 'a');
    }
    writeOneChar(END);  // end the word with a special token
    while (dictIndex < dictLen &&
           dict[dictIndex] <= ' ')  // skip whitespace until next word
      dictIndex++;
  }
  inline bool comparePrefix(const char prefix[], uint32_t prefixLen,
                            uint32_t dictIndex) {
    for (uint32_t k = 0; k < prefixLen; k++) {
      if (prefix[k] != dict[dictIndex + k]) return false;
    }
    return true;
  }

  inline uint32_t countThisPrefix(char prefix[], uint32_t prefixLen,
                                  uint32_t dictIndex) {
    uint32_t countWordsThisPrefix = 0;
    for (uint32_t j = dictIndex; j < dictLen;) {
      if (comparePrefix(prefix, prefixLen, j)) {
        countWordsThisPrefix++;
        j += prefixLen;
        if (countWordsThisPrefix > maxNodeSize)
          return countWordsThisPrefix;  // too big, so stop counting so the
                                        // caller can split and recurse
        else
          ;
        // skip to next word
        while (j < dictLen && dict[j] >= 'a' && dict[j] <= 'z')
          j++;  // skip remaining letters in word
        while (j < dictLen && dict[j] <= ' ')
          j++;  // skip whitespace after word
      } else {
        break;
      }
    }
    return countWordsThisPrefix;
  }

 public:
  /*
search through the file for all words starting with prefix[] with prefixLen
characters. if the number of words <= maxNodeSize end recursion by writing out
the hash node and problem: in order to write this node, we need to know how many
children there are. So we will have to search once to get all the letters under
this one, and then recurse The other alternative (faster) is to allocate the
space for the bits of the trie node, not knowing what they are, recurse, return
the bits and then go back and write them.
*/
  CompressedDict1(const char filename[])
      : bitMem(new uint64_t[3000]), bits(bitMem, 0) {
    ifstream f(filename);
    f.seekg(0, std::ios::end);  // go to the end
    dictLen = f.tellg();
    compressedWords.reserve(dictLen / 13 + 2);
    compressedWords.push_back(0);
    f.seekg(0, std::ios::beg);  // go back to the beginning
    dict = new char[dictLen];
    f.read((char *)dict, dictLen);  // read the whole file into the buffer
    /*
current prefix, should only need about 4-6 of these characters.
64 is overkill. At each point count how many words begin with this string
if too many (>k, perhaps k = 100 or 150) then split the group with
another level of trie.
For our sample dictionary of 213k words, 2000 prefixes means no single
group requires > 107 words, keeping each section small and removing
a maximal number of letters from the front of the dictionary.
*/
    char prefix[16] = {0};
    uint32_t countWordsThisPrefix = 0;
    uint32_t prefixLen = 1;
    // start with 1 letter prefixes and recurse every time any node has too
    // many (words > 128)
    uint32_t dictIndex = 0;
    // at the start of the dictionary, first skip potential spaces...

    for (char first = 'a'; first <= 'z'; first++) {
      prefix[0] = first;
      uint32_t index = recursiveFindPrefix(prefix, 1, dictIndex);
    }
    delete[] dict;
  }

  uint8_t recursiveFindPrefix(char prefix[], uint32_t prefixLen,
                              uint32_t &dictIndex) {
    // find each letter after the current prefix, ie for a --> aa, ab, ac,
    // ad ...
    // TODO: load prefix
    uint32_t count;
    uint32_t childBits = 0;
    if ((count = countThisPrefix(prefix, prefixLen, dictIndex)) > maxNodeSize) {
      // too many, split up by considering one more letter prefix
      BitIterator parentBits = bits;
      uint32_t isWord = uint32_t(dict[dictIndex + prefixLen] < ' ');
      bits.orBits((1 << 27) | (isWord << 26), 28);

      // for each letter under this prefix, aa, ab, ac, ...
      for (uint32_t i = dictIndex; i < dictLen;) {
        // make the new prefix the one we are looking for
        // the last character in the word: (check if wordLen == prefixLen)
        if (!comparePrefix(prefix, prefixLen, i)) {
          dictIndex = i;
          return 0;
        }
        if (dict[i + prefixLen] <= ' ') {
          i += prefixLen;
          while (dict[i] >= 'a' && dict[i] <= 'z') i++;
          while (dict[i] <= ' ') i++;
        }

        prefix[prefixLen] = dict[i + prefixLen];
        uint8_t childChar = recursiveFindPrefix(prefix, prefixLen + 1, i);
        if (prefix[0] == 'z' && prefix[1] == 'y' && prefix[2] == 'z') return 0;
        if (childChar != 0) childBits |= (1 << (childChar - 'a'));

        // the i= in for loop skips forward past this prefix to the next one
      }
      parentBits.orBits(childBits, 26);
      return prefix[prefixLen - 1];
    } else {
      uint32_t isWord = uint32_t(dict[dictIndex + prefixLen] < ' ');
      // encode this hash node 0 isWord count count=7 bits at the moment
      bits.orBits((isWord << hashSizeBits) | count, hashSizeBits + 2);
      // append all words to the list of arithmetic-encoded without the prefix
      current = 0;
      power = 1;
      // uint32_t childBits = 0;
      for (int k = 0; k < prefixLen; k++) cerr << prefix[k];
      cerr << "\t" << count << endl;
      for (uint32_t j = 0; j < count; j++) {
        // childBits |= 1 << (dict[dictIndex+prefixLen]);
        writeOneWord(dictIndex, prefixLen);
      }
      return prefix[prefixLen - 1];
    }
  }

  void writeCompressed(const char filename[]) {
    ofstream bin(filename, ios::binary);
    bin.write((char *)bitMem, (bits.endPointer() - bitMem) * sizeof(uint64_t));

    bin.write((char *)&compressedWords[0],
              compressedWords.size() * sizeof(uint64_t));
  }
#if 0
  // read the compressed words back from a binary file
  void readCompressed(ifstream &in) {
    uint64_t buffer[256];
    uint32_t len = 0;
    while (in.read((char *)buffer, sizeof(buffer))) {
      //			cout << "len=" << len;
      for (uint32_t i = 0; i < 256; i++) {
        uint64_t current = buffer[i];
        for (uint32_t j = 0; j < 13; j++) {
          uint8_t c = current % base;
          cout << (c < (base - 1) ? (char)(c + 'a') : ' ');
          current /= base;
        }
      }
    }
  }
#endif
};

int main() {
  CompressedDict1 dict("../dict.txt");
  dict.writeCompressed("dict.bin");
#if 0
	{
    ifstream bin("dict.bin");
    CompressedDict1 dict2;
    dict2.readCompressed(bin);
  }
#endif
}

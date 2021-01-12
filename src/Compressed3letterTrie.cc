/*
 Compressed format:
First 3 letters index into list of hash maps
aaa --> bin 1, list how many words in this hash map
aab --> bin 2, list number of words...
zzz --> bin 17500

approximately 17500 bins total. The problem is most have size 0, some have size
3000. numbers are 12 bits, so the header is 17500 * 1.5 bytes

This is wasteful but at least is simple.
We store the words in a single list arithmetically encoded with base 27
code 26 means end of word. 13 words fit into a single 64-bit word. The first 3
letters are removed. Only words longer than 2 letters are stored.

This is not a "real" dictionary format but is being implemented quickly just to
compare to the better one in CompressedDict.cc

        words are inserted using arithmetic encoding
        example: aa, aal, aalii, aam, aani, aardvark, aardwolf, aaronic,
aaronical, aaronite, aaronitic, aaru

        l END lii END m END ni END rdvark END rdwolf END ronic END ronical END
ronite END ronitic END ru END

bdeh END bua END c END ca END cate END cay END cinate END cination END

        57 tokens = 57/13 = 4+1 = 5*8 = 40 bytes
*/

#include <cstring>
#include <fstream>
#include <iostream>
//#include <string>
#include <vector>

#include "Bitstream.hh"
using namespace std;

class CompressedDict {
 private:
  uint64_t *bitMem;
  BitIterator bits;
  const char *dict;
  uint32_t dictLen;
  vector<uint64_t> compressedWords;
  uint64_t current;
  uint64_t power;

  static constexpr uint32_t hashSizeBits = 12;
  static constexpr uint32_t maxNodeSize = 1 << hashSizeBits;
  static constexpr uint64_t base = 27;
  static constexpr uint64_t base2 = base * base;
  static constexpr uint64_t base4 = base2 * base2;
  static constexpr uint64_t base8 = base4 * base4;

  static constexpr uint64_t baseto13 = base8 * base4 * base;
  static constexpr uint8_t END = base - 1;
  static constexpr uint8_t END2 = base - 2;

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

 public:
  CompressedDict(const char filename[])
      : bitMem(new uint64_t[3000]), bits(bitMem, 0) {
    ifstream f(filename);
    f.seekg(0, std::ios::end);  // go to the end
    dictLen = f.tellg();
    compressedWords.reserve(dictLen / 13 + 2);
    // compressedWords.push_back(0);
    f.seekg(0, std::ios::beg);  // go back to the beginning
    dict = new char[dictLen];
    f.read((char *)dict, dictLen);  // read the whole file into the buffer
    char prefix[3] = {0};
    for (uint32_t i = 0; i < dictLen;) {
      if (!islower(dict[i]) || !islower(dict[i + 1]) || !islower(dict[i + 2])) {
        while (islower(dict[i]))  // skip remainder of short word
          i++;
        while (dict[i] <= ' ')  // skip space
          i++;
        continue;
      }
    }
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

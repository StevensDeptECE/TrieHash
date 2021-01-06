#if 0
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
#include <string>
#include <vector>
using namespace std;

class CompressedDict1 {
 private:
  string words;
  vector<uint32_t> offsets;

  /*
     1* ('a'-'a') = 0
           27*('b'-'a') = 27
           27*27 *('d'-'a') = 3*27*27=2187


  */
  void writeOneWord(ofstream& out, uint64_t& current, uint64_t& power,
                    uint64_t buffer[256], uint32_t& bufIndex, uint8_t code) {
    if (power < 205891132094649ULL * 27 * 27 * 27) {  // 27**13 fits in one word
      current += code * power;
      power *= 27;
    } else {
      buffer[bufIndex++] = current;
      power = 1;
      current = code;
      if (bufIndex >= 256) {
        out.write((char*)buffer, 1024);
        bufIndex = 0;
      }
    }
  }

 public:
  CompressedDict1(uint32_t estCharCapacity, uint32_t estWordCapacity) {
    words.reserve(estCharCapacity);
    offsets.reserve(estWordCapacity);
  }

  bool comparePrefix(const char prefix[], const char* dict, uint32_t prefixLen,
                     uint32_t dictIndex) {
    for (uint32_t k = 0; k < prefixLen; k++) {
      if (prefix[k] != dict[dictIndex + k]) return false;
      return true;
    }
    constexpr maxNodeSize = 128;

    uint32_t countThisPrefix(char prefix[], uint32_t prefixLen) {
      for (uint32_t j = i; j <= len; j++) {
        if (comparePrefix(prefix, p + j, prefixLen)) {
          countWordsThisPrefix++;
          if (countWordsThisPrefix > maxNodeSize)
            return countWordsThisPrefix;  // too big, so stop counting and
                                          // recurse
          else
            break;
          // skip to next word
          while (j <= len && p[j] >= 'a' && p[j] <= 'z')
            j++;  // skip remaining letters in word
          while (j <= len && p[j] <= ' ') j++;  // skip whitespace after word
        }
        int TODO = 0;
        return TODO;
      }

      void recursiveFindPrefix(char prefix[], uint32_t prefixLen,
                               const char dict[], uint32_t dictLen,
                               uint32_t dictIndex) {
        // find each letter after the current prefix, ie for a --> aa, ab, ac,
        // ad ...
        if (countThisPrefix(prefix, prefixLen, dict, dictLen, dictIndex) >
            maxNodeSize) {
          for (uint32_t i = dictIndex; i < dictLen;) {
            if (comparePrefix(prefix, dict, prefixLen, i)) {
              prefix[prefixLen] = dict[i + prefixLen];
              i = recursiveFindPrefix(prefix, prefixLen + 1, dict, dictLen,
                                      dictIndex);
              // findNextLetter(dict[i+prefixLen])
            }
          }
          else {
            prefix[prefixLen] = pref;
            // encode this hash node
            // write out in 9 bits: 0 0 numberofwords
            // append to arithmetic-encoded word list all words without the
            // prefix
          }
        }

        void loadASCII(const char filename[]) {
          ifstream dict(filename);
          dict.seekg(0, std::ios::end);  // go to the end
          uint32_t len = dict.tellg();
          dict.seekg(0, std::ios::beg);  // go back to the beginning
          const char* p = new char[len];
          dict.read(p, len);  // read the whole file into the buffer
          /*
                  current prefix, should only need about 4-6 of these
             characters. 64 is overkill. At each point count how many words
             begin with this string if too many (>k, perhaps k = 100 or 150)
             then split the group with another level of trie. For our sample
             dictionary of 213k words, 2000 prefixes means no single group
             requires > 107 words, keeping each section small and removing a
             maximal number of letters from the front of the dictionary.
          */
          char prefix[16] = {0};
          uint32_t countWordsThisPrefix = 0;
          uint32_t prefixLen = 2;
          vector<uint64_t> compressedSuffixes;
          // start with 2 letter prefixes because we know that every starting
          // letter is in the dictionary
          uint32_t i = 0;
          // at the start of the dictionary, first skip potential spaces...

          for (char pref = 'a'; pref <= 'z'; pref++) {
            prefix[0] = pref;
            recursiveFindPrefix(prefix, 1, p, len, i);

            do {
            skipspace:
              while (p[i] <= ' ')
                i++;  // skip spaces or control characters to get to next word
              for (int j = 0; j < prefixLen; j++) {
                if (p[i] <=
                    ' ') {  // this word isn't long enough for current prefix
                  cout << "prefix error! ";
                  for (int k = i - j; p[k] >= ' '; k++) cout << p[k];
                  cout << '\n';
                  goto skipspace;
                }
                prefix[i + j] = p[i++];
              }
              countWordsThisPrefix = 1;

              // now, count how many of this prefix are present.
              // if > k, then create a trie node, split and repeat for each
              // child
              uint32_t bitmask =
                  0;  // track how many letters are used after prefix

              for (uint32_t j = i; j <= len; j++) {
                if (comparePrefix(prefix, p + j, prefixLen))
                  countWordsThisPrefix++;
                else
                  break;
                // skip to next word
                while (j <= len && p[j] >= 'a' && p[j] <= 'z')
                  j++;  // skip remaining letters in word
                while (j <= len && p[j] <= ' ')
                  j++;  // skip whitespace after word
              }
              // by now we have counted how many words start with this prefix,
              // ie aba
              if (countWordsThisPrefix > maximumWords) {
                prefixLen++;
              }

              for (uint32_t k = 0; k < prefixLen; k++) {
                if (prefix[k] != p[j + k]) uint8_t nextLetter = p[j];

                ? ? ?

                    if (nextLetter >= 'a' && nextLetter <= 'z') {
                  bitmask |=
                      (1 << (nextLetter - 'a'));  // set bit as having been seen
                  while (j < len && p[j] >= ' ') j++;  // skip to end of word
                  while (j < len && p[j] <= ' ') j++;  // skipspaces
                  for (int k = 0; k < prefixLen; k++)
                    if (p[j + k] != prefix[k]) {
                      cout << "new prefix!\n";
                      endPrefix = j;
                      goto endPrefix;
                    }
                }
              }
            endPrefix:
              if (countWordsThisPrefix < desiredCap) {
                // now write out words for this prefix stripped of prefix
                // in arithmetic encoding

                uint64_t power = 1;
                uint64_t current = 0;
                uint32_t bufIndex = 0;
                for (int j = i + prefixLen; j < endPrefix; j++) {
                  writeOneWord(current, power, buffer, bufIndex,
                               words[j] - 'a');
                }
                // now write the code indicating termination of word
                writeOneWord(out, current, power, buffer, bufIndex, 26);
              }
            }
            else {
              // add a new level of trie with one more letter of prefix, repeat
              // for all letters under this prefix
            }
          }

          delete[] p;
        }
        void add(const char word[], uint32_t len) {
          offsets.push_back(words.length());
          for (int i = 0; i < len; i++) words += word[i];
        }
        // write the compressed words to a binary file
        void writeCompressed(ofstream & out) {
          uint64_t power = 1;
          uint64_t current = 0;
          uint64_t buffer[256];
          uint32_t bufIndex = 0;
          for (uint32_t i = 0; i < offsets.size() - 1; i++) {
            for (uint32_t j = offsets[i]; j < offsets[i + 1]; j++) {
              writeOneWord(out, current, power, buffer, bufIndex,
                           words[j] - 'a');
            }
            // now write the code indicating termination of word
            writeOneWord(out, current, power, buffer, bufIndex, 26);
          }
          // flush out the last part of the buffer at the end
          out.write((char*)buffer, bufIndex * sizeof(uint64_t));
        }
        // read the compressed words back from a binary file
        void readCompressed(ifstream & in) {
          uint64_t buffer[256];
          uint32_t len = 0;
          while (in.read((char*)buffer, sizeof(buffer))) {
            //			cout << "len=" << len;
            for (uint32_t i = 0; i < 256; i++) {
              uint64_t current = buffer[i];
              for (uint32_t j = 0; j < 13; j++) {
                uint8_t c = current % 27;
                cout << (c < 26 ? (char)(c + 'a') : ' ');
                current /= 27;
              }
            }
          }
        }
      };

      class CompressedDict2 {
       private:
        uint8_t startChar, endChar;
        // bitlist usedChars; // filter out characters in the range?
      };

      int main() {
        CompressedDict1 dict(3000000, 220000);  // preallocate enough room
        ifstream f("dict.txt");
        char word[256];
        while (f.getline(word, sizeof(word))) {
          dict.add(word, strlen(word) - 1);
        }
        {
          ofstream bin("en_ae.bin");
          dict.writeCompressed(bin);
        }
        {
          ifstream bin("en_ae.bin");
          CompressedDict1 dict2(3000000, 220000);  // preallocate enough room
          dict2.readCompressed(bin);
        }
      }
#endif

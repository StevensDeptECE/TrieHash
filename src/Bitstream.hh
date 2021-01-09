#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>

class Bitstream {
 private:
  uint64_t *bits;     // base pointer to buffer
  uint64_t *current;  // cursor, the word currently being written
  uint32_t bitpos;    // bit position within the current word
 public:
  Bitstream(uint64_t *bits) : bits(bits), current(bits), bitpos(0) {
    bits[0] = 0;
  }

  uint32_t length() const { return current - bits; }

  /* write a bit value into the stream
   */
  void write(uint64_t v, uint32_t len) {
    uint32_t remaining = 64 - bitpos;  // bits remaining in current word
    if (len < remaining) {
      *current |= (v << bitpos);
      bitpos += len;
    } else {
      *current++ |= (v << bitpos);
      *current = v >> remaining;
      bitpos = len - remaining;
    }
  }
  void write(uint32_t a, uint32_t abits, uint32_t b, uint32_t bbits, uint32_t c,
             uint32_t cbits) {
    write((((a << bbits) | b) << cbits) | c, abits + bbits + cbits);
  }
  /*
          read len bits out of the current stream and return the value
  */
  uint64_t read(uint32_t len) {
    uint32_t remaining = 64 - bitpos;
    if (remaining >= len) {
      uint64_t v = (*current >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64 - len));
      bitpos += len;
      return v;
    } else {
      uint64_t v =
          (*current++ >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64 - len));
      bitpos = len - remaining;
      v |= *current & (0xFFFFFFFFFFFFFFFFULL >> (64 - bitpos));
      return v;
    }
  }
  friend std::ostream &operator<<(std::ostream &s, const Bitstream &b) {
    s << std::hex;
    for (uint32_t i = 0; i < b.length(); i++) s << b.bits[i] << ' ';
    return s;
  }
};
class BitIterator {
 private:
  uint64_t *word;   // location of the word containing the bits
  uint32_t bitpos;  // bit position within the word
 public:
  BitIterator(uint64_t *word, uint32_t bitpos) : word(word), bitpos(bitpos) {}
  /*
                   write a bit value at a particular location without disturbing
     other bits already there
          */
  void orBits(uint64_t v, uint32_t len) {
    uint32_t remaining = 64 - bitpos;  // bits remaining in current word
    if (len < remaining) {
      *word |= (v << bitpos);
      bitpos += len;
    } else {
      *word++ |= (v << bitpos);
      *word = v >> remaining;
      bitpos = len - remaining;
    }
  }

  /*
          First zero out bits, then or in
  */
  void replace(uint64_t v, uint32_t len) {
    uint32_t remaining = 64 - bitpos;  // bits remaining in current word
    uint64_t mask = 0xFFFFFFFFFFFFFFFFULL >> (64 - len);
    if (len < remaining) {
      *word = *word & (mask << bitpos) | (v << bitpos);
      bitpos += len;
    } else {
      *word = *word & (mask << bitpos) | (v << bitpos);
      ++word;
      *word = *word & (0xFFFFFFFFFFFFFFFFULL >> remaining) | (v >> remaining);
      bitpos = len - remaining;
    }
  }
  void operator-=(uint32_t pos) {
#if 0
		if (bitpos > 64) {
			word -= bitpos / 64;
			bitpos = bitpos & 63;
		}
#endif
    if (bitpos > pos) {
      bitpos -= pos;
    } else {
      word--;
      bitpos = 64 - (pos - bitpos);
    }
  }
  /*
          read len bits out of the current stream and return the value
  */
  uint64_t read(uint32_t len) {
    uint32_t remaining = 64 - bitpos;
    if (remaining >= len) {
      uint64_t v = (*word >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64 - len));
      bitpos += len;
      return v;
    } else {
      uint64_t v = (*word++ >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64 - len));
      bitpos = len - remaining;
      v |= *word & (0xFFFFFFFFFFFFFFFFULL >> (64 - bitpos));
      return v;
    }
  }
  const uint64_t *endPointer() const {
    if (bitpos == 0) return word;
    return word + 1;
  }
};

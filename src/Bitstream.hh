#include <iostream>
#include <iomanip>
#include <cstdint>

class Bitstream {
private:
	uint64_t* bits; // base pointer to buffer
	uint64_t* current; // cursor, the word currently being written
	uint32_t bitpos; // bit position within the current word
public:
	Bitstream(uint64_t* bits) :
		bits(bits), current(bits), bitpos(0) {
		bits[0] = 0;
	}

	uint32_t length() const { return current - bits; }
	
	/* write a bit value into the stream
	 */
	void write(uint64_t v, uint32_t len) {
		uint32_t remaining = 64-bitpos; // bits remaining in current word
		if (len < remaining) {
			*current |= (v << bitpos);
			bitpos += len;
		} else {
			*current++ |= (v << bitpos);
			*current = v >> remaining;
			bitpos = len - remaining;
		}
	}
	void write(uint32_t a, uint32_t abits,
						 uint32_t b, uint32_t bbits,
						 uint32_t c, uint32_t cbits) {
		write( (((a << bbits) | b) << cbits) | c, abits+bbits+cbits);
	}
	/*
		read len bits out of the current stream and return the value
	*/
	uint64_t read(uint32_t len) {
		uint32_t remaining = 64 - bitpos;
		if (remaining >= len) {
			uint64_t v = (*current >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64-len));
			bitpos += len;
			return v;
		} else {
			uint64_t v = (*current++ >> bitpos) & (0xFFFFFFFFFFFFFFFFULL >> (64-len));
			bitpos = len - remaining;
			v |= *current & (0xFFFFFFFFFFFFFFFFULL >> (64-bitpos));
			return v;
		}
	}
	friend std::ostream& operator <<(std::ostream& s, const Bitstream& b) {
		s << std::hex;
		for (uint32_t i = 0; i < b.length(); i++)
			s << b.bits[i] << ' ';
		return s;
	}
};
		

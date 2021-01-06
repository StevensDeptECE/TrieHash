#include "Bitstream.hh"
using namespace std;

int main() {
  uint64_t a[20];
  Bitstream b(a);
  b.write(3, 2);    // write a 2-bit value
  b.write(3, 4);    // write a 4-bit value
  b.write(2, 8);    // write an 8-bit value
  b.write(6, 16);   // write a 16-bit value
  b.write(17, 16);  // write a 16-bit value
  b.write(31, 18);  // write an 18-bit value
  // first word should be
  // 0000 0000 0000 0111 1100 0000 0000 0100 0100 0000 0000 0001 1000 0000 1000
  // 1111
  // in hex: 0x0007C0044001808F

  b.write(3, 15);
  b.write(3, 15);
  b.write(3, 15);
  b.write(3, 15);  // 60 bits so far...
  b.write(3, 15);  // wraps into next word
  b.write(3, 15);
  b.write(3, 15);
  b.write(3, 15);
  b.write(255, 8);  // last 8 bits
  // values should be 0011 0000 0000 0000 0110 0000 0000 0000 1100 0000 0000
  // 0001 1000 0000 0000 0011 0x30006000C0018003

  // 1111 1111 0000 0000 0000 0110 0000 0000 0000 1100 0000 0000 0001 1000 0000
  // 0000 0xFF0006000C001800
  cout << b << '\n';
}

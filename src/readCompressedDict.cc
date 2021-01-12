#include <cstdint>
#include <fstream>
#include <iostream>
using namespace std;

constexpr uint64_t base = 27;
constexpr uint64_t base2 = base * base;
constexpr uint64_t base4 = base2 * base2;
constexpr uint64_t base8 = base4 * base4;

constexpr uint64_t baseto13 = base8 * base4 * base;
constexpr uint8_t END = base - 1;
constexpr uint8_t END2 = base - 2;

int main() {
  ifstream bin("words.bin", ios::binary);
  bin.seekg(0, std::ios::end);  // go to the end
  uint32_t bytes = bin.tellg();
  const uint32_t size = (bytes + 7) / 8;
  bin.seekg(0, std::ios::beg);  // go back to the beginning
  uint64_t* p = new uint64_t[(bytes + 7) / 8];
  bin.read((char*)p, bytes);

  for (uint32_t i = 0; i < size; i++) {
    uint64_t current = p[i];
    for (int j = 0; j < 12; j++) {
      uint8_t c = current % base;
      cout << (c < END ? (char)(c + 'a') : ' ');
      current /= base;
    }
    uint8_t c = current;
    cout << (c < END ? (char)(c + 'a') : ' ');
  }
}

#include "TrieDict.hh"
#include <unordered_map>
#include <fstream>
#include <cstring>

using namespace std;

void load(TrieHashDict& dict) {
	ifstream f("dict.txt");
	char word[256];
	while (!f.eof()) {
		f.getline(word, sizeof(word));
		if (word[0]=='\0')
			continue;
		
		uint32_t len = 0;
		for (; word[len] >= ' '; len++)
			;
		dict.add(word, len);
	}
}

void save(TrieHashDict& dict) {
	dict.save("dict.bin"); // save the binary form of the dictionary for fast loading
}

void fastLoad(TrieHashDict& x) {
	TrieHashDict dict("dict.bin");
}



#if 0
unordered_map<string, int> mymap;

void loadunordered_map(TrieHashDict& x) {
	ifstream f("dict.txt");
	char word[256];
	int wordCount = 0;
	while (!f.eof()) {
		f.getline(word, sizeof(word));
		if (word[0]=='\0')
			continue;
		dict[word] = wordCount++;
	}
}

void getunordered_map(uint32_t n) {
	const char* myword = "cat";
	uint32_t sum = 0;
	for (int i = 0; i < n; i++)
		sum += mymap[myword];
}
#endif

template<typename Func>
void benchmark(const char msg[], Func f, TrieHashDict& dict) {
	clock_t t0 = clock();
	f(dict);
	clock_t t1 = clock();
	cout << msg << "\t" << (t1-t0) << '\n';
}

int main() {
	TrieHashDict dict;
	benchmark("load", load, dict);
	benchmark("save", save, dict);
	benchmark("fastload", fastLoad, dict);
	//	benchmark("unordered_map", loadunordered_map, dict);
}

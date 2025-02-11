#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <algorithm>
#include <bitset>
#include <cmath>

using namespace std;

/*reference.lst: memory access
----------------------------
.benchmark testcase1
0000 00 00 -> 0 0 0 MISS
0001 00 00 -> 1 0 0 MISS
0010 00 00 -> 2 0 0 MISS
0000 00 00 -> 0 0 0 MISS
0010 11 00 -> 2 3 0 MISS
0000 00 00 -> 0 0 0 HIT
0010 11 00 -> 2 3 0 HIT
.end
----------------------------*/
/*cache.org: configuration of cache
---------------------------------
Address_bits: 8
Block_size: 4
Cache_sets: 4
Associativity: 2
---------------------------------*/

int cacheConfiguration[4];

class Cache {
    typedef struct A {
        bitset<32> address;
        bitset<32> tag;
        bitset<32> index;
        bitset<32> offset;
    } entry;

private:
    int addressBits;
    int blockSize;
    int cacheSets;
    int associativity;
    
    int numOfOffset;
    int numOfIndex;
    int numOfTag;
    int numOfReference;
    
    int hit;
    int miss;
    
    void toBitsets();
    void checkEntry(entry);
    
    bitset<32> getAddress(string);
    bitset<32> getOffset(string);
    bitset<32> getIndex(string);
    bitset<32> getTag(string);
    
    vector<string> memoryRead;
    vector<string> referenceLog;
    vector<entry> referenceList;
    vector<list<entry>*> cacheSetsList;
    
public:
    Cache();
    void outputFile(string);
    void readAccess(string);
    void runSimulation();
    string header;
};

Cache::Cache() {
    addressBits = cacheConfiguration[0];
    blockSize = cacheConfiguration[1];
    cacheSets = cacheConfiguration[2];
    associativity = cacheConfiguration[3];
    
    numOfOffset = log2(blockSize);
    numOfIndex = log2(cacheSets);
    numOfTag = addressBits - numOfOffset - numOfIndex;
    numOfReference = 0;
    
    hit = 0;
    miss = 0;
    
    cacheSetsList.resize(cacheSets);
    for (int i = 0; i < cacheSetsList.size(); i++) {
        cacheSetsList.at(i) = nullptr;
    }
}

void Cache::toBitsets() {
    for (int i = 0; i < memoryRead.size(); i++) {
        entry temp;
        temp.address = getAddress(memoryRead.at(i));
        temp.offset = getOffset(memoryRead.at(i));
        temp.index = getIndex(memoryRead.at(i));
        temp.tag = getTag(memoryRead.at(i));
        referenceList.push_back(temp);
    }
}

void Cache::readAccess(string fileName) {
    ifstream fin;
    string path = "./testcases/bench/" + fileName;
    fin.open(path, ios::in);
    
    string data;
    int i = 0;
    while (fin >> data) {
        if (i == 1) { 
            header = data; 
        }
        if (i > 1 && (data != string(".end"))) {
            memoryRead.push_back(data);
            numOfReference++;
        }
        i++;
    }
    toBitsets();
}

void Cache::runSimulation() {
    for (int i = 0; i < referenceList.size(); i++) {
        checkEntry(referenceList.at(i));
    }
}

void Cache::checkEntry(entry reference) {
    int static logIndex = 0;
    referenceLog.resize(numOfReference);
    int setIndex = (int)reference.index.to_ulong();
    list<entry>* currentSet = cacheSetsList.at(setIndex);
    
    if (currentSet == nullptr) {
        miss++;
        currentSet = new list<entry>;
        currentSet->push_back(reference);
        cacheSetsList.at(setIndex) = currentSet;
        referenceLog.at(logIndex++) = "miss";
        return;
    }
    
    list<entry>::iterator it;
    for (it = currentSet->begin(); it != currentSet->end(); it++) {
        if (it->tag == reference.tag) { 
            break; 
        }
    }
    
    if (currentSet->size() == associativity) {
        if (it == currentSet->end()) {
            miss++;
            currentSet->pop_front();
            currentSet->push_back(reference);
            referenceLog.at(logIndex++) = "miss";
        } else {
            hit++;
            currentSet->erase(it);
            currentSet->push_back(reference);
            referenceLog.at(logIndex++) = "hit";
        }
    } else if (currentSet->size() < associativity) {
        if (it == currentSet->end()) {
            miss++;
            currentSet->push_back(reference);
            referenceLog.at(logIndex++) = "miss";
        } else {
            hit++;
            referenceLog.at(logIndex++) = "hit";
        }
    }
}

void Cache::outputFile(string fileName) {
    ofstream fout;
    fout.open(fileName, ios::out);
    
    fout << "Address bits: " << addressBits << endl
         << "Cache sets: " << cacheSets << endl
         << "Associativity: " << associativity << endl
         << "Block size: " << blockSize << endl << endl;
    
    fout << "Indexing bit count: " << numOfIndex << endl
         << "Indexing bits: ";
    for (int i = numOfIndex + numOfOffset - 1; i >= numOfOffset; i--) {
        if (i == numOfOffset) {
            fout << i << endl;
        } else {
            fout << i << " ";
        }
    }
    fout << "Offset bit count: " << numOfOffset << endl << endl;
    
    fout << ".benchmark " << header << endl;
    for (int i = 0; i < memoryRead.size(); i++) {
        fout << memoryRead.at(i) << " " << referenceLog.at(i) << endl;
    }
    fout << ".end" << endl << endl;
    
    fout << "Total cache miss count: " << miss << endl;
}

void readCacheConfig(string fileName) {
    ifstream fin;
    string path = "./testcases/config/" + fileName;
    fin.open(path, ios::in);
    
    string data;
    int i = 0;
    while (fin >> data) {
        if (i % 2 == 1) {
            cacheConfiguration[i/2] = stoi(data);
        }
        i++;
    }
}

int main(int argc, char* argv[]) {
    readCacheConfig(string(argv[1]));
    Cache cache;
    cache.readAccess(string(argv[2]));
    cache.runSimulation();
    cache.outputFile(string(argv[3]));
    return 0;
}

bitset<32> Cache::getAddress(string addr) {
    return bitset<32>(addr);
}

bitset<32> Cache::getOffset(string addr) {
    return bitset<32>(addr, numOfTag + numOfIndex, numOfOffset);
}

bitset<32> Cache::getIndex(string addr) {
    return bitset<32>(addr, numOfTag, numOfIndex);
}

bitset<32> Cache::getTag(string addr) {
    return bitset<32>(addr, 0, numOfTag);
}
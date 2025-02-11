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
    
    void findIndexing();
    void findQualityList();
    void findCorrelationMatrix();
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
    
    vector<float> bitQuality;
    vector<vector<float>> bitCorrelation;
    vector<int> selectedIndex;
    vector<int> selectedTag;

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

void Cache::findQualityList() {
    bitQuality.resize(addressBits);
    
    for (int i = 0; i < addressBits; i++) {
        float zeroCounter = 0;
        float oneCounter = 0;
        
        for (int j = 0; j < memoryRead.size(); j++) {
            char bit = memoryRead.at(j).at(i);
            if (bit == '0') zeroCounter++;
            else if (bit == '1') oneCounter++;
        }
        
        float quality = (zeroCounter > oneCounter) ? 
            oneCounter / zeroCounter : 
            (oneCounter > zeroCounter) ? zeroCounter / oneCounter : 1;
            
        bitQuality.at(addressBits - 1 - i) = quality;
    }
}

void Cache::findCorrelationMatrix() {
    bitCorrelation.resize(addressBits, vector<float>(addressBits));
    
    for (int i = 0; i < addressBits; i++) {
        for (int j = 0; j < addressBits; j++) {
            float hit = 0;
            float miss = 0;
            
            for (const string& mem : memoryRead) {
                if (mem.at(i) == mem.at(j)) hit++;
                else miss++;
            }
            
            float correlation = (hit > miss) ? 
                miss / hit : 
                (miss > hit) ? hit / miss : 1;
                
            bitCorrelation.at(addressBits - 1 - i).at(addressBits - 1 - j) = correlation;
        }
    }
}

void Cache::findIndexing() {
    for (int i = 0; i < numOfOffset; i++) {
        bitQuality.at(i) = -1;
    }
    
    selectedIndex.reserve(numOfIndex);
    selectedTag.reserve(numOfTag);
    
    for (int i = 0; i < numOfIndex; i++) {
        float max = -1;
        int maxIndex = -1;
        
        for (int j = 0; j < addressBits; j++) {
            if (bitQuality.at(j) != -1 && bitQuality.at(j) > max) {
                max = bitQuality.at(j);
                maxIndex = j;
            }
        }
        
        bitQuality.at(maxIndex) = -1;
        selectedIndex.push_back(maxIndex);
        
        for (int k = 0; k < addressBits; k++) {
            if (bitQuality.at(k) >= 0) {
                bitQuality.at(k) *= bitCorrelation.at(maxIndex).at(k);
            }
        }
    }
    
    for (int i = 0; i < addressBits; i++) {
        if (bitQuality.at(i) >= 0) {
            selectedTag.push_back(i);
        }
    }
    
    sort(selectedTag.rbegin(), selectedTag.rend());
    sort(selectedIndex.rbegin(), selectedIndex.rend());
}

void Cache::toBitsets() {
    for (const string& addr : memoryRead) {
        entry temp;
        temp.address = getAddress(addr);
        temp.offset = getOffset(addr);
        temp.index = getIndex(addr);
        temp.tag = getTag(addr);
        referenceList.push_back(temp);
    }
}

void Cache::readAccess(string fileName) {
    ifstream fin("./testcases/bench/" + fileName);
    string data;
    int i = 0;
    
    while (fin >> data) {
        if (i == 1) header = data;
        if (i > 1 && data != ".end") {
            memoryRead.push_back(data);
            numOfReference++;
        }
        i++;
    }
    
    findQualityList();
    findCorrelationMatrix();
    findIndexing();
    toBitsets();
}

void Cache::runSimulation() {
    for (const entry& ref : referenceList) {
        checkEntry(ref);
    }
}

void Cache::checkEntry(entry reference) {
    static int logIndex = 0;
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
    
    auto it = find_if(currentSet->begin(), currentSet->end(),
        [&reference](const entry& e) { return e.tag == reference.tag; });
    
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
    } else {
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
    ofstream fout(fileName);
    
    fout << "Address bits: " << addressBits << endl
         << "Cache sets: " << cacheSets << endl
         << "Associativity: " << associativity << endl
         << "Block size: " << blockSize << endl << endl;
    
    fout << "Indexing bit count: " << numOfIndex << endl
         << "Indexing bits: ";
         
    for (int i = 0; i < numOfIndex; i++) {
        fout << selectedIndex.at(i);
        if (i < numOfIndex - 1) fout << " ";
    }
    fout << endl;
    
    fout << "Offset bit count: " << numOfOffset << endl << endl;
    
    fout << ".benchmark " << header << endl;
    for (size_t i = 0; i < memoryRead.size(); i++) {
        fout << memoryRead.at(i) << " " << referenceLog.at(i) << endl;
    }
    fout << ".end" << endl << endl;
    
    fout << "Total cache miss count: " << miss << endl;
}

void readCacheConfig(string fileName) {
    ifstream fin("./testcases/config/" + fileName);
    string data;
    int i = 0;
    
    while (fin >> data) {
        if (i % 2 == 1) {
            cacheConfiguration[i/2] = stoi(data);
        }
        i++;
    }
}

bitset<32> Cache::getAddress(string addr) {
    return bitset<32>(addr);
}

bitset<32> Cache::getOffset(string addr) {
    return bitset<32>(addr, numOfTag + numOfIndex, numOfOffset);
}

bitset<32> Cache::getIndex(string addr) {
    bitset<32> addressBits(addr);
    bitset<32> indexBits;
    indexBits.reset();
    
    for (int i = 0; i < numOfIndex; i++) {
        indexBits[i] = addressBits[selectedIndex.at(i)];
    }
    return indexBits;
}

bitset<32> Cache::getTag(string addr) {
    bitset<32> addressBits(addr);
    bitset<32> tagBits;
    tagBits.reset();
    
    for (int i = 0; i < numOfTag; i++) {
        tagBits[i] = addressBits[selectedTag.at(i)];
    }
    return tagBits;
}

int main(int argc, char* argv[]) {
    readCacheConfig(string(argv[1]));
    Cache cache;
    cache.readAccess(string(argv[2]));
    cache.runSimulation();
    cache.outputFile(string(argv[3]));
    return 0;
}
#include <ostream>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

#include "../include/id.h"

void echo(id::Identifier id) {
    std::cout << "UID: " << id.uid << ", String: " << id.String() << ", Chars: " << id.Chars() << ", Storage: " << id.Storage() << std::endl;
}

void testBasic() {
    auto a = id::Identifier("Hello");
    id::Identifier b = "Howdy!";

    std::cout << "UID: " << a.uid << ", String: " << a.String() << ", Chars: " << a.Chars() << std::endl;
    std::cout << "UID: " << b.uid << ", String: " << b.String() << ", Chars: " << b.Chars() << std::endl;

    echo("Meow");

    auto sm = id::SparseMap32<float>();
    sm["Hi"] = 3.4f;
    std::cout << sm["Hi"] << std::endl;
    std::cout << sm.Ptr("missing!") << std::endl;
    std::cout << sm.Get("also not there and returns default float") << std::endl;

    std::cout << std::endl << "All defined ids:" << std::endl;
    for (auto id : id::All()) {
        std::cout << " - " << id << std::endl; 
    }

    auto dm = id::DenseMap8<std::string>();
    dm["a"] = "Apple";
    dm["b"] = "Banana";
    dm["a"] = "Actually";
    dm["c"] = "Corn";
    std::cout << std::endl << "Dense map data:" << std::endl;
    for (auto& str : dm.Values()) {
        std::cout << " - " << str << std::endl;
    }

    dm.Remove("a", true);

    std::cout << std::endl << "Dense map data after removing a (keep order):" << std::endl;
    for (auto& str : dm.Values()) {
        std::cout << " - " << str << std::endl;
    }

    dm["d"] = "Donut";
    dm.Remove("b", false);

    std::cout << std::endl << "Dense map data after adding d and removing b (ignoring order):" << std::endl;
    for (auto& str : dm.Values()) {
        std::cout << " - " << str << std::endl;
    }
}

const int keyCount = 1024;
const int mapRounds = 256;
const int accessCount = 2048;
std::string mapKeys[keyCount];
id::Identifier idKeys[keyCount];
int updateOrder[accessCount];

void populateKeys() {
    for (int i = 0; i < keyCount; i++) {
        mapKeys[i] = "mapKey:" + std::to_string(i);
        idKeys[i] = mapKeys[i];
    }

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, keyCount-1); 
    
    for (int a = 0; a < accessCount; a++) {
        updateOrder[a] = distr(gen);
    }
}

void testMapWrite(std::string prefix) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int k = 0; k < keyCount; k++) {
            m[mapKeys[k]] = 0;
        }
    }
    auto end = std::chrono::steady_clock::now();;
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    std::cout << prefix << (duration * 0.000000001) << "s for writes: " << (mapRounds * keyCount) << std::endl;
}

void testDenseMapWrite(std::string prefix, id::Area<id::id_t, uint16_t>* area) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int k = 0; k < keyCount; k++) {
            m.Set(idKeys[k], 0);
        }
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    std::cout << prefix << (duration * 0.000000001) << "s for writes: " << (mapRounds * keyCount) << std::endl;
}

void testMapUpdate(std::string prefix) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int k = 0; k < keyCount; k++) {
            m[mapKeys[k]] = 0;
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            auto i = m.find(mapKeys[updateOrder[a]]);
            if (i != m.end()) {
                i->second++;
            }
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for updates: " << (mapRounds * accessCount) << std::endl;
}

void testDenseMapUpdate(std::string prefix, id::Area<id::id_t, uint16_t>* area) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int k = 0; k < keyCount; k++) {
            m.Set(idKeys[k], 0);
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            auto& v = m.Take(idKeys[updateOrder[a]]);
            v++;
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for updates: " << (mapRounds * accessCount) << std::endl;
}

void testMapIteration(std::string prefix) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int k = 0; k < keyCount; k++) {
            m[mapKeys[k]] = 0;
        }
        auto start = std::chrono::steady_clock::now();
        for (auto& p : m) {
            p.second++;
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for iterations: " << (mapRounds * keyCount) << std::endl;
}

void testDenseMapIteration(std::string prefix, id::Area<id::id_t, uint16_t>* area) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int k = 0; k < keyCount; k++) {
            m.Set(idKeys[k], 0);
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            auto& v = m.Take(idKeys[updateOrder[a]]);
            v++;
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for iterations: " << (mapRounds * keyCount) << std::endl;
}

void testMapRemove(std::string prefix) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int k = 0; k < keyCount; k++) {
            m[mapKeys[k]] = 0;
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            m.erase(mapKeys[updateOrder[a]]);
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for removes: " << (mapRounds * accessCount) << std::endl;
}

void testDenseMapRemove(std::string prefix, id::Area<id::id_t, uint16_t>* area, bool preserveOrder) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int k = 0; k < keyCount; k++) {
            m.Set(idKeys[k], 0);
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            m.Remove(idKeys[updateOrder[a]], preserveOrder);
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for removes: " << (mapRounds * accessCount) << std::endl;
}

void testVectorRemove(std::string prefix, bool preserveOrder) {
    long long total = 0;
    for (int i = 0; i < mapRounds; i++) {
        auto v = std::vector<int>();
        for (int k = 0; k < keyCount; k++) {
            v.push_back(k);
        }
        auto start = std::chrono::steady_clock::now();
        for (int a = 0; a < accessCount; a++) {
            auto i = updateOrder[a];
            if (i >= v.size()) {
                i = v.size() / 2;
            }
            if (i < v.size()) {
                if (preserveOrder) {
                    v.erase(v.begin() + i);
                } else {
                    v[i] = std::move(v.back());
                    v.pop_back();
                }
            }
        }
        auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
    }
    std::cout << prefix << (total * 0.000000001) << "s for removes: " << (mapRounds * accessCount) << std::endl;
}

void testSet() {
    auto s = id::Set();
    std::cout << "testSet has maybe: expected: false, actual: " << s.Has("Should not generate identifier") << std::endl;
    std::cout << "testSet ensure maybe works: expected: -1, actual: " << id::memory.Peek("Should not generate identifier") << std::endl;

    s.Add("This should exist now");
    s.Add("Okay!");

    auto size = 0;
    for (auto id : s) {
        size++;
        std::cout << "testSet iteration: " << id << std::endl;
    }
    std::cout << "testSet add then iteration size: expected: 2, actual: " << size << std::endl;

    s.Remove("This should exist now");

    std::cout << "testSet has removed: expected: 0, actual: " << s.Has("This should exist now") << std::endl;
}

void testSmallSet() {
    auto s = id::SmallSet();
    std::cout << "testSmallSet has maybe: expected: false, actual: " << s.Has("Should not generate identifier") << std::endl;
    std::cout << "testSmallSet ensure maybe works: expected: -1, actual: " << id::memory.Peek("Should not generate identifier") << std::endl;

    s.Add("This should exist now");
    s.Add("Okay!");

    auto size = 0;
    for (auto id : s) {
        size++;
        std::cout << "testSmallSet iteration: " << id << std::endl;
    }
    std::cout << "testSmallSet add then iteration size: expected: 2, actual: " << size << std::endl;

    s.Remove("This should exist now");

    std::cout << "testSmallSet has removed: expected: 0, actual: " << s.Has("This should exist now") << std::endl;
}

int main() {
    testBasic();
    testSet();
    testSmallSet();
    populateKeys();

    std::cout << std::fixed << std::setprecision(9);

    auto area = id::Area<id::id_t, uint16_t>(120, keyCount);

    testMapWrite(           "testMapWrite:                      ");
    testDenseMapWrite(      "testDenseMapWrite:                 ", nullptr);
    testDenseMapWrite(      "testDenseMapWrite (with area):     ", &area);
    testMapUpdate(          "testMapUpdate:                     ");
    testDenseMapUpdate(     "testDenseMapUpdate:                ", nullptr);
    testDenseMapUpdate(     "testDenseMapUpdate (with area):    ", &area);
    testMapIteration(       "testMapIteration:                  ");
    testDenseMapIteration(  "testDenseMapIteration:             ", nullptr);
    testDenseMapIteration(  "testDenseMapIteration (with area): ", &area);
    testMapRemove(          "testMapRemove:                     ");
    testDenseMapRemove(     "testDenseMapRemove (no order):     ", nullptr, false);
    testDenseMapRemove(     "testDenseMapRemove (ordered):      ", nullptr, true);
    testDenseMapRemove(     "testDenseMapRemove (area, -order): ", &area, false);
    testDenseMapRemove(     "testDenseMapRemove (area, +order): ", &area, true);
    testVectorRemove(       "testVectorRemove (no order):       ", false);
    testVectorRemove(       "testVectorRemove (ordered):        ", true);

    return 0;
}
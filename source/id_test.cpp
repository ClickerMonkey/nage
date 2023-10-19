#include <ostream>
#include <iostream>
#include <chrono>
#include <random>

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
const int mapRounds = 100;
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

void testMapWrite() {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int i = 0; i < keyCount; i++) {
            m[mapKeys[i]] = 0;
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testMapWrite:      " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

void testDenseMapWrite(id::Area<id::id_t, uint16_t>* area) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int i = 0; i < keyCount; i++) {
            m.Set(idKeys[i], 0);
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testDenseMapWrite: " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

void testMapUpdate() {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int i = 0; i < keyCount; i++) {
            m[mapKeys[i]] = 0;
        }
        for (int a = 0; a < accessCount; a++) {
            auto i = m.find(mapKeys[updateOrder[a]]);
            if (i != m.end()) {
                i->second++;
            }
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testMapUpdate:      " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

void testDenseMapUpdate(id::Area<id::id_t, uint16_t>* area) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int i = 0; i < keyCount; i++) {
            m.Set(idKeys[i], 0);
        }
        for (int a = 0; a < accessCount; a++) {
            auto& v = m.Take(idKeys[updateOrder[a]]);
            v++;
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testDenseMapUpdate: " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

void testMapIteration() {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = std::map<std::string, int>();
        for (int i = 0; i < keyCount; i++) {
            m[mapKeys[i]] = 0;
        }
        for (auto& p : m) {
            p.second++;
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testMapIteration:      " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

void testDenseMapIteration(id::Area<id::id_t, uint16_t>* area) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < mapRounds; i++) {
        auto m = id::DenseMap<int, uint16_t, uint16_t>(area);
        for (int i = 0; i < keyCount; i++) {
            m.Set(idKeys[i], 0);
        }
        for (auto& v : m.Values()) {
            v++;
        }
    }
    auto end = std::chrono::steady_clock::now();;
    std::cout << "testDenseMapIteration: " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count() << " ns" << std::endl;
}

int main() {
    testBasic();
    populateKeys();
    testMapWrite();
    testDenseMapWrite(nullptr);
    testMapUpdate();
    testDenseMapUpdate(nullptr);
    testMapIteration();
    testDenseMapIteration(nullptr);

    return 0;
}
#include <ostream>
#include <iostream>

#include "../include/id.h"

void echo(id::Identifier id) {
    std::cout << "UID: " << id.uid << ", String: " << id.String() << ", Chars: " << id.Chars() << ", Storage: " << id.Storage() << std::endl;
}

int main() {
    auto a = id::ID("Hello");
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
    for (auto id : id::All) {
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

    return 0;
}
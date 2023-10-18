#include "../include/types.h"

// Example with metadata. 
struct GameMetadata {
    bool Animatable;
    bool Serializable;
    size_t Bytes;
    std::string XmlName;

    friend std::ostream& operator<<(std::ostream& os, const GameMetadata& a) {
        os << "GameMetadata{Animatable=" << a.Animatable << ", Serializable=" << a.Serializable << ", Bytes=" << a.Bytes << ", XmlName=" << a.XmlName << "}";
        return os;
    }
};

struct Vec {
    float x, y;
};

// Types for testing metadata objects
auto TFloat   = types::New<float>("float");
auto TVec     = types::New<Vec>("vec");

// Define types
void DefineTypes() {
    TFloat->Define(types::Def<float>()
        .DefaultCreate()
        .ToString([](float s) -> std::string { return std::to_string(s); })
        .FromString([](const std::string& s) -> float { return std::stof(s); })
    );
    TVec->Define(types::Def<Vec>()
        .DefaultCreate()
        .Prop<float>("x", TFloat, [](auto v) -> auto { return &v->x; })
        .Prop<float>("y", TFloat, [](auto v) -> auto { return &v->y; })
    );
}

void DefineMetadata() {
    types::SetTypeMeta(TFloat, GameMetadata{
        .Animatable = true,
        .Serializable = true,
        .Bytes = 4,
        .XmlName = "Float"
    });
    types::SetTypeMeta(TVec, GameMetadata{
        .Animatable = true,
        .Serializable = true,
        .Bytes = 8,
        .XmlName = "Vec"
    });
    types::SetPropMeta(TVec, "x", GameMetadata{
        .XmlName = "X"
    });
}

int main() {
    DefineTypes();
    DefineMetadata();

    for (auto type : types::Types()) {
        auto m = types::GetTypeMeta<GameMetadata>(type);
        if (m != nullptr) {
            std::cout << type->Name() << ": " << *m << std::endl;
        }
        for (auto& prop : type->Props()) {
            auto pm = types::GetPropMeta<GameMetadata>(type, prop.name);
            if (pm != nullptr) {
                std::cout << type->Name() << "." << prop.name << ": " << *m << std::endl;
            }
        }
    }
    
    return 0;
}
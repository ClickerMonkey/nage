#include "../include/calcs.h"

// Game types
struct Vec {
    float x, y;
};

// Create reflection types
auto TFloat = types::New<float>("float");
auto TVec   = types::New<Vec>("vec");

// To add calculator support you need to add speciations for the following
// calc functions:
// - Adds, Mul, Div, IsLess, Dot, Components, Get, Set
// OR implement the following operators and speciations:
// - +, *, /, <, Dot, Components, Get, Set
template<>
constexpr Vec calc::Adds(const Vec& a, const Vec& b, float scale) {
    return Vec{.x = a.x + b.x * scale, .y = a.y + b.y * scale};
}
template<>
constexpr Vec calc::Mul(const Vec& a, const Vec& b) {
    return Vec{.x = a.x * b.x, .y = a.y * b.y};
}
template<>
constexpr Vec calc::Div(const Vec& a, const Vec& b) {
    return Vec{.x = calc::Div<float>(a.x, b.x), .y = calc::Div<float>(a.y, b.y)};
}
template<>
constexpr bool calc::IsLess(const Vec& a, const Vec& b) {
    return a.x < b.x ? true : a.y < b.y;
}
template<>
constexpr float calc::Dot(const Vec& a, const Vec& b) {
    return a.x * b.x + a.y * b.y;
}
template<>
constexpr int calc::Components<Vec>() {
    return 2;
}
template<>
constexpr float calc::Get(const Vec& a, int index) {
    switch (index) {
        case 0: return a.x;
        case 1: return a.y;
    }
    return 0.0f;
}
template<>
constexpr void calc::Set(Vec& a, int index, float value) {
    switch (index) {
        case 0: a.x = value; break;
        case 1: a.y = value; break;
    }
}

int main() {
    // Add definition to types.
    TFloat->Define(types::Def<float>()
        .DefaultCreate()
        .ToString([](float s) -> std::string { return std::to_string(s); })
        .FromString([](std::string s) -> float { return std::stof(s); })
    );

    TVec->Define(types::Def<Vec>()
        .DefaultCreate()
        .Prop<float>("x", TFloat, [](auto v) -> auto { return &v->x; })
        .Prop<float>("y", TFloat, [](auto v) -> auto { return &v->y; })
    );

    // Register calculators (speciations for calc functions need to exist)
    calc::Register<float>(TFloat);
    calc::Register<Vec>(TVec);

    // Float test
    auto c = calc::For(TFloat);
    auto a = TFloat->New(1.0f);
    auto b = TFloat->New(2.0f);
    auto d = c->Add(a, b);
    auto v = d.Get<float>();

    std::cout << "float add expected 3; actual: " << v << std::endl;

    // Vec test
    auto c2 = calc::For(TVec);
    auto a2 = TVec->New(Vec{.x = 1, .y = 2});
    auto b2 = TVec->New(Vec{.x = 3, .y = 4});
    auto d2 = c2->Add(a2, b2);
    auto v2 = d2.Get<Vec>();
    auto l2 = c2->Lerp(a2, b2, 0.5f).Get<Vec>();

    std::cout << "vec add expected 4,6; actual: " << v2.x << "," << v2.y << std::endl;
    std::cout << "vec lerp expected 2,3; actual: " << l2.x << "," << l2.y << std::endl;

    return 0;
}
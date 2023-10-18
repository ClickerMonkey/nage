#include <iostream>

template<typename T>
struct Value {
    float& v;

    Value(float& value): v(value) {}

    constexpr T Get() const noexcept {
        return static_cast<T>(v);
    }
    constexpr void Set(const T& value) noexcept {
        v = static_cast<float>(value);
    }

    constexpr operator T() const noexcept { return Get(); }
    constexpr void operator=(const T& value) noexcept { Set(value); }
};

template<> constexpr bool Value<bool>::Get() const noexcept { return v != 0.0f; }
template<> constexpr void Value<bool>::Set(const bool& b) noexcept { v = b ? 1.0f : 0.0f; }

template<typename T>
struct ValueProperty {};

struct ValueHolder {
    float x;

    template<typename T>
    Value<T> operator [](ValueProperty<T>) { return Value<T>(x); }
};

int main() {
    ValueProperty<int> vp;
    ValueProperty<bool> vb;

    auto vh = ValueHolder();
    auto i = vh[vp]; 

    std::cout << typeid(i).name() << " " << i.Get() << std::endl;

    vh[vp] = 34;

    std::cout << i.Get() << " " << vh.x << std::endl;

    std::cout << (i < 45) << std::endl;

    std::cout << vh[vb] << std::endl;
    std::cout << (vh[vb] == true) << std::endl;

    vh[vb] = true;

    std::cout << i.Get() << " " << vh.x << std::endl;

    return 0;
}
#include "../include/types.h"

// Game objects
struct Vec {
    float x, y;
    Vec add(Vec v) { return Vec{this->x+v.x, this->y+v.y}; }
    Vec sub(Vec v) { return Vec{this->x-v.x, this->y-v.y}; }
};

struct Sprite {
    float angle;
    Vec position;
    Vec size;
    int frame;
};

struct Game {
    std::vector<Sprite> sprites;
};

// Types for Game objects
auto TInt     = types::New<int>("int");
auto TFloat   = types::New<float>("float");
auto TAngle   = types::New<float>("angle");
auto TString  = types::New<std::string>("string");
auto TVec     = types::New<Vec>("vec");
auto TSprite  = types::New<Sprite>("sprite");
auto TSprites = types::New<std::vector<Sprite>>("sprites");
auto TGame    = types::New<Game>("game");

// Define types
void DefineTypes() {
    TInt->Define(types::Def<int>()
        .DefaultCreate()
        .ToString([](int s) -> std::string { return std::to_string(s); })
        .FromString([](const std::string& s) -> int { return std::stoi(s); })
    );

    TFloat->Define(types::Def<float>()
        .DefaultCreate()
        .ToString([](float s) -> std::string { return std::to_string(s); })
        .FromString([](const std::string& s) -> float { return std::stof(s); })
    );

    TAngle->Define(types::Def<float>()
        .DefaultCreate()
        .ToString([](float s) -> std::string { return std::to_string(s); })
        .FromString([](const std::string& s) -> float { return std::stof(s); })
    );

    TString->Define(types::Def<std::string>()
        .DefaultCreate()
        .ToString([](std::string s) -> std::string { return s; })
        .FromString([](const std::string& s) -> std::string { return s; })
    );

    TVec->Define(types::Def<Vec>()
        .DefaultCreate()
        .Prop<float>("x", TFloat, [](auto v) -> auto { return &v->x; })
        .Prop<float>("y", TFloat, [](auto v) -> auto { return &v->y; })
    );

    TSprite->Define(types::Def<Sprite>()
        .DefaultCreate()
        .Prop<float> ("angle",       TAngle, [](auto v) -> auto { return &v->angle; })
        .Prop<Vec>   ("position",    TVec,   [](auto v) -> auto { return &v->position; })
        .Prop<Vec>   ("size",        TVec,   [](auto v) -> auto { return &v->size; })
        .Prop<int>   ("frame",       TInt,   [](auto v) -> auto { return &v->frame; })
        .Virtual<Vec>("bottomRight", TVec,   
            [](auto v) -> auto { return v->position.add(v->size); },
            [](auto v, auto br) -> auto { v->position = br.sub(v->size); return true; }
        )
    );

    TGame->Define(types::Def<Game>()
        .DefaultCreate()
        .Prop<std::vector<Sprite>>("sprites", TSprites, [](auto s) -> auto { return &s->sprites; })
    );
    
    TSprites->Define(types::Def<std::vector<Sprite>>()
        .DefaultCreate()
        .Computed<int>("size", TInt, [](auto s) -> auto { return s->size(); })
        .Vector<Sprite>(TInt, TSprite, [](std::vector<Sprite>* s) -> std::vector<Sprite>* { return s; })
    );
}

auto getHelloWorld() {
    return types::ValueOf<std::string>("hello");
}

int main() {
    DefineTypes();

    auto hw = getHelloWorld();
    auto i = types::ValueOf<int>(34);
    
    std::cout<<"[value type name   ] expected: string, actual: "<<(hw.GetType()->Name())<<std::endl;
    std::cout<<"[type by name      ] expected: 1, actual: "<<(types::Types().Get("FLOAT")->ID())<<std::endl;
    std::cout<<"[copy wrong type   ] expected: , actual: "<<i.Get<std::string>()<<std::endl;
    std::cout<<"[copy correct type ] expected: 34, actual: "<<i.Get<int>()<<std::endl;
    
    try {
        types::FamilyFor<bool>()->Base();
        std::cout<<"[get undefined type] expected: type b must be defined before being referenced, actual: "<<std::endl;
    } catch (std::invalid_argument e) {
        std::cout<<"[get undefined type] expected: type b must be defined before being referenced, actual: "<<e.what()<<std::endl;
    }
    
    auto a = types::ValueOf(12);
    auto b = a.Set(i);
    std::cout<<"[value set         ] expected: 34, actual: "<<a.Get<int>()<<std::endl;
    
    auto v = types::ValueOf(Vec{1, 2});
    
    std::cout<<"[get vec x         ] expected: 1, actual: "<<v.Prop("x").Get<float>()<<std::endl;
    std::cout<<"[get vec y         ] expected: 2, actual: "<<v.Prop("y").Get<float>()<<std::endl;
    
    v.Prop("x").Set(3.0f);
    std::cout<<"[get vec x         ] expected: 3, actual: "<<v.Prop("x").Get<float>()<<std::endl;

    auto g = new Game();
    g->sprites.push_back(Sprite{});
    g->sprites.push_back(Sprite{45, Vec{0.5, 0.5}, Vec{1, 1}, 0});
    auto gv = types::ValueTo(g, TGame);
    auto sprites = gv.Prop("sprites").Collection();
    auto sprite1 = sprites->Get(types::ValueOf(1));

    std::cout<<"[get value by key  ] expected: 45, actual: "<<sprite1.Prop("angle").Get<float>()<<std::endl;

    sprites->Iterate([](types::Collection::iter_type* kv) -> bool {
        auto index = kv->key.Get<int>();
        auto angle = kv->value.Prop("angle").Get<float>();
        std::cout<<"iterated sprite at "<<index<<" with angle "<<angle<<std::endl;
        return true;
    });

    std::cout<<std::endl<<std::endl<<"Types:"<<std::endl;
    for (auto type : types::Types()) {
        std::cout<<"Type "<<type->Name()<<std::endl;
        for (auto& prop : type->Props()) {
            std::cout<<"\t- has property "<<prop.name<<std::endl;
        }
        if (type->IsCollection()) {
            auto& c = type->Collection();
            std::cout<<"\t- is a collection of "<<c->key->Name()<<"->"<<c->value->Name()<<" pairs"<<std::endl;
        }
    }

    return 0;
}
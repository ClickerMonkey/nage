// #define LOG_ENABLED true

#include "../include/anim.h"

// Inputs that can drive the effect and condition functions.
struct Input {
    static const inline state::UserStateProperty<bool>  Jump = 0;
    static const inline state::UserStateProperty<bool>  OnGround = 1; 
    static const inline state::UserStateProperty<bool>  GrabbingLedge = 2;
    static const inline state::UserStateProperty<bool>  PullLedge = 3;
    static const inline state::UserStateProperty<float> ForwardSpeed = 4; // -1=backwards, 0=still, 1=forwards
    static const inline state::UserStateProperty<float> SideSpeed = 5;    // -1=left, 0=still, 1=right  
    static const inline state::UserStateProperty<float> FallingSpeed = 6; // -1=up, 0=still, 1=down
};

state::UserState NewInput() {
    return state::UserState(7);
}

// Type definitions
auto TFloat = types::New<float>("float");

void DefineTypes() {
    TFloat->Define(types::Def<float>()
        .DefaultCreate()
        .ToString([](float s) -> std::string { return std::to_string(s); })
        .FromString([](std::string s) -> float { return std::stof(s); })
    );

    calc::Register<float>(TFloat);
}

// Loads an animation
anim::Animation Load(const std::string& name, float dataStart = 0) {
    return anim::Animation{
        .name = name,
        .options = {
            .duration = 1.0f,
            .repeat = -1,
            .path = anim::LinearPath,
            .easing = anim::Linear,
        },
        .attributes = {{
            .attribute = "position",
            .points = {
                {.time=0.0f, .data=TFloat->New(dataStart+0.0f)},
                {.time=0.5f, .data=TFloat->New(dataStart+1.0f)},
                {.time=1.0f, .data=TFloat->New(dataStart+0.5f)}
            }
        }}
    };
}

// Helper functions
template<typename T>
T Abs(T value) {
    return value < 0 ? -value : value;
}
template<typename T>
T Min(T a, T b) {
    return a < b ? a : b;
}
template<typename T>
T Max(T a, T b) {
    return a >= b ? a : b;
}

// Effect functions
anim::AnimationOptions IdleEffect(const state::UserState& i, const state::UserState& u) { 
    auto speed = 1.0f-Max(Abs(i.Get(Input::ForwardSpeed)), Abs(i.Get(Input::SideSpeed)));
    return anim::AnimationOptions{ .scale = speed };
}
anim::AnimationOptions ForwardEffect(const state::UserState& i, const state::UserState& u) { 
    auto speed = i.Get(Input::ForwardSpeed) > 0 ? i.Get(Input::ForwardSpeed) : 0;
    return anim::AnimationOptions{ .scale = speed };
}
anim::AnimationOptions BackwardEffect(const state::UserState& i, const state::UserState& u) { 
    auto speed = i.Get(Input::ForwardSpeed) < 0 ? -i.Get(Input::ForwardSpeed) : 0;
    return anim::AnimationOptions{ .scale = speed };
}
anim::AnimationOptions RightEffect(const state::UserState& i, const state::UserState& u) { 
    auto speed = i.Get(Input::SideSpeed) > 0 ? i.Get(Input::SideSpeed) : 0;
    return anim::AnimationOptions{ .scale = speed };
}
anim::AnimationOptions LeftEffect(const state::UserState& i, const state::UserState& u) { 
    auto speed = i.Get(Input::SideSpeed) < 0 ? -i.Get(Input::SideSpeed) : 0;
    return anim::AnimationOptions{ .scale = speed };
}
// Transition conditions
bool Jumped(const state::UserState& i, const state::UserState& u) { return i.Get(Input::Jump); }
bool IsFalling(const state::UserState& i, const state::UserState& u) { return i.Get(Input::FallingSpeed) < 0 && !i.Get(Input::OnGround); }
bool OnGround(const state::UserState& i, const state::UserState& u) { return i.Get(Input::OnGround); }
bool LedgeGrabbed(const state::UserState& i, const state::UserState& u) { return i.Get(Input::GrabbingLedge); }
bool LedgeLetGo(const state::UserState& i, const state::UserState& u) { return !i.Get(Input::GrabbingLedge) || i.Get(Input::OnGround); }
bool LedgePulled(const state::UserState& i, const state::UserState& u) { return i.Get(Input::PullLedge); }

int main() {
    DefineTypes();

    std::cout << "It compiles!" << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    auto initialInput = NewInput();
    initialInput.Set(Input::Jump, false);
    initialInput.Set(Input::OnGround, true);
    initialInput.Set(Input::GrabbingLedge, false);
    initialInput.Set(Input::PullLedge, false);
    initialInput.Set(Input::ForwardSpeed, 0.0f);
    initialInput.Set(Input::SideSpeed, 0.0f);
    initialInput.Set(Input::FallingSpeed, 0.0f);
    
    auto update = anim::NewUpdate();
    update.Set(anim::Update::DeltaTime, 0.1f);

    // All states are constantly active and blended together based on speed & direction.
    auto grounded = anim::NewSubDefinition(anim::MachineOptions{.FullyActive = true});
    grounded.AddState(anim::StateDefinition("idle", Load("idle", 0), IdleEffect, true));
    grounded.AddState(anim::StateDefinition("forward", Load("forward", 1), ForwardEffect, true));
    grounded.AddState(anim::StateDefinition("backward", Load("backward", 2), BackwardEffect, true));
    grounded.AddState(anim::StateDefinition("left", Load("strafeLeft", 3), LeftEffect, true));
    grounded.AddState(anim::StateDefinition("right", Load("strafeRight", 4), RightEffect, true));

    // All states are constantly active and blended together based on speed & direction.
    auto ledge = anim::NewSubDefinition(anim::MachineOptions{.FullyActive = true});
    ledge.AddState(anim::StateDefinition("idle", Load("ledgeIdle", 5), IdleEffect, true));
    ledge.AddState(anim::StateDefinition("forward", Load("ledgeUp", 6), ForwardEffect, true));
    ledge.AddState(anim::StateDefinition("backward", Load("ledgeDown", 7), BackwardEffect, true));
    ledge.AddState(anim::StateDefinition("left", Load("ledgeLeft", 8), LeftEffect, true));
    ledge.AddState(anim::StateDefinition("right", Load("ledgeRight", 9), RightEffect, true));

    auto One = anim::AnimationOptions{ .scale = 1.0f };

    auto def = anim::NewDefinition(initialInput, anim::MachineOptions{.ProcessQueueImmediately = true});
    def.AddState(anim::StateDefinition("grounded", grounded));
    def.AddState(anim::StateDefinition("ledge", ledge));
    def.AddState(anim::StateDefinition("ledgeGrab", Load("ledgeGrab", 10), One));
    def.AddState(anim::StateDefinition("ledgeDrop", Load("ledgeGrab", 11), One, anim::Options{.animation={.scale=-1.0f}}));
    def.AddState(anim::StateDefinition("ledgePullUp", Load("ledgePullUp", 12), One));
    def.AddState(anim::StateDefinition("jumping", Load("jumping", 13), One));
    def.AddState(anim::StateDefinition("falling", Load("falling", 14), One));
    def.AddState(anim::StateDefinition("landing", Load("landing", 15), One));

    def.AddTransition(anim::Transition("grounded", OnGround));
    def.AddTransition(anim::Transition("falling", IsFalling));
    def.AddTransition(anim::Transition("grounded", "jumping", Jumped, true, anim::Options{.transition={.time=0}})); // player jumped
    def.AddTransition(anim::Transition("grounded", "falling", IsFalling, true, anim::Options{.transition={.time=0}})); // player walked off edge
    def.AddTransition(anim::Transition("jumping", "falling", IsFalling, true, anim::Options{.transition={.time=0}})); // player reached peak of jump, starting to fall
    def.AddTransition(anim::Transition("falling", "landing", OnGround, true, anim::Options{.transition={.time=0}})); // player hit ground, land
    def.AddTransition(anim::Transition("landing", "grounded", false)); // land to grounded right away
    def.AddTransition(anim::Transition("grounded", "ledgeGrab", LedgeGrabbed, true, anim::Options{.transition={.time=0}})); // player walked up to ledge
    def.AddTransition(anim::Transition("ledgeGrab", "ledge", false)); // player now climbing
    def.AddTransition(anim::Transition("ledge", "ledgePullUp", LedgePulled, true, anim::Options{.transition={.time=0}})); // player reached top of edge and wants to go up
    def.AddTransition(anim::Transition("ledgePullUp", "grounded", false)); // finished getting to top of ledge, now grounded
    def.AddTransition(anim::Transition("ledge", "landing", LedgeLetGo, true, anim::Options{.transition={.time=0}})); // player was climbing but let go
    
    auto animator = anim::Animator{};
    animator.Init("position", TFloat);
    animator.minTotalScale = 1.0f;

    auto machine = anim::Machine(&def, animator);
    machine.Init(update);

    // [ 0- 4] idle
    // [ 5- 9] walk half speed
    // [10-19] run full speed
    // [20-20] jump
    // [21-29] go up and fall down
    // [30-30] land
    // [31-31] continue running
    // [32-32] grab ledge 
    // [33-33] climb up and left
    // [34-34] hang still on edge
    // [35-35] pull yourself up edge
    // [36-xx] you pulled yourself up and are standing idly

    for (int i = 0; i < 40; i++) {
        auto& input = machine.GetInput();

        if (i == 5) { // walk
            input->Set(Input::ForwardSpeed, 0.5f);
        } else if (i == 10) { // run
            input->Set(Input::ForwardSpeed, 1.0f);
        } else if (i == 20) { // jump
            input->Set(Input::Jump, true);
            input->Set(Input::FallingSpeed, 1.0f);
            input->Set(Input::OnGround, false);
        } else if (i > 20 && i < 30) { // up & down
            input->Set(Input::Jump, false);
            input->Set(Input::FallingSpeed, input->Get(Input::FallingSpeed) - 0.2f);
        } else if (i == 30) { // land
            input->Set(Input::OnGround, true);
            input->Set(Input::FallingSpeed, 0.0f);
        } else if (i == 32) { // climb up and left
            input->Set(Input::GrabbingLedge, true);
            input->Set(Input::OnGround, false);
            input->Set(Input::SideSpeed, -1.0f);
            input->Set(Input::ForwardSpeed, 1.0f);
        } else if (i == 34) { // pause climbing
            input->Set(Input::SideSpeed, 0.0f);
            input->Set(Input::ForwardSpeed, 0.0f);
        } else if (i == 35) { // pull self up
            input->Set(Input::PullLedge, true);
        } else if (i == 36) { // standing on ledge that I climbed up
            input->Set(Input::PullLedge, false);
            input->Set(Input::OnGround, true);
            input->Set(Input::GrabbingLedge, false);
        }
        
        // LogLevel = i >= 33 && i <= 36 ? debug : none;

        machine.Update(update);
        machine.Apply(update);
        std::cout << "Frame[" << i << "]: " << animator << std::endl;
    }

    // Thoughts:
    // - I don't like many of the pointers in state.h. I tried to store defs as references but then those types couldn't be used in containers. I feel like shared_ptr is better, but it feels heavy to me.
    // - The above code uses maps. There are more efficient structures that could be used. In previous implementations something like AddState would return an identifier that could be used in AddTransition. It was a simple auto-incrementing value. But what about attributes & animations?
    // - Maybe instead of passing around pointers in state, add state::Registry which holds states and machines that can be referenced by small identifiers?

    return 0;
}

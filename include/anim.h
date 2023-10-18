#pragma once

#include <iostream>
#include <cstring>
#include <cmath>
#include <string>
#include <map>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_set>
#include <iomanip>


#include "state.h"
#include "types.h"
#include "calcs.h"

// The anim namespace stores objects necessary for animating any sort of value.
namespace anim {

    // How to identify attributes, animations, and states.
    using animation_id = std::string;
    using attribute_id = std::string;
    using state_id = std::string;

    // Concepts
    // - Easing = a function to describe animation velocity (the rate at which something moves at a given time/delta between 0 and 1).
    // - Path = converts points/frames into a single point given a time in seconds.
    // - Param = something you can specify to control an animation or transition, and they can stack up.
    // - AnimationOptions = can describe how to animate a single attribute or entire animation
    // - TransitionOptions = can describe how to transition to another animation.
    // - Options = tranisition & animation options for a state machine transition.
    // - Update = the state passed to all the state machine functions that's necessary.
    // - Input = variables a state machine can use to evaluate if a transition should be made.
    // - Attribute = a property on a subject that can be animated.
    // - AttributeAnimation = animation data and options to animate a specific attribute.
    // - Animation = a named collection of AttributeAnimation and options.
    // - Animator = something that animates a subject.
    // - AnimatorAnimation = an animation an animator is playing and it's current state.

    // TODO
    // - More easings.
    // - Proper transition math given the TransitionOptions available.
    // - A TypeAnimator is given a reflect::Value and machine definition and will animate its properties upon update.
    // - Documentation.
    // - Refactor Param.
    // - Refactor AnimationOptions.
    // - Refactor TransitionOptions.
    // - Refactor Options.
    // - Refactor Animation/Attribute structs into classes.
    // - Remove logging.
    // - Full stop/pause/resume operations.
    // - Queue animations. 
    // - Switch over strings to a tag.

    using Easing = std::function<float(float)>;
    float Linear(float d) { return d; }
    float Quad(float d) { return d * d; }
    float Ease(float d, Easing easer) { return easer ? easer(d) : d; }
    Easing JoinEasing(Easing a, Easing b) { return a && b ? [a, b](float d) -> float { return b(a(d)); } : (a ? a : b); }

    struct Point {
        float time;
        Easing easing;
        types::Value data;
    };

    using Path = std::function<void(const std::vector<Point>&, float, types::Value&)>;
    void PointPath(const std::vector<Point>& points, float time, types::Value& out) {
        out.Set(points[0].data);
    }
    void TweenPath(const std::vector<Point>& points, float time, types::Value& out) {
        auto c = calc::For(out.GetType());
        auto easingDelta = Ease(time, points[0].easing);
        out.Set(c->Lerp(points[0].data, points[1].data, easingDelta));
    }
    void LinearPath(const std::vector<Point>& points, float time, types::Value& out) {
        auto n = points.size() - 1;
        for (auto i = 1; i <= n; i++) {
            auto& curr = points[i];
            if (time < curr.time) {
                auto& prev = points[i-1];
                auto delta = (time - prev.time) / (curr.time - prev.time);
                auto easingDelta = Ease(delta, prev.easing);
                auto c = calc::For(out.GetType());
                out.Set(c->Lerp(prev.data, curr.data, easingDelta));
                break;
            }
        }
    }
    void QuadraticPath(const std::vector<Point>& points, float d1, types::Value& out) {
        auto d2 = d1 * d1;
        auto i1 = 1 - d1;
        auto i2 = i1 * i1;

        auto c = calc::For(out.GetType());
        c->Zero(out);
        out.Set(c->Adds(out, points[0].data, i2));
        out.Set(c->Adds(out, points[1].data, 2 * i1 * d1));
        out.Set(c->Adds(out, points[2].data, d2));
    }
    void CubicPath(const std::vector<Point>& points, float d1, types::Value& out) {
        auto d2 = d1 * d1;
        auto d3 = d1 * d2;
        auto i1 = 1 - d1;
        auto i2 = i1 * i1;
        auto i3 = i1 * i2;

        auto c = calc::For(out.GetType());
        c->Zero(out);
        out.Set(c->Adds(out, points[0].data, i3));
        out.Set(c->Adds(out, points[1].data, 3 * i2 * d1));
        out.Set(c->Adds(out, points[2].data, 3 * i1 * d2));
        out.Set(c->Adds(out, points[3].data, d3));
    }

    enum class ParamType { unset, set, add, multiply };

    template<typename T>
    struct Param {
        T value;
        ParamType type;

        Param(): value(T()), type(ParamType::unset) {}
        Param(T v): value(std::move(v)), type(ParamType::set) {}
        Param(T v, ParamType t): value(std::move(v)), type(t) {}

        void Set(T v, ParamType t) { value = std::move(v); type = t; }
        Param<T>& operator=(T v) { Set(v, ParamType::set); return *this; }
        Param<T>& operator+=(T v) { Set(v, ParamType::add); return *this; }
        Param<T>& operator*=(T v) { Set(v, ParamType::multiply); return *this; }
        T Get(T defaultValue) const { return Param::Joins(defaultValue, {*this}); }
        Param<T> Join(T defaultValue, const Param<T>& o) const {
            if (type == ParamType::unset) return o;
            if (o.type == ParamType::unset) return *this;

            return Param<T>(Param::Joins(defaultValue, {*this, o}));
        }

        static T Joins(T defaultValue, std::initializer_list<Param<T>> params) {
            auto value = defaultValue;
            for (auto &param : params) {
                switch (param.type) {
                case ParamType::set: value = param.value; break;
                case ParamType::add: value += param.value; break;
                case ParamType::multiply: value *= param.value; break;
                case ParamType::unset: /* ignore*/ break;
                }
            }
            return value;
        }
    };

    struct AnimationOptions {
        Param<float> delay; // seconds before animation starts
        Param<float> duration; // how longs animation lasts
        Param<float> sleep; // sleeps between animating
        Param<int> repeat; // repeat how many times? (-1=forever)
        Param<float> scale; // how much to scale the animation relative to its default/resting value.
        Param<float> clipStart; // seconds to start in the animation data, excluding everything before this
        Param<float> clipEnd; // seconds to end in the animation data, excluding everything before this
        Path path; // convert animation data into a single point given a time (0->1)
        Easing easing; // animation velocity function

        AnimationOptions Join(const AnimationOptions& next) const noexcept {
            auto o = AnimationOptions{};
            o.delay = delay.Join(0, next.delay);
            o.duration = duration.Join(0, next.duration);
            o.sleep = sleep.Join(0, next.sleep);
            o.repeat = repeat.Join(1, next.repeat);
            o.clipStart = clipStart.Join(0, next.clipStart);
            o.clipEnd = clipEnd.Join(1, next.clipEnd);
            o.scale = scale.Join(1, next.scale);
            o.path = next.path ? next.path : path;
            o.easing = JoinEasing(easing, next.easing);
            return o;
        }
        
        friend std::ostream& operator<<(std::ostream& os, const AnimationOptions& a) {
            os << a.scale.Get(1.0f);
            return os;
        }
    };
    struct TransitionOptions{ 
        Param<float> time; // how long it should take to transition
        Param<float> intro; // how many seconds into the next animation should we smoothly transition into? 0=transition into start of next animation
        Param<float> outro; // how many seconds past point in current animation should we smoothly transition out of? 0=transition from current value
        Param<float> lookup; // intro can be negative to smoothly transition to first point, this is the number of seconds
        Param<int> granularity; // specify > 2 here and the outro & intro animation velocity will be maintained using this number of points 
        Easing easing; // an easing to use along the transition path

        TransitionOptions Join(const TransitionOptions& next) const noexcept  {
            auto o = TransitionOptions{};
            o.time = time.Join(0, next.time);
            o.intro = intro.Join(0, next.intro);
            o.outro = outro.Join(0, next.outro);
            o.lookup = lookup.Join(0, next.lookup);
            o.granularity = granularity.Join(0, next.granularity);
            o.easing = JoinEasing(easing, next.easing);
            return o;
        }
    };
    struct Options {
        TransitionOptions transition;
        AnimationOptions animation;

        Options Join(const Options& next) const noexcept {
            auto o = Options{};
            o.transition = transition.Join(next.transition);
            o.animation = animation.Join(next.animation);
            return o;
        }
    };

    struct AnimationAttribute {
        attribute_id attribute;
        AnimationOptions options;
        std::vector<Point> points;
    };
    struct Animation {
        animation_id name;
        AnimationOptions options;
        std::vector<AnimationAttribute> attributes;
    };

    struct AttributeAnimator {
        animation_id animation;
        const AnimationAttribute* attribute;
        AnimationOptions options;
        float delay; // seconds before animation starts
        float duration; // how longs animation lasts
        float sleep; // sleeps between animating
        int repeat; // repeat how many times? (-1=forever)
        float offset; // seconds to start in the animation data, excluding everything before this
        float clipStart;  // seconds to start in the animation data, excluding everything before this
        float clipEnd; // seconds to the end in the aninmation data, excluding everything after this
        float scale; // how much to scale the animation relative to its default/resting value
        float time; // how many seconds the animation has been playing
        float stopAt; // if > 0, stop here
        bool done;
        bool apply;
        float applyDelta;

        AttributeAnimator() = default;
        AttributeAnimator(const animation_id& anim, const AnimationAttribute* attr, const AnimationOptions opts): 
            animation(anim), attribute(attr), options(opts)
        {
            ApplyOptions();
            time = 0.0f;
            done = duration == 0 || repeat == 0;
            stopAt = -1;
            apply = false;
            applyDelta = 0;
        }
        constexpr float IterationTime() const noexcept { return duration + sleep; }
        constexpr float IntoAnimation(float t) const noexcept { return t - delay; }
        constexpr float MaxLifetime() const noexcept { return repeat < 0 ? -1 : delay + duration + (repeat - 1) * IterationTime(); }
        constexpr float ActualLifetime() const noexcept { return stopAt >= 0 ? stopAt : MaxLifetime(); }
        constexpr float AnimationTime(float t) const noexcept { return fmod(IntoAnimation(t), IterationTime()); }
        constexpr float Delta(float t) const noexcept { return AnimationTime(t) / duration; }
        constexpr float ApplyDelta(float t) const noexcept { auto d = Delta(t); return d < 0 || d > 1 ? -1 : Ease(calc::Lerp<float>(clipStart, clipEnd, d), options.easing); }
        constexpr bool IsEffective() const noexcept { return scale != 0; }
        constexpr int Iteration(float t) const noexcept { return t < delay ? -1 : floor(IntoAnimation(t) / IterationTime()); }
        constexpr bool IsStopping() const noexcept { return stopAt != -1; }
        void Update(float dt) {
            if (done) return;
            auto next = time + dt;
            auto nextApplyDelta = ApplyDelta(next);
            auto end = ActualLifetime();
            
            time = next;
            apply = nextApplyDelta != -1 || applyDelta != -1;
            applyDelta = nextApplyDelta != -1 ? nextApplyDelta : 1;
            done = end != -1 && time >= end;

            LOG("DataAnimator::Update time: "<<time<<" apply: "<<apply<<" applyDelta: "<<applyDelta<<" done: "<<done<<" scale: "<<scale)
        }
        void StopIn(float dt) noexcept {
            stopAt = time + dt;
        }
        void AddOptions(const AnimationOptions& additional) {
            options = options.Join(additional);
            ApplyOptions();
        }
        void ApplyOptions() {
            delay = options.delay.Get(0);
            duration = options.duration.Get(0);
            sleep = options.sleep.Get(0);
            repeat = options.repeat.Get(1);
            clipStart = options.clipStart.Get(0);
            clipEnd = options.clipEnd.Get(1);
            scale = options.scale.Get(1);
        }
    };
    struct Attribute {
        std::vector<AttributeAnimator> animators;
        long frame;
        long lastUpdatedFrame;

        Attribute() = default;

        void Update(float dt, types::Value& value) {
            auto c = calc::For(value.GetType());
            auto temp1 = c->Create();
            auto temp2 = c->Create();

            frame++;

            auto animator = animators.begin();
            while (animator != animators.end()) {
                animator->Update(dt);
                if (animator->apply) {
                    auto scale = animator->scale;
                    if (scale > 0) {
                        auto& path = animator->options.path;
                        if (!path) {
                            path = LinearPath;
                        }
                        path(animator->attribute->points, animator->applyDelta, temp1);
                        temp2.Set(c->Adds(temp2, temp1, scale));

                        lastUpdatedFrame = frame;
                    }
                    LOG("Attribute::Update applying for "<<animator->animation<<"."<<animator->attribute->attribute<<" with scale "<<scale)
                }

                if (animator->done) {
                    LOG("Attribute::Update animator for "<<animator->animation<<"."<<animator->attribute->attribute<<" done after "<<animator->time)

                    animators.erase(animator);
                } else {
                    animator++;
                }
            }

            if (lastUpdatedFrame == frame) {
                value.Set(temp2);
            }
        }
        bool WasUpdated() {
            return lastUpdatedFrame == frame;
        }
        bool IsAnimating(const animation_id& animation) const noexcept {
            for (auto& animator : animators) {
                if (!animator.done && animator.animation == animation) {
                    return true;
                }
            }
            return false;
        }
        void ApplyOptions(const animation_id& animation, const AnimationOptions& weight) {
            for (auto animator = animators.rbegin(); animator != animators.rend(); ++animator) {
                if (!animator->done && animator->animation == animation) {
                    LOG("Attribute::ApplyOptions "<<animation<<" "<<weight)
                    animator->AddOptions(weight);
                    return;
                }
            }
        }
        void StopIn(const animation_id& animation, float dt) noexcept {
            for (auto& animator : animators) {
                if (!animator.done && animator.animation == animation) {
                    animator.StopIn(dt);
                }
            }
        }
        auto ForAnimations(const std::unordered_set<animation_id>& animations) {
            auto found = std::vector<AttributeAnimator*>();
            for (auto& animator : animators) {
                if (!animator.done && animations.find(animator.animation) != animations.end()) {
                    found.push_back(&animator);
                }
            }
            return found;
        }

        friend std::ostream& operator<<(std::ostream& os, const Attribute& a) {
            for (auto& anim : a.animators) {
                if (anim.IsEffective()) {
                    os << anim.animation << "(scale=" << anim.scale << ", time=" << anim.time << ") ";
                }
            }
            // for (auto& anim : a.animators) {
            //     if (anim.weight <= 0) {
            //         os << anim.data.animation << "(inactive, time=" << anim.time << ") ";
            //     }
            // }
            return os;
        }
    };

    using AnimateRequest = std::tuple<const Animation&, AnimationOptions>;

    struct AttributeSet {
        std::map<attribute_id, Attribute> set;

        AttributeSet() = default;
        AttributeSet(const std::vector<AnimateRequest>& requests) {
            for (auto& request : requests) {
                auto [anim, options] = request;
                LOG("AttributeSet::AttributeSet handling request "<<anim.name<<" with attributes: "<<anim.attributes.size())
                for (auto& animAttr : anim.attributes) {
                    auto resolvedOptions = anim.options.Join(animAttr.options).Join(options);
                    auto existing = GetAttribute(animAttr.attribute);
                    if (existing == nullptr) {
                        set[animAttr.attribute] = Attribute{};
                        existing = GetAttribute(animAttr.attribute);
                    }
                    existing->animators.emplace_back(
                        anim.name,
                        &animAttr,
                        resolvedOptions
                    );
                }
            }
        }
        Attribute* GetAttribute(const attribute_id& name) {
            auto iter = set.find(name);
            return iter == set.end() ? nullptr : &iter->second;
        }
        const Attribute* GetAttribute(const attribute_id& name) const {
            auto iter = set.find(name);
            return iter == set.end() ? nullptr : &iter->second;
        }
        bool IsAnimating(const animation_id& animation) const noexcept {
            for (auto& attr : set) {
                if (attr.second.IsAnimating(animation)) {
                    return true;
                }
            }
            return false;
        }
        void Transition(const AttributeSet& attrs, const TransitionOptions& options, const std::unordered_set<animation_id>& outro) {
            for (auto& attr : attrs.set) {
                if (attr.second.animators.empty()) {
                    continue;
                }

                auto existing = GetAttribute(attr.first);

                LOG("AttributeSet::Transition "<<attr.first<<" new animators: "<<attr.second.animators.size())
                if (existing == nullptr || existing->animators.empty()) {
                    set[attr.first] = attr.second;
                } else {
                    auto forOutro = existing->ForAnimations(outro);
                    if (forOutro.size() > 0) {
                        auto& nextUp = attr.second.animators;
                        float minDelay = nextUp[0].delay;
                        for (int i = 1; i < nextUp.size(); i++) {
                            float animatorDelay = nextUp[i].delay;
                            if (animatorDelay < minDelay) {
                                minDelay = animatorDelay;
                            }
                        }

                        for (auto& animator : forOutro) {
                            animator->StopIn(minDelay);
                        }
                        LOG("AttributeSet::Transition stopped "<<forOutro.size()<<" in "<<minDelay)
                    }
                    // TODO insert in any transition animators based on transition options
                    // see: https://github.com/anim8js/anim8js/blob/master/src/AttrimatorMap.js#L269
                    existing->animators.insert(
                        existing->animators.end(), 
                        attr.second.animators.begin(),
                        attr.second.animators.end()
                    );
                }
            }

            for (auto& attr : set) {
                auto forOutro = attr.second.ForAnimations(outro);
                for (auto& animator : forOutro) {
                    if (!animator->IsStopping()) {
                        animator->StopIn(0);
                    }
                }
            }
        }
        void Update(float dt, std::map<attribute_id, types::Value>& values) {
            for (auto& attr : set) {
                auto value = values.find(attr.first);
                if (value != values.end()) {
                    attr.second.Update(dt, value->second);
                }
            }
        }
        void ApplyOptions(const animation_id& animation, const AnimationOptions& weight) {
            for (auto& attr : set) {
                attr.second.ApplyOptions(animation, weight);
            }
        }
        void StopIn(const animation_id& animation, float dt) {
            for (auto& attr : set) {
                attr.second.StopIn(animation, dt);
            }
        }
    };


    struct AnimationDefinition {

    };

    struct Animator {
        AttributeSet attributes;
        std::map<attribute_id, types::Value> values;
        float minTotalScale;
        float maxTotalScale;
        float minEffectiveScale;

        Animator() = default;

        bool IsAnimating(const animation_id& animation) const noexcept {
            return attributes.IsAnimating(animation);
        }
        void Transition(const std::vector<AnimateRequest>& requests, const TransitionOptions& options, const std::unordered_set<animation_id>& outro) {
            auto transitionAttributes = AttributeSet(requests);
            LOG("Animator::Transition "<<requests.size()<<" to attributes: "<<transitionAttributes.set.size())
            attributes.Transition(transitionAttributes, options, outro);
        }
        void ApplyOptions(const animation_id& animation, const AnimationOptions& weight) {
            attributes.ApplyOptions(animation, weight);
        }
        void Update(float dt) {
            attributes.Update(dt, values);
        }
        void Play(const AnimateRequest& request) {
            auto requests = std::vector<AnimateRequest>();
            requests.push_back(request);
            auto options = anim::TransitionOptions{};
            Transition(requests, options, std::unordered_set<animation_id>());
        }
        void Play(const Animation& animation, const AnimationOptions& options) {
            Play(AnimateRequest(animation, options));
        }
        void Play(const Animation& animation) {
            auto options = anim::AnimationOptions{};
            Play(animation, options);
        }
        void StopIn(const animation_id& animation, float dt) {
            attributes.StopIn(animation, dt);
        }
        // Stop/Pause/Resume can be done via animation(s) or attribute(s).
        void Set(const attribute_id& attribute, types::Value value) {
            values[attribute] = value;
        }
        void Init(const attribute_id& attribute, types::Type* type) {
            Set(attribute, type->Create());
        }
        types::Value Get(const attribute_id& attribute) {
            auto iter = values.find(attribute);
            return iter == values.end() ? types::Value::Invalid() : iter->second;
        }

        friend std::ostream& operator<<(std::ostream& os, const Animator& a) {
            for (auto& attrValue : a.values) {
                auto point = attrValue.second;
                os << attrValue.first << "{" << point.ToString() << "}";

                auto attr = a.attributes.GetAttribute(attrValue.first);
                if (attr != nullptr) {
                    os << " = " << *attr;
                }
            }
            return os;
        }
    };

    // Animation state machine types
    struct Update {
        static const inline state::UserStateProperty<float> DeltaTime = 0;
    };
    state::UserState NewUpdate() {
        return state::UserState(1);
    }

    using StateTypes = state::Types<state_id, Animator, Animation, state::UserState, Options, state::UserState, AnimationOptions>;
    using State = state::Active<StateTypes>;
    using StateDefinition = state::Definition<StateTypes>;
    using Machine = state::Machine<StateTypes>;
    using MachineDefinition = state::MachineDefinition<StateTypes>;
    using MachineOptions = state::MachineOptions<StateTypes>;
    using Transition = state::Transition<StateTypes>;
    
    // Start the given state, and outro the given state (if given).
    // The state could be a single animation or a sub machine with any number of states and sub machines.
    // If it has sub states they each will have Start called after this, no iteration necessary.
    // Outro will be given when the parent state is being started, so take that into consideration when
    // doing transitions from outro & intro animations.
    bool Start(Animator& subject, const State& state, const Transition& trans, const State* outro) {
        auto transitionRequests = std::vector<AnimateRequest>();
        auto& transitionOptions = trans.GetOptions();
        auto transitionOutro = std::unordered_set<animation_id>();

        LOG("Start "<<state.GetDefinition()->GetID())

        if (!state.HasSub()) {
            auto def = state.GetDefinition();
            auto& animation = def->GetData();
            auto weight = state.GetWeight();
            auto animationOptions = def->GetOptions().animation.Join(transitionOptions.animation).Join(weight);
            
            transitionRequests.emplace_back(animation, animationOptions);
        }

        LOG("Start "<<state.GetDefinition()->GetID()<<" requests "<<transitionRequests.size())

        if (outro != nullptr) {
            outro->Iterate([&](const State& state) -> bool {
                auto& animation = state.GetDefinition()->GetData().name;
                transitionOutro.insert(animation);
                return true;
            });
        }

        LOG("Start "<<state.GetDefinition()->GetID()<<" outros "<<transitionOutro.size())

        subject.Transition(transitionRequests, transitionOptions.transition, transitionOutro);
        
        return true;
    }
    // Apply the given active states to the subject and update it.
    void Apply(Animator& subject, const std::vector<State*>& active, const state::UserState& update) {
        // Get total scale.
        float totalEffectiveScale = 0.0f;
        for (auto state : active) {
            state->Iterate([&totalEffectiveScale, &subject](const State& state) -> bool {
                float scale = state.GetWeight().scale.Get(1.0f);
                if (scale > subject.minEffectiveScale) {
                    totalEffectiveScale += scale;
                }
                return true;
            });
        }

        // This is preferential, this will allow a total weight < 1. Remove it to make sure total weight across all animations is 1.0.
        float scaleModifier = 1.0f;
        if (totalEffectiveScale == 0.0f) {
            // do nothing, we don't have anything to apply
        } else if (totalEffectiveScale < subject.minTotalScale) {
            scaleModifier = subject.minTotalScale / totalEffectiveScale;
        } else if (totalEffectiveScale > subject.maxTotalScale && subject.maxTotalScale != 0) {
            scaleModifier = subject.maxTotalScale / totalEffectiveScale;
        }

        // Apply normalized weight
        for (auto state : active) {
            state->Iterate([scaleModifier, &subject, &update](const State& state) -> bool {
                auto& animation = state.GetDefinition()->GetData().name;
                auto weight = state.GetWeight();
                float scale = weight.scale.Get(1.0f);

                if (scale <= subject.minEffectiveScale) {
                    scale = 0;   
                }

                weight.scale = scale * scaleModifier;
                subject.ApplyOptions(animation, weight);
                return true;
            });
        }
        
        // Update subject.
        LOG("Apply given "<<active.size()<<" active with "<<subject.attributes.set.size()<<" attributes and dt "<<update.Get(Update::DeltaTime))
        subject.Update(update.Get(Update::DeltaTime));
        LOG("Apply done")
    }

    // If the state is done, it's non-live transitions can be evaluated.
    bool IsDone(const Animator& subject, const State& state) {
        auto& animation = state.GetDefinition()->GetData().name;
        
        return !subject.IsAnimating(animation);
    }

    // A definition for the root machine.
    MachineDefinition NewDefinition(state::UserState input, MachineOptions options = MachineOptions{}) {
        return MachineDefinition(Start, Apply, IsDone, input, options);
    }

    // A definition for a sub machine.
    MachineDefinition NewSubDefinition(MachineOptions options = MachineOptions{}) {
        return MachineDefinition(Start, Apply, IsDone, options);
    }
}
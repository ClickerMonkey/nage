#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <vector>
#include <memory>
#include <variant>
#include <ranges>
#include <bits/stdc++.h>

#include "debug.h"
#include "core.h"

// The state namespace stores a generic state machine.
// This state machine can operate fuzzy or finite.
// A fuzzy machine can blend states. A state in the machine
// can also be a machine and have sub-states.
namespace state {
    
    // Concepts
    // - Subject = The thing that has states and can move through them.
    // - Data = The state data that is applied to the subject.
    // - Input = Environmental data that can trigger transitions.
    // - Options = Variables that control how the state data is applied or is transitioned.
    // - Condition = Given environmental data, should a state be transitioned to?
    // - Transition = Given a start state (optional), end state, and condition - it controls which state(s) are moved to next.
    // - Machine = Defines states, transitions, and all logic necessary to update a subject.
    // - Instance = An instance of a machine, moving a subject through state(s).

    // Using animation as an example of a use-case for state:
    // - Subject = Animator
    // - Data = Animation of 1 or more attributes of the subject
    // - Input = Values like "onGround", "lookAt", "moveDirection", "moveSpeed"
    // - Options = Has information like animation speed, looping, and transition details.

    // TODO
    // - Add machine registry & id so you don't have to deal with references to machine instances or machines?
    // - TODO rename effect to something better? Effect?

    // A definition for a state a machine can be in.
    template<typename T>
    struct Definition;

    // A state the machine can be in.
    template<typename T>
    struct Active;

    // An instance of a machine.
    template<typename T>
    struct Machine;

    // If condition is true, transition from start to end.
    template<typename T>
    struct Transition;

    // A definition for a machine. The states it can be in, the default input, and how it handles
    // Starting states, ending states, and applying active states.
    template<typename T>
    class MachineDefinition;

    // When you define a state machine you define a traits class
    template<typename ID, typename Subject, typename Data, typename Input, typename Options, typename UpdateState, typename Effect>
    struct Types {
        // Core types
        using this_type = Types<ID, Subject, Data, Input, Options, UpdateState, Effect>;
        using id_type = ID;
        using subject_type = Subject;
        using data_type = Data;
        using input_type = Input;
        using option_type = Options;
        using update_type = UpdateState;
        using effect_type = Effect;

        // Given input and metadata, should something happen?
        // This will be called for live transitions every update
        // and for all other transitions when a state is done. Conditions
        // can be wrapped to handle throttling, etc.
        using condition_type = std::function<bool(const input_type&, const update_type&)>;

        // Given input and metadata, how much sould a state weigh for fuzzy machines? 
        // For finite machines with multiple states to choose from, the greatest effect is chosen.
        using get_effect_type = std::function<effect_type(const input_type&, const update_type&)>;

        // Given a subject and state, is the state done?
        // Once all states are done all transitions without a start are evaluated to see what to do next.
        // This function is invoked every update for every active state when a machine is updated.
        using done_type = std::function<bool(const subject_type&, const Active<this_type>&)>;

        // The state has been started, do what's needed to the subject.
        using start_type = std::function<bool(subject_type&, const Active<this_type>&, const Transition<this_type>&, const Active<this_type>* outro)>;

        // Apply the active states to the subject. The passed states may have effects that add up to any number, it's up
        // to the implementation to adjust the effects before using them (ex: normalize them).
        using apply_type = std::function<void(subject_type&, const std::vector<Active<this_type>*>&, const update_type&)>;

        // A function for sorting states. Return true if a should be sorted before b.
        using sort_type = std::function<bool(const Active<this_type>&, const Active<this_type>&)>;
    };

    // Produces a condition that's only true when all conditions are true
    template<typename condition_type>
    constexpr auto And(condition_type a){
        return a;
    }
    // Produces a condition that's only true when all conditions are true
    template<typename condition_type, typename ...condition_types>
    constexpr auto And(condition_type a, condition_types... b){
        return [a, remain = And(b...)](auto value){ return a(value) && remain(value);};
    }

    // Produces a condition that's true when any condition is true
    template<typename condition_type>
    constexpr auto Or(condition_type a){
        return a;
    }
    // Produces a condition that's true when any condition is true
    template<typename condition_type, typename ...condition_types>
    constexpr auto Or(condition_type a, condition_types... b){
        return [a, remain = Or(b...)](auto value){ return a(value) || remain(value);};
    }

    // Produces a condition that's true when all conditions are false
    template<typename condition_type>
    constexpr auto Not(condition_type a){
        return a;
    }
    // Produces a condition that's true when all conditions are false
    template<typename condition_type, typename ...condition_types>
    constexpr auto Not(condition_type a, condition_types... b){
        return [a, remain = Not(b...)](auto value){ return !(a(value) || remain(value));};
    }

    // If condition is true, transition from start to end.
    template<typename T>
    class Transition {
        using id_t        = typename T::id_type;
        using condition_t = typename T::condition_type;
        using option_t    = typename T::option_type;
        using input_t     = typename T::input_type;
        using update_t    = typename T::update_type;
    public:

        // Transition from any state into the end state instantly.
        Transition(id_t end):
            m_hasStart(false), m_start(), m_end(std::move(end)), m_condition(), m_live(false), m_options()  {}
        // Transition from any state into the end state given a condition.
        Transition(id_t end, condition_t condition):
            m_hasStart(false), m_start(), m_end(std::move(end)), m_condition(std::move(condition)), m_live(false), m_options() {}
        // Transition from any state into the end state given a condition (with options).
        Transition(id_t end, condition_t condition, bool live, option_t options):
            m_hasStart(false), m_start(), m_end(std::move(end)), m_condition(std::move(condition)), m_live(live), m_options(std::move(options)) {}
        // Transition from start to end when condition is true.
        Transition(id_t start, id_t end, condition_t condition, bool live, option_t options):
            m_hasStart(true), m_start(std::move(start)), m_end(std::move(end)), m_condition(std::move(condition)), m_live(live), m_options(std::move(options)) {}
        // Transition from start to end automatically (with options).
        Transition(id_t start, id_t end, bool waitForDone, option_t options):
            m_hasStart(true), m_start(std::move(start)), m_end(std::move(end)), m_condition(), m_live(!waitForDone), m_options(std::move(options)) {}
        // Transition from start to end instantly.
        Transition(id_t start, id_t end, bool waitForDone):
            m_hasStart(true), m_start(std::move(start)), m_end(std::move(end)), m_condition(), m_live(!waitForDone), m_options() {}

        // Returns if the transition has a start. A transition without a start is considered a global transition and is stored on the machine definition
        // instead of the state definition. It's evaluated no matter what the current state is.
        constexpr auto HasStart() const noexcept { return m_hasStart; }
        // The id of the starting state (if any).
        constexpr auto GetStart() const noexcept { return m_start; }
        // The id of the ending state.
        constexpr auto GetEnd() const noexcept { return m_end; }
        // The condition on the transition. This is optional, and when not given it's considered to always return true.
        constexpr auto GetCondition() const noexcept { return m_condition; }
        // The options specified on the transition, to be passed to the start function.
        constexpr const auto& GetOptions() const noexcept { return m_options; }
        // Returns whether this transition is live. A live transition is checked each machine update. A non-live transition
        // is only checked if the starting state of the transition is done (as defined by the done function).
        constexpr auto IsLive() const noexcept { return m_live; }
        // Evalutes the input and update and returns whether this transition should be done.
        constexpr auto Eval(const input_t& input, const update_t& update) const { return !m_condition || m_condition(input, update); }

    private:
        const bool m_hasStart;
        const id_t m_start; // optional
        const id_t m_end; // end state
        const condition_t m_condition; // transition to end?
        const bool m_live; // actively check each update (false=only when start/any state finishes)
        const option_t m_options; // transition options
    };

    // A definition for a state.
    template<typename T>
    class Definition {
        using id_t         = typename T::id_type;
        using option_t     = typename T::option_type;
        using input_t      = typename T::input_type;
        using update_t     = typename T::update_type;
        using get_effect_t = typename T::get_effect_type;
        using effect_t     = typename T::effect_type;
        using data_t       = typename T::data_type;
    public:

        // A state with no effect calculation.
        Definition(id_t id, data_t data, effect_t fixedEffect): 
            m_id(id), m_data(std::move(data)), m_fixedEffect(fixedEffect), m_effect(), m_effectLive(false), m_options(), m_sub(nullptr), m_transitions() {}
        // A state with no effect calculation but with options.
        Definition(id_t id, data_t data, effect_t fixedEffect, option_t options): 
            m_id(id), m_data(std::move(data)), m_fixedEffect(fixedEffect), m_options(std::move(options)), m_effect(), m_effectLive(false), m_sub(nullptr), m_transitions() {}
        // A state with a effect calculation.
        Definition(id_t id, data_t data, get_effect_t effect, bool effectLive): 
            m_id(id), m_data(std::move(data)), m_fixedEffect(), m_effect(std::move(effect)), m_effectLive(effectLive), m_options(), m_sub(nullptr), m_transitions() {}
        // A state with a effect calculation and options.
        Definition(id_t id, data_t data, get_effect_t effect, bool effectLive, option_t options): 
            m_id(id), m_data(std::move(data)), m_fixedEffect(), m_effect(std::move(effect)), m_effectLive(effectLive), m_options(std::move(options)), m_sub(nullptr), m_transitions() {}
        // A state that actually has a sub machine.
        Definition(id_t id, const MachineDefinition<T> sub): 
            m_id(id), m_sub(std::make_shared<MachineDefinition<T>>(sub)), m_data(), m_fixedEffect(), m_effect(), m_effectLive(false), m_options(), m_transitions() {}
        // A state that actually has a sub machine with options.
        Definition(id_t id, const MachineDefinition<T> sub, option_t options): 
            m_id(id), m_sub(std::make_shared<MachineDefinition<T>>(sub)), m_options(std::move(options)), m_data(), m_fixedEffect(), m_effect(), m_effectLive(false), m_transitions() {}

        // The id of the state definition.
        constexpr auto GetID() const noexcept { return m_id; }
        // The data associated with the state.
        constexpr auto& GetData() const noexcept { return m_data; }
        // Computes the effect for a state given the input & update states.
        // If a effect function is not returned, the fixed effect is used.
        constexpr auto GetEffect(const input_t& input, const update_t& update) const { return m_effect ? m_effect(input, update) : m_fixedEffect; }
        // Returns the fixed effect specified on the state definition. 
        constexpr auto GetFixedEffect() const noexcept { return m_fixedEffect; };
        // Returns whether the effect function should be evaluated each frame. If false it's only evaluated when a state
        // instance is created. If there is no effect function it's never considered live.
        constexpr auto IsEffectLive() const noexcept { return m_effect ? m_effectLive : false; }
        // The options provided for the state. Potentially will be removed in the future since GetData() should store enough information.
        constexpr auto& GetOptions() const noexcept { return m_options; }
        // Returns the transitions that can be made out of this state.
        constexpr auto& GetTransitions() const noexcept { return m_transitions; }
        // Returns the sub-machine definition on this state or nullptr if none exists.
        constexpr const MachineDefinition<T>* GetSub() const noexcept { return m_sub.get(); }

        friend class MachineDefinition<T>;
    private:
        const id_t m_id;
        const data_t m_data; // state data
        const get_effect_t m_effect; // calculates effect
        const effect_t m_fixedEffect; // the effect to use if a effect function is not supplied
        const bool m_effectLive; // if the effect should be calculated each update (false = at start)
        const option_t m_options;
        std::vector<Transition<T>> m_transitions; // transitions out of this state
        const std::shared_ptr<MachineDefinition<T>> m_sub; // a sub machine
    };

    // Options for a machine definition. You can
    template<typename T>
    struct MachineOptions {
    private:
        using sort_t     = typename T::sort_type;
    public:
        // If > 0, only pass this many states to the apply function.
        int AppliedMax;
        // If AppliedMax > 0, use this function to sort the states before grabbing the first AppliedMax. If this is not specified the order of the states will be used.
        sort_t AppliedPriority;
        // If > 0, only keep this number of active states. During transition periods all possible states are evaluated and using the ActivePriority
        // function below they are ordered based on their initial/current state. If ActivePriority is not defined then the queue order is used.
        int ActiveMax;
        // If ActiveMax > 0, use this function to sort the states before grabbing the first ActiveMax. 
        sort_t ActivePriority;
        // All states are always active, no need to do any transition logic.
        bool FullyActive;
        // A queue of possible states are built on update, but processing isn't typically done until the next update. 
        // This allows you to process the queue right at the end of an update and updates the new active statuses.
        bool ProcessQueueImmediately;
    };

    // Returns the options for a finite state machine.
    template<typename T>
    MachineOptions<T> Finite() {
        return MachineOptions<T>{.AppliedMax=1, .ActiveMax=1};
    }

    // A definition for a machine. The states it can be in, the default input, and how it handles
    // Starting states, ending states, and applying active states.
    template<typename T>
    class MachineDefinition {
        using id_t        = typename T::id_type;
        using subject_t   = typename T::subject_type;
        using done_t      = typename T::done_type;
        using start_t     = typename T::start_type;
        using apply_t     = typename T::apply_type;
        using option_t    = typename T::option_type;
        using input_t     = typename T::input_type;
        using update_t    = typename T::update_type;
    public:

        // A machine with the default input value and default options (fuzzy)
        MachineDefinition(start_t start, apply_t apply, done_t done): 
            m_start(std::move(start)), m_apply(std::move(apply)), m_done(std::move(done)), m_initialInput(), m_options(), m_transitions() {}
        // A machine with the default input value and given options
        MachineDefinition(start_t start, apply_t apply, done_t done, MachineOptions<T> options): 
            m_start(std::move(start)), m_apply(std::move(apply)), m_done(std::move(done)), m_initialInput(), m_options(std::move(options)), m_transitions() {}
        // A machine with the given initial input state and default options (fuzzy)
        MachineDefinition(start_t start, apply_t apply, done_t done, input_t initialInput):
            m_start(std::move(start)), m_apply(std::move(apply)), m_done(std::move(done)), m_initialInput(std::move(initialInput)), m_options(), m_transitions() {}
        // A machine with the given initial input state and given options.
        MachineDefinition(start_t start, apply_t apply, done_t done, input_t initialInput, MachineOptions<T> options):
            m_start(std::move(start)), m_apply(std::move(apply)), m_done(std::move(done)), m_initialInput(std::move(initialInput)), m_options(std::move(options)), m_transitions() {}
        
        // The initial input of a machine when one is created with this definition.
        constexpr auto GetInitialInput() const noexcept { return m_initialInput; }
        // Returns whether the subject and given active state is "done". A done state is removed from the active list
        // of states and its non-live transitions are checked.
        constexpr auto IsDone(const subject_t& subject, const Active<T>& state) const noexcept { return m_done(subject, state); }
        // Starts subject, active state, the transition that triggered the start (if applicable), and the active state we might
        // be transitioning out of.
        constexpr auto Start(subject_t& subject, const Active<T>& state, const Transition<T>& trans, const Active<T>* outro) const noexcept { return m_start ? m_start(subject, state, trans, outro) : true; }
        // Applies the results of the state machine to the given subject. It's provided a list of the active states
        // and the update state.
        constexpr void Apply(subject_t& subject, const std::vector<Active<T>*>& active, const update_t& update) const noexcept { if (m_apply) m_apply(subject, active, update); }
        // The options for this machine definition.
        constexpr auto& GetOptions() const noexcept { return m_options; }
        // The states added to the machine definition.
        constexpr auto& GetStates() const noexcept { return m_states; }
        // The global transitions (without a specific start) on the machine definition.
        constexpr auto& GetTransitions() const noexcept { return m_transitions; }
        // Adds a state to the definition.
        constexpr void AddState(Definition<T> state) {
            m_states.push_back(std::move(state));
        }
        // Returns the state with the given identifier.
        Definition<T>* GetState(const id_t id) {
            for (auto& s : m_states) {
                if (s.GetID() == id) {
                    return &s;
                }
            }
            return nullptr;
        }
        // Returns a const state definition with the given identifier.
        const Definition<T>* GetState(const id_t id) const {
            for (auto& s : m_states) {
                if (s.GetID() == id) {
                    return &s;
                }
            }
            return nullptr;
        }
        // Adds a transition to the machine definition. If the end or start state defined on the transition does not
        // exist yet in the machine definition an exception is thrown.
        void AddTransition(Transition<T> trans) {
            if (GetState(trans.GetEnd()) == nullptr) {
                throw std::invalid_argument("end state of transition was not defined on the machine");
            } 
            if (trans.HasStart()) {
                auto start = GetState(trans.GetStart());
                if (start == nullptr) {
                    throw std::invalid_argument("start state of transition was not defined on the machine");
                }
                start->m_transitions.push_back(std::move(trans));
            } else {
                m_transitions.push_back(std::move(trans));
            }
        }

    private:
        std::vector<Definition<T>> m_states;
        std::vector<Transition<T>> m_transitions;
        const input_t m_initialInput;
        const done_t m_done;
        const start_t m_start;
        const apply_t m_apply;
        const MachineOptions<T> m_options;
    };

    // An instance of a machine for a subject.
    template<typename T>
    class Machine {
        using id_t        = typename T::id_type;
        using subject_t   = typename T::subject_type;
        using option_t    = typename T::option_type;
        using input_t     = typename T::input_type;
        using update_t    = typename T::update_type;
    public:

        // A machine with the given definition, for the subject, and optionally a parent machine.
        Machine(const MachineDefinition<T>* def, subject_t& subject, Machine<T>* parent = nullptr):
            m_def(def), 
            m_subject(subject), 
            m_parent(parent), 
            m_input(parent != nullptr ? parent->m_input : std::make_shared<input_t>(def->GetInitialInput())),
            m_active(),
            m_activeQueue()
        {}

        // Returns the parent machine instance (if this is not the root machine).
        constexpr const auto GetParent() const noexcept { return m_parent; }
        // Returns the definition of this machine instance.
        constexpr const auto GetDefinition() const noexcept { return m_def; }
        // Returns the subject of this machine.
        constexpr auto& GetSubject() noexcept { return m_subject; }
        // Returns the current input of this machine. Manipulate this variable to affect the state.
        constexpr auto& GetInput() noexcept { return m_input; }
        // Returns a list of the current active states.
        constexpr const auto& GetActive() const noexcept { return m_active; }
        // Returns a list of the states in a queue to be potentially activated during the next Update call.
        constexpr const auto& GetActiveQueue() const noexcept { return m_activeQueue; }
        // Returns a list of the states that were considered applicable to be applied during the last Apply call.
        constexpr const auto& GetApplicable() const noexcept { return m_applicable; }
        // Returns an active state with the given identifier or null if one doesn't exist.
        Active<T>* GetActive(const id_t id);
        // Evaluates the given list of transitions and adds any states that should be transitioned to
        // to the active queue. 
        int Transitions(const std::vector<Transition<T>>& transitions, const update_t& update, const bool onlyLive, const Active<T>* outro);
        // Initializes the machine if there are no active or queued states.
        void Init(const update_t& update);
        // Updates the machine with the given update state.
        void Update(const update_t& update);
        // Applies the applicable states to the subject with the given update state.
        void Apply(const update_t& update);

    private: 
        // Updates all active states.
        void UpdateActive(const update_t& update);
        // Processes the queue of potential active states.
        void ProcessQueue(const update_t& update);

        const Machine<T>* m_parent;
        const MachineDefinition<T>* m_def;
        subject_t& m_subject;
        std::shared_ptr<input_t> m_input;
        std::vector<Active<T>> m_active;
        std::vector<Active<T>> m_activeQueue;
        std::vector<Active<T>*> m_applicable;
    };

    // An instance of a state.
    template<typename T>
    class Active {
        using subject_t   = typename T::subject_type;
        using input_t     = typename T::input_type;
        using update_t    = typename T::update_type;
        using effect_t    = typename T::effect_type;
        using done_t      = typename T::done_type;
    public:

        // Default constructor
        Active() = default;
        // An active state with the given definition, subject, current state of input & update (used to calculate current effect), and the parent machine.
        Active(const Definition<T>* def, subject_t& subject, const input_t& input, const update_t& update, Machine<T>* parent):
            m_def(def),
            m_effect(def->GetEffect(input, update)),
            m_sub(def->GetSub() ? std::make_unique<Machine<T>>(def->GetSub(), subject, parent) : nullptr) 
        {}

        // The definition of this state instance.
        constexpr const auto GetDefinition() const noexcept { return m_def; }
        // The current effect of this state. 
        constexpr auto GetEffect() const noexcept { return m_effect; }
        // Returns whether this state has a sub-machine instance.
        constexpr auto HasSub() const noexcept { return !!m_sub; }
        // Returns the sub-machine instance (if any).
        constexpr auto& GetSub() const noexcept { return m_sub; }
        // Updates this state. If this has a sub-machine, that's updated. Otherwise if the state
        // has live effect - that's updated.
        void Update(const input_t& input, const update_t& update) {
            if (m_sub) {
                LOG(debug, "Active::Update sub "<<m_def->GetData().name)

                m_sub->Update(update);
            } else {
                if (m_def->IsEffectLive()) {
                    m_effect = m_def->GetEffect(input, update);

                    LOG(debug, "Active::Update effect "<<m_def->GetData().name<<": "<<m_effect)
                }
            }
        }
        // Iterates through all bottom states connected to this state. A bottom state is
        // one that does not have a sub-machine. This will traverse through all sub machines
        // and return the active states.
        bool Iterate(std::function<bool(const Active<T>&)> fn) const {
             if (HasSub()) {
                for (auto& sub : GetSub()->GetActive()) {
                    if (!sub.Iterate(fn)) {
                        return false;
                    }
                }
                return true;
            } else {
                return fn(*this);
            }
        }
        // Returns whether this state appears done. If this state has a sub-machine in order
        // for it to be considered done there should be no states in queue and all sub-states
        // also need to be considered "not done".
        bool IsDone(const subject_t& subject, const MachineDefinition<T>* def) const {
            auto& sub = GetSub();
            if (sub) {
                if (sub->GetActiveQueue().size() > 0) {
                    return false;
                }
                for (auto& subState : sub->GetActive()) {
                    if (!subState.IsDone(subject, def)) {
                        return false;
                    }
                }
                return true;
            } else {
                return def->IsDone(subject, *this);
            }
        }

    private:
        const Definition<T>* m_def;
        effect_t m_effect;
        std::unique_ptr<Machine<T>> m_sub;
    };
    
    template<typename T>
    int Machine<T>::Transitions(const std::vector<Transition<T>>& transitions, const update_t& update, const bool onlyLive, const Active<T>* outro) {
        auto transitioned = 0;
        for (auto& trans : transitions) {
            if (onlyLive && !trans.IsLive()) {
                continue;
            }
            auto end = trans.GetEnd();
            auto active = GetActive(end);
            if (active != nullptr) {
                LOG(warn, "Machine::Transitions cannot transition to, already active: "<<active->GetDefinition()->GetData().name)

                continue;
            }
            auto& input = *m_input;
            if (trans.Eval(input, update)) {
                const auto stateDef = m_def->GetState(end);
                LOG(info, "Machine::Transitions transition to "<<stateDef->GetID())

                Active<T> state(stateDef, m_subject, input, update, this);
                if (state.HasSub()) {
                    LOG(info, "Machine::Transitions init sub machine "<<stateDef->GetID())

                    state.GetSub()->Init(update);
                }
                if (m_def->Start(m_subject, state, trans, outro)) {
                    LOG(info, "Machine::Transitions started "<<stateDef->GetID())

                    m_activeQueue.push_back(std::move(state));
                    transitioned++;
                }
            }
        }
        return transitioned;
    }

    template<typename T>
    Active<T>* Machine<T>::GetActive(const id_t id) {
        for (auto& state : m_active) {
            if (state.GetDefinition()->GetID() == id) {
                return &state;
            }
        }
        return nullptr;
    }

    template<typename T>
    void Machine<T>::Init(const update_t& update) {
        // Only init if there are no possible active states.
        if (!m_active.empty() || !m_activeQueue.empty()) {
            return;
        }

        // Machine options
        auto& options = m_def->GetOptions();

        // Global transitions
        auto& transitions = m_def->GetTransitions();

        // If always live, add all states to m_activeQueue (only if transitions is empty, otherwise use user supplied transition order)
        if (options.FullyActive && transitions.empty()) {
            auto& input = *m_input;
            LOG(info, "Machine::Init starting live states: "<<m_def->GetStates().size())

            for (const auto& stateDef : m_def->GetStates()) {
                LOG(info, "Machine::Init try start "<<stateDef.GetID())

                Active<T> state(&stateDef, m_subject, input, update, this);
                if (state.HasSub()) {
                    LOG(info, "Machine::Init init sub machine "<<stateDef.GetID())

                    state.GetSub()->Init(update);
                }
                Transition<T> trans(stateDef.GetID());
                if (m_def->Start(m_subject, state, trans, nullptr)) {
                    LOG(info, "Machine::Init started "<<stateDef.GetID())

                    m_activeQueue.push_back(std::move(state));
                }
            }
        } else if (!transitions.empty()) {
            LOG(info, "Machine::Init using global transitions")

            Transitions(transitions, update, false, nullptr);
        }
    }

    template<typename T>
    void Machine<T>::ProcessQueue(const update_t& update) {
        // Machine options
        auto& options = m_def->GetOptions();

        // If there are new states queued up waiting to be activated...
        if (!m_activeQueue.empty()) {
            // If the definition has a restricting on the max active states running...
            if (options.ActiveMax > 0) {
                // How many queued states can we add to the current list of active?
                auto remainingSpace = options.ActiveMax - m_active.size();
                if (remainingSpace >= m_activeQueue.size()) {
                    // Do nothing, we have plenty of space.
                } else if (remainingSpace > 0) {
                    // If a user has supplied a custom sorter, use it.
                    if (options.ActivePriority) {
                        LOG(info, "Machine::Update sorting with custom priority function.")

                        std::sort(m_activeQueue.begin(), m_activeQueue.end(), options.ActivePriority);
                    }
                    // Chop off the unwanted states.
                    LOG(info, "Machine::Update chopping off from queue: "<<m_activeQueue.size()-remainingSpace)

                    m_activeQueue.resize(remainingSpace);
                } else {
                    // We can't make any more active.
                    LOG(info, "Machine::Update active list full, unable to add requested: "<<m_activeQueue.size())

                    m_activeQueue.clear();
                }
            }
            // If there are still states to activate...
            if (!m_activeQueue.empty()) {
                for (auto& active : m_activeQueue) {
                    LOG(info, "Machine::Update added to active: "<<active.GetDefinition()->GetData().name)

                    m_active.push_back(std::move(active));
                }
                m_activeQueue.clear();
            }
        }
    }

    template<typename T>
    void Machine<T>::UpdateActive(const update_t& update) {
        // Machine options
        auto& options = m_def->GetOptions();

        // Start here
        auto state = m_active.begin();

        // If all states are always active, no need to do done or transition logic.
        if (options.FullyActive) {
            while (state != m_active.end()) {
                state->Update(*m_input, update);
                state++;
            }
        } else {
            // Otherwise check for done, evaluate transitions, and possible remove a state from being active.
            while (state != m_active.end()) {
                auto done = state->IsDone(m_subject, m_def);
                if (!done) {
                    state->Update(*m_input, update);
                } else {
                    LOG(info, "Machine::Update "<<state->GetDefinition()->GetID()<<" done due to IsDone")
                }

                auto transitioned = Transitions(state->GetDefinition()->GetTransitions(), update, !done, &(*state));

                if (transitioned > 0 && !done) {
                    LOG(info, "Machine::Update "<<state->GetDefinition()->GetID()<<" done due to transition")

                    done = true;
                }

                if (done) {
                    m_active.erase(state);
                } else {
                    state++;
                }
            }
        }
    }

    template<typename T>
    void Machine<T>::Update(const update_t& update) {
        // Machine options
        auto& options = m_def->GetOptions();

        // If not all are not always live, check global transitions
        if (!options.FullyActive) {
            // Do we have any states?
            auto hasState = m_active.size() > 0 || m_activeQueue.size() > 0;

            // Global transitions or initial transitions if there are no states.
            Transitions(m_def->GetTransitions(), update, hasState, nullptr);
        }

        // Process queue into active
        ProcessQueue(update);

        // If we don't worry our selves about transitions, just update all the states.
        UpdateActive(update);

        // Process queue immediately?
        if (options.ProcessQueueImmediately && !m_activeQueue.empty()) {
            // Start updating active at this point...
            auto currentActive = m_active.size();

            // Add queued to active
            ProcessQueue(update);
        }
    }

    template<typename T>
    void Machine<T>::Apply(const update_t& update) {
        // If no active states, exit early.
        if (m_active.empty()) {
            return;
        }
        
        // Machine options
        auto& options = m_def->GetOptions();

        // Copy list to avoid affecting order or size of active states.
        m_applicable.clear();
        for (auto& state : m_active) {
            m_applicable.push_back(&state);
        }
        
        // If there is a restriction on how many to apply and we are over that, lets sort and reduce it.
        if (options.AppliedMax > 0 && options.AppliedMax < m_active.size()) {
            // Sort if a priority function was supplied
            if (options.AppliedPriority) {
                std::sort(m_applicable.begin(), m_applicable.end(), [cmp = options.AppliedPriority](const Active<T>* a, const Active<T>*b) -> bool {
                    return cmp(*a, *b);
                });
            }
            // Remove states off back
            m_applicable.resize(options.AppliedMax);
        }
        
        LOG(debug, "Machine::Applying "<<m_applicable.size()<<" out of "<<m_active.size()<<" active states")

        m_def->Apply(m_subject, m_applicable, update);
    }

    // A typed property in user state.
    template<typename T>
    struct UserStateProperty {
        int Index;

        constexpr UserStateProperty(const int i): Index(i) {}
        constexpr UserStateProperty<T>& operator=(const int i) { Index = i; return *this; }
    };

    // A general purpose user state object that can be used for the Input or UpdateState types.
    // You define a class with static UserStateProperty(s) and use that in the first parameters.
    // You can add speciations for the Get & Set as long as the data type can be converted to and from a float.
    // ```cpp
    // struct Input {
    //    static const inline state::UserStateProperty<bool>  Jump = 0;
    //    static const inline state::UserStateProperty<float> Speed = 1; // -1=backwards, 0=still, 1=forwards
    // };
    // auto u = UserState(2);
    // auto jumping = u.Get(Input::Jump);
    // u.Set(Input::Speed, 0.5f);
    // ```
    class UserState {
    public:
        // No user state
        UserState(): m_data() {}

        // Creates a UserState given the number of possible variables.
        UserState(size_t size): m_data(size) {}

        // Gets the value for the given user state property.
        template<typename T>
        inline T Get(UserStateProperty<T> prop) const {
            return static_cast<T>(m_data[prop.Index]);
        }

        // Sets the value for the given user state property.
        template<typename T>
        inline void Set(UserStateProperty<T> prop, T value) {
            m_data[prop.Index] = static_cast<float>(value);
        }

    private:
        std::vector<float> m_data;
    };

    // Get for bool
    template<>
    inline bool UserState::Get(UserStateProperty<bool> prop) const {
        return m_data[prop.Index] == 1.0f;
    }

    // Set for bool
    template<>
    inline void UserState::Set(UserStateProperty<bool> prop, bool value) {
        m_data[prop.Index] = value ? 1.0f : 0.0f;
    }

}
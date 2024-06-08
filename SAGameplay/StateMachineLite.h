#pragma once

namespace StateMachineImpl
{
#pragma region TinyFSM
	// TinyFSM from LostWorld API: https://github.com/hedge-dev/LWAPI/blob/master/Sonic2013/Hedgehog/Game/TinyFsm.h

#define TiFSM_SIGNAL_SUPER  -1
#define TiFSM_SIGNAL_INIT   -2
#define TiFSM_SIGNAL_ENTER  -3
#define TiFSM_SIGNAL_LEAVE  -4
#define TiFSM_SIGNAL_UPDATE  0
#define TiFSM_SIGNAL_MESSAGE 1
#define TiFSM_SIGNAL_USER    100

	template <typename T, typename TEvent>
	class TTinyFsmState;

	template <typename T, typename TEvent>
	class TTinyFsmEvent;

	template <typename T>
	class TiFsmBasicEvent;

	template <typename T, typename TEvent = TiFsmBasicEvent<T>, const bool Hierarchical = false>
	class TTinyFsm
	{
	private:
		typedef TTinyFsmState<T, TEvent> State_t;

	public:
		typedef TEvent TiFsmEvent_t;
		typedef State_t TiFsmState_t;

	private:
		State_t m_Src;
		State_t m_Cur;

		State_t _top(const TEvent& e) { return {}; }

		State_t Trigger(const State_t& in_state, const TEvent& in_event)
		{
			return in_state(static_cast<T*>(this), in_event);
		}

		State_t Super(const State_t& in_state)
		{
			return Trigger(in_state, { TiFSM_SIGNAL_SUPER });
		}

		State_t Init(const State_t& in_state)
		{
			return Trigger(in_state, { TiFSM_SIGNAL_INIT });
		}

		State_t Enter(const State_t& in_state)
		{
			return Trigger(in_state, { TiFSM_SIGNAL_ENTER });
		}

		State_t Leave(const State_t& in_state)
		{
			return Trigger(in_state, { TiFSM_SIGNAL_LEAVE });
		}

		void InitCurrentState()
		{
			while (Init(m_Cur) == nullptr)
			{
				Enter(m_Cur);
			}
		}

	public:
		inline static State_t FSM_TOP()
		{
			return { &TTinyFsm::_top };
		}

		void FSM_INIT(State_t in_state)
		{
			m_Cur = in_state;
		}

		void FSM_TRANSITION(State_t in_state)
		{
			if (m_Cur == in_state)
				return;

			if constexpr (Hierarchical)
			{
				for (State_t state = m_Cur; state != m_Src; state = Super(state))
				{
					Leave(state);
				}

				if (m_Src == in_state)
				{
					Leave(m_Src);
					Enter(in_state);
				}
				else
				{
					std::vector<State_t> oldStates{};
					std::vector<State_t> newStates{};
					State_t oldState = m_Cur;
					State_t newState = in_state;

					while (oldState != FSM_TOP())
					{
						oldStates.push_back(oldState);
						oldState = Super(oldState);
					}

					while (newState != FSM_TOP())
					{
						newStates.push_back(newState);
						newState = Super(newState);
					}

					// I honestly don't even know what this is supposed to do
					std::reverse(oldStates.begin(), oldStates.end());
					std::reverse(newStates.begin(), newStates.end());

					auto* enumerator = oldStates.begin();
					auto* enumerator2 = newStates.begin();

					while (*enumerator != oldStates[0] && *enumerator2 != newStates[0] && *enumerator == *enumerator2)
					{
						++enumerator;
						++enumerator2;
					}

					State_t oldUniqueState = *enumerator;
					std::reverse(oldStates.begin(), oldStates.end());

					for (auto& state : oldStates)
					{
						Leave(state);
						if (state == oldUniqueState)
							break;
					}

					while (*enumerator2 != newStates[0])
					{
						Enter(*enumerator2);
						++enumerator2;
					}
				}

				m_Cur = in_state;
				InitCurrentState();
			}
			else
			{
				Leave(m_Cur);
				m_Cur = in_state;
				Enter(m_Cur);
			}
		}

		void FSM_SETSTATE(State_t in_state)
		{
			m_Src = m_Cur;
			FSM_TRANSITION(in_state);
		}

		void InitFSM()
		{
			if constexpr (Hierarchical)
			{
				std::vector<State_t> stateStack{};
				State_t fsmState = m_Cur;

				while (fsmState != FSM_TOP())
				{
					stateStack.push_back(fsmState);
					fsmState = Super(fsmState);
				}

				std::reverse(stateStack.begin(), stateStack.end());
				for (auto& state : stateStack)
				{
					Enter(state);
				}

				InitCurrentState();
			}
			else
			{
				Enter(m_Cur);
			}
		}

		void ShutdownFSM()
		{
			if constexpr (Hierarchical)
			{
				State_t state = m_Cur;
				while (state != FSM_TOP())
				{
					Leave(state);
					state = Super(state);
				}

				m_Cur.Clear();
				m_Src.Clear();
			}
			else
			{
				Leave(m_Cur);
				m_Cur.Clear();
			}
		}

		void DispatchFSM(const TEvent& in_event)
		{
			if constexpr (Hierarchical)
			{
				m_Src = m_Cur;

				while (m_Src)
				{
					State_t state = Trigger(m_Src, in_event);
					if (!state)
						break;

					m_Src = Super(m_Src);
				}
			}
			else
			{
				Trigger(m_Cur, in_event);
			}
		}

		TTinyFsm()
			: m_Src(FSM_TOP()),
			m_Cur(nullptr)
		{
		}

		TTinyFsm(State_t in_src)
			: m_Src(FSM_TOP()),
			m_Cur(in_src)
		{
		}

		virtual ~TTinyFsm() = default;
	};

	template <typename T, typename TEvent>
	class TTinyFsmState
	{
	public:
		typedef TTinyFsmState(T::* Delegate_t)(const TEvent& e);
		Delegate_t m_Delegate{};

		TTinyFsmState() = default;

		TTinyFsmState(Delegate_t in_func)
			: m_Delegate(in_func)
		{
		}

		[[nodiscard]] bool IsValid() const
		{
			return m_Delegate != nullptr;
		}

		void Clear()
		{
			m_Delegate = nullptr;
		}

		TTinyFsmState Call(T* in_pObj, const TEvent& e) const
		{
			if (IsValid())
				return (in_pObj->*m_Delegate)(e);

			return {};
		}

		operator bool() const
		{
			return IsValid();
		}

		operator Delegate_t() const
		{
			return m_Delegate;
		}

		bool operator==(const TTinyFsmState& in_other) const
		{
			return m_Delegate == in_other.m_Delegate;
		}

		bool operator!=(const TTinyFsmState& in_other)
		{
			return m_Delegate != in_other.m_Delegate;
		}

		bool operator==(const Delegate_t& in_other) const
		{
			return m_Delegate == in_other;
		}

		bool operator!=(const Delegate_t& in_other) const
		{
			return m_Delegate != in_other;
		}

		TTinyFsmState operator()(T* in_pObj, const TEvent& e) const
		{
			return Call(in_pObj, e);
		}
	};

	template <typename T, typename TEvent>
	class TTinyFsmEvent
	{
	public:
		int m_Sig{};

		TTinyFsmEvent(int in_sig)
			: m_Sig(in_sig)
		{
		}

		int getSignal() const
		{
			return m_Sig;
		}
	};

	template <typename T>
	class TiFsmBasicEvent : public TTinyFsmEvent<T, TiFsmBasicEvent<T>>
	{
	public:
		union
		{
			int m_Int;
			float m_Float;
			Hedgehog::Universe::Message* m_pMessage{};
		};

		TiFsmBasicEvent(int in_sig)
			: TTinyFsmEvent<T, TiFsmBasicEvent>(in_sig)
		{
		}

		TiFsmBasicEvent(int in_sig, float in_float)
			: TTinyFsmEvent<T, TiFsmBasicEvent>(in_sig)
		{
			m_Float = in_float;
		}

		TiFsmBasicEvent(int in_sig, int in_int)
			: TTinyFsmEvent<T, TiFsmBasicEvent>(in_sig)
		{
			m_Int = in_int;
		}

		TiFsmBasicEvent(int in_sig, Hedgehog::Universe::Message& in_msg)
			: TTinyFsmEvent<T, TiFsmBasicEvent>(in_sig)
		{
			m_pMessage = &in_msg;
		}

		[[nodiscard]] float getFloat() const { return m_Float; }
		[[nodiscard]] int getInt() const { return m_Int; }
		[[nodiscard]] Hedgehog::Universe::Message& getMessage() const { return *m_pMessage; }

		inline static TiFsmBasicEvent CreateUpdate(float in_deltaTime)
		{
			return { TiFSM_SIGNAL_UPDATE, in_deltaTime };
		}

		inline static TiFsmBasicEvent CreateMessage(const Hedgehog::Universe::Message& in_msg)
		{
			return { TiFSM_SIGNAL_MESSAGE, in_msg };
		}
	};
#pragma endregion

	// Lighter-weight variation of CStateMachineBase in Sonic Generations, closer to TinyStateMachine but without boost::function.
	template <typename TObject>
	class StateMachineLite : public TTinyFsm<TObject>
	{
	public:
#define STATE_NAME(x) const char* GetName() override { return x; }
		class StateBase : public hh::base::CObject
		{
		public:
			TObject* m_pObject{};
			virtual ~StateBase() = default;

			void Init(TObject* owner)
			{
				m_pObject = owner;
			}

			virtual const char* GetName()
			{
				return "";
			}

			virtual void Enter() {};
			virtual void Update(float deltaTime) {};
			virtual void Leave() {};

			template<typename TState>
			void ChangeState()
			{
				m_pObject->template ChangeState<TState>();
			}
			TObject* GetContext()
			{
				return m_pObject;
			}
		};

		std::unique_ptr<StateBase> m_CurrentState;

		template<typename TState>
		TTinyFsm<TObject>::TiFsmState_t StateFunc(const TiFsmBasicEvent<TObject>& rEvent)
		{
			switch (rEvent.getSignal())
			{
			case TiFSM_SIGNAL_ENTER:
			{
				m_CurrentState.reset(new TState());
				m_CurrentState->Init(static_cast<TObject*>(this));
				m_CurrentState->Enter();
				break;
			}

			case TiFSM_SIGNAL_UPDATE:
			{
				m_CurrentState->Update(rEvent.getFloat());
			}

			case TiFSM_SIGNAL_LEAVE:
			{
				m_CurrentState->Leave();
				break;
			}
			}

			return TTinyFsm<TObject>::FSM_TOP();
		}

		template<typename T>
		void ChangeState()
		{
			TTinyFsm<TObject>::FSM_TRANSITION(&StateMachineLite::StateFunc<T>);
		}

		void UpdateStateMachine(float deltaTime)
		{
			TTinyFsm<TObject>::DispatchFSM(TiFsmBasicEvent<TObject>::CreateUpdate(deltaTime));
		}
	};
}

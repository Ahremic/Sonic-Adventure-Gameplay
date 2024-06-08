#pragma once

namespace SetObjectsCustom
{
	class CObjECRubberBandRail : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;

		// Shitty state machine logic
		// UNDONE: Too shitty
		/*
		class State
		{
		public:
			virtual ~State() = default;
			virtual void Update() = 0;
			virtual void End() = 0;

			CObjECRubberBandRail* m_Context;
			bool m_PrepareDelete = false;

			State(CObjECRubberBandRail* context) : m_Context(context) {}
		};

		class StateWait : public State
		{
		public:
			StateWait(CObjECRubberBandRail* context) : State(context)
			{

			}

			void Update() override
			{
				if (m_Context->m_SonicContext)
					return;

				const float sizeWidth  = 7.5f;
				const float sizeHeight = 1.875f;
				//const float sizeThick  =

				Sonic::CMatrixNodeTransform* node = m_Context->m_spMatrixNodeTransform.get();
				CQuaternion& rotation = node->m_Transform.m_Rotation;

				const CVector forward = rotation * AXIS_FWD;
				const CVector right   = rotation * AXIS_RGHT;
				const CVector up      = rotation * AXIS_UP;

				const CVector sonicPosition = m_Context->m_SonicContext->m_spMatrixNode->m_Transform.m_Position;
				const CVector relativePosition = sonicPosition - node->m_Transform.m_Position;

				if (std::abs(CVector::Dot(relativePosition, right)) < sizeWidth
				&&  std::abs(CVector::Dot(relativePosition, up))    < sizeHeight)
					DebugDrawText::log("SONIC IS IN BOUNDS", 0, 0, {.2, 1, .6, 1});
			}
			void End() override
			{

			}
		};
		*/

	public:
		BB_SET_OBJECT_MAKE("RubberBandRail")

		static const inline CVector AXIS_FWD = CVector(1, 0, 0);
		static const inline CVector AXIS_UP = CVector(0, 1, 0);
		static const inline CVector AXIS_RGHT = CVector(0, 0, 1);

		static constexpr float halfSizeWidth = 7.5f * 0.5f;
		static constexpr float halfSizeHeight = 1.875f * 0.5f;
		static constexpr float activationThickness = 0.4f;
		static constexpr float ropeOffset = 0.4f;

		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable;
		boost::shared_ptr<hh::anim::CAnimationPose> m_spAnimationPose;

		// HOTFIX: use externalcontrol to patch out horrible softlock
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spSonicControlNode;
		CVector m_VelocityExternalControl = CVector::Zero();
		bool m_IsExternalControl = false;


		float m_OffsetSideInitial = 0.0f;
		float m_OffsetSide = 0.0f;
		float m_OffsetStretch = 0.0f;
		bool  m_IsSonicFromBehind = true;

		float m_CurrentTime = 0.0f;
		float m_DeltaTime = 0.0f;
		float m_Sign = 1.0f;

		CVector m_OriginalHorizontalVelocity = CVector::Zero();

		enum EState
		{
			SWait,
			SStretch,
			SPrepareRebound,
			SRebound,
		} m_State = SWait;

		Sonic::Player::CPlayerSpeedContext* m_SonicContext = nullptr;

		// States
		// TODO: Understand how the state machine system works & handle this *there* instead, unless this is actually faster!
		//-----------------

		void ChangeState(EState inState);

		void GenericStateUpdate();

		void EaseRopeToPosition();

		void ReactToPlayer();

		void StateStart_REBOUND() const
		{
			//m_SonicContext->PlaySound(4002017, false);
		}

		void StateStart_PREPARE_REBOUND();

		void StateUpdate_PREPARE_REBOUND();

		void StateUpdate_WAIT();

		void StateUpdate_STRETCH();

		void StateUpdate_REBOUND();

		// Basic stuff
		//------------------

		Sonic::Player::CPlayerSpeedContext* GetPlayer() const;

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override;

		// Need this to be a pretty late message actor to accurately get Sonic's position w/o stutter.
		void SetAddUpdateUnit(Sonic::CGameDocument* in_pGameDocument) override;
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjECRubberBandRail)

}
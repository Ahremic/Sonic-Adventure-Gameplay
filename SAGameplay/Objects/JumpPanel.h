#pragma once

namespace Sonic
{
	class CGlitterPlayer;
}

namespace SetObjectsCustom
{
	// Jump on pannel number 1!
	class CObjPanelVisual : public Sonic::CGameObject3D
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable;

		const char* m_Name;
		CVector     m_Position;
		CQuaternion m_Rotation;

		CObjPanelVisual(const char* name, const CVector& position, const CQuaternion rotation) : m_Name(name), m_Position(position), m_Rotation(rotation) {}

		void AddCallback(const Hedgehog::Base::THolder<Sonic::CWorld>& worldHolder, Sonic::CGameDocument* pGameDocument,
			const boost::shared_ptr<Hedgehog::Database::CDatabase>& spDatabase) override
		{
			CGameObject3D::AddCallback(worldHolder, pGameDocument, spDatabase);

			hh::mr::CMirageDatabaseWrapper wrapper(spDatabase.get());
			m_spRenderable = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData(m_Name));
			m_spRenderable->BindMatrixNode(m_spMatrixNodeTransform);

			m_spMatrixNodeTransform->m_Transform.SetRotationAndPosition(m_Rotation, m_Position);
			m_spMatrixNodeTransform->NotifyChanged();

			AddRenderable("Object", m_spRenderable);
		}
	};

	class CObjNumberedJumpPanel : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("NumberedJumpPannel")
			static constexpr const char* ms_ModelName = "numberedpannel";

		int Count = 1;

		std::vector<Hedgehog::Math::CVector>     m_Positions;
		std::vector<Hedgehog::Math::CQuaternion> m_Rotations;

		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMatrixNodeCollision;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMatrixNodeEvent;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spFXNode;
		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBody;
		Sonic::CGlitterPlayer* m_pGlitterPlayer = nullptr;

		std::vector<boost::shared_ptr<CObjPanelVisual>> m_Panels;

		// Sonic OutOfControl stuff
		///////////////////////////

		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spSonicControlNode;
		CVector m_TargetPosition;
		CVector m_Direction;
		float m_MoveSpeed = 80.0f;
		float m_WaitTime = 0.0f;
		int m_CurrentPanel = 1;
		bool m_PrepareFinish = false;

		static constexpr float MaxWaitTime = 0.49f; // NOTE: I remember it either being 32 frames from when sonic contacts the platform, or 28/29 frames after he sits still

		Sonic::Player::CPlayerSpeedContext* m_pPlayerContext = nullptr;
		bool m_HasBegunLinkJump = false;
		bool m_IsOverlap = false;
		float m_CurrentTime = 0.0f;

		enum State
		{
			SIdle = 0,
			SMovePanel = 1,
			SWait = 2
		} m_State = SIdle;

		// Debug properties
		float dbg_NodeHeightMin = 0.5f;
		float dbg_NodeHeightMax = 1.5f;



		// States

		void StateUpdate_Idle();

		void StateUpdate_Move(float deltaTime);

		void StateUpdate_Wait(float deltaTime);

		bool DoDetatch(float deltaTime);

		void ProcMsgHitEventCollision(Sonic::Message::MsgHitEventCollision& msg);

		void ProcMsgLeaveEventCollision(Sonic::Message::MsgLeaveEventCollision& msg);



		// Overrides

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override;

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;

		bool SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;

		void KillCallback() override;

		void InitializeEditParam(Sonic::CEditParam& in_rEditParam) override;

		bool ProcessMessage(Hedgehog::Universe::Message& message, bool flag) override;



		// OUTDATED:

		void UpdateOld(const Hedgehog::Universe::SUpdateInfo& updateInfo);

		void StartLinkJumpAction();

		bool DebugCancelExternalControl();
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjNumberedJumpPanel)
}

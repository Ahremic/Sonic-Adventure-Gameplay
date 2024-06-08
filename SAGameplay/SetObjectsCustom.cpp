#include "SetObjectsCustom.h"
#include "StateMachineLite.h"

namespace Sonic
{
	// Originally I inherited the rising lava platforms from crisis city for my floating rock object.
	// Eventually I reimplemented its springy behavior on my own. Here for reference.
	class CObjCscLavaRide : public CObjectBase, public CSetObjectListener
	{
		// Function pointers
		//------------------
		static inline BB_FUNCTION_PTR(void, __stdcall, fpCtor, 0x00F1FF20, CObjCscLavaRide* This);

	public:

		CObjCscLavaRide(const bb_null_ctor&) : CObjectBase(bb_null_ctor{}), CSetObjectListener(bb_null_ctor{}) {}
		CObjCscLavaRide() : CObjCscLavaRide(bb_null_ctor{})
		{
			fpCtor(this);
		}


		// Method overrides
		//-----------------

		BB_OVERRIDE_FUNCTION_PTR(void, CGameObject, DeathCallback, 0xF1DE30, (Sonic::CGameDocument*, in_pGameDocument));
		BB_OVERRIDE_FUNCTION_PTR(void, CGameObject, CGameObject30, 0xF1E880, (void*, A1));

		BB_OVERRIDE_FUNCTION_PTR(void, CObjectBase, SetAddUpdateUnit, 0xF1CD80, (Sonic::CGameDocument*, in_pGameDocument));
		BB_OVERRIDE_FUNCTION_PTR(bool, CObjectBase, SetAddRenderables, 0xF1E4D0,
			(Sonic::CGameDocument*, in_pGameDocument), (const boost::shared_ptr<Hedgehog::Database::CDatabase>&, in_spDatabase));
		BB_OVERRIDE_FUNCTION_PTR(bool, CObjectBase, SetAddColliders, 0xF1F890, (const boost::shared_ptr<Hedgehog::Database::CDatabase>&, in_spDatabase));
		BB_OVERRIDE_FUNCTION_PTR(bool, CObjectBase, SetAddStateMachine, 0xF1DEE0,
			(const Hedgehog::Base::THolder<Sonic::CWorld>&, in_worldHolder), (Sonic::CGameDocument*, in_pGameDocument),
			(const boost::shared_ptr<Hedgehog::Database::CDatabase>&, in_spDatabase));
		BB_OVERRIDE_FUNCTION_PTR(void, CObjectBase, SetUpdateParallel, 0xF1D4A0, (const Hedgehog::Universe::SUpdateInfo&, updateInfo))

		BB_OVERRIDE_FUNCTION_PTR(bool, CMessageActor, ProcessMessage, 0xF20370, (Hedgehog::Universe::Message&, message), (bool, flag))
		BB_OVERRIDE_FUNCTION_PTR(Hedgehog::Universe::IStateMachineMessageReceiver*, CMessageActor, GetStateMachineMessageReceiver, 0x004C9070, (bool, flag))

		BB_OVERRIDE_FUNCTION_PTR(void, CSetObjectListener, InitializeEditParam, 0xF1E8C0, (CEditParam&, in_rEditParam))
		BB_OVERRIDE_FUNCTION_PTR(void, CSetObjectListener, CSetObjectListener1C, 0xF1E730, (void*, A1))

		// Members
		//-------------

		int ModelType;
		float LavaLength;
		bool IsActive;
		float ActiveHeight;
		float ActiveInterval;
		bool IsMessageOn;
		float MoveLength;
		float UpTime;
		float UpWaitTime;
		float DownTime;
		float DownWaitTime;
		float PillarRadiusScale;
		float CollisionLength;
		Hedgehog::Universe::CTinyStateMachineBase m_StateMachine;
		boost::shared_ptr<Hedgehog::Mirage::CSingleElement> m_spRenderable;
		char m_Field170;
		int m_Field174;
		boost::shared_ptr<void> m_spSingleElementEffectMotionAll;
		boost::shared_ptr<CRigidBody> m_spRigidBodyPlatform;
		boost::shared_ptr<CRigidBody> m_spRigidBodyLava;
		char m_Field190;
		char HasStartedDownward;
		float InitialDownVelocity;
		int m_Field198;
		boost::shared_ptr<CMatrixNodeTransform> m_spMatrixNodeTransform_Model;
		boost::shared_ptr<CMatrixNodeTransform> m_spMatrixNodeTransform_Lava;
		int m_Field1AC;
		int m_ActionType;
		float m_SomethingWithStorage;
		float m_Storage;
		float m_SpringResistVelocity;
		bool m_Field1C0;
		bool m_IsContact;
		uint32_t m_ContactActorID;
		float m_InitialThrust;
		int m_Field1CC;
		int m_Field1D0;
		int m_Field1D4;
		int m_Field1D8;
		int m_Field1DC;
		int m_Field1E0;
		float m_UpDownIntervalOffset;
		float m_SpringForce;
		float m_YOffsetSin;
		int m_Field1F0;
		boost::shared_ptr<void> m_spShapeCastCollisionRising; // Sonic::CShapeCastCollision
		char m_Field1FC;
	};
	BB_ASSERT_SIZEOF(CObjCscLavaRide, 0x200);
	BB_ASSERT_OFFSETOF(CObjCscLavaRide, m_StateMachine, 0x138);
}

// TODO: Move most of this to their own CPP files and headers.
namespace SetObjectsCustom
{
	float MouseDelta = 0;

	using namespace StateMachineImpl;
	class CObjDolphin : public Sonic::CObjectBase, public Sonic::CSetObjectListener, public StateMachineLite<CObjDolphin>
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("ECDolphin")

		boost::shared_ptr<hh::anim::CAnimationPose> m_spAnimationPose;
		boost::shared_ptr<hh::mr::CSingleElement>   m_spRenderable;
		boost::shared_ptr<Sonic::CRayCastCollision> m_spRayCastCollision;
		std::unique_ptr<StateBase> m_CurrentState{};

		float m_waterHeight = 0.0f;
		float m_StateTime = 0.0f;
		float m_ObjectTime = 0.0f;

		class StateDive : public StateBase
		{
		public:
			STATE_NAME("Dive")

			void Update(float deltaTime) override
			{
				DebugDrawText::log("I am NOT dolf", 0, 0, { 1, 0.2, 0.5, 1 });
			}
		};
		class StateIdle : public StateBase
		{
		public:
			STATE_NAME("Idle")

				float m_CurrentTime = 0.0f;

			void Enter() override
			{
				m_CurrentTime = 0.0f;
			}

			void Update(float deltaTime) override
			{
				m_CurrentTime += deltaTime;

				if (m_CurrentTime > 2)
					ChangeState<StateDive>();

				DebugDrawText::log("I am dolf", 0, 0, { 0.2, 0.5, 1.0, 1 });
			}
		};
		class StateJump : public StateBase
		{
			STATE_NAME("Jump")
		};

		bool SetAddStateMachine(const Hedgehog::Base::THolder<Sonic::CWorld>& in_worldHolder, Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			// Initialize in idle state
			ChangeState<StateIdle>();
			return true;
		}

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			// TODO: Add model and animation pose.
			//const char* assetName = "EC_Dolphin_SD";
			const char* assetName = "cmn_obj_ring_HD";
			const char* skelName = "EC_Dolphin_SD_event";

			hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());
			m_spRenderable = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData(assetName));

			// TODO: Change matrix node to independent one.
			m_spRenderable->BindMatrixNode(m_spMatrixNodeTransform);

			CGameObject::AddRenderable("Object", m_spRenderable, true);

			m_spRayCastCollision = boost::make_shared<Sonic::CRayCastCollision>(m_pMember->m_pWorld.get());

			DebugDrawText::log(format("Phantom: %X", m_spRayCastCollision->m_pHkpAabbPhantom), 20.0f);
			DebugDrawText::log(format("World:   %X", m_spRayCastCollision->m_pPhysicsWorld->m_pHkpWorld), 20.0f);

			return true;
		}

		hh::math::CVector ScreenToWorld(float depth = 1.0f)
		{
			using namespace hh::math;

			const float WIDTH = (float)*(uint32_t*)0x1DFDDDC;
			const float HEIGHT = (float)*(uint32_t*)0x1DFDDE0;

			POINT mousePos{};
			GetCursorPos(&mousePos);
			ScreenToClient(GetForegroundWindow(), &mousePos);

			Sonic::CCamera* camera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera().get();

			CVector2 screenCoord((float)mousePos.x / WIDTH, 1.0f - (float)mousePos.y / HEIGHT);

			CVector  ndc(screenCoord.x(), screenCoord.y(), 0.5f);
			ndc -= CVector(0.5f, 0.5f, 0.5f);
			ndc *= 2.0f;

			CVector4 ViewCoordinates = camera->m_MyCamera.m_Projection.inverse() * CVector4(ndc.x(), ndc.y(), ndc.z(), 1.0f);
			CVector4 PerspCoordinate = camera->m_MyCamera.m_View.inverse() * ViewCoordinates;

			CVector worldPosition = CVector(PerspCoordinate.x(), PerspCoordinate.y(), PerspCoordinate.z()) / PerspCoordinate.w();
			CVector direction = (worldPosition - camera->m_Position).normalized();

			return direction * depth + camera->m_Position;
		}

		hh::math::CVector QueryWorldHit(float depth = 1.0f)
		{
			using namespace hh::math;

			const float WIDTH = (float)*(uint32_t*)0x1DFDDDC;
			const float HEIGHT = (float)*(uint32_t*)0x1DFDDE0;

			POINT mousePos{};
			GetCursorPos(&mousePos);
			ScreenToClient(GetForegroundWindow(), &mousePos);

			Sonic::CCamera* camera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera().get();

			CVector2 screenCoord((float)mousePos.x / WIDTH, 1.0f - (float)mousePos.y / HEIGHT);

			CVector  ndc(screenCoord.x(), screenCoord.y(), 0.5f);
			ndc -= CVector(0.5f, 0.5f, 0.5f);
			ndc *= 2.0f;

			const CVector4 ViewCoordinates = camera->m_MyCamera.m_Projection.inverse() * CVector4(ndc.x(), ndc.y(), ndc.z(), 1.0f);
			const CVector4 PerspCoordinate = camera->m_MyCamera.m_View.inverse() * ViewCoordinates;

			const CVector worldPosition = CVector(PerspCoordinate.x(), PerspCoordinate.y(), PerspCoordinate.z()) / PerspCoordinate.w();
			const CVector direction = (worldPosition - camera->m_Position).normalized();
			const CVector rayEnd = direction * depth + camera->m_Position;

			Sonic::SCollisionHitPointInfo hitInfo;

			if (!m_spRayCastCollision->CheckLineCollisionClosest(*(uint32_t*)0x01E0AFB4, camera->m_Position, rayEnd, &hitInfo))
				return rayEnd;

			return CVector(hitInfo.Position);
		}

		static inline float scrollDepth = 50.0f;

		void UpdatePositionWithMouse(float deltaTime)
		{
			using namespace hh::math;
			constexpr float scrollSpeed = 3.0f;

			// The lower order bit flashes when held. The higher order bit stays on when held, off when released.
			const bool  isShiftKey = GetAsyncKeyState(VK_SHIFT) & 0b1000000000000000;
			const float shiftScale = isShiftKey ? 5.0f : 1.0f;

			scrollDepth += MouseDelta * scrollSpeed * shiftScale * deltaTime;
			MouseDelta = 0.0f;

			//m_spMatrixNodeTransform->m_Transform.SetPosition(ScreenToWorld(scrollDepth));
			m_spMatrixNodeTransform->m_Transform.SetPosition(QueryWorldHit(scrollDepth));
			m_spMatrixNodeTransform->NotifyChanged();
		}

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override
		{
			//m_ObjectTime += updateInfo.DeltaTime;
			//UpdateStateMachine(updateInfo.DeltaTime);

			UpdatePositionWithMouse(updateInfo.DeltaTime);
		}
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjDolphin)


	////////////////////
	//
	// Guillotine
	// ------------------


	class CObjECGuillotine : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("ECGuillotine")

		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable_SpikeBar;
		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable_SideRails;

		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spNodeTransformSpikeBar;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodySideRails;

		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodySpikeBar;

		static constexpr float TopHeight = 4.00f;
		static constexpr float BottomHeight = 0.25f;

		enum State
		{
			SWaitUp = 0,
			SMoveDown = 1,
			SWaitDown = 2,
			SMoveUp = 3
		} m_State;
		float m_StateTimer = 0;

		float TimeWaitUp = 2.95f;
		float TimeMoveDown = 0.7833f;
		float TimeWaitDown = 0.5833f;
		float TimeMoveUp = 1.3833f;

		float m_divTimeWaitUp = 1.0f / TimeWaitUp;
		float m_divTimeMoveDown = 1.0f / TimeMoveDown;
		float m_divTimeWaitDown = 1.0f / TimeWaitDown;
		float m_divTimeMoveUp = 1.0f / TimeMoveUp;

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			m_spNodeTransformSpikeBar = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_spNodeTransformSpikeBar->SetParent(m_spMatrixNodeTransform.get());
			m_spNodeTransformSpikeBar->m_Transform.SetPosition(CVector(0, TopHeight, 0));
			m_spNodeTransformSpikeBar->NotifyChanged();


			hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());

			m_spRenderable_SideRails = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData("seaobj_guillotine_base"));
			m_spRenderable_SideRails->BindMatrixNode(m_spMatrixNodeTransform);

			m_spRenderable_SpikeBar = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData("seaobj_guillotine_spikebar"));
			m_spRenderable_SpikeBar->BindMatrixNode(m_spNodeTransformSpikeBar);

			CGameObject::AddRenderable("Object", m_spRenderable_SideRails, true);
			CGameObject::AddRenderable("Object", m_spRenderable_SpikeBar, true);

			return true;
		}

		bool SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			AddRigidBody(m_spRigidBodySideRails, "seaobj_guillotine", "seaobj_guillotine_base", *pColID_Common, m_spMatrixNodeTransform, in_spDatabase);

			hk2010_2_0::hkpBoxShape* boxShapeCollider = new hk2010_2_0::hkpBoxShape(5.7175f, 0.4f, 0.4f);
			AddRigidBody(m_spRigidBodySpikeBar, boxShapeCollider, *pColID_Common, m_spNodeTransformSpikeBar);
			boxShapeCollider->removeReference();

			//Havok::BoxShape* boxShapeDamage = new Havok::BoxShape(4.3575f, 2.325f, 0.345f);
			hk2010_2_0::hkpBoxShape* boxShapeDamage = new hk2010_2_0::hkpBoxShape(4.3575f, 2.325f, 0.75f);
			AddEventCollision((char*)0x0154DE02, boxShapeDamage, *(int*)0x01E0AFC4, true, m_spNodeTransformSpikeBar);
			boxShapeDamage->removeReference();

			return true;
		}

		void StateUp(float deltaTime)
		{
			if (m_StateTimer <= TimeWaitUp)
				return;

			m_StateTimer = 0;
			m_State = SMoveDown;
			StateMoveDown(deltaTime);
		}
		void StateMoveDown(float deltaTime)
		{
			const float t = m_StateTimer * m_divTimeMoveDown;
			//const float tPwr = (std::reversePower(t, 3) + t) * 0.5f;
			//const float height = std::lerp(TopHeight, BottomHeight, tPwr);
			const float height = std::lerp(TopHeight, BottomHeight, std::reversePower(t, 2));
			m_spNodeTransformSpikeBar->m_Transform.SetPosition(CVector(0, height, 0));
			m_spNodeTransformSpikeBar->NotifyChanged();

			if (m_StateTimer <= TimeMoveDown)
				return;
			auto* transform = &m_spMatrixNodeTransform->m_Transform;
			CVector spawnCenter = transform->m_Position - CVector(0, 0.8f, 0);

			m_pGlitterPlayer->PlayOneshot(spawnCenter + (transform->m_Rotation.Left() * 1.25f), "ef_guillotine_press_smoke", 0.25f, 9);
			m_pGlitterPlayer->PlayOneshot(spawnCenter - (transform->m_Rotation.Left() * 1.25f), "ef_guillotine_press_smoke", 0.25f, 9);

			//m_pGlitterPlayer->PlayContinuous(m_pMember->m_pGameDocument, m_spNodeTransformSpikeBar, "ef_ch_sng_yh1_boost2", 1.25f, 1, 0);
			//m_pGlitterPlayer->PlayContinuousByNode(syncptr, m_spNodeTransformSpikeBar, "ef_ch_sng_yh1_boost2", 1.25f, 1, 0);

			m_StateTimer = 0;
			m_State = SWaitDown;
		}
		void StateDown(float deltaTime)
		{
			if (m_StateTimer <= TimeWaitDown)
				return;

			m_StateTimer = 0;
			m_State = SMoveUp;
			StateMoveUp(deltaTime);
		}
		void StateMoveUp(float deltaTime)
		{
			const float t = m_StateTimer * m_divTimeMoveUp;
			const float height = std::lerp(BottomHeight, TopHeight, std::QuickPower(t, 2));
			m_spNodeTransformSpikeBar->m_Transform.SetPosition(CVector(0, height, 0));
			m_spNodeTransformSpikeBar->NotifyChanged();

			if (m_StateTimer <= TimeMoveUp)
				return;

			m_StateTimer = 0;
			m_State = SWaitUp;
		}

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override
		{
			const float deltaTime = updateInfo.DeltaTime;
			switch (m_State)
			{
			default:
			case SWaitUp:
				StateUp(deltaTime);
				break;
			case SMoveDown:
				StateMoveDown(deltaTime);
				break;
			case SWaitDown:
				StateDown(deltaTime);
				break;
			case SMoveUp:
				StateMoveUp(deltaTime);
				break;
			}

			m_StateTimer += deltaTime;
		}

		void OnHit(uint32_t hitID)
		{
			SendMessage(hitID, boost::make_shared<Sonic::Message::MsgDamage>(*(int*)0x01E0BE28, m_spNodeTransformSpikeBar->m_Transform.m_Position));
		}

		bool ProcessMessage(Hedgehog::Universe::Message& message, bool flag) override
		{
			if (!flag)
				return CObjectBase::ProcessMessage(message, flag);

			if (message.Is<Sonic::Message::MsgHitEventCollision>())
			{
				OnHit(message.m_SenderActorID);
				return true;
			}

			return CObjectBase::ProcessMessage(message, flag);
		}

	};
	BB_SET_OBJECT_MAKE_HOOK(CObjECGuillotine)

	///////////////////////////
	// Floating rock platform
	// -----------------------

	class CObjECFloatingPlatform : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("ECFloatingPlatform")

		/* Params from Lavaride */
		bool m_IsContact = false;
		int m_ActionID = 0;
		int m_ContactActorID = 0;
		float m_SpringResistVelocity = 0.0f;
		float m_SpringForce = 0.0f;
		float m_InitialThrust = 0.0f;
		float m_Storage = 0.0f;
		float m_SomethingWithStorage = 0.0f;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMatrixNodeTransform_Model;
		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyPlatform;

		bool m_IsDebug = false;

		static constexpr float MaxAngle = 10.0f;

		CVector m_InitialPosition = CVector::Zero();
		CQuaternion m_InitialRotation = CQuaternion::Identity();

		CVector m_TargetDirectionVector = CVector::Zero();
		CQuaternion m_TargetRotation = CQuaternion::Identity();

		//boost::shared_ptr<Hedgehog::Mirage::CSingleElement> m_spDebugRenderable;

		void InitObject()
		{
			// Force null instead of uninitialized
			//m_spSingleElementEffectMotionAll = nullptr;
			Hedgehog::Mirage::CTransform* transform = &m_spMatrixNodeTransform->m_Transform;
			m_InitialPosition = transform->m_Position;
			m_InitialRotation = transform->m_Rotation;
			m_TargetRotation = m_InitialRotation;


			m_spMatrixNodeTransform_Model = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_spRigidBodyPlatform = boost::make_shared<Sonic::CRigidBody>();
		}

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			InitObject();

			m_spMatrixNodeTransform->SetChild(m_spMatrixNodeTransform_Model.get());
			SElementInfo info(m_spRenderable);
			AddRenderable("seaobj_flootiwa", info, m_spMatrixNodeTransform_Model, in_spDatabase);

			return true;
		}

		bool SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			AddRigidBody(m_spRigidBodyPlatform, "seaobj_flootiwa", "seaobj_flootiwa", *(int*)0x01E0AF30, m_spMatrixNodeTransform_Model, in_spDatabase);

			Sonic::CHavokDatabaseWrapper havokWrapper(in_spDatabase.get());

			hk2010_2_0::hkpShape* triggerShape =
				havokWrapper.GetRigidBodyContainer("seaobj_flootiwa")->GetRigidBody("seaobj_flootiwa_event", nullptr)->GetShape();

			AddEventCollision("Stomping", triggerShape, *(int*)0x01E0AFD8, true, m_spMatrixNodeTransform_Model);
			triggerShape->removeReference();

			return true;
		}

		// Code taken & modified from Gens for the LavaRide object.
		void ComputeFloatState(float deltaTime)
		{
			struct SLavaRideParams
			{
				float DownSpeed = 10.0f;
				float StoreTimer = 1.0f;
				float SpringResist = -10.0f;
				float AirResist = -0.69999999f;
			} constexpr LavaRideParams { 0.5f, 1.0f, -10.0f, -1.5f };

			if (m_IsContact)
			{
				m_IsContact = false;

				if (m_ActionID == 3 || m_ActionID == 0)
				{
					m_SpringResistVelocity = m_SpringResistVelocity + m_InitialThrust * 0.25f;
					m_ActionID = 3;
				}
			}

			// We want a constant downward pressure on the platform when standing on it,
			// so it stays underwater when just walking around.
			if (m_ContactActorID)
			{
				m_SpringResistVelocity -= 5.0f * deltaTime;
			}

			if (m_ActionID == 0)
			{
				m_Storage = LavaRideParams.StoreTimer;
				m_SpringForce = m_SomethingWithStorage;

				m_ActionID = 2;
				return;
			}

			//if (m_ActionType == 3 && (m_ActionType - 2)) /* ...What? */
			if (m_ActionID == 3)
			{
				m_SpringResistVelocity = LavaRideParams.SpringResist * m_SpringForce * deltaTime + m_SpringResistVelocity;
				m_SpringResistVelocity = (m_SpringResistVelocity * LavaRideParams.AirResist) * deltaTime + m_SpringResistVelocity;
				const float targetVelocity = m_SpringResistVelocity * deltaTime + m_SpringForce;

				if (targetVelocity * m_SpringForce < 0.0)
				{
					if (fabs(m_SpringResistVelocity) < 0.005)
					{
						m_SpringResistVelocity = 0.0;
						m_ActionID = 0;
						m_SpringForce = 0.0f;
						return;
					}
				}
				m_SpringForce = targetVelocity;
			}
			else
			{
				m_Storage -= deltaTime;
				if (m_Storage < 0.0)
				{
					m_SpringResistVelocity = 0.0;
					m_ActionID = 3;
				}
			}
		}

		void ComputeTargetRotation(float deltaTime)
		{
			const float lerpAmount = m_ContactActorID
			                       ? 0.1f
			                       : 0.08f;

			if (!m_ContactActorID)
			{
				m_TargetRotation = CQuaternion::Slerp(m_TargetRotation, m_InitialRotation, lerpAmount);
				return;
			}

			CVector sonicPosition(0, 0, 0);
			SendMessageImm(m_pMessageManager->GetMessageActor(m_ContactActorID), Sonic::Message::MsgGetPosition(&sonicPosition));

			const CVector platformPosition = m_spMatrixNodeTransform_Model->m_Transform.m_Position + m_spMatrixNodeTransform->m_Transform.m_Position;

			const CVector directionVector = CVector::ProjectOnPlane(sonicPosition - platformPosition, CVector::Up());

			if (m_IsContact)
			{
				m_TargetDirectionVector = directionVector;
			}
			else
			{
				m_TargetDirectionVector = CVector::Lerp(directionVector, m_TargetDirectionVector, 0.35f);
			}

			const float   distance = m_TargetDirectionVector.norm();
			const CVector direction = m_TargetDirectionVector.normalizedSafe();

			const CVector axis = -CVector::Cross(direction, CVector::Up());
			const float angle = MaxAngle * distance * DEG2RAD;

			// This is a little janky, but hopefully it works...
			const CQuaternion result = CQuaternion::FromAngleAxis(angle, axis).Normalized().normalized();

			m_TargetRotation = CQuaternion::Slerp(m_TargetRotation, result, lerpAmount);
		}

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override
		{
			BB_FUNCTION_PTR(void, __thiscall, TStateMachineUpdate, 0x00772D70, Hedgehog::Universe::CTinyStateMachineBase * stateMachine, const Hedgehog::Universe::SUpdateInfo & updateInfo);
			BB_FUNCTION_PTR(void, __thiscall, MatrixNodeVerify, 0x006F3F80, Hedgehog::Mirage::CMatrixNode*);

			//TStateMachineUpdate(&m_StateMachine, updateInfo);
			ComputeTargetRotation(updateInfo.DeltaTime);
			ComputeFloatState(updateInfo.DeltaTime);

			m_spMatrixNodeTransform_Model->m_Transform.SetPosition(CVector(0, m_SpringForce, 0));
			m_spMatrixNodeTransform_Model->m_Transform.SetRotation(m_TargetRotation);
			m_spMatrixNodeTransform_Model->NotifyChanged();
			//MatrixNodeVerify(m_spMatrixNodeTransform_Model.get());

			if (!m_IsDebug)
				return;

			//m_spMatrixNodeTransform_Lava->m_Transform.SetPosition(CVector(1, m_SpringResistVelocity, 0));
			//m_spMatrixNodeTransform_Lava->NotifyChanged();
		}

		void InitializeEditParam(Sonic::CEditParam& in_rEditParam) override
		{
			//CObjCscLavaRide::InitializeEditParam(in_rEditParam);

			in_rEditParam.CreateParamBool(&m_IsDebug, "IsDebug");
		}

		void ProcMsgHit(const Sonic::Message::MsgHitEventCollision& message)
		{
			CVector previousVelocity = CVector::Zero();
			if (!SendMessageImm(message.m_SenderActorID, Sonic::Message::MsgGetPreviousVelocity(&previousVelocity)))
				return;

			m_IsContact = true;
			m_ContactActorID = message.m_SenderActorID;
			m_InitialThrust = previousVelocity.y();
		}

		bool ProcessMessage(Hedgehog::Universe::Message& message, bool flag) override
		{
			using namespace Sonic::Message;
			if (!flag)
				return CObjectBase::ProcessMessage(message, flag);

			if (message.Is<MsgHitEventCollision>())
			{
				ProcMsgHit(*static_cast<MsgHitEventCollision*>(&message));
				return true;
			}

			if (message.Is<MsgLeaveEventCollision>())
			{
				m_ContactActorID = 0;
				return true;
			}

			return CObjectBase::ProcessMessage(message, flag);
		}

	};

	// Simple variant fallback
	class CObjECFloatingPlatform_Simple : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("ECFloatingPlatform")

			boost::shared_ptr<hh::mr::CSingleElement>   m_spRenderable;
		boost::shared_ptr<hh::anim::CAnimationPose> m_spAnimationPose;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBody;

		float m_Time = 0.0f;
		float m_deltaTime = 1.0f / 60.0f;
		float m_OriginalHeight = 0.0f;
		float m_TargetHeightOffset = 0.0f;
		bool m_IsContact = false;

		bool m_HasContacted = false;
		float m_InitialVelocity = 0.0f;

		float m_SpringOffset = 0.0f;
		float m_SpringVelocity = 0.0f;

		static constexpr float MaxAngle = 10.0f;

		CVector m_InitialPosition = CVector::Zero();
		CQuaternion m_InitialRotation = CQuaternion::Identity();
		CQuaternion m_TargetRotation = CQuaternion::Identity();

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			const Hedgehog::Mirage::CTransform* transform = &m_spMatrixNodeTransform->m_Transform;

			m_InitialPosition = transform->m_Position;
			m_InitialRotation = transform->m_Rotation;
			m_TargetRotation = m_InitialRotation;

			m_OriginalHeight = transform->m_Position.y();

			hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());
			m_spRenderable = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData("seaobj_flootiwa"));
			m_spRenderable->BindMatrixNode(m_spMatrixNodeTransform);

			CGameObject::AddRenderable("Object", m_spRenderable, true);

			return true;
		}

		bool SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			AddRigidBody(m_spRigidBody, "seaobj_flootiwa", "seaobj_flootiwa", *pColID_Common, m_spMatrixNodeTransform, in_spDatabase);

			Sonic::CHavokDatabaseWrapper havokWrapper(in_spDatabase.get());

			hk2010_2_0::hkpShape* triggerShape =
				havokWrapper.GetRigidBodyContainer("seaobj_flootiwa")->GetRigidBody("seaobj_flootiwa_event", nullptr)->GetShape();

			AddEventCollision("Stomping", triggerShape, *(int*)0x01E0AFD8, true, m_spMatrixNodeTransform);
			triggerShape->removeReference();

			return true;
		}

		CQuaternion GetTargetRotation(Sonic::Player::CPlayerSpeedContext* context) const
		{
			if (!m_IsContact)
				return m_InitialRotation;

			CVector directionVector = CVector::ProjectOnPlane(context->m_spMatrixNode->m_Transform.m_Position - m_spMatrixNodeTransform->m_Transform.m_Position, CVector::Up());

			const float   distance = directionVector.norm();
			const CVector direction = directionVector.normalizedSafe();

			const CVector axis = -CVector::Cross(direction, CVector::Up());
			const float angle = MaxAngle * distance * DEG2RAD;

			// This is a little janky, but hopefully it works...
			return CQuaternion::FromAngleAxis(angle, axis).Normalized().normalized();
		}

		void ComputeSpringPhysics(float deltaTime)
		{
			const float frameDeltaTime = deltaTime * 60.0f;
			const float dt = frameDeltaTime * 0.025f;

			if (m_HasContacted)
			{
				m_SpringOffset += m_InitialVelocity;
				m_HasContacted = false;
			}

			constexpr float stiffness = 7.165 * 0.25f;
			constexpr float friction = 1.85 * 0.25f;

			m_SpringVelocity -= dt * (stiffness * m_SpringOffset + friction * m_SpringVelocity);
			m_SpringOffset += dt * m_SpringVelocity;
		}

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override
		{
			m_Time += updateInfo.DeltaTime;
			m_deltaTime = updateInfo.DeltaTime;

			auto* const context = Sonic::Player::CPlayerSpeedContext::GetInstance();
			if (!context)
				return;

			m_TargetRotation = CQuaternion::Slerp(m_TargetRotation, GetTargetRotation(context), 0.1f);

			ComputeSpringPhysics(updateInfo.DeltaTime);

			m_spMatrixNodeTransform->m_Transform.SetPosition(m_InitialPosition + CVector(0, m_SpringOffset, 0));
			m_spMatrixNodeTransform->m_Transform.SetRotation(m_TargetRotation);
			m_spMatrixNodeTransform->NotifyChanged();
		}

		void ProcContact()
		{
			m_IsContact = true;
			m_HasContacted = true;

			auto* context = Sonic::Player::CPlayerSpeedContext::GetInstance();
			if (!context)
				return;

			m_InitialVelocity = CVector::Dot(CVector::Up(), context->GetVelocity()) * m_deltaTime;
		}

		bool ProcessMessage(Hedgehog::Universe::Message& message, bool flag) override
		{
			using namespace Sonic::Message;
			if (!flag)
				return CObjectBase::ProcessMessage(message, flag);

			if (message.Is<MsgHitEventCollision>())
			{
				ProcContact();
				return true;
			}

			if (message.Is<MsgLeaveEventCollision>())
			{
				m_IsContact = false;
				return true;
			}

			return CObjectBase::ProcessMessage(message, flag);
		}

		void SetAddUpdateUnit(Sonic::CGameDocument* in_pGameDocument) override
		{
			in_pGameDocument->AddUpdateUnit("a", this);
		}
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjECFloatingPlatform)

	// Hook that should hopefully prevent crashes if inhereted objects using CSingleEffectMotionAll leave those null.
	HOOK(int, __fastcall, _EffectMotionUpdate, 0x00752F00, void* This, void*, float deltaTime)
	{
		if (!This)
			return 0;

		return original_EffectMotionUpdate(This, nullptr, deltaTime);
	}

	// Attempt to get the water working...
	class CObjOceanTile : public Sonic::CGameObject3D
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		boost::shared_ptr<hh::mr::CSingleElement> m_spRenderable;

		const char* m_Name;
		CVector     m_Position;
		CQuaternion m_Rotation;

		CObjOceanTile(const char* name, const CVector& position, const CQuaternion rotation) : m_Name(name), m_Position(position), m_Rotation(rotation) {}

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

	class CObjECOceanManager : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;
	public:
		BB_SET_OBJECT_MAKE("ECOcean")
			static constexpr const char* ms_ModelName = "oceanpiece";

		std::vector<boost::shared_ptr<void>> m_OceanTiles;


		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override
		{
			return true;
		}

		void KillCallback() override
		{
		}
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjECOceanManager)

	// TODO: MOVE THIS
	// Hook windproc so we get the mouse scroll, since this is somehow the only place in our program we can get this.
	HOOK(LRESULT, __stdcall, WndProc, 0xE7B6C0, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg)
		{
		default:
			break;
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			MouseDelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
			break;
		}

		return originalWndProc(hWnd, Msg, wParam, lParam);
	}
}


#include "Objects/FloatingBridge.h"
#include "Objects/RubberBandRail.h"
#include "Objects/JumpPanel.h"

void SetObjectsCustom::Init()
{
	// Mouse stuff
	INSTALL_HOOK(WndProc)

	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjECFloatBridgeManager)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjECRubberBandRail)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjDolphin)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjECFloatingPlatform)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjECGuillotine)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjNumberedJumpPanel)
	BB_INSTALL_SET_OBJECT_MAKE_HOOK(CObjECOceanManager)

	// Hook patch
	INSTALL_HOOK(_EffectMotionUpdate)
}

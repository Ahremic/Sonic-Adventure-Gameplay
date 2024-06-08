#include "Pch.h"
#include "SetObjectMods.h"

#include <chrono>
#include <Hedgehog/Universe/Engine/hhStateMachineBase.h>

#include "Messages.h"

namespace SetObjectMods
{
	// Fun experiment with dash/rainbow rings
	HOOK(void, __fastcall, _DashRingCollisionEnter, 0x0115A5C0, Sonic::CGameObject3D* This, void*, hh::fnd::Message& message)
	{
		DebugDrawText::log("HITDASHRING", 10.0f, 1, { 1.0, 0.25, 0, 1 });
		original_DashRingCollisionEnter(This, nullptr, message);
	}
	HOOK(void, __fastcall, _DashRingUpdate, 0x0115A4A0, Sonic::CGameObject3D* This, void*, hh::fnd::SUpdateInfo& updateInfo)
	{
		using namespace hh::math;

		constexpr double dashRingActiveTime = 0.8; // in gens it's literally 0.800000011920929
		constexpr auto dashRingActiveMultiplier = static_cast<float>(1.0 / dashRingActiveTime);

		const int offset = 0x100;
		auto* const data = GetPointer(This, offset);
		const bool hasHit = GetValue<bool>(data, 0x34);
		const bool isTailsRing = GetValue<int>(data, 0x14) >= 2;

		if (isTailsRing || !hasHit)
		{
			original_DashRingUpdate(This, nullptr, updateInfo);
			return;
		}

		const float hitTime = GetValue<float>(data, 0x38);

		Sonic::CMatrixNodeTransform* node = This->m_spMatrixNodeTransform.get();

		const CVector position = node->m_Transform.m_Position;
		const CQuaternion rotation = node->m_Transform.m_Rotation;

		const float scaleXY = cosf(hitTime * M_PI * dashRingActiveMultiplier * 2.0f);

		if (false)
		{
			std::stringstream stream;
			stream.precision(4);
			//stream << rotation.ToRotationMatrix();

			CMatrix mat;
			mat = Eigen::Translation3f(position);
			CMatrix44& mat44 = *(CMatrix44*)&mat;
			stream << mat44;

			DebugDrawText::log(stream.str().c_str(), 0, 0, { 1, .6, 0, 1 });
			stream.str("");

			mat44.transposeInPlace();
			stream << mat44;
			DebugDrawText::log(stream.str().c_str(), 0, 0, { .6, 1, 0, 1 });
			stream.str("");

			stream << rotation.ToRotationMatrix();
			DebugDrawText::log(stream.str().c_str(), 0, 0, { 1, 0, 0, 1 });
		}

		//const auto& spSingleElement = *GetPointer<boost::shared_ptr<hh::mr::CSingleElement>>(data, 0x1C);

		CMatrix mat = Eigen::Translation3f(position) * rotation;

		// Hacking our way into local XY scale.
		CVector* pVecs = (CVector*)&mat;
		float* pFloat = (float*)&mat;

		pVecs[0] *= scaleXY;
		pVecs[1] *= scaleXY;

		// Remove 1s from this area in the matrix
		pFloat[3] = 0;
		pFloat[7] = 0;

		node->m_Transform.m_Matrix = mat;
		node->NotifyChanged();
		original_DashRingUpdate(This, nullptr, updateInfo);
	}

	// Give CPZPipe "teleport" functionality.
#pragma region Teleporter
	class CSetObjectListenerTemp
	{
	public:
		CSetObjectListenerTemp() = default;
		virtual ~CSetObjectListenerTemp() = default;
	};
	class DummyObject : public Sonic::CGameObject3D
	{
		int m_Junk;
	};

	class CObjPipeEnterCollision : public DummyObject, CSetObjectListenerTemp, bb_insert_padding<0x6C>
	{
	public:
		bool m_IsTeleporter;
		hh::math::CVector     m_TeleportPosition;
		hh::math::CQuaternion m_TeleportRotation;
		int m_TargetCameraID;
		int m_TargetBGMChangeID;

		void InitializeLocalParams(Sonic::CEditParam* params)
		{
			m_IsTeleporter = false;
			m_TeleportPosition = hh::math::CVector::Zero();
			m_TeleportRotation = hh::Math::CQuaternion::Identity();
			m_TargetCameraID = 0;
			m_TargetBGMChangeID = 0;

			params->CreateParamBase(Sonic::CParamPosition::Create(&m_TeleportPosition), "TeleportPosition");
			params->CreateParamBase(Sonic::CParamRotation::Create(&m_TeleportRotation), "TeleportRotation");
			params->CreateParamInt(&m_TargetCameraID, "TargetCameraID");
			params->CreateParamInt(&m_TargetBGMChangeID, "TargetBGMChangeID");

			params->CreateParamBool(&m_IsTeleporter, "IsTeleporter");
		}

	};
	//ASSERT_SIZEOF(CObjPipeEnterCollision, 0x168);
	//ASSERT_EQUALITY(sizeof(CObjPipeEnterCollision), 0x168);

	HOOK(void, __fastcall, _PipeEnterCollisionSetParams, 0x00FBEF90, CSetObjectListenerTemp* listener, void*, Sonic::CEditParam* params)
	{
		original_PipeEnterCollisionSetParams(listener, nullptr, params);

		CObjPipeEnterCollision* collision = (CObjPipeEnterCollision*)listener;
		collision->InitializeLocalParams(params);

		//DebugDrawText::log(format("Offset A: %X", listener), 10.0f);
		//DebugDrawText::log(format("Offset B: %X", collision), 10.0f);
		//DebugDrawText::log(format("DIFF: %X", (int)listener - (int)collision), 10.0f);
	}
	HOOK(void, __fastcall, _PipeEnterCollisionContact,   0x00FBED50, CObjPipeEnterCollision* This,     void*, Hedgehog::Universe::MessageTypeSet* msg)
	{
		if (!This->m_IsTeleporter)
		{
			original_PipeEnterCollisionContact(This, nullptr, msg);
			return;
		}

		Hedgehog::Universe::CMessageActor* const object = This->m_pMessageManager->GetMessageActor(msg->m_SenderActorID);

		int objectID = *(int*)object;
		bool isPlayer = objectID == 0x016D1148 // Base (CPlayerSpeed)
		             || objectID == 0x016D4BC8 // Modern
		             || objectID == 0x016D8678 // Classic
		;

		if (!isPlayer)
			return;

		Sonic::CGameDocument* gameDocument = Sonic::CGameDocument::GetInstance().get().get();

		DebugDrawText::log("WARPED", 10.0f);

		auto* const player  = (Sonic::Player::CPlayerSpeed*)object;
		auto* const context = player->GetContext();

		using namespace hh::math;
		hh::mr::CTransform* transform = &context->m_spMatrixNode->m_Transform;

		const CQuaternion rotation  = context->m_HorizontalRotation;
		const CVector localVelocity = rotation.inverse() * context->GetVelocity();

		const CQuaternion targetRotation = CQuaternion::LookRotation(
		                                                This->m_TeleportRotation.Forward().ProjectOnPlane(CVector::Up()).normalizedSafe(),
		                                                CVector::Up()
		                                                );

		const CVector targetVelocity = targetRotation * localVelocity;;
		context->m_HorizontalRotation = targetRotation;
		context->SetVelocity(targetVelocity);

		transform->SetPosition(This->m_TeleportPosition);

		// Have to do some crazy shit now
		//...............................

		FUNCTION_PTR(bool, __thiscall, SetObjectSendMessage, 0x00EB3A60,
			Hedgehog::Universe::CMessageActor * _messageActor,
			Sonic::CSetObjectManager * _setObjectManager,
			uint32_t setObjectID,
			boost::shared_ptr<Hedgehog::Universe::Message>);

		// Simulate hit event on camera volume so the transition is instantaneous.
		SetObjectSendMessage(player,
			gameDocument->m_pGameActParameter->m_pSetObjectManager,
			This->m_TargetCameraID,
			boost::make_shared<Sonic::Message::MsgHitEventCollision>());

		// Also simulate hit event on BGM changer so the music changes *when* we warp, not any time before.
		SetObjectSendMessage(player,
			gameDocument->m_pGameActParameter->m_pSetObjectManager,
			This->m_TargetBGMChangeID,
			boost::make_shared<Sonic::Message::MsgLeaveEventCollision>());

		// HACK: Force the camera to update on this frame so visibility culling is smooth as butter.
		Sonic::CCamera* camera = gameDocument->GetWorld()->GetCamera().get();
		camera->UpdateParallel(camera->m_UpdateInfo);
	}
#pragma endregion

	// JumpCollision patch that *ONLY* cuts your velocity if past a certain threshold.
#pragma region JumpCollision
	class CObjJumpCollision : public Sonic::CGameObject3D, bb_insert_padding<0x04>
	{
	public:
		class Listener
		{
			void* dtor = nullptr;
		public:
			float CollisionWidth {};
			float CollisionHeight {};
			int m_Field00C {};
			int m_Field010 {};
			int m_Field014 {};
			int m_Field018 {};
			int m_Field01C {};
			int m_Field020 {};
			int m_Field024 {};
			int m_Field028 {};
			int m_Field02C {};
			int m_Field030 {};
			int m_Field034 {};
			int m_Field038 {};
			int m_Field03C {};
			int m_Field040 {};
			int m_Field044 {};
			int m_Field048 {};
			float OutOfControl {};
			float ImpulseSpeedOnNormal {};
			float ImpulseSpeedOnBoost  {};
			float Pitch {};
			float SpeedMin {};
			float TerrainIgnoreTime {};
			bool IsStartVelocityConstant {};
			bool m_IsGroundOnly;
			bool m_IsOnlyLimitVelocity; // New one!
			bool field_67;
			float m_ClassicSpinThreshold;
		};

		Listener listener;
	};
	//ASSERT_EQUALITY(sizeof(CObjJumpCollision), 0x164);

	HOOK(void, __fastcall, _JumpCollisionSetParams, 0x011C65D0, CObjJumpCollision::Listener* listener, void*, Sonic::CEditParam* params)
	{
		original_JumpCollisionSetParams(listener, nullptr, params);

		// Default to false since this is normally uninitialized in the constructor.
		listener->m_IsOnlyLimitVelocity = false;
		//params->SetBool(&listener->m_IsOnlyLimitVelocity, "OnlyLimitVelocity");
		params->CreateParamBool(&listener->m_IsOnlyLimitVelocity, "OnlyLimitVelocity");
	}

	HOOK(void, __fastcall, _JumpCollisionOnEventTrigger, 0x011C5A00, CObjJumpCollision* This, void*, uint32_t actorID)
	{
		using namespace hh::math;
		using namespace Sonic::Player;

		if (!This->listener.m_IsOnlyLimitVelocity)
		{
			original_JumpCollisionOnEventTrigger(This, nullptr, actorID);
			return;
		}

		// Do this to confirm our actor is at LEAST CPlayerSpeedContext.
		if (!This->SendMessageImm(actorID, Sonic::Message::MsgGetPlayerType()))
			return;

		auto* const player = static_cast<CPlayerSpeed*>(This->m_pMessageManager->GetMessageActor(actorID));
		auto* const context = player->GetContext();

		const CVector velocity = context->GetVelocity();

		if (velocity.norm() < This->listener.SpeedMin)
			return;

		//This->SendMessageImm(player, MsgApplyImpulse())

		// Should be simple enough now.
		FUNCTION_PTR(void, __stdcall, SonicSetOutOfControl, 0x00E5AC00, CPlayerSpeedContext * _context, float _oocTime);
		context->SetVelocity(velocity.normalizedSafe() * This->listener.SpeedMin);
		SonicSetOutOfControl(context, This->listener.OutOfControl);
	}

#pragma endregion

	// Crabmeat patch, make missiles target player.
	struct __declspec(align(16)) SCrabmeatMissleData
	{
		Hedgehog::Math::CVector m_VecA;
		Hedgehog::Math::CVector m_VecB;
		float m_Speed;
		float m_Distance;
		Hedgehog::Math::CVector m_VecC;
		float m_FloatA;
		Hedgehog::Math::CVector m_VecE;
		float m_FloatB;
	};
	class CrabmeatMissile : public Sonic::CGameObject3D
	{
	public:
		void* whatever1 {};
		void* m_pParticlePlayer {};
		void* m_pStrangePtr {};
		int buffer;
		int field_104;
		int field_108;
		int field_10C;
		float m_LifeTime;
		boost::shared_ptr<void> m_spModelSingleElement;
		bool field_11C;
		int field_120;
		int field_124;
		int field_128;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spOtherMatrixNode;
		bool m_IsActive;
		int field_138;
		int field_13C;
		float field_140;
		int field_144;
		int field_148;
		int field_14C;
		SCrabmeatMissleData m_MissileInfo;
		bool field_1C0;
		int field_1C4;
		int field_1C8;
		int end;
	};
	ASSERT_OFFSETOF(CrabmeatMissile, m_spOtherMatrixNode, 0x12C);
	ASSERT_OFFSETOF(CrabmeatMissile, field_1C0, 0x1C0);
	ASSERT_SIZEOF(CrabmeatMissile, 0x1D0);

	HOOK(void, __fastcall, _CrabmeatMissileUpdate, 0x00BC8A30, CrabmeatMissile* This, void*, const hh::fnd::SUpdateInfo& updateInfo)
	{
		//DebugDrawText::log(format("UNKFloat: %f", This->field_140), 0, 1, { 1,0.2,0.2,1 });
		// TODO: Make a field that determines if this is a seek missile or not.
		auto* const sonic = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!sonic)
		{
			original_CrabmeatMissileUpdate(This, nullptr, updateInfo);
			return;
		}

		// Do kill code a frame early why not
		This->m_LifeTime -= updateInfo.DeltaTime;
		if (This->m_LifeTime < 0)
		{
			// TODO: Make explode instead
			This->SendMessageImm<Sonic::Message::MsgKill>(This->m_ActorID);
			return;
		}

		// Re-purpose this as a "current time" float since it doesn't seem to be used by anything else.
		const float currentTime = This->field_140 - 1.0f;
		This->field_140 += updateInfo.DeltaTime;

		using namespace hh::math;
		FUNCTION_PTR(void, __stdcall, SetPosition, 0x00D5CE10, Sonic::CGameObject3D * _This, const hh::math::CVector & position);

		if (!This->m_IsActive)
			return;

		Hedgehog::Mirage::CTransform* const transform = &This->m_spMatrixNodeTransform->m_Transform;

		const float fallDownAmount = std::inverseLerp(2.0f, 4.0f, currentTime);
		const CVector playerDirection = CVector::Normalize(transform->m_Position - sonic->m_spMatrixNode->m_Transform.m_Position);
		const CVector seekDirection   = CVector::Slerp(-playerDirection, CVector::Down(), fallDownAmount * fallDownAmount);

		const CQuaternion rotation = CQuaternion::RotateTowards(transform->m_Rotation,
		                                                        CQuaternion::LookRotation(seekDirection).normalized(),
		                                                        120.0f * DEG2RAD * updateInfo.DeltaTime);

		const CVector missileSeekVector = rotation.Forward() * (This->m_MissileInfo.m_Speed * 0.5f * updateInfo.DeltaTime);

		transform->m_Rotation = rotation;
		SetPosition(This, transform->m_Position + missileSeekVector);
		This->m_spMatrixNodeTransform->NotifyChanged();

		//This->m_MissileInfo.m_Speed = 0.1f;
		//original_CrabmeatMissileUpdate(This, nullptr, updateInfo);
	}

	// Re-implement and then change crabmeat "find" function.
	class CEnemyCrab : public Sonic::CGameObject3D
	{
	public:
		struct SetData
		{
			//BB_INSERT_PADDING(0x150);
			BB_INSERT_PADDING(0xF0) {};
			Hedgehog::Math::CVector m_WaypointA {};
			Hedgehog::Math::CVector m_WaypointB {};
			float m_TurnTime {};
			bool m_IsTurnOver {};
			float m_SeekSpanTime {};
			float m_MoveSpeed {};
			bool m_IsEnableAttack {};
			float m_ChargeTime {};
			int m_Field_128 {};
			float m_DistanceMissile {};
			float m_SpeedMissile {};
			float m_DistanceStepMove {};
			float m_TargetLostTime {};
			int m_TargetLostWaitTime {};
			bool m_IsContinueAttack {};
			bool m_IsSimpleAttack {};
			bool m_IsChargeInPose {};
			float m_ActivationExtentY {};

			// Custom properties now
			bool m_UseDefaultSearchMethod {};
			bool m_UseSearchPoint {};

			hh::math::CVector m_SearchPosition {};
		};
		BB_INSERT_PADDING(0x0C);
		SetData setData;
	};
	ASSERT_OFFSETOF(CEnemyCrab::SetData, m_WaypointA, 0xF0);
	ASSERT_OFFSETOF(CEnemyCrab::SetData, m_WaypointB, 0x100);
	ASSERT_OFFSETOF(CEnemyCrab::SetData, m_IsSimpleAttack, 0x141);
	ASSERT_OFFSETOF(CEnemyCrab::SetData, m_IsChargeInPose, 0x142);
	ASSERT_OFFSETOF(CEnemyCrab::SetData, m_ActivationExtentY, 0x144);
	//ASSERT_SIZEOF(CEnemyCrab::SetData, 0x150);

	ASSERT_OFFSETOF(CEnemyCrab, setData, 0x100);
	//ASSERT_EQUALITY(sizeof(CEnemyCrab), 0x250);

	// Init undefined parameters
	HOOK(void, __fastcall, _CrabmeatSetobjectListener, 0x00BC8C40, CEnemyCrab::SetData* object, void*, Sonic::CEditParam* param)
	{
		original_CrabmeatSetobjectListener(object, nullptr, param);

		// Initialize here cuz constructor is optimized, and we can't hook that w/o a midasm hook (which I'd rather not do).
		object->m_UseSearchPoint = false;
		object->m_UseDefaultSearchMethod = true;
		object->m_SearchPosition = hh::math::CVector::Zero();

		param->CreateParamBool(&object->m_UseDefaultSearchMethod, "UseDefaultSearchMethod");
		param->CreateParamBool(&object->m_UseSearchPoint, "UseSearchPoint");
		param->CreateParamBase(Sonic::CParamPosition::Create(&object->m_SearchPosition), "SearchPosition");
	}

	bool __cdecl CrabmeatIsTargetInRange(CEnemyCrab* crabmeat, float range)
	{
		bool isEnableAttack = GetValue<bool>(crabmeat, 0x220);
			
		if (!isEnableAttack)
			return false;

		using namespace hh::math;
		auto* sonic = Sonic::Player::CPlayerSpeedContext::GetInstance();

		auto* const transform = &crabmeat->m_spMatrixNodeTransform->m_Transform;

		const CVector positionSonic = sonic->m_spMatrixNode->m_Transform.m_Position;
		const CVector positionCrab = transform->m_Position;

		const CQuaternion rotation = transform->m_Rotation;

		const CVector directionVector = positionSonic - positionCrab;

		if (directionVector.squaredNorm() > range * range)
			return false;

		if (crabmeat->setData.m_UseDefaultSearchMethod)
		{
			// Original method, very... "made for 2D."
			if (crabmeat->setData.m_ActivationExtentY <= fabs(directionVector.y()))
				return false;
		}
		else
		{
			// TODO: Set view frustum angle
			if (CVector::Dot(directionVector, rotation.Forward()) <= 0.0f)
				return false;
		}

		return true;
	}

	ASMHOOK CrabmeatIsTargetInRange_asm()
	{
		__asm
		{
			push[esp + 04h] // range
			push esi // a1

			call CrabmeatIsTargetInRange

			pop esi // a1
			add esp, 4 // range
			retn 04h
		}
	}

	// Motobugs

	// Rotate to player when charging
	HOOK(void, __fastcall, _MotobugStateCharge, 0x005F8870, void* state, void*, Sonic::CGameObject3D* motobug, float deltaTime)
	{
		original_MotobugStateCharge(state, nullptr, motobug, deltaTime);

		bool isPathMove = GetValue<bool>(motobug, 0x258);
		if (isPathMove)
			return;

		using namespace hh::math;

		auto* const player = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!player)
			return;

		auto* const node = motobug->m_spMatrixNodeTransform.get();

		const CQuaternion rotation = node->m_Transform.m_Rotation;
		const CVector upVector = rotation.Up();

		const CVector playerDirection = CVector::ProjectOnPlane(player->m_spMatrixNode->m_Transform.m_Position - node->m_Transform.m_Position, upVector).normalizedSafe();

		node->m_Transform.SetRotation(CQuaternion::RotateTowards(rotation, CQuaternion::LookRotation(playerDirection), 360.0f * DEG2RADf * deltaTime));
		node->NotifyChanged();
	}

	// Orcas - Fix animation speed.
	class Orca : Sonic::CGameObject3D
	{
		int a, b, c;
	public:
		struct Listener
		{
			void* VTBL;
			void* animContextVTBL;
			boost::shared_ptr<Hedgehog::Mirage::CSingleElement> m_spSingleElement;
			char m_Field110;
			int m_Field114;
			boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spOrcaMatrixNodeTransform;
			boost::shared_ptr<Hedgehog::Animation::CAnimationPose> m_spAnimationPose;
			boost::shared_ptr<Sonic::CAnimationStateMachine> m_spAnimationStateMachine;
			boost::shared_ptr<void> m_spSingleElementEffectMotionAll;
			float m_Field138;
			char m_IsActive;
			float m_CurrentTime;
			float m_OceanFloorHeight;
			int m_Field148;
			int m_Field14C;
			Hedgehog::Math::CVector DstPos;
			float MoveTime;
			float RelHeight;
			char m_SpawnParticleInArc;
			int m_Field16C;
			Hedgehog::Math::CVector m_LastPosition;
		};
		Listener listener;

	};
	BB_ASSERT_OFFSETOF(Orca::Listener, m_spSingleElement, 0x8);
	BB_ASSERT_OFFSETOF(Orca, listener, 0x100);
	BB_ASSERT_SIZEOF(Orca, 0x180);

	//FUNCTION_PTR(boost::shared_ptr<Hedgehog::Animation::CAnimationControlSingle>*, __fastcall, GetThing, 0x00CE0B40, void* a1);

	//HOOK(void, __fastcall, _OrcaAddCallback, 0x0113C990, Orca* This, void*, void* a2, void* a3)
	//HOOK(void, __fastcall, _OrcaUpdate, 0x0113D1D0, Orca* This, void*, Hedgehog::Universe::SUpdateInfo& a2)

	// Fake hook just to test something
	HOOK(void, __fastcall, _OrcaUpdate, 0x0113C990, Orca* This, void*, void* a2, void* a3)
	{
		const float moveTime = fmax(This->listener.MoveTime, 0.125f);
		const float playbackSpeed = fmin(1.0f / moveTime, 1.0f);

		//const bool moveFar = This->listener.MoveTime > 2.5f;

		//DebugDrawText::log(format("--- MOVETIME --- %f", moveTime), 10.0f);
		//DebugDrawText::log(format("--- ANIMPLAY --- %f", playbackSpeed), 10.0f);

		WRITE_MEMORY(0x01A4A8C0 + 0x08, float, playbackSpeed);
		WRITE_MEMORY(0x01A4A920 + 0x08, float, std::lerp(playbackSpeed, 1.0f, 0.75f));
		original_OrcaUpdate(This, nullptr, a2, a3);
	}


	// Homing attack item boxes.
	bool ProcMsgHomingAttack(hh::fnd::CMessageActor* This, hh::fnd::Message& message)
	{
		if (message.Is<Sonic::Message::MsgGetHomingAttackPriority>())
		{
			auto& msg = static_cast<Sonic::Message::MsgGetHomingAttackPriority&>(message);
			*msg.m_pPriority = 10;
			return true;
		}

		if (message.Is<Sonic::Message::MsgGetHomingAttackPosition>())
		{
			auto& msg = static_cast<Sonic::Message::MsgGetHomingAttackPosition&>(message);
			auto* obj = static_cast<Sonic::CGameObject3D*>(This);

			const hh::math::CQuaternion rotation = obj->m_spMatrixNodeTransform->m_Transform.m_Rotation;

			*msg.m_pPosition = obj->m_spMatrixNodeTransform->m_Transform.m_Position + (rotation.Up() * 0.5f);// +(rotation.Forward() * 0.5f);
			return true;
		}

		return false;
	}

	HOOK(bool, __fastcall, _ItemboxProcessMessage, 0x01058460, hh::fnd::CMessageActor* This, void*, hh::fnd::Message& message, bool flag)
	{
		using namespace Sonic::Message;
		if (!flag)
			return original_ItemboxProcessMessage(This, nullptr, message, flag);

		if (ProcMsgHomingAttack(This, message))
			return true;

		//DebugDrawText::log(message.GetType(), 2.0, 0, {0.4, 0.65, 1.0, 1.0});

		if (message.Is<MsgHitEventCollision>() || message.Is<MsgDamage>())
		{
			auto* obj = static_cast<Sonic::CGameObject3D*>(This);
			This->SendMessageImm(message.m_SenderActorID,
			                     MsgDamageSuccess(obj->m_spMatrixNodeTransform->m_Transform.m_Position,
			                                      true, true, true, 8));
		}

		bool result = original_ItemboxProcessMessage(This, nullptr, message, flag);

		return result;
	}

	// Virtualmirror patch
	class CObjVirtualMirror : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
	public:
		// Official:
		float m_ReflectDirection[4];
		int m_SurfaceNumber;
		// Custom:
		bool m_TwoSided;
	};

	HOOK(void, __fastcall, _MirrorAddCallback, 0x011F1680, CObjVirtualMirror* obj, void*, void* a2)
	{
		hh::math::CQuaternion* rotPtr = &obj->m_spMatrixNodeTransform->m_Transform.m_Rotation;

		original_MirrorAddCallback(obj, nullptr, a2);
	}

	HOOK(void, __fastcall, _MirrorEditParam, 0x011F1830, Sonic::CSetObjectListener* listener, void*, Sonic::CEditParam* param)
	{
		CObjVirtualMirror* object = reinterpret_cast<CObjVirtualMirror*>((int)listener - sizeof(Sonic::CObjectBase));


		//CObjVirtualMirror* object = obj;
		hh::math::CQuaternion* rotPtr = &object->m_spMatrixNodeTransform->m_Transform.m_Rotation;

		std::stringstream stream;
		//stream << std::hex << "ADDRES1: 0x" << std::uppercase << object << "\n";
		//stream << std::hex << "ADDRES2: 0x" << std::uppercase << listener;

		stream << std::hex << "ADDRES: 0x" << std::uppercase << rotPtr;
		//MessageBoxA(nullptr, stream.str().c_str(), "", MB_OK);

		original_MirrorEditParam(listener, nullptr, param);
	}

	class MsgWhatever : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		Hedgehog::Math::CVector m_Direction;
		Hedgehog::Math::CVector m_Position;
		int m_Type = 0;
	};


	// Override message actor RND
	HOOK(void, __fastcall, _MirrorMessage, 0x57B3A0, void* This, void*, MsgWhatever* msg)
	{
		using namespace hh::math;

		DebugDrawText::log(format("Mirror Y: %f", msg->m_Direction.y()));

		//msg->m_Direction = -CVector::Up();
		original_MirrorMessage(This, nullptr, msg);
	}

	// Force mysterious vector invert
	//HOOK(bool, __fastcall, _OddFunction, 0x6587D0, void* This, void*, hh::math::CVector* inVec, int a3, int a4, int a5, float a6)
	//{
	//	*inVec = -*inVec;
	//	return original_OddFunction(This, nullptr, inVec, a3, a4, a5, a6);
	//}
	HOOK(bool, __fastcall, _OddFunction, 0x6587D0, void* This, void*,
		hh::math::CVector& in_pDirection, hh::math::CVector& in_pPosition, int surfaceNumber, hh::math::CVector& VecA, hh::math::CVector& VecB)
	{
		std::stringstream stream;
		stream << VecA;
		//DebugDrawText::log(stream.str().c_str());

		return original_OddFunction(This, nullptr, in_pDirection, in_pPosition, surfaceNumber, VecA, VecB);
	}



	///////////////
	/// TO - DO's
	//-------------

	// Dash pads -- Extend their functionality to include simulating "hit" events
	class CObjDashPanel
	{

	};


	// Need to replace this ASAP.
	// Giant Chopper, used for whale chase.
	HOOK(bool, __fastcall, _GiantChopperProcessMessage, 0x00FCD590, Hedgehog::Universe::CMessageActor* actor, void*, Hedgehog::Universe::Message& msg, bool useMainClass)
	{
		DebugDrawText::log(msg.GetType(), 10.0f);

		if (!msg.Is<Sonic::Message::MsgHitNotifyRigidBody>())
			//if (!msg.Is<Sonic::Message::MsgHitEventCollision>())
			return original_GiantChopperProcessMessage(actor, nullptr, msg, useMainClass);

		Hedgehog::Universe::CMessageActor* senderActor = actor->m_pMessageManager->GetMessageActor(msg.m_SenderActorID);
		DebugDrawText::log(format("Contact: 0x%X", *(int*)senderActor), 10.0f);

		actor->SendMessageImm(senderActor, Sonic::Message::MsgNotifyObjectEvent(12));

		//bool Funny = true;
		return true;
	}

	// Temporarily extend messages for CGameObject3D
	HOOK(bool, __fastcall, _GameObjectProcessMessage, 0x00D60590, Hedgehog::Universe::CMessageActor* This, void*, Hedgehog::Universe::Message& msg, bool useMainClass)
	{
		const char* type = msg.GetType();

		if (!useMainClass)
			return original_GameObjectProcessMessage(This, nullptr, msg, false);

		if (!msg.Is<Sonic::Message::MsgGetActorID>())
			return original_GameObjectProcessMessage(This, nullptr, msg, true);

		((Sonic::Message::MsgGetActorID*)&msg)->m_ID = This->m_ActorID;
		return true;
	}

	//////////////
	/// OBSOLETE
	//------------

	/*
	// Rooftop Run gondalas
	void NOINLINE CreateBoxShape(void* out, float X, float Y, float Z)
	{
		constexpr int func = 0x00E92CF0;
		__asm
		{
			push Z
			push Y
			push X
			mov esi, out
			call func
			add esp, 0x0C
		}
	}

	void* __cdecl GondolaColliderPatch(void* out, void* Gondola, float Length, float Height, float Depth)
	{
		Sonic::CRigidBody* rigidBody = GetPointer<boost::shared_ptr<Sonic::CRigidBody>>(Gondola, 0x15C)->get();
		void* havokCollider = GetValue<void*>(rigidBody, 0x04);
		void* shape = GetValue<void*>(havokCollider, 0x10);
		hh::math::CVector* halfExtents = GetPointer<hh::math::CVector>(shape, 0x20);

		CreateBoxShape(out, halfExtents->x() - 0.0625f, Height, halfExtents->z() - 0.0625f);
		return out;
	}

	ASMHOOK GondolaColliderPatch_asm()
	{
		static constexpr int jumpOut = 0x00F0D698;
		__asm
		{
			push edi
			push esi
			call GondolaColliderPatch
			add esp, 0x14
			jmp jumpOut
		}
	}
	*/
}

// TODO: Actually map this sometime.
//namespace Hedgehog::Universe
//{
//	class CTinyStateMachineBase
//	{
//		BB_INSERT_PADDING(0x30) {};
//	};
//}

/*
namespace Hedgehog::Motion
{
	class CMotionDatabaseWrapper;

	class CModelData;

	//static inline BB_FUNCTION_PTR(void, __thiscall, fpCMirageDatabaseWrapperGetVisibilityTreeData, 0x72F490,
	//	CMirageDatabaseWrapper* This, boost::shared_ptr<CVisibilityTreeData>& out_spVisibilityTreeData, const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown);

	class CMotionDatabaseWrapper : public Hedgehog::Base::CObject
	{
	public:
		Hedgehog::Database::CDatabase* m_pDatabase;
		bool m_Flag;

		CMotionDatabaseWrapper(Hedgehog::Database::CDatabase* pDatabase)
		{
			fpCMirageDatabaseWrapperCtor(this, pDatabase);
		}

		virtual ~CMotionDatabaseWrapper() = default;

		boost::shared_ptr<CGIMipLevelLimitationData> GetGIMipLevelLimitationData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CGIMipLevelLimitationData> spGIMipLevelLimitationData;
			fpCMirageDatabaseWrapperGetGIMipLevelLimitationData(this, spGIMipLevelLimitationData, in_rName, in_Unknown);
			return spGIMipLevelLimitationData;
		}

		boost::shared_ptr<CGITextureGroupInfoData> GetGITextureGroupInfoData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CGITextureGroupInfoData> spGITextureGroupInfoData;
			fpCMirageDatabaseWrapperGetGITextureGroupInfoData(this, spGITextureGroupInfoData, in_rName, in_Unknown);
			return spGITextureGroupInfoData;
		}

		boost::shared_ptr<CLightData> GetLightData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CLightData> spLightData;
			fpCMirageDatabaseWrapperGetLightData(this, spLightData, in_rName, in_Unknown);
			return spLightData;
		}

		boost::shared_ptr<CLightFieldTreeData> GetLightFieldTreeData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CLightFieldTreeData> spLightFieldTreeData;
			fpCMirageDatabaseWrapperGetLightFieldTreeData(this, spLightFieldTreeData, in_rName, in_Unknown);
			return spLightFieldTreeData;
		}

		boost::shared_ptr<CLightListData> GetLightListData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CLightListData> spLightListData;
			fpCMirageDatabaseWrapperGetLightListData(this, spLightListData, in_rName, in_Unknown);
			return spLightListData;
		}

		boost::shared_ptr<CLodModelData> GetLodModelData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CLodModelData> spLodModelData;
			fpCMirageDatabaseWrapperGetLodModelData(this, spLodModelData, in_rName, in_Unknown);
			return spLodModelData;
		}

		boost::shared_ptr<CMaterialData> GetMaterialData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CMaterialData> spMaterialData;
			fpCMirageDatabaseWrapperGetMaterialData(this, spMaterialData, in_rName, in_Unknown);
			return spMaterialData;
		}

		boost::shared_ptr<CModelData> GetModelData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CModelData> spModelData;
			fpCMirageDatabaseWrapperGetModelData(this, spModelData, in_rName, in_Unknown);
			return spModelData;
		}

		boost::shared_ptr<CPictureData> GetPictureData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CPictureData> spPictureData;
			fpCMirageDatabaseWrapperGetPictureData(this, spPictureData, in_rName, in_Unknown);
			return spPictureData;
		}

		boost::shared_ptr<CPixelShaderCodeData> GetPixelShaderCodeData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CPixelShaderCodeData> spPixelShaderCodeData;
			fpCMirageDatabaseWrapperGetPixelShaderCodeData(this, spPixelShaderCodeData, in_rName, in_Unknown);
			return spPixelShaderCodeData;
		}

		boost::shared_ptr<CPixelShaderData> GetPixelShaderData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CPixelShaderData> spPixelShaderData;
			fpCMirageDatabaseWrapperGetPixelShaderData(this, spPixelShaderData, in_rName, in_Unknown);
			return spPixelShaderData;
		}

		boost::shared_ptr<CPixelShaderParameterData> GetPixelShaderParameterData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CPixelShaderParameterData> spPixelShaderParameterData;
			fpCMirageDatabaseWrapperGetPixelShaderParameterData(this, spPixelShaderParameterData, in_rName, in_Unknown);
			return spPixelShaderParameterData;
		}

		boost::shared_ptr<CSceneData> GetSceneData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CSceneData> spSceneData;
			fpCMirageDatabaseWrapperGetSceneData(this, spSceneData, in_rName, in_Unknown);
			return spSceneData;
		}

		boost::shared_ptr<CShaderListData> GetShaderListData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CShaderListData> spShaderListData;
			fpCMirageDatabaseWrapperGetShaderListData(this, spShaderListData, in_rName, in_Unknown);
			return spShaderListData;
		}

		boost::shared_ptr<CTerrainBlockSphereTreeData> GetTerrainBlockSphereTreeData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTerrainBlockSphereTreeData> spTerrainBlockSphereTreeData;
			fpCMirageDatabaseWrapperGetTerrainBlockSphereTreeData(this, spTerrainBlockSphereTreeData, in_rName, in_Unknown);
			return spTerrainBlockSphereTreeData;
		}

		boost::shared_ptr<CTerrainData> GetTerrainData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTerrainData> spTerrainData;
			fpCMirageDatabaseWrapperGetTerrainData(this, spTerrainData, in_rName, in_Unknown);
			return spTerrainData;
		}

		boost::shared_ptr<CTerrainGroupData> GetTerrainGroupData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTerrainGroupData> spTerrainGroupData;
			fpCMirageDatabaseWrapperGetTerrainGroupData(this, spTerrainGroupData, in_rName, in_Unknown);
			return spTerrainGroupData;
		}

		boost::shared_ptr<CTerrainInstanceInfoData> GetTerrainInstanceInfoData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTerrainInstanceInfoData> spTerrainInstanceInfoData;
			fpCMirageDatabaseWrapperGetTerrainInstanceInfoData(this, spTerrainInstanceInfoData, in_rName, in_Unknown);
			return spTerrainInstanceInfoData;
		}

		boost::shared_ptr<CTerrainModelData> GetTerrainModelData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTerrainModelData> spTerrainModelData;
			fpCMirageDatabaseWrapperGetTerrainModelData(this, spTerrainModelData, in_rName, in_Unknown);
			return spTerrainModelData;
		}

		boost::shared_ptr<CTexsetData> GetTexsetData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTexsetData> spTexsetData;
			fpCMirageDatabaseWrapperGetTexsetData(this, spTexsetData, in_rName, in_Unknown);
			return spTexsetData;
		}

		boost::shared_ptr<CTextureData> GetTextureData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CTextureData> spTextureData;
			fpCMirageDatabaseWrapperGetTextureData(this, spTextureData, in_rName, in_Unknown);
			return spTextureData;
		}

		boost::shared_ptr<CVertexShaderCodeData> GetVertexShaderCodeData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVertexShaderCodeData> spVertexShaderCodeData;
			fpCMirageDatabaseWrapperGetVertexShaderCodeData(this, spVertexShaderCodeData, in_rName, in_Unknown);
			return spVertexShaderCodeData;
		}

		boost::shared_ptr<CVertexShaderData> GetVertexShaderData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVertexShaderData> spVertexShaderData;
			fpCMirageDatabaseWrapperGetVertexShaderData(this, spVertexShaderData, in_rName, in_Unknown);
			return spVertexShaderData;
		}

		boost::shared_ptr<CVertexShaderParameterData> GetVertexShaderParameterData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVertexShaderParameterData> spVertexShaderParameterData;
			fpCMirageDatabaseWrapperGetVertexShaderParameterData(this, spVertexShaderParameterData, in_rName, in_Unknown);
			return spVertexShaderParameterData;
		}

		boost::shared_ptr<CVisibilityGridData> GetVisibilityGridData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVisibilityGridData> spVisibilityGridData;
			fpCMirageDatabaseWrapperGetVisibilityGridData(this, spVisibilityGridData, in_rName, in_Unknown);
			return spVisibilityGridData;
		}

		boost::shared_ptr<CVisibilityGridInfoData> GetVisibilityGridInfoData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVisibilityGridInfoData> spVisibilityGridInfoData;
			fpCMirageDatabaseWrapperGetVisibilityGridInfoData(this, spVisibilityGridInfoData, in_rName, in_Unknown);
			return spVisibilityGridInfoData;
		}

		boost::shared_ptr<CVisibilityTreeData> GetVisibilityTreeData(const Hedgehog::Base::CSharedString& in_rName, size_t in_Unknown = 0)
		{
			boost::shared_ptr<CVisibilityTreeData> spVisibilityTreeData;
			fpCMirageDatabaseWrapperGetVisibilityTreeData(this, spVisibilityTreeData, in_rName, in_Unknown);
			return spVisibilityTreeData;
		}
	};

	BB_ASSERT_OFFSETOF(CMirageDatabaseWrapper, m_pDatabase, 0x4);
	BB_ASSERT_OFFSETOF(CMirageDatabaseWrapper, m_Flag, 0x8);
	BB_ASSERT_SIZEOF(CMirageDatabaseWrapper, 0xC);
}
*/

void SetObjectMods::Init()
{
	// Extend messages for all gameobjects temporarily, until we can just get the actor from a setobject ID.
	INSTALL_HOOK(_GameObjectProcessMessage)


	INSTALL_HOOK(_MirrorMessage)
	//INSTALL_HOOK(_OddFunction)
	//INSTALL_HOOK(_MirrorAddCallback)
	//INSTALL_HOOK(_MirrorEditParam)

	// DEBUG
	//WRITE_MEMORY(0x00F0C69D + 1, char*, "seaobj_table")

	//INSTALL_HOOK(_DashRingCollisionEnter)
	//INSTALL_HOOK(_DashRingUpdate)

	// Csc Platform
	WRITE_MEMORY(0x00F1F6CE + 1, char*, "SpringResist")

	// Euc Platform
	//WRITE_JUMP(0x00F0D690, GondolaColliderPatch_asm)

	// Teleporter
	WRITE_MEMORY(0x00FBF500, uint32_t, sizeof(CObjPipeEnterCollision))
	INSTALL_HOOK(_PipeEnterCollisionSetParams)
	INSTALL_HOOK(_PipeEnterCollisionContact)

	// JumpCollision
	INSTALL_HOOK(_JumpCollisionOnEventTrigger)
	INSTALL_HOOK(_JumpCollisionSetParams)

	// Crabmeat stuff
	INSTALL_HOOK(_CrabmeatMissileUpdate)
	INSTALL_HOOK(_CrabmeatSetobjectListener)
	WRITE_JUMP(0x00BC8210, CrabmeatIsTargetInRange_asm)
	WRITE_MEMORY(0x00BCB755 + 1, uint32_t, sizeof(CEnemyCrab))
	WRITE_MEMORY(0x00BCB595 + 1, uint32_t, sizeof(CEnemyCrab))

	// Motobug
	INSTALL_HOOK(_MotobugStateCharge)

	// Orca
	INSTALL_HOOK(_OrcaUpdate)

	// Giant Chopper (Attack Orca)
	INSTALL_HOOK(_GiantChopperProcessMessage)
	WRITE_MEMORY(0x00FCCDA3 + 2, uint32_t, 0x01E0AFE8) // This lets the orca push the wood away, which is cool!

	// Handle homing lock-on for Item (Credit to Brian for this, slight mods to use itembox instead of "item")
	WRITE_MEMORY(0x01057DFE + 0x02, uint32_t, 0x1E0AF34);
	WRITE_MEMORY(0x01057D93 + 0x01, uint32_t, 0x1E0AF34);
	INSTALL_HOOK(_ItemboxProcessMessage);
	//INSTALL_HOOK(Itembox_CObjItemMsgHitEventCollision);
}

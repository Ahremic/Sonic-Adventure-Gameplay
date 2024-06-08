#pragma once
// Messages to eventually apply to BlueBlur
namespace Sonic::Message
{
	class MsgGetGroundInfo : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167FB50);

		// The normal vector of the ground underneath Sonic.
		// This returns a vector even when airborne. Will return CVector::Up() if distance >= 30.0f.
		hh::math::CVector m_GroundNormal;
		// Distance to the ground surface.
		// 0 when grounded, maxes out at 30.0f
		float m_Distance;
		// Returns 1 if player has made contact with the ground.
		bool m_OnGround;
		// Surface flags. TODO: Document these.
		uint32_t m_Flags;

		MsgGetGroundInfo()
		{
			using namespace hh::math;
			m_GroundNormal = CVector::Zero();
			m_Distance = 0.0f;
			m_OnGround = false;
			m_Flags = 0;
		}
	};

	BB_ASSERT_OFFSETOF(MsgGetGroundInfo, m_OnGround, 0x24);
	BB_ASSERT_OFFSETOF(MsgGetGroundInfo, m_Flags, 0x28);

	class MsgGetDriftState : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F660);

		bool m_IsDrifting;
		bool m_Cond2;
		bool m_IsRight;

		MsgGetDriftState()
		{
			m_IsDrifting = false;
			m_Cond2 = false;
			m_IsRight = false;
		}
	};

	class MsgGetForCamera3DState : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F718);

		bool m_IsNearBorder;
		BB_INSERT_PADDING(15) {};
		Hedgehog::Math::CVector m_WorldInput;
		bool m_IgnoreWorldInput;
		BB_INSERT_PADDING(15) {};

		MsgGetForCamera3DState()
		{
			using namespace hh::math;
			m_IsNearBorder = false;
			m_WorldInput = CVector::Zero();
			m_IgnoreWorldInput = false;
		}
	};

	BB_ASSERT_OFFSETOF(MsgGetForCamera3DState, m_IsNearBorder, 0x10);
	BB_ASSERT_OFFSETOF(MsgGetForCamera3DState, m_WorldInput, 0x20);
	BB_ASSERT_OFFSETOF(MsgGetForCamera3DState, m_IgnoreWorldInput, 0x30);

	class MsgGetDashModeInfo : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F79C);

		bool m_IsOnPath;
		bool m_Bool2;
		hh::math::CVector m_ClosestPointOnPath; // 0x20
		hh::math::CVector m_UpDirection; // 0x30
		hh::math::CVector m_FrontDirection; // 0x40
		float m_PathRadius;
		bool m_Bool3;

		MsgGetDashModeInfo()
		{
			using namespace hh::math;
			m_IsOnPath = false;
			m_Bool2 = false;
			m_Bool3 = false;
			m_ClosestPointOnPath = CVector::Zero();
			m_UpDirection = CVector::Zero();
			m_FrontDirection = CVector::Zero();
			m_PathRadius = 0;
		}
	};

	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_IsOnPath, 0x10);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_Bool2, 0x11);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_ClosestPointOnPath, 0x20);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_UpDirection, 0x30);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_FrontDirection, 0x40);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_PathRadius, 0x50);
	BB_ASSERT_OFFSETOF(MsgGetDashModeInfo, m_Bool3, 0x54);

	class MsgGetHomingAttackTargetInfo : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167FB88);

		uint32_t m_TargetID = 0;
		hh::math::CVector m_Position = hh::math::CVector::Zero();
	};

	class MsgGetCameraTargetPosition : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F690);

		hh::math::CVector* m_pPosition = nullptr;
		float m_unk1 = 0.0f;
		bool m_boolA = false;
		bool m_NoOffset = false;
		bool m_UseModelMatrix = true;

		MsgGetCameraTargetPosition(Hedgehog::Math::CVector* position)
			: m_pPosition(position)
		{
		}

		MsgGetCameraTargetPosition(Hedgehog::Math::CVector& position)
			: m_pPosition(&position)
		{
		}
	};

	class MsgGetVelocity : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016818A0);

		hh::math::CVector* m_Velocity = nullptr;

		MsgGetVelocity(Hedgehog::Math::CVector* vec)
			: m_Velocity(vec)
		{
		}

		MsgGetVelocity(Hedgehog::Math::CVector& vec)
			: m_Velocity(&vec)
		{
		}
	};

	class MsgGetPreviousVelocity : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016818B0);

		hh::math::CVector* m_Velocity = nullptr;

		MsgGetPreviousVelocity(Hedgehog::Math::CVector* vec)
			: m_Velocity(vec)
		{
		}

		MsgGetPreviousVelocity(Hedgehog::Math::CVector& vec)
			: m_Velocity(&vec)
		{
		}
	};

	class MsgGetUpDirection : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F580);

		hh::math::CVector* m_Direction = nullptr;

		MsgGetUpDirection(Hedgehog::Math::CVector* vec)
			: m_Direction(vec)
		{
		}

		MsgGetUpDirection(Hedgehog::Math::CVector& vec)
			: m_Direction(&vec)
		{
		}
	};

	class MsgGetFrontDirection : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F5AC);

		hh::math::CVector* m_Direction = nullptr;

		MsgGetFrontDirection(Hedgehog::Math::CVector* vec)
			: m_Direction(vec)
		{
		}

		MsgGetFrontDirection(Hedgehog::Math::CVector& vec)
			: m_Direction(&vec)
		{
		}
	};

	class MsgGetModelUpDirection : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F594);

		hh::math::CVector* m_Direction = nullptr;

		MsgGetModelUpDirection(Hedgehog::Math::CVector* vec)
			: m_Direction(vec)
		{
		}

		MsgGetModelUpDirection(Hedgehog::Math::CVector& vec)
			: m_Direction(&vec)
		{
		}
	};

	class MsgIsOnBoard : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F680);

		bool* m_IsOnBoard = nullptr;

		MsgIsOnBoard(bool* out)
			: m_IsOnBoard(out)
		{
		}

		MsgIsOnBoard(bool& out)
			: m_IsOnBoard(&out)
		{
		}
	};

	class MsgIsPlayerDead : public hh::fnd::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167FF94);

		bool m_IsDead;

		MsgIsPlayerDead()
		{
			m_IsDead = false;
		}
	};

	BB_ASSERT_SIZEOF(MsgIsOnBoard, 0x14);
	BB_ASSERT_SIZEOF(MsgGetVelocity, 0x14);
	BB_ASSERT_SIZEOF(MsgGetGroundInfo, 0x30);
	BB_ASSERT_SIZEOF(MsgGetDriftState, 0x14);
	BB_ASSERT_SIZEOF(MsgGetDashModeInfo, 0x60);
	BB_ASSERT_SIZEOF(MsgGetForCamera3DState, 0x40);
	BB_ASSERT_SIZEOF(MsgGetCameraTargetPosition, 0x1C);
	BB_ASSERT_SIZEOF(MsgGetHomingAttackTargetInfo, 0x30);

	class MsgNotifyObjectEvent : public hh::fnd::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016811C0);

		int m_EventType = 0;
		bool m_Condition = false;

		MsgNotifyObjectEvent(int type) : m_EventType(type), m_Condition(false) {}
	};

	class MsgHitNotifyRigidBody : public hh::fnd::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016368D4);
	};


	class MsgGetPlayerType : public Hedgehog::Universe::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167FF00);
	};

	class MsgApplyImpulse : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01680D84);

		Hedgehog::Math::CVector m_Vec01;
		Hedgehog::Math::CVector m_Vec02;
		float m_OutOfControl;
		float m_Unk09 = 0;
		int m_ImpulseFlags;
		float m_Unk11 = 0;
		bool m_SetVelocity;
		bool m_ActiveOnGround;
		char field_42 = 0;
		char field_43 = 0;
		char m_Unk13 = 0;
		char field_45 = 0;
		char field_46 = 0;
		char field_47 = 0;
		int m_Unk14 = 0;
		int m_Unk15 = 0;
		Hedgehog::Math::CVector m_Unk16 = hh::math::CVector::Zero();
		float m_Unk20 = -1.0f;
		int m_Unk21 = 0;
		int m_Unk22 = 0;
		int m_Unk23 = 0;

		MsgApplyImpulse(const hh::math::CVector& VecA, const hh::math::CVector& VecB, float outOfControl, int flags, bool setVelocity, bool activeOnGround = true)
			: m_Vec01(VecA),
			m_Vec02(VecB),
			m_OutOfControl(outOfControl),
			m_ImpulseFlags(flags),
			m_SetVelocity(setVelocity),
			m_ActiveOnGround(activeOnGround)
		{}
	};


	class MsgDamage : public hh::fnd::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01681E80);

		//Hedgehog::Base::SSymbolNode* m_DamageType {};
		uint32_t m_DamageType {};
		int m_DamageEffectID {};
		Hedgehog::Math::CVector m_DamagePosition {};
		Hedgehog::Math::CVector m_PositionB {};
		Hedgehog::Math::CVector m_Velocity {};
		int m_SuccessActorIDOverride {};

		MsgDamage(uint32_t in_DamageType, const Hedgehog::Math::CVector& position, const Hedgehog::Math::CVector& velocity)
		: m_DamageType(in_DamageType), m_DamageEffectID(0), m_DamagePosition(position), m_PositionB(position), m_Velocity(velocity),  m_SuccessActorIDOverride(0)
		{}

		MsgDamage(uint32_t in_DamageType, const Hedgehog::Math::CVector& position)
		: m_DamageType(in_DamageType), m_DamageEffectID(0), m_DamagePosition(position), m_PositionB(position), m_Velocity(Hedgehog::Math::CVector::Zero()),  m_SuccessActorIDOverride(0)
		{}
	};
	BB_ASSERT_SIZEOF(MsgDamage, 0x60);


	class MsgDamageSuccess : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x1681E6C);

		Hedgehog::Math::CVector m_Position;
		size_t m_Field20;
		size_t m_Field24; // value from MsgDamage + 0x14?
		float m_AwardBoostRatio; // used when m_AwardBoostType = 8, award boost by (MaxBoost * ratio)
		size_t m_AwardBoostType; // has to do with awarding boost
		bool m_Flag; // unknown?
		bool m_Field31; // always false?
		size_t m_DisableHitParticle;
		size_t m_DisableBounce;
		BB_INSERT_PADDING(0x4);

		// the same order as sub_4F9E90
		MsgDamageSuccess
		(
			Hedgehog::Math::CVector const& in_Position,
			bool in_Flag,
			size_t in_DisableHitParticle = 0,
			size_t in_DisableBounce = 0,
			size_t in_AwardBoostType = 0,
			float in_AwardBoostRatio = 0.0f,
			size_t in_Field20 = 0,
			size_t in_Field24 = 0
		)
			: m_Position(in_Position)
			, m_Flag(in_Flag)
			, m_Field20(in_Field20)
			, m_AwardBoostType(in_AwardBoostType)
			, m_Field24(in_Field24)
			, m_AwardBoostRatio(in_AwardBoostRatio)
			, m_Field31(0)
			, m_DisableHitParticle(in_DisableHitParticle)
			, m_DisableBounce(in_DisableBounce)
		{}
	};
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_Position, 0x10);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_Field20, 0x20);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_Field24, 0x24);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_AwardBoostRatio, 0x28);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_AwardBoostType, 0x2C);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_Flag, 0x30);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_Field31, 0x31);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_DisableHitParticle, 0x34);
	BB_ASSERT_OFFSETOF(MsgDamageSuccess, m_DisableBounce, 0x38);
	BB_ASSERT_SIZEOF(MsgDamageSuccess, 0x40);

	// Have these here just for the "Is<>" method
	class MsgHitEventCollision : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01682064);

		int m_ID;
		int m_ID2;

		// Default constructor, most hit/leave events don't require IDs at all.
		MsgHitEventCollision() : m_ID(0), m_ID2(0) {}

		MsgHitEventCollision(int id, int id2) : m_ID(id), m_ID2(id2) {}
	};
	class MsgLeaveEventCollision : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0168207C);

		int m_ID;

		MsgLeaveEventCollision() : m_ID(0) {}
		MsgLeaveEventCollision(int id) : m_ID(id) {}
	};

	class __declspec(align(4)) MsgStartExternalControl : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01680810);

		boost::shared_ptr<CMatrixNodeTransform> m_spControlNode;
		bool TerrainCollisionEnable;
		bool ForceGroundMaybe;
		bool ChangeToSpin;
		bool DisableSuperSonic = false;
		bool ObserveBoost = false;
		bool ObserveSpin = false;
		bool ObserveInput = false;
		bool EscapeEnabled = false;
		bool NoDamage = false;
		int ChangeCollisionFlags;
		int DamageType;
		char m_Field_2C = 1;
		char m_Field_2D = 0;
		char m_Field_2E = 1;
		char m_Field_2F = 0;

		MsgStartExternalControl(
			const boost::shared_ptr<CMatrixNodeTransform>& in_spControlNode,
			bool in_TerrainCollisionEnable, bool in_ForceGroundMaybe, bool in_ChangeToSpin, int in_ChangeCollisionFlags
		)
		: m_spControlNode(in_spControlNode),
		  TerrainCollisionEnable(in_TerrainCollisionEnable),
		  ForceGroundMaybe(in_ForceGroundMaybe),
		  ChangeToSpin(in_ChangeToSpin),
		  ChangeCollisionFlags(in_ChangeCollisionFlags),
		  DamageType(*reinterpret_cast<int*>(0x01E61B5C))
		{}

		MsgStartExternalControl(
			const boost::shared_ptr<CMatrixNodeTransform>& in_spControlNode,
			bool in_TerrainCollisionEnable, bool in_ForceGroundMaybe, bool in_ChangeToSpin
		)
		: m_spControlNode(in_spControlNode),
		  TerrainCollisionEnable(in_TerrainCollisionEnable),
		  ForceGroundMaybe(in_ForceGroundMaybe),
		  ChangeToSpin(in_ChangeToSpin),
		  ChangeCollisionFlags(0),
		  DamageType(*reinterpret_cast<int*>(0x01E61B5C))
		{}

		MsgStartExternalControl(
			const boost::shared_ptr<CMatrixNodeTransform>& in_spControlNode,
			bool in_TerrainCollisionEnable, bool in_ForceGroundMaybe
		)
		: m_spControlNode(in_spControlNode),
		  TerrainCollisionEnable(in_TerrainCollisionEnable),
		  ForceGroundMaybe(in_ForceGroundMaybe),
		  ChangeToSpin(false),
		  ChangeCollisionFlags(0),
		  DamageType(*reinterpret_cast<int*>(0x01E61B5C))
		{}

		// These aren't in the base game, but can prove useful.
		MsgStartExternalControl(
			const boost::shared_ptr<CMatrixNodeTransform>& in_spControlNode,
			bool in_TerrainCollisionEnable, bool in_ForceGroundMaybe, bool in_ChangeToSpin, int in_ChangeCollisionFlags,
			int in_DamageType
		)
		: m_spControlNode(in_spControlNode),
		  TerrainCollisionEnable(in_TerrainCollisionEnable),
		  ForceGroundMaybe(in_ForceGroundMaybe),
		  ChangeToSpin(in_ChangeToSpin),
		  ChangeCollisionFlags(in_ChangeCollisionFlags),
		  DamageType(in_DamageType)
		{}

		MsgStartExternalControl(
			const boost::shared_ptr<CMatrixNodeTransform>& in_spControlNode
		)
		: m_spControlNode(in_spControlNode),
		  TerrainCollisionEnable(true),
		  ForceGroundMaybe(false),
		  ChangeToSpin(false),
		  ChangeCollisionFlags(0),
		  DamageType(*reinterpret_cast<int*>(0x01E61B5C))
		{}
	};

	class MsgFinishExternalControl : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01680828)

		enum class EChangeState
		{
			FALL = 0x0,
			JUMP = 0x1,
			DEAD = 0x2,
			FINISH = 0x3,
			STAND = 0x4,
			ROCKETLAUNCH = 0x5,
			ROCKETLAUNCH2 = 0x6,
			SPIKEWALK = 0x7,
			SPIKEJUMP = 0x8,
			SPIKEFALL = 0x9,
		};

		EChangeState ExitState = EChangeState::FALL;
		bool EnableHomingAttack = false;
		bool SetBallModel = false;
		bool ForceFinish = false;
		int RocketLaunchInt = 0;
		Hedgehog::Math::CVector UpVector = Hedgehog::Math::CVector::Zero();

		MsgFinishExternalControl() = default;

		MsgFinishExternalControl(EChangeState in_ChangeState)
		: ExitState(in_ChangeState)
		{}

		MsgFinishExternalControl(EChangeState in_ChangeState, bool in_EnableHomingAttack)
		: ExitState(in_ChangeState), EnableHomingAttack(in_EnableHomingAttack)
		{}

		MsgFinishExternalControl(EChangeState in_ChangeState, bool in_EnableHomingAttack, bool in_SetBallModel)
		: ExitState(in_ChangeState), EnableHomingAttack(in_EnableHomingAttack), SetBallModel(in_SetBallModel)
		{}
	};

	class MsgChangeMotionInExternalControl : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016808B8);

		Hedgehog::Base::CSharedString AnimationName;
		enum ChangeType
		{
			SPIN = 0,
			RISINGFAN = 1,
			CUSTOM = 2
		} m_ChangeType = CUSTOM;
		bool m_Field18 = true;
		bool m_Field19 = false;
		bool m_Field1A = false;
		bool m_UseOriginalBlendSpeed = true;

		MsgChangeMotionInExternalControl(const Hedgehog::Base::CSharedString& animName) : AnimationName(animName) {}

		MsgChangeMotionInExternalControl(const Hedgehog::Base::CSharedString& animName, ChangeType type)
		: AnimationName(animName), m_ChangeType(type) {}

		MsgChangeMotionInExternalControl(const Hedgehog::Base::CSharedString& animName, ChangeType type, bool a, bool b, bool c, bool overrideBlend)
		: AnimationName(animName), m_ChangeType(type), m_Field18(a), m_Field19(b), m_Field1A(c), m_UseOriginalBlendSpeed(!overrideBlend) {}

		MsgChangeMotionInExternalControl(const Hedgehog::Base::CSharedString& animName, bool a, bool b, bool c, bool overrideBlend)
		: AnimationName(animName), m_Field18(a), m_Field19(b), m_Field1A(c), m_UseOriginalBlendSpeed(!overrideBlend) {}

		MsgChangeMotionInExternalControl(const Hedgehog::Base::CSharedString& animName, bool overrideBlend)
		: AnimationName(animName), m_UseOriginalBlendSpeed(!overrideBlend) {}
	};


	class MsgIsExternalControl : public Hedgehog::Universe::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01680B4C);

		bool IsTrue = false;
	};
	BB_ASSERT_SIZEOF(MsgIsExternalControl, 0x14);


	// Debug related stuff
	class MsgDebugView : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x016820B4);

		int m_Type;
		bool m_Active;

		MsgDebugView() : m_Type(1), m_Active(true) {}
		MsgDebugView(int Type) : m_Type(Type), m_Active(true) {}
	};

	class MsgSetObjectSave : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167EDB4);

		Hedgehog::Base::CSharedString m_Name;	
		bool m_Success;

		MsgSetObjectSave(const Hedgehog::Base::CSharedString& name) : m_Name(name), m_Success(false) {}
	};

	class MsgSaveSetLayer : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167F03C);

		uint32_t m_LayerIndex;
		bool m_Success;

		MsgSaveSetLayer() : m_LayerIndex(0), m_Success(false) {}
		MsgSaveSetLayer(int count) : m_LayerIndex(count), m_Success(false) {}
	};

	class MsgGatherActorID : public Hedgehog::Universe::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x01681F24);

		hh::vector<uint32_t>** m_pIDs;

		MsgGatherActorID(hh::vector<uint32_t>* idVector) : m_pIDs(&idVector) {}
	};

	// Custom messages!
	class MsgGetActorID : public Hedgehog::Universe::MessageTypeGet
	{
	public:
		HH_FND_MSG_MAKE_TYPE("MsgGetActorID");

		uint32_t m_ID;

		MsgGetActorID() : m_ID(0) {}
	};



	/*
	class Message : public Base::CObject
	{
	public:
		uint32_t m_SenderActorID;
		boost::shared_ptr<const Message> m_spSelf;

		virtual ~Message() = default;

		virtual bool IsOfType(const char* pType) const = 0;
		virtual const char* GetType() const = 0;

		template<typename T>
		bool Is() const
		{
			return IsOfType(T::ms_Type);
		}
	};

	BB_ASSERT_OFFSETOF(Message, m_SenderActorID, 0x4);
	BB_ASSERT_OFFSETOF(Message, m_spSelf, 0x8);
	BB_ASSERT_SIZEOF(Message, 0x10);

	class MessageTypeGet : public Message {};
	class MessageTypeSet : public Message {};
	*/
}
// UNDONE ReSharper disable CppCStyleCast
// ReSharper disable CppClangTidyClangDiagnosticOldStyleCast

#include "../DebugDrawText.h"
#include "Cameras.h"

#include <chrono>
#include <Hedgehog/Universe/Engine/hhStateMachineBase.h>

#include "Collisions.h"
#include "Config.h"

// TODO: Add to Blueblur
namespace Sonic
{
	class CRayCastCollision;
	class CBoostCameraPlugin;

	class CPlayer3DNormalCamera : public Hedgehog::Universe::TStateMachine<Sonic::CCamera>::TState
	{
	public:
		struct SParams
		{
			float TargetSideSensitive;
			float TargetSideSensitiveInQuickStep;
			float m_ParamDashPathSideMoveRate;
		};
		struct CListener
		{
			void* vftable;
			BB_INSERT_PADDING(0x0C);
			Hedgehog::Math::CVector m_Vector01;
			Hedgehog::Math::CVector m_Vector02;
			Hedgehog::Math::CVector m_Vector03;
			boost::shared_ptr<Sonic::CRayCastCollision> m_spRayCastCollision;
		};

		float m_FovInRadians{};
		bool m_Field68{};
		int m_Field6C{};
		Hedgehog::Math::CVector m_CameraPositionVisual = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraUpVector = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraTargetPosition = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraPositionInputReference = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraUpVector2 = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraTargetPosition2 = Hedgehog::Math::CVector::Zero();
		char m_FieldD0[16]{};
		int m_FieldE0{};
		char m_FieldE4[28]{};
		SParams* m_pParams{};
		char m_Field104[12]{};
		void* characterProxy{};
		float m_CameraRotationUnknown{};
		float m_CameraTargetPositionY{};
		int m_Field11C{};
		Hedgehog::Math::CVector m_TargetFrontOffset = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_TargetSideOffset = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_CameraPositionCollision = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_TargetOffsetPosition = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_WorkingUpVector = Hedgehog::Math::CVector::Zero();
		Sonic::CCharacterProxy* m_pCharacterProxy;
		int m_Field174{};
		Sonic::CBoostCameraPlugin* m_pBoostCameraPlugin{};
		void* unk_void{};
		bool m_IsQuickstepping{};
		bool m_IsOnBoard{};
		__int16 gap182{};
		float m_TargetSideQuickstepTimer{};
		float m_CameraTargetPitch{};
		float m_DistanceOffset1{};
		float m_DistanceOffset2{};
		char m_Field194[8]{};
		int m_Field19C{};
		Hedgehog::Math::CVector m_Field1A0 = Hedgehog::Math::CVector::Zero();
		float m_CameraOrbitX{};
		float m_CameraOrbitY{};
		char m_Field1B8[8]{};
		Hedgehog::Math::CVector m_DashPathRightVector = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_DashPathEasePositionTarget = Hedgehog::Math::CVector::Zero();
		Hedgehog::Math::CVector m_DashPathEasePositionCamera = Hedgehog::Math::CVector::Zero();

		bool m_IsDashPathEasing{};
		bool m_IsFacingForward{};
		float m_DashPathEaseTime{};
		CListener* m_pListener{};
		char m_Field1FC{};
		char m_Field1FD{};
		char m_Field1FE{};
		char m_Field1FF{};
	};
	ASSERT_SIZEOF(CPlayer3DNormalCamera, 0x200);


	class CBoostCameraPlugin : public Hedgehog::Universe::CStateMachineBase::CStateBase
	{
	public:
		int start;
		int m_Field64;
		int m_Field68;
		int m_Field6C;
		int m_Field70;
		int m_Field74;
		int m_Field78;
		float m_DefaultCameraTransitionTime;
		bool m_IsBoosting;
		float m_BoostTimer;
		int m_Field88;
		int m_Field8C;
		int m_Field90;
		int m_Field94;
		int m_Field98;
		int m_Field9C;
		int m_FieldA0;
		int m_FieldA4;
		int m_FieldA8;
		int m_FieldAC;
		int m_FieldB0;
		int m_FieldB4;
		int m_FieldB8;
		int m_FieldBC;
		int m_FieldC0;
		int m_FieldC4;
		int m_FieldC8;
		int m_FieldCC;
		int m_FieldD0;
		int m_FieldD4;
		int m_FieldD8;
		int m_FieldDC;
		int m_FieldE0;
		int m_FieldE4;
		int m_FieldE8;
		int m_FieldEC;
	};
}

// Dependencies
namespace DefaultCameraDependencies
{
	class CameraParams
	{
	public:
		/*
		 * Unleashed params:
		 *
		 *	static const float DRIFT_TARGET_SIDE_OFFSET_SCALE  =   3.0f
		 *	static const float CAMERA_PITCH_MOVE_VELOCITY_MIN  =  20.0f
		 * 	static const float CAMERA_MAX_PITCH                =  60.0f
		 *
		 */

		static inline float TARGET_FRONT_OFFSET_SCALE = 0.012000f;
		static inline float TARGET_FRONT_OFFSET_SENSITIVE = 50.000000f;
		static inline float SLOPE_SENSITIVE = 1.000000f;
		static inline float SLOPE_SENSITIVE_VELOCITY_SCALE = 0.500000f;
		static inline float SLOPE_SENSITIVE_VELOCITY_SCALE_VELOCITY_OFFSET = 20.000000f;
		static inline float SLOPE_SENSITIVE_MAX = 8.000000f;
		static inline float SLOPE_SENSITIVE_AIR = 6.000000f;
		static inline float SLOPE_CAMERAUP_RATE = 0.350000f;
		static inline float SLOPE_AFFECT_MIN_VELOCITY = 20.000000f;
		static inline float SLOPE_AFFECT_MAX_VELOCITY = 50.000000f;
		static inline float TARGET_UP_OFFSET_IN_AIR = 1.000000f;
		static inline float TARGET_UP_OFFSET_SENSITIVE = 4.000000f;
		static inline float TARGET_UP_FINAL_OFFSET = -0.620000f;
		static inline float TARGET_UP_SENSITIVE = 30.000000f;
		static inline float TARGET_DOWN_SENSITIVE_GROUND_NEAR = 50.000000f;
		static inline float TARGET_DOWN_SENSITIVE_GROUND_FAR_NEAR_BORDER = 5.000000f;
		static inline float CAMERA_MAX_PITCH = 67.000000f;
		static inline float CAMERA_MIN_PITCH = -20.000000f;
		static inline float CAMERA_TARGET_PITCH_SENSITIVE = 4.000000f;
		static inline float CAMERA_PITCH_MOVE_VELOCITY_MIN = 14.700000f;
		static inline float CAMERA_DISTANCE_MAX_RATE = 6.000000f;
		static inline float CAMERA_DISTANCE_MAX_RATE_WATER = 100.000000f;
		static inline float CAMERA_DISTANCE_SCALE = 1.150000f;
		static inline float CAMERA_DISTANCE_MINIMUM = 0.800000f;
		static inline float CAMERA_DISTANCE_NEGATIVE_PITCH_SCALE = 0.034000f;
		static inline float CAMERA_DISTANCE_POSITIVE_PITCH_SCALE = 0.120000f;
		static inline float CAMERA_DISTANCE_BASIC_RANGE = 15.000000f;
		static inline float CAMERA_POSITION_VELOCITY_RATE = 0.750000f;
		static inline float TARGET_SIDE_OFFSET_SCALE = 1.680000f;
		static inline float TARGET_SIDE_OFFSET_VELOCITY_SCALE = 0.020000f;
		static inline float TARGET_SIDE_OFFSET_SENSITIVE = 1.000000f;
		static inline float TARGET_SIDE_OFFSET_RECOVER_SENSITIVE = 4.000000f;
		static inline float TARGET_SENSITIVE = 50.000000f;
		static inline float TARGET_UP_POSITIVE_SENSITIVE_GROUND = 5.000000f;
		static inline float TARGET_UP_NEGATIVE_SENSITIVE_GROUND = 5.000000f;
		static inline float TARGET_UP_POSITIVE_SENSITIVE_VELSCALE_GROUND = 20.000000f;
		static inline float TARGET_UP_POSITIVE_SENSITIVE_AIR = 10.000000f;
		static inline float TARGET_UP_NEGATIVE_SENSITIVE_AIR = 10.000000f;
		static inline float TARGET_UP_DIFF_POSITIVE_MAX = 1.000000f;
		static inline float TARGET_UP_DIFF_NEGATIVE_MAX = 1.500000f;
		static inline float TARGET_SIDE_DIFF_MAX = 3.000000f;
		static inline float POSITION_SENSITIVE = 100.000000f;
		static inline float POSITION_UP_POSITIVE_SENSITIVE = 100.000000f;
		static inline float POSITION_UP_NEGATIVE_SENSITIVE = 100.000000f;
		static inline float POSITION_UP_DIFF_POSITIVE_MAX = 2.000000f;
		static inline float POSITION_UP_DIFF_NEGATIVE_MAX = 30.000000f;
		static inline float DISTANCE_OFFSET_BASE_VELOCITY = 60.000000f;
		static inline float DISTANCE_OFFSET = 0.400000f;
		static inline float DISTANCE_OFFSET_MAX = 10.000000f;
		static inline float DISTANCE_OFFSET1_SENSITIVE = 3.000000f;
		static inline float DISTANCE_OFFSET2_SENSITIVE = 2.500000f;
		static inline float DRIFT_YAW_SENSITIVE_BASE = 250.000000f;
		static inline float DRIFT_YAW_SENSITIVE_SPEED_SCALE = 3.000000f;
		static inline float DRIFT_FINISH_YAW_VELOCITY_SENSITIVE = 90.000000f;
		static inline float DRIFT_TARGET_SIDE_OFFSET_SCALE = 3.000000f;
		static inline float COLLISION_RADIUS = 0.100000f;
		static inline float HOMING_ATTACK_TARGET_SENSITIVE = 1.500000f;
		static inline float CAMERA_POSITION_VELOCITY_RATE_IN_DASHMODE = 0.950000f;
		static inline float DASHPATH_BINRM_SENSITIVE = 5.000000f;
		static inline float DASHPATH_EASE_TIME = 0.500000f;
		static inline float TARGET_FRONT_OFFSET_BIAS = 0.000000f;
		static inline float TARGET_DOWN_SENSITIVE_GROUND_FAR = 0.000000f;
		static inline float DRIFT_YAW_OFFSET = 0.000000f;
	};

	// Extension of the default camera for our purposes.
	class CPlayer3DNormalCamera_Extended : public Sonic::CPlayer3DNormalCamera
	{
	public:
		hh::math::CVector m_OldUpVector = hh::math::CVector::Zero();
		bool m_IsKnucklesCamera = false;

		float m_XRotation = 0.0f;

		CPlayer3DNormalCamera_Extended()
		{
			fpCtor(this);
		}

	private:
		void NOINLINE fpCtor(CPlayer3DNormalCamera_Extended* This) volatile
		{
			static constexpr int func = 0x010EC1D0;
			__asm
			{
				mov eax, This
				call func
			}
		}

	};

	hh::math::CVector* GetVectorFromMatrixRow(void* This, hh::math::CVector* Vec, int row)
	{
		using namespace hh::math;
		FUNCTION_PTR(CVector*, __thiscall, func, 0x006F1530, void* _This, Hedgehog::Math::CVector * a2, int row);
		return func(This, Vec, row);
	}

	hh::math::CQuaternion* QuaternionAngleAxis(void* result, void* normal, float angle)
	{
		using namespace hh::math;
		FUNCTION_PTR(CQuaternion*, __thiscall, func, 0x006F1800, void* result, void* normal, float angle);
		return func(result, normal, angle);
	}

	hh::math::CVector* TransformCoordinate(void* This, void* result, void* value)
	{
		FUNCTION_PTR(hh::math::CVector*, __thiscall, func, 0x009BE6B0, void* This_, void* Result_, void* Value_);
		return func(This, result, value);
	}

	int UNKNOWN_COLLISION_FUNC(int a1, float a2)
	{
		static uint32_t func = 0x010E3CF0;
		int result = 0;

		__asm
		{
			push a2
			mov eax, a1
			call func
			mov result, eax
		}

		return result;
	}

	enum class AxisType
	{
		XZ,
		XY,
		ZY,
	};

	float GetPlanarMagnitude(const hh::math::CVector& vec, AxisType index = AxisType::XZ)
	{
		float sqrMag;
		switch (index)
		{
		default:
		case AxisType::XZ:
			sqrMag = vec.x() * vec.x() + vec.z() * vec.z();
			break;
		case AxisType::XY:
			sqrMag = vec.x() * vec.x() + vec.y() * vec.y();
			break;
		case AxisType::ZY:
			sqrMag = vec.z() * vec.z() + vec.y() * vec.y();
			break;
		}
		return std::sqrt(sqrMag);
	}
}

namespace CustomCameras
{
	using namespace hh::math;
	using namespace DefaultCameraDependencies;


	constexpr float cameraTargetAngle = 10.0f;
	// Manual hook typedef setup:
	typedef void __fastcall _NormalCameraUpdate(Sonic::CPlayer3DNormalCamera* This);
	_NormalCameraUpdate* original_NormalCameraUpdate = (_NormalCameraUpdate*)(0x010EC7E0);

	// Custom camera implementation that's very WIP.
	// It's not used, so the only reason it's here is archival.
	void CameraUpdate_Simple(CPlayer3DNormalCamera_Extended* This)
	{
		using namespace Sonic::Message;
		using namespace hh::math;

		// Boilerplate
#pragma region Boilerplate
		void* sonicCameraParams = This->m_IsOnBoard ? This->characterProxy : *(void**)((uint32_t)This + 0xE0);
#define GET_CAMERA_PARAM(i) *((float*)sonicCameraParams + i)

		const float param_Fovy = GET_CAMERA_PARAM(0); // Usually 45.0f
		const float param_Distance = GET_CAMERA_PARAM(1); // Usually 3.0f
		const float param_VerticalOffset = GET_CAMERA_PARAM(2); // Usually 1.7f (for Modern Sonic?)
		const float param_TargetPitch = GET_CAMERA_PARAM(3); // Usually 0.0f in official stages, sometimes 15.0f?
		const float param_TargetYaw = GET_CAMERA_PARAM(4); // Usually 0.0f
#undef GET_CAMERA_PARAM()

		Sonic::CCamera* const camera = This->GetContext();
		const float deltaTime = This->GetDeltaTime();

		const int targetActorID = GetValue<int>(camera, 0x42C);
		auto* const player = (Sonic::Player::CPlayerSpeed*)camera->m_pMessageManager->GetMessageActor(targetActorID);
		if (!player)
			return;
		auto* const sonicContext = (player)->GetContext();
		if (!sonicContext)
			return;

		//CVector cameraTargetPosition = CVector::Zero();
		//if (!camera->SendMessageImm(targetActorID, boost::make_shared<MsgGetCameraTargetPosition>(cameraTargetPosition)))
		//	return;
		CVector cameraTargetPosition = sonicContext->m_spMatrixNode->m_Transform.m_Position;
		cameraTargetPosition += sonicContext->GetUpDirection() * 0.5f;

		CVector cameraTargetVelocity = CVector::Zero();
		if (!camera->SendMessageImm(targetActorID, boost::make_shared<MsgGetVelocity>(cameraTargetVelocity)))
			return;
#pragma endregion

		//return;

		const CVector up = CVector::Up();
		This->m_WorkingUpVector = up;

		//const float param_VerticalOffset = 1.25f;
		//cameraTargetPosition += This->m_WorkingUpVector * (param_VerticalOffset - 1.700000047683716);

		cameraTargetPosition += This->m_WorkingUpVector * ((param_VerticalOffset - 1.700000047683716) + 0.5f);
		//cameraTargetPosition += This->m_WorkingUpVector * 0.5f;

		//////////
		// Code //
		//////////

		// Delay the Y position deliberately

		const float delayRate = 15.0f;
		const float delayFactor = delayRate * deltaTime;

		// Avoid camera delay if running on walls. Trust me, it feels gross.
		const float upVectorPlanarFactor   = 1.0f - fabs(CVector::Dot(sonicContext->m_FloorAndGrindRailNormal, CVector::Up())); // 1.0f if character Up is parallel to the Y plane.
		const float rightDirectionUpFactor = fabs(CVector::Dot(sonicContext->GetRightDirection(), CVector::Up())); // Again we only care about wall running.
		const float runningOnWallAmount = upVectorPlanarFactor * rightDirectionUpFactor;

		DebugDrawText::log(format("Allow Delay Amount: %f", 1.0f - runningOnWallAmount));

		// Delay code
		const float cameraYDelayed = Config::ms_CustomCameraDelay
		                           ? std::lerp(This->m_CameraTargetPosition.y(), cameraTargetPosition.y(), std::lerp(delayFactor, 1.0f, runningOnWallAmount))
		                           : cameraTargetPosition.y();


		constexpr float YMaxExtent =  1.5f;
		constexpr float YMinExtent = -1.5f;

		const float cameraYDifference = cameraTargetPosition.y() - cameraYDelayed;
		const float cameraYDifferenceLimited = fmin(YMaxExtent, fmax(YMinExtent, cameraYDifference));
		//DebugDrawText::log(format("Delay factor: %f", cameraYDifference));
		cameraTargetPosition.y() -= cameraYDifferenceLimited;

		// TODO: make parameters
		constexpr float rotationRate = 10.0f;
		constexpr float panRate = 7.5f;
		constexpr float panMin = 0.0f;
		constexpr float panMax = 6.0f;


		// HACK: get camera position directly from the MyCamera struct.
		const CVector cameraStartPosition = camera->m_MyCamera.m_Position; // or try This->m_CameraPositionVisual ?

		// Rotation & camera up/down
		auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();
		float camRotationYaw;
		float camRotationPitch;

		// Classic sonic supports L and R triggers.
		const float triggerSign = Config::ms_InvertTriggers ? 1.0f : -1.0f;
		const float triggerAmnt = (-input->LeftTrigger + input->RightTrigger) * triggerSign;

		const float stickSign = Config::ms_InvertRightStick ? -1.0f : 1.0f;
		const float rightStickHorizontal = input->RightStickHorizontal * stickSign;

		bool cameraInputUseEasing = true;
		if (cameraInputUseEasing)
		{
			const float easeFactor = 0.07000000029802322f * 3.0f; // Value in Gens' code is fairly slow, so this is the same value 3x faster.

			This->m_CameraOrbitX = std::lerp(This->m_CameraOrbitX, rightStickHorizontal - triggerAmnt, easeFactor);
			This->m_CameraOrbitY = std::lerp(This->m_CameraOrbitY, input->RightStickVertical,          easeFactor);

			camRotationPitch = -This->m_CameraOrbitY * 1.5f * deltaTime;
			camRotationYaw   =  This->m_CameraOrbitX * 2.0f * deltaTime;
		}
		else
		{
			// Unleashed version:
			camRotationPitch = -input->RightStickVertical * 1.5f * deltaTime;
			camRotationYaw   = (input->RightStickHorizontal - triggerAmnt) * 2.0f * deltaTime;
		}

		// Better to operate on origin space.
		const CVector cameraVector = cameraStartPosition - This->m_CameraTargetPosition;

		// Horizontal is pretty easy...
		const CVector horizontalVector = CVector::ProjectOnPlane(cameraVector, up);
		      CVector horizontalDirection = horizontalVector.normalizedSafe();
		//const float horizontalDistance = horizontalVector.norm();

		// Vertical is a little trickier.
		This->m_CameraTargetPositionY = fmin(panMax, fmax(panMin, This->m_CameraTargetPositionY + (camRotationPitch * panRate))); // Repurposing CameraTargetPositionY

		const float verticalHeightCurrent = CVector::Dot(cameraVector, up);
		const float verticalHeightTarget  = This->m_CameraTargetPositionY;

		// FIXME: This will ease our camera Y, ALL THE TIME, which is unideal. We don't want to ease camera pitch correction, just the transition from collision.
		//const float verticalHeight = std::lerp(verticalHeightCurrent, verticalHeightTarget, deltaTime * 5.0f);
		const float verticalHeight = verticalHeightTarget;
		//DebugDrawText::log(format("VertHeight: %f", verticalHeight));
		//const float horizontalDistance = 6.0f;
		const float horizontalDistance = param_Distance;

		// rotate-with-player nonsense
		if (This->m_IsKnucklesCamera)
		{
			const CVector cameraPlayerTurnDirection = CVector::ProjectOnPlane(cameraStartPosition - cameraTargetPosition, up).normalizedSafe();
			horizontalDirection = CVector::Slerp(horizontalDirection, cameraPlayerTurnDirection, 0.5f);
		}

		const CVector cameraRotatedDirection = CQuaternion::FromAngleAxis(-camRotationYaw, CVector::Up()) * horizontalDirection;


		//const float distanceFactor = -fmin(0.0f, cameraYDifference * 2.5f) / 3.0f;
		//CVector cameraPosition = cameraVector + cameraTargetPosition;
		CVector cameraPosition = cameraTargetPosition
		                       + (cameraRotatedDirection * (horizontalDistance))
		                       + (up * verticalHeight);
		// EXPERIMENTAL: Have the camera "aim" towards the player a little esp. when falling.
		cameraPosition.y() -= cameraYDifference * 1.5f;


		// Collision
		if (Config::ms_CustomCameraCollision)
		{
			const float CAMERA_DISTANCE_MAX_RATE = *(float*)0x01A48B70;
			FUNCTION_PTR(CVector&, __thiscall, GetBodyPosition, 0x010E0920, void* proxy, CVector* inVector);

			CVector previousCamPosition = This->m_CameraPositionCollision;

			const CVector collisionDifference = cameraTargetPosition - previousCamPosition;
			const float distanceMaxRate = param_Distance * CAMERA_DISTANCE_MAX_RATE;

			if (distanceMaxRate * distanceMaxRate > collisionDifference.LengthSqr())
			{
				CVector cameraNewVelocity = CVector::Divide((cameraPosition - previousCamPosition), deltaTime);

				This->m_pListener->m_Vector01 = cameraTargetPosition;
				This->m_pListener->m_Vector02 = previousCamPosition;
				This->m_pListener->m_Vector03 = cameraNewVelocity;

				This->m_pCharacterProxy->m_Position = previousCamPosition;
				This->m_pCharacterProxy->m_UpVector = This->m_WorkingUpVector;
				This->m_pCharacterProxy->m_Velocity = cameraNewVelocity;

				UNKNOWN_COLLISION_FUNC((int)This->m_pCharacterProxy, deltaTime);//
				CVector outVec = CVector::Zero();
				cameraPosition = GetBodyPosition(This->m_pCharacterProxy, &outVec);
			}
		}

		// Final assignments.

		// CAM POSITION SET
		This->m_CameraPositionCollision = cameraPosition;

		//This->m_FovInRadians = param_Fovy * DEG2RAD * 1.5f; // HACK: Multiply by 1.5 so Classic's default FOV of 30 doesn't feel as horrible.
		//This->m_FovInRadians = param_Fovy * DEG2RAD;

		// SA1 FOV at all times for now
		This->m_FovInRadians = 55.0f * DEG2RAD;
		This->m_Field68 = true;

		camera->m_FieldOfView = This->m_FovInRadians;
		camera->m_FieldOfViewTarget = This->m_FovInRadians;

		This->m_CameraPositionVisual = cameraPosition;
		This->m_CameraTargetPosition = cameraTargetPosition;
		This->m_CameraUpVector = up;

		This->m_CameraPositionInputReference = cameraPosition;
		This->m_CameraTargetPosition2 = cameraTargetPosition;
		This->m_CameraUpVector2 = up;
		
	}

	// The "Knuckles" camera is used in MANY places in SA1. This is interesting, as this camera is the backbone of quite a lot of them.
	// It's also the generic camera behavior in SA1 so we definitely want to nail this if we want the free-roam camera to "feel" right.
	void CameraUpdate_Knuckles(CPlayer3DNormalCamera_Extended* This, float delta = 0.0f, bool useCollision = true)
	{
		using namespace Sonic::Message;
		using namespace hh::math;

		const CVector up = CVector::Up();
		This->m_WorkingUpVector = up;

		// These are currently unused but we may want to do something with them later.
#pragma region Boilerplate
		void* sonicCameraParams = This->m_IsOnBoard ? This->characterProxy : *(void**)((uint32_t)This + 0xE0);
#define GET_CAMERA_PARAM(i) *((float*)sonicCameraParams + i)

		const float param_Fovy = GET_CAMERA_PARAM(0); // Usually 45.0f
		const float param_Distance = GET_CAMERA_PARAM(1); // Usually 3.0f
		const float param_VerticalOffset = GET_CAMERA_PARAM(2); // Usually 1.7f (for Modern Sonic?)
		const float param_TargetPitch = GET_CAMERA_PARAM(3); // Usually 0.0f in official stages, sometimes 15.0f?
		const float param_TargetYaw = GET_CAMERA_PARAM(4); // Usually 0.0f
#undef GET_CAMERA_PARAM()

		Sonic::CCamera* const camera = This->GetContext();
		const float deltaTime = delta < FLT_EPSILON ? This->GetDeltaTime() : delta;
		const float frameDeltaTime = 60.0f * deltaTime;

		const int targetActorID = GetValue<int>(camera, 0x42C);
		auto* const player = (Sonic::Player::CPlayerSpeed*)camera->m_pMessageManager->GetMessageActor(targetActorID);
		if (!player)
			return;
		auto* const sonicContext = (player)->GetContext();
		if (!sonicContext)
			return;

		CVector cameraTargetPosition = sonicContext->m_spMatrixNode->m_Transform.m_Position;
		cameraTargetPosition += sonicContext->GetUpDirection() * 0.5f // Center point
			                  + up * 0.5f; // Eye height
			;

		CVector cameraTargetVelocity = CVector::Zero();
		if (!camera->SendMessageImm(targetActorID, boost::make_shared<MsgGetVelocity>(cameraTargetVelocity)))
			return;
#pragma endregion

		//cameraTargetPosition += This->m_WorkingUpVector * ((param_VerticalOffset - 1.700000047683716) + 0.5f);

		// Need this for a few things.
		auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();

		//////////
		// Code //
		//////////

		// TODO: make parameters
		constexpr float rotationRate = 10.0f;
		constexpr float panRate = 7.5f;
		constexpr float panMin = 0.0f;
		constexpr float panMax = 6.0f;

		// TODO: Fix the fact i'm calculating this vector SEVERAL TIMES.
		const float oldLength = (This->m_CameraPositionCollision - This->m_CameraTargetPosition).norm();

		// Camera rotation, TODO: Up/down panning (vertical angle adjust probably)
		float camRotationYaw;
		float camRotationPitch;

		// Classic sonic supports L and R triggers.
		const float triggerSign = Config::ms_InvertTriggers ? 1.0f : -1.0f;
		const float triggerAmnt = (-input->LeftTrigger + input->RightTrigger) * triggerSign;

		const float stickSign = Config::ms_InvertRightStick ? -1.0f : 1.0f;
		const float rightStickHorizontal = input->RightStickHorizontal * stickSign;

		// TODO: Make configurable
		bool cameraInputUseEasing = true;
		if (cameraInputUseEasing)
		{
			const float easeFactor = 0.07000000029802322f * 3.0f; // Value in Gens' code is fairly slow, so this is the same value 3x faster.

			// UNDONE: Don't do Y rotation lerp, because we already ease into the target angle.
			//This->m_CameraOrbitY = std::lerp(This->m_CameraOrbitY, input->RightStickVertical, easeFactor);
			  This->m_CameraOrbitY = input->RightStickVertical;
			  This->m_CameraOrbitX = std::lerp(This->m_CameraOrbitX, rightStickHorizontal - triggerAmnt, easeFactor);

			camRotationPitch = This->m_CameraOrbitY * 2.0f * deltaTime;
			camRotationYaw   = This->m_CameraOrbitX * 2.0f  * deltaTime;
		}
		else
		{
			// Unleashed version no easing:
			camRotationPitch =  input->RightStickVertical                  * 2.0f * deltaTime;
			camRotationYaw   = (rightStickHorizontal - triggerAmnt) * 2.0f * deltaTime;
		}

		// Interpolate height angle towards 10 degrees.
		{
			// Level out camera height if we click in on right stick.
			This->m_XRotation = input->IsTapped(Sonic::eKeyState_RightStick)
			                  ? cameraTargetAngle
			                  : std::clamp(This->m_XRotation + camRotationPitch * RAD2DEGf, -5.0f, 45.0f);

			const CVector axisZ = CVector::ProjectOnPlane(camera->m_MyCamera.m_Direction, up).normalizedSafe();
			const CVector axisX = CVector::Cross(axisZ, -up);

			const CVector cameraUp = sonicContext->m_Grounded
			                       ? sonicContext->m_ModelUpDirection.ProjectOnPlane(axisX).normalizedSafe()
			                       : up;

			const CVector camForward = CVector::ProjectOnPlane(camera->m_MyCamera.m_Direction, cameraUp).normalizedSafe();
			const CVector cameraSide = CVector::Cross(camForward, -cameraUp);

			const CVector cameraVectorInitial = This->m_CameraPositionCollision - This->m_CameraTargetPosition;
			const float   distance = cameraVectorInitial.norm();
			const CVector cameraDirectionInitial = cameraVectorInitial.normalizedSafe();

			const CVector targetAngleDirection = CQuaternion::FromAngleAxis(This->m_XRotation * DEG2RADf, cameraSide)
			                                   * cameraDirectionInitial.ProjectOnPlane(cameraUp).normalizedSafe();


			const CVector finalDirection = CVector::Slerp(cameraDirectionInitial, targetAngleDirection, 0.05f * frameDeltaTime);
			This->m_CameraPositionCollision = finalDirection * distance + This->m_CameraTargetPosition;
		}


		// Target position nonense that gets smooth position data, allegedly
		const CVector frameMovement = sonicContext->GetVelocity() * deltaTime;
		const CVector frameOffset = cameraTargetPosition + (frameMovement * 8.0f);
		This->m_CameraTargetPosition += (frameOffset - This->m_CameraTargetPosition) * 0.1f;
		const CVector targetPosition = This->m_CameraTargetPosition;

		const CVector cameraDirectionVector = targetPosition - This->m_CameraPositionCollision;
		const CVector cameraDirection = cameraDirectionVector.normalizedSafe();
		const float length = cameraDirectionVector.norm();

		auto GetCameraDistance = [](float initialDistance, float previousDistance, const CVector& targetPosition, const CVector& cameraPosition) -> float
		{
			constexpr float min_distance = 2.0f;
			constexpr float max_distance = 9.0f;
			constexpr float min_ease = 3.5f;
			constexpr float max_ease = 6.0f;

			// Pre-max extent
			if (initialDistance > max_ease)
			{

				//result = (result - 6.0f) * 0.9f + 6.0f;
				float result = std::lerp(max_ease, initialDistance, 0.9f);
				if (result <= max_distance)
				{
					return result;
				}

				CVector v = CVector::Normalized(targetPosition - cameraPosition);
				v.x() *= 1.0f;
				v.y() *= 2.0f;
				v.z() *= 1.0f;

				// original SA1 code, not used due to it likely invoking camera collision pretty early.
				// v22 = cameraPosition;
				//if (!MSetPositionNW(&v22, &v, 0, 4.0) || njScalor(&v) >= 3.0)

				if (v.Length() >= 0.3f)
					return max_distance;
				
				// Seemingly not hit here probably because of the lack of collision checks.
				// Unsure if this matters much outside the context of SA1.
				return min_distance;
			}

			// This is stupid. Abandon SA1 code due to possibility of jitter.
			/*
			if (result < 3.5f)
			{
				result *= 1.06f;
				return fmax(result, 2.0f);
			}
			*/

			// Pre-min extent, use a heuristic to prevent awful jitter
			if(initialDistance < min_ease)
			{
				const float reducedDistance = fmax(0.0f, previousDistance - initialDistance);
				DebugDrawText::log(format("ReducedDistance: %f", reducedDistance));
				if (reducedDistance >= 0.0f)
					return initialDistance;

				const float result = previousDistance - std::lerp(0.0f, reducedDistance, std::inverseLerp(min_distance, min_ease, initialDistance));
				return fmax(result, min_distance);
			}

			return initialDistance;
		};

		float cameraDistance = GetCameraDistance(length, oldLength, targetPosition, This->m_CameraPositionCollision);

		// Vertical stuff, here but not used.
		//This->m_CameraTargetPositionY = fmin(panMax, fmax(panMin, This->m_CameraTargetPositionY + (camRotationPitch * panRate))); // Repurposing CameraTargetPositionY

		CVector cameraPosition = targetPosition
		                       - (CQuaternion::FromAngleAxis(-camRotationYaw, up) * cameraDirection)
		                       * cameraDistance;

		// Calculate minimum distance (Probably was done cuz the near-cutoff math in the above function kinda doesn't work, haha)

		// I'm not a fan of this not having any effect when the camera is below the target... Let's just get rid of that honestly.
		//if (cameraPosition.y() > targetPosition.y())
		{
			CVector v(cameraPosition.x() - targetPosition.x(), 0, cameraPosition.z() - targetPosition.z());
			if (v.z() * v.z() + v.x() * v.x() < 9.0f) // 3 * 3 = 9
			{
				v = v.normalizedSafe();
				cameraPosition.x() = v.x() * 3.0f + targetPosition.x();
				cameraPosition.z() = v.z() * 3.0f + targetPosition.z();
			}
		}

		// Collision
		if (Config::ms_CustomCameraCollision && useCollision)
		{
			const float CAMERA_DISTANCE_MAX_RATE = *(float*)0x01A48B70;
			FUNCTION_PTR(CVector&, __thiscall, GetBodyPosition, 0x010E0920, void* proxy, CVector* inVector);

			CVector previousCamPosition = This->m_CameraPositionCollision;

			const CVector collisionDifference = This->m_CameraTargetPosition - previousCamPosition;
			const float distanceMaxRate = cameraDistance * CAMERA_DISTANCE_MAX_RATE;

			if (distanceMaxRate * distanceMaxRate > collisionDifference.LengthSqr())
			{
				CVector cameraNewVelocity = CVector::Divide((cameraPosition - previousCamPosition), deltaTime);

				This->m_pListener->m_Vector01 = cameraTargetPosition;
				This->m_pListener->m_Vector02 = previousCamPosition;
				This->m_pListener->m_Vector03 = cameraNewVelocity;

				This->m_pCharacterProxy->m_Position = previousCamPosition;
				This->m_pCharacterProxy->m_UpVector = This->m_WorkingUpVector;
				This->m_pCharacterProxy->m_Velocity = cameraNewVelocity;

				UNKNOWN_COLLISION_FUNC((int)This->m_pCharacterProxy, deltaTime);//
				CVector outVec = CVector::Zero();
				cameraPosition = GetBodyPosition(This->m_pCharacterProxy, &outVec);
			}
		}

		// Final assignments.

		// CAM POSITION SET
		This->m_CameraPositionCollision = cameraPosition;

		// SA1 FOV at all times for now
		This->m_FovInRadians = 55.0f * DEG2RAD;
		This->m_Field68 = true;

		camera->m_FieldOfView = This->m_FovInRadians;
		camera->m_FieldOfViewTarget = This->m_FovInRadians;

		This->m_CameraPositionVisual = cameraPosition;
		//This->m_CameraTargetPosition = cameraTargetPosition;
		This->m_CameraUpVector = up;

		This->m_CameraPositionInputReference = cameraPosition;
		This->m_CameraTargetPosition2 = This->m_CameraTargetPosition;
		This->m_CameraUpVector2 = up;

		// UNDONE: SA1 specific stuff
		//CVector camDiff = cameraPosition - targetPosition;
		//This->m_XRotation = atan2(-camDiff.y(), sqrt(camDiff.z() * camDiff.z() + camDiff.x() * camDiff.x())) * RAD2DEGf;
	}

	// This is a patch to the default camera, for whenever I plan on going back to supporting it,
	// which fixes some nasty jittering when shifting from ground to air.
#pragma region NormalCameraPatch
	void __cdecl CameraPatchA(CPlayer3DNormalCamera_Extended* This, Hedgehog::Math::CVector* vec)
	{
		constexpr float cameraAirUpDirectionCorrectionRate = 5.0f;

		*vec = CVector::Lerp(This->m_OldUpVector, CVector::Up(), cameraAirUpDirectionCorrectionRate * This->GetDeltaTime());
		This->m_OldUpVector = *vec;
	}
	void __cdecl CameraPatchB(CPlayer3DNormalCamera_Extended* This, Hedgehog::Math::CVector* vec)
	{
		// TODO: Convert this to pure assembly to save a function call, unless we have to do more here (unlikely)
		This->m_OldUpVector = *vec;
	}
	ASMHOOK CameraUpVectorPatchA_ASM()
	{
		static constexpr uint32_t jumpOut = 0x010ECFAC;
		__asm
		{
			push eax
			push ecx
			push edx

			sub esp, 0x14
			movss[esp + 0x00], xmm0
			movss[esp + 0x04], xmm1
			movss[esp + 0x08], xmm2
			movss[esp + 0x0C], xmm3
			movss[esp + 0x10], xmm4

			// Stack is now 0x2E0 - 0x0C - 0x14, stack variable - 0x130
			lea edx, [esp + 0x1D0]
			// Arg2
			push edx
			// Arg1, camera
			push ebx

			call CameraPatchA

			pop ebx
			pop edx

			movss xmm0, [esp + 0x00]
			movss xmm1, [esp + 0x04]
			movss xmm2, [esp + 0x08]
			movss xmm3, [esp + 0x0C]
			movss xmm4, [esp + 0x10]
			add esp, 0x14

			pop edx
			pop ecx
			pop eax

			jmp [jumpOut]
		}
	}
	ASMHOOK CameraUpVectorPatchB_ASM()
	{
		static constexpr uint32_t jumpOut = 0x010ED00F;
		__asm
		{
			// Preamble
			push eax
			push ecx
			push edx

			// Preserve float data in the stack
			sub esp, 0x14
			movss [esp + 0x00], xmm0
			movss [esp + 0x04], xmm1
			movss [esp + 0x08], xmm2
			movss [esp + 0x0C], xmm3
			movss [esp + 0x10], xmm4

			// Stack is now 0x2E0 - 0x0C - 0x14, stack variable - 0x130
			lea edx, [esp+0x1D0]
			// Arg2
			push edx
			// Arg1, camera
			push ebx

			call CameraPatchB

			pop ebx
			pop edx

			// Restore float data & clear extra stack
			movss xmm0, [esp + 0x00]
			movss xmm1, [esp + 0x04]
			movss xmm2, [esp + 0x08]
			movss xmm3, [esp + 0x0C]
			movss xmm4, [esp + 0x10]
			add esp, 0x14

			pop edx
			pop ecx
			pop eax

			// Cut out code
			cvtps2pd xmm1, xmm1
			cvtps2pd xmm0, xmm4

			// return
			jmp[jumpOut]
		}
	}
#pragma endregion

	void __fastcall implOf_NormalCameraUpdate(Sonic::CPlayer3DNormalCamera* This)
	{

		// Otherwise do this stuff to change which default camera we end up using based on if we're Adv. Movement or not.
		auto context = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!context)
		{
			original_NormalCameraUpdate(This);
			return;
		}

		// TODO: only do this when "Adventure Mode," currently just checking if Classic Sonic.
		if (*(int*)context == 0x016D86FC)
		{
			switch (Config::ms_CameraType)
			{
				case DefaultCameraType::CAMERA_GENS:
				default:
				{
					// HACK: force triggers to influence right stick
					auto* inputState = Sonic::CInputState::GetInstance().get().get();
					auto* input = &inputState->m_PadStates[inputState->m_CurrentPadStateIndex];
					const float triggerAmnt = -input->LeftTrigger + input->RightTrigger;

					// This is where the "global" right stick is.
					float* rightStickX = reinterpret_cast<float*>(0x01E77B74);
					*rightStickX -= triggerAmnt;

					original_NormalCameraUpdate(This);
					break;
				}
				case DefaultCameraType::CAMERA_SA1:
				{
					auto Cam = static_cast<CPlayer3DNormalCamera_Extended*>(This);
					//if (Cam->m_IsKnucklesCamera)
					if (Config::ms_IsSA1LevelPreset)
						CameraUpdate_Knuckles(Cam);
					else
						CameraUpdate_Simple(Cam);
					break;
				}
			}
			
		}
		else
		{
			original_NormalCameraUpdate(This);
		}
	}

	void PrepareNormalCamera(CPlayer3DNormalCamera_Extended* This, bool initializeHeight = false)
	{
		auto camera = This->GetContext();
		const int targetActorID = GetValue<int>(camera, 0x42C);
		auto* const actor = camera->m_pMessageManager->GetMessageActor(targetActorID);

		if (!actor)
			return;
		auto sonicActor = (Sonic::Player::CPlayerSpeed*)actor;
		if (!actor)
			return;

		auto sonicContext = sonicActor->GetContext();

		This->m_CameraTargetPosition = sonicContext->m_spMatrixNode->m_Transform.m_Position + sonicContext->GetUpDirection() * 1.5f;
		This->m_CameraTargetPosition2 = This->m_CameraTargetPosition;

		if (initializeHeight)
		{
			This->m_CameraTargetPositionY = 1.5f;
		}
		else
		{
			This->m_CameraTargetPositionY = fmax(0.0f, CVector::Dot(camera->m_Position - This->m_CameraTargetPosition, CVector::Up()));
		}

		This->m_WorkingUpVector = CVector::Up();
		This->m_CameraUpVector = CVector::Up();
		This->m_CameraUpVector2 = CVector::Up();

		const CVector camForward = camera->m_MyCamera.m_Direction.ProjectOnPlane(CVector::Up()).normalizedSafe();

		//This->m_CameraPositionCollision = This->m_CameraTargetPosition + (sonicContext->GetFrontDirection() * -3.5f) + (This->m_CameraUpVector * 0.5f);
		This->m_CameraPositionCollision = This->m_CameraTargetPosition + (camForward * -3.5f) + (This->m_CameraUpVector * 0.5f);
		//This->m_CameraPositionCollision = camera->m_Position;
		This->m_CameraPositionVisual = This->m_CameraPositionCollision;
		This->m_CameraPositionInputReference = This->m_CameraPositionCollision;

		// Default X rotation for Sonic & Knuckles cameras.
		This->m_XRotation = cameraTargetAngle;
	}

	// Executes every time the default camera starts.
	HOOK(void, __fastcall, _NormalCameraStart,      0x010EF7C0, CPlayer3DNormalCamera_Extended* This)
	{
		// Do some initialization stuff for the main camera.
		This->m_OldUpVector = CVector::Up();
		original_NormalCameraStart(This);

		// Patch camera data if "simple"
		// TODO: Switch statement as needed.
		if (Config::ms_CameraType == DefaultCameraType::CAMERA_GENS)
			return;

		PrepareNormalCamera(This);

		void* sonicCameraParams = This->m_IsOnBoard ? This->characterProxy : *(void**)((uint32_t)This + 0xE0);
		This->m_CameraTargetPositionY = GetValue<float>(sonicCameraParams, 0x04 * 2);

		//const float param_Fovy           = GetValue<float>(sonicCameraParams, 0x04 * 0); // Usually 45.0f
		//const float param_Distance       = GetValue<float>(sonicCameraParams, 0x04 * 1); // Usually 3.0f
		//const float param_VerticalOffset = GetValue<float>(sonicCameraParams, 0x04 * 2); // Usually 1.7f (for Modern Sonic?)
		//const float param_TargetPitch    = GetValue<float>(sonicCameraParams, 0x04 * 3); // Usually 0.0f in official stages, sometimes 15.0f?
		//const float param_TargetYaw      = GetValue<float>(sonicCameraParams, 0x04 * 4); // Usually 0.0f
	}

	// Executes on level load.
	HOOK(void, __fastcall, _NormalCameraParamStart, 0x010EC540, CPlayer3DNormalCamera_Extended* This, void*, CVector* A1, CVector* A2)
	{
		original_NormalCameraParamStart(This, nullptr, A1, A2);

		// Patch camera data if "simple"
		// TODO: Switch statement as needed.
		if (Config::ms_CameraType == DefaultCameraType::CAMERA_GENS)
			return;

		PrepareNormalCamera(This, true);

		// We're gonna want to configure what our default is w/ our enum at some point... For now force KNUCKLES camera.
		This->m_IsKnucklesCamera = true;


		void* sonicCameraParams = This->m_IsOnBoard ? This->characterProxy : *(void**)((uint32_t)This + 0xE0);

		float& param_Fovy           = *GetPointer<float>(sonicCameraParams, 0x04 * 0); // Usually 45.0f
		float& param_Distance       = *GetPointer<float>(sonicCameraParams, 0x04 * 1); // Usually 3.0f
		float& param_VerticalOffset = *GetPointer<float>(sonicCameraParams, 0x04 * 2); // Usually 1.7f (for Modern Sonic?)
		float& param_TargetPitch    = *GetPointer<float>(sonicCameraParams, 0x04 * 3); // Usually 0.0f in official stages, sometimes 15.0f?
		float& param_TargetYaw      = *GetPointer<float>(sonicCameraParams, 0x04 * 4); // Usually 0.0f

		param_Fovy = 55.0f;
		param_Distance = 6.0f;
		//param_VerticalOffset = 0.0f;
		param_TargetPitch = 0.0f;
		param_TargetYaw   = 0.0f;
	}

	HOOK(uint32_t*, __stdcall, _GetCameraController, 0x101A320, Sonic::CGameObject3D* This, uint32_t* a2)
	{
		uint32_t* result = original_GetCameraController(This, a2);

		CPlayer3DNormalCamera_Extended* normalCamera = *(CPlayer3DNormalCamera_Extended**)a2;
		normalCamera->m_IsKnucklesCamera = GetValue<bool>(This->m_pMember, 0x58);

		return result;
	}

	void Init()
	{
		WRITE_MEMORY(0x0040338A, uint32_t, sizeof(CPlayer3DNormalCamera_Extended))
		WRITE_MEMORY(0x011ACEFA, uint32_t, sizeof(CPlayer3DNormalCamera_Extended))
		WRITE_MEMORY(0x005AC37B, uint32_t, sizeof(CPlayer3DNormalCamera_Extended))
		WRITE_MEMORY(0x010EF8D4, uint32_t, sizeof(CPlayer3DNormalCamera_Extended))

		INSTALL_HOOK(_GetCameraController)
		//INSTALL_HOOK(_ChangeNormalCameraEventFired)
		//INSTALL_HOOK(_ChangeNormalCameraEventFired2)

		INSTALL_HOOK(_NormalCameraStart)
		INSTALL_HOOK(_NormalCameraParamStart)
		INSTALL_HOOK(_NormalCameraUpdate)

		//INSTALL_HOOK(_CameraEnd)

		// Camera patches for the up vector
		WRITE_JUMP(0x010ECF91, CameraUpVectorPatchA_ASM)
		WRITE_JUMP(0x010ED009, CameraUpVectorPatchB_ASM)
	}

}

void DecompInit()
{
	using namespace Cameras;
	// HACK: replace condition with raw jump
	// TODO: Remove
	//WRITE_MEMORY(0x010EEF64, uint8_t, 0xE9, 0x01, 0x03, 0x00, 0x00, 0x90)
	WRITE_MEMORY(0x010EEF64, uint8_t, 0xE9, 0x01, 0x03, 0x00)

	//INSTALL_HOOK(NormalCameraCalculate)
}


// Set camera stuff
namespace SetCameras
{
	namespace Types
	{
		class CCameraPositionTube_Eye;

		class __declspec(align(4)) ISetCamera
		{
		public:
			virtual ~ISetCamera() {}
			// Make more later I guess, if we need or want to?
		};
		class CMultipurposeCamera : public ISetCamera
		{
		public:
			__declspec(align(4)) Hedgehog::Base::CSharedString m_Name;
			float m_DeltaTime{};
			int m_Unk00{};
			int m_Unk01{};
			int m_Unk02{};
			float m_Unk03{};
			float m_Unk04{};
			boost::shared_ptr<void> m_spCameraPositionController;
			boost::shared_ptr<void> m_spTargetPositionController;
			boost::shared_ptr<void> m_spUpVectorController;
			float m_Unk11{};
			int m_Unk12{};
			Hedgehog::Math::CVector m_Unk13;
			Hedgehog::Math::CVector m_TargetPosition;
			Hedgehog::Math::CVector m_Unk21;
			boost::shared_ptr<void> m_spRayCastCollision;
			int m_Unk27{};
			int m_Unk28{};

			Sonic::PathAnimationController* GetPathAnimationController();
		};

		ASSERT_OFFSETOF(CMultipurposeCamera, m_Unk28, 0x7C);
		ASSERT_SIZEOF(CMultipurposeCamera, 0x80);

		//class CMultipurposeCameraExtended : public CMultipurposeCamera
		//{
		//public:
		//	CPathController* m_pPathController {};
		//};

		class CObjCamera : public Sonic::CGameObject3D
		{
		public:
			int m_FieldF4 = 0;
			hh::map<void*, void*> m_CameraMap;
			int m_Field104 = 0;
			int m_Field108 = 0;
			int m_Field10C = 0;
			int m_Field110 = 0;
			char m_Field114 = 0;
			char m_Field115 = 0;
			char m_Field116 = 0;
			char m_Field117 = 0;
			int m_Field118 = 0;
			int m_Field11C = 0;
			char m_Field120 = 0;
			char m_Field121 = 0;
			char m_Field122 = 0;
			char m_Field123 = 0;
			int m_Field124 = 0;
		};
		//ASSERT_SIZEOF(CObjCamera, 0x128);

		class __declspec(align(4)) CObjCameraTube : public CObjCamera
		{
		public:
			int m_Unk00;
			int m_Unk01;
			boost::shared_ptr<void> m_spCameraPositionTubeEye;
			boost::shared_ptr<void> m_spCameraPositionTubeTrg;
			int m_Unk06;
			int m_Unk07;
			char m_Cond01;
			int m_Unk09;
			boost::shared_ptr<Sonic::CPathController> m_spPathController;
			int m_Unk12;
			int m_Unk13;
		};

		class CameraInterpolator // hh::fnd::CStateMachineBase::CInterpolator
		{
		public:
			float m_OnePointZero = 0.0f;
			float m_EaseTime = 0.0f;
			float m_Time = 0.0f;
			int m_BlendAmountMaybe = 0;
			INSERT_PADDING(0x04); // junk
			CameraInterpolator* m_spPreviousInterpolator = nullptr;
			INSERT_PADDING(0x04); // boost
			void* m_spCameraController = nullptr;
			INSERT_PADDING(0x04); // boost
			void* m_spInterpolationStrategy = nullptr;
			INSERT_PADDING(0x08); // boost, then junk
		};

		class CCameraPositionTube_Eye
		{
		public:
			void* __vftable;
			boost::shared_ptr<Sonic::CPathController> m_PathController;
			Hedgehog::Math::CVector m_Vec01;
			Hedgehog::Math::CVector m_PositionMaybe;
			Hedgehog::Math::CVector m_UpVector;
			Hedgehog::Math::CVector m_ForwardVector;
			float m_DeltaTime;
		};

		// External function definitions
		Sonic::PathAnimationController* CMultipurposeCamera::GetPathAnimationController()
		{
			const auto* spPositionController = reinterpret_cast<boost::shared_ptr<CCameraPositionTube_Eye>*>(&m_spCameraPositionController);
			return spPositionController->get()->m_PathController->m_spPathAnimationController.get();
		}

	}
	using namespace Types;

	static float s_PathFloat = 0;
	static bool s_DebugTubeCams = false;
	static float s_EyeRatio = 0;
	static float s_EyeOffsetY = 1.2;
	static float s_EyeOffsetZ = 4.5;
	static float s_TrgRatio = 0;
	static float s_TrgOffsetY = 1;
	static float s_TrgOffsetZ = 0;

	static double s_TubeCameraSeekDistance = 0.1;

	// HACK: Fun note about tube cameras.
	// If you make the tube camera target Sonic's closest point to the spline, a height offset of "1" aligns with his head. Makes sense.
	// Apparently, if it targets Sonic instead, to get the same relative height you need an offset of 0.8.
	// This means any height offset calculations need to pre-calculate the height offset by *= lerp(1.0f, 0.8f, s_TrgRatio).
	// Curiously, this is Classic's scale multiplier in the white world. Most likely a coincidence, but still...

	HOOK(void, __cdecl, InitializeApplicationParams_PathDebug, 0x00D65180, Sonic::CParameterFile* This)
	{
		boost::shared_ptr<Sonic::CParameterGroup> parameterGroup = This->CreateParameterGroup("Adventure", "Adventure");

		// Depereciated
		// Sonic::CEditParam* pc_Path = parameterGroup->CreateParameterCategory("Path debugging", "Test path stuff");
		//pc_Path->CreateParamFloat(&s_PathFloat, "Float to control something relating to paths.");

		Sonic::CEditParam* pc_TubeEye = parameterGroup->CreateParameterCategory("Tube Camera Eye", "Test overrides for Tube Camera EYE");
		pc_TubeEye->CreateParamFloat(&s_EyeRatio, "Ratio ( 0 = Path || 1 = Player)");
		pc_TubeEye->CreateParamFloat(&s_EyeOffsetY, "Offset Y");
		pc_TubeEye->CreateParamFloat(&s_EyeOffsetZ, "Offset Z");
		pc_TubeEye->CreateParamBool(&s_DebugTubeCams, "EnableDebug");
		parameterGroup->Flush();

		Sonic::CEditParam* pc_TubeTrg = parameterGroup->CreateParameterCategory("Tube Camera Target", "Test overrides for Tube Camera TARGET");
		pc_TubeTrg->CreateParamFloat(&s_TrgRatio, "Ratio ( 0 = Path || 1 = Player)");
		pc_TubeTrg->CreateParamFloat(&s_TrgOffsetY, "Offset Y");
		pc_TubeTrg->CreateParamFloat(&s_TrgOffsetZ, "Offset Z");
		pc_TubeTrg->CreateParamBool(&s_DebugTubeCams, "EnableDebug");
		parameterGroup->Flush();

		originalInitializeApplicationParams_PathDebug(This);
	}

	// UNDONE: DEBUG STUFF
	HOOK(void, __fastcall, _SetCameraControllerUpdateDBG, 0x010E7420, hh::fnd::CStateMachineBase::CStateBase* This)
	{
		original_SetCameraControllerUpdateDBG(This);

		auto* const camera = GetValue(This, 0x08);
		auto* const multipurposeCamera = GetValue(This, 0xE0);
		void* const cameraStateMachine = GetValue(This, 0x0C);

		const char* name = multipurposeCamera ? GetPointer<hh::base::CSharedString>(multipurposeCamera, 0x04)->c_str()
		                                      : "UNKNOWN";

		const float transitionTime = cameraStateMachine ? GetValue<float>(cameraStateMachine, 0x54) * GetValue<float>(This, 0x20)
		                                                : GetValue<float>(This, 0x20);

		const     int id = *(int*)name;
		constexpr int nameID = 0x65627554; // Tube
		const bool isTube = id == nameID;


		if (!isTube) return;

		using namespace CustomCameras;
		using namespace hh::math;

		// Let's debug the camera path.
		auto mpCamera = (CMultipurposeCamera*)multipurposeCamera;

		const auto* spPositionController = reinterpret_cast<boost::shared_ptr<CCameraPositionTube_Eye>*>(&mpCamera->m_spCameraPositionController);
		const auto* pathAnimController = spPositionController->get()->m_PathController->m_spPathAnimationController.get();
		auto* const kfPath = pathAnimController->m_pAnimationEntity->m_spKeyframedPath.get();

		// TODO: Get sonic's velocity in a more intuitive way somehow!!!
		auto context = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (context)
		{
			const float ZSpeed = context->GetHorizontalVelocity().norm();
			const float frameDistance = ZSpeed * This->GetDeltaTime();

			CVector PositionA(0, 0, 0);
			CVector PositionB(0, 0, 0);

			CVector _(0, 0, 0);

			kfPath->SamplePathGranular(pathAnimController->m_DistanceAlongPath,                 &PositionA, &_, &_);
			kfPath->SamplePathGranular(pathAnimController->m_DistanceAlongPath + frameDistance, &PositionB, &_, &_);

			const CVector moveDirection = ZSpeed > FLT_EPSILON
			                            ? (PositionB - PositionA).normalized()
			                            : context->GetFrontDirection();

			std::stringstream stream;
			stream << moveDirection;
			DebugDrawText::log(stream.str().c_str());
		}
	}
	HOOK(void, __fastcall, _SetCameraControllerEnd,    0x010E70E0, hh::fnd::CStateMachineBase::CStateBase* This)
	{
		auto* const multipurposeCamera = GetValue(This, 0xE0);
		const char* name = multipurposeCamera ? GetPointer<hh::base::CSharedString>(multipurposeCamera, 0x04)->c_str()
		                                      : "UNKNOWN";
		const     int id = *(int*)name;
		constexpr int nameID = 0x65627554; // Tube
		const bool isTube = id == nameID;

		if (!isTube)
		{
			original_SetCameraControllerEnd(This);
			return;
		}
		original_SetCameraControllerEnd(This);
	}

	// Handle Tube cameras because they're a bit of a special case.
#ifdef _USE_EXTENDED_MULTIPURPOSECAM
	HOOK(void*, __stdcall, _InitMultipurposeCameraTypeTube, 0x01041A40, CObjCameraTube* This, const boost::shared_ptr<CMultipurposeCameraExtended>& spMultipurposeCamera)
	{
		auto result = original_InitMultipurposeCameraTypeTube(This, spMultipurposeCamera);
		CMultipurposeCameraExtended* const mpCamera = spMultipurposeCamera.get();

		// If by some cosmic accident the name-setting fails, bail I guess.
		// TODO: We shouldn't have to do this name check here.... Consider removing it.
		const char* name = mpCamera->m_Name.c_str();

		const     int id = *(int*)name;
		constexpr int nameID = 0x65627554; // Tube

		if (id != nameID)
			return result;

		// Camera is a tube, 
		mpCamera->m_pPathController = This->m_spPathController.get();
		//DebugDrawText::log("Initialized Tube Camera", 20.0f);
		return result;
	}
#endif

	// HACK: Permanent Knuckles camera running in the background like in SA1.
	class CameraExtended : public Sonic::CCamera
	{
	public:
		DefaultCameraDependencies::CPlayer3DNormalCamera_Extended* m_DefaultCamera;
	};

	// RAII
	void __cdecl CCameraCtorHook(CameraExtended* This)
	{
		This->m_DefaultCamera = new DefaultCameraDependencies::CPlayer3DNormalCamera_Extended;
	}
	HOOK(void, __fastcall, _CameraDtor, 0x010FB630, CameraExtended* This)
	{
		delete This->m_DefaultCamera;
		original_CameraDtor(This);
	}
	ASMHOOK CCameraCtorHook1_asm()
	{
		__asm
		{
			push eax
			push ecx
			push edx

			push edi
			call CCameraCtorHook
			pop  edi

			pop  edx
			pop  ecx
			pop  eax

			// End
			pop  edi
			pop  esi
			pop  ebx
			mov  esp, ebp
			pop  ebp
			retn 4
		}
	}

	// Hook camera
	class __declspec(align(16)) CCameraController : public Hedgehog::Universe::CStateMachineBase::CStateBase
	{
	public:
		bool m_Cond01;
		float m_FOV;
		bool m_Cond02;
		bool m_Cond03;
		bool m_Cond04;
		Hedgehog::Math::CVector m_CameraPosition1;
		Hedgehog::Math::CVector m_UpVector1;
		Hedgehog::Math::CVector m_TargetPosition1;
		Hedgehog::Math::CVector m_CameraPosition2;
		Hedgehog::Math::CVector m_UpVector2;
		Hedgehog::Math::CVector m_TargetPosition2;
		float m_Float02;
	};
	ASSERT_SIZEOF(CCameraController, 0xE0);
	class __declspec(align(8)) CSetCameraController : public CCameraController
	{
		boost::shared_ptr<void> m_spSetCamera;
		float m_Float01 {};
		bool m_IsControllable {};
		float m_CameraPanX {};
		float m_CameraPanY {};
		float m_Float04 {};
	};

	HOOK(void, __fastcall, _SetCameraControllerUpdate, 0x010E7420, CSetCameraController* This)
	{
		original_SetCameraControllerUpdate(This);

		auto* const camera = GetValue<CameraExtended*>(This, 0x08);
		auto* const defaultCam = camera->m_DefaultCamera;

		auto* const sonic = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!sonic)
			return;

		//const hh::math::CVector targetPosition = sonic->m_spMatrixNode->m_Transform.m_Position;

		/*

		This->m_CameraPosition1 = defaultCam->m_CameraPositionVisual;
		This->m_CameraPosition2 = defaultCam->m_CameraPositionVisual;
		This->m_TargetPosition1 = defaultCam->m_CameraTargetPosition2;
		This->m_TargetPosition2 = defaultCam->m_CameraTargetPosition2;
		This->m_UpVector1 = defaultCam->m_CameraUpVector;
		This->m_UpVector2 = defaultCam->m_CameraUpVector;
		*/
	}

	// Hooking CCamera's updateparallel here attempts to accomplish two things.
	//
	// 1 - Allow a fallback to the default camera if you press the triggers or use the right stick while in a set camera, like SA1.
	// This is not properly implemented yet, but some code setting it up is done here, so hopefully it can be done later...
	// 2 - Log how much the camera is being influenced by certain set cameras, such as the "Tube" camera.
	// This lets us adjust gameplay based on which camera we have active, which is SOMETIMES what we want.
	// This needs to be approached with caution, though. Players don't want the game to feel completely different due to a perspective shift.
	// The biggest benefit to this, right now, is tube cameras, which in the Emerald Coast level function the same as 1D path cameras in SA1.

	// Set camera stuff has been migrated to its own function for clarity.
	void ParseSetCameraProperties(Sonic::CCamera* in_pCamera, const hh::fnd::SUpdateInfo& in_rUpdateInfo, CameraInterpolator* in_pInterpolator)
	{
		using namespace hh::math;
		// Using the singleton sucks, but there's no real way to pipe the kind of data I want to sonic cleanly, right now anyway.
		// TODO: We could probably access the *camera* as a singleton (this makes much more sense) & process all this in PLAYER MOVEMENT. Do this whenever possible.

		auto* const context = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!context)
			return;

		const float zSpeed = context->GetHorizontalVelocity().norm();
		const float frameDistance = zSpeed * in_rUpdateInfo.DeltaTime;
		const CVector frontDirection = context->GetFrontDirection();

		float amountDefaultCamera = 0.0f;
		float amountTubeCamera = 0.0f;
		CVector pathCameraForward(0, 0, 0);

		std::vector<CameraInterpolator*> interpolators;
		// These can stack up fast, so let's just do this to get everything in a simple, clean iteration.
		// Put in a scope so "workingInterpolator" gets cleared to prevent confusion with our for loop's interpolator.
		{
			CameraInterpolator* workingInterpolator = in_pInterpolator;
			while (workingInterpolator != nullptr)
			{
				interpolators.push_back(workingInterpolator);
				// With how Gens works, this WILL eventually be a nullpointer, so this is all we need to check.
				// In most cases it's a nullpointer on the second pass, but if set cameras have long transition times, it gets... big.
				workingInterpolator = workingInterpolator->m_spPreviousInterpolator;
			}
		}

		// Todo: Make an elegant solution for getting all the CVector's necessary from multiple tube cameras, so they can be summed up.
		for (const auto* currentInterpolator : interpolators)
		{
			auto* const cameraController = currentInterpolator->m_spCameraController;
			if (!cameraController)
				continue;

			// Handle case by case situations now.
			// TODO: Make a switch statement...? Maybe?

			if (*(int*)cameraController == 0x0169FBA0) // Sonic::CPlayer3DNormalCamera || i.e. the "Default" camera.
			{
				void* const controllerStateMachine = GetValue(cameraController, 0x0C);
				const float cameraInfluence = controllerStateMachine ? GetValue<float>(controllerStateMachine, 0x54) * GetValue<float>(cameraController, 0x20)
				                                                     : GetValue<float>(cameraController, 0x20);

				amountDefaultCamera += cameraInfluence;
				continue;
			}

			// For now, jump out at this point if controller isn't a set camera,
			// cuz set cameras & 3DNormal are all we care about at the moment.
			// TODO: Look into "CTubeCamera" (0x0169FE18) as it might be a bit weird but necessary to check.
			if (*(int*)cameraController != 0x0169FF9C) // Sonic::CSetCameraController || EVERY SetObject camera you can think of.
				continue;

			// From here on out we can assume cameraController is of type CSetCameraController.
			auto* const multipurposeCamera = GetValue<CMultipurposeCamera*>(cameraController, 0xE0);
			void* const controllerStateMachine = GetValue(cameraController, 0x0C);

			const char* name = multipurposeCamera ? GetPointer<hh::base::CSharedString>(multipurposeCamera, 0x04)->c_str()
				: "NONE";

			// HACK: small-string comparison for speed; no other set camera is called "Tube" so we can get away with cameraController.
			// All "nameIDs" are exact-case matches.
			constexpr int nameID_Tube = 0x65627554; // "Tube"
			if (*(int*)name == nameID_Tube)
			{
				const float cameraInfluence = controllerStateMachine ? GetValue<float>(controllerStateMachine, 0x54) * GetValue<float>(cameraController, 0x20)
				                                                     : GetValue<float>(cameraController, 0x20);

				amountTubeCamera += cameraInfluence;

				// Lovely back and fourth nonsense continues.
				// Compute position, get facing direction, & sample along spline by velocity.

				auto* const pathAnimController = multipurposeCamera->GetPathAnimationController();
				auto* const kfPath = pathAnimController->m_pAnimationEntity->m_spKeyframedPath.get();

				CVector PositionA(0, 0, 0);
				CVector PositionB(0, 0, 0);
				CVector InitialPathForward(0, 0, 0);

				CVector _(0, 0, 0);

				// Get our first sample so we get both position & path facing direction, towards or away from camera (so we sample correctly)
				kfPath->SamplePathGranular(pathAnimController->m_DistanceAlongPath, &PositionA, nullptr, &InitialPathForward);

				const float sign = CVector::Dot(InitialPathForward, frontDirection) > 0.0f
					? 1.0f
					: -1.0f;

				// Second sample then is used to look ahead, and make forward = the center of the path.
				kfPath->SamplePathGranular(pathAnimController->m_DistanceAlongPath + (frameDistance * sign), &PositionB, nullptr, nullptr);
				const CVector moveDirection = static_cast<CVector>(PositionB - PositionA).normalizedSafe();

				// Let's hope this works!
				pathCameraForward += moveDirection * cameraInfluence;
				continue;
			}
		}

		// Just to be safe.
		CVector::Normalize(&pathCameraForward);

		// Finally do this nonsense so the world makes sense for a minute.
		//const CVector defaultCameraForward = in_pCamera->m_MyCamera.m_Direction.ProjectOnPlane(context->m_FloorAndGrindRailNormal).normalizedSafe();
		const CVector defaultCameraForward = in_pCamera->m_MyCamera.m_Direction; // This gets projected on sonic's axis anyway so no need to do this right now.

		using namespace CustomCameras;
		CameraExtensions::m_DefaultCamAmount = Config::ms_CameraType == DefaultCameraType::CAMERA_GENS
		                                     ? amountDefaultCamera
		                                     : 0.0f;
		CameraExtensions::m_PathCamAmount = amountTubeCamera;
		CameraExtensions::m_CameraMoveDirection = CVector::Slerp(defaultCameraForward, pathCameraForward, amountTubeCamera);
	}

	// Let's try to clean ALL of this up by logging data from CCamera *directly,* as state changes are handled here anyhow.
	HOOK(void, __fastcall, _CCameraUpdateParallel, 0x010FB770, CameraExtended* This, void*, const hh::fnd::SUpdateInfo& updateInfo)
	{
		typedef Hedgehog::Universe::TStateMachine<Sonic::CCamera> CameraStateMachine;
		using namespace hh::math;

		// I eventually want to let people rotate the camera at any point like in SA1.
		// This is harder than it seems, and a previous attempt, which has some vestigial code here, has... unexpected results.
		// I'll look into what can be done someday, maybe, haha..
#pragma region SecondaryDefaultCamera

		// Initialize custom default camera cuz this doesn't get initialized already, it seems.
		auto* const defaultCam = This->m_DefaultCamera;
		if (defaultCam->m_pContext == nullptr)
		{
			defaultCam->m_pContext = This;
			defaultCam->m_pStateMachine = GetPointer<CameraStateMachine>(This, 0xA8);

			CustomCameras::PrepareNormalCamera(defaultCam, true);
		}

		CustomCameras::CameraUpdate_Knuckles(defaultCam, updateInfo.DeltaTime, false);

#pragma endregion

		original_CCameraUpdateParallel(This, nullptr, updateInfo);

		CameraStateMachine* stateMachine = GetPointer<CameraStateMachine>(This, 0xA8);
		CameraInterpolator* initialInterpolator = GetValue<CameraInterpolator*>(stateMachine, 0x58);

		if (!initialInterpolator)
			return;

		ParseSetCameraProperties(This, updateInfo, initialInterpolator);
	}

	// ----------------------------------------------------------------------------------------
	// Alright time to deal with volumes having weird hitches if they target the same camera.
	// This is only really a problem because I combine camera volumes to fill up space.
	// By default, this is not something Gens supports really.
	// You CAN do it, but your camera will either pause in place,
	// or you'll get FOV undulating as the camera transitions... into itself...
	//
	// The solution I settled on is basically reference counting camera on/off calls,
	// and ignore any cases of cameras being enabled/disabled if they're referenced more than once.
	// This *MIGHT* have some drawbacks, I recall some happening, but it seems stable enough now.
	// ----------------------------------------------------------------------------------------

	struct CameraMap
	{
	private:
		// Required stuff
		void* ptrA = nullptr;
		void* ptrB = nullptr;
		void* ptrC = nullptr;
		int dataA = 0;
		int dataB = 0;
		int dataC = 0;
		bool condA = true;
		bool condB = false;

	public:
		// Custom stuff
		int referenceCount = 0;
		uint32_t lastActorID = 0;

		void* operator new(const size_t size)
		{
			FUNCTION_PTR(CameraMap*, __cdecl, SmallAlloc, 0x0065FC80, size_t size, void* metadata);
			return SmallAlloc(size, nullptr);
		}

		void operator delete(void* pMem)
		{
			return __HH_FREE(pMem);
		}
	};
	class ExtendedCamera
	{
		INSERT_PADDING(0xFC) {}; // 0xF8 offset + 0x04 dead space
	public:
		CameraMap* map;
	};

	// Override map allocator for camera base class so we can do our own bookkeeping stuff.
	CameraMap* AllocateCameraMap()
	{
		return new CameraMap;
	}

	HOOK(void, __fastcall, _MsgCameraOn,  0x01051EE0, ExtendedCamera* camera, void*, Hedgehog::Universe::Message* msg)
	{
		CameraMap* map = camera->map;
		if (++map->referenceCount > 1)
		{
			//msg->m_SenderActorID = map->lastActorID;
			return;
		}

		map->lastActorID = msg->m_SenderActorID;
		original_MsgCameraOn(camera, nullptr, msg);
	}
	HOOK(void, __fastcall, _MsgCameraOff, 0x01051DB0, ExtendedCamera* camera, void*, Hedgehog::Universe::Message* msg)
	{
		CameraMap* map = camera->map;
		if (map->referenceCount-- > 1)
		{
			//msg->m_SenderActorID = map->lastActorID;
			return;
		}

		msg->m_SenderActorID = map->lastActorID;
		original_MsgCameraOff(camera, nullptr, msg);
	}

	// Tube camera debugging

	class ICameraPositionController
	{
	public:
		ICameraPositionController() {}
		virtual ~ICameraPositionController() {};
	};
	class CameraTubePositionNode : public ICameraPositionController
	{
	public:
		boost::shared_ptr<void> m_PathController{};
		Hedgehog::Math::CVector m_Position{};

		float m_PlayerInfluenceRatio;
		float m_OffsetY;
		float m_OffsetZ;

		Hedgehog::Math::CVector m_UpVector{};
		Hedgehog::Math::CVector m_ForwardVector{};
		float m_DeltaTime = 1.0f / 60.0f;
	};

	ASSERT_OFFSETOF2(CameraTubePositionNode, m_PathController, 0x04);
	ASSERT_OFFSETOF2(CameraTubePositionNode, m_ForwardVector, 0x40);
	ASSERT_OFFSETOF2(CameraTubePositionNode, m_PlayerInfluenceRatio, 0x20);
	ASSERT_OFFSETOF2(CameraTubePositionNode, m_OffsetY, 0x24);
	ASSERT_OFFSETOF2(CameraTubePositionNode, m_OffsetZ, 0x28);
	ASSERT_EQUALITY(sizeof(CameraTubePositionNode), 0x60);

	HOOK(void, __fastcall, _UpdateTubePositionEye, 0x01041270, CameraTubePositionNode* node, void*, int a2, int* a3)
	{
		if (s_DebugTubeCams)
		{
			node->m_PlayerInfluenceRatio = s_EyeRatio;
			node->m_OffsetY = s_EyeOffsetY;
			node->m_OffsetZ = s_EyeOffsetZ;
		}

		original_UpdateTubePositionEye(node, nullptr, a2, a3);
	}
	HOOK(void, __fastcall, _UpdateTubePositionTrg, 0x01040D60, CameraTubePositionNode* node, void*, int a2, int* a3)
	{
		if (s_DebugTubeCams)
		{
			node->m_PlayerInfluenceRatio = s_TrgRatio;
			node->m_OffsetY = s_TrgOffsetY;
			node->m_OffsetZ = s_TrgOffsetZ;
		}

		original_UpdateTubePositionTrg(node, nullptr, a2, a3);
	}



	void Init()
	{
		INSTALL_HOOK(InitializeApplicationParams_PathDebug)
		INSTALL_HOOK(_CCameraUpdateParallel)

		// Add permanent knuckles camera pointer.
		WRITE_MEMORY(0x00D63867 + 1, uint32_t, sizeof(CameraExtended))
		WRITE_MEMORY(0x0058CBB6 + 1, uint32_t, sizeof(CameraExtended))
		WRITE_JUMP(0x010FCD20, CCameraCtorHook1_asm)
		INSTALL_HOOK(_CameraDtor)
		// Override w/ Knuckles camera
		INSTALL_HOOK(_SetCameraControllerUpdate)

		//INSTALL_HOOK(_CameraPointEnter)
		//INSTALL_HOOK(_CameraPanEnter)

		// Hitch fixing
		WRITE_CALL(0x0105267C, AllocateCameraMap);
		INSTALL_HOOK(_MsgCameraOn)
		INSTALL_HOOK(_MsgCameraOff)

		// Tube camera debug
		INSTALL_HOOK(_UpdateTubePositionEye)
		INSTALL_HOOK(_UpdateTubePositionTrg)

		// Fuck off sonic team.
		WRITE_MEMORY(0x01041CB3, uint32_t, 0x01A48CD4) // Replaces shitty hardcoded 45.0f FOV with "DEFAULT_FOV_Y" like the game SHOULD do.

		// Hack fix for tube camera position / up vector seek distance.
		// TODO: See if it's possible to make a DOUBLE changeable via parameters, or if we have to just force the static double to = what our float is on stage load...
		WRITE_MEMORY(0x010413E2 + 4, double*, &s_TubeCameraSeekDistance)
		WRITE_MEMORY(0x01041406 + 4, double*, &s_TubeCameraSeekDistance)
		WRITE_MEMORY(0x01041520 + 4, double*, &s_TubeCameraSeekDistance)
		WRITE_MEMORY(0x01041541 + 4, double*, &s_TubeCameraSeekDistance)

		//INSTALL_HOOK(_CameraVolumeOnEnter)
		//INSTALL_HOOK(_CameraVolumeOnLeave)

		// We only expand & initialize the memory of our data here, because this is called... A LOT.
		// We're abusing the fact MultiPurposeCamera's have a name ID. If they didn't, we'd have to allocate for all instances.
		// This means we have to be ABSOLUTELY, UNEQUIVOCALLY SURE that we have a TUBE CAMERA active, and *NOT* anything else!!
		//WRITE_MEMORY(0x01041C72 + 1, uint32_t, sizeof(CMultipurposeCameraExtended))
		//INSTALL_HOOK(_InitMultipurposeCameraTypeTube)

	}
}

void Cameras::Init()
{
	CustomCameras::Init();
	SetCameras::Init();
}
void Cameras::OnFrame()
{
	using namespace CustomCameras;
	DebugDrawText::log(format("DEFAULTCAM: %f", CameraExtensions::m_DefaultCamAmount));
	DebugDrawText::log(format("PATHCAM:    %f", CameraExtensions::m_PathCamAmount));
}
// Word of warning: This file is very messy.

#include "PhysicsSpindash.h"

#include "Cameras.h"
#include "Collisions.h"
#include "PhysicsGameplay.h"
#include "Input.h"

#include <numbers>
#include "../DebugDrawText.h"


using namespace GlobalInput;
using namespace Sonic::Player;
using namespace hh::math;

namespace Gameplay_Spindash
{
#pragma region FunctionPtrs
	FUNCTION_PTR(void, __fastcall, Posture3DCommon_MovementRoutine, 0x00E37FD0, CPlayerSpeedContext::CStateSpeedBase* state);
	FUNCTION_PTR(bool, __thiscall, SonicGeneralStateChangeMethod,   0x00DC5EF0, CPlayerSpeedContext* This, CPlayerSpeedContext::CStateSpeedBase* state);

	__declspec(noinline) uint32_t __cdecl SpawnHomingTrail(void* This, bool a2)
	{
		uint32_t func = 0x00E5FDD0;
		uint32_t result = 0;
		__asm
		{
			mov eax, This
			push a2
			call func
			mov result, eax
		}
		return result;
	}

	__declspec(noinline) bool __cdecl CheckInputHeld(Sonic::Player::CInputPad* pad, uint32_t id)
	{
		static constexpr uint32_t func = 0x00D97DA0;
		bool result = false;
		__asm
		{
			mov edi, id
			mov esi, pad
			call func
			mov result, al
		}
		return result;
	}
	__declspec(noinline) bool __cdecl CheckInputTapped(Sonic::Player::CInputPad* pad, uint32_t id)
	{
		static constexpr uint32_t func = 0x00D97E00;
		bool result = false;
		__asm
		{
			mov edi, id
			mov eax, pad
			call func
			mov result, al
		}
		return result;
	}

#pragma endregion 

	// A lot of these parameters are set to be configurable for my own debugging purposes.
	// However, I've left them debuggable in this mod in case anyone wants to adjust these params themselves without building from source.
	HOOK(void, __cdecl, InitializeApplicationParams_SPIN, 0x00D65180, Sonic::CParameterFile* This)
	{
		using namespace Gameplay_Adventure;
		boost::shared_ptr<Sonic::CParameterGroup> parameterGroup = This->CreateParameterGroup("Adventure", "SA-Style related parameters");
		{
			Sonic::CEditParam* category = parameterGroup->CreateParameterCategory("Spindash", "Spindash");

			category->CreateParamFloat(&ModParameters::spinGravityMultiplier, "Gravity multiplier");
			category->CreateParamFloat(&ModParameters::spinGravityLossCurveLow,  "Gravity loss curve LOW");
			category->CreateParamFloat(&ModParameters::spinGravityLossCurveHigh, "Gravity loss curve HIGH");
			category->CreateParamFloat(&ModParameters::spinDecelCurveLow, "Friction curve LOW");
			category->CreateParamFloat(&ModParameters::spinDecelCurveLow, "Friction curve HIGH");
			category->CreateParamFloat(&ModParameters::spinWorldInputExponent, "Input curve exponent value");
			category->CreateParamFloat(&ModParameters::spinTurnRestrictionMaxSpeed, "Spindash stiff-turning MAX speed");
			category->CreateParamFloat(&ModParameters::spinTurnRestrictionMinSpeed, "Spindash stiff-turning MIN speed");
			category->CreateParamFloat(&ModParameters::spinTurnEaseFactor, "Rolling ease factor");
			category->CreateParamFloat(&ModParameters::spinTurnAdjustedByVelocity, "Spindash affected-by-velocity amount.");
		}
		parameterGroup->Flush();

		originalInitializeApplicationParams_SPIN(This);
	}

#pragma region HomingTrailPatch

	// Bit of a "fun" one.
	// Generations spawns the homing attack trail, but self-manages its lifetime causing it to fade and then delete itself when necessary.
	// The trail object ITSELF has a timer controlling how opaque it is, and when it hits 0, it destroys itself.
	// For an Adventure-style spindash trail, we of course want this to have a lifetime of "1" when rolling.
	// ...But there's a problem.
	//
	// The default "spawn trail" function returns -void-, meaning we do not get a reference to the trail when it's created.
	// This kind of sucks if we want to spawn the homing trail and actually modify it at runtime, because this means we have NO REFERENCE to it.
	// To combat this, I use these 3 assembly patches which do the following in order:
	//
	// - Store the value of the EBX register (which is unused in this function by default) by pushing it on the stack in the function's prologue
	// - Grab the pointer to the trail before it's dropped in this function and store it in EBX
	// - Set our EAX (return) register to the value of EBX, which has a pointer to our trail, and pop EBX's original value.
	//
	// In the future, to avoid conflicts / mods doing the same thing, I'll probably want to re-implement gens' function for this.
	// -----------------------------------------------------------------------------------------------------------------------------------------
	
	ASMHOOK SpawnHomingAttackHook_1()
	{
		static constexpr uint32_t ret = 0x00E5FDEA;
		__asm
		{
			// boilerplate
			cmp     byte ptr[ecx + 6Fh], 0
			push    edi

			// push ebx to get it back later
			push ebx

			jmp[ret]
		}
	}
	ASMHOOK SpawnHomingAttackHook_2()
	{
		static constexpr uint32_t ret = 0x00E5FEC9;
		__asm
		{
			// boilerplate
			mov     eax, [esp + 20h - 10h]
			test    eax, eax

			// copy to EBX so we can move this back to EAX later.
			mov ebx, eax

			jmp[ret]
		}
	}
	ASMHOOK SpawnHomingAttackHook_3()
	{
		__asm
		{
			// replace EAX with what's in EBX, what we wanted all along.
			mov eax, ebx

			// pop our EBX first
			pop ebx

			// function epilogue
			pop     edi
			pop     esi
			mov     esp, ebp
			pop     ebp

			// end of func, we can just return here.
			retn    4
		}
	}

#pragma endregion

	// Helper funcs
	inline bool RollOnCrouch(CPlayerSpeedContext* const context)
	{
		if (!(*Input::pressedButtons & Input::Buttons::Crouch)) return false;

		// whenever we crouch, we want to keep our velocity and aim in the direction we're going.
		// In the future, we want to 
		const Hedgehog::Math::CVector hVelocity = context->GetHorizontalVelocity();
		context->ChangeState(ePlayerSpeedState_Spin);
		context->SetHorizontalVelocityClearChanged(hVelocity);
		return true;
	}
	inline void RotateToVelocity(CPlayerSpeedContext* context, const hh::math::CQuaternion invVertRotation)
	{
		using namespace hh::math;
		using namespace Sonic::Player;

		CVector currentHorizontalDirection = context->GetHorizontalVelocityDirection();
		currentHorizontalDirection = invVertRotation * currentHorizontalDirection;

		// HACK: Trying to fix jitter through some... nonsense.
		//currentHorizontalDirection = CVector::ProjectOnPlane(currentHorizontalDirection, CVector::Up()).normalizedSafe
		currentHorizontalDirection.y() = 0;
		CQuaternion targetRotation = CQuaternion::LookRotation(currentHorizontalDirection.normalizedSafe());

		context->m_HorizontalRotation = targetRotation;
	}

	// This is a rather complicated, perhaps OVER-complicated, block of gameplay code that attempts to do a few things:
	//
	// - Prioritize downhill movement increasing speed for "fun factor"
	// - Avoid uphill movement making you lose too much speed
	// - Accomplish the above without running into "perpetual motion" situations, i.e. gaining more speed downhill than we lose uphill
	// - Give the player the familiar control of SA1's spindash, but with *actual* physics
	//
	// There are a lot of problems with this code that might get thrown out, might not.
	// One could argue a "sonic adventure" spindash should just be what SA1 did as close as possible,
	// while I argue SA1's rolling physics are quite poor and the weakest part of the game.
	// That's all very opinionated, and honestly my own opinion on this can/will change, so... TBD.
	//
	// ---------------------------------------------------------------
	void SpindashMovement(CPlayerSpeedContext::CStateSpeedBase* state)
	{
		using namespace hh::math;
		using namespace Sonic::Player;
		using namespace Sonic::Message;
		using namespace Gameplay_Adventure;

		CPlayerSpeedContext* const context = state->GetContext();
		const float deltaTime = state->GetDeltaTime();
		const CVector gravityDirection = CVector(0, -1, 0); // TODO: ARBITRARY GRAVITY?? Lost World support???

		const CVector startHorizontalVelocity = context->GetHorizontalVelocity();
		const CVector startVelocityDirection = context->GetHorizontalVelocityDirection();
		// We need this for later.
		const CVector originalVerticalVelocity = context->m_VerticalVelocity;

		const float velocityMag = static_cast<float>(startHorizontalVelocity.Magnitude());

		// Necessary for certain computations, especially dash paths.
		auto* camera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera().get();
		

		// Code stuff

		const float gravityPrecompute = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_Gravity)
		                              * ModParameters::spinGravityMultiplier * deltaTime;

		// Limit the force of gravity based on slope angle.
		const float gravitySlopeDot = 1.0f - fabs(CVector::Dot(context->m_FloorAndGrindRailNormal, gravityDirection)); // 0.0f = ground, 1.0f = wall
		const float gravityForceMultiplier = std::lerp(0.5f, 1.0f, gravitySlopeDot);
		// UNDONE: Weaker gravity if we're moving slower
		//const float initialGravityForce = std::lerp(gravityPrecompute * 0.4f, gravityPrecompute, std::inverseLerp(0.0f, 10.0f, velocityMag));
		const float initialGravityForce = gravityPrecompute;
		
		// Basic physics: Apply gravity every frame
		//const CVector addVelocity = context->GetVelocity() + (gravityDirection * (initialGravityForce * gravityForceMultiplier));
		context->AddVelocity(gravityDirection * (initialGravityForce * gravityForceMultiplier));

		// Reset component velocities for convenience, for friction math & input.
		context->SetHorizontalVelocity(context->GetHorizontalVelocity());
		context->m_VerticalVelocity = originalVerticalVelocity;

		// Gravity needs to be computed before input, so now we base forward path input off our gravity.

#pragma region input

		// 
		// If we're on a dash path and out-of-control, we NEED to make the target direction the relative forward direction of the path.
		auto GetInputDirection = [](CPlayerSpeedContext* context, Sonic::CCamera* camera, float deltaTime) -> CVector
		{
			// We want to process world input the same way between movement & spindash based on context.
			const float zSpeed = context->m_HorizontalVelocity.norm();
			const CVector worldInput   =  GetGuidedInputDirection(context, context->m_WorldInput, zSpeed, Core::SPhysicsEssentials::Get(context), true);
			const auto& pathController = *GetPointer<boost::shared_ptr<Sonic::CPathController>>(context, 0x128C);

			// Don't need to do anything if we're not moving at all.
			if (zSpeed < FLT_EPSILON)
				return worldInput;

			// If we're not on a path, we only really need "input" to be our forward direction.
			if (!pathController)
				return context->StateFlag(eStateFlag_OutOfControl)
				? context->GetFrontDirection()
				: worldInput;

			const auto* pathAnimController = pathController.get()->m_spPathAnimationController.get();
			Sonic::KeyframedPath* kfPath   = pathAnimController->m_pAnimationEntity->m_spKeyframedPath.get();

			const float pathDistance = pathAnimController->m_DistanceAlongPath;

			CVector pathDirection(0, 0, 0);
			CVector pathUp(0, 0, 0);
			CVector pathPosition(0, 0, 0);

			// First sample iteration, get path direction at closest point & dot it with camera forward.
			kfPath->SamplePathGranular(pathDistance, nullptr, &pathUp, &pathDirection);

			// Check if our velocity is in the direction of the path or not.
			const bool isPathForward = CVector::Dot(context->m_HorizontalVelocity.ProjectOnPlane(pathUp), pathDirection) > 0.0f;

			// Now we need our horizontal velocity downwards.
			float frameSpeed = zSpeed * deltaTime;

			if (!isPathForward)
				frameSpeed *= -1.0f;

			// Second sample iteration, get the position along the path given one frame of velocity forward.
			kfPath->SamplePathGranular(pathDistance + frameSpeed, &pathPosition, nullptr, nullptr);

			const CVector pathAimDirection = CVector::Normalized(pathPosition - context->m_spMatrixNode->m_Transform.m_Position);

			const CVector result = context->StateFlag(eStateFlag_OutOfControl)
			                     ? pathAimDirection
			                     : worldInput;

			return result;
		};

		const CVector worldInputDirection = GetInputDirection(context, camera, deltaTime);
		const float   worldInputStrength  = context->m_WorldInput.Length();

		// TODO: FIND WHAT EVEN SETS HORIZONTAL/VERTICAL ROTATION SO YOU DONT HAVE TO BE A HACK!!
		const CQuaternion invVertRotation = context->m_VerticalRotation.inverse();
		CVector worldInputHorizontalAligned = invVertRotation * worldInputDirection;
		worldInputHorizontalAligned.y() = 0;
		worldInputHorizontalAligned = worldInputHorizontalAligned.normalizedSafe() * worldInputStrength;


		//const float frictionCorrected = std::lerp(0.0f, frictionParameter * deltaTime, std::inverseLerp(-15.0f, 20.0f, velocityMag));

		const float frictionFrame = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_StaticResistanceForceWhenRun) * deltaTime;
		const float inputLerp = std::clamp(std::pow(worldInputStrength, ModParameters::spinWorldInputExponent), 0.0f, 1.0f);

#pragma endregion

		const float gravityVelocityMag = context->m_HorizontalVelocity.Length();
		const CVector gravityVelocity = context->GetHorizontalVelocity();
		const CVector gravityVelocityDirection = gravityVelocity.normalizedSafe(); // FIXME: I forgot what i was going to use this for lol


		const bool IsMoving = gravityVelocity.LengthSqr() > FLT_EPSILON;
		const bool IsStickHeld = worldInputStrength > FLT_EPSILON || context->StateFlag(eStateFlag_OutOfControl);

		// Not moving and not holding the stick, just dip.
		if (!IsMoving && !IsStickHeld)
		{
			Posture3DCommon_MovementRoutine(state);
			return;
		}

		const float velocityLost   = fmax(0.0f, velocityMag - gravityVelocityMag);
		const float velocityGained = fmax(0.0f, gravityVelocityMag - velocityMag);
		const float frictionForce  = fmax(0.0f, frictionFrame - velocityLost);

		// Unused
		const float decelerationForceTotal = velocityLost + frictionForce;

		// Let's cap our forward velocity so we don't exceed the game's limits.
		auto SetForwardAccel = [&context, &velocityGained, &velocityMag]() -> float
		{
			const float velocityLimitMin = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_MaxDownVelocity);
			const float velocityLimitMax = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_MaxVelocityFinalMax); // We cannot exceed this.

			const float finalAcceleration = std::lerp(velocityGained, 0.0f,
			                                std::inverseLerp(velocityLimitMin, velocityLimitMax, velocityGained + velocityMag));
			return finalAcceleration;
		};

#ifdef _DEBUG
		if (IsStickHeld)
			DebugDrawText::log("HELD", 0, 0, { 0,1,0.2,1 });
		else
			DebugDrawText::log("RELEASED", 0, 0, { 1,0,0.2,1 });
#endif

		// Moving, but not holding the stick? Only apply basic physics (gravity and friction) + speed cap
		if (IsMoving && !IsStickHeld)
		{
			const float frictionForceTemp = std::lerp(0.0f, frictionForce,
				                                      std::inverseLerp(ModParameters::spinDecelCurveLow, ModParameters::spinDecelCurveHigh, velocityMag));
			const CVector velocityDirection = context->GetHorizontalVelocityDirection();

			const CVector frictionVector   = velocityDirection * frictionForceTemp;

			// For some WEIRD reason, if we're slowing down, there's a bit of a hitch.
			// Something I'm doing has to be wrong, but that's the only case that happens.
			// Because this is dealing with a speed cap, we shouldn't have to worry about this other than this one edge case.
			// So to compensate, we just operate as normal if your velocity is under our min cap range.
			if (velocityMag < context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_MaxDownVelocity))
			{
				context->SetHorizontalVelocityClearChanged(context->m_HorizontalVelocity - frictionVector);
			}
			else
			{
				const CVector speedCappedAccel = velocityDirection * (SetForwardAccel() - velocityLost);
				context->SetHorizontalVelocityClearChanged(startHorizontalVelocity + speedCappedAccel - frictionVector);
			}

			RotateToVelocity(context, invVertRotation);

			const CVector velocity = context->GetVelocity();
			const float UpwardForceDot = fmax(0.0f, CVector::Dot(velocity, context->m_FloorAndGrindRailNormal));
			context->SetVelocity(velocity - (context->m_FloorAndGrindRailNormal * UpwardForceDot));

			//const bool isDownForce = context->StateFlag(eStateFlag_DisableDownForce);
			Posture3DCommon_MovementRoutine(state);
			//context->StateFlag(eStateFlag_DisableDownForce) = isDownForce;
			return;
		}

		// Stick movement
		CVector targetMoveVector;
		if (startVelocityDirection.LengthSqr() < FLT_EPSILON)
		{
			targetMoveVector = worldInputDirection;
		}
		else
		{
			const float targetAngle = CVector::Angle(startVelocityDirection, worldInputDirection) * RAD2DEG;

			// SA1 angle stuff
			float angleAdjustment = 11.25f * SA1Parameters::TargetFramerate * deltaTime;
			if (targetAngle <= 45.0f)
			{
				angleAdjustment = targetAngle / 4.0f;

				if (targetAngle <= 22.5f)
				{
					angleAdjustment = targetAngle / 8.0f;
				}
			}

			// HACK: Make rotation less ASS at high speed

			//angleAdjustment = std::lerp(angleAdjustment, 0.0f, std::inverseLerp(0.0f, 300.0f, velocityMag));
			const float spinTurnLerp = std::inverseLerp(ModParameters::spinTurnRestrictionMinSpeed, ModParameters::spinTurnRestrictionMaxSpeed, velocityMag)
			                         * std::clamp(ModParameters::spinTurnAdjustedByVelocity, 0.0f, 1.0f);

			const float camDistance = CVector::Length(camera->m_MyCamera.m_Position -
			                                          context->m_spMatrixNode->m_Transform.m_Position);

			DebugDrawText::log(format("Camera Distance: %f", camDistance));
			const float cameraDistanceLerpValue = 1.0f - std::inverseLerp(4.5f, 10.0f, camDistance);

			angleAdjustment = std::lerp(angleAdjustment, angleAdjustment * 0.001f,
			                            std::reversePower(spinTurnLerp, ModParameters::spinTurnEaseFactor) // High speed interp
			                * CustomCameras::CameraExtensions::m_DefaultCamAmount * cameraDistanceLerpValue); // Only interp if default cam is enabled.

			const float angleLerpValue = std::inverseLerp(0, targetAngle, angleAdjustment);

			/*
			float angleLerpValue = std::inverseLerp(0, targetAngle, angleAdjustment);
			// HACK: Make rotation less ASS at high speed
			angleLerpValue = std::lerp(angleLerpValue, 0.0f, std::inverseLerp(0.0f, 100.0f, velocityMag));
			*/

			targetMoveVector = CVector::Slerp(startVelocityDirection, worldInputDirection, angleLerpValue);
		}
		
		//const float moveForwardMagnitude = std::max(gravityVelocityMag, velocityMag);
		const float moveForwardMagnitude = velocityMag + SetForwardAccel();
		float decelAmount = velocityLost + std::lerp(0.0f, frictionForce,
			std::inverseLerp(ModParameters::spinDecelCurveLow, ModParameters::spinDecelCurveHigh, velocityMag));

		// Stick + Gravity mixing
		if (worldInputStrength > FLT_EPSILON)
		{
			//const float velocityLerpValue = std::inverseLerp(10.0f, 30.0f, gravityVelocityMag);
			//const float inputVelocityMag = std::lerp(gravityVelocityMag, std::max(gravityVelocityMag, velocityMag), velocityLerpValue);
			//const float moveAmount = std::lerp(gravityVelocityMag, inputVelocityMag, inputLerp);

			// TODO: See about weather or not wall friction should use THIS value or not.
			//targetMoveVector =  CVector::Slerp(startVelocityDirection, targetMoveVector, inputLerp);

			// Logic for ignoring the force of gravity if input is perpendicular to gravity.

			const float ignoreGravityWithInputCutoff = 0.01f;
			float gravityProjectLength = 0.0f;

			CVector horizontalVelocityDirection = context->m_HorizontalVelocity.normalizedSafe();
			CVector projectVector = CVector::Zero();
			if (horizontalVelocityDirection.LengthSqr() > FLT_EPSILON)
			{
				projectVector = CVector::ProjectOnPlane(gravityDirection, context->m_ModelUpDirection);
				gravityProjectLength = projectVector.Length();
			}

			float wallFrictionLerpValue = 0.0f;
			if (gravityProjectLength > ignoreGravityWithInputCutoff)
			{
				wallFrictionLerpValue = 1.0f - std::abs(CVector::Dot(projectVector.normalizedSafe(), context->m_VerticalRotation * targetMoveVector));
			}

			const float gravityLoss = std::lerp(velocityLost, 0.0f, wallFrictionLerpValue);
			const float frictionFinal = std::lerp(frictionForce, frictionFrame, wallFrictionLerpValue);

			// Rotate our vector based on stick weight.
			targetMoveVector = CVector::Slerp(startVelocityDirection, targetMoveVector, inputLerp);

			// TODO: SPLIT GRAVITY'S INVERSE LERP AND FRICTION'S, THEN ADD THEM
			// FIXME: NO THIS IS VERY IMPORTANT SERIOUSLY, FRICTION NEEDS TO BE -15.0F WHILE GRAVITY IS 0.0F (update: -5 i guess)
			// Consider changing Gravity's CurveHigh as well
			// const float decelTotalCurve = std::lerp(0.0f, decelerationForceTotal, std::inverseLerp(0.0f, decelCurveHigh, velocityMag));

			const float decelCurveGrav = std::lerp(0.0f, gravityLoss,
			                             std::inverseLerp(ModParameters::spinGravityLossCurveLow, ModParameters::spinGravityLossCurveHigh, velocityMag));
			const float decelCurveFric = std::lerp(0.0f, frictionFinal,
			                             std::inverseLerp(ModParameters::spinDecelCurveLow,       ModParameters::spinDecelCurveHigh,       velocityMag));

			const float decelTotalCurve = decelCurveFric + decelCurveGrav;

			decelAmount = std::lerp(decelAmount, decelTotalCurve, inputLerp);
		}

		// Move in our target direction + forward force
		context->SetHorizontalVelocity(targetMoveVector * moveForwardMagnitude);

		// Deceleration (gravity + friction) step
		const CVector frictionVector = targetMoveVector * decelAmount;
		context->SetHorizontalVelocityClearChanged(context->m_HorizontalVelocity - frictionVector);

		// Rotate towards velocity
		if (IsMoving)
			RotateToVelocity(context, invVertRotation);

		// Handle all collision and wall/bump related deceleration.
		Posture3DCommon_MovementRoutine(state);
	}

#pragma region SpindashCharge

	// -------------------------------------------------------------------------------
	//
	// The functions below have to do with the actual "spindash," the CHARGING action, which involves a few things getting changed.
	// It's a bit messy and perhaps these should be put in their own CPP file, and some things get changed that probably shouldn't be.
	//
	// -------------------------------------------------------------------------------

	void SpindashChargeRotation(CPlayerSpeedContext::CStateSpeedBase* state)
	{
		using namespace hh::math;
		using namespace Sonic::Player;

		CPlayerSpeedContext* const context = state->GetContext();
		//const float deltaTime = state->GetDeltaTime();

		if (context->m_WorldInput.LengthSqr() < FLT_EPSILON) return;
		if (context->m_Is2DMode) return;

		const CMatrix44 TransposedRotationMatrix = context->m_VerticalRotation.ToRotationMatrix().transpose();
		CVector frontDirection      = TransposedRotationMatrix.TransformVector(context->GetFrontDirection());
		CVector worldInputDirection = TransposedRotationMatrix.TransformVector(context->m_WorldInput.normalizedSafe());
		CVector velocityDirection   = TransposedRotationMatrix.TransformVector(context->GetHorizontalMovementDirection());

		// HACK: zero out Y value for these because fuck me i guess
		frontDirection.y()      = 0.0f; frontDirection = frontDirection.normalizedSafe();
		worldInputDirection.y() = 0.0f; worldInputDirection = worldInputDirection.normalizedSafe();

		const float deltaTime = state->GetDeltaTime();

		// SA1-accurate option. For some reason I moved away from this, so here it is for posterity. Feels similar enough?
		/*
		const float targetAngle = CVector::Angle(frontDirection, worldInputDirection) * RAD2DEG;
		const float frameDeltaTime = SA1Parameters::TargetFramerate * deltaTime;

		// SA1 angle stuff
		float angleAdjustment = 11.25f;
		if (targetAngle <= 45.0f)
		{
			angleAdjustment = targetAngle / 4.0f;

			if (targetAngle <= 22.5f)
			{
				angleAdjustment = targetAngle / 8.0f;
			}
		}
		//const float   angleLerpValue    =  std::inverseLerp(0, targetAngle, angleAdjustment * frameDeltaTime);
		//const CVector targetMoveVector  =  CVector::Slerp(frontDirection, worldInputDirection, angleLerpValue);
		*/

		const CVector targetMoveVector  =  CVector::RotateTowards(frontDirection, worldInputDirection, 720.0f * DEG2RADf * deltaTime);


		// If your input is an EXACT 180 from your direction, this won't work. TODO: Consider using Gens' global angle method.
		const CQuaternion newYawRotation = CQuaternion::LookRotation(targetMoveVector, CVector::Up());
		context->SetYawRotation(newYawRotation);

		// Rotate velocity now.
		const float velocityDot = fmax(0.0f, CVector::Dot(frontDirection, velocityDirection));
		const CQuaternion finalRotation = context->m_VerticalRotation * newYawRotation;
		const CVector horizontalVelocity = context->GetHorizontalVelocity();
		const float horizontalSpeed = horizontalVelocity.norm();
		const float verticalSpeed = context->m_VerticalVelocity.norm();
		const CVector velocity = context->m_Velocity;

		//context->SetHorizontalVelocity_Dirty(CVector::Lerp(horizontalVelocity, finalRotation * CVector(0, 0, horizontalSpeed), velocityDot));
		context->SetVelocity(CVector::Lerp(velocity, finalRotation * CVector(0, verticalSpeed, horizontalSpeed), velocityDot));
	}

	// TODO: MAKE CLASS MEMBER FOR STATESPIN
	float spinZeroVelocityHeuristicTimer = 0.0f;

	// Hooks
	HOOK(void, __fastcall, _ClassicSlideUpdate, 0x011D6140, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		using namespace hh::math;
		auto context = a1->GetContext();

		// This lets us grip the ground at all times.
		const bool previousForceGrip = context->StateFlag(eStateFlag_ForceLandForCaught);
		context->StateFlag(eStateFlag_ForceLandForCaught) = context->GetHorizontalVelocity().squaredNorm() > 5.0f * 5.0f
		                                                  ? true : previousForceGrip;

		SpindashMovement(a1);

		context->StateFlag(eStateFlag_ForceLandForCaught) = previousForceGrip;
		CInputPad* pad = context->m_spInputPad.get();
	}
	HOOK(void, __fastcall, _ClassicWalkStart,       0x01241B50, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		using namespace Hedgehog::Math;

		auto context = a1->GetContext();
		const float deltaTime = a1->GetDeltaTime();

		if (context->m_TimeGrounded < deltaTime * 3.0f
			&& *Input::heldButtons & Input::Buttons::Crouch)
		{
			CVector hVelocity = context->GetHorizontalVelocity();
			context->ChangeState(ePlayerSpeedState_Spin);
			context->SetHorizontalVelocityClearChanged(hVelocity);

			// inefficient, but... let's also add the force of gravity on this frame
			// so if you land on a slope without moving at *all,* you'll still roll.
			CVector vVelocity = context->m_VerticalVelocity;
			context->AddVelocity(CVector(0, -context->m_spParameter->Get<float>(ePlayerSpeedParameter_Gravity) * std::numbers::pi * deltaTime, 0));
			context->HandleVelocityChanged();
			context->m_VerticalVelocity = vVelocity;

			return;
		}

		original_ClassicWalkStart(a1);
	}
	HOOK(void, __fastcall, _ClassicWalkUpdate,      0x012419B0, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		if (RollOnCrouch(a1->GetContext()))
			return;
		original_ClassicWalkUpdate(a1);
	}
	HOOK(void, __fastcall, _ClassicWalkSlideUpdate, 0x0123A330, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		if (RollOnCrouch(a1->GetContext()))
			return;
		original_ClassicWalkSlideUpdate(a1);
	}

	HOOK(void, __fastcall, _ClassicSpinStateStart,  0x011BB0E0, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		spinZeroVelocityHeuristicTimer = 0.0f;
		uint32_t* spindashTrail = (uint32_t*)((uint32_t)a1 + 0xA0);
		*spindashTrail = 0;

		*spindashTrail = SpawnHomingTrail(a1->GetContext(), false);

		original_ClassicSpinStateStart(a1);
	}
	HOOK(void, __fastcall, _ClassicSpinStateUpdate, 0x011BA930, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		bool doOriginalCode = false;
		if (doOriginalCode)
		{
			original_ClassicSpinStateUpdate(a1);
			return;
		}

		using namespace hh::math;
		using namespace Sonic::Player;

		// Hacky means of getting our Spindash trail and forcing it to be perpetually enabled.
		uint32_t spindashTrailOffset = *(uint32_t*)((uint32_t)a1 + 0xA0);
		*reinterpret_cast<float*>(spindashTrailOffset + 0x18) = 1.0f;

		CPlayerSpeedContext* const context = a1->GetContext();
		if (SonicGeneralStateChangeMethod(context, a1)) return;

		context->StateFlag(eStateFlag_IgnorePadInput) = false;

		const float spinToStandThreshold = context->m_spParameter->Get<float>(ePlayerSpeedParameter_StandToWalkVelocity) * 2;

		if (context->m_Grounded && context->m_HorizontalVelocity.LengthSqr() < spinToStandThreshold * spinToStandThreshold)
		{
			if (spinZeroVelocityHeuristicTimer > (4.0f * (1 / 60.0f)))
			{
				context->ChangeState(ePlayerSpeedState_Squat);
				return;
			}
			spinZeroVelocityHeuristicTimer += a1->GetDeltaTime();
		}
		else
		{
			spinZeroVelocityHeuristicTimer = 0.0f;
		}
		

		if (
			*Input::pressedButtons & Input::Buttons::B ||
			*Input::pressedButtons & Input::Buttons::X
			)
		{
			context->ChangeState(ePlayerSpeedState_Walk);
			return;
		}
	}
	HOOK(void, __fastcall, _ClassicSpinStateEnd,    0x011BAFD0, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		spinZeroVelocityHeuristicTimer = 0.0f;
		uint32_t spindashTrailOffset = *(uint32_t*)((uint32_t)a1 + 0xA0);
		*reinterpret_cast<float*>(spindashTrailOffset + 0x18) = 0.5f;

		original_ClassicSpinStateEnd(a1);
	}

	// TODO: Add to blueblur(?)
	class __declspec(align(4)) CSonicClassicStateSpinCharge : public CPlayerSpeedContext::CStateSpeedBase
	{
	public:
		float m_ChargeTime = 0.0f;
		float m_LaunchSpeed = 0.0f;
		bool hasPlayedSpinChargeSound = false;
	};

	HOOK(void, __fastcall, _ClassicSpinChargeStateUpdate, 0x01250AA0, CSonicClassicStateSpinCharge* a1)
	{
		auto* const context   = a1->GetContext();
		const float deltaTime = a1->GetDeltaTime();

		const bool useOriginalCode = context->m_Is2DMode; // TODO: Add config option? I guess?
		if (useOriginalCode)
		{
			original_ClassicSpinChargeStateUpdate(a1);
			return;
		}

		if (SonicGeneralStateChangeMethod(context, a1))
			return;

		SpindashChargeRotation(a1);

		// Play spindash charge sound only if we held for more than a second pretty much, so spamdash doesn't break your ears.
		// SA1 just stops the sound outright, which idk how to do in gens just yet. We'll see, there has to be a way to do that..
		//if (!a1->hasPlayedSpinChargeSound && a1->m_ChargeTime > context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinChargeTimeLv1))
		if (!a1->hasPlayedSpinChargeSound && a1->m_ChargeTime > 1.0f / 60.0f * 6.0f)
		{
			context->PlaySound(2001048, true);
			a1->hasPlayedSpinChargeSound = true;
		}

		a1->m_ChargeTime += deltaTime;

		// Create secondary spindash smoke / water kickup
		FUNCTION_PTR(void, __thiscall, PlayParticleEffect, 0x00DC2200, CPlayerSpeedContext * This, float deltaTime);
		PlayParticleEffect(context, deltaTime);

		// We don't want to do anything if we're still holding the stick.
		CInputPad* inputPad = context->m_spInputPad.get();
		if (CheckInputHeld(inputPad, 0x17))
			return;

		// temp
		const bool useIngameParameterChargeTime = false;
		const float spinChargeTimeMax = useIngameParameterChargeTime
		                              ? context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinChargeTimeLv2)
		                              : 0.85f;

		// Released button time.
		//const float currentSpeed = context->GetHorizontalVelocity().norm();
		const float trueSpeed = context->GetHorizontalVelocity().norm();
		const float origSpeed = GetValue<float>(a1, 0x6C); // This is the velocity on the first frame of activation, before our heavy decel.
		const float currentSpeed  = std::lerp(origSpeed, trueSpeed, std::inverseLerp(deltaTime * 2.0f, 1.5f * 0.5f, a1->m_Time));
		const float timeLerpValue = std::inverseLerp(0.0f, spinChargeTimeMax, a1->m_Time);


		// Lv0 is so ridiculously low that it's a complete joke. Let's split the difference between lv0 and lv1 for now.
		const float launchSpeed1 = context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinVelocityWithChargeLv0)
		                         + context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinVelocityWithChargeLv1)
		                         * 0.1f
		;
		//const float launchSpeed2 = context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinVelocityWithChargeLv2) * 1.25f;
		const float launchSpeed2 = context->m_spParameter->Get<float>(Sonic::Player::eSonicClassicParameter_SpinVelocityWithChargeLv2) * 0.95f;

		// Instead of a harsh cutoff between states, let's lerp based on hold time. Feels more natural.
		//const float launchAddSpeed = std::lerp(launchSpeed1, launchSpeed2, timeLerpValue * timeLerpValue);
		const float timeLerpValueAdjusted = ((timeLerpValue * timeLerpValue) + timeLerpValue) * 0.5f;
		const float launchAddSpeed = std::lerp(launchSpeed1, launchSpeed2, timeLerpValueAdjusted);

		a1->HoldPropertyFloat("SpinVelocity", currentSpeed + launchAddSpeed); // Mashdash is fun because we *ADD* velocity, so let's do that.
		a1->HoldPropertyBool("SpinFromCharge", true); // hacky stuff sonic team does lol

		context->ChangeState(ePlayerSpeedState_Spin);
	}

	// TODO: Find out what this function ACTUALLY does.
	// This seems to be a method used BOTH to actually apply gravity to sonic when necessary, like when standing still,
	// AND allows for general physics interactions with moveable floors (like if one is moving upward).
	// Could just be a general physics function? Not sure what to call it though, so...
	NOINLINE void DoGravityThing(CPlayerSpeedContext* context, float deltaTime, float multiplier)
	{
		uint32_t func = 0x00E59C30;
		__asm
		{
			mov eax, context
			push multiplier
			push deltaTime
			call func
		}
	}

	HOOK(void, __fastcall, _CPostureStopUpdate, 0x0119DB00, CPlayerSpeedContext::CStateSpeedBase* a1)
	{
		auto context = a1->GetContext();
		if (context->m_Is2DMode)
		//if (true)
		{
			original_CPostureStopUpdate(a1);
			return;
		}

		const float deltaTime = a1->GetDeltaTime();

		// Gotta do this i guess.
		DoGravityThing(context, deltaTime, 0.0);

		//constexpr float spindashDecelerateRate = 80.0f;
		constexpr float spinReductionMultiplier = 6.0f;

		const CVector horizontalVelocity = context->GetHorizontalVelocity();
		const CVector velocityDirection = horizontalVelocity.normalizedSafe();
		const float velocitySpeed = horizontalVelocity.norm();

		int a = eSonicParameter_HurdleReleaseAnimSpeed;
		int b = eSonicClassicParameter_SpinVelocityWithChargeLv0;

		//context->SetHorizontalVelocity_Dirty(velocityDirection * fmax(0.0f, velocitySpeed - spindashDecelerateRate * deltaTime));
		context->SetHorizontalVelocity(velocityDirection * fmax(0.0f, velocitySpeed - (velocitySpeed * spinReductionMultiplier * deltaTime)));

		auto basePosture = GetValue<CPlayerSpeedContext::CStateSpeedBase*>(a1, 0x64);
		Posture3DCommon_MovementRoutine(basePosture);
	}

#pragma endregion

#pragma region SpindashVFX

	// HACK: Fix Spindash water VFX bug
	// For some reason this game thinks you're "on water" if you're ABOVE a water surface. Must be a flaw with their collider size, I don't know.
	// To compensate & fix the error where your spindash dust becomes water if you're not actually *IN* water, we're gonna do a distance check
	// by using the game's own "distance to water surface" calc, but actually USE the distance, projected onto our up vector.

	struct CastInfo
	{
		Eigen::Vector3f m_Normal {};
		Eigen::Vector3f m_Position {};
		Sonic::CRigidBody* m_pBody {};
		void* m_Unk1 {};
		void* m_Unk2 {};
	};

	bool NOINLINE ShapeCollisionGetClosestPoint(Sonic::CShapeCollision* shapeCollision, CastInfo* castInfo, int actorID = 0)
	{
		static constexpr int func = 0x0117E6C0;
		bool result = false;
		__asm
		{
			mov eax, actorID
			mov edi, castInfo
			push shapeCollision
			call func
			mov result, al
		}
		return result;
	}

	void SpinChargeVFXPatch(CSonicClassicContext* context)
	{
		CastInfo castInfo;
		const bool contactSuccess = ShapeCollisionGetClosestPoint(context->m_spShapeCastCollision_Water->m_spShapeCollider.get(), &castInfo);

		if (!contactSuccess)
			return;

		// Run-on-water is disabled in Emerald Coast, but we still want to account for that state.
		if (context->m_GroundDistance > FLT_EPSILON * 8.0f) // Sufficiently small number w/ some give?
			return;

		const CVector position = context->m_spMatrixNode->m_Transform.m_Position;
		const CVector up = context->GetUpDirection();

		float waterDistance = CVector::Dot(position - castInfo.m_Position, up);

		// Honestly don't know why they never stored this information themselves, but...
		if (waterDistance > 0.0f)
			context->StateFlag(eStateFlag_OnWater) = false;
	}

	HOOK(void, __fastcall, _PlaySpinChargeVFX, 0x00DC2200, CSonicClassicContext* context, void*, float deltaTime)
	{
		// I'd rather not asm patch this function, so let's just preserve & override the flag used to play the water VFX for now.
		const bool wasOnWater = context->StateFlag(eStateFlag_OnWater);

		SpinChargeVFXPatch(context);
		original_PlaySpinChargeVFX(context, nullptr, deltaTime);

		context->StateFlag(eStateFlag_OnWater) = wasOnWater;
	}

	// Duplicate code can't entirely be avoided here.
	HOOK(void, __fastcall, _PlaySpinChargeInitialVFX, 0x00DC2440, CSonicClassicContext* context)
	{
		const bool wasOnWater = context->StateFlag(eStateFlag_OnWater);

		SpinChargeVFXPatch(context);
		original_PlaySpinChargeInitialVFX(context);

		context->StateFlag(eStateFlag_OnWater) = wasOnWater;
	}

#pragma endregion
}

void Gameplay_Spindash::Init()
{
	// Change SpinState size from 0xA0 to 0xA4 size.
	WRITE_MEMORY(0x0053350A, uint8_t, 0xA4)

	// Remove "IgnorePadInput" from SpinCharge
	WRITE_NOP(0x01251067, 0x0125106E - 0x01251067)
	// Don't zero out velocity when charging, we're gonna handle that ourselves.
	WRITE_MEMORY(0x01250D31, BYTE, 0xEB)

	// Make it that we ACTUALLY GET A REFERENCE TO THE SPIN TRAIL
	WRITE_JUMP(0x00E5FDE5, SpawnHomingAttackHook_1)
	WRITE_JUMP(0x00E5FEC3, SpawnHomingAttackHook_2)
	WRITE_JUMP(0x00E5FF03, SpawnHomingAttackHook_3)

	// Don't cut velocity when releasing spindash.
	WRITE_JUMP(0x01250D31, 0x01250D4F)

	// Rework "stop" to be a more reasonable deceleration than the nonsense Generations does.
	INSTALL_HOOK(_CPostureStopUpdate)

	INSTALL_HOOK(_ClassicWalkStart)
	INSTALL_HOOK(_ClassicWalkSlideUpdate)
	INSTALL_HOOK(_ClassicWalkUpdate)

	INSTALL_HOOK(_ClassicSpinStateStart)
	INSTALL_HOOK(_ClassicSpinStateEnd)
	INSTALL_HOOK(_ClassicSpinStateUpdate)

	INSTALL_HOOK(_ClassicSpinChargeStateUpdate)

	INSTALL_HOOK(_ClassicSlideUpdate)

	INSTALL_HOOK(InitializeApplicationParams_SPIN)


	// VFX patch
	INSTALL_HOOK(_PlaySpinChargeInitialVFX)
	INSTALL_HOOK(_PlaySpinChargeVFX)
}

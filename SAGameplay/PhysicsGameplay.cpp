// ReSharper disable CppClangTidyModernizeUseAuto
// ReSharper disable CppClangTidyBugproneNarrowingConversions
// ReSharper disable CppInconsistentNaming
#include "PhysicsGameplay.h"
#include <numbers>
#include <Sonic/Player/Parameter/PlayerSpeedParameter.h>
#include "stdExtensions.h"

// Fair warning: This file is a *MESS.*
// It's a combination of iterating on MANY things and treating gameplay with a "change things and see what works" mentality.
// I do not like the code quality of this file, and would like to rewrite it, but I do not have the time.
// Other files have been modified or rewritten to be cleaner to both read and maintain. This is not one of them, clearly.
// Please do *NOT* reference this file for anything other than concepts. I implore you. This is not a holy grail of secrets, it's a madman's race to the finish.

// TODO: Don't leave this here
#include "PhysicsGameplay.h"

#include "Common.h"
#include "Config.h"
#include "Cameras.h"
#include "../DebugDrawText.h"

using namespace Sonic::Player;

// TODO: Make this a configurable field. This code *could* be ported to a game like Lost World in the future, so accounting for the direction of gravity is nice!
static const Hedgehog::Math::CVector GravityDirection = Hedgehog::Math::CVector(0, -1, 0);

namespace Gameplay_Adventure::Core
{
	using namespace Hedgehog::Math;
	using namespace Sonic::Player;
	using namespace Sonic::Message;
	using namespace Cameras;
	typedef CustomCameras::CameraExtensions DefaultCameraExtension;

	SPhysicsEssentials SPhysicsEssentials::Get(CPlayerSpeedContext* context)
	{
		//
		// Custom speed lerp-ing based on factors i.e. default cam or dash paths.
		//--------------------------------------------------------------------

		const bool useGensMaxSpeedParam = false; // todo: make configurable

		constexpr float maxForwardSpeedSA1 = SA1Parameters::max_x_spd * 1.5f; // In the context of gens, 18 as a max speed still kinda sucks.
		                                                                      // TODO: Oh come on I seriously fudged the max speed? Nah that's not okay.
		const float maxForwardSpeedGens = useGensMaxSpeedParam
		                                ? context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_MaxVelocityBasis)
		                                : 50.0f; // Classic has a stupid max velocity if we're being real. Modern's is much nicer.


		auto dashInfo = MsgGetDashModeInfo();
		context->m_pPlayer->SendMessageImm(context->m_pPlayer, dashInfo);

		// HACK: Pointer math instead of data access because these path controllers, depsite being in the SAME PLACE, are only part of CSonic/Classic context ?!?!?!?
		const bool isForwardPath = GetValue<bool>(context, 0x1278);
		const bool isDashPath = dashInfo.m_IsOnPath;

		const float dashPathInfluence = isDashPath ? 1.0f : 0.0f;
		const float camAmount = DefaultCameraExtension::m_DefaultCamAmount + DefaultCameraExtension::m_PathCamAmount;
		const float dashOrDefaultCamAmount = fmax(camAmount, dashPathInfluence);

		// And now we get our max speed for zoomed-in & hallway gameplay.
		const float maxForwardSpeed = std::lerp(maxForwardSpeedSA1, maxForwardSpeedGens, dashOrDefaultCamAmount);

		return { maxForwardSpeed, dashOrDefaultCamAmount, dashInfo };
	}

	CVector GetGuidedInputDirection(CPlayerSpeedContext* context, const CVector& inputDirection, float SpeedZ, const SPhysicsEssentials& essentials, bool aimToVelocity)
	{
		constexpr float rushSpeed = SA1Parameters::rush_speed;
		const auto dashInfo   = essentials.dashInfo;
		const bool isDashPath = dashInfo.m_IsOnPath;

		// Force input direction to be aligned with camera/path forward, if we're going fast enough.
		// It *MIGHT* be worth considering grabbing the function that MAKES forward input, and feeding in modified stick data. Maybe.

		const CVector safeVelocityDirection = context->m_HorizontalVelocity.squaredNorm() > FLT_EPSILON
		                                    ? context->m_HorizontalVelocity
		                                    : context->GetFrontDirection();

		const CVector characterUp = context->m_VerticalRotation.Up();
		const CVector characterFront = CVector::ProjectOnPlane(aimToVelocity ? safeVelocityDirection : context->GetFrontDirection(),
		                                                       characterUp).normalizedSafe();

		const CVector workingForwardDirection = !isDashPath ? DefaultCameraExtension::m_CameraMoveDirection
		                                                    : dashInfo.m_FrontDirection
		                                                             * std::fsign(CVector::Dot(dashInfo.m_FrontDirection, characterFront));

		//const CVector guidedFrontDir = CVector::ProjectOnPlane(workingForwardDirection, characterUp).normalizedSafe();

		const CVector targetInputDir = CVector::ProjectOnPlane(inputDirection,          characterUp).normalizedSafe();
		const CVector guidedInputDir = CVector::ProjectOnPlane(workingForwardDirection, characterUp).normalizedSafe();

		// Need to do this so you don't just roll forward perpetually on walls.
		const CVector targetDirection = CVector::Slerp(guidedInputDir, targetInputDir, inputDirection.norm());

		// Now process that guided input and lerp based on speed.
		// ------------------------------------------------------

		float guideAngleLerp = fmax(0.0f, CVector::Dot(targetDirection, guidedInputDir));
		float speedLerp = std::inverseLerp(rushSpeed, essentials.MaxSpeed, SpeedZ);
		const float cameraGuideLerp = essentials.LerpAmount * guideAngleLerp;

		float amountRestricted;
		if (!isDashPath)
		{
			amountRestricted = cameraGuideLerp * speedLerp;
		}
		else
		{
			const float radiusTValue = std::inverseLerp(1.85f, 10.0f, dashInfo.m_PathRadius);
			amountRestricted = std::lerp(1.0f, cameraGuideLerp, radiusTValue)
			                 * std::lerp(std::lerp(0.5f, 1.0f, speedLerp), speedLerp, radiusTValue);
		}

		//DebugDrawText::log(format("Restriction: %f", finalLerpValue));
		const CVector finalInputDirection = CVector::Slerp(targetDirection, guidedInputDir, amountRestricted);
		return finalInputDirection;
	}
}

namespace Gameplay_Adventure::Physics
{
	typedef hh::math::CVector CVector;
	typedef hh::math::CQuaternion CQuaternion;
	typedef CustomCameras::CameraExtensions DefaultCameraExtension;
	typedef Sonic::Message::MsgGetDashModeInfo MsgGetDashModeInfo;

	// Forward declaring this for ease of use...
	void AdjustAngleAir(CPlayerSpeedContext* context, const CQuaternion& targetRotation, float deltaTime);

	bool IsNearZero(float value)
	{
		const bool result = std::abs(value) < std::numeric_limits<float>::epsilon();
		return result;
	}

	void AdjustAngle(CPlayerSpeedContext* context, const CQuaternion& targetRotation, float deltaTime, bool forceRotation = false)
	{
		// HACK: I need to fix this and just have a bool for airborne in here i guess, I don't know
		if (!context->m_Grounded)
		{
			AdjustAngleAir(context, targetRotation, deltaTime);
			return;
		}

		// Will need these for later 
		const CVector initialVelocity = context->GetHorizontalVelocity();
		const CVector initialVelocityDirection = initialVelocity.normalizedSafe();

		// This lets us slide along walls without everything feeling jank.
		// Note: SA1 actually doesn't support this at all! This is a technique picked up from Generations' code.
		const CVector initialVelocityGroundDirection = initialVelocityDirection.ProjectOnPlane(context->m_FloorAndGrindRailNormal).normalizedSafe();
		const CVector frongGroundDirection       = context->GetFrontDirection().ProjectOnPlane(context->m_FloorAndGrindRailNormal).normalizedSafe();
		const float velocityDot = fmax(CVector::Dot(initialVelocityGroundDirection, frongGroundDirection), 0.0f);

		const float frameDeltaTime = SA1Parameters::TargetFramerate * deltaTime;

		const CQuaternion CurrentRotation = context->m_HorizontalRotation;

		const float deltaAngle = CQuaternion::Angle(CurrentRotation, targetRotation) * RAD2DEG;
		const float angleAdjustment = [deltaAngle]
		{
			if (deltaAngle <= 22.5f)
				return deltaAngle / 8.0f; // Max angle: 2.8125

			if (deltaAngle <= 45.0f)
				return deltaAngle / 4.0f; // Max angle: 11.25

			return 11.25f;
		}();

		CQuaternion finalYawRotation = CQuaternion::RotateTowards(CurrentRotation, targetRotation, angleAdjustment * frameDeltaTime * DEG2RAD);

		const CQuaternion preRotation = context->m_VerticalRotation * context->m_HorizontalRotation;
		CVector speed = preRotation.inverse() * context->GetHorizontalVelocity();
		context->SetYawRotation(finalYawRotation);
		const CQuaternion finalRotation = context->m_VerticalRotation * finalYawRotation;
		const CVector localVelocity = finalRotation.inverse() * initialVelocity;

		if (context->m_Grounded)
		{
			float lerpValue = [context, CurrentRotation]
			{
				// It seems like this isn't even necessary, because technically how we handle interpolation with forward speed is just wrong.
				// Not doing this makes rolling have massive friction down slopes though, so this is what we got for now.
				if (context->m_WorldInput.LengthSqr() < FLT_EPSILON)
					return 0.05f;

				const float result = CVector::Dot(CurrentRotation.Up(), -GravityDirection) <= 0.4f
				                   ? 0.5f
				                   : 0.99f;
				return result;
			}();

			// TODO: Scale lerp value by friction whenever possible.
			float inverseLerpValue = 1.0f - lerpValue;
			speed = CVector::Lerp(speed, localVelocity, inverseLerpValue * frameDeltaTime);
		}
		else
		{
			speed = CVector::Lerp(speed, localVelocity, 0.9f * frameDeltaTime);
		}

		if (forceRotation)
		{
			speed.x() = 0.0f;
			context->SetHorizontalVelocity(finalRotation * speed);
			return;
		}

		context->SetHorizontalVelocity(CVector::Slerp(initialVelocity, finalRotation * speed, velocityDot));
	}

	void AdjustAngleFast(CPlayerSpeedContext* context, const CQuaternion& targetRotation, float deltaTime)
	{
		const float frameDeltaTime = deltaTime * SA1Parameters::TargetFramerate;
		CQuaternion finalRotation = CQuaternion::RotateTowards(context->m_HorizontalRotation, targetRotation, 45.0f * frameDeltaTime * DEG2RAD);

		context->SetYawRotation(finalRotation);
	}

	void AdjustAngleSlow(CPlayerSpeedContext* context, const CQuaternion& targetRotation, float deltaTime, bool forceRotation = false)
	{
		const float frameDeltaTime = deltaTime * SA1Parameters::TargetFramerate;

		// Will need these for later 
		const CVector horizontalVelocity = context->GetHorizontalVelocity();
		const CVector initialVelocityDirection = horizontalVelocity.normalizedSafe();

		// This lets us slide along walls without everything feeling jank.
		const CVector initialVelocityGroundDirection = initialVelocityDirection.ProjectOnPlane(context->m_FloorAndGrindRailNormal).normalizedSafe();
		const CVector frongGroundDirection       = context->GetFrontDirection().ProjectOnPlane(context->m_FloorAndGrindRailNormal).normalizedSafe();
		const float velocityDot = fmax(CVector::Dot(initialVelocityGroundDirection, frongGroundDirection), 0.0f);

		const CQuaternion initialYawRotation = context->m_HorizontalRotation;
		const CQuaternion initialRotation = context->m_VerticalRotation * context->m_HorizontalRotation;

		const CVector initialVelocity = context->GetHorizontalVelocity();
		const float forwardSpeed = (initialRotation.inverse() * initialVelocity).z();

		// This is for making sonic struggle to rotate when going downhill, since rotation would otherwise be insane.
		// TODO: Consider making this more comprehensive. Maybe lean in on Gens' own "rotation force" idea, lerping into that based on slope angle?
		float maxAngleDelta = 1.406250f;
		if (forwardSpeed > SA1Parameters::dash_speed)
			maxAngleDelta += std::sqrt((forwardSpeed - SA1Parameters::dash_speed) * 0.0625f) * maxAngleDelta;

		CQuaternion finalYawRotation = CQuaternion::RotateTowards(initialYawRotation, targetRotation, maxAngleDelta * frameDeltaTime * DEG2RAD);

		const CVector PreviousSpeed = initialRotation.inverse() * context->GetHorizontalVelocity();
		context->SetYawRotation(finalYawRotation);
		const CQuaternion finalRotation = context->m_VerticalRotation * finalYawRotation;
		const CVector newVelocity = finalRotation * PreviousSpeed;

		// TODO: see if I don't have to convert to speed here for whatever reason.
		const CVector initialSpeed = initialRotation.inverse() * initialVelocity;
		const CVector postSpeed    =  finalRotation.inverse()  *   newVelocity;

		// if dot of up & gravity <= 0.4, use 0.5. otherwise...
		CVector newSpeed = CVector::Lerp(postSpeed, initialSpeed, 0.9f * frameDeltaTime);

		if (forceRotation)
		{
			context->SetHorizontalVelocity(finalRotation * newSpeed);
			return;
		}

		const CVector velocity1 = initialVelocity;
		const CVector velocity2 = finalRotation * newSpeed;
		context->SetHorizontalVelocity(CVector::Slerp(velocity1, velocity2, velocityDot));
	}

	void AdjustAngleAir(CPlayerSpeedContext* context, const CQuaternion& targetRotation, float deltaTime)
	{
		// Will need these for later
		const CVector initialVelocity = context->GetHorizontalVelocity();
		const CVector initialVelocityDirection = initialVelocity.normalizedSafe();
		// This lets us slide along walls without everything feeling jank.
		const float velocityDot = fmax(CVector::Dot(initialVelocityDirection, context->m_HorizontalRotation.Forward()), 0.0f);
		const float orientDot = fmax(CVector::Dot(context->m_ModelUpDirection, CVector::Up()), 0.0f);

		const CQuaternion initialYawRotation  = context->m_HorizontalRotation;

		const float frameDeltaTime = SA1Parameters::TargetFramerate * deltaTime;

		const float deltaAngle = CQuaternion::Angle(initialYawRotation, targetRotation) * RAD2DEG;
		const float angleAdjustment = [deltaAngle]
		{
			if (deltaAngle <= 22.5f)
				return deltaAngle / 8.0f; // Max angle: 2.8125

			if (deltaAngle <= 45.0f)
				return deltaAngle / 4.0f; // Max angle: 11.25

			return 11.25f;
		}();

		const CQuaternion finalYawRotation = CQuaternion::RotateTowards(initialYawRotation, targetRotation, angleAdjustment * frameDeltaTime * DEG2RAD);
		context->SetYawRotation(finalYawRotation);

		const float zSpeed = CVector::Lerp(initialYawRotation.inverse() * initialVelocity,
		                                   finalYawRotation.inverse() * initialVelocity,
		                                   0.9f * frameDeltaTime).z();

		const CVector newVelocity = (finalYawRotation * (initialYawRotation.inverse() * initialVelocityDirection)) * zSpeed;

		context->SetHorizontalVelocity(CVector::Lerp(initialVelocity, newVelocity, std::inverseLerp(0.8f, 0.95f, velocityDot) * orientDot));
	}

	// TODO: MAKE GLOBAL PARAMS
	float turnRestrictionMaxSpeed = 100.0f;
	float turnEaseFactor = 5.0f;
	float turnAdjustedByVelocity = 1.0f;

	// Complicated SA1 ground accel function. SA1 decomp done by members of X-Hax, including some of my own research, adapted to Generations by me.
	CVector GetAcceleration(CPlayerSpeedContext* context, float inputMagnitude, const CVector& inputVector, float deltaTime)
	{
		const float frameDeltaTime = deltaTime * SA1Parameters::TargetFramerate;
		const bool haveInput = inputVector.LengthSqr() > FLT_EPSILON;

		// Rotate velocity towards forward direction.
		// Do this before using it in any way with our math.
		// TODO: This makes sense for *general* ground movement, however it can potentially introduce unintended behavior for scenarios like ice/oil slippage.
		// For that reason we need to find a better solution for this if possible.
		{
			const CVector velocity = context->GetHorizontalVelocity();
			const float velocityMag = velocity.norm();
			const float rotAmount = 100.0f * DEG2RADf * deltaTime;
			context->SetHorizontalVelocity(CVector::LerpTowards(velocity, context->GetFrontDirection() * velocityMag, rotAmount));
		}

		const CVector hVel = context->GetHorizontalVelocity();
		CVector Speed(0.0f, 0.0f, hVel.Length()); 

		auto essentials = Core::SPhysicsEssentials::Get(context);
		const CVector inputDirection = GetGuidedInputDirection(context, inputVector.normalizedSafe(), Speed.z(), essentials);

		const float maxForwardSpeed = essentials.MaxSpeed;
		const float camAmount = essentials.LerpAmount;

		CVector localGravity = GravityDirection;

		// pre-check input and immediately set up result.z().
		// HACK: since result is CVector.zero, delta time is applied right after calculations here

		auto setInitialZSpeed = [&]() -> float
		{
			if (!haveInput)
			{
				if (Speed.z() > SA1Parameters::run_speed || Speed.z() < 0.0f)
				{
					return Speed.z() * SA1Parameters::air_resist;
				}
				if (Speed.z() > maxForwardSpeed)
				{
					return (Speed.z() - maxForwardSpeed) * SA1Parameters::air_resist;
				}

				return 0.0f;
			}

			if (Speed.z() <= maxForwardSpeed)
			{
				if (Speed.z() < 0.0f)
				{
					return Speed.z() * SA1Parameters::air_resist;
				}
				if (Speed.z() > maxForwardSpeed)
				{
					return (Speed.z() - maxForwardSpeed) * SA1Parameters::air_resist;
				}

				return 0.0f;
			}

			// This gives us a tapered dropoff as we approach our max speed like in SA1.
			return (Speed.z() - maxForwardSpeed) * SA1Parameters::air_resist * 1.7f;
		};

		CVector outSpeed(0, 0, setInitialZSpeed());

		// "Weight" is factored into characters' acceleration and air resistance, interestingly.
		// TODO: It'd be nice to find a way to trim some of this fat, baking in these calculations and making things less coupled together.
		float gravityAccel = (SA1Parameters::air_resist_y * Speed.y() * frameDeltaTime) + (-SA1Parameters::weight * frameDeltaTime);

		// For the curious, this is not *exactly* what SA1 does, as gravity is calculated into movement here.
		// However, since Generations already applies gravity to the player as a plugin at all times, we just let the game do that instead.
		outSpeed.y() += SA1Parameters::air_resist_y * Speed.y();
		outSpeed.x() += SA1Parameters::air_resist_z * Speed.x(); // recall X and Z are flipped.

		outSpeed *= frameDeltaTime;

		float addForward = 0.0f;

		// Rotation
		//------------------------

		auto RotateCharacter = [&](float diffAngle, float angleThresholdSmall, float angleThresholdLarge, const CQuaternion& targetRotation)
		{
			if (IsNearZero(Speed.z()) && diffAngle > angleThresholdSmall)
			{
				addForward = 0.0f;
				AdjustAngleFast(context, targetRotation, deltaTime);
				return;
			}

			if (Speed.z() >= (SA1Parameters::run_speed + SA1Parameters::jog_speed) * 0.5f && diffAngle > angleThresholdSmall)
			{
				addForward = SA1Parameters::slow_down * frameDeltaTime;
				if (context->m_Grounded)
					AdjustAngle(context, targetRotation, deltaTime);
				else
					AdjustAngleAir(context, targetRotation, deltaTime);
				return;
			}

			if (Speed.z() >= SA1Parameters::jog_speed && diffAngle < angleThresholdSmall)
			{
				AdjustAngleSlow(context, targetRotation, deltaTime);
				return;
			}

			if (Speed.z() >= SA1Parameters::dash_speed)
			{
				AdjustAngleSlow(context, targetRotation, deltaTime);
				return;
			}

			if (SA1Parameters::jog_speed <= Speed.z()  &&
			    Speed.z() <= SA1Parameters::rush_speed &&
			    diffAngle > angleThresholdLarge)
			{
				addForward *= 0.8f;
			}

			if (context->m_Grounded)
				AdjustAngle(context, targetRotation, deltaTime);
			else
				AdjustAngleAir(context, targetRotation, deltaTime);
		};

		if (!haveInput)
		{
			// NOTE: Originally also checked if floor dot product was >= 0.71f
			if (!IsNearZero(Speed.z()))
			{
				addForward = SA1Parameters::slow_down * std::fsign(Speed.z()) * frameDeltaTime;
				
			}
		}
		else
		{
			addForward = [&]
			{
				if (Speed.z() >= maxForwardSpeed)
				{
					const float multiplier = Speed.z() > maxForwardSpeed
					                       ? 0.4f
					                       : 1.0f;

					return SA1Parameters::run_accel * inputMagnitude * multiplier * frameDeltaTime;
				}

				float result = 0.0f;

				if (Speed.z() < SA1Parameters::jog_speed)
				{
					const float jogSpeedScaled = SA1Parameters::jog_speed * 0.4f;
					const float result = (inputMagnitude > 0.5f || Speed.z() < jogSpeedScaled)
					                   ?  SA1Parameters::run_accel * inputMagnitude * frameDeltaTime
					                   :  0.0f;
					return result;
				}

				if (Speed.z() >= SA1Parameters::run_speed)
				{
					const float multiplier = SA1Parameters::rush_speed > Speed.z() && inputMagnitude <= 0.9f
					                       ? 0.3f
					                       : 1.0f;

					return SA1Parameters::run_accel * inputMagnitude * multiplier * frameDeltaTime;
					
				}

				if (inputMagnitude <= 0.7f)
				{
					// SA1 actually fucking multiplies runSpeed here by 2.0 and then 0.5
					if (Speed.z() < SA1Parameters::run_speed)
					{
						result = SA1Parameters::run_accel * inputMagnitude * frameDeltaTime;
					}
				}
				else
				{
					result = SA1Parameters::run_accel * inputMagnitude * frameDeltaTime;
				}

				return result;
			}();

			constexpr float angleThresholdSmall = 22.5f;
			constexpr float angleThresholdLarge = 45.0f;

			// Input scales 
			float inputMagRescaled = std::lerp(inputMagnitude, 1.0f, std::inverseLerp(5, 0, Speed.z()));
			inputMagRescaled = inputMagRescaled * inputMagRescaled * inputMagRescaled;

			const CVector projFwdVector = CVector::ProjectOnPlane(context->GetFrontDirection(), context->m_VerticalRotation.Up()).normalizedSafe();
			const CVector projTrgVector = CVector::ProjectOnPlane(inputDirection, context->m_VerticalRotation.Up()).normalizedSafe();

			const float sign = CVector::Dot(projTrgVector, context->GetRightDirection()) > 0.0
			                 ? -1.0f
			                 : +1.0f;

			const float diffAngleRaw = CVector::Angle(projFwdVector, projTrgVector);
			// Scale how much we're turning based on speed & weather or not we're in default cam mode.
			const float turnLerp = std::inverseLerp(0.0f, turnRestrictionMaxSpeed, Speed.z())
			                       // Only interp if default cam is enabled.
			                       * std::clamp(turnAdjustedByVelocity * camAmount, 0.0f, 1.0f);

			auto info = Common::SCommonInfo::Get(context);
			const float oocLerp = 1.0f - info->m_TimeUntilInControl * Common::SCommonInfo::ms_TValueHelper;

			const float diffAngle   = std::lerp(diffAngleRaw, diffAngleRaw * 0.25f, std::reversePower(turnLerp, turnEaseFactor)) * oocLerp;
			const float targetAngle = diffAngle * sign * inputMagRescaled;

			// FIXME: Wait what the fuck do I calculate this twice?
			const float sign2 = CVector::Dot(context->m_HorizontalRotation.Forward(), CVector::Right()) > 0.0
			                  ? -1.0f
			                  : +1.0f;
			const float originalAngle = CVector::Angle(context->m_HorizontalRotation.Forward(), CVector::Forward()) * sign2;

			RotateCharacter(diffAngle * RAD2DEG,
			                angleThresholdSmall,
			                angleThresholdLarge,
			                CQuaternion::FromAngleAxis(originalAngle + targetAngle, CVector::Up()));
		}


		CVector originalSpeed = outSpeed;
		[&]
		{
			// Odd constant that gets compared a lot here, for some reason this isn't a labeled parameter!
			// Maybe it was a #define? Who knows...
			// Constant scaled from SA1 units-per-frame to units-per-second (x * 0.1 * 60)
			constexpr float UnknownConstant = 0.306f;

			float forwardSpeed = Speed.z();

			// Check if forward speed is near zero.
			// Todo: Don't run this codepath at all, or migrate what I did earlier here, because something just isn't working lol
			if (IsNearZero(forwardSpeed))
			{
				outSpeed.z() = originalSpeed.z() + addForward;
				const float gravityAcceleration = SA1Parameters::lim_frict * originalSpeed.y();

				if (!haveInput && (outSpeed.z() < fabs(gravityAcceleration) || fabs(outSpeed.z()) < UnknownConstant))
				{
					// HACK: prevents gravity from making sonic oscillate between moving and standing
					// FIXME: no it doesn't LMAO
					localGravity.z() = 0.0f;
					outSpeed.z() = 0.0f;
				}
				return;
			}

			if (addForward < 0.0f)
			{
				// This stuff all looks suspiciously like FMax checks...
				if (forwardSpeed > 0.0f)
				{
					const float sign = std::fsign(addForward);
					float nrz = originalSpeed.z() + addForward;

					if (nrz * sign < 0.0f)
					{
						nrz = 0.0f;
					}

					outSpeed.z() = nrz;
					return;
				}

				float newResultZ = 0.0f;

				bool condition = !haveInput &&
				                 forwardSpeed <= SA1Parameters::jog_speed &&
				                 fabs(originalSpeed.z()) < UnknownConstant;

				if (!condition)
				{
					newResultZ = (SA1Parameters::lim_frict * gravityAccel) + addForward;
				}

				if (condition || 0.0f <= newResultZ)
				{
					newResultZ = 0.0f;
				}

				outSpeed.z() = newResultZ;
				return;
			}

			if (forwardSpeed < 0.0f && fabs(originalSpeed.z()) < UnknownConstant)
			{
				outSpeed.z() = fmax(originalSpeed.z() + addForward, 0.0f);
				return;
			}

			const float gravityAcceleration = SA1Parameters::lim_frict * gravityAccel;

			if (addForward <= gravityAcceleration || gravityAcceleration <= 0.0f)
			{
				if (haveInput)
				{
					outSpeed.z() = originalSpeed.z() + addForward;
					return;
				}

				if (forwardSpeed < SA1Parameters::jog_speed && fabs(originalSpeed.z()) < UnknownConstant)
				{
					outSpeed.z() = 0.0f;
					return;
				}
			}

			outSpeed.z() = fmin(originalSpeed.z() + addForward, gravityAcceleration);
		}();

		// Lateral acceleration
		outSpeed.x() = [&Speed, originalSpeed](float originalAccelX) -> float
		{
			if (IsNearZero(Speed.x()))
			{
				const float result = fabs(originalSpeed.x()) < fabs(SA1Parameters::lim_frict * originalSpeed.y())
				                   ? 0.0f
				                   : originalAccelX;

				return result;
			}

			float setResultX_Pre = 0.0f;
			float setResultX = 0.0f;

			if (originalSpeed.y() < 0.0f)
			{
				setResultX = SA1Parameters::grd_frict_z * originalSpeed.y();
			}

			if (originalSpeed.x() <= 0.0f)
			{
				if (originalSpeed.x() < 0.0f)
				{
					setResultX_Pre = setResultX;
				}
			}
			else
			{
				setResultX_Pre = -setResultX;
			}

			setResultX = originalSpeed.x() + setResultX_Pre;

			if (!IsNearZero(originalSpeed.x()) && !IsNearZero(setResultX_Pre) && setResultX * originalSpeed.x() < 0.0f)
			{
				setResultX = 0.0f;
			}

			return setResultX;
		}
		(outSpeed.x());

		localGravity = CVector::Normalized(localGravity);
		localGravity.x() = 0.0f;
		localGravity.y() = 0.0f;

		// TODO: determine the right metric to use - maybe using weight would be better? very strong though!
		outSpeed += localGravity * (SA1Parameters::run_accel * frameDeltaTime);

		// HACK: Patch vertical acceleration so it's nulled out when grounded. This fixes some... odd behaviors, like sliding backward.
		outSpeed.y() = 0.0f;

		return outSpeed;
	}

#pragma region FUNCTION_PTRS
	FUNCTION_PTR(void, _stdcall, fpSonicRotationAdvance, 0x00E310A0,
		CPlayerSpeedContext::CStateSpeedBase* state, Hedgehog::Math::CVector* targetDirection, float turnRate1,
		float turnRateMultiplier, bool doSteerSonicWithVelocity, float turnRate2);

	FUNCTION_PTR(void*, __stdcall, fpSonicAcceleration, 0x00E319F0, CPlayerSpeedContext::CStateSpeedBase* state,
		hh::math::CVector* horizAccel, const hh::math::CVector& inputDir, float lowSpeedMax, bool isHighSpeed,
		float accelForceMultiplier, float decelForceMultiplier);
#pragma endregion

	void* __stdcall CustomSonicAcceleration(CPlayerSpeedContext::CStateSpeedBase* state,
		hh::math::CVector* horizAccel, const hh::math::CVector& inputDir, float lowSpeedMax, bool isHighSpeed,
		float accelForceMultiplier, float decelForceMultiplier)
	{
		using namespace hh::math;
		auto context = state->GetContext();
		const float deltaTime = state->GetDeltaTime();

		const bool noCustomAccel = context->StateFlag(eStateFlag_OutOfControl)
		                        || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                        || context->StateFlag(eStateFlag_AirOutOfControl)
		                        || context->StateFlag(eStateFlag_InvokeSkateBoard)
		                        || context->StateFlag(eStateFlag_Squat)
		;

		if (*(uint32_t*)context == idModernContext || noCustomAccel)
			return fpSonicAcceleration(state, horizAccel, inputDir, lowSpeedMax, isHighSpeed, accelForceMultiplier, decelForceMultiplier);

		// We need to do some things with this after-the-fact for a few reasons. Keep this here now.
		float addSpeed = GetAcceleration(context, inputDir.Length(), inputDir, deltaTime).z();
		//float addSpeed = 0.0f;

		// Post Processing
		//----------------------------

		// Handle case where sonic just jitters or some shit at low speed.
		// Eventually use parameter for min speed & start using input deadzone instead of FLT_EPSILON
		const float velLengthSqr = context->GetHorizontalVelocity().LengthSqr();

		if (inputDir.LengthSqr() < (FLT_EPSILON * 2.0f) && velLengthSqr < 1.0f)
		{
			context->m_HorizontalVelocity *= 0.9f;
			*horizAccel = CVector::Zero();
			return horizAccel;
		}

		// Handle deceleration.
		if (addSpeed < 0.0f)
		{
			*horizAccel = context->GetHorizontalVelocityDirection() * (addSpeed / deltaTime);
			return horizAccel;
		}

		// HACK: Make sonic accelerate faster on startup to mimic SA1's single frame of extra torque.
		const float addSpeedMultiplier = std::lerp(2.0f, 1.0f, std::inverseLerp(2.0f, 5.0f, context->m_HorizontalVelocity.norm()));
		addSpeed *= addSpeedMultiplier;

		// Fight against gravity walking up slopes.
		const float slopeDot = fmax(0.0f, CVector::Dot(context->GetHorizontalVelocityDirection(), CVector::Up()));
		const float gravityForce = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_Gravity) * slopeDot * 0.5f;

		const CVector groundedFrontDirection = CVector::ProjectOnPlane(context->GetFrontDirection(), context->m_FloorAndGrindRailNormal).normalizedSafe();
		*horizAccel = groundedFrontDirection * ((addSpeed / deltaTime) + gravityForce);
		return horizAccel;
	}

	void* __stdcall CustomAccelerationAir(CPlayerSpeedContext::CStateSpeedBase* state,
		hh::math::CVector* horizAccel, const hh::math::CVector& inputDir, float lowSpeedMax, bool isHighSpeed,
		float accelForceMultiplier, float decelForceMultiplier)
	{
		using namespace hh::math;
		auto* const context = state->GetContext();
		auto* const info = Common::SCommonInfo::Get(context);

		const bool noCustomAccel = context->StateFlag(eStateFlag_OutOfControl)
		                        || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                        || context->StateFlag(eStateFlag_AirOutOfControl)
		                        || context->StateFlag(eStateFlag_InvokeSkateBoard)
		                        || context->StateFlag(eStateFlag_Squat)
		;

		// Bail if modern sonic or w/e
		if (*(uint32_t*)context == idModernContext || noCustomAccel)
			return fpSonicAcceleration(state, horizAccel, inputDir, lowSpeedMax, isHighSpeed, accelForceMultiplier, decelForceMultiplier);

		// Hack fix to do what SA1 does during the homing attack state, because of course.
		const bool isHomingAttack = *(int*)context->m_pPlayer->m_StateMachine.GetCurrentState().get() == 0x016D9A84;
		const CVector homingInputVector = context->m_WorldInput.LengthSqr() > FLT_EPSILON
		                                ? context->m_WorldInput.normalizedSafe()
		                                : context->GetFrontDirection();

		if (isHomingAttack)
			return CustomSonicAcceleration(state, horizAccel, homingInputVector, lowSpeedMax, isHighSpeed, accelForceMultiplier, decelForceMultiplier);

		const float inputMagnitude = context->m_WorldInput.Length();
		const float deltaTime = state->GetDeltaTime();
		const bool stickHeld = inputMagnitude > FLT_EPSILON;

		// Rotate velocity towards forward direction
		{
			const CVector velocity = context->GetHorizontalVelocity();
			const float velocityMag = velocity.norm();
			const float rotAmount = 100.0f * DEG2RADf * deltaTime * context->m_WorldInput.norm();
			context->SetHorizontalVelocity(CVector::LerpTowards(velocity, context->m_HorizontalRotation.Forward() * velocityMag, rotAmount));
		}

		if (context->m_VelocityChanged)
			context->HandleVelocityChanged();

		const CVector localVelocity
		(
			CVector::Dot(context->GetRightDirection(), context->m_HorizontalVelocity),
			CVector::Dot(context->GetUpDirection(), context->m_VerticalVelocity),
			CVector::Dot(context->GetFrontDirection(), context->m_HorizontalVelocity)
		);

		const float forwardLerpValue = inputMagnitude > FLT_EPSILON
		                             ? fmax(CVector::Dot(inputDir.normalizedSafe(), context->GetFrontDirection()), 0.0f)
		                             : 1.0f;

		// HACK: Make Air Resistance less powerful when holding forward if we're doing Gens compatible physics.
		const float airResistForce = Config::ms_ReduceAirFriction
		                           ? std::lerp(SA1Parameters::air_resist_air, SA1Parameters::air_resist_air * 0.5f,
		                             std::inverseLerp(0.8f, 0.9f, forwardLerpValue))
		                           : SA1Parameters::air_resist_air;

		if (!stickHeld)
		{
			if (context->GetHorizontalVelocity().SqrMagnitude() < FLT_EPSILON)
			{
				*horizAccel = CVector::Zero();
				return horizAccel;
			}
			
			const CVector direction = context->GetHorizontalVelocityDirection();

			// This is a force that, in SA1, is subtracted per frame. We need this to be per-second, so *= 60.0f.
			// Do note: this might... not work at all at high frame rates. Need to verify that. The solution might be 60 * 60 * deltaTime.
			const float accelAmount = -fabs(localVelocity.z() * (airResistForce * SA1Parameters::TargetFramerate));

			*horizAccel = direction * accelAmount;
			return horizAccel;
		}

		const CVector airResistance(SA1Parameters::air_resist_z,
		                            SA1Parameters::air_resist_y,
		                            airResistForce);

		CVector addSpeed = CVector::Scale(localVelocity, airResistance);


		// Air rotation is actually really simple as it turns out.

		// We don't want Sonic to have full rotation precision the instant outofcontrol time is over.
		// TODO: Would be nice to make the act of jumping cancel out this blend, mainly when we jump out of dash path outofcontrol *specifically.*
		const float inControlInfluence = 1.0f - info->m_TimeUntilInControl * Common::SCommonInfo::ms_TValueHelper;
		const float camAmount = DefaultCameraExtension::m_DefaultCamAmount + DefaultCameraExtension::m_PathCamAmount;
		
		const CVector inputDirection = CVector::ProjectOnPlane(context->m_WorldInput.squaredNorm() > FLT_EPSILON
		                                                           ? context->m_WorldInput.normalizedSafe()
		                                                           : context->m_HorizontalRotation.Forward(),
		                                                       CVector::Up()).normalizedSafe();
		const CQuaternion currentRotation = context->m_HorizontalRotation;

		// Scale how much we're turning based on speed & weather or not we're in default cam mode.
		const float turnLerp = std::inverseLerp(0.0f, turnRestrictionMaxSpeed, localVelocity.z())
		                       // Only interp if default cam is enabled.
		                       * std::clamp(turnAdjustedByVelocity * camAmount, 0.0f, 1.0f);

		const float defaultCamMultipleir = std::lerp(1.0f, 0.25f, std::reversePower(turnLerp, turnEaseFactor));
		const float angleLerp = inControlInfluence * defaultCamMultipleir;

		const CQuaternion inputRotation = 
		    CQuaternion::LookRotation(CVector::Slerp(context->m_HorizontalRotation.Forward(), inputDirection, angleLerp),
		                              CVector::Up());

		// Now for the hard part, velocity & speed dampening.
		const float angleDeltaAbs = fabs(CVector::Angle(currentRotation.Forward(), inputDirection)) * RAD2DEG;

		AdjustAngleAir(context, inputRotation, deltaTime);

		auto forwardAdd = [&]() -> float
		{
			const float upwardForceMultiplier = localVelocity.y() >= 0.0f
			                                  ? 1.0f
			                                  : 2.0f;

			if (localVelocity.z() > SA1Parameters::run_speed && angleDeltaAbs > 135.0f)
				return SA1Parameters::air_break * inputMagnitude;

			if (angleDeltaAbs <= 22.5f)
				return SA1Parameters::air_accel * inputMagnitude * upwardForceMultiplier;

			return 0.0f;
		};
		addSpeed.z() += forwardAdd();

		// This is a force that, in SA1, is subtracted per frame. We need this to be per-second, so *= 60.0f.
		// Do note: this might... not work at all at high frame rates. Need to verify that. The solution might be 60 * 60 * deltaTime.
		*horizAccel = context->GetFrontDirection() * (addSpeed.z() * SA1Parameters::TargetFramerate);
		return horizAccel;
	}


	// Rotation is tightly coupled with the ground movement code.
	// I tried decoupling it, but it requires more rnd of the original code to properly sparate the two while staying "accurate."
	// Because of that, for now, just stub out gens' rotation method when appropriate.
	void __stdcall CustomRotation(CPlayerSpeedContext::CStateSpeedBase* state, Hedgehog::Math::CVector* targetDirection, float turnRate1,
		float turnRateMultiplier, bool doSteerSonicWithVelocity, float turnRate2)
	{
		auto context = state->GetContext();
		const bool noCustomAccel = context->StateFlag(eStateFlag_OutOfControl)
		                        || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                        || context->StateFlag(eStateFlag_AirOutOfControl)
		                        || context->StateFlag(eStateFlag_InvokeSkateBoard)
		                        || context->StateFlag(eStateFlag_Squat)
		;

		if (*(int*)context != idClassicContext || noCustomAccel)
		{
			fpSonicRotationAdvance(state, targetDirection, turnRate1, turnRateMultiplier, doSteerSonicWithVelocity, turnRate2);
		}
	}

	void __stdcall CustomRotationAir(CPlayerSpeedContext::CStateSpeedBase* state, Hedgehog::Math::CVector* targetDirection, float turnRate1,
		float turnRateMultiplier, bool doSteerSonicWithVelocity, float turnRate2)
	{
		auto* const context = state->GetContext();
		const bool noCustomAccel = context->StateFlag(eStateFlag_OutOfControl)
		                        || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                        || context->StateFlag(eStateFlag_AirOutOfControl)
		                        || context->StateFlag(eStateFlag_InvokeSkateBoard)
		                        || context->StateFlag(eStateFlag_Squat)
		;

		if (*reinterpret_cast<uint32_t*>(context) != idClassicContext || noCustomAccel)
		{
			fpSonicRotationAdvance(state, targetDirection, turnRate1, turnRateMultiplier, doSteerSonicWithVelocity, turnRate2);
		}
	}

	// ! ------  MIGRATE THIS ALL SOMEWHERE ELSE
	////////////////////////////////////////////////////

	// Air break
	bool __stdcall SonicDoAirBreak(CPlayerSpeedContext* context)
	{
		FUNCTION_PTR(bool, __stdcall, AirBreakOriginal, 0x00E55330, CPlayerSpeedContext* _context);

		// Only Modern Sonic air-break's for now.
		// Eventually this might just get disabled completely since air-break doesn't really fit with SA1,
		// though that might change depending on how dash paths and auto run is handled.
		const bool outOfControl = context->StateFlag(eStateFlag_OutOfControl)
		                       || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                       || context->StateFlag(eStateFlag_AirOutOfControl);
		const bool noAirBreak = *(uint32_t*)context == idClassicContext
		                         && !outOfControl
		                         && !context->m_Is2DMode;

		if (!noAirBreak)
			return AirBreakOriginal(context);

		return false;
	}

	// Jump cancel implementation
	bool SonicDoJumpCancel(CPlayerSpeedContext* context, const Sonic::SPadState* input)
	{
		if (!input->IsTapped(Sonic::eKeyState_X))
			return false;

		context->SetHorizontalVelocity(CVector::Zero());
		context->StateFlag(eStateFlag_EnableHomingAttack) = false;
		context->ChangeState("Fall");
		return true;
	}


	// Remove airdrag behavior when in SA1 mode, because we handle that on our own as opposed to Gens' plugin.
	HOOK(void, __fastcall, _PluginAirDrag, 0x0119D650, CPlayerSpeedContext::CStateSpeedBase* state)
	{
		using namespace hh::math;

		auto context = state->GetContext();

		if (*(uint32_t*)context == idModernContext)
		{
			original_PluginAirDrag(state);
			return;
		}

		const bool outOfControl = context->StateFlag(eStateFlag_OutOfControl)
		                       || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                       || context->StateFlag(eStateFlag_AirOutOfControl);

		const bool shouldAirDrag = outOfControl
		                        || context->m_Is2DMode;

		auto info = Common::SCommonInfo::Get(context);
		const float lerpValue = info->m_TimeUntilInControl * Common::SCommonInfo::ms_TValueHelper;

		// TODO: Implement a cooldown timer. See if Generations already has one?
		if (!shouldAirDrag)
			return;

		const CVector hVelocityA = context->GetHorizontalVelocity();

		original_PluginAirDrag(state);

		const CVector hVelocityB = context->GetHorizontalVelocity();
		context->SetHorizontalVelocity(CVector::Lerp(hVelocityA, hVelocityB, lerpValue));
	}

	// Jump rising
	HOOK(void, __fastcall, _ClassicJumpBallUpdate, 0x01114FD0, CPlayerSpeedContext::CStateSpeedBase* state)
	{
		auto context = state->GetContext();
		// No influencing 2D mode... for now.
		if (context->m_Is2DMode)
		{
			original_ClassicJumpBallUpdate(state);
			return;
		}

		const Sonic::SPadState* input = &Sonic::CInputState::GetInstance()->GetPadState();

		constexpr float ContinuousJumpMultiplier = 0.8f;
		constexpr float hangTimeMax = 1.0f;

		const float frameDeltaTime = state->GetDeltaTime() * SA1Parameters::TargetFramerate;

		// Jumping has extra air friction on the vertical axis.
		if (context->m_VelocityChanged)
			context->HandleVelocityChanged();
		context->m_VerticalVelocity += context->GetUpDirection()
		                            * (context->m_VerticalVelocity.Length() * SA1Parameters::air_resist_y * frameDeltaTime);
		context->SetVelocity(context->m_HorizontalVelocity + context->m_VerticalVelocity);

		if (input->IsDown(Sonic::eKeyState_A) && context->m_TimeAirborne < hangTimeMax)
		{
			//context->AddVelocity(CVector::Up() * (jmpAddit * ContinuousJumpMultiplier * state->GetDeltaTime()));
			//context->AddVelocity(CVector::Up() * (jmp_addit * ContinuousJumpMultiplier * state->GetDeltaTime()));
			context->AddVelocity(CVector::Up() * (SA1Parameters::jmp_addit * ContinuousJumpMultiplier * frameDeltaTime));
		}

		SonicDoJumpCancel(context, input);
		original_ClassicJumpBallUpdate(state);
	}

	HOOK(void, __cdecl, _SonicClassicAnimTree, 0x01281D50)
	{
		original_SonicClassicAnimTree();

		// Fall speed playback
		*(float*)0x1A50EB8 = 1.0f;
		*(float*)0x1A50EE8 = 1.0f;
	}

	// Fix angle threshold nonsense
	static constexpr uint32_t patch_start_addr = 0x00E36692;
	ASMHOOK BrakeAngleThresholdPatch()
	{
		static constexpr uint32_t patch_call_addr  = 0x009BF650;
		static constexpr uint32_t patch_jump_addr  = 0x00E36699;

		//static double angleThreshold = -(45.0 / 180.0);
		static constexpr double angleThreshold = 0.8;
		__asm
		{
			call patch_call_addr
			fld ds : angleThreshold
			jmp patch_jump_addr
		}
	}

	// HACK: Using this garbage to get deltaTime information.
	// There's probably a MUCH better way to do this, but I wrote this over a year ago and can't be bothered to find a better way right now.
	HOOK(void, __stdcall, _SonicMovementRoutine, 0x00E32180, CPlayerSpeedContext::CStateSpeedBase* This, Hedgehog::Math::CVector* Velocity)
	{
		auto* context = This->GetContext();
		auto* const commonInfo = Common::SCommonInfo::Get(context);
		commonInfo->m_PhysicsDeltaTime = This->GetDeltaTime();

		original_SonicMovementRoutine(This, Velocity);
	}

	void Init()
	{
		INSTALL_HOOK(_SonicMovementRoutine)

		// Classic fall state anim
		WRITE_MEMORY(0x01282860, char*, "sc_fall_loop")
		WRITE_MEMORY(0x012828C7, char*, "sc_fall_loop")
		INSTALL_HOOK(_SonicClassicAnimTree)
		INSTALL_HOOK(_PluginAirDrag)

		// Jump modification
		INSTALL_HOOK(_ClassicJumpBallUpdate)

		// Milktose attempt at making sonic's animations look nicer at low speed.
		static float NewPlaybackSpeed = 20.0f;
		static float NewPlaybackSpeed2 = 2.5f;
		WRITE_MEMORY(0x01281E06, float*, &NewPlaybackSpeed2)
		//WRITE_MEMORY(0x01281E75, float*, &NewPlaybackSpeed2)

		// Patch out this, frankly, abysmal shortbreak speed threshold (was 0.1 for some godforsaken reason)
		static double shortBrakeThreshold = 2.0;
		WRITE_MEMORY(0x00E36665, double*, &shortBrakeThreshold);

		// Similarly, let's make the angle threshold something more sensible than 90 degrees.
		// This unfortunately has to be done in assembly.
		WRITE_JUMP(patch_start_addr, BrakeAngleThresholdPatch);
	}
}

// Collision stuff is its WHOLE INDEPENDENT BEAST. It's VERY insane.
namespace Gameplay_Adventure::Collision
{
	using namespace hh::math;

	// Collision bullshit
	constexpr int COLID_SLOPE = 0x202D;
	constexpr int COLID_FLOOR = 0x202E;
	constexpr int COLID_WALL  = 0x202F;

	HOOK(void, __cdecl, _RigidBodyApplyProperties, 0x0117EF80, boost::shared_ptr<Sonic::CRigidBody>* spRigidBody, Hedgehog::Base::CSharedString& name)
	{
		original_RigidBodyApplyProperties(spRigidBody, name);

		// Now our own stuff

		Sonic::CRigidBody* rigidBody = spRigidBody->get();

		rigidBody->AddBoolProperty(JStrHash("sa_slope"), COLID_SLOPE);
		rigidBody->AddBoolProperty(JStrHash("sa_floor"), COLID_FLOOR);
		rigidBody->AddBoolProperty(JStrHash("sa_wall"),  COLID_WALL);
	}

	static constexpr float FLOOR_DOT_THRESHOLD = 0.001f;

	// Raycast function that gets surface information, wish I didn't have to use this but whatever...
	bool NOINLINE CheckSurfaceHasID(CPlayerSpeedContext* player, const CVector& start, const CVector& end, int mask)
	{
		static constexpr uint32_t func = 0x00E5F7E0;
		bool result = false;
		__asm
		{
			mov eax, start
			mov ecx, end
			push mask
			push player

			call func
			mov result, al
		}
		return result;
	}

	struct RayHitData
	{
		Eigen::Vector3f Normal{};
		Eigen::Vector3f Position{};
		Sonic::CRigidBody* pRigidBody{};
		float field_1C = 0;
		int field_20 = 0;
		bool hitSuccess = false;
	};
	FUNCTION_PTR(bool, __stdcall, RaycastFindFloor, 0x00E58140, CPlayerSpeedContext* _This, RayHitData* a2, const CVector& start, const CVector& end);

	// Surface check re-implementation & SA specific stuff
	bool CheckIfCanLandOnSurface_Common(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		if (context->StateFlag(eStateFlag_WallWalkJump))
			return true;

		const bool airborne = !context->m_Grounded;
		const double add    = airborne ? 0 : 2.5;

		const double landAngle = (static_cast<double>(context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_LandEnableMaxSlope)) + add) * DEG2RAD;

		return upVector.dot(surfaceNormal) >= cos(landAngle);
	}

	bool __cdecl CheckIfCanLandOnSurface(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		if (context->StateFlag(eStateFlag_WallWalkJump))
			return true;


		// Manual raycast search.
		// UNDONE: I DUNNO MAN ITS NOT REALLY WORKING
#if 0
		constexpr float searchDistance = 0.125f;

		RayHitData data;

		const auto* commonInfo = Common::SCommonInfo::Get(context);
		const float deltaTime = commonInfo->m_PhysicsDeltaTime;

		const CVector velocity = context->GetVelocity();
		const CVector moveDirection = velocity.normalizedSafe();
		const float frameDistance = velocity.norm() * deltaTime;

		const CVector position = context->m_spMatrixNode->m_Transform.m_Position;
		const CVector nextPosition = position + (moveDirection * (frameDistance + searchDistance));
		const CVector up = context->GetUpDirection();
		const CVector center = position + up * 0.5f;

		const CVector centerToMoveVector = nextPosition - center;

		const CVector start = center; // We want to start from the center for this cast, I THINK.
		const CVector end = centerToMoveVector.normalizedSafe() * (centerToMoveVector.Length() + searchDistance) + center;

		if (RaycastFindFloor(context, &data, start, end))
		{
			Sonic::CRigidBody* rigidBody = data.pRigidBody;

			if (rigidBody->GetBoolProperty(COLID_FLOOR) && CVector::Dot(context->GetVelocity().normalizedSafe(), upVector) > FLOOR_DOT_THRESHOLD)
				return false;
			if (rigidBody->GetBoolProperty(COLID_WALL))
				return false;
			if (rigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}
#endif

		// Check surface contact stuff
		// HACK: Checks VFTable entry to verify data isn't corrupted, probably a BAD IDEA?

		const uint32_t in_type = context->m_ResolveGroundNormalCompleteResultType;
		CPlayerSpeedContext::SGroundSearch* const pGroundSearch = &context->m_aGroundSearch[in_type];

#ifdef _DEBUG
		if (!pGroundSearch->isContact)
		//	DebugDrawText::log("NO CONTACT", 10.0f, 0, {1, .2, .2, 1});
			DebugDrawText::log("NO CONTACT", 0.0f, 0, {1, .2, .2, 1});
#endif

		Sonic::CRigidBody* const rigidBody = pGroundSearch->pRigidBody;

		auto VerifyRigidBody = [&rigidBody]() -> bool
		{
			if (!rigidBody)
				return false;

			int vtblID = *(int*)rigidBody;
			const bool isValid = vtblID == 0x016A18A0 // Sonic::CRigidBody
			                     || vtblID == 0x016D0750 // Sonic::CPathRigidBodyCollision
				;

			// The following likely happens because a setobject was your last contact rigidbody and was destructed, but not yet freed.
			if (vtblID == 0x016A04D8) // Sonic::CPhysicsUnit
			{
				DebugDrawText::log("WARNING!! Contact rigidbody is UNINITIALIZED!", 10.0f, 0, {1,0,0,1});
				return false;
			}

			// This one feels like a bad static cast situation, somehow the "contact" rigidbody is the event collision?
			if (vtblID == 0x016A03B4) // Sonic::CEventCollision
			{
				DebugDrawText::log("WARNING!! Contact is EVENT COLLISION!", 10.0f, 0, {1,0.3,0,1});
				return false;
			}

#if _DEBUG
			if (!isValid)
				MessageBoxA(nullptr, "Please attach a debugger and verify what caused this!", "ERROR: CONTACT RIGIDBODY IS INVALID!", MB_OK | MB_ICONERROR);
#endif

			return isValid;
		};

		if (pGroundSearch->isContact && VerifyRigidBody())
		{
			if (rigidBody->GetBoolProperty(COLID_FLOOR) && CVector::Dot(context->GetVelocity().normalizedSafe(), upVector) > FLOOR_DOT_THRESHOLD)
				return false;
			if (rigidBody->GetBoolProperty(COLID_WALL))
				return false;
			if (rigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}

		return CheckIfCanLandOnSurface_Common(context, upVector, surfaceNormal);
	}

	// Same deal with "Can I walk on this?" logic.
	bool CheckIfCanWalkOnSurface_Common(CPlayerSpeedContext* context, const CVector& lhs, const CVector& rhs, bool useInnerAngle)
	{
		const EPlayerSpeedParameter angleType = useInnerAngle
		                                      ? ePlayerSpeedParameter_MoveEnableGroundAngleInner
		                                      : ePlayerSpeedParameter_MoveEnableGroundAngleOuter;

		const float multiplier  = context->StateFlag(eStateFlag_InvokePtmSpike) ? 2.0f : 1.0f;
		const float groundAngle = context->m_spParameter->Get<float>(angleType) * multiplier * DEG2RAD;

		return CVector::Dot(lhs, rhs) >= cosf(groundAngle);
	}

	bool __cdecl CheckIfCanWalkOnSurface(CPlayerSpeedContext* context, const CVector& lhs, const CVector& rhs, bool useInnerAngle)
	{
		// HACK: Check here if we're gonna detatch from a wall, then pop off of it SA1 style.
		// NOTE: It... doesn't super work that well, but we can at least try.
		if (context->GetHorizontalVelocity().squaredNorm() < 3.0f * 3.0f
			&& (CVector::Angle(context->m_FloorAndGrindRailNormal, CVector::Up()) > 45.0f * DEG2RADf))
		{
			//context->ChangeState("Fall");
			context->AddVelocity(context->m_FloorAndGrindRailNormal * 60.0f); // This just happens to be 60, has nothing to do with 60fps lol
			return false;
		}

		// Check surface contact stuff
		RayHitData data;

		const auto* commonInfo = Common::SCommonInfo::Get(context);
		const float deltaTime  = commonInfo->m_PhysicsDeltaTime;
		const CVector position = context->m_spMatrixNode->m_Transform.m_Position + context->GetHorizontalVelocity() * deltaTime;
		//const CVector up = context->GetUpDirection();
		const CVector up = context->m_FloorAndGrindRailNormal;

		const CVector start = position + up * 1.0f; // Todo: see if I want to cast from further above sonic's head...
		const CVector end   = position - up * 1.125f;

		if (true)
		{
			if (RaycastFindFloor(context, &data, start, end))
			{
				if  (data.pRigidBody->GetBoolProperty(COLID_WALL) ||
					(data.pRigidBody->GetBoolProperty(COLID_FLOOR) && context->GetFrontDirection().dot(lhs) > FLOOR_DOT_THRESHOLD))
						return false;

				if (data.pRigidBody->GetBoolProperty(COLID_SLOPE)
					//&& !(CVector::Angle(context->m_FloorAndGrindRailNormal, CVector::Up()) > 45.0f) && 
					)
					return true;
			}
		}

#if 0
		const uint32_t in_type = context->m_ResolveGroundNormalCompleteResultType;
		Sonic::CRigidBody* const rigidBody = context->m_aGroundSearch[in_type].pRigidBody;
		if (rigidBody)
		{
			if ( rigidBody->GetBoolProperty(COLID_WALL) ||
			     (rigidBody->GetBoolProperty(COLID_FLOOR) && context->GetFrontDirection().dot(lhs) > FLOOR_DOT_THRESHOLD))
				return false;

			if (rigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}
#endif

		return CheckIfCanWalkOnSurface_Common(context, lhs, rhs, useInnerAngle);
	}

	// Usercall hook
	static ASMHOOK CheckIfCanLandOnSurface_asm()
	{
		__asm
		{
			push[esp + 08h] // a3
			push[esp + 08h] // a2
			push eax // a1

			call CheckIfCanLandOnSurface

			add esp, 4 // a1<eax> is also used for return value
			add esp, 4 // a2
			add esp, 4 // a3
			retn 08h
		}
	}
	static ASMHOOK CheckIfCanWalkOnSurface_asm()
	{
		__asm
		{
			push[esp + 0Ch] // useInnerAngle
			push[esp + 0Ch] // rhs
			push[esp + 0Ch] // lhs
			push esi // a1

			call CheckIfCanWalkOnSurface

			pop esi // a1
			add esp, 4 // lhs
			add esp, 4 // rhs
			add esp, 4 // useInnerAngle
			retn 0Ch
		}
	}

	/*
	typedef void* CheckLand;
	bool __cdecl CheckLand_1(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		DebugDrawText::log("Check Land Number:", 0, 0, { 0,1,0,1 });
		DebugDrawText::log("1", 0, 0, { 0,1,0,1 });
		return CheckIfCanLandOnSurface(context, upVector, surfaceNormal);
	}
	static ASMHOOK CheckLand_1_asm()
	{
		__asm
		{
			push[esp + 08h]
			push[esp + 08h]
			push eax

			call CheckIfCanLandOnSurface

			add esp, 4
			add esp, 4
			add esp, 4
			retn 08h
		}
	}
	*/

#define MAKE_CHECK_LAND(number,color) \
	bool __cdecl CheckLand_##number(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal) \
	{ \
		DebugDrawText::log("Check Land Number:", 0, 0, { 0,1,0,1 }); \
		DebugDrawText::log(#number, 0, 0, DebugDrawText::color); \
		return CheckIfCanLandOnSurface(context, upVector, surfaceNormal); \
	} \
	static ASMHOOK CheckLand_##number##_asm() \
	{ \
		__asm { push[esp + 08h] }\
		__asm { push[esp + 08h] }\
		__asm { push eax }\
\
		__asm { call CheckLand_##number }\
\
		__asm { add esp, 4 }\
		__asm { add esp, 4 }\
		__asm { add esp, 4 }\
		__asm { retn 08h }\
	}

#define INSTALL_CHECK_LAND(number,address) WRITE_CALL(address, CheckLand_##number##_asm)

	// Quicker float debugging in VS this way, because Eigen is silly and makes you scavenger hunt for the data.
	union VEC3
	{
		struct FloatArray
		{
			float x; float y; float z; float w;
		} array;
		CVector vector;
	};

	struct SContactData
	{
		bool field0;
		VEC3 m_ContactNormal;
		VEC3 v2;
		VEC3 v3;
		VEC3 v4;
		VEC3 m_ContactPosition;
		int32_t someInt;
		int32_t m_NumContacts;
		int32_t someInt3;
		int32_t someInt4;
	};
	ASSERT_OFFSETOF(SContactData, m_ContactNormal, 0x10);


	//MAKE_CHECK_LAND(1, Color(1,1,0,1))
	//MAKE_CHECK_LAND(2, Color(1,.5,0,1))
	//MAKE_CHECK_LAND(3, Color(1,.2,0,1))
	//MAKE_CHECK_LAND(4, Color(.8,1,0,1))
	//MAKE_CHECK_LAND(5, Color(.4,1,0,1))
	//MAKE_CHECK_LAND(6, Color(1,1,.5,1))
	//MAKE_CHECK_LAND(7, Color(1,.4,.2,1))

#if 0
	bool __cdecl CheckLand_DbgTemplate(CPlayerSpeedContext* context, const CVector& upVector, const CVector* surfaceNormal)
	{
		char buffer[128];
		sprintf_s(buffer, "Address: 0x%X", surfaceNormal);
		MessageBoxA(nullptr, buffer, "Land type X", MB_OK);

		return CheckIfCanLandOnSurface(context, upVector, *surfaceNormal);
	}
#endif

	bool DoRaycastOrFallback(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal, const CVector& start, const CVector& end)
	{
		// Check surface contact stuff
		RayHitData data;
		if (RaycastFindFloor(context, &data, start, end))
		{
			if (data.pRigidBody->GetBoolProperty(COLID_FLOOR) && CVector::Dot(context->GetVelocity().normalizedSafe(), data.Normal) > FLOOR_DOT_THRESHOLD)
				return false;
			if (data.pRigidBody->GetBoolProperty(COLID_WALL))
				return false;
			if (data.pRigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}

		return CheckIfCanLandOnSurface_Common(context, upVector, surfaceNormal);
	}

	// Land type 1
	// --------------------------

	bool __cdecl CheckLand_1(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		// They really should have just put this here in the first place rather than a fake dot product lol
		if (context->StateFlag(eStateFlag_InvokePtmSpike))
			return true;

		const CVector up = context->m_FloorAndGrindRailNormal;
		const CVector position = context->m_spMatrixNode->m_Transform.m_Position + up * 0.5f;

		const CVector start = position + upVector * 0.25f;
		const CVector end   = position - upVector * 0.55f;

		return DoRaycastOrFallback(context, upVector, surfaceNormal, start, end);
	}

	static void __declspec(naked) CheckLand_1_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_1 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	// Land type 2
	// --------------------------

	bool __cdecl CheckLand_2(CPlayerSpeedContext* context, const CVector& upVector, const CVector* surfaceNormal)
	{
		// NOTE: Never do this shit lol.
		SContactData* contactData = (SContactData*)((int)surfaceNormal - offsetof(SContactData, m_ContactNormal));

		// OLD - Casting a ray from sonic; we don't actually have to do this & shouldn't.
		/*
		const CVector up = context->m_FloorAndGrindRailNormal;
		const CVector position = context->m_spMatrixNode->m_Transform.m_Position + up * 0.5f;

		const CVector start = position + upVector * 0.25f;
		const CVector directionVector = *(CVector*)&contactData->m_ContactPosition - start;
		const CVector end   = start + directionVector + (directionVector.normalizedSafe() * 0.015f);
		*/

		// NEW: Casting a ray from the hit result's normal w/ some give. Hack way to get the rigidbody from this information.
		const CVector position = contactData->m_ContactPosition.vector;
		const CVector normal   = contactData->m_ContactNormal.vector;

		constexpr float offset = 0.015f;

		const CVector start = position + (normal * offset);
		const CVector end   = position - (normal * offset);

		return DoRaycastOrFallback(context, upVector, *surfaceNormal, start, end);
	}

	static void __declspec(naked) CheckLand_2_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_2 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	// Land type 3
	// --------------------------

	bool __cdecl CheckLand_3(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		//MessageBoxA(nullptr, "Landing check type 3 hit!\nPlease note where this happened and ensure the output is correct.\n\nAddres: 0x00E329A2", "TYPE 3 HIT", MB_OK);
		return CheckLand_1(context, upVector, surfaceNormal);
	}

	static void __declspec(naked) CheckLand_3_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_3 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	// Land type 4 -- MOST COMMON
	// ---------------------------

	bool __cdecl CheckLand_4(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		if (context->StateFlag(eStateFlag_WallWalkJump))
			return true;

		CPlayerSpeedContext::SGroundSearch* const pGroundSearch = &context->m_aGroundSearch[0xA];


		Sonic::CRigidBody* const rigidBody = pGroundSearch->pRigidBody;

		auto VerifyRigidBody = [&rigidBody]() -> bool
		{
#if _DEBUG
			if (!rigidBody)
				return false;

			int vtblID = *(int*)rigidBody;
			const bool isValid =
				   vtblID == 0x016A18A0 // Sonic::CRigidBody
				|| vtblID == 0x016D0750 // Sonic::CPathRigidBodyCollision
				;

			// The following likely happens because a setobject was your last contact rigidbody and was destructed, but not yet freed.
			if (vtblID == 0x016A04D8) // Sonic::CPhysicsUnit
			{
				DebugDrawText::log("WARNING!! Contact rigidbody is UNINITIALIZED!", 10.0f, 0, { 1,0,0,1 });
				return false;
			}


			if (!isValid)
				MessageBoxA(nullptr, "Please attach a debugger and verify what caused this!", "ERROR: CONTACT RIGIDBODY IS INVALID!", MB_OK | MB_ICONERROR);

			return isValid;
#else
			return true;
#endif
		};

		if (VerifyRigidBody() && pGroundSearch->isContact)
		{
			if (rigidBody->GetBoolProperty(COLID_FLOOR) && CVector::Dot(context->GetVelocity().normalizedSafe(), pGroundSearch->Normal) > 0.001f)
				return false;
			if (rigidBody->GetBoolProperty(COLID_WALL))
				return false;
			if (rigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}

		return CheckIfCanLandOnSurface_Common(context, upVector, surfaceNormal);
	}

	static void __declspec(naked) CheckLand_4_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	bool __cdecl CheckLand_5(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		//MessageBoxA(nullptr, "Tripped Type 5 check", "Type 5", MB_OK);
		if (context->StateFlag(eStateFlag_WallWalkJump))
			return true;

		CPlayerSpeedContext::SGroundSearch* const pGroundSearch = &context->m_aGroundSearch[context->m_ResolveGroundNormalCompleteResultType];

		Sonic::CRigidBody* const rigidBody = pGroundSearch->pRigidBody;

		auto VerifyRigidBody = [&rigidBody, &pGroundSearch]() -> bool
		{
#if _DEBUG
			if (!rigidBody)
				return false;

			int vtblID = *(int*)rigidBody;
			const bool isValid =
				vtblID == 0x016A18A0 // Sonic::CRigidBody
				|| vtblID == 0x016D0750 // Sonic::CPathRigidBodyCollision
				;

			// The following likely happens because a setobject was your last contact rigidbody and was destructed, but not yet freed.
			if (vtblID == 0x016A04D8) // Sonic::CPhysicsUnit
			{
				DebugDrawText::log("WARNING!! Contact rigidbody is UNINITIALIZED!", 10.0f, 0, { 1,0,0,1 });
				return false;
			}


			if (!isValid)
				MessageBoxA(nullptr, "Please attach a debugger and verify what caused this!", "ERROR: CONTACT RIGIDBODY IS INVALID!", MB_OK | MB_ICONERROR);

			return isValid;
#else
			return true;
#endif
		};

		if (VerifyRigidBody() && pGroundSearch->isContact)
		{
			if (rigidBody->GetBoolProperty(COLID_FLOOR) && CVector::Dot(context->GetVelocity().normalizedSafe(), pGroundSearch->Normal) > 0.001f)
				return false;
			if (rigidBody->GetBoolProperty(COLID_WALL))
				return false;
			if (rigidBody->GetBoolProperty(COLID_SLOPE))
				return true;
		}

		return CheckIfCanLandOnSurface_Common(context, upVector, surfaceNormal);
	}

	static void __declspec(naked) CheckLand_5_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_5 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	bool __cdecl CheckLand_6(CPlayerSpeedContext* context, const CVector& upVector, const CVector* surfaceNormal)
	{
		return CheckLand_2(context, upVector, surfaceNormal);
	}

	static void __declspec(naked) CheckLand_6_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_6 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	// Land type 7 -- SECOND MOST COMMON
	// ---------------------------------

	bool __cdecl CheckLand_7(CPlayerSpeedContext* context, const CVector& upVector, const CVector& surfaceNormal)
	{
		DebugDrawText::log("Check Land Number:", 0, 0, { 0, 1, 0, 1 });
		DebugDrawText::log("7", 0, 0, DebugDrawText::Color(1, .4, .2, 1));
		return CheckIfCanLandOnSurface(context, upVector, surfaceNormal);
	}

	static void __declspec(naked) CheckLand_7_asm()
	{
		__asm { push[esp + 08h] }
		__asm { push[esp + 08h] }
		__asm { push eax }
		__asm { call CheckLand_7 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { add esp, 4 }
		__asm { retn 08h }
	}

	MAKE_CHECK_LAND(8, Color(.2,1,1,1))
	MAKE_CHECK_LAND(9, Color(.2,.5,1,1))
	MAKE_CHECK_LAND(10, Color(1,.4,1,1))

	// Garbage I have to set up to make pitchroll work
	static ASMHOOK SetPitchRoll_asmOriginal()
	{
		static constexpr int jumpOut = 0x00E5200A;
		__asm
		{
			// prologue
			push    ebp
			mov     ebp, esp
			and esp, 0xFFFFFFF0
			sub     esp, 0x48
			push    ebx

			jmp[jumpOut]
		}
	}
	CVector* __cdecl SetPitchRollHOOK(CPlayerSpeedContext* context, const CVector& upVector, CVector* out)
	{
		__asm
		{
			push out
			mov edi, upVector
			mov eax, context
			call SetPitchRoll_asmOriginal
		}

		*out = context->m_FloorNormal;
		return out;
	}
	static ASMHOOK SetPitchRoll_asmHook()
	{
		__asm
		{
			push[esp + 04h] // out
			push edi // sonicUpVector
			push eax // sonicContext

			// Call your __cdecl function here:
			call SetPitchRollHOOK

			add esp, 4 // sonicContext<eax> is also used for return value
			pop edi // sonicUpVector
			add esp, 4 // out
			retn 04h
		}
	}

#pragma region ContactDebugging
	HOOK(void, __stdcall, _SonicScanContactPoints, 0x010E2160, Sonic::CCharacterProxy* a1, SContactData* a2, float deltaTime, const VEC3& direction)
	{
		original_SonicScanContactPoints(a1, a2, deltaTime, direction);
	}
#pragma endregion

	void Init()
	{
		// Optimized function hooks
		// TODO: Make these BETTER
		//WRITE_JUMP(0x00E54F50, CheckIfCanLandOnSurface_asm);
		WRITE_JUMP(0x00E54EB0, CheckIfCanWalkOnSurface_asm);

		INSTALL_HOOK(_SonicScanContactPoints)
		// Rigidbody tag extensions
		INSTALL_HOOK(_RigidBodyApplyProperties)


		// HACK: Doing a bunch of garbage to debug WHICH "check land on surface" happens.

		INSTALL_CHECK_LAND(1, 0x00E323A6)

		//INSTALL_CHECK_LAND(1, 0x00E323A6)
		INSTALL_CHECK_LAND(2, 0x00E32904)
		INSTALL_CHECK_LAND(3, 0x00E329A2)
		INSTALL_CHECK_LAND(4, 0x00E32AED)
		INSTALL_CHECK_LAND(5, 0x00E32BA7)
		INSTALL_CHECK_LAND(6, 0x00E32BDE)
		INSTALL_CHECK_LAND(7, 0x00E32C98)
		INSTALL_CHECK_LAND(8, 0x00E33745)
		INSTALL_CHECK_LAND(9, 0x00E3378A)
		INSTALL_CHECK_LAND(10, 0x00E337E7)
	}
}

// Bugfixes and extra flavor.
namespace Gameplay_Adventure::Extras
{
	using namespace hh::math;

#pragma region ParameterOverriding

	// This is a h4ck fix i implemented to set parameters on the fly. It's... not good.
	// Honestly I shouldn't be modifying params at runtime like this, if I'm trying to get an accurate SA1 jump regardless of your current config--
	// --then I should just use my own params instead of the game's. I think there was a reason for this at one point?? Can't remember now though..
	struct ValueHolder
	{
		void* vtable = nullptr;
		float value = 0.0f;
	};
	struct ValueHolderPtr
	{
		ValueHolder* data;
	};

	// TODO: Put in helpers?
	float* GetFloatParameterPointer(CParameter* params, uint32_t paramID)
	{
		if (!params)
			return nullptr;
		auto node = params->m_scpNode.get();
		if (!node)
			return nullptr;

		boost::any* boostAny = &params->m_scpNode->m_ValueMap[paramID];
		ValueHolderPtr* holder = (ValueHolderPtr*)boostAny;
		return &holder->data->value;
	}

	bool InitializeParamOverrides(CPlayerSpeedContext* context)
	{
		auto& spParams3D  = *(boost::shared_ptr<CParameter>*)0x01E61C54;
		auto& spParamsCmn = *(boost::shared_ptr<CParameter>*)0x01E61C4C;

		if (!spParams3D.get() || !spParamsCmn.get())
			return false;
		if (!spParams3D->m_scpNode.get() || !spParamsCmn->m_scpNode.get())
			return false;

		auto& params3D  =  spParams3D->m_scpNode->m_ValueMap;
		auto& paramsCmn = spParamsCmn->m_scpNode->m_ValueMap;

		// Allow instant action after a homing attack.
		params3D[ePlayerSpeedParameter_ActionEnableAfterHomingAttack] = 0.0f;
		// Make the actual *homing in* closer to SA1's quick speed, Modern gets this right idk why classic doesn't.
		params3D[ePlayerSpeedParameter_HomingSpeed] = 70.0f;
		// JumpPower should be smaller like in SA1.
		params3D[ePlayerSpeedParameter_JumpPower] = SA1Parameters::jmp_y_spd;

		// Patch to make sure this is *never* modified, otherwise if it is, you won't be able to jump, like at all.
		params3D[ePlayerSpeedParameter_JumpShortReleaseTime] = 0.0f;

		// Yall like spamdash?
		paramsCmn[eSonicClassicParameter_SpinChargeTimeLv0] = 0.0f;

		// Rotation interpolation cancelled out for now until I find a better way to prevent "sliding around."
		// TODO: Probably can get away with that by projecting both horizontal velocity direction & input direction onto the "floor normal." Worth a shot.

#pragma region Attempts
		//paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMaxDif] = 0.0f;
		//paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMinDif] = 0.0f;

		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMax] = 720.0f;
		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMin] = 720.0f;
		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMinVelocity] = 0.0f;
		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateSpeedMaxVelocity] = 4.0f;
		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateMinVelocity] = 0.0f;
		// paramsCmn[ePlayerSpeedParameter_PitchRollRotateMaxVelocity] = 0.0f;
#pragma endregion

		// Wall walking is jank because Generations is silly and papered over this for classic, because of course lol
		paramsCmn[ePlayerSpeedParameter_WalkWallMotionSlopeAngleMin] = 362.0f; // I dare this game to try returning an angle over 360 degrees lolol
		paramsCmn[ePlayerSpeedParameter_WalkWallMotionSlopeAngleMax] = 362.0f;

		return true;
	}

	void OverrideParamsPreUpdate(CPlayerSpeedContext* context)
	{
		// Original tech
		auto& spParams3D = *(boost::shared_ptr<CParameter>*)0x01E61C54;
		/*
		if (!spParams3D)
			return;
		CParameter* params3D = spParams3D->get();
		if (!params3D)
			return;
		auto node = params3D->m_scpNode.get();
		if (!node)
			return;
		auto& params = params3D->m_scpNode->m_ValueMap;
		*/

		using namespace hh::math;
		// We want to do something nutty for land enable slope.
		const float upAngleDot = fmax(0.0f, CVector::Dot(context->m_ModelUpDirection, CVector::Up()));
		// Land on slopes easier if our up vector is aligned to world up.
		const float slopeAngle = std::lerp(45.0f, 80.0f, upAngleDot);
		float* pParam = GetFloatParameterPointer(spParams3D.get(), ePlayerSpeedParameter_LandEnableMaxSlope);
		*pParam = slopeAngle;

		// FIXME: Writing to this dynamically makes dash paths break.... WHAT!? See why that is sometime I guess!
		// It might have to do with the fact we're getting "3D" params and not dash mode params, not sure.
		//params[ePlayerSpeedParameter_LandEnableMaxSlope] = slopeAngle;

		//params[parameter_] = slopeAngle;
	}

	void OverrideParamsPostUpdate(CPlayerSpeedContext* context)
	{
		auto spParams3D = (boost::shared_ptr<CParameter>*)0x01E61C54;
		if (!spParams3D)
			return;
		CParameter* params3D = spParams3D->get();
		if (!params3D)
			return;
		auto node = params3D->m_scpNode.get();
		if (!node)
			return;
		auto& params = params3D->m_scpNode->m_ValueMap;

		using namespace hh::math;
		// We want to do something nutty for land enable slope.
		const float upAngleDot = fmax(0.0f, CVector::Dot(context->m_ModelUpDirection, CVector::Up()));
		// Land on slopes easier if our up vector is aligned to world up.
		const float slopeAngle = std::lerp(45.0f, 80.0f, upAngleDot);
		params[ePlayerSpeedParameter_LandEnableMaxSlope] = slopeAngle;
	}

#pragma endregion

	// Fix Crisis City nonsense.
	HOOK(void, __fastcall, _ClassicProcMsgForceMoveToGround, 0x00DDACC0, CPlayerSpeedContext* context, void*, void* msg)
	{
		// For some reason "force to ground" is also called WHEN YOU ARE GROUNDED ALREADY, and is buggged for classic.
		// Just ignore this message if you're grounded already, there's no point. Autorun is already busted.
		//if (context->m_Grounded)
		//	return;

		// FIXME: THAT ACTUALLY DOESN'T WORK FOR SOME REASON.
		// Until this proves necessary I'm actually just bailing whenever this gets called, this does more harm than good.
		// TODO: Would probably be useful to FIX ForceMoveToGround since the core issue seems to be WHERE sonic moves is completely wrong. One day?
		//original_ClassicProcMsgForceMoveToGround(context, nullptr, msg);
	}

	// TODO: Make parameter?
	constexpr float HomingAttackRotateRate = 360.0f * DEG2RAD * 2.0f;

	class CSonicClassicStateHomingAttack : public CPlayerSpeedContext::CStateSpeedBase
	{
	public:
		int field_68;
		float m_PreviousVerticalSpeed;
		char HomingCalibrateAirDrag;
		// This is normally just padding.
		Eigen::Vector3f m_RotationXYZ;

		CVector m_MoveDir;
		int PluginThing;

		// Padding as well.
		float m_RotationW;
		//int field_98;
		//int field_9C;

		// Custom helper methods since I'm being a hack.
		void SetRotation(const CQuaternion& inRotation)
		{
			m_RotationXYZ = { inRotation.x(), inRotation.y(), inRotation.z() };
			m_RotationW   = inRotation.w();
		}

		CQuaternion GetRotation()
		{
			return CQuaternion(m_RotationW, m_RotationXYZ.x(), m_RotationXYZ.y(), m_RotationXYZ.z());
		}
	};
	BB_ASSERT_OFFSETOF(CSonicClassicStateHomingAttack, field_68, 0x68);
	BB_ASSERT_OFFSETOF(CSonicClassicStateHomingAttack, m_MoveDir, 0x80);
	//BB_ASSERT_OFFSETOF(CSonicClassicStateHomingAttack, field_9C, 0x9C);
	BB_ASSERT_SIZEOF(CSonicClassicStateHomingAttack, 0xA0);


	// Custom homing attack logic similar to what SA1 does.
	void DoHomingAttack(CPlayerSpeedContext* context, CSonicClassicStateHomingAttack* state, float deltaTime)
	{
		typedef CVector Vector3;
		typedef CQuaternion Quaternion;

		/*
		const CQuaternion preRotation = context->m_VerticalRotation * context->m_HorizontalRotation;
		CVector speed = preRotation.inverse() * context->GetHorizontalVelocity();
		context->SetYawRotation(finalYawRotation);
		//context->SetHorizontalVelocity_Dirty(finalRotation * speed);
		const CQuaternion finalRotation = context->m_VerticalRotation * finalYawRotation;
		const CVector localVelocity = finalRotation.inverse() * initialVelocity;
		*/

		const Vector3 charPosition   = context->m_spMatrixNode->m_Transform.m_Position;
		const Vector3 targetPosition = context->m_HomingAttackPosition;

		const Vector3 toTargetVector = targetPosition - charPosition;
		const Vector3 toTargetNormal = toTargetVector.normalizedSafe();

		const Vector3 targetHorizontal = Vector3::ProjectOnPlane(toTargetNormal,  GravityDirection);
		const Vector3 targetVertical   = Vector3::Project(toTargetNormal,        -GravityDirection);
		const float verticalSign = Vector3::Dot(targetVertical.normalizedSafe(), -GravityDirection);
		const float targetVerticalSpeed = targetVertical.Magnitude() * verticalSign;

		//Quaternion currentRotation = context->m_VerticalRotation * context->m_HorizontalRotation;
		const Quaternion currentRotation = context->m_HorizontalRotation;
		float angle = Vector3::SignedAngle(targetHorizontal, currentRotation.Forward(), -GravityDirection) * RAD2DEGf;

		// Referencing unity's way of doing things. TODO: Change this to use global funcs or sonic team's own method.
		auto repeat = [](float t, float m) -> float
		{
			return std::clamp(t - floorf(t / m) * m, 0.0f, m);
		};
		auto LerpAngle = [repeat](float a, float b, float t) -> float
		{
			const float dt = repeat(b - a, 360.0f);
			return std::lerp(a, a + (dt > 180.0f ? dt - 360.0f : dt), t);
		};

		// TODO: Put out of scope
		constexpr float _homingAttackRotationRate = 25.0f;
		constexpr float _homingAttackPower        = 50.0f;

		const float rotationAngle      = LerpAngle(0, angle, _homingAttackRotationRate * deltaTime) * DEG2RADf;
		//const float rotationAngle      = 0.5f * DEG2RADf;
		const Quaternion speedRotation = currentRotation * Quaternion::FromAngleAxis(-rotationAngle, -GravityDirection);
		const Vector3 speed = Vector3(0.0f, targetVerticalSpeed, targetHorizontal.Magnitude()) * _homingAttackPower;

		context->SetVelocity(speedRotation * speed);
		context->SetYawRotation(speedRotation);
		state->SetRotation(speedRotation);
	}

	HOOK(void, __fastcall, _ClassicHomingAttackBegin,  0x0122EDD0, CSonicClassicStateHomingAttack* This)
	{
		using namespace hh::math;
		auto sonic = This->GetContext();
		bool hasNoTarget = !sonic->m_HomingAttackTargetActorID;


		// This stuff here controls weather or not homing attack makes you move forward at a set speed & isn't in control.
		// Unleashed and Generations have Sonic shoot forward in a straight line out-of-control,
		// the adventure games instead have a natural downward arc and let you turn around.
		// Both games do not have you affected by gravity if you're 
		// TODO: Don't do this metaprogramming garbage lol. Just re-implement the function.

		if (hasNoTarget)
		{
			//WRITE_NOP(0x0122F0C8, 0x0122F112 - 0x0122F0C8)
			WRITE_JUMP(0x0122F0C8, 0x0122F112)
		}
		else
		{
			WRITE_MEMORY(0x0122F0C8, uint8_t, 0x0F, 0x28, 0x83, 0x90, 0x02)
			//BYTE memSet[0x0122F112 - 0x0122F0C8] = { 0x0F, 0x28, 0x83, 0x90, 0x02, 0x00, 0x00, 0xD9, 0x05, 0x14, 0x86, 0x5C, 0x01, 0x0F, 0x29, 0x44, 0x24, 0x40, 0xF3, 0x0F, 0x10, 0x44, 0x24, 0x44, 0xF3, 0x0F, 0x11, 0x47, 0x6C, 0x8B, 0x8B, 0x34, 0x05, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x05, 0xF8, 0x39, 0x70, 0x01, 0x51, 0xF3, 0x0F, 0x11, 0x83, 0xB0, 0x07, 0x00, 0x00, 0xD9, 0x1C, 0x24, 0x8B, 0x51, 0x04, 0x53, 0xC6, 0x42, 0x17, 0x00, 0xE8, 0xF4, 0xBA, 0xC2, 0xFF, 0x89, 0x87, 0x90, 0x00, 0x00, 0x00 };
			//WRITE_MEMORY(0x0122F0C8, BYTE, 0x0F, 0x28, 0x83, 0x90, 0x02, 0x00, 0x00, 0xD9, 0x05, 0x14, 0x86, 0x5C, 0x01, 0x0F, 0x29, 0x44, 0x24, 0x40, 0xF3, 0x0F, 0x10, 0x44, 0x24, 0x44, 0xF3, 0x0F, 0x11, 0x47, 0x6C, 0x8B, 0x8B, 0x34, 0x05, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x05, 0xF8, 0x39, 0x70, 0x01, 0x51, 0xF3, 0x0F, 0x11, 0x83, 0xB0, 0x07, 0x00, 0x00, 0xD9, 0x1C, 0x24, 0x8B, 0x51, 0x04, 0x53, 0xC6, 0x42, 0x17, 0x00, 0xE8, 0xF4, 0xBA, 0xC2, 0xFF, 0x89, 0x87, 0x90, 0x00, 0x00, 0x00)
		}

		// HACK: Store forward direction, explained below why.
		const CVector frontDirection = sonic->GetFrontDirection();
		const CQuaternion horizontalRot = sonic->m_HorizontalRotation;

		original_ClassicHomingAttackBegin(This);

		if (hasNoTarget)
		{
			// Just re-implementing what Gens does, but without the gravitytimer change that makes sonic float.

			This->m_PreviousVerticalSpeed = sonic->m_VerticalVelocity.y();
			//sonic->m_GravityTimer = -100000000.0;
			sonic->StateFlag(eStateFlag_EnableHomingAttack) = false;

			// We want to treat "homing" differently... so bail after this.
			return;
		}

		// HACK: We're gonna implement SA1's homing attack action where we rotate towards our target.
		// To do this "easily," we're gonna take our forward direction & interpolate from that to our target direction.
		// Magnitude will be the same as whatever is computed after homing attack has been ran.
		// Do this for update as well.

		const float homingMoveSpeed = sonic->GetVelocity().Length();
		This->SetRotation(sonic->m_VerticalRotation * sonic->m_HorizontalRotation);

		sonic->SetHorizontalVelocityClearChanged(frontDirection * homingMoveSpeed);
		sonic->SetYawRotation(horizontalRot);
	}
	HOOK(void, __fastcall, _ClassicHomingAttackUpdate, 0x0122EA10, CSonicClassicStateHomingAttack* This)
	{
		using namespace hh::math;
		auto sonic = This->GetContext();
		bool hasNoTarget = !sonic->m_HomingAttackTargetActorID;

		if (hasNoTarget)
		{
			const float homingAttackDecelMultiplier =
			                                          0.89999999999f; // SA1
		//	                                          0.98f;          // SA2

			const CVector horizontalVelocity = sonic->GetHorizontalVelocity();
			const float speed = horizontalVelocity.norm();
			const CVector direction = horizontalVelocity.normalizedSafe();

			const float decelAmount = (speed - (speed * homingAttackDecelMultiplier)) * SA1Parameters::TargetFramerate * This->GetDeltaTime();
			sonic->SetHorizontalVelocityClearChanged(direction * fmax(0.0f, speed - decelAmount));
		}

		auto* input = &Sonic::CInputState::GetInstance()->GetPadState();
		if (input->IsTapped(Sonic::eKeyState_X) || input->IsTapped(Sonic::eKeyState_B))
		{
			sonic->ChangeState("Fall");
			return;
		}

		// Originally I wanted to use the *velocity direction* instead, but that proved... unstable. It's unclear why right now.
		// Gens will force sonic to aim in the direction of the target 100% of the time so we just need to reorient him. Front direction should be fine for this.
		//const CVector frontDirection = sonic->GetFrontDirection();
		//const CVector frontDirection = sonic->GetVelocity().normalizedSafe();

		original_ClassicHomingAttackUpdate(This);

		if (hasNoTarget)
			return;

		DoHomingAttack(sonic, This, This->GetDeltaTime());

		/*
		DebugDrawText::log(format("ID: %d", sonic->m_HomingAttackTargetActorID), 0,0, {1,1,0.8,1});

		const CVector originalVelocity = sonic->GetVelocity();
		const float homingMoveSpeed  = originalVelocity.Length();

		const CVector moveDirection = CVector::RotateTowards(frontDirection,
		                                                     originalVelocity.normalizedSafe(),
		                                                     HomingAttackRotateRate * This->GetDeltaTime());
		sonic->SetHorizontalVelocity(frontDirection * homingMoveSpeed);
		sonic->SetYawRotation(CQuaternion::LookRotation(moveDirection.ProjectOnPlane(CVector::Up()).normalizedSafe()));

		// NOTE: current implementation shoots sonic into the sky LOL
		*/
	}

	// UNUSED
	HOOK(void, __fastcall, _ClassicHomingAttackEnd, 0x0122ED10, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		auto sonic = This->GetContext();
		bool hasNoTarget = !sonic->m_HomingAttackTargetActorID;

		if (hasNoTarget)
		{
			// Debug thing.
			//sonic->SetHorizontalVelocity(sonic->GetFrontDirection() * 600.0f);
			return;
		}

		original_ClassicHomingAttackEnd(This);
	}

	// Make Sonic fall off a surface if he's in his standing state but over a 45 degree incline.
	bool SonicDetatchFromWall(CPlayerSpeedContext* context, bool speedCheck = false)
	{
		// TODO: Make parameters
		constexpr float wallPushForce    = 10.0f;
		constexpr float wallDetatchAngle = 45.0f * DEG2RADf;
		constexpr float wallDetatchSpeed = 0.15000001f; // value copied from Gens

		DebugDrawText::log(format("WalkSpeed: %f", context->GetHorizontalVelocity().norm()), 0, 0, { 1,0.2,0.2,1 });

		const CVector upDirection = context->m_FloorAndGrindRailNormal;

		// Only do our check if we're on an incline in the first place.
		if (CVector::Angle(upDirection, CVector::Up()) <= wallDetatchAngle)
			return false;

		// The walk state requires a speed check, bail if we're over the limit & holding the stick.
		if (  !speedCheck // Don't detatch if we don't have a speed check.
			|| context->m_WorldInput.norm() > FLT_EPSILON                  // Don't detatch if holding the stick.
			|| context->GetHorizontalVelocity().norm() > wallDetatchSpeed) // Don't detatch if over threshold.
			return false;

		// in the SA games we get pushed off the wall when we're standing on a steep incline, so let's do the same here.
		// Todo: maybe make the force of the wall push proportional to the incline?

		context->SetVelocity(context->GetVelocity() + (upDirection * wallPushForce));

		context->ChangeState("Fall");
		return true;
	}

	HOOK(void, __fastcall, _ClassicStandStateStart, 0x00DB9F90, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		// Only care about detatching in Stand state if we're on an incline we shouldnt be.
		if (SonicDetatchFromWall(This->GetContext()))
			return;

		original_ClassicStandStateStart(This);
	}

	HOOK(void, __fastcall, _ClassicWalkStateUpdate, 0x012419B0, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		// Detatch ONLY IF we're moving slowly and aren't holding the stick
		if (SonicDetatchFromWall(This->GetContext(), true))
			return;

		original_ClassicWalkStateUpdate(This);
	}

	// let Classic homing attack when he rolls off ledges or hits a widespring
	// + live swap gravity/jump force parameters as needed.
	HOOK(void, __fastcall, _ClassicFallStateStart,  0x01115A60, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		using namespace hh::math;
		using namespace Sonic::Player;

		auto context = This->GetContext();

		// Set original values
		// TODO: extend the class size & add our own stuff.
		auto info = Common::SCommonInfo::Get(context);
		//info->m_Params.m_Gravity   = *gravityParam;
		//info->m_Params.m_JumpPower = *jumpPowerParam;

		// If I remember right, "ball mode" actually gets set once the state *starts,* so this has to be called now.
		original_ClassicFallStateStart(This);

		bool* isBall = GetPointer<bool>(This, 0x6D);
		if (*isBall != true)
			return;

		// Now we do whatever else we want at this step.

		context->StateFlag(eStateFlag_EnableHomingAttack)  = true;
		context->StateFlag(eStateFlag_EnableAirOnceAction) = true;

		//*gravityParam   = 28.8f; // TODO: Make these params, I guess? They're just what SA1's values are, so...
		//*jumpPowerParam = SA1Parameters::jmp_y_spd;
	}
	HOOK(void, __fastcall, _ClassicFallStateUpdate, 0x01115790, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		auto* const context = This->GetContext();
		const float deltaTime = This->GetDeltaTime();

		bool* isBall = (bool*)((int)This + 0x6D);
		auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();

		// Jump canceling
		if (*isBall && Physics::SonicDoJumpCancel(context, input))
			*isBall = false;

		original_ClassicFallStateUpdate(This);
	}
	HOOK(void, __fastcall, _ClassicFallStateEnd,    0x011159B0, CPlayerSpeedContext::CStateSpeedBase* This)
	{
		CPlayerSpeedContext* context = This->GetContext();
		original_ClassicFallStateEnd(This);
	}


	// Live swap parameters
	// JumpBallStart isn't necessary because JumpPower is applied BEFORE the state actually starts.
	HOOK(void, __stdcall, _ChangeToJump, 0x00E5D7D0, CPlayerSpeedContext* context, const char* name, float jumpForce)
	{
		if (*(int*)context != idClassicContext)
		{
			original_ChangeToJump(context, name, jumpForce);
			return;
		}

		auto& spParams3D  = *(boost::shared_ptr<CParameter>*)0x01E61C54;
		auto& spParamsCmn = *(boost::shared_ptr<CParameter>*)0x01E61C4C;

		volatile float* jumpPowerParam = GetFloatParameterPointer(spParams3D.get(), ePlayerSpeedParameter_JumpPower);

		// Now that those values have at least been initialized...
		if (context->m_Is2DMode) // No influencing 2D mode right now.
		{
			original_ChangeToJump(context, name, jumpForce);
			return;
		}

		// TODO: Make this a param, I guess? It's just what SA1's value is, so...
		*jumpPowerParam = SA1Parameters::jmp_y_spd;

		// After jump force has been modified, we can call the jump change.
		// Even though the function that calls "ChangeToJump" does everything under this already, that function is optimized.
		// So, unfortunately, we have to redo all of this junk ourselves.
		// That said, this will give us more control, so if needed we can do alternate jump power stuff.
		// Just a shame this happens like, twice.

		auto GetJumpPower = [&context](float baseJumpPower) -> float
		{
			int whateverThisIs = GetValue<int>(context, 0xDD8);

			if (whateverThisIs)
			{
				const float blockedJumpPowerDec = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_EnemyBlockedJumpPowerDec);
				const float blockedJumpPower    = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_EnemyBlockedJumpPower1);

				return fmax(0.0f, blockedJumpPower - (float)whateverThisIs * blockedJumpPowerDec);
			}

			if (context->StateFlag(eStateFlag_InvokeSuperSonic))
			{
				const float powerAdd = context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_JumpPowerBySkill);
				return baseJumpPower + powerAdd;
			}
			if (context->StateFlag(eStateFlag_Damaging))
			{
				return context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_JumpPowerDamageAir);
			}
			if (context->StateFlag(eStateFlag_InvokeSkateBoard))
			{
				return context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_JumpPowerOnBoard);
			}
			if (context->StateFlag(eStateFlag_OnWaterSeEnabled) && (context->Skills() & 0x400) != 0)
			{
				return baseJumpPower + context->m_spParameter->Get<float>(Sonic::Player::ePlayerSpeedParameter_JumpPowerAllRounder);
			}
			return baseJumpPower;
		};
		const float newJumpPower = GetJumpPower(*jumpPowerParam);

		original_ChangeToJump(context, name, newJumpPower);

		// We don't need the rest of this, but just in case we want to get crazy...
		return;

		/*
		using namespace hh::math;
		const CVector upVector = context->m_FloorAndGrindRailNormal;
		original_ChangeToJump(context, name, jumpForce);

		const float thrustForce = context->m_JumpThrust.norm();
		//context->m_JumpThrust = CVector::Slerp(context->m_JumpThrust.normalizedSafe(), context->m_FloorAndGrindRailNormal, 0.5f) * thrustForce;

		// Let's increase jump force based on slope.
		const float slopeMultiplier = fmin(1.0f - CVector::Dot(upVector, CVector::Up()), 1.0f);
		// We don't want this compensation boost if we aren't holding forward.
		// TODO: Do something simpler for the love of god lol
		const float inputMultiplier = 1.0f - std::clamp(context->m_WorldInput.norm(), 0.0f, 1.0f);
		context->m_JumpThrust = upVector * thrustForce;
		//context->m_JumpThrust += upVector * ;
		context->m_JumpThrust += upVector * (slopeMultiplier * inputMultiplier * 10.0f);

		*/
	}
	HOOK(void, __fastcall, _ClassicJumpBallEnd,   0x01114F00, CSonicClassicContext::CStateSpeedBase* This)
	{
		CPlayerSpeedContext* context = This->GetContext();
		auto info = Common::SCommonInfo::Get(context);

		original_ClassicJumpBallEnd(This);
	}

	ASMHOOK homingTimerPatch_Classic()
	{
		static uint32_t returnAddress = 0x0122EBBB;
		static uint32_t func = 0x0053A9F0;

		//static float homingTime = 16.0f / SA1Parameters::TargetFramerate; // SA2
		static float homingTime = 7.0f / SA1Parameters::TargetFramerate; // SA1

		__asm
		{
			// just keep this here so we don't break anything
			call func
			//override param value
			movss xmm0, dword ptr[homingTime]

			jmp[returnAddress]
		}
	}
	ASMHOOK JumpDashAdditive()
	{
		static uint32_t returnAddress = 0x0122F02F;
		__asm
		{
			movaps  xmm1, xmmword ptr[ebx + 2A0h]
			addps   xmm0, xmm1
			movaps  xmmword ptr[ebx + 2A0h], xmm0
			// While we're here, let's set Y to 0.
			// TODO: This is risky and should probably be re-considered if we ever do arbitrary gravity.
			mov dword ptr[ebx + 2A4h], 0
			jmp[returnAddress]
		}
	}

	// Stick stuff
	struct StickInfo
	{
		float x;
		float y;
		int16_t xInt;
		int16_t yInt;
	};

	// Removes the (absurdly large) cardinal deadzones, replaces square deadzone with circular.
	// Credit to Skyth setting up an improved formula.
	HOOK(void, __cdecl, _SetStick, 0x009C6AE0, StickInfo* stickData, int16_t Deadzone, int16_t OuterDeadzone)
	{
		// My first attempt on my own, cause NAN errors if you don't have an analog stick.
		/*
			constexpr double DEAD_ZONE = 0.25;
			//const double DEAD_ZONE = Deadzone / 32767.0;

			double x = static_cast<double>(stickData->xInt) / 32767.0;
			double y = static_cast<double>(stickData->yInt) / 32767.0;

			const double norm = sqrt(x * x + y * y);

			if (norm < DEAD_ZONE)
			{
				stickData->x = 0.0f;
				stickData->y = 0.0f;
				return;
			}

			const double newNorm = std::max(0.0, norm - DEAD_ZONE) / (1.0 - DEAD_ZONE);

			x /= norm;
			x *= newNorm;

			y /= norm;
			y *= newNorm;

			if (x > 1.0) x = 1.0;
			else if (x < -1.0) x = -1.0;

			if (y > 1.0) y = 1.0;
			else if (y < -1.0) y = -1.0;

			stickData->x = x;
			stickData->y = y;
			return;
		*/

		int16_t argX = stickData->xInt;
		int16_t argY = stickData->yInt;

		const double x = fmin(1.0, fmax(-1.0, argX / 32767.0));
		const double y = fmin(1.0, fmax(-1.0, argY / 32767.0));
		const double magnitude = sqrt(x * x + y * y);
		const double deadzone = Deadzone / 32767.0;

		if (magnitude < deadzone)
		{
			stickData->x = 0.0f;
			stickData->y = 0.0f;
			return;
		}

		const double newMagnitude = fmax(0.0, fmin(1.0, (magnitude - deadzone) / (1 - deadzone)));

		stickData->x = (x / magnitude * newMagnitude);
		stickData->y = (y / magnitude * newMagnitude);
	}

	// SA1 style widespring behavior
	// Get a type definition for this message, because we actually want to modify it.

	class MsgWideSpringImpulse : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		Hedgehog::Math::CVector m_ImpactPoint{};
		Hedgehog::Math::CVector m_ImpulseDirection{};
		float m_OutOfControlTime{};
		float m_ImpulseHeight{};
		bool m_Cond01{};
		bool m_DoPositionInterp{};
	};

	HOOK(void, __fastcall, _ProcMsgWideSpringImpulse, 0x00E6C310, CPlayerSpeed* This, void*, MsgWideSpringImpulse* msg)
	{
		// Jump out early if modern sonic.
		if (*(int*)This == idModernObject)
		{
			original_ProcMsgWideSpringImpulse(This, nullptr, msg);
			return;
		}
		msg->m_ImpulseHeight += msg->m_OutOfControlTime;
		msg->m_OutOfControlTime = 0.0f;
		original_ProcMsgWideSpringImpulse(This, nullptr, msg);
		This->GetContext()->ChangeState("Jump");
	}

	//////////////
	// BUGFIXES //
	//////////////

	// ShoesSliding, for some reason, enables moonsault. This is... SO bizzare, because classic shouldn't have moonsault enabled in the first place, ever???
	// Todo: Just set this at the end of his object update if that's possible.
	// UNDONE: Can't remember why I commented this out but whatever
	//HOOK(void*, __fastcall, _StateShoesSlidingStart, 0x01234130, CPlayerSpeedContext::CStateSpeedBase* state)
	//{
	//	auto context = state->GetContext();
	//	if (*(int*)context == idClassicContext)
	//		context->StateFlag(eStateFlag_DisableMoonsault) = true;
	//	return original_StateShoesSlidingStart(state);
	//}

	// Speed Highway helicopter uses MsgSpringImpulse to launch you off.
	// MsgSpringImpulse has an oversight where it will set "ChangePath" to true, when normally this is false.
	// It's unclear how much of the game gets affected by this being defaulted to "on,"
	// so correcting this should be case-by-case.
	//
	// "ChangePath" being true is what sets sonic to forward mode when touching a forward path,
	// something that's surprisingly hard to undo when walking off a forward path.
	// Because of that, this issue is something that definitely needs fixing when it's found. Oh well.
	//
	// Easy fix for Speed Highway is to check if we're in the "HangOn" state, and if we are, override the flag with what it was *before* the message.
	// Pass in a function pointer because we call this in two hooks that do the same damn thing.
	void SonicMsgSpringImpulse(CPlayerSpeed* This, void* msg, void(__fastcall* originalFn)(CPlayerSpeed* This, void*, void* msg))
	{
		Hedgehog::Universe::CStateMachineBase::CStateBase* state = This->m_StateMachine.GetCurrentState().get();
		const CPlayerSpeedContext* context = This->GetContext();

		// Hacky int comparison to check current state
		if (*reinterpret_cast<int*>(state) != 0x016D22F4) // VFTable for CPlayerSpeedStateHangOn
		{
			originalFn(This, nullptr, msg);
			return;
		}

		const bool originalChangePath = context->StateFlag(eStateFlag_ChangePath);
		originalFn(This, nullptr, msg);
		context->StateFlag(eStateFlag_ChangePath) = originalChangePath;
	}

	// Modern -- Mostly did this because Modern's where I noticed this, and if I ever want to give him "SA2 physics" then this'll need fixing.
	HOOK(void, __fastcall, _ProcMsgSpringImpulseM, 0x00E28120, CPlayerSpeed* This, void*, void* msg)
	{
		SonicMsgSpringImpulse(This, msg, original_ProcMsgSpringImpulseM);
	}

	// Classic -- We also want to disable homing attack when using a spring for him specifically.
	HOOK(void, __fastcall, _ProcMsgSpringImpulseC, 0x00DEACC0, CPlayerSpeed* This, void*, void* msg)
	{
		SonicMsgSpringImpulse(This, msg, original_ProcMsgSpringImpulseC);
		This->GetContext()->StateFlag(eStateFlag_EnableHomingAttack) = false;
	}

	// FIXME: STOP HOOKING THIS FOR THE LOVE OF GOD AND MAKE FUNCTIONS WE CALL IN A GLOBAL CPP IM AT MY LIMIT

	float PatchParametersPreUpdate(CPlayerSpeed* This, const hh::fnd::SUpdateInfo& updateInfo)
	{
		using namespace Core;
		auto* context = This->GetContext();
		auto info = Common::SCommonInfo::Get(context);

		// Handle out of control ease timer.
		const bool outOfControl = context->StateFlag(eStateFlag_OutOfControl)
		                       || context->StateFlag(eStateFlag_NoLandOutOfControl)
		                       || context->StateFlag(eStateFlag_AirOutOfControl);

		info->m_TimeUntilInControl = fmax(info->m_TimeUntilInControl - updateInfo.DeltaTime, 0.0f);
		if (outOfControl)
		{
			info->m_TimeUntilInControl = Common::SCommonInfo::ms_MaxOutOfControlTime;
		}

		// let's disable moonsault every frame, because Classic should *NEVER* use this.
		context->StateFlag(eStateFlag_DisableMoonsault) = true;
		
		// Shitty heuristic

		auto& spParams3D = *(boost::shared_ptr<CParameter>*)0x01E61C54;
		float* gravityPtr = GetFloatParameterPointer(spParams3D.get(), ePlayerSpeedParameter_Gravity);

		const float originalGravity = *gravityPtr;
		auto state = context->m_pPlayer->m_StateMachine.GetCurrentState();
		bool isBall = false;
		DebugDrawText::log(state->m_Name.c_str());
		if (state->m_Name == "Fall")
		{
			isBall = GetValue<bool>(state.get(), 0x6D);
		}
		if (state->m_Name == "Jump" || state->m_Name == "JumpShort" || isBall)
		{
			*gravityPtr = 28.8f;
		}
		
		if (!info->m_Params.m_Initialized)
		{
			info->m_Params.m_Initialized = InitializeParamOverrides(context);
		}

		return originalGravity;
	}
	void PatchParametersPostUpdate(CPlayerSpeed* This, float originalGravity)
	{
		// Do it here too idk we're getting some one-frame blips
		auto* context = This->GetContext();
		context->StateFlag(eStateFlag_DisableMoonsault) = true;

		auto& spParams3D = *(boost::shared_ptr<CParameter>*)0x01E61C54;
		float* gravityPtr = GetFloatParameterPointer(spParams3D.get(), ePlayerSpeedParameter_Gravity);
		*gravityPtr = originalGravity;
	}

	// Fix input to match SA1 sort of. Control input by making it a factor of how much you turn around in Yaw space.
	HOOK(void*, __fastcall, _Input3DStandardUpdate, 0x00E30370, CPlayerSpeedContext::CStateSpeedBase* state)
	{
		using namespace hh::math;
		DebugDrawText::log("CustomInput");

		auto* const context = state->GetContext();
		if (context->m_Is2DMode || !context->m_Grounded || context->StateFlag(eStateFlag_IgnorePadInput))
			return original_Input3DStandardUpdate(state);

		auto* const camera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera().get();
		const CVector forwardVector = camera->m_MyCamera.m_Direction.ProjectOnPlane(CVector::Up()).normalizedSafe();
		      CVector rightVector   = CVector::Cross(forwardVector, CVector::Up());

		auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();

		const CVector directionVector = (input->LeftStickVertical * forwardVector) + (input->LeftStickHorizontal * rightVector);
		const CVector direction = directionVector;
		const float inputMag = directionVector.norm();

		// Fallback for now
		if (direction.squaredNorm() < FLT_EPSILON)
			return original_Input3DStandardUpdate(state);

		const CQuaternion rotation = context->m_VerticalRotation * CQuaternion::LookRotation(direction, CVector::Up());
		context->m_WorldInput = rotation.Forward() * inputMag;
		return nullptr;
	}

	// Just disable "sliding" for classic right now, honestly.
	HOOK(bool, __fastcall, _SetSliding, 0x00DC31B0, CPlayerSpeedContext* context, void*, int Unknown)
	{
		if (*(int*)context != idClassicContext)
			return original_SetSliding(context, nullptr, Unknown);

		return false;
	}

	// Patch walk state to have you stick to surfaces.
	// TODO: I fixed this another way at some point but I forgot how that happened... find that out lol
	/*
	HOOK(void, __fastcall, _SpeedPosture3DStandardUpdate, 0x00E365A0, CPlayerSpeedContext::CStateSpeedBase* state)
	{
		auto* const context = state->GetContext();
		if (*(int*)context != idClassicContext)
		{
			original_SpeedPosture3DStandardUpdate(state);
			return;
		}


		const bool previousForceGrip = context->StateFlag(eStateFlag_ForceLandForCaught);
		context->StateFlag(eStateFlag_ForceLandForCaught) = context->GetHorizontalVelocity().squaredNorm() > 5.0f * 5.0f
		                                                  ? true : previousForceGrip;

		original_SpeedPosture3DStandardUpdate(state);

		context->StateFlag(eStateFlag_ForceLandForCaught) = false;
	}
	*/

	// Test
	HOOK(void*, __fastcall, _hkApplyID, 0x0090A9A0, void* This, void*, int a1, int a2, int a3)
	{
		DebugDrawText::log(format("Output: %X", a1), 10.0f, 0, { 0.5, 0.9, 1.0, 1.0 });
		if (a1 == 0x201F)
		{
			bool Break = true;
		}
		return original_hkApplyID(This, nullptr, a1, a2, a3);
	}
	HOOK(void*, __fastcall, _hkApplyID2, 0x0090A7B0, void* This, void*, int a1, int a2, int a3, int a4)
	{
		DebugDrawText::log(format("Output: %X", a1), 10.0f, 0, { 0.5, 0.9, 1.0, 1.0 });
		if (a1 == 0x2002)
		{
			bool Break = true;
		}
		return original_hkApplyID2(This, nullptr, a1, a2, a3, a4);
	}

	// Patch MsgStart/End Dive to just do nothing. Classic doesn't support it yet, so kill it to prevent hard crashes.
	void __fastcall StubMsgFunction(CPlayerSpeed* This, void*, void* msg) {}
	// Have to do this because Luna made the upreel in Ultimate Test Stage have, like, 5K velocity, it's absurd.
	// Skydive gives you a shitload of air friction when you hit it so it cancels out, but this should suffice.
	void __fastcall ClassicDiveStartPatch(CPlayerSpeed* This, void*, void* msg)
	{
		using namespace hh::math;
		auto* const context = This->GetContext();

		CVector velocity = context->GetVelocity();
		velocity.y() = fmin(velocity.y(), 50.0f);

		context->SetVelocity(velocity);
	}

	// PATCH: FallDead just doesn't work if velocity is going up, Crisis City specific patch. MIGHT BREAK.
	HOOK(void, __fastcall, _ProcMsgFallDeadHitCollision, 0x011EB690, Sonic::CGameObject3D* This, void*, Hedgehog::Universe::MessageTypeSet* msg)
	{
		auto* const player  = (CPlayerSpeed*)This->m_pMessageManager->GetMessageActor(msg->m_SenderActorID);
		auto* const context = player->GetContext();

		if (context->GetVelocity().y() > 0.0f)
			return;

		original_ProcMsgFallDeadHitCollision(This, nullptr, msg);
	}

	// TEST: Animation fix
	HOOK(CVector*, __fastcall, _ClassicAnimContextUpdate1, 0x00DD92B0, void* This, void*, CVector* a2)
	{
		*a2 = CVector(20, 0, 0);
		return a2;
	}
	HOOK(CVector*, __fastcall, _ClassicAnimContextUpdate2, 0x00DD9240, void* This, void*, CVector* a2)
	{
		*a2 = CVector(20, 0, 0);
		return a2;
	}

	// Patch animation blending.
	void NOINLINE SetAnimationBlend(Sonic::CAnimationStateMachine* stateMachine, const hh::base::CSharedString& state, const hh::base::CSharedString& targetState, float blendTime)
	{
		//state@<eax>, stateMachine@<ecx>, targetState@<edi>, float a3
		static constexpr uint32_t func = 0x00CE0720;
		__asm
		{
			mov ecx, stateMachine
			mov eax, targetState
			mov edi, state
			push blendTime
			call func
		}
	}

	HOOK(void, __fastcall, _PrepareClassicAnimations, 0x00DDF1C0, CPlayerSpeed* player, void*, Sonic::CAnimationStateMachine* stateMachine, hh::anim::CAnimationPose* pose)
	{
		original_PrepareClassicAnimations(player, nullptr, stateMachine, pose);

		SetAnimationBlend(stateMachine, "SpinAttack",   "Fall", 0.25f);
		SetAnimationBlend(stateMachine, "JumpBoard",    "Fall", 0.125f);
		SetAnimationBlend(stateMachine, "JumpBoardRev", "Fall", 0.125f);
	}
	HOOK(void, __fastcall, _PrepareModernAnimations, 0x00E1B6C0, CPlayerSpeed* player, void*, Sonic::CAnimationStateMachine* stateMachine, hh::anim::CAnimationPose* pose)
	{
		original_PrepareModernAnimations(player, nullptr, stateMachine, pose);

		//SetAnimationBlend(stateMachine, "StompingSquat", "Walk", 0.25f);
		//SetAnimationBlend(stateMachine, "Stand", "Walk", 0.25f);
		SetAnimationBlend(stateMachine, "Walk", "Stand", 0.25f);
	}

	// Modern sonic blending patch
	// TODO: avoid doing this, and instead patch out the part of modern's stomp->stand blending that feels like shit, directly.
	struct ImmediatePlaybackInfo
	{
		int a;
		int b;
		float TransitionTime;
		float c;
	};
	HOOK(void, __fastcall, _PlayAnimWithFlags, 0x00E74BF0, CPlayerSpeedContext* context, void*, const hh::base::CSharedString& name, struct ImmediatePlaybackInfo* data)
	{
		if (data->TransitionTime < FLT_EPSILON)
		{
			context->ChangeAnimation(name);
			return;
		}

		original_PlayAnimWithFlags(context, nullptr, name, data);
	}

	// Patch IsNoStanding stuff because this kinda sucks.
	HOOK(bool, __fastcall, _IsNoStanding, 0x00E606D0, CPlayerSpeedContext* This)
	{
		const CVector up = This->GetUpDirection();
		const CVector center = This->m_spMatrixNode->m_Transform.m_Position + (up * 0.5f);
		const CVector velocity = This->GetVelocity();

		CVector rayEnd = center - up * 1.5f;
		if (CVector::Dot(velocity, up) < 0.0f)
			rayEnd += velocity.normalizedSafe();

		return Collision::CheckSurfaceHasID(This, center, rayEnd, 0x201B);
	}

	// Fix water footstep positions.
	HOOK(bool, __fastcall, _SetWaterFootstepPosition, 0x011DCEE0, CPlayerSpeedContext* This, void*, CVector* Vec)
	{
		const bool result = original_SetWaterFootstepPosition(This, nullptr, Vec);

		if (!result)
			return false;

		*Vec += CVector(0, 0.05, 0);
		return true;
	}

	inline void Init()
	{
		INSTALL_HOOK(_Input3DStandardUpdate)
		INSTALL_HOOK(_SetSliding)
		//INSTALL_HOOK(_SpeedPosture3DStandardUpdate)

		// Annoying CSC patch
		INSTALL_HOOK(_ClassicProcMsgForceMoveToGround)

		// Another CSC patch because of course. For that one spring that kills you every time.
		// FIXME: DOESN'T WORK!
		//INSTALL_HOOK(_ProcMsgFallDeadHitCollision)

		// Patch out skydive messages
		WRITE_CALL(0x00DEB43A, ClassicDiveStartPatch)
		WRITE_CALL(0x00DEB460, StubMsgFunction)

		INSTALL_HOOK(_ClassicHomingAttackBegin)
		INSTALL_HOOK(_ClassicHomingAttackUpdate)

		// fix sticking to walls in a stupid way
		INSTALL_HOOK(_ClassicStandStateStart)

		INSTALL_HOOK(_ClassicFallStateStart)
		INSTALL_HOOK(_ClassicFallStateUpdate)
		INSTALL_HOOK(_ClassicFallStateEnd)

		INSTALL_HOOK(_SetStick)
		INSTALL_HOOK(_IsNoStanding)

		INSTALL_HOOK(_ProcMsgSpringImpulseM)
		INSTALL_HOOK(_ProcMsgSpringImpulseC)
		INSTALL_HOOK(_ProcMsgWideSpringImpulse)
		//INSTALL_HOOK(_StateShoesSlidingStart)

		// DEBUG: overriding the jumpdash time w/o param editing just for iteration's sake.
		WRITE_JUMP(0x0122EBB6, homingTimerPatch_Classic)

		WRITE_JUMP(0x0122F028, JumpDashAdditive)
		WRITE_MEMORY(0x0122F035, BYTE, 0x00) // True  -> False
		WRITE_MEMORY(0x0122F03C, BYTE, 0x01) // False -> True

		// Remove the *SHITTY* 0.3 multiplier for classic's homing attack.
		WRITE_MEMORY(0x0122E9BB, BYTE, 0xE8);// Classic
		WRITE_NOP(0x0122E9BC, 4);

		// We don't want spinning to be ear-piercing, so replace roll SFX with SpinFromCharge's so it matches SA1's anyway.
		WRITE_MEMORY(0x011BB4C3 + 1, uint32_t, 0x1E8899);

		// Fix stumbling animations
		WRITE_MEMORY(0x01283497, char*, "sc_stumble00_f_loop")
		WRITE_MEMORY(0x01283430, char*, "sc_stumble00_f_loop")
		// And jumpselectors
		WRITE_MEMORY(0x012834FE, char*, "sc_springjump_up_loop")
		WRITE_MEMORY(0x01284581, char*, "sc_springjump_up_loop")
		WRITE_MEMORY(0x012845E8, char*, "sc_springjump_up_loop")
		// Try to fix blending
		INSTALL_HOOK(_PrepareClassicAnimations)
		//INSTALL_HOOK(_PrepareModernAnimations)

		// Disable homing attack animations for classic.
		WRITE_NOP(0x01115365, 0x0111536A - 0x01115365)

		// Do this for modern sonic too why not
		//WRITE_MEMORY(0x01231B0C, uint8_t, 0xE8)
		//WRITE_NOP(0x01231B0D, 4)

		// Fix water footsteps having z-fighting if your water collision is 1:1 with the water mesh.
		INSTALL_HOOK(_SetWaterFootstepPosition)

		// Fix sand SFX to use grass instead of stone (what the fuck is this oversight)
		WRITE_MEMORY(0x00DC365C, uint8_t, 0x04)


		// DEBUG: Disable velocity writes
		// WRITE_NOP(0x0122F028, 0x0122F041 - 0x0122F028)
	}
}

void Gameplay_Adventure::Init()
{
	//using namespace GameplayMod;

	Physics::Init();
	Collision::Init();
	Extras::Init();


	// Mandatory codes for preventing spindash crash + enabling homing attack at all times.
	WRITE_MEMORY(0x16D8CD4, uint32_t, 0x11D6140)	// 3D Spindash vftable swap
	WRITE_NOP(0x00DC50DF, 6)	// Homing attack

	// Outdated patches that affect both modern and classic. TODO: Get rid of these and live-swap impulse flags.
	// Patch "No Trick Rainbow Rings" by "Hyper"
	WRITE_NOP(0x115A6AF, 2);
	// Patch "No Trick Jump Boards" - modified to use a better impulse
	WRITE_MEMORY(0x1014866, uint8_t, 0x06);

	// Patch "Longer Blue Trail" by "N69 & Nekit"
	WRITE_NOP(0xE5FB17, 6)
	WRITE_MEMORY(0xE5FE10, uint8_t, 0x48)
	WRITE_MEMORY(0xE5FE70, uint8_t, 0x48)


	// Function overrides
	//-----------------------

	// Walking
	WRITE_CALL(0x00E3614D, Physics::CustomSonicAcceleration);
	WRITE_CALL(0x00E36115, Physics::CustomRotation);
	// Air
	WRITE_CALL(0x00E3868B, Physics::CustomAccelerationAir);
	WRITE_CALL(0x00E387BF, Physics::CustomRotationAir);
	// AirBreak
	WRITE_CALL(0x00E387C5, Physics::SonicDoAirBreak);

	// There is no situation where we want rolling to not step up stairs when walking does.
	// Genuinely feels awful, so just make these the same.
	WRITE_MEMORY(0x00DC3147, uint8_t, 0x9A) // 9B = SquatAndSlidingStairsMax, 9A = StairsMax

	// Do these animation patches when we get better animations that have classic sonic holding his arm out.
	/*
	WRITE_MEMORY(0x0128204A, char*, "sc_dash_l_loop") // Jet Wall L
	WRITE_MEMORY(0x012820B9, char*, "sc_dash_r_loop") // Jet Wall R
	*/


	// TEMPORARY: No air break
	//WRITE_JUMP(0x00E387CC, 0x00E3889E);
}

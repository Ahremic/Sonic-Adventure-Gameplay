#pragma once

// Forward declarations
namespace Sonic::Message
{
	class MsgGetDashModeInfo;
}

namespace Gameplay_Adventure
{
	class SinglePlayerGlobals;
	// Relevant parameters from SA1 regarding gameplay code.
	// TODO: Probably make them configurable? And/or just use Gens' own parameters but in a different way, remapping them as necessary.
	class SA1Parameters
	{
	public:
		// Have this here so we can easily scale certain operations by deltatime when they were originally designed for 60fps timesteps *only.*
		// TODO: This is still dubious. I want to benchmark this code at some point and really confirm it's doing what we expect, AND properly adhere to integrals.
		// If that doesn't make sense, just check this video: https://youtu.be/yGhfUcPjXuE?si=xvXrdFJs9lbNCVJu&t=633
		// Note: Might require changing how some stuff works in gens, haha.
		constexpr static float TargetFramerate = 60.0f;

		constexpr static float HangTimeSeconds = 1.0f;

		// Gravity in SA1 is calculated with "Weight." This amusingly means heavier players fall faster.
		// Obviously that makes some sense for gamefeel even if it introduces inconsistency.
		// Since we're only porting Sonic's gameplay, this is just "SA1's Gravity."
		//constexpr static float DefaultWeight = 0.079999998f;
		constexpr static float weight = 0.079999998f;


		// These values are measures of distance over time.
		// They were scaled from SA1 data by (x * 0.1 * 60.0) or (x * 6).
		constexpr static float jog_speed = 2.76f;
		constexpr static float run_speed = 8.34f;
		constexpr static float rush_speed = 13.8f;
		constexpr static float max_x_spd = 18.0f;
		constexpr static float crash_speed = 22.2f; // If we ever want to re-implement Unleashed's unused wall crash, it'd be funny to make SA gameplay's limit match SA1 lol
		constexpr static float dash_speed = 30.54f; // Normally you shouldn't be at this speed, but when you are, in SA1 you get the special "top speed" animation and steering is much harder.
		constexpr static float lim_h_spd = 96.0f;   // SA1 hard-limit speed cap. Not used currently, but worth keeping in case?
		constexpr static float lim_frict = -1.695f;
		constexpr static float grd_frict_z = -3.6f;
		constexpr static float run_accel = 0.3f;
		constexpr static float air_accel = 0.186f;
		constexpr static float jmp_y_spd = 9.96f;
		constexpr static float jmp_addit = 0.456f;
		constexpr static float slow_down = -0.36f;
		constexpr static float air_break = -1.02f;

		// These are multipliers. They do not get scaled.
		constexpr static float air_resist_air = -0.028000001f;
		constexpr static float air_resist = -0.0080000004f;
		constexpr static float air_resist_y = -0.0099999998f;
		constexpr static float air_resist_z = -0.40000001f;
	};

	class ModParameters
	{
		// i just hate redundancy man.
#define STATIC static inline
	public:
		// Spindash
		STATIC float spinGravityMultiplier = 3.141592653589793f; // std::numbers::pi
		STATIC float spinGravityLossCurveLow = -5.0f;
		STATIC float spinGravityLossCurveHigh = 50.0f;
		STATIC float spinDecelCurveLow = -15.0f;
		STATIC float spinDecelCurveHigh = 20.0f;
		STATIC float spinWorldInputExponent = 3.0f;
		STATIC float spinTurnRestrictionMinSpeed = 7.0f;
		STATIC float spinTurnRestrictionMaxSpeed = 300.0f;
		STATIC float spinTurnEaseFactor = 5.0f;
		STATIC float spinTurnAdjustedByVelocity = 1.0f;

		// Config (todo: ?)

#undef STATIC
	};

	namespace Extras
	{
		float PatchParametersPreUpdate (Sonic::Player::CPlayerSpeed* This, const hh::fnd::SUpdateInfo& updateInfo);
		void  PatchParametersPostUpdate(Sonic::Player::CPlayerSpeed* This, float originalGravity = 33.3f);
	}

	namespace Core
	{
		struct SPhysicsEssentials
		{
			float MaxSpeed;
			float LerpAmount;
			Sonic::Message::MsgGetDashModeInfo& dashInfo;

			static SPhysicsEssentials Get(Sonic::Player::CPlayerSpeedContext* context);
		};

		Hedgehog::Math::CVector GetGuidedInputDirection(Sonic::Player::CPlayerSpeedContext* context,
		                                                const Hedgehog::Math::CVector& inputDirection, float SpeedZ,
		                                                const SPhysicsEssentials& essentials, bool aimToVelocity = false);
	}

	void Init();
}
#include "ProceduralAnimation.h"
//#include <BlueBlur.inl>


#include "../DebugDrawText.h"
#include "Common.h"
#include "Config.h"
#include "Cameras.h"
//#include "HavokEssentials.h"
using namespace Common;

// Private classes for this CPP file that have our extended data information.
namespace AnimPosePatch
{
	// Typedefs set up in the event we want to switch to Havok's SDK, using their names as a base
	typedef hk2010_2_0::hkaSkeleton hkaSkeleton;
	typedef hk2010_2_0::hkaAnimatedSkeleton hkaAnimatedSkeleton;
	typedef hk2010_2_0::hkaBone hkaBone;
	typedef void hkaAnimationBinding;
	typedef void hkpCachingShapePhantom;

	// In order to properly apply procedural animation in the way we intend, i.e. modifying the transform hierarchy,
	// we need to modify it AFTER animation has been sampled, which happens at a very specific time in CAnimationPose's update.
	// So, we need to extend CAnimationPose's size. Simple, right? Well there's a problem.
	//
	// CAnimationPose is a generic class used for any and all animated assets in the game.
	// Unfortunately, its constructor is not wrapped in a "make shared" function in all of its use cases.
	// This means every time CAnimationPose is heap allocated, it pushes a unique copy of its size onto the stack.
	// That integer would have to be modified to compensate our extended data in EVERY case, or we'd get out-of-bounds errors.
	//
	// CAnimationPose's constructor is called in *128* places. This would be very time consuming to patch.
	// Instead, we'll hijack the allocator for an std::map<> to give ourselves some wiggle room.
	// DO NOTE: The allocator for this is not a 32-bit integer, but an 8-bit one. We only have a block size of 128 bytes available.
	// This gives us 32 integers to play with, which is still a lot, but that can get eaten up quickly with complex data, i.e. vectors.

	struct __declspec(align(4)) CAnimationPose_Alternate
	{
		struct Map
		{
			// Original properties.
			INSERT_PADDING(0x10) {};

			// Custom extensions.
			ProceduralAnimation::ProceduralData procData;
		};

		INSERT_PADDING(0x11C);
		Map* m_pMap;
	};
	// Prevent allocator overflowing.
	static_assert(sizeof(CAnimationPose_Alternate::Map) <= 0x7F);
}
using namespace AnimPosePatch;

// Forward declarations
//----------------------------------

namespace ProceduralAnimation::Impl
{
	void HeadTurnCore(hh::anim::CAnimationPose* pose, Sonic::Player::CPlayerSpeed* player);
	void HeadTurnClassic(hh::anim::CAnimationPose* pose, Sonic::Player::CPlayerSpeed* player);
}

// System management
//-----------------------------------------

namespace ProceduralAnimation::Core
{
	// Debug parameters
	//------------------------------
	float QuillStiffness = 7.164850514373732;
	float QuillFriction = 1.842068074395237;
	float QuillTimeScale = 0.1f;

	float QuillMultiplierBase = 0.25f;
	float QuillMultiplierTip = 0.45f;

	float QuillFlopStrength = 0.7f;

	int boneToAffect = 0;
	bool debugBoneWithInput = false;
	float debugBoneAngle = 0.0f;

	// Hook for ParamEditor bone debugging.
	HOOK(void, __cdecl, InitializeApplicationParams_GPM, 0x00D65180, Sonic::CParameterFile* This)
	{
		boost::shared_ptr<Sonic::CParameterGroup> parameterGroup = This->CreateParameterGroup("AnimTest", "AnimTest");
		Sonic::CEditParam* pc_TestRotation = parameterGroup->CreateParameterCategory("Bone debug", "Test bone rotations based on index");

		pc_TestRotation->CreateParamInt(&boneToAffect, "Bone index");
		pc_TestRotation->CreateParamBool(&debugBoneWithInput, "Use debug input to test bone index");
		pc_TestRotation->CreateParamFloat(&debugBoneAngle, "Float to control bone rotation directly (Degrees)");

		parameterGroup->Flush();

		Sonic::CEditParam* pc_QuillRotation = parameterGroup->CreateParameterCategory("Quill Physics",
			"Controls the secondary movement of Sonic's quills,\nboth classic and modern.");
		pc_QuillRotation->CreateParamFloat(&QuillStiffness, "Stiffness");
		pc_QuillRotation->CreateParamFloat(&QuillFriction, "Friction");
		pc_QuillRotation->CreateParamFloat(&QuillTimeScale, "Time scale");
		pc_QuillRotation->CreateParamFloat(&QuillFlopStrength, "Flop strength");
		pc_QuillRotation->CreateParamFloat(&QuillMultiplierBase, "Base multiplier");
		pc_QuillRotation->CreateParamFloat(&QuillMultiplierTip, "Tip multiplier");

		parameterGroup->Flush();

		// TODO: REMOVE ME
		Sonic::CEditParam* pc_EFighterG = parameterGroup->CreateParameterCategory("Egg Fighter Gunner",
			"Params for \"EnemyEFighterG\" properties we want to adjust manually.");
		pc_EFighterG->CreateParamFloat(&Config::ms_EFighterG_RotAngleY, "RotationAngleY");
		pc_EFighterG->CreateParamFloat(&Config::ms_EFighterG_RotAngleX, "RotationAngleX");

		parameterGroup->Flush();

		originalInitializeApplicationParams_GPM(This);
	}

	// Functions
	//--------------------------------

	// Helper function for getting the camera front direction if we have one (which should always be the case, but you never know...)
	// Made a function to reduce nesting in case we hit null exceptions.
	hh::math::CVector GetCameraFrontVector(hh::math::CVector fallback)
	{
		const auto gameDocument = Sonic::CGameDocument::GetInstance();
		if (!gameDocument.get().get())
			return fallback;

		const auto world = gameDocument->GetWorld();
		if (!world.get().get())
			return fallback;

		const auto camera = world->GetCamera();
		if (!camera)
			return fallback;

		return camera->m_MyCamera.m_Direction;
	}

	
	// Here we control our eye movement logic.
	// Modern and Classic both handle this individually, presumably due to slight differences,
	// but their data structures for their eyes are the same, and can be treated about the same too.
	// Note: These classes may be understood in blueblur now, but I had to reference this stuff manually and as such I kinda just winged it.
	// The majority of the mapped fields are not used and instead data is accessed through pointer math, but its a nice reference.
	// TODO: Map these classes correctly and commit to blueblur.
	void UpdateUVAnim(Sonic::Player::CPlayerSpeed* This, const hh::fnd::SUpdateInfo& updateInfo)
	{
		using namespace hh::math;
		struct WeirdEyeStruct
		{
			uint32_t m_field0;
			uint32_t m_field4;
			uint32_t m_field8;
			uint32_t m_fieldC;
			uint32_t m_field10;
			uint32_t m_field14;
			uint32_t m_field18;
			uint32_t m_field1C;
			uint32_t m_field20;
			uint32_t m_field24;
			uint32_t m_field28;
			uint32_t m_field2C;
			uint32_t m_field30;
			uint32_t m_field34;
			uint32_t m_field38;
			uint32_t m_field3C;
			uint32_t m_field40;
			uint32_t m_field44;
			uint32_t m_field48;
			uint32_t m_field4C;
			uint32_t m_field50;
			uint32_t m_field54;
			uint32_t m_field58;
			uint32_t m_field5C;
		};
		struct WeirdMatStruct
		{
			char gap00[0x50];
			WeirdEyeStruct* m_EyeData;
		};
		struct CMaterialData
		{
			uint32_t m_field0;
			uint32_t m_field4;
			uint32_t m_field8;
			uint32_t m_fieldC;
			uint32_t m_field10;
			uint32_t m_field14;
			uint32_t m_field18;
			uint32_t m_field1C;
			uint32_t m_field20;
			uint32_t m_field24;
			uint32_t m_field28;
			uint32_t m_MatStuff;
			uint32_t m_field30;
			uint32_t m_field34;
			uint32_t m_field38;
			uint32_t m_field3C;
			uint32_t m_field40;
			uint32_t m_field44;
			uint32_t m_field48;
			uint32_t m_field4C;
			uint32_t m_field50;
			uint32_t m_field54;
			uint32_t m_field58;
			uint32_t m_field5C;
		};

		// TODO: Replace these manual GETs w/ blueblur declarations.
		auto eyeAnimMatData1 = *(CMaterialData**)((uint32_t)This + 0x368);
		auto eyeAnimMatData2 = *(CMaterialData**)((uint32_t)This + 0x370);

		// Unused as of now.
		// void* textureAnimData1 = *(void**)((uint32_t)This + 0x378);
		// void* textureAnimData2 = *(void**)((uint32_t)This + 0x380);

		auto preparePtr1 = *reinterpret_cast<uint32_t*>(eyeAnimMatData1->m_MatStuff + 0x50);
		auto preparePtr2 = *reinterpret_cast<uint32_t*>(eyeAnimMatData2->m_MatStuff + 0x50);
		CVector2* eyeL = (CVector2*)(*reinterpret_cast<uint32_t*>(preparePtr1 + 8));
		CVector2* eyeR = (CVector2*)(*reinterpret_cast<uint32_t*>(preparePtr2 + 8));

		// POSITIVE = LEFT, NEGATIVE = RIGHT

		const float eyeX_MaxDistance = 0.20f;
		const float eyeX_MinDistance = 0.05f;

		auto context = This->GetContext();
		auto info = SCommonInfo::Get(context);

		// Store the original eye offset after Generations did its thing.
		info->m_OriginalEyeOffsetL = *eyeL;
		info->m_OriginalEyeOffsetR = *eyeR;

#ifdef _USE_DUPLICATE
		// FIXME: DUPLICATE CODE, SEE WHAT WE'RE GONNA DO ABOUT EYES MOVING FASTER THAN HEAD
#pragma region EyeLookDuplicate
		using namespace hh::math;
		const CVector frontDirection = context->GetFrontDirection();
		const CVector rightDirection = context->GetRightDirection();

		const CVector worldInputVec = context->m_WorldInput.normalizedSafe();
		const float   worldInputMag = context->m_WorldInput.Length();

		const float speed = context->GetHorizontalVelocity().Magnitude();
		const float sign = CVector::Dot(rightDirection, worldInputVec) > 0.0f ? -1.0f : 1.0f; // RIGHT+ LEFT-
		const float targetLeanX = static_cast<float>(std::min(std::abs(1.0f - CVector::Dot(frontDirection, worldInputVec)), 1.0)) * sign * worldInputMag;

		float eyeOffsetL_X = -targetLeanX;
		float eyeOffsetR_X = -targetLeanX;
#pragma endregion
#endif

		float eyeOffsetL_X = -info->m_targetEyes.x();
		float eyeOffsetR_X = -info->m_targetEyes.x();

		// HACK: Simple *incorrect* restriction based on the target eye offset being positive or negative.
		// This is a "linear" remap, doesn't matter but isn't "correct" so eh.
		// TODO: Remap eyes correctly based on their actual position in eye-space.
		eyeOffsetL_X = eyeOffsetL_X < 0 ? eyeOffsetL_X * eyeX_MinDistance : eyeOffsetL_X * eyeX_MaxDistance;
		eyeOffsetR_X = eyeOffsetR_X > 0 ? eyeOffsetR_X * eyeX_MinDistance : eyeOffsetR_X * eyeX_MaxDistance;

		// STUB: Y movement is not handled right now. This will happen when look-at-objects is implemented.
		float eyeOffsetL_Y = 0.0f;
		float eyeOffsetR_Y = 0.0f;

		// Apply our animation by adding to what's already there.
		// TODO: Implement some kind of safeguards to make sure eyeAnim + procAnim doesn't overshoot min/max values.
		// There's a smart way to do this--the ADDITIVE animation can't overshoot, but STeam's UV Anims can.
		// BUG: There is a REALLY bad problem where sometimes (like on slopes?) the eye offset is NOT reset but only ADDED to?!? Fix this!!
		*eyeL += CVector2(eyeOffsetL_X, eyeOffsetL_Y);
		*eyeR += CVector2(eyeOffsetR_X, eyeOffsetR_Y);
	}

	// HACK: We use this function to save Gens' eye UV anim and re-apply it before update runs, to stop WEIRD eye sliding nonsense.
	void RestoreUVAnim(Sonic::Player::CPlayerSpeed* This)
	{
		using namespace hh::math;

		// Gens lazy-loads some information, so we're just.. gonna lazy load too lol
		// Restore eye stuff
		auto context = This->GetContext();
		auto info = SCommonInfo::Get(context);

		if (!info->m_UVsInitialized)
		{
			info->m_UVsInitialized = true;
			return;
		}

		// FIXME: Duplicate code/declarations
#pragma region duplicates
		struct WeirdEyeStruct
		{
			uint32_t m_field0;
			uint32_t m_field4;
			uint32_t m_field8;
			uint32_t m_fieldC;
			uint32_t m_field10;
			uint32_t m_field14;
			uint32_t m_field18;
			uint32_t m_field1C;
			uint32_t m_field20;
			uint32_t m_field24;
			uint32_t m_field28;
			uint32_t m_field2C;
			uint32_t m_field30;
			uint32_t m_field34;
			uint32_t m_field38;
			uint32_t m_field3C;
			uint32_t m_field40;
			uint32_t m_field44;
			uint32_t m_field48;
			uint32_t m_field4C;
			uint32_t m_field50;
			uint32_t m_field54;
			uint32_t m_field58;
			uint32_t m_field5C;
		};
		struct WeirdMatStruct
		{
			char gap00[0x50];
			WeirdEyeStruct* m_field1C;
		};
		struct CMaterialData
		{
			uint32_t m_field0;
			uint32_t m_field4;
			uint32_t m_field8;
			uint32_t m_fieldC;
			uint32_t m_field10;
			uint32_t m_field14;
			uint32_t m_field18;
			uint32_t m_field1C;
			uint32_t m_field20;
			uint32_t m_field24;
			uint32_t m_field28;
			uint32_t m_MatStuff;
			uint32_t m_field30;
			uint32_t m_field34;
			uint32_t m_field38;
			uint32_t m_field3C;
			uint32_t m_field40;
			uint32_t m_field44;
			uint32_t m_field48;
			uint32_t m_field4C;
			uint32_t m_field50;
			uint32_t m_field54;
			uint32_t m_field58;
			uint32_t m_field5C;
		};

		// TODO: Replace these manual GETs w/ blueblur declarations.
		auto eyeAnimMatData1 = *(CMaterialData**)((uint32_t)This + 0x368);
		auto eyeAnimMatData2 = *(CMaterialData**)((uint32_t)This + 0x370);

		void* preparePtr1 = (void*)*reinterpret_cast<uint32_t*>(eyeAnimMatData1->m_MatStuff + 0x50);
		void* preparePtr2 = (void*)*reinterpret_cast<uint32_t*>(eyeAnimMatData2->m_MatStuff + 0x50);
#pragma endregion

		const bool matEnabledA = *(bool*)((int)This + 0x388);
		const bool matEnabledB = *(bool*)((int)This + 0x389);

		if (!matEnabledA || !matEnabledB)
			return;

		float* eyeL = (float*)*reinterpret_cast<uint32_t*>((int)preparePtr1 + 8);
		float* eyeR = (float*)*reinterpret_cast<uint32_t*>((int)preparePtr2 + 8);

		// Restore eye stuff

		eyeL[0] = info->m_OriginalEyeOffsetL.x();
		eyeL[1] = info->m_OriginalEyeOffsetL.y();

		eyeR[0] = info->m_OriginalEyeOffsetR.x();
		eyeR[1] = info->m_OriginalEyeOffsetR.y();

		//*eyeL = info->m_OriginalEyeOffsetL;
		//*eyeR = info->m_OriginalEyeOffsetR;
	}

	// Constructors for both initializing our newly generated fields to zero, and for adding whatever else we need.
	// Functions are written for these due to unfortunate code duplication in the game binary. No need for US to repeat ourselves.

	void CAnimationPoseInit_Ctor(Hedgehog::Animation::CAnimationPose* pose)
	{
		ProceduralData* procData = ProceduralData::Get(pose);
		procData->m_pObject        = nullptr;
		procData->UpdateProcedural = nullptr;

	}
	void CAnimationPoseInit_AddCallbackC(app::Player::CPlayerSpeed* player)
	{
		ProceduralData* procData = ProceduralData::Get(player->m_spAnimationPose.get());
		procData->m_pObject = player;
		procData->SetUpdateFunction(Impl::HeadTurnClassic);
	}
	void CAnimationPoseInit_AddCallbackM(app::Player::CPlayerSpeed* player)
	{
		ProceduralData* procData = ProceduralData::Get(player->m_spAnimationPose.get());
		procData->m_pObject = player;
		procData->SetUpdateFunction(Impl::HeadTurnCore);
	}


	// Hooks
	//--------------------------

	// Initialize fields as null by default
	HOOK(void*, __fastcall, _ConstructCAnimationPose_Common, 0x006CB140, hh::anim::CAnimationPose* This, void*, void* a2, void* a3)
	{
		void* result = original_ConstructCAnimationPose_Common(This, nullptr, a2, a3);
		CAnimationPoseInit_Ctor(This);
		return result;
	}
	HOOK(void*, __fastcall, _ConstructCAnimationPose_Msn, 0x006CA910, hh::anim::CAnimationPose* This, void*, void* a2, void* a3, void* a4)
	{
		void* result = original_ConstructCAnimationPose_Msn(This, nullptr, a2, a3, a4);
		CAnimationPoseInit_Ctor(This);
		return result;
	}

	// Initialize fields appropriately for Sonic--CAnimationPose is assigned in AddCallback so we need to hook that.
	HOOK(void, __fastcall, _SonicInitializeRenderables_Modern, 0x00E267F0, app::Player::CPlayerSpeed* This, void*, void* a2, int a3)
	{
		original_SonicInitializeRenderables_Modern(This, nullptr, a2, a3);
		CAnimationPoseInit_AddCallbackM(This);
	}
	HOOK(void, __fastcall, _SonicInitializeRenderables_Classic, 0x00DE93A0, app::Player::CPlayerSpeed* This, void*, void* a2, int a3)
	{
		original_SonicInitializeRenderables_Classic(This, nullptr, a2, a3);
		CAnimationPoseInit_AddCallbackC(This);
	}

	// Hook here to intercept a given CAnimationPose's hkaAnimaiton sampling, and override the result as needed.
	// For Sonic, this is the core of how we do procedural head movement.
	// Since this is a function pointer, we're able to overload this any way we want for ANY CAnimPose.
	// I'd like this to extend to some API so other mods can make use of this, for things like set objects and whatever.

	HOOK(void, __fastcall, _CAnimationPose_SampleAnimation, 0x6CC950, hh::anim::CAnimationPose* This)
	{
		original_CAnimationPose_SampleAnimation(This);

		ProceduralData* procData = ProceduralData::Get(This);
		if (procData->UpdateProcedural == nullptr || procData->m_pObject == nullptr)
			return;

		procData->UpdateProcedural(This, procData->m_pObject);
	}
}

// External implementations
//--------------------------------------

#pragma region Definitions
ProceduralAnimation::ProceduralData*
ProceduralAnimation::ProceduralData::Get(Hedgehog::Animation::CAnimationPose* pose)
{
	return &reinterpret_cast<CAnimationPose_Alternate*>(pose)->m_pMap->procData;
}
void ProceduralAnimation::ProceduralData::SetUpdateFunction(void* functionPointer)
{
	UpdateProcedural = (FPtrUpdateProcedural)functionPointer;
}



// Now onto the fun stuff.
void ProceduralAnimation::Impl::HeadTurnCore(hh::anim::CAnimationPose* pose, Sonic::Player::CPlayerSpeed* player)
	{
		using namespace hh::math;
		using namespace hh::anim;

		auto context = player->GetContext();
		auto info = SCommonInfo::Get(context);

		enum class BoneIndex_C
		{
			CenterBody = 7,
			Neck = 8,
			Head = 9,

			BCQuill = 50,
			BLQuill = 51,
			BRQuill = 52,
			TCQuill = 53,
			TLQuill = 54,
			TRQuill = 55,

			LeftLeg = 56,
			RightLeg = 74,
			LeftArm = 92,
			RightArm = 124,
			Chest = 156,
		};
		enum class BoneIndex_M
		{
			LowerChest = 5,
			UpperChest = 6,
			Neck = 7,
			Head = 8,

			EarL = 12,
			EarR = 13,

			BCQuill_INNER = 42,	// Needle_B_C
			BCQuill_OUTER = 43,	// Needle1_B_C
			BLQuill_INNER = 44,
			BLQuill_OUTER = 45,
			BRQuill_INNER = 46,
			BRQuill_OUTER = 47,
			TCQuill_INNER = 48,
			TCQuill_OUTER = 49,
			TLQuill_INNER = 50,
			TLQuill_OUTER = 51,
			TRQuill_INNER = 52,
			TRQuill_OUTER = 53,

			Nose = 53,
		};

		enum class BoneIndex_UNLEASHED
		{
			LowerChest = 5,
			UpperChest = 6,
			Neck = 7,
			Head = 8,

			EarL = 12,
			EarR = 13,

			BCQuill_INNER = 41,
			BCQuill_OUTER = 42,
			BLQuill_INNER = 43,
			BLQuill_OUTER = 44,
			BRQuill_INNER = 45,
			BRQuill_OUTER = 46,
			TCQuill_INNER = 47,
			TCQuill_OUTER = 48,
			TLQuill_INNER = 49,
			TLQuill_OUTER = 50,
			TRQuill_INNER = 51,
			TRQuill_OUTER = 52,

			Nose = 53,
		};

		//////////////
		// Code start
		//////////////
		//-----------------------------------------

		uint32_t ClassicSonicPlayerID = 0x016D85DC;
		uint32_t ModernSonicPlayerID  = 0x016D4B2C;

		uint32_t playerID = *(uint32_t*)player;

		// Safeguard in case something goes terribly wrong
		if (playerID != ClassicSonicPlayerID && playerID != ModernSonicPlayerID)
			return;

		// We don't want to do anything if we're not in the right state.
		const auto state = player->m_StateMachine.GetCurrentState().get();
		if (!state) return;
		const uint32_t stateID = *(uint32_t*)state;


		const CVector frontDirection = CVector::ProjectOnPlane(context->GetFrontDirection(), context->m_VerticalRotation.Up()).normalizedSafe();
		const CVector rightDirection = CVector::ProjectOnPlane(context->GetRightDirection(), context->m_VerticalRotation.Up()).normalizedSafe();


		const CVector worldInputVec = CVector::ProjectOnPlane(context->m_WorldInput, context->m_VerticalRotation.Up()).normalizedSafe();
		const float   worldInputMag = context->m_WorldInput.Length();

		const float speed = context->GetHorizontalVelocity().Magnitude();
		const float sign = CVector::Dot(rightDirection, worldInputVec) > 0.0f ? -1.0f : 1.0f; // RIGHT+ LEFT-

		//const float chestTiltFactor = static_cast<float>(std::min(std::abs(1.0f - CVector::Dot(frontDirection, worldInputVec)), 1.0));
		//const float turningFactor = -context->m_turningLeanAmount

		const bool isWalking = stateID == 0x016D78C8  /* CSonicStateWalk */
		                    || stateID == 0x016DA1EC  /* CSonicClassicContext::CStateWalk */

		                    // NEW!
		                    //|| stateID == 0x016D9950  /* CSonicClassicStateFall */
		;

		const bool isStanding = !(
			   stateID != 0x016D6438 // CSonicStateStand
			&& stateID != 0x016D7594 // CSonicStateStand::CNormal
			&& stateID != 0x016D75BC // CSonicStateStand::CSleep
			&& stateID != 0x016D9F24 // CSonicClassicStateStand
			&& stateID != 0x016D9F60 // CSonicClassicStateStand::CNormal
			&& stateID != 0x016D9F88 // CSonicClassicStateStand::CSleep
			&& stateID != 0x016D63B4 // CSonicStateMoveStop
			&& stateID != 0x016D91E4 // CSonicClassicStateMoveStop
			);

		const bool isSkateboarding = context->StateFlag(eStateFlag_InvokeSkateBoard);


		float targetLeanX = 0.0f;
		float targetLeanY = 0.0f;

		// Input-based targeting.

		if (worldInputMag > std::numeric_limits<float>::epsilon() && (isWalking || isSkateboarding) /**/ && !context->StateFlag(eStateFlag_OutOfControl))
		{
			// We want sonic to ANTICIPATE the direction he's about to move.
			const float lerpMaxSpeed = std::lerp(50.0f, 10.0f, CustomCameras::CameraExtensions::m_DefaultCamAmount);
			const float headTurnLeanAmount = fabs(1.0f - CVector::Dot(frontDirection, worldInputVec));
			const float leanAmountMultiplier = playerID == ClassicSonicPlayerID
			                                 ? std::lerp(3.0f, 1.0f, std::inverseLerp(0.0f, lerpMaxSpeed, speed))
			                                 : 1.0f;

			// Now we need out of control timer factored in.
			const float oocLerp = 1.0f - info->m_TimeUntilInControl * SCommonInfo::ms_TValueHelper;

			targetLeanX = fmin(headTurnLeanAmount * leanAmountMultiplier, 1.0f) * sign * worldInputMag * oocLerp;
		}

		// Set our raw values here for the eyes.

		info->m_targetEyes.x() = std::lerp(std::reversePower(targetLeanX, 3), info->m_targetEyes.x(), 0.05f);
		info->m_targetEyes.y() = std::lerp(targetLeanY, info->m_targetEyes.y(), 0.05f);

		if (worldInputMag < std::numeric_limits<float>::epsilon() || !isWalking)
		{
			info->m_target.x() = std::lerp(info->m_target.x(), targetLeanX, 0.2f);
		}
		else
		{
			info->m_target.x() = std::lerp(info->m_target.x(), targetLeanX, 0.28f);
		}

		const float turningFactor = info->m_target.x();
		//const float turningFactorDelayed = info->m_target.x() - info->m_targetDelayed.x() * 1.5f;
		//const float turningFactorDelayed1 = info->m_targetDelayedList->size() == 0
		//                                  ? turningFactor
		//                                  : info->m_target.x() - info->m_targetDelayedList->front().x() * 1.5f;

		const float turningFactorDelayed1 = -info->m_targetDelayedOffset.x();

		const float lookMultiplier = std::lerp(1.0f, 0.25f, std::inverseLerp(20.0f, 50.0f, speed));
		const float chestLookMultiplier = lookMultiplier * 0.65f;
		//const float chestLookMultiplier = std::lerp(1.0f, 0.25f, std::inverseLerp(20.0f, 50.0f, speed)) * 0.65f;

		// Let's try and make his head look ahead earlier at low speed
		const float headTurnFactor = std::reversePower(turningFactor, 3);

		hk2010_2_0::hkArray<hkQsTransform>* tArray = &pose->m_pAnimData->m_TransformArray;

		CQuaternion modernChestRotation = CQuaternion::Identity();
		if (playerID == ModernSonicPlayerID)
		{
			hkQsTransform* tChest = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_M::UpperChest);
			CQuaternion chestOriginalRotation = tChest->m_Rotation;
			tChest->m_Rotation = chestOriginalRotation * CQuaternion::FromEuler(turningFactor * chestLookMultiplier, 0, 0);
			modernChestRotation = tChest->m_Rotation;
		}

		// Classic's a special case sadly.
		else if (playerID == ClassicSonicPlayerID)
		{
			// Reminder Classic is a cartoon character so we can/should cheat the visuals based on camera angle.

			// Camera forward direction, if it exists.
			CVector cameraDirection = Core::GetCameraFrontVector(frontDirection);
			CVector cameraForward = CVector::ProjectOnPlane(cameraDirection, context->m_ModelUpDirection).normalizedSafe();

			const float cameraInterpLerpValue = std::inverseLerp(0.5f, 0.8f, CVector::Dot(cameraForward, frontDirection));

			// Since Classic doesn't have animations for leaning left/right, let's do that on our own.
			//const float leanMultiplier = std::inverseLerp(10.0f, 35.0f, speed);
			const float leanMultiplier = std::inverseLerp(8.0f, 20.0f, speed);
			const float lean = -turningFactor * leanMultiplier;


			// Chest
			hkQsTransform* tChest = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::Chest);
			CQuaternion chestOriginalRotation = tChest->m_Rotation;

			CQuaternion chestQuat = chestOriginalRotation
			                      * CQuaternion::FromEuler(std::lerp(0, turningFactor * lookMultiplier, cameraInterpLerpValue), 0, 0)
			                      * CQuaternion::FromEuler(0, 0, -lean * 2);
			tChest->m_Rotation = chestQuat;

			// Arms - These are kind of complicated.
			const float armRotationAmount = turningFactor * chestLookMultiplier * 0.5f;

			// Center transform for reference
			hh::math::CQuaternion centerRotation = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::CenterBody)->m_Rotation;

			CQuaternion armRotationEuler = CQuaternion::FromEuler(0, armRotationAmount, 0)
			                             * CQuaternion::FromEuler(0, 0, lean);
			CQuaternion armRotationAxisCorrected = centerRotation.inverse() * armRotationEuler * centerRotation;

			// LArm
			hkQsTransform* tLArm = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::LeftArm);
			tLArm->m_Rotation = armRotationAxisCorrected * tLArm->m_Rotation;

			// RArm
			hkQsTransform* tRArm = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::RightArm);
			tRArm->m_Rotation = armRotationAxisCorrected * tRArm->m_Rotation;

			// Neck?
			const CQuaternion neckAxisCorrected = centerRotation.inverse() * CQuaternion::FromEuler(0, 0, lean) * centerRotation;
			hkQsTransform* tNeck = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::Neck);
			tNeck->m_Rotation = neckAxisCorrected * tNeck->m_Rotation;
		}

		// Head movement for both
		hkQsTransform* tHead = playerID == ClassicSonicPlayerID
			? pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_C::Head)
			: pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex_M::Head);
		const CQuaternion originalRot_HEAD = tHead->m_Rotation;

		// Modern Sonic has to negate the chest rotation first before rotating his head.
		if (playerID == ModernSonicPlayerID)
		{
			//const CQuaternion rotation_HEAD = CQuaternion::FromEuler(-turningFactor * chestLookMultiplier, 0, 0)
			//                                  * originalRot_HEAD * CQuaternion::FromEuler(turningFactor * lookMultiplier, 0, 0);

			const CQuaternion rotation_HEAD
			    = CQuaternion::FromEuler(-turningFactor * chestLookMultiplier * 0.5f, 0, 0)
			    * originalRot_HEAD
			    * (modernChestRotation.inverse()
			        * CQuaternion::FromEuler(turningFactor * lookMultiplier, 0, 0)
			        * modernChestRotation);

			tHead->m_Rotation = rotation_HEAD;
		}
		// Classic can just rotate his head it's fine
		else
		{
			const CQuaternion rotation_HEAD = originalRot_HEAD * CQuaternion::FromEuler(turningFactor * lookMultiplier, 0, 0);
			tHead->m_Rotation = rotation_HEAD;
		}

		// Quills!
		if (playerID == ClassicSonicPlayerID)
		{
			for (int i = (int)BoneIndex_C::BCQuill; i <= (int)BoneIndex_C::TRQuill; ++i)
			{
				hkQsTransform* tQuill = pose->m_pAnimData->m_TransformArray.GetIndex(i);
				CQuaternion quillRot = tQuill->m_Rotation;
				tQuill->m_Rotation = quillRot * CQuaternion::FromEuler(0, 0, -turningFactor * lookMultiplier * 0.65f);
			}
		}
		else if (playerID == ModernSonicPlayerID)
		{
			// Check for bone name.
			// TODO: Do a search for ALL bone names, and save their index somewhere. Better for alt. skeleton types.

			// Old: checked for a single bone comparison... Not a good idea???
			/*
			auto* const havokSkel = static_cast<hkaAnimatedSkeleton*>(pose->m_pHavokSkeleton);
			auto* const bones = (hvkArray<hkaBone>*)&havokSkel->getSkeleton()->m_bones;
			std::string quillBoneName = bones->GetIndex(BoneIndex_M::BCQuill_INNER)->m_name.cString();
			int quillOffset = quillBoneName == "Needle_B_C" ? 0 : 1;
			*/

			// New: check for the ROOT BONE NAME like a normal person.
			auto* const havokSkel = static_cast<hkaAnimatedSkeleton*>(pose->m_pHavokSkeleton);
			std::string skelName = havokSkel->getSkeleton()->m_name.cString();
			int quillOffset = skelName == "chr_Sonic_HD" ? 0 : 1;

			// Quill base
			for (int i = (int)BoneIndex_M::BCQuill_INNER - quillOffset; i <= (int)BoneIndex_M::TRQuill_INNER - quillOffset; i += 2)
			{
				hkQsTransform* tQuill = pose->m_pAnimData->m_TransformArray.GetIndex(i);
				CQuaternion quillRot = tQuill->m_Rotation;
				//tQuill->m_Rotation = quillRot * CQuaternion::FromEuler(-turningFactor * lookMultiplier * 0.45f, 0, 0);
				//tQuill->m_Rotation = CQuaternion::FromEuler(-turningFactor * lookMultiplier * 0.30f, 0, 0) * quillRot;
				const float yRotation = turningFactorDelayed1 * lookMultiplier * Core::QuillMultiplierBase * Core::QuillFlopStrength;
				tQuill->m_Rotation = CQuaternion::FromEuler(yRotation, 0, 0) * quillRot;
			}
			// Quill tips
			for (int i = (int)BoneIndex_M::BCQuill_OUTER - quillOffset; i <= (int)BoneIndex_M::TRQuill_OUTER - quillOffset; i += 2)
			{
				hkQsTransform* tQuill = pose->m_pAnimData->m_TransformArray.GetIndex(i);
				CQuaternion quillRot = tQuill->m_Rotation;
				//tQuill->m_Rotation = quillRot * CQuaternion::FromEuler(0, 0, -turningFactor * lookMultiplier * 0.25f);
				const float yRotation = -turningFactor * lookMultiplier * Core::QuillMultiplierTip * Core::QuillFlopStrength;
				tQuill->m_Rotation = CQuaternion::FromEuler(0, 0, yRotation) * quillRot;
			}
		}


		info->m_targetDelayed.x() = std::lerp(info->m_targetDelayed.x(), info->m_target.x(), 0.65f);


		// Quill jiggle physics.
		// Source: https://gamedev.stackexchange.com/questions/105728/how-to-program-a-fully-controllable-spring-damped-motion

		const float frameDeltaTime = info->m_PhysicsDeltaTime * 60.0f;
		const float dt = frameDeltaTime * 0.1f;

		info->m_targetDelayedOffset += info->m_target;

		info->m_targetDelayedVelocity -= dt * (Core::QuillStiffness * info->m_targetDelayedOffset + Core::QuillFriction * info->m_targetDelayedVelocity);
		info->m_targetDelayedOffset   += dt * info->m_targetDelayedVelocity;


		// Debug shit again
		if (Core::debugBoneWithInput)
		{
			hkQsTransform* tDebug = pose->m_pAnimData->m_TransformArray.GetIndex(Core::boneToAffect);
			//auto tDebug = pose->m_pAnimData->m_TransformArray.GetIndex(7);

#define _DBG_ROTATION

#ifdef _DGB_TRANSLATION
			CVector dbgPosition = *(CVector*)&tDebug->getTranslation();
			const float dbgScaleFactor = 2.0f;
			dbgPosition += CVector(*Input::rightStickX * dbgScaleFactor,
				*Input::rightStickY * dbgScaleFactor,
				(*Input::rightTrigger - *Input::leftTrigger) * dbgScaleFactor);
			tDebug->setTranslation(*(hkVector4*)&dbgPosition);
#endif
#ifdef _DBG_ROTATION
			auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();

			CQuaternion dbgRotation = tDebug->m_Rotation;
			const float dbgScaleFactor = 360.0f * DEG2RAD;
			CQuaternion dbgNewRotation = CQuaternion::FromEuler(input->RightStickHorizontal * dbgScaleFactor,
				 input->RightStickVertical * dbgScaleFactor,
				(input->RightTrigger - input->LeftTrigger) * dbgScaleFactor);

			//CQuaternion dbgFinalRotation = dbgNewRotation.inverse() * dbgRotation * dbgNewRotation * dbgNewRotation;
			CQuaternion dbgFinalRotation = dbgRotation * dbgNewRotation;

			tDebug->m_Rotation = dbgFinalRotation;
#endif
		}
	}
void ProceduralAnimation::Impl::HeadTurnClassic(hh::anim::CAnimationPose* pose, Sonic::Player::CPlayerSpeed* player)
{
	using namespace hh::math;
	using namespace hh::anim;

	auto context = player->GetContext();
	auto info = SCommonInfo::Get(context);

	enum class BoneIndex
		{
			CenterBody = 7,
			Neck = 8,
			Head = 9,

			BCQuill = 50,
			BLQuill = 51,
			BRQuill = 52,
			TCQuill = 53,
			TLQuill = 54,
			TRQuill = 55,

			LeftLeg = 56,
			RightLeg = 74,
			LeftArm = 92,
			RightArm = 124,
			Chest = 156,
		};

	//////////////
	// Code start
	//////////////
	//-----------------------------------------

	// We don't want to do anything if we're not in the right state.
	const auto state = player->m_StateMachine.GetCurrentState().get();
	if (!state)
		return;
	const uint32_t stateID = *(uint32_t*)state;

	const CVector frontDirection = CVector::ProjectOnPlane(context->GetFrontDirection(), context->m_VerticalRotation.Up()).normalizedSafe();
	const CVector rightDirection = CVector::ProjectOnPlane(context->GetRightDirection(), context->m_VerticalRotation.Up()).normalizedSafe();

	const CVector worldInputVec = CVector::ProjectOnPlane(context->m_WorldInput, context->m_VerticalRotation.Up()).normalizedSafe();
	const float   worldInputMag = context->m_WorldInput.Length();

	const float speed = context->GetHorizontalVelocity().Magnitude();
	const float sign = CVector::Dot(rightDirection, worldInputVec) > 0.0f ? -1.0f : 1.0f; // RIGHT+ LEFT-

	const bool isWalking  =  stateID == 0x016DA1EC  /* CSonicClassicContext::CStateWalk */

	                    // NEW!
	                    //|| stateID == 0x016D9950  /* CSonicClassicStateFall */
	;

	// UNDONE: Not necessary?
	const bool isStanding = !(
		   stateID != 0x016D9F24 // CSonicClassicStateStand
		&& stateID != 0x016D9F60 // CSonicClassicStateStand::CNormal
		&& stateID != 0x016D9F88 // CSonicClassicStateStand::CSleep
		&& stateID != 0x016D91E4 // CSonicClassicStateMoveStop
		);

	const bool isSkateboarding = context->StateFlag(eStateFlag_InvokeSkateBoard);

	float targetLeanX = 0.0f;
	float targetLeanY = 0.0f;

	float targetLeanXEyes = 0.0f;

	// Looking towards input.
	if (worldInputMag > FLT_EPSILON && (isWalking || isSkateboarding) && !context->StateFlag(eStateFlag_OutOfControl))
	{
		// We want sonic to ANTICIPATE the direction he's about to move.
		const float lerpMaxSpeed = std::lerp(50.0f, 10.0f, CustomCameras::CameraExtensions::m_DefaultCamAmount);
		const float lerpSpeedA   = std::inverseLerp(1.0f, 10.0f, speed);
		const float lerpSpeedB   = std::inverseLerp(0.0f, lerpMaxSpeed, speed);

		const float headTurnLeanAmount = fabs(1.0f - CVector::Dot(frontDirection, worldInputVec));
		//const float leanXAmount = headTurnLeanAmount > 0.0001 ? headTurnLeanAmount * 5.0f : headTurnLeanAmount;
		const float leanXAmount     = headTurnLeanAmount * (4.0f * (1.0f - std::QuickPower(lerpSpeedA, 1)) + 1.0f);
		const float leanXAmountEyes = headTurnLeanAmount * 5.0f;

		const float leanAmountMultiplier = std::lerp(3.0f, 1.0f, lerpSpeedB);

		// Now we need out of control timer factored in.
		const float outOfControlLerp = 1.0f - info->m_TimeUntilInControl * SCommonInfo::ms_TValueHelper;

		//targetLeanX = fmin(headTurnLeanAmount * leanAmountMultiplier, 1.0f) * sign * worldInputMag * outOfControlLerp;
		targetLeanX     = fmin(leanXAmount     * leanAmountMultiplier, 1.0f) * sign * worldInputMag * outOfControlLerp;
		targetLeanXEyes = fmin(leanXAmountEyes * leanAmountMultiplier, 1.0f) * sign * worldInputMag * outOfControlLerp;
	}

	// FIXME: Eyes are an excellent example of why this section shouldn't be done here.
	info->m_targetEyes.x() = std::lerp(std::reversePower(targetLeanXEyes, 3), info->m_targetEyes.x(), 0.05f);
	info->m_targetEyes.y() = std::lerp(targetLeanY, info->m_targetEyes.y(), 0.05f);

	if (worldInputMag < FLT_EPSILON || !isWalking)
	{
		info->m_target.x() = std::lerp(info->m_target.x(), targetLeanX, 0.2f);
	}
	else
	{
		info->m_target.x() = std::lerp(info->m_target.x(), targetLeanX, 0.28f);
	}

	const float turningFactor = info->m_target.x();
	const float turningFactorDelayed1 = -info->m_targetDelayedOffset.x();

	const float lookMultiplier = std::lerp(1.0f, 0.25f, std::inverseLerp(20.0f, 50.0f, speed));
	const float chestLookMultiplier = lookMultiplier * 0.65f;

	// (Failed) attempt at making the head look ahead before anything else.
	const float headTurnFactor = std::reversePower(turningFactor, 3);

	// Above const values are used, so we can apply our physics for the next frame below.
	info->m_targetDelayed.x() = std::lerp(info->m_targetDelayed.x(), info->m_target.x(), 0.65f);

	// Jiggle physics implementation: https://gamedev.stackexchange.com/questions/105728/how-to-program-a-fully-controllable-spring-damped-motion
	const float dt = 0.1f;

	info->m_targetDelayedOffset += info->m_target;

	info->m_targetDelayedVelocity -= dt * (Core::QuillStiffness * info->m_targetDelayedOffset + Core::QuillFriction * info->m_targetDelayedVelocity);
	info->m_targetDelayedOffset += dt * info->m_targetDelayedVelocity;

	//////////////////////////////
	// Manipulate bones
	// 
	// Reminder: Classic is a cartoon character, so we can/should cheat the visuals based on camera angle.
	// ---------------------------------------

	// Camera forward direction, if it exists.
	// Side note, if you don't have a camera, something terrible might have happened.
	CVector cameraDirection = Core::GetCameraFrontVector(frontDirection);
	CVector cameraForward = CVector::ProjectOnPlane(cameraDirection, context->m_ModelUpDirection).normalizedSafe();

	const float cameraInterpLerpValue = std::inverseLerp(0.5f, 0.8f, CVector::Dot(cameraForward, frontDirection));

	// Since Classic doesn't have animations for leaning left/right, let's do that on our own.
	//const float leanMultiplier = std::inverseLerp(10.0f, 35.0f, speed);
	const float leanMultiplier = std::inverseLerp(8.0f, 20.0f, speed);
	const float lean = -turningFactor * leanMultiplier;

	// Chest
	hkQsTransform* tChest = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::Chest);
	CQuaternion chestOriginalRotation = tChest->m_Rotation;

	CQuaternion chestQuat = chestOriginalRotation
	                        * CQuaternion::FromEuler(std::lerp(0, turningFactor * lookMultiplier, cameraInterpLerpValue), 0, 0)
	                        * CQuaternion::FromEuler(0, 0, -lean * 2);
	tChest->m_Rotation = chestQuat;

	// Arms - These are kind of complicated.
	const float armRotationAmount = turningFactor * chestLookMultiplier * 0.5f;

	// Center transform for reference
	hh::math::CQuaternion centerRotation = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::CenterBody)->m_Rotation;

	CQuaternion armRotationEuler = CQuaternion::FromEuler(0, armRotationAmount, 0)
	                               * CQuaternion::FromEuler(0, 0, lean);
	CQuaternion armRotationAxisCorrected = centerRotation.inverse() * armRotationEuler * centerRotation;

	//// LArm
	hkQsTransform* tLArm = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::LeftArm);
	tLArm->m_Rotation = armRotationAxisCorrected * tLArm->m_Rotation;

	//// RArm
	hkQsTransform* tRArm = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::RightArm);
	tRArm->m_Rotation = armRotationAxisCorrected * tRArm->m_Rotation;

	//// Neck?
	const CQuaternion neckAxisCorrected = centerRotation.inverse() * CQuaternion::FromEuler(0, 0, lean) * centerRotation;
	hkQsTransform* tNeck = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::Neck);
	tNeck->m_Rotation = neckAxisCorrected * tNeck->m_Rotation;

	// Head movement.
	hkQsTransform* tHead = pose->m_pAnimData->m_TransformArray.GetIndex(BoneIndex::Head);
	const CQuaternion rotation_HEAD = tHead->m_Rotation * CQuaternion::FromEuler(turningFactor * lookMultiplier, 0, 0);
	tHead->m_Rotation = rotation_HEAD;

	// Quills!
	for (int i = (int)BoneIndex::BCQuill; i <= (int)BoneIndex::TRQuill; ++i)
	{
		hkQsTransform* tQuill = pose->m_pAnimData->m_TransformArray.GetIndex(i);
		CQuaternion quillRot = tQuill->m_Rotation;
		tQuill->m_Rotation = quillRot * CQuaternion::FromEuler(0, 0, -turningFactor * lookMultiplier * 0.65f);
	}

	/////////////////////
	// Debug
	//-------------------

	if (Core::debugBoneWithInput)
	{
		hkQsTransform* tDebug = pose->m_pAnimData->m_TransformArray.GetIndex(Core::boneToAffect);
		//auto tDebug = pose->m_pAnimData->m_TransformArray.GetIndex(7);

#define _DBG_ROTATION

#ifdef _DGB_TRANSLATION
		CVector dbgPosition = *(CVector*)&tDebug->getTranslation();
		const float dbgScaleFactor = 2.0f;
		dbgPosition += CVector(*Input::rightStickX * dbgScaleFactor,
			*Input::rightStickY * dbgScaleFactor,
			(*Input::rightTrigger - *Input::leftTrigger) * dbgScaleFactor);
		tDebug->setTranslation(*(hkVector4*)&dbgPosition);
#endif
#ifdef _DBG_ROTATION
		auto* const input = &Sonic::CInputState::GetInstance()->GetPadState();

		CQuaternion dbgRotation = tDebug->m_Rotation;
		const float dbgScaleFactor = 360.0f * DEG2RAD;
		CQuaternion dbgNewRotation = CQuaternion::FromEuler(input->RightStickHorizontal * dbgScaleFactor,
			 input->RightStickVertical * dbgScaleFactor,
			(input->RightTrigger - input->LeftTrigger) * dbgScaleFactor);

		//CQuaternion dbgFinalRotation = dbgNewRotation.inverse() * dbgRotation * dbgNewRotation * dbgNewRotation;
		CQuaternion dbgFinalRotation = dbgRotation * dbgNewRotation;

		tDebug->m_Rotation = dbgFinalRotation;
#endif
	}
}
#pragma endregion

// Entry point
//---------------------------------------

void ProceduralAnimation::Init()
{
	using namespace AnimPosePatch;
	using namespace Sonic::Player;

	using namespace Core;

	//WRITE_MEMORY(0x00E57740, byte, 0xC3) // cancels out "fix horizontal rotation" as it were
	

	// Patch this data's size + initialize last field to nullptr.
	WRITE_MEMORY(0x006CB29D, uint8_t, sizeof(CAnimationPose_Alternate::Map))
	INSTALL_HOOK(_ConstructCAnimationPose_Common)
	INSTALL_HOOK(_ConstructCAnimationPose_Msn)

	// Set our back-reference in both sonic classes.
	INSTALL_HOOK(_SonicInitializeRenderables_Modern)
	INSTALL_HOOK(_SonicInitializeRenderables_Classic)

	// This lets us do procgen animation
	INSTALL_HOOK(_CAnimationPose_SampleAnimation)

	// Eye animation

	// DEBUG: Do this if we want to cycle through bones and see what makes what tick.
	// The "important" bones have been indexed so this is mostly vestigial right now.
	// NOTE: While that's the case, it's important to keep this around in the event we missed something.
	INSTALL_HOOK(InitializeApplicationParams_GPM)
}

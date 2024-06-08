#include "Common.h"

// Hook includes.
#include "Config.h"
#include "PhysicsGameplay.h"
#include "ProceduralAnimation.h"

namespace Common
{
	using namespace Sonic::Player;

	struct StateFlagsAppened
	{
	private:
		INSERT_PADDING(0x10) {}; // original mem size
	public:
		SCommonInfo* m_Info = nullptr;
	};

	SCommonInfo* SCommonInfo::Get(CPlayerSpeedContext* context)
	{
		return reinterpret_cast<StateFlagsAppened*>(context->m_pStateFlag)->m_Info;
	}

	// RAII
	//-----

	HOOK(void*, __stdcall, _CPlayerSpeedContextCTOR, 0x00E668B0, CPlayerSpeedContext* context, CPlayer* player)
	{
		void* const result = original_CPlayerSpeedContextCTOR(context, player);
		reinterpret_cast<StateFlagsAppened*>(context->m_pStateFlag)->m_Info = new SCommonInfo;

		return result;
	}
	HOOK(void*, __fastcall, _CPlayerSpeedContextDTOR, 0x00E62980, CPlayerSpeedContext* context)
	{
		delete reinterpret_cast<StateFlagsAppened*>(context->m_pStateFlag)->m_Info;
		return original_CPlayerSpeedContextDTOR(context);
	}


	// General update methods for Sonic, using a single hook so we can more deliberately control the order of events.
	//--------------------------------------------------------------------------------------------------------------

	HOOK(void, __fastcall, _ClassicUpdate, 0x00DDABA0, Sonic::Player::CPlayerSpeed* This, void*, const hh::fnd::SUpdateInfo& updateInfo)
	{

		const float originalGravity = Gameplay_Adventure::Extras::PatchParametersPreUpdate(This, updateInfo);

		// Hack: force sonic to not walk on water if SA1 mode.
		if (Config::ms_IsSA1LevelPreset)
			This->GetContext()->StateFlag(eStateFlag_AcceptBuoyancyForce) = false;

		ProceduralAnimation::Core::RestoreUVAnim(This);
		original_ClassicUpdate(This, nullptr, updateInfo);
		ProceduralAnimation::Core::UpdateUVAnim(This, updateInfo);

		Gameplay_Adventure::Extras::PatchParametersPostUpdate(This, originalGravity);

	}
	HOOK(void, __fastcall, _ModernUpdate, 0x00E17E80, Sonic::Player::CPlayerSpeed* This, void*, const hh::fnd::SUpdateInfo& updateInfo)
	{
		ProceduralAnimation::Core::RestoreUVAnim(This);
		original_ModernUpdate(This, nullptr, updateInfo);
		ProceduralAnimation::Core::UpdateUVAnim(This, updateInfo);
	}
}

void Common::Init()
{
	// Patch to give CPlayerSpeedContext extra data to play with, i.e. look direction vector.
	WRITE_MEMORY(0x00E67DD1, uint8_t, sizeof(StateFlagsAppened))

	// Initialize & free our memory as needed.
	INSTALL_HOOK(_CPlayerSpeedContextCTOR)
	INSTALL_HOOK(_CPlayerSpeedContextDTOR)

	// Update hooks
	INSTALL_HOOK(_ClassicUpdate)
	INSTALL_HOOK(_ModernUpdate)
}

#pragma once
namespace ProceduralAnimation
{
	void Init();

	// C/C++ Function Pointers are the devil, but we need one here for a "vtable."
	typedef void (*FPtrUpdateProcedural)(Hedgehog::Animation::CAnimationPose*, void*);

	class ProceduralData
	{
	public:
		Sonic::CGameObject* m_pObject = nullptr;
		FPtrUpdateProcedural UpdateProcedural = nullptr;

		// Methods
		static ProceduralData* Get(Hedgehog::Animation::CAnimationPose* pose);
		void SetUpdateFunction(void* functionPointer);
	};

	namespace Core
	{
		void UpdateUVAnim(Sonic::Player::CPlayerSpeed* This, const hh::fnd::SUpdateInfo& updateInfo);
		void RestoreUVAnim(Sonic::Player::CPlayerSpeed* This);
	}
}
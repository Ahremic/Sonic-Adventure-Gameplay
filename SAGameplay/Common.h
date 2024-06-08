#pragma once

namespace Common
{
	struct SCommonInfo
	{
		typedef hh::math::CVector  CVector;
		typedef hh::math::CVector2 CVector2;

		static constexpr float ms_MaxOutOfControlTime = 0.65f;
		static constexpr float ms_TValueHelper = 1.0 / ms_MaxOutOfControlTime;

		struct Params
		{
			bool m_Initialized = false;
			float m_JumpPower = 0.0f;
			float m_Gravity = 0.0f;
		};
		float m_TimeUntilInControl = 0.0f;
		float m_PhysicsDeltaTime = 1.0f / 60.0f;

		// Head turning
		CVector2 m_targetEyes = { 0.0f, 0.0f };
		CVector2 m_target = { 0.0f, 0.0f };
		CVector2 m_targetDelayed = { 0.0f, 0.0f };
		CVector2 m_targetDelayedOffset = { 0.0f, 0.0f };
		CVector2 m_targetDelayedVelocity = { 0.0f, 0.0f };

		CVector2 m_OriginalEyeOffsetL = { 0.0f, 0.0f };
		CVector2 m_OriginalEyeOffsetR = { 0.0f, 0.0f };

		bool m_UVsInitialized = false;
		Params m_Params;

		static SCommonInfo* Get(Sonic::Player::CPlayerSpeedContext* context);
	};
	//static_assert(sizeof(SCommonInfo) < 0xFF, "Too much data in SCommonInfo");

	void Init();
}

#pragma once
namespace HUD
{
	class SoundManager
	{
	public:
		static inline float s_StartingVolume = 1.0f;
		static inline float s_TargetVolume   = 1.0f;
		static inline float s_CurrentVolume  = 1.0f;
		static inline float s_CurrentTime = 0.0f;

		// simple func, should be fine.
		static inline float Lerp(float a, float b, float t)
		{
			return (b - a) * t + a;
		}

		static void Init();
		static void OnFrame();
	};
}
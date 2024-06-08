#pragma once
namespace Cameras
{
	void Init();
	// TODO: remove this when class members are appended to classic sonic.
	void OnFrame();
}

namespace CustomCameras
{
#define STATIC static inline

	// TODO: MAKE THESE CLASS MEMBERS FOR SONIC? Hard to say, because the camera *ONLY* ever affects player 1...
	class CameraExtensions
	{
	public:
		STATIC float m_DefaultCamAmount = 0.0f;
		STATIC float m_PathCamAmount = 0.0f;
		STATIC hh::math::CVector m_CameraMoveDirection = hh::math::CVector::Forward();
	};

#undef STATIC
}
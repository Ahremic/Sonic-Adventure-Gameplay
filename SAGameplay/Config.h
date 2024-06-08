#pragma once

enum class DefaultCameraType
{
	CAMERA_SA1 = 0,
	CAMERA_GENS = 1,
	CAMERA_CUSTOM = 2,
};
enum class Playstyle
{
	STYLE_AUTO = 0,
	STYLE_SA1 = 1,
	STYLE_GENS = 2,
	STYLE_CUSTOM = 3,
};

struct ModInfo;

class Config
{
#define STATIC static inline
	typedef DefaultCameraType TCAM;
	typedef Playstyle STYL;
public:
	static void StartConfiguration(ModInfo* modInfo);
	static void ApplyPatches();

	// Quality of Life settings for another time.
	STATIC bool ms_SmartCameraControls = false;
	STATIC TCAM ms_CameraType          = DefaultCameraType::CAMERA_SA1;
	STATIC STYL ms_Playstyle           = Playstyle::STYLE_AUTO;
	STATIC bool ms_CustomCameraCollision = true;
	STATIC bool ms_CustomCameraDelay = true;

	STATIC float ms_EFighterG_RotAngleY = 0.0f;
	STATIC float ms_EFighterG_RotAngleX = 0;


	STATIC bool ms_InvertTriggers   = true;
	STATIC bool ms_InvertRightStick = true;

	STATIC bool ms_ReduceAirFriction = false;

	STATIC bool ms_IsSA1LevelPreset = false;

#undef STATIC
};

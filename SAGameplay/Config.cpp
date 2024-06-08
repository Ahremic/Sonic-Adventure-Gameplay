// ReSharper disable CppPolymorphicClassWithNonVirtualPublicDestructor
#include "Config.h"
#include "Extra\LostCodeLoaderV2.h"
#include "Extra\RegistryHelper.h"
#include "ModLoader.h"


void Config::StartConfiguration(ModInfo* modInfo)
{
	if (!GetModLoaderHModule())
	{
		const wchar_t* msg =
			L"This mod requires Hedgehog Engine 1 Mod Loader (HE1ML).\n"
			L"Go to the Settings tab of HedgeModManager, check for updates,\n"
			L"and make sure to install & enable the mod loader.\n"
			;

		MessageBox(nullptr, msg,
			TEXT("HE1ML NOT DETECTED!"), MB_ICONERROR);

		exit(-1);
	}

	const INIReader reader("Config.ini");
	ms_InvertRightStick = reader.GetBoolean("Config", "InvertStick", false);
	ms_InvertTriggers   = reader.GetBoolean("Config", "InvertTriggers", false);

	// Unused for now
	ms_SmartCameraControls = reader.GetBoolean("QOL", "SmartCamera", false);
	ms_CameraType = (DefaultCameraType)reader.GetInteger("QOL", "CameraBehavior", 0);

	ms_CustomCameraCollision = reader.GetBoolean("CustomCamera", "CameraCollision", true);
	ms_CustomCameraDelay     = reader.GetBoolean("CustomCamera", "CameraDelay", true);
}


// Time to make configurable stuff for Stage.stg.xml

namespace XmlExtensions
{
	// TODO: Ask Skyth how to go about these...

	// No definition necessary as this type is only ever passed as a pointer for the game to handle.
	// Only declaring this in the first place out of habit really.
	class XMLData;

	class CXMLTypeSLTxt // ReSharper disable once CppInconsistentNaming  // NOLINT(cppcoreguidelines-special-member-functions)
	{
		class CMember  // NOLINT(cppcoreguidelines-special-member-functions)
		{
		public:
			virtual void Destroy(bool condition = false) {}
		};

	public:
		void ApplyParams(XMLData* data)
		{
			fpMakeParams(this, data);
		}

		void AddFloat(const char* name, float* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3AA0, CXMLTypeSLTxt * This, const char* a2, float* a3);
			AddParam(this, name, value);
		}
		void AddVector(const char* name, Hedgehog::Math::CVector* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3A00, CXMLTypeSLTxt * This, const char* a2, Hedgehog::Math::CVector* a3);
			AddParam(this, name, value);
		}
		void AddBool(const char* name, bool* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3B40, CXMLTypeSLTxt * This, const char* a2, bool* a3);
			AddParam(this, name, value);
		}

		CXMLTypeSLTxt()
		{
			fpCtor(this);
		}
		~CXMLTypeSLTxt()
		{
			// Manually invoke destructor cus this is what the game does.
			// Destructor is called "Destroy" right now because it's not a conventional destructor,
			// rather it's a virtual func with a condition that's always false in this use case.
			m_pMember->Destroy();
			fpDtor(this, m_pMember);
		}

	private:
		static void NOINLINE fpCtor(CXMLTypeSLTxt* This)
		{
			constexpr uint32_t func = 0x00CE3D20;
			__asm
			{
				mov eax, This
				call func
			}
		}
		static void NOINLINE fpDtor(CXMLTypeSLTxt* This, CMember* member)
		{
			constexpr uint32_t func = 0x0059A410;
			__asm
			{
				mov ecx, This
				mov eax, member
				call func
			}
		}
		static void NOINLINE fpMakeParams(CXMLTypeSLTxt* This, XMLData* data)
		{
			constexpr uint32_t func = 0x00CE31C0;
			__asm
			{
				mov eax, data
				push This
				call func
			}
		}

		BB_INSERT_PADDING(0x1018) {};
		CMember* m_pMember = nullptr;
	};
	BB_ASSERT_SIZEOF(CXMLTypeSLTxt, 0x101C);

	/////////////
	/// Hooks ///
	/////////////

	HOOK(void, __fastcall, _ParseStgXML_ParamSonic, 0x011D2670, void* This, void*, XMLData* xmlData)
	{
		original_ParseStgXML_ParamSonic(This, nullptr, xmlData);

		CXMLTypeSLTxt properties;

		// Initialize to False when loading a level, then override to True if specified in the XML deliberately.
		// That way, levels that support adventure physics, but want compatibility with vanilla, can just add this property.
		// HACK: ok set to true just for withermin bby
		Config::ms_IsSA1LevelPreset = true;
		properties.AddBool("IsAdventure", &Config::ms_IsSA1LevelPreset);
		properties.ApplyParams(xmlData);
	}
}

void Config::ApplyPatches()
{
	using namespace XmlExtensions;

	INSTALL_HOOK(_ParseStgXML_ParamSonic)
}

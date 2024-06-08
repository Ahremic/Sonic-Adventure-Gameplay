#include <LostCodeLoader.h>

#undef _USE_LOG

#include "Common.h"
#include "PhysicsGameplay.h"

#include "JumpBall.h"
#include "Cameras.h"
#include "PhysicsSpindash.h"
#include "ProceduralAnimation.h"
#include "Config.h"

#include "SetObjectMods.h"
#include "SetObjectsCustom.h"

// Fix annoying oversight so I can actually access this without recalculating it on the GPU.
// Credit to Skyth fixing this: https://github.com/blueskythlikesclouds/GenerationsPBRShaders/blob/master/Source/GenerationsPBRShaders/ShaderHandler.cpp#L732
HOOK(void, __fastcall, _CRenderingDeviceSetViewMatrix, hh::mr::fpCRenderingDeviceSetViewMatrix,
    hh::mr::CRenderingDevice* This, void*, const Eigen::Matrix4f& viewMatrix)
{
    const Eigen::Matrix4f inverseViewMatrix = viewMatrix.inverse();
    This->m_pD3DDevice->SetPixelShaderConstantF(94, inverseViewMatrix.data(), 4);

    original_CRenderingDeviceSetViewMatrix(This, nullptr, viewMatrix);
}

int NOINLINE CreateReflectionRenderMatrix(const Hedgehog::Math::CVector& a1,
    const Hedgehog::Math::CVector& a2,
    const Hedgehog::Math::CVector& a3,
    const Hedgehog::Math::CVector& a4,
    float a5,
    const Hedgehog::Math::CMatrix44& a6,
    const Hedgehog::Math::CVector& a7,
    int a8)
{
    static constexpr uint32_t func = 0x6584A0;
    int result = 0;

    __asm
    {
        push a8
        push a7
        push a6
        push a5

        mov edx, a1
        mov ecx, a2
        mov edi, a3
        mov esi, a4

        call func
        add esp, 10h

        mov result, eax
    }

    return result;
}

HOOK(bool, __fastcall, _ClipPatch, 0x658860, void* This, void*, int a2, hh::math::CMatrix44& a3, hh::math::CMatrix44& cameraMatrix, int a5, Hedgehog::Math::CVector& a6)
{
    using namespace hh::math;
    CMatrix44* mat = !*((char*)This + 0x224) ? (CMatrix44*)((char*)This + 0x200) : (CMatrix44*)((char*)This + 0x20 * a2);

    CVector* pVecs = (CVector*)mat;

    CVector direction = -cameraMatrix.GetVectorFromRow(2);
    CVector up = cameraMatrix.GetVectorFromRow(1);

    FUNCTION_PTR(void, __thiscall, GetPositionFromMatrix, 0x006F1730, const CMatrix44 & m, CVector * out_v);

    CVector position(0, 0, 0);
    GetPositionFromMatrix(cameraMatrix, &position);

    CVector refVector = pVecs[0]; // reflection vector is the first vec in this matrix

    if (CVector::Dot(refVector, position) < 0) // Only invert our components if the camera is BEHIND the mirror.
    {
        direction = direction.Reflect(refVector);
        up = up.Reflect(refVector);
        position = position.Reflect(refVector);
        refVector = -refVector;
    }

    CreateReflectionRenderMatrix(up, direction, a6, refVector, pVecs[1].x(), a3, position, a5);

    return true;
}

EXPORT void Init(ModInfo* modInfo)
{
    //#define _DEBUG
#ifdef _DEBUG
    MessageBoxA(nullptr, "Attach VS via CTRL/SHIFT + ALT + P.\nAttach Cheat Engine when convenient.", "DEBUG READY !", MB_OK);
#endif

    Config::StartConfiguration(modInfo);
    Config::ApplyPatches();

    Gameplay_Adventure::Init();
    Gameplay_Spindash::Init();
    Common::Init();
    JumpBall::Init();
    ProceduralAnimation::Init();

    Cameras::Init();

    SetObjectMods::Init();
    SetObjectsCustom::Init();

    // Fix graphical hiccups.
    INSTALL_HOOK(_CRenderingDeviceSetViewMatrix)
    INSTALL_HOOK(_ClipPatch)
}

// TODO: Get rid of this old implementation entirely.
EXPORT void OnFrame()
{
    JumpBall::OnFrame();
}

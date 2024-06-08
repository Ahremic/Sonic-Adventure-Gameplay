#include "JumpBall.h"


FUNCTION_PTR(void**, __thiscall, CSharedString__ct, 0x006621A0, void** This, const char* a2);
FUNCTION_PTR(int*, __thiscall, CSharedString__dt, 0x00661550, void* This);
FUNCTION_PTR(int, __thiscall, SetSphereMdl, 0x00E4FA40, void* This, int a2);

void __cdecl pSetBallAnimation(const Hedgehog::Base::CSharedString& a1, Sonic::Player::CPlayerSpeedContext* a2)
{
	//@<eax>@<ecx>
	uint32_t func = 0x00E4F770;
	__asm
	{
		mov eax, a1
		mov ecx, a2
		call func
	}
}
void SetBallAnimation(Sonic::Player::CPlayerSpeedContext* context, const Hedgehog::Base::CSharedString& text)
{
	pSetBallAnimation(text, context);
}
void SetBallAnimation(Sonic::Player::CPlayerSpeedContext* context, const char* text)
{
	pSetBallAnimation(text, context);;
}


using namespace Sonic::Message;
class SpinBall : public Sonic::CGameObject
{
public:
	boost::shared_ptr<hh::mr::CSingleElement> m_spBallRenderable;
	boost::shared_ptr<hh::anim::CAnimationPose> m_spAnimationPose;
	hh::math::CVector     m_position;
	hh::math::CQuaternion m_rotation;
	float rotationDeg = 0.0f;
	float rotationRate = 20.0f;
	bool visible = true;

	void UpdateParallel(const hh::fnd::SUpdateInfo& updateInfo) override
	{
		using namespace hh::math;
		// TODO: Get this working for Super Sonic.

		const float deltaTime = updateInfo.DeltaTime;
		rotationDeg += deltaTime * rotationRate;

		const auto playerID = GetGameDocument()->m_pMember->m_PlayerIDs.begin()[0];

		// GetPosition is flawed for what we're doing, unfortunately.
		//SendMessageImm<MsgGetPosition>(playerID, m_position);
		// Instead, we need to get the model position.
		const auto context = static_cast<Sonic::Player::CPlayerSpeed*>(m_pMessageManager->GetMessageActor(playerID))->m_spContext.get();
		m_position = context->m_spModelMatrixNode->m_WorldMatrix.GetVectorFromColumn(3);

		// Todo: See why this isn't working with blueblur update...

		//SendMessageImm<MsgGetRotation>(playerID, m_rotation);
		//SendMessageImm<MsgGetRotation>(playerID, &m_rotation);
		SendMessageImm(playerID, MsgGetRotation(&m_rotation));

		// Use the animation pose to get Sonic's model scale, because Classic gets shrunk in the hubworld.
		const float modelScale = context->m_pPlayer->GetAnimationPose()->m_Scale;

		const float scale = (visible ? 1.5f : 0.0005f) * modelScale;
		const float offset = 0.45f * modelScale;

		const Eigen::Matrix<float, 3, 1, 0> upVec = m_rotation * CVector::Up();
		const Eigen::Translation3f globalPosition = Eigen::Translation3f(m_position + (upVec * offset));

		const Eigen::UniformScaling<float> globalScale = Eigen::Scaling(scale);

		m_spBallRenderable->m_spInstanceInfo->m_Transform = globalPosition * m_rotation * globalScale;
		// Local rotation
		m_spAnimationPose->m_pMatrixListA[1] = CQuaternion::FromAngleAxis(rotationDeg, CVector::Left());
	}

	virtual void AddCallback(const hh::base::THolder<Sonic::CWorld>& worldHolder,
		Sonic::CGameDocument* pGameDocument, const boost::shared_ptr<hh::db::CDatabase>& spDatabase) override
	{
		Sonic::CApplicationDocument::GetInstance()->AddMessageActor("GameObject", this);
		pGameDocument->AddUpdateUnit("a", this);

		hh::mr::CMirageDatabaseWrapper wrapper(spDatabase.get());
		//boost::shared_ptr<hh::mr::CModelData> spModelData;

		//wrapper.GetModelData(spModelData, "chr_classic_sonic_spin", 0);
		const char* assetName = "chr_classic_sonic_spin";
		boost::shared_ptr<hh::mr::CModelData> spModelData = wrapper.GetModelData(assetName, 0);

		// Debug models
		// BasicCylinder
		// BasicSphere
		// Camera
		// DebugArrow
		// DebugSphere
		// environmental_sound
		// EventSetter
		// Grouper
		// hint
		// SwitchManager
		// target
		
		//wrapper.GetModelData(spModelData, "target", 0);

		m_spBallRenderable = boost::make_shared<hh::mr::CSingleElement>(spModelData);
		if (!spModelData)
			return;
		AddRenderable("Object", m_spBallRenderable, true);

		m_spAnimationPose = boost::make_shared<hh::anim::CAnimationPose>(spDatabase, assetName);
		m_spBallRenderable->BindPose(m_spAnimationPose);
	}
};

float GetDeltaTime(int This)
{
	return *(float*)(*((uint32_t*)This + 3) + 0x24);
}

float GetDeltaTime(void* This)
{
	return GetDeltaTime((int)This);
}

boost::detail::sp_if_not_array<SpinBall>::type spinballSharedPtr;
bool isVisible = true;
bool typeFlicker = false;
float flickerTimer = 0.0f;
const float flickerTimerMax = (1.0f / 60.0f) * 16.0f;

bool PrepareRemoveSpinball = false;
bool spinballExists = false;

Sonic::Player::CPlayerSpeedContext* GetCSonicContext(int This)
{
	return *(Sonic::Player::CPlayerSpeedContext**)(This + 8);
}

Sonic::Player::CPlayerSpeedContext* GetCSonicContext(void* This)
{
	return GetCSonicContext((int)This);
}

void BallFlicker(Sonic::Player::CPlayerSpeedContext* context, float deltaTime)
{
	// Patch for when we start the jump ball animation.
	// We can't rely on the states right now due to state-change nonsense.
	// In the future this should be removed in favor of a detect-animation approach anyway.
	if (spinballSharedPtr->visible && isVisible && !typeFlicker)
	{
		SetSphereMdl(context, 2);
		isVisible = false;
	}

	flickerTimer += deltaTime;
	if (flickerTimer >= flickerTimerMax)
	{
		flickerTimer = 0.0f;
		typeFlicker = !typeFlicker;
	}

	// DEBUGGING:
	//typeFlicker = false;
	//isVisible = true;

	if (typeFlicker)
	{
		SetSphereMdl(context, isVisible ? 0 : 2);
		isVisible = !isVisible;
		spinballSharedPtr->visible = isVisible;
	}
	else if (!spinballSharedPtr->visible)
	{
		SetSphereMdl(context, 2);
		isVisible = false;
		spinballSharedPtr->visible = true;
	}
}

void ResetBallFlags()
{
	isVisible = true;
	typeFlicker = false;
	flickerTimer = 0.0f;
	PrepareRemoveSpinball = false;

	Sonic::Player::CPlayerSpeedContext** contextPtr = (Sonic::Player::CPlayerSpeedContext**)0x1E5E2F0;
	if (!contextPtr) return;
	if (!*contextPtr) return;
	SetSphereMdl(*contextPtr, 0);
}

void SetupBall(void* This)
{
	//auto* const context = GetCSonicContext(This);
	//SetSphereMdl(context, 2);
	PrepareRemoveSpinball = false;

	spinballSharedPtr = boost::make_shared<SpinBall>();
	Sonic::CGameDocument::GetInstance()->AddGameObject(spinballSharedPtr);
}
void DestroyBall()
{
	spinballSharedPtr->SendMessageSelfImm<MsgKill>();
	spinballSharedPtr = nullptr;
	PrepareRemoveSpinball = true;

	//CSonicContext** contextPtr = (CSonicContext**)0x1E5E2F0;
	//SetSphereMdl(*contextPtr, 0);
	//isVisible = true;
}

HOOK(char, __fastcall, ClassicJumpBallState_Begin,     0x01114F30, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	SetupBall(This);

	auto result = originalClassicJumpBallState_Begin(This);
	return result;
}
HOOK(void, __fastcall, ClassicJumpBallState_Calculate, 0x01114FD0, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	auto* const context = GetCSonicContext(This);
	const auto deltaTime = GetDeltaTime(This);

	BallFlicker(context, deltaTime);

	originalClassicJumpBallState_Calculate(This);
}
HOOK(int,  __fastcall, ClassicJumpBallState_End,       0x01114F00, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	if (spinballSharedPtr)
	{
		DestroyBall();
		PrepareRemoveSpinball = true;
	}

	auto result = originalClassicJumpBallState_End(This);
	return result;
}

HOOK(void*, __fastcall, StateFallStart, 0x01115A60, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	auto result = originalStateFallStart(This);

	bool* isBall = (bool*)((int)This + 0x6D);

	if (*isBall)
	{
		SetupBall(This);
	}

	return result;
}
HOOK(char,  __fastcall, StateFallCalc,  0x01115790, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	auto* const context = GetCSonicContext(This);
	const auto deltaTime = GetDeltaTime(This);

	bool* isBall = (bool*)((int)This + 0x6D);

	if (*isBall && spinballSharedPtr)
	{
		BallFlicker(context, deltaTime);
	}
	// Safety check
	else if (!*isBall && spinballSharedPtr)
	{
		DestroyBall();
	}

	auto result = originalStateFallCalc(This);
	return result;
}
HOOK(void,  __fastcall, StateFallEnd,   0x011159B0, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	if (spinballSharedPtr)
	{
		DestroyBall();
	}
	originalStateFallEnd(This);
}

HOOK(void, __fastcall, StateSpinStart, 0x011BB0E0, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	if (true)
	{
		SetupBall(This);
	}
	else
	{
		SetSphereMdl(This->GetContext(), 1);
		SetBallAnimation(This->GetContext(), "Sphere");
	}
	originalStateSpinStart(This);
}
HOOK(void, __fastcall, StateSpinEnd, 0x011BAFD0, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	if (true)
	{
		DestroyBall();
		ResetBallFlags();
	}
	else
	{
		SetSphereMdl(This->GetContext(), 0);
	}

	originalStateSpinEnd(This);
}
HOOK(void, __fastcall, StateSpinCalc, 0x011BA930, Sonic::Player::CPlayerSpeedContext::CStateSpeedBase* This)
{
	auto* const context = GetCSonicContext(This);
	const auto deltaTime = GetDeltaTime(This);

	BallFlicker(context, deltaTime);

	originalStateSpinCalc(This);
}


void JumpBall::Init()
{
	INSTALL_HOOK(ClassicJumpBallState_Begin)
	INSTALL_HOOK(ClassicJumpBallState_Calculate)
	INSTALL_HOOK(ClassicJumpBallState_End)

	INSTALL_HOOK(StateFallStart)
	INSTALL_HOOK(StateFallCalc)
	INSTALL_HOOK(StateFallEnd)

	INSTALL_HOOK(StateSpinStart)
	INSTALL_HOOK(StateSpinEnd)
	INSTALL_HOOK(StateSpinCalc)

	//INSTALL_HOOK(sub_ScaleChanger)
	//INSTALL_HOOK(sub_MsgChangeScale)
}

void JumpBall::OnFrame()
{
	if (PrepareRemoveSpinball)
	{
		ResetBallFlags();
	}
}

namespace UNUSED
{
	bool sentFromChangeScale = false;
	HOOK(int, __fastcall, sub_MsgChangeScale, 0xE6A590, void* This, void* ebx, int a2)
	{
		//DebugDrawText::log(format("%X", *This), 10.0f);
		//sentFromChangeScale = true;
		//DebugDrawText::log(format("%X"), (int)This->IAnimationContext_vftable);
		return originalsub_MsgChangeScale(This, ebx, a2);
	}

	HOOK(void, __fastcall, sub_ScaleChanger, 0x006C79A0, float* This, void* ebx, float a2)
	{
		//if (sentFromChangeScale)
		//{
		//	DebugDrawText::log(format("%X", *(int*)This), 10.0f);
		//	sentFromChangeScale = false;
		//}
		This[31] = a2;
	}
}


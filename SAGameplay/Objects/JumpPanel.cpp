#include "JumpPanel.h"
#include "../SetObjectsCustom.h"
#include "../Extra/BlueBlurExtensions.h"

namespace SetObjectsCustom
{
	void CObjNumberedJumpPanel::StateUpdate_Idle()
	{
		if (!m_pPlayerContext)
			return;

		if (!m_IsOverlap)
			return;

		// debug
		//if (m_pPlayerContext->m_spInputPad.get()->CheckInputTapped(Sonic::Player::CInputPad::eInputID_ButtonB))


		if (!m_pPlayerContext->m_spInputPad.get()->CheckInputTapped(Sonic::Player::CInputPad::eInputID_ButtonA))
			return;


		const auto* pSonicTransform = &m_pPlayerContext->m_spMatrixNode->m_Transform;
		auto* pControlTransform = &m_spSonicControlNode->m_Transform;

		const float height = 0.29f;
		const int i = m_CurrentPanel - 1;
		m_TargetPosition = m_Positions.at(i) + m_Rotations.at(i) * CVector(0, height, 0);
		m_Direction = (m_TargetPosition - pSonicTransform->m_Position).normalized();

		pControlTransform->SetRotationAndPosition(
		                                          CQuaternion::LookRotation(m_Direction),
		                                          pSonicTransform->m_Position
		                                         );
		m_spSonicControlNode->NotifyChanged();

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgStartExternalControl(m_spSonicControlNode));
		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgChangeMotionInExternalControl("SpinAttack"));
		m_pPlayerContext->PlaySound(2002027, true); // Jump SFX

		m_WaitTime = MaxWaitTime;
		m_State = SMovePanel;
	}

	void CObjNumberedJumpPanel::StateUpdate_Move(float deltaTime)
	{
		// Debug stuff
		//DebugDrawText::log("Moving...", 0, -1, { 1,1,0.2,1 });
		//DebugCancelExternalControl();

		const bool isLastPanel = m_CurrentPanel >= Count - 1;

		const float FrameSpeed = m_MoveSpeed * deltaTime;
		const float SnapDistance = FrameSpeed + 0.01f; // Prevents overshooting
		Hedgehog::Mirage::CTransform* transform = &m_spSonicControlNode->m_Transform;

		if (m_PrepareFinish)
		{
			SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl(Sonic::Message::MsgFinishExternalControl::EChangeState::FINISH));
			m_pPlayerContext->SetHorizontalVelocity(CVector::Zero());
			m_CurrentPanel = 1;
			m_State = SIdle;
			m_PrepareFinish = false;
			return;
		}

		// Check if we're about to go past our target position.
		if (CVector::LengthSqr(transform->m_Position - m_TargetPosition) >= SnapDistance * SnapDistance)
		{
			// If not, do basic movement & advance.
			// This is why we check if we're on the last panel too, because we go forward regardless.
			transform->m_Position += m_Direction * FrameSpeed;
			transform->UpdateMatrix();
			m_spSonicControlNode->NotifyChanged();
			return;
		}

		// Bail immediately if this is our last panel.
		//if (m_CurrentPanel >= Count - 1) // Our current panel will be 1 less than the target panel, meaning we want to check if we're aiming for the finish.
		//{
		//	SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl());
		//	m_pPlayerContext->SetVelocity(CVector::Zero());
		//	m_CurrentPanel = 1;
		//	m_State = SIdle;
		//	return;
		//}

		const CQuaternion rotationRaw = m_Rotations.at(m_CurrentPanel - 1);
		const CQuaternion rotation = CQuaternion::LookRotation(rotationRaw * CVector(1, 0, 0), rotationRaw.Up());
		transform->SetRotationAndPosition(rotation, m_TargetPosition);
		m_spSonicControlNode->NotifyChanged();

		if (isLastPanel)
		{
			m_PrepareFinish = true;
			return;
		}

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgChangeMotionInExternalControl("Squat", true));

		m_spFXNode->m_Transform.SetRotationAndPosition(rotation, m_TargetPosition);
		m_spFXNode->NotifyChanged();
		m_pGlitterPlayer->PlayOneshot(m_spFXNode, "ef_ob_com_yh1_spring_wave", 1.0f, 9);

		m_CurrentPanel++;
		m_State = SWait;
	}

	void CObjNumberedJumpPanel::StateUpdate_Wait(float deltaTime)
	{
		// Debug stuff
		//DebugDrawText::log("Waiting...", 0, -1, { 0.5,1,0.2,1 });
		//DebugCancelExternalControl();

		if (DoDetatch(deltaTime))
			return;

		if (!m_pPlayerContext->m_spInputPad.get()->CheckInputTapped(Sonic::Player::CInputPad::eInputID_ButtonA))
			return;

		// FIXME: TRY TO NOT COPY/PASTE CODE!
		//const auto* pParentTransform  = &m_spMatrixNodeTransform->m_Transform;
		auto* pControlTransform = &m_spSonicControlNode->m_Transform;

		const int i = m_CurrentPanel - 1;
		// FIXME: Too lazy to figure out what this is right now. Get an actual number for this! 
		const float height = 0.29f;

		m_TargetPosition = m_Positions.at(i) + m_Rotations.at(i) * CVector(0, height, 0);
		m_Direction = (m_TargetPosition - pControlTransform->m_Position).normalized();

		pControlTransform->SetRotation(CQuaternion::LookRotation(m_Direction));
		m_spSonicControlNode->NotifyChanged();

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgStartExternalControl(m_spSonicControlNode));
		m_pGlitterPlayer->PlayOneshot(m_spMatrixNodeEvent, "ef_ob_com_yh1_spring_wave", 1.0f, 9);

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgChangeMotionInExternalControl("SpinAttack"));
		m_pPlayerContext->PlaySound(2002027, true); // Jump SFX
		m_WaitTime = MaxWaitTime;
		m_State = SMovePanel;
	}

	bool CObjNumberedJumpPanel::DoDetatch(float deltaTime)
	{
		if (m_WaitTime > 0.0f)
		{
			m_WaitTime -= deltaTime;
			return false;
		}

		const CVector upDir = m_pPlayerContext->GetUpDirection();
		const float detatchImpulse = (1.0f - fabs(CVector::Dot(upDir, CVector::Up()))) * 5.0f;

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl());
		m_pPlayerContext->SetVelocity(upDir * detatchImpulse);

		m_pPlayerContext = nullptr;
		m_CurrentPanel = 1;
		m_PrepareFinish = false;
		m_State = SIdle;
		return true;
	}

	void CObjNumberedJumpPanel::ProcMsgHitEventCollision(Sonic::Message::MsgHitEventCollision& msg)
	{
		// Bail if the contacting object is not sonic.
		if (!SendMessageImm(msg.m_SenderActorID, Sonic::Message::MsgGetPlayerType()))
			return;

		m_IsOverlap = true;
		//DebugDrawText::log("OVERLAPPED!", 10.0f, -1, { 0,1,0.2,1 });

		if (m_pPlayerContext)
			return;

		m_pPlayerContext = static_cast<Sonic::Player::CPlayerSpeed*>(m_pMessageManager->GetMessageActor(msg.m_SenderActorID))->GetContext();
		m_CurrentPanel = 1;

		m_HasBegunLinkJump = false;
	}

	void CObjNumberedJumpPanel::ProcMsgLeaveEventCollision(Sonic::Message::MsgLeaveEventCollision& msg)
	{
		// Bail if the contacting object is not sonic.
		if (!SendMessageImm(msg.m_SenderActorID, Sonic::Message::MsgGetPlayerType()))
			return;

		// No matter what, we want to log we're not overlapping.
		m_IsOverlap = false;
		//DebugDrawText::log("STEPPED OFF!", 10.0f, -1, { 1,0.2,0.0,1 });

		//if (!m_pPlayerContext)
		//	return;

		// Only want to process jump action if we didn't do this already.
		//if (!m_HasBegunLinkJump)
		//	StartLinkJumpAction();
	}

	//
	// Overrides
	// ----------------

	void CObjNumberedJumpPanel::SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo)
	{
		// Update states
		switch (m_State)
		{
		default:
		case SIdle:
		{
			StateUpdate_Idle();
			break;
		}
		case SMovePanel:
		{
			StateUpdate_Move(updateInfo.DeltaTime);
			break;
		}
		case SWait:
		{
			StateUpdate_Wait(updateInfo.DeltaTime);
			break;
		}
		}

		//UpdateOld(updateInfo);
	}

	bool CObjNumberedJumpPanel::SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		Hedgehog::Mirage::CMatrixNode* rootNode = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spMatrixNodeRoot.get();
		m_spSonicControlNode = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spSonicControlNode->SetParent(rootNode);
		m_spFXNode = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spFXNode->SetParent(rootNode);

		m_pGlitterPlayer = Sonic::CGlitterPlayer::Make(in_pGameDocument);

		hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());

		char assetName[std::char_traits<char>::length(ms_ModelName) + 4];
		sprintf_s(assetName, "%s_0%d", ms_ModelName, 1);
		SElementInfo info(m_spRenderable);
		AddRenderable(assetName, info, m_spMatrixNodeTransform, in_spDatabase);

		Hedgehog::Mirage::CMatrixNode* parent = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spMatrixNodeRoot.get();

		m_Panels.reserve(Count - 2);

		for (int i = 0; i < Count - 2; ++i)
		{
			// TODO: Update assetName to use panel numbers 2+
			sprintf_s(assetName, "%s_0%d", ms_ModelName, i + 2);

			auto panel_visual = boost::make_shared<CObjPanelVisual>(assetName, m_Positions.at(i), m_Rotations.at(i));
			m_Panels.push_back(panel_visual);
			in_pGameDocument->AddGameObject(panel_visual);
		}

		return true;
	}

	bool CObjNumberedJumpPanel::SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		const char* containerName = "jumppannel_col";
		const char* eventColName = "jumppannel_event";

		m_spMatrixNodeCollision = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spMatrixNodeCollision->m_Transform.SetPosition(CVector(0, 0.125f * 0.25f, 0));
		m_spMatrixNodeCollision->NotifyChanged();
		m_spMatrixNodeCollision->SetParent(m_spMatrixNodeTransform.get());

		m_spMatrixNodeEvent = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spMatrixNodeEvent->m_Transform.SetPosition(CVector(0, 0.125f, 0));
		m_spMatrixNodeEvent->NotifyChanged();
		m_spMatrixNodeEvent->SetParent(m_spMatrixNodeTransform.get());

		AddRigidBody(m_spRigidBody, containerName, containerName, *(int*)0x01E0AF30, m_spMatrixNodeCollision, in_spDatabase);


		// Hacky event collection stuff
		hk2010_2_0::hkpShape* triggerShape =
			Sonic::CHavokDatabaseWrapper(in_spDatabase.get()).GetRigidBodyContainer(containerName)->GetRigidBody(eventColName, nullptr)->GetShape();

		AddEventCollision("Stomping", triggerShape, *(int*)0x01E0AFD8, true, m_spMatrixNodeEvent);
		triggerShape->removeReference();

		return true;
	}

	void CObjNumberedJumpPanel::KillCallback()
	{
		for (int i = 0; i < m_Panels.size(); ++i)
		{
			SendMessageImm(m_Panels.at(i).get(), Sonic::Message::MsgKill());
		}
	}

	void CObjNumberedJumpPanel::InitializeEditParam(Sonic::CEditParam& in_rEditParam)
	{
		//in_rEditParam.CreateParamInt(&NextPanelID, "NextPanelID");
		in_rEditParam.CreateParamInt(&Count, "Count");

		char namePosition[10];
		char nameRotation[10];

		m_Positions.reserve(8);
		m_Rotations.reserve(8);

		for (int i = 0; i < 8; ++i)
		{
			const int j = i + 2;
			sprintf_s(namePosition, "Position%d", j);
			sprintf_s(nameRotation, "Rotation%d", j);

			m_Positions.push_back(CVector::Zero());
			m_Rotations.push_back(CQuaternion::Identity());

			in_rEditParam.CreateParamBase(Sonic::CParamPosition::Create(&m_Positions.at(i)), namePosition);
			in_rEditParam.CreateParamBase(Sonic::CParamRotation::Create(&m_Rotations.at(i)), nameRotation);
		}
	}

	bool CObjNumberedJumpPanel::ProcessMessage(Hedgehog::Universe::Message& message, bool flag)
	{
		using namespace Sonic::Message;
		if (!flag)
			return CObjectBase::ProcessMessage(message, flag);

		if (message.Is<MsgHitEventCollision>())
		{
			ProcMsgHitEventCollision(static_cast<MsgHitEventCollision&>(message));
			return true;
		}
		if (message.Is<MsgLeaveEventCollision>())
		{
			ProcMsgLeaveEventCollision(static_cast<MsgLeaveEventCollision&>(message));
			return true;
		}

		return CObjectBase::ProcessMessage(message, flag);
	}

	//
	// OUTDATED
	// -------------------

	void CObjNumberedJumpPanel::UpdateOld(const Hedgehog::Universe::SUpdateInfo& updateInfo)
	{
		if (!m_HasBegunLinkJump)
			StartLinkJumpAction();

		if (!m_pPlayerContext)
			return;

		// If we exit without being in external control, remove our player reference.
		if (!m_IsOverlap)
		{
			Sonic::Message::MsgIsExternalControl isExternalControl;
			SendMessageImm(m_pPlayerContext->m_pPlayer, isExternalControl);
			if (!isExternalControl.IsTrue)
			{
				m_pPlayerContext = nullptr;
				DebugDrawText::log("REMOVED PLAYER!", 10.0f, -1, { 0.8,0.8,0.0,1 });
				return;
			}
		}

		m_CurrentTime += updateInfo.DeltaTime;

		const float nodeHeightMax = dbg_NodeHeightMax - dbg_NodeHeightMin;

		if (DebugCancelExternalControl())
			return;

		// Test: get sonic attached to an oscillating node first.
		m_spSonicControlNode->m_Transform.SetPosition(CVector(0,
		                                                      sinf(m_CurrentTime * 3.5f) * nodeHeightMax + dbg_NodeHeightMin,
		                                                      0));
		//m_spSonicControlNode->m_Transform.SetRotation(CQuaternion::FromEuler(45.0f * DEG2RAD, 90.0f * DEG2RAD, 0));
		m_spSonicControlNode->NotifyChanged();
	}

	void CObjNumberedJumpPanel::StartLinkJumpAction()
	{
		if (!m_pPlayerContext)
			return;

		if (!m_pPlayerContext->m_spInputPad.get()->CheckInputTapped(Sonic::Player::CInputPad::eInputID_ButtonA))
			return;

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgStartExternalControl(m_spSonicControlNode));
		m_pGlitterPlayer->PlayOneshot(m_spMatrixNodeEvent, "ef_ob_com_yh1_spring_wave", 1.0f, 9);

		m_HasBegunLinkJump = true;
	}

	bool CObjNumberedJumpPanel::DebugCancelExternalControl()
	{
		if (!Sonic::CInputState::GetInstance()->GetPadState().IsTapped(Sonic::eKeyState_B))
			return false;

		SendMessageImm(m_pPlayerContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl());
		m_CurrentPanel = 1;
		m_PrepareFinish = false;
		m_State = SIdle;

		// If we're no longer on the platform, we should remove our player reference.
		//if (!m_IsOverlap)
		//{
		//	DebugDrawText::log("FALLING!", 10.0f, -1, { 0.6,0.0,0.0,1 });
		//	m_pPlayerContext = nullptr;
		//}
		return true;
	}
}

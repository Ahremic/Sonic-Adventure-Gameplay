#include "RubberBandRail.h"
#include "..\SetObjectsCustom.h"

namespace SetObjectsCustom
{
	bool CObjECRubberBandRail::SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());

		const char* assetName = "seaobj_gomubane";

		m_spRenderable = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData(assetName));
		m_spRenderable->BindMatrixNode(m_spMatrixNodeTransform);
		CGameObject::AddRenderable("Object", m_spRenderable, true);

		//AddRenderable(assetName, m_spRenderable, m_spMatrixNodeTransform, in_spDatabase);

		m_spAnimationPose = boost::make_shared<hh::anim::CAnimationPose>(in_spDatabase, assetName);
		m_spRenderable->BindPose(m_spAnimationPose);

		//Hedgehog::Mirage::CMatrixNode* rootNode = Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spMatrixNodeRoot.get();
		m_spSonicControlNode = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spSonicControlNode->SetParent(Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spMatrixNodeRoot.get());

		return true;
	}

	void CObjECRubberBandRail::ChangeState(EState inState)
	{
		// on End
		/*
			switch (m_State)
			{
				default:
					break;
			}
			*/

		// on Start
		switch (inState)
		{
			default:
				break;
			case SPrepareRebound:
				StateStart_PREPARE_REBOUND();
				break;
			case SRebound:
				StateStart_REBOUND();
				break;
		}

		m_State = inState;
	}

	void CObjECRubberBandRail::EaseRopeToPosition()
	{
		if (std::abs(m_OffsetSide) < FLT_EPSILON && std::abs(m_OffsetStretch) < FLT_EPSILON)
			return;

		// TODO: Make parameter?
		const float lerpRate = 5.0f;
		const float t = lerpRate * m_DeltaTime;
		m_OffsetSide = std::lerp(m_OffsetSide, 0, t);
		m_OffsetStretch = std::lerp(m_OffsetStretch, 0, t);

		Sonic::CMatrixNodeTransform* node = m_spMatrixNodeTransform.get();
		CQuaternion& rotation = node->m_Transform.m_Rotation;

		const CVector forward = rotation * AXIS_FWD;
		const CVector right = rotation * AXIS_RGHT;

		m_spAnimationPose->m_pMatrixListA[2] = Eigen::Translation3f(rotation.inverse() * (forward * m_OffsetStretch + right * m_OffsetSide));
	}

	void CObjECRubberBandRail::ReactToPlayer()
	{
		Sonic::CMatrixNodeTransform* node = m_spMatrixNodeTransform.get();
		CQuaternion& rotation = node->m_Transform.m_Rotation;

		const CVector forward = rotation * AXIS_FWD;
		const CVector right = rotation * AXIS_RGHT;
		const CVector up = rotation * AXIS_UP;

		const CVector horizontalVelocity = m_SonicContext->GetHorizontalVelocity();

		const CVector sonicPosition = m_SonicContext->m_spMatrixNode->m_Transform.m_Position + horizontalVelocity * m_DeltaTime;
		const CVector ropePosition = node->m_Transform.m_Position + forward * (ropeOffset + m_OffsetStretch);
		const CVector relativePosition = sonicPosition - ropePosition;

		const float dotFwd = CVector::Dot(relativePosition, forward);

		m_IsSonicFromBehind = dotFwd < 0.0f;
		const float sign = m_IsSonicFromBehind ? -1.0f : 1.0f;

		//DebugDrawText::log(format("DOT: %f", dotFwd), 0, 0, { 1,0.8, 0.2, 1 });
		DebugDrawText::log(format("SIGN: %f", sign), 0, 0, { 1,0.8, 0.2, 1 });

		if (dotFwd * sign > activationThickness)
			return;

		const float dotRight = std::abs(CVector::Dot(relativePosition, right));
		if (dotRight > halfSizeWidth)
			return;

		const float dotUp = std::abs(CVector::Dot(relativePosition, up));
		if (dotUp > halfSizeHeight)
			return;

		constexpr float maxTensionVelocity = 12.5f;
		if (std::abs(CVector::Dot(forward, horizontalVelocity)) > maxTensionVelocity)
		{
			// Have to do some nonsense to force sonic to be *on* the rubberband.
			// FIXME: This is bugprone.
			const float rightDot = CVector::Dot(relativePosition, right);
			CVector newPosition = ropePosition + right * rightDot;
			newPosition.y() = sonicPosition.y();

			m_spSonicControlNode->m_Transform.SetRotationAndPosition(m_SonicContext->m_spMatrixNode->m_Transform.m_Rotation, newPosition);
			m_spSonicControlNode->NotifyChanged();

			ChangeState(SPrepareRebound);

			m_VelocityExternalControl = m_OriginalHorizontalVelocity;
			m_IsExternalControl = true;

			SendMessageImm(m_SonicContext->m_pPlayer, Sonic::Message::MsgStartExternalControl(m_spSonicControlNode, true, true));
		}
		else
		{
			ChangeState(SStretch);
		}
	}

	void CObjECRubberBandRail::StateStart_PREPARE_REBOUND()
	{
		m_OriginalHorizontalVelocity = m_SonicContext->GetHorizontalVelocity();
		m_CurrentTime = 0;

		FUNCTION_PTR(bool, __stdcall, SonicSetOutOfControlTime, 0x00E5AC00, Sonic::Player::CPlayerSpeedContext * context, float outOfControlTime);
		SonicSetOutOfControlTime(m_SonicContext, 0.3f);
	}

	void CObjECRubberBandRail::StateUpdate_PREPARE_REBOUND()
	{
		//if (m_IsExternalControl)
		//{
		//	m_SonicContext->SetHorizontalVelocity(m_OriginalHorizontalVelocity);
		//	SendMessageImm(m_SonicContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl(Sonic::Message::MsgFinishExternalControl::EChangeState::FINISH));
		//}

		// TODO: I see something happening here that feels weird, we'll want to fix this...
		//if (ropeTension < 2.0f)
		//	return;
		Sonic::CMatrixNodeTransform* node = m_spMatrixNodeTransform.get();
		CQuaternion& rotation = node->m_Transform.m_Rotation;

		const CVector forward = rotation * AXIS_FWD;
		const CVector right = rotation * AXIS_RGHT;
		const CVector up = rotation * AXIS_UP;

		//const CVector horizontalVelocity = m_SonicContext->GetHorizontalVelocity();
		const CVector horizontalVelocity = m_VelocityExternalControl;

		const float sign = m_IsSonicFromBehind ? 1.0f : -1.0f;
		const float offsetPosition = activationThickness * sign * 1.2f;

		const CVector sonicPosition = m_SonicContext->m_spMatrixNode->m_Transform.m_Position;
		const CVector ropePosition = m_spMatrixNodeTransform->m_Transform.m_Position + forward * ropeOffset;
		const CVector relativePosition = sonicPosition - ropePosition;


		CVector ropeOffsetPosition = relativePosition + forward * offsetPosition;
		ropeOffsetPosition.y() = 0;

		m_spAnimationPose->m_pMatrixListA[2] = Eigen::Translation3f(rotation.inverse() * ropeOffsetPosition);
		m_OffsetStretch = CVector::Dot(forward, ropeOffsetPosition);
		m_OffsetSide = CVector::Dot(right, ropeOffsetPosition);

		if (m_CurrentTime > 0.125f)
		{
			m_CurrentTime = 0.0f;

			// boing time
			const CVector reflectVector = CVector::Reflect(m_OriginalHorizontalVelocity * 0.8f, forward * sign);
			const CQuaternion yawRot = CQuaternion::LookRotation(reflectVector.ProjectOnPlane(CVector::Up()).normalizedSafe());
			m_spSonicControlNode->m_Transform.SetRotation(yawRot);
			m_spSonicControlNode->NotifyChanged();

			SendMessageImm(m_SonicContext->m_pPlayer, Sonic::Message::MsgFinishExternalControl(Sonic::Message::MsgFinishExternalControl::EChangeState::FINISH));
			m_SonicContext->SetHorizontalVelocity(reflectVector);
			m_SonicContext->SetYawRotation(yawRot);

			m_Sign = sign;
			m_OffsetSideInitial = m_OffsetSide;
			m_SonicContext->PlaySound(4002017, false);
			ChangeState(SRebound);
			return;
		}
		else
		{
			//constexpr float decelRate = 10.0f;
			constexpr float decelRate = 5.0f;
			const float speed = horizontalVelocity.norm();
			const float speedDecelRate = decelRate * (speed * 1 / 10.0f);
			const CVector newVelocity = horizontalVelocity.normalizedSafe() * std::lerp(speed, 0, (decelRate + speedDecelRate) * m_DeltaTime);

			//m_SonicContext->SetHorizontalVelocity_Dirty(newVelocity);
			m_VelocityExternalControl = newVelocity;
			m_spSonicControlNode->m_Transform.SetPosition(m_spSonicControlNode->m_Transform.m_Position + (m_VelocityExternalControl * m_DeltaTime));
			m_spSonicControlNode->NotifyChanged();
		}
	}

	void CObjECRubberBandRail::StateUpdate_WAIT()
	{
		EaseRopeToPosition();

		// HACK: Bandaid fix to prevent some unintentional, ironic, rubberbanding.
		if (m_CurrentTime < 0.10f)
			return;

		ReactToPlayer();
	}

	void CObjECRubberBandRail::StateUpdate_STRETCH()
	{
		//const CQuaternion realRotation = m_spMatrixNodeTransform->m_Transform.m_Rotation;
		//const CQuaternion rotation(realRotation.w(), realRotation.z(), realRotation.y(), realRotation.x());
		const CQuaternion rotation = m_spMatrixNodeTransform->m_Transform.m_Rotation;
		const CVector forward = rotation * AXIS_FWD;
		const CVector right = rotation * AXIS_RGHT;
		const CVector up = rotation * AXIS_UP;

		auto* pose = m_spAnimationPose.get();

		const float sign = m_IsSonicFromBehind ? 1.0f : -1.0f;
		const float offsetPosition = activationThickness * sign * 1.2f;

		const CVector sonicPosition = m_SonicContext->m_spMatrixNode->m_Transform.m_Position;
		const CVector ropePosition = m_spMatrixNodeTransform->m_Transform.m_Position + forward * ropeOffset;
		const CVector relativePosition = sonicPosition - ropePosition;

		const float dot = CVector::Dot(relativePosition, forward) + (activationThickness + 0.05) * sign;

		if (dot * sign < 0)
		{
			// Wait state for now.
			ChangeState(SWait);
			return;
		}

		const CVector previousHorizontalVelocity = m_SonicContext->GetHorizontalVelocity();
		const float ropeTension = std::abs(dot);

		constexpr float baseVelocityAddRate = 30.0f;
		const float velocityAddRate = baseVelocityAddRate + (ropeTension * baseVelocityAddRate);// +(initialSpeed * baseVelocityAddRate * 0.5f);

		const CVector addVelocity = forward * (velocityAddRate * -sign * m_DeltaTime);
		const CVector horizontalVelocity = previousHorizontalVelocity + addVelocity;

		m_SonicContext->SetHorizontalVelocity(horizontalVelocity);

		DebugDrawText::log(format("DOT: %f", CVector::Dot(forward * offsetPosition, relativePosition)), 0, 0, { 1,0.8, 0.2, 1 });

		CVector ropeOffsetPosition = relativePosition + forward * offsetPosition;
		ropeOffsetPosition.y() = 0;

		pose->m_pMatrixListA[2] = Eigen::Translation3f(rotation.inverse() * ropeOffsetPosition);
		m_OffsetStretch = CVector::Dot(forward, ropeOffsetPosition);
		m_OffsetSide = CVector::Dot(right, ropeOffsetPosition);

		// Jump handling
		const float dotUp = std::abs(CVector::Dot(relativePosition, up));
		if (dotUp > halfSizeHeight)
		{
			ChangeState(SWait);
			return;
		}

		constexpr float maxTensionVelocity = 10.0f;
		if (CVector::Dot(forward * sign, horizontalVelocity) > maxTensionVelocity)
			ChangeState(SPrepareRebound);
	}

	void CObjECRubberBandRail::StateUpdate_REBOUND()
	{
		// TODO: Make parameter?
		const float lerpRate = 5.0f;
		const float wiggleFrequency = 25.0f;
		const float wiggleAmpMax = 1.25f;
		constexpr float wiggleTime = 1.0f;

		constexpr float div = 1.0f / wiggleTime;
		const float t = (1.0f - (m_CurrentTime * div));

		m_OffsetSide = std::lerp(0, m_OffsetSideInitial, t);
		m_OffsetStretch = cosf(m_CurrentTime * wiggleFrequency) * (wiggleAmpMax * t) * m_Sign;
		// Cos starts from +1, need it to start from -1, so we invert the sign.

		Sonic::CMatrixNodeTransform* node = m_spMatrixNodeTransform.get();
		CQuaternion& rotation = node->m_Transform.m_Rotation;

		const CVector forward = rotation * AXIS_FWD;
		const CVector right = rotation * AXIS_RGHT;

		m_spAnimationPose->m_pMatrixListA[2] = Eigen::Translation3f(rotation.inverse() * (forward * m_OffsetStretch + right * m_OffsetSide));

		if (m_CurrentTime > wiggleTime)
			ChangeState(SWait);

		// HACK: Bandaid fix to prevent some unintentional, ironic, rubberbanding.
		if (m_CurrentTime < 0.20f)
			return;

		if (m_SonicContext)
			ReactToPlayer();
	}

	void CObjECRubberBandRail::GenericStateUpdate()
	{
		if (!m_SonicContext)
		{
			if (m_State == SRebound)
			{
				StateUpdate_REBOUND();
				return;
			}

			if (m_State != SWait)
				ChangeState(SWait);
			EaseRopeToPosition();
			return;
		}

		switch (m_State)
		{
			default:
			case SWait:
				StateUpdate_WAIT();
				break;
			case SStretch:
				StateUpdate_STRETCH();
				break;
			case SPrepareRebound:
				StateUpdate_PREPARE_REBOUND();
				break;
			case SRebound:
				StateUpdate_REBOUND();
				break;
		}
	}

	Sonic::Player::CPlayerSpeedContext* CObjECRubberBandRail::GetPlayer() const
	{
		// TODO: REPLACE WITH EVENTCOLLISION.
		auto* context = Sonic::Player::CPlayerSpeedContext::GetInstance();
		if (!context)
			return nullptr;

		auto* const node = m_spMatrixNodeTransform.get();

		const float maxDistanceDebug = 5.0f;
		if (CVector::LengthSqr(context->m_spMatrixNode->m_Transform.m_Position - node->m_Transform.m_Position)
		    > maxDistanceDebug * maxDistanceDebug)
			return nullptr;

		return context;
	}

	void CObjECRubberBandRail::SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo)
	{
		auto* const pose = m_spAnimationPose.get();
		int numBones = pose->m_numBones;

		m_DeltaTime = updateInfo.DeltaTime;
		m_CurrentTime += m_DeltaTime;

		// TODO: Probably fix the blender-ism of having the armature root bone because that honestly sucks lol
		if (numBones < 3)
			return;

		m_SonicContext = GetPlayer();

		GenericStateUpdate();
	}

	void CObjECRubberBandRail::SetAddUpdateUnit(Sonic::CGameDocument* in_pGameDocument)
	{
		in_pGameDocument->AddUpdateUnit("a", this);
	}
}

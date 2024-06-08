#include "FloatingBridge.h"
#include "..\SetObjectsCustom.h"

#include <chrono>

namespace SetObjectsCustom
{
	///////////////////////////
	//
	//  Float bridge - Manager
	//------------------------------------------

	bool CObjECFloatBridgeManager::SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		if (m_Count < 1)
			return false;

		// Temp
		m_WavePointA.Position = CVector(100, 0, 500);
		m_WavePointB.Position = CVector(-250, 0, 700);

		float pushOffset = 0;

		for (int i = 0; i < m_Count; ++i)
		{
			int type = 1;
			if (i == 0)
				type = 0;
			if (i == m_Count - 1)
				type = 2;


			//const float offset = (CObjECFloatBridgePiece::WidthTypeA + m_GapSize);
			// FIXME: Shitty hardcoded distances idk
			const CVector position = m_spMatrixNodeTransform->m_Transform.m_Position + CVector(pushOffset, 0, 0);

			auto bridgePiece = boost::make_shared<CObjECFloatBridgePiece>(position, this, i, type);
			m_BridgePieces.push_back(bridgePiece);
			in_pGameDocument->AddGameObject(bridgePiece);

			const float offset = i == 0 || i == m_Count - 2
				? 9.56f : 4.0f;

			pushOffset += offset;
		}

		return true;
	}
	void CObjECFloatBridgeManager::KillCallback()
	{
		for (auto piece : m_BridgePieces)
		{
			piece->SendMessageImm(piece.get(), Sonic::Message::MsgKill());
		}
	}

	///////////////////////////
	//
	//  Float bridge - Pieces
	//------------------------------------------

	bool CObjECFloatBridgePiece::SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		m_spMatrixNodeTransform->m_Transform.SetPosition(m_StartPosition);
		m_spMatrixNodeTransform->NotifyChanged();

		// I guess we just set this to parent the main node? And then we get to move it...?
		m_spNodeRenderable->SetParent(m_spMatrixNodeTransform.get());

		hh::mr::CMirageDatabaseWrapper wrapper(in_spDatabase.get());
		const char* assetName = m_Type == 1 ? "seaobj_hasi_b" : "seaobj_hasi_a";
		m_spBridgePieceRenderable = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData(assetName));
		m_spBridgePieceRenderable->BindMatrixNode(m_spNodeRenderable);

		CGameObject::AddRenderable("Object", m_spBridgePieceRenderable, true);

#ifdef _PREVIEW_DEBUG
			m_spRingDebugRenderable01 = boost::make_shared<hh::mr::CSingleElement>(wrapper.GetModelData("cmn_obj_thornball_HD"));
			CGameObject::AddRenderable("Object", m_spRingDebugRenderable01, true);
#endif

		return true;
	}

	bool CObjECFloatBridgePiece::SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase)
	{
		const CVector2 railOffset = m_Type == 1
		                          ? CVector2(2.1949f, 0.72034f)
		                          : CVector2(4.4657f, 1.59683f);

		// Initiate collision root, do NOT parent it to the node transform because it needs to be asynchronous.
		Hedgehog::Mirage::CTransform* rootNodeTransform = &m_spMatrixNodeTransform->m_Transform;
		//m_spNodeCollisionRoot->m_Transform.SetRotationAndPosition(rootNodeTransform->m_Rotation, rootNodeTransform->m_Position);
		//m_spNodeCollisionRoot->NotifyChanged();

		// Gotta do this garbage i guess.
		m_spNodeCollisionRoot->SetParent(m_spMatrixNodeTransform.get());


		// Now set up our rails. Make sure to parent these to the *collision root.*
		m_spMNTransformLeftRail->m_Transform.SetPosition(CVector(0, railOffset.y(), -railOffset.x()));
		m_spMNTransformLeftRail->NotifyChanged();
		m_spMNTransformLeftRail->SetParent(m_spNodeCollisionRoot.get());

		m_spMNTransformRightRail->m_Transform.SetPosition(CVector(0, railOffset.y(), railOffset.x()));
		m_spMNTransformRightRail->NotifyChanged();
		m_spMNTransformRightRail->SetParent(m_spNodeCollisionRoot.get());

		// Reminder: Width and Length are flipped because this object is X-Forward.

		hk2010_2_0::hkpBoxShape* shapeGround = new hk2010_2_0::hkpBoxShape(m_ColliderLength, m_ColliderHeight, m_ColliderWidth);
		AddRigidBody(m_spRigidBodyFloor, shapeGround, *pColID_Common, m_spNodeCollisionRoot);
		shapeGround->removeReference();

		const CVector railHalfExtents = m_Type == 1
		                              ? CVector(3.6f, 1.0f, 0.285f)
		                              : CVector(14.375f, 1.03f, 0.323f);

		hk2010_2_0::hkpBoxShape* shapeRail = new hk2010_2_0::hkpBoxShape(railHalfExtents * 0.5f);
		AddRigidBody(m_spRigidBodyLeftRail, shapeRail,  *pColID_Common, m_spMNTransformLeftRail);
		AddRigidBody(m_spRigidBodyRightRail, shapeRail, *pColID_Common, m_spMNTransformRightRail);
		shapeRail->removeReference();
		shapeRail->removeReference();

		// Sets the floor and rails' ground attribute to Wood (ID = 5)
		m_spRigidBodyFloor->AddProperty(0x2004, 0x05);
		m_spRigidBodyLeftRail->AddProperty(0x2004, 0x05);
		m_spRigidBodyRightRail->AddProperty(0x2004, 0x05);

		// HACK: Temp fix to sample the correct point with the wave function
		m_ColliderWidth = WidthTypeA;

		// Event collision preview
		m_spNodeEventCollision = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spNodeEventCollision->m_Transform.SetPosition(CVector(0, 2, 0));
		m_spNodeEventCollision->NotifyChanged();
		m_spNodeEventCollision->SetParent(m_spNodeCollisionRoot.get());

		hk2010_2_0::hkpBoxShape* shapeEventTrigger = new hk2010_2_0::hkpBoxShape(2, 2, 2);
		AddEventCollision("Damage", shapeEventTrigger, *pColID_PlayerEvent, true, m_spNodeEventCollision);
		shapeEventTrigger->removeReference();

		// Add two more colliders for the end pieces
		if (m_Type == 1)
			return true;

		m_spMNTransformFrontLeftRail = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spMNTransformFrontRightRail = boost::make_shared<Sonic::CMatrixNodeTransform>();
		m_spMNTransformFrontLeftRail->m_Transform.SetPosition(CVector(7.012, 1.6, 3.185));
		m_spMNTransformFrontRightRail->m_Transform.SetPosition(CVector(7.012, 1.6, -3.185));
		m_spMNTransformFrontLeftRail->NotifyChanged();
		m_spMNTransformFrontRightRail->NotifyChanged();
		m_spMNTransformFrontLeftRail->SetParent(m_spNodeCollisionRoot.get());
		m_spMNTransformFrontRightRail->SetParent(m_spNodeCollisionRoot.get());

		hk2010_2_0::hkpBoxShape* shapeFrontRail = new hk2010_2_0::hkpBoxShape(0.322924, 1.028, 2.28206);
		AddRigidBody(m_spRigidBodyFrontLeftRail, shapeFrontRail,  *pColID_Common, m_spMNTransformFrontLeftRail);
		AddRigidBody(m_spRigidBodyFrontRightRail, shapeFrontRail, *pColID_Common, m_spMNTransformFrontRightRail);
		shapeFrontRail->removeReference();
		shapeFrontRail->removeReference();

		m_spRigidBodyFrontLeftRail->AddProperty(0x2004, 0x05);
		m_spRigidBodyFrontRightRail->AddProperty(0x2004, 0x05);

		return true;
	}

	void CObjECFloatBridgePiece::SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo)
	{
		if (!m_Manager)
		{
			SendMessageImm(this, Sonic::Message::MsgKill());
			return;
		}

		//return;

#ifdef _DEBUG
#define CHRONO std::chrono
#define TIME_NOW CHRONO::duration_cast<CHRONO::microseconds>(CHRONO::high_resolution_clock::now().time_since_epoch())
#define DBG_GET_TIME_NOW(x) CHRONO::microseconds x = TIME_NOW
#else
#define DBG_GET_TIME_NOW(x) 
#endif

		DBG_GET_TIME_NOW(ms_start);

		const float halfWidth = m_ColliderWidth * 0.5f;
		const float halfLength = m_ColliderLength * 0.5f;

		const float yOffsetBase = m_Type == 1
			                          ? 0.75f
			                          : -0.1f;
		const float yawAngleAdjust = m_Type == 2
			                             ? 180.0f * DEG2RAD
			                             : 0.0f;

		auto GetDip = [this]() -> CVector2
		{
			const float fCount = (float)(m_Manager->m_Count - 1);
			const float fHalfCount = fCount * 0.5f;

			// We want to ignore the end pieces entirely from this calc
			//const int i = std::max(1, std::min(m_Index, m_Manager->m_Count - 2));
			const int i = m_Index;
			const float halfDiv = 1.0f / fHalfCount;

			const float distanceNear = 1.0f - std::QuickPower(std::abs(static_cast<float>(i - 1) - fHalfCount) * halfDiv, 2);
			const float distanceFar = 1.0f - std::QuickPower(std::abs(static_cast<float>(i) - fHalfCount) * halfDiv, 2);

			return CVector2(distanceNear, distanceFar) * m_Manager->m_Depression;
		};
		const CVector2 dipAmount = GetDip();

		const CVector     basePosition = m_StartPosition;
		const CQuaternion baseRotation = CQuaternion::FromAngleAxis(m_DirectionAngle + yawAngleAdjust, CVector::Up());

		DBG_GET_TIME_NOW(ms_mid);

		//DebugDrawText::log(format("Time elapsed A: %f", (ms_mid - ms_start).count() / 1000000.0), 0.0f, 0, { 0.35, 1.0, 0.45, 1.0 });


		// FIXME: Gens is Z *BACK*, X Right, but my CVector class is built around Sonic's gameplay, which is the OPPOSITE.
		// HOWEVER, SA1 objects are *X FORWARD* by default, so... OOPS!
		const CVector front = baseRotation * CVector(1, 0, 0);
		const CVector right = baseRotation * CVector(0, 0, 1);

		const Eigen::Vector4f waveRipplesA = ComputeWave(m_Manager->m_WavePointA, right * halfWidth, front * halfLength, m_Type == 1);
		const Eigen::Vector4f waveRipplesB = ComputeWave(m_Manager->m_WavePointB, right * halfLength, front * halfWidth, m_Type == 1);

		DBG_GET_TIME_NOW(ms_after);

		//DebugDrawText::log(format("Time elapsed B: %f", (ms_after - ms_mid).count() / 1000000.0), 0.0f, 0, { 1.0, 1.0, 0.45, 1.0 });

		const Eigen::Vector4f waveFull = (waveRipplesA + waveRipplesB) * 0.5f;
		//const Eigen::Vector4f waveFull = waveRipplesA;

		// Just makes it more explicit what's what.
		const float yOffsetFront = waveFull.x() - dipAmount.x(); // far
		const float yOffsetBack = waveFull.y() - dipAmount.y(); // near
		const float yOffsetRight = waveFull.z();
		const float yOffsetLeft = waveFull.w();

		const float yOffsetCenter = (yOffsetFront + yOffsetBack) * 0.25f;

#ifdef _PREVIEW_DEBUG
			m_spRingDebugRenderable01->m_spInstanceInfo->m_Transform = Eigen::Translation3f(front + right + basePosition + CVector(0, 2, 0)) * Eigen::Scaling(0.5f);
#endif

		const float rotationPitch = CVector::SignedAngle(front,
		                                                 CVector::Normalized(front * halfLength
		                                                                     + CVector(0, (yOffsetBack - yOffsetFront) * 0.35f, 0)),
		                                                 right);

		const float rotationRoll = CVector::SignedAngle(right,
		                                                CVector::Normalized(right * halfWidth
		                                                                    + CVector(0, (yOffsetRight - yOffsetLeft) * 0.35f, 0)),
		                                                front);

		const CQuaternion tiltRotation =
			CQuaternion::FromAngleAxis(rotationPitch, right) * CQuaternion::FromAngleAxis(rotationRoll, front) * baseRotation;
		//CQuaternion::FromAngleAxis(rotationPitch, right) * baseRotation;


		DBG_GET_TIME_NOW(ms_end);

#ifdef _DEBUG
		//long long timeElapsed = (ms_end - ms_start).count();
		//DebugDrawText::log(format("Time elapsed: %f", timeElapsed / 1000000.0), 0.0f, 0, {1.0, 1.0, 0.45, 1.0});
#endif

		// Debug
#if 0
			const auto translation1 = Eigen::Translation3f(CVector(0, yOffsetFront, -halfLength) + basePosition);
			m_spRingDebugRenderable01->m_spInstanceInfo->m_Transform = translation1;
			m_spRingDebugRenderable01->m_spInstanceInfo->m_PrevTransform = translation1;

			const auto translation2 = Eigen::Translation3f(CVector(halfWidth, yOffsetRight, 0) + basePosition);
			m_spRingDebugRenderable02->m_spInstanceInfo->m_Transform = translation2;
			m_spRingDebugRenderable02->m_spInstanceInfo->m_PrevTransform = translation2;
#endif

		const CVector tiltPosition(0, yOffsetCenter + yOffsetBase, 0);

		m_spNodeRenderable->m_Transform.SetRotationAndPosition(tiltRotation, tiltPosition);
		m_spNodeRenderable->NotifyChanged();

		// HACK: Get sonic context this way whatever we'll do it better later.
		//auto sonic = Sonic::Player::CPlayerSpeedContext::GetInstance();
		//if (CVector::LengthSqr(sonic->m_spMatrixNode->m_Transform.m_Position - m_spMatrixNodeTransform->m_Transform.m_Position) >= 15.0f * 15.0f)
		//	return;

		m_spNodeCollisionRoot->m_Transform.SetRotationAndPosition(tiltRotation, tiltPosition);
		m_spNodeCollisionRoot->NotifyChanged();
	}
}

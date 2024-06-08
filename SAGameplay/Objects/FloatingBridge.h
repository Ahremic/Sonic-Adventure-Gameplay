#pragma once

namespace SetObjectsCustom
{
	// Forward declarations
	class CObjECFloatBridgeManager;
	class CObjECFloatBridgePiece;

	// Control unit
	class CObjECFloatBridgeManager : public Sonic::CObjectBase, public Sonic::CSetObjectListener
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CQuaternion CQuaternion;

	public:
		BB_SET_OBJECT_MAKE("FloatBridge")

		struct WavePoint
		{
			CVector Position = CVector::Zero();
			float Amplitude = 1.0f;
			float Frequency = 1.0f;
			float WaveSize = 2.5f;
			float WaveHeight_Length = 0.5f;
			float WaveHeight_Width = 0.25f;

			// Cache for speed
			float WaveSizeInverse = WaveSize < FLT_EPSILON
			                      ? 0.0f
			                      : (1.0f / WaveSize);
		};

		int m_Count = 1;
		float m_Depression = 2.0f;

		WavePoint m_WavePointA;
		WavePoint m_WavePointB;

		float m_CurrentTime = 0.0f;
		//float m_GapSize = 0.25f;
		//float m_GapSize = 0.05f;
		float m_GapSize = 0.0f;

		std::vector<boost::shared_ptr<CObjECFloatBridgePiece>> m_BridgePieces;

		void InitializeEditParam(Sonic::CEditParam& in_rEditParam) override
		{
			in_rEditParam.CreateParamInt(&m_Count, "Count");
		}

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override
		{
			m_CurrentTime += updateInfo.DeltaTime;
		}

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;
		void KillCallback() override;
	};
	BB_SET_OBJECT_MAKE_HOOK(CObjECFloatBridgeManager)

	class CObjECFloatBridgePiece : public Sonic::CObjectBase
	{
		typedef hh::math::CVector CVector;
		typedef hh::math::CVector2 CVector2;
		typedef hh::math::CQuaternion CQuaternion;

	public:
		CObjECFloatBridgeManager* m_Manager;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyFloor;
		boost::shared_ptr<hh::mr::CSingleElement> m_spBridgePieceRenderable;

		// This is necessary for us to avoid updating collision when we are not close to the bridge segment.
		// Updating colliders like this is expensive, so if we don't HAVE to do it, we SHOULDN'T.
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spNodeCollisionRoot;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spNodeRenderable;

		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMNTransformLeftRail;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMNTransformRightRail;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyLeftRail;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyRightRail;

		// End pieces only
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMNTransformFrontLeftRail;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spMNTransformFrontRightRail;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyFrontLeftRail;
		boost::shared_ptr<Sonic::CRigidBody> m_spRigidBodyFrontRightRail;

		int m_Index = 0;
		int m_Type = 1;

		// Debug
		boost::shared_ptr<hh::mr::CSingleElement> m_spRingDebugRenderable01;
		boost::shared_ptr<hh::mr::CSingleElement> m_spRingDebugRenderable02;
		boost::shared_ptr<Sonic::CMatrixNodeTransform> m_spNodeEventCollision;

		static inline float WidthTypeA = 5.0f;
		static inline float WidthTypeB = 10.025f;

		static inline float LengthTypeA = 4.0f;
		static inline float LengthTypeB = 15.0f;

		float m_DirectionAngle = 0.0f;
		float m_BankingMultiplier = 1.0f;

		CVector m_StartPosition = CVector::Zero();

		float m_ColliderLength = LengthTypeA;
		float m_ColliderWidth = WidthTypeA;
		float m_ColliderHeight = 0.45f;

		CObjECFloatBridgePiece(const CVector& spawnPosition, CObjECFloatBridgeManager* manager, int index, int type = 1)
			: m_Manager(manager),
			m_Index(index),
			m_Type(type),
			m_StartPosition(spawnPosition)
		{
			m_spNodeCollisionRoot = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_spNodeRenderable = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_spMNTransformLeftRail = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_spMNTransformRightRail = boost::make_shared<Sonic::CMatrixNodeTransform>();
			m_BankingMultiplier = type == 1 ? 1.0f : 0.5f;
			if (type == 2)
				m_BankingMultiplier *= 0.65f; // Weird bug is causing the 3rd platform to rotate more...

			if (type != 1)
			{
				m_ColliderWidth = 10.025f;
				m_ColliderLength = 15.0f;
				m_ColliderHeight = 2.225f;
			}
		}

		//#define _PREVIEW_DEBUG

		Eigen::Vector4f ComputeWave(const CObjECFloatBridgeManager::WavePoint& wavePoint, const CVector& rightEdge, const CVector& frontEdge, bool isCenter = true)
		{
			// Costly implementation
#if 0
			// Old, costlier method
			// NOTE- apparently this isnt any more expensive...?
			const float waveWidth = wavePoint.WaveSize < FLT_EPSILON
				? 0.0f
				: (1.0f / wavePoint.WaveSize);

			//const float waveWidth = wavePoint.WaveSizeInverse;

			const float t = m_Manager->m_CurrentTime * wavePoint.Frequency;

			const CVector rightPlusFront = rightEdge + frontEdge;
			const CVector RightEdge = isCenter ? rightEdge : rightPlusFront;

			Eigen::Vector4f result(
				sinf(CVector::Distance(-frontEdge + m_StartPosition, wavePoint.Position) * waveWidth + t) * wavePoint.WaveHeight_Length * m_BankingMultiplier,
				sinf(CVector::Distance(frontEdge + m_StartPosition, wavePoint.Position) * waveWidth + t) * wavePoint.WaveHeight_Length * m_BankingMultiplier,
				sinf(CVector::Distance(RightEdge + m_StartPosition, wavePoint.Position) * waveWidth + t) * wavePoint.WaveHeight_Width * m_BankingMultiplier,
				sinf(CVector::Distance(-RightEdge + m_StartPosition, wavePoint.Position) * waveWidth + t) * wavePoint.WaveHeight_Width * m_BankingMultiplier
			);
			return result;
#endif
			// Eigen-friendly implementation thanks to Skyth

#ifdef _DEBUG
//#define _FAST_BRIDGE
#endif

//#ifndef _FAST_BRIDGE
			const float waveWidth = wavePoint.WaveSize < FLT_EPSILON
				? 0.0f
				: (1.0f / wavePoint.WaveSize);

			const float t = m_Manager->m_CurrentTime * wavePoint.Frequency;

			const CVector rightPlusFront = rightEdge + frontEdge;
			const CVector RightEdge = isCenter ? rightEdge : rightPlusFront;

			Eigen::Vector4f result;
			result.x() = CVector::Distance(-frontEdge + m_StartPosition, wavePoint.Position);
			result.y() = CVector::Distance(frontEdge + m_StartPosition, wavePoint.Position);
			result.z() = CVector::Distance(RightEdge + m_StartPosition, wavePoint.Position);
			result.w() = CVector::Distance(-RightEdge + m_StartPosition, wavePoint.Position);

			// these will compile to mulps addps
			result *= waveWidth;
			result += Eigen::Vector4f(t, t, t, t); // Doesn't do this on its own I guess.

			// i think eigen had some cwise sin functions for stuff like this i dont remember
			// If it does, the type is incomplete sadly.
			result.x() = sinf(result.x());
			result.y() = sinf(result.y());
			result.z() = sinf(result.z());
			result.w() = sinf(result.w());

			// should compile to shuffles if the compiler is smart
			const Eigen::Vector4f factor
			(
				wavePoint.WaveHeight_Length,
				wavePoint.WaveHeight_Length,
				wavePoint.WaveHeight_Width,
				wavePoint.WaveHeight_Width
			);

			// will compile to mulps
			result = result.cwiseProduct(factor);
			result *= m_BankingMultiplier;

			return result;
			//#else
						//float height = sinf(((m_StartPosition.x() + m_StartPosition.y()) * 0.5f) + m_Manager->m_CurrentTime * wavePoint.Frequency);
			float height = 0.0f;
			return Eigen::Vector4f(height, height, height, height);
			//#endif
		}

		bool SetAddRenderables(Sonic::CGameDocument* in_pGameDocument, const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;

		bool SetAddColliders(const boost::shared_ptr<Hedgehog::Database::CDatabase>& in_spDatabase) override;

		void SetUpdateParallel(const Hedgehog::Universe::SUpdateInfo& updateInfo) override;
	};

}
#pragma once

// Todo: Add these to blueblur

namespace Sonic
{
	class CPathController;
	class KeyframedPath;
	class PathAnimationController;

	class __declspec(align(4)) IPathAnimation
	{
	public:

		virtual ~IPathAnimation() {}

	protected:
		virtual bool   fn01(int a1, int a2, int a3, int a4) = 0;
		virtual double fn02(float a2) = 0;
		virtual bool vSamplePath(float pathDistance, Hedgehog::Math::CVector* closestPoint, Hedgehog::Math::CVector* upVector, Hedgehog::Math::CVector* pathDirection) = 0;
		virtual bool vSamplePathGranular(float pathDistance, Hedgehog::Math::CVector* closestPoint, Hedgehog::Math::CVector* upVector, Hedgehog::Math::CVector* pathDirection) = 0;
		virtual float GetPathLength() = 0;
		virtual double WrapValue(float a2) = 0;
	};
	class __declspec(align(4)) KeyframedPath : public IPathAnimation
	{
	public:
		typedef Hedgehog::Math::CVector    CVector;
		typedef Hedgehog::Math::CMatrix44  CMatrix44;

		bool   fn01(int a1, int a2, int a3, int a4) override { return true; }
		double fn02(float a2) override
		{
			FUNCTION_PTR(double, __thiscall, _FUNC, 0x00E7F7F3, void* This, float _a2);
			return _FUNC(this, a2);
		}
	protected:
		bool vSamplePath(float pathDistance, CVector* closestPoint, CVector* upVector, CVector* pathDirection) override
		{
			FUNCTION_PTR(bool, __thiscall, _FUNC, 0x00E7F630, void* This, float _a1, void* _a2, void* _a3, void* _a4);
			return _FUNC(this, pathDistance, closestPoint, upVector, pathDirection);
		}
		bool vSamplePathGranular(float pathDistance, CVector* closestPoint, CVector* upVector, CVector* pathDirection) override
		{
			FUNCTION_PTR(bool, __thiscall, _FUNC, 0x00E80490, void* This, float _a1, void* _a2, void* _a3, void* _a4);
			return _FUNC(this, pathDistance, closestPoint, upVector, pathDirection);
		}

	public:
		bool SamplePathGranular(float in_PathDistance, CVector* out_pClosestPoint, CVector* out_pUpVector, CVector* out_pPathDirection)
		{
			// Allow nullptr input for discarded writes.
			CVector temp(0, 0, 0);

			CVector* a = out_pClosestPoint  ? out_pClosestPoint  : &temp;
			CVector* b = out_pUpVector      ? out_pUpVector      : &temp;
			CVector* c = out_pPathDirection ? out_pPathDirection : &temp;
			return vSamplePathGranular(in_PathDistance, a, b, c);
		}

		bool SamplePath(float in_PathDistance, CVector* in_pClosestPoint, CVector* in_pUpVector, CVector* in_pPathDirection)
		{
			// Allow nullptr input for discarded writes.
			CVector temp(0, 0, 0);

			CVector* a = in_pClosestPoint  ? in_pClosestPoint  : &temp;
			CVector* b = in_pUpVector      ? in_pUpVector      : &temp;
			CVector* c = in_pPathDirection ? in_pPathDirection : &temp;
			return vSamplePath(in_PathDistance, a, b, c);
		}

		float GetPathLength() override
		{
			return m_PathLength;
		}
		double WrapValue(float a2) override
		{
			FUNCTION_PTR(double, __thiscall, _FUNC, 0x00E7F550, void* This, float _a2);
			return _FUNC(this, a2);
		}

		void* m_PointData = nullptr;
		int m_FirstPointIndex = 0;
		int m_LastPointIndex = 0;
		float m_PathLength = 0;
		boost::shared_ptr<void> m_spSplinePathSpline; // Sonic::SplinePath::Spline

		CMatrix44 m_Matrix;

		int m_Unk24 = 0;
		int m_Unk25 = 0;
		int m_Unk26 = 0;
		int m_Unk27 = 0;
		int m_CurrentPoint = 0;
		int m_Unk29 = 0;
		int m_Unk30 = 0;
		int m_Unk31 = 0;
	};
	ASSERT_OFFSETOF(KeyframedPath, m_Matrix, 0x20);
	ASSERT_SIZEOF(KeyframedPath, 0x80);

	class __declspec(align(4)) PathAnimationEntity
	{
		void* __vftable;
		int m_Unk00;
		int m_Unk01;
		int m_Unk02;
	public:
		boost::shared_ptr<KeyframedPath> m_spKeyframedPath;
	};

	class __declspec(align(16)) PathAnimationController
	{
		void* __vftable;
		int gap04;
	public:
		__declspec(align(16)) PathAnimationEntity* m_pAnimationEntity;
		int gap14;
		int field_18;
		int m_Self;
		float m_DistanceAlongPath;
		Hedgehog::Math::CVector field_30;
		Hedgehog::Math::CVector field_40;
		Hedgehog::Math::CVector m_pathDireciton;
		char gap60[16];
	};

	static constexpr uint32_t pCPathControllerGetFromID = 0x00D64DD0;
	static void __declspec(noinline) fCPathControllerGetFromID(
		CGameActParameter* in_pGameActParameter,
		int in_PathID,
		boost::shared_ptr<CPathController>* in_pspPathController)
	{
		__asm
		{
			mov eax, in_pGameActParameter
			mov ecx, in_PathID
			mov edi, in_pspPathController
			call [pCPathControllerGetFromID]
		}
	}

	class CPathController
	{
	public:
		boost::shared_ptr<PathAnimationEntity> m_spPathAnimationEntity;
		boost::shared_ptr<PathAnimationController> m_spPathAnimationController;
		Hedgehog::Math::CMatrix44 m_Matrix;
		bool m_TransformPosition;
		int field_54;
		int field_58;
		int field_5C;
		Hedgehog::Base::CSharedString m_Name;
		int field_64;
		int field_68;
		int field_6C;

		static boost::shared_ptr<CPathController> GetFromID(uint32_t in_PathID)
		{
			boost::shared_ptr<CPathController> result;
			fCPathControllerGetFromID(CGameDocument::GetInstance()->m_pGameActParameter, in_PathID, &result);
			return result;
		}

		float GetPathLength() const
		{
			return m_spPathAnimationEntity->m_spKeyframedPath->m_PathLength;
		}

		bool GetPosition(float in_PathDistance, Hedgehog::Math::CVector* out_pPosition) const
		{
			const bool pathValid =
				m_spPathAnimationEntity->m_spKeyframedPath->SamplePath(in_PathDistance, out_pPosition, nullptr, nullptr);

			if (!pathValid)
				return false;

			if (!m_TransformPosition)
				return true;

			BB_FUNCTION_PTR(void, __thiscall, TransformCoordinate, 0x9BE6B0,
				const Hedgehog::Math::CMatrix44 & in_rMatrix, Hedgehog::Math::CVector* out_Result, Hedgehog::Math::CVector* in_Vector);

			TransformCoordinate(m_Matrix, out_pPosition, out_pPosition);

			return true;
		}
	};

}

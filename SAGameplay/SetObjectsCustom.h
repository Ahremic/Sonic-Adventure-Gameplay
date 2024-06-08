#pragma once
#include <Sonic/Havok/RigidBody.h>

namespace Sonic
{
	class CParticleController;

	class CGlitterPlayer : public Hedgehog::Base::CObject
	{
	public:
		virtual ~CGlitterPlayer();

	private:
		virtual void PlayOneshotByMatrix(const Hedgehog::Math::CMatrix& in_Matrix, const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int in_ID) {}
		virtual void PlayOneshotByNode(const boost::shared_ptr<Hedgehog::Mirage::CMatrixNode>& in_MatrixNode, const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int in_ID) {}

		virtual bool PlayContinuousByMatrix(Hedgehog::Base::THolder<CGameDocument> in_Holder,
			const Hedgehog::Math::CMatrix& in_Matrix,
			const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int usuallyOne,
			int usuallyZero)
		{
			return false;
		}
		virtual bool PlayContinuousByNode(Hedgehog::Base::THolder<CGameDocument> in_Holder,
			const boost::shared_ptr<Hedgehog::Mirage::CMatrixNode>& in_spNode,
			const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int usuallyOne,
			int usuallyZero)
		{
			return false;
		}

	public:

		// I prefer function overloads personally, so I'm doing it this way.
		void PlayOneshot(const Hedgehog::Math::CMatrix& in_Matrix, const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int in_ID)
		{
			PlayOneshotByMatrix(in_Matrix, in_AssetName, in_Size, in_ID);
		}
		void PlayOneshot(const boost::shared_ptr<Hedgehog::Mirage::CMatrixNode>& in_MatrixNode, const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int in_ID)
		{
			PlayOneshotByNode(in_MatrixNode, in_AssetName, in_Size, in_ID);
		}
		bool PlayContinuous(const Hedgehog::Base::TSynchronizedPtr<Sonic::CGameDocument>& pGameDocument,
			const boost::shared_ptr<Hedgehog::Mirage::CMatrixNode>& in_spNode,
			const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int usuallyOne = 1, int usuallyZero = 0)
		{
			return PlayContinuousByNode(pGameDocument.get(), in_spNode, in_AssetName, in_Size, usuallyOne, usuallyZero);
		}
		bool PlayContinuous(const Hedgehog::Base::TSynchronizedPtr<Sonic::CGameDocument>& pGameDocument,
			const Hedgehog::Math::CMatrix& in_Matrix,
			const Hedgehog::Base::CSharedString& in_AssetName, float in_Size, int usuallyOne = 1, int usuallyZero = 0)
		{
			return PlayContinuousByMatrix(pGameDocument.get(), in_Matrix, in_AssetName, in_Size, usuallyOne, usuallyZero);
		}

		CParticleController* m_pParticleController;

		static CGlitterPlayer* Make(CGameDocument* pGameDocument)
		{
			BB_FUNCTION_PTR(CGlitterPlayer*, __cdecl, Func, 0x01255B40, Sonic::CGameDocument * in_pGameDocument);
			CGlitterPlayer* result = Func(pGameDocument);

			return result;
		}
	};

	// TODO: Move to BlueBlur

	class CRigidBodyContainer
	{
	public:
		boost::shared_ptr<Sonic::CRigidBody> GetRigidBody(const Hedgehog::Base::CSharedString& rbName, Hedgehog::Universe::CMessageActor* pMessageActor) volatile
		{
			static constexpr int func = 0x1180300;

			boost::shared_ptr<Sonic::CRigidBody> spResult = boost::make_shared<Sonic::CRigidBody>();
			void* pResult = &spResult;

			__asm
			{
				push pMessageActor
				push pResult
				push this
				mov edi, rbName
				call func
			}

			return spResult;
		}
	};

	class CHavokDatabaseWrapper
	{
		static void fGetRigidBodyContainer(const Hedgehog::Base::CSharedString* containerName, const CHavokDatabaseWrapper* This, const boost::shared_ptr<CRigidBodyContainer>* pResult)
		{
			static constexpr int func = 0x010C0B20;
			__asm
			{
				push pResult
				push This
				mov edi, containerName
				call func
			}
		}

	public:
		Hedgehog::Database::CDatabase* m_pDatabase;

		virtual ~CHavokDatabaseWrapper() = default;

		CHavokDatabaseWrapper(Hedgehog::Database::CDatabase* pDatabase)
		{
			// set vtable
			*(int*)this = 0x016A1664;
			m_pDatabase = pDatabase;
		}

		void GetRigidBodyContainer(const Hedgehog::Base::CSharedString& containerName, const boost::shared_ptr<CRigidBodyContainer>& spContainer) const
		{
			fGetRigidBodyContainer(&containerName, this, &spContainer);
		}

		boost::shared_ptr<CRigidBodyContainer> GetRigidBodyContainer(const Hedgehog::Base::CSharedString& containerName) const
		{
			boost::shared_ptr<CRigidBodyContainer> spContainer;
			fGetRigidBodyContainer(&containerName, this, &spContainer);
			return spContainer;
		}
	};

}

namespace SetObjectsCustom
{
	// Global collider types
	// TODO: Move to somewhere else, these are StringMap/StringEnum's
	static const int* pColID_BasicTerrain = reinterpret_cast<int*>(0x01E0AFAC);
	static const int* pColID_Common = reinterpret_cast<int*>(0x01E0AF30);
	static const int* pColID_PlayerEvent = reinterpret_cast<int*>(0x01E0AFD8);

	void Init();
}
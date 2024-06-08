#include "SetEditor.h"

#if 0
// https://stackoverflow.com/a/64471501
std::wstring to_wide(const std::string& multi)
{
	std::wstring wide;
	wchar_t w;
	mbstate_t mb {};
	size_t n = 0, len = multi.length() + 1;
	while (auto res = mbrtowc(&w, multi.c_str() + n, len - n, &mb))
	{
		if (res == size_t(-1) || res == size_t(-2))
			//throw "invalid encoding";
			return nullptr;

		n += res;
		wide += w;
	}
	return wide;
}

// https://stackoverflow.com/a/5253245
std::string space2underscore(std::string text)
{
	std::replace(text.begin(), text.end(), ' ', '_');
	return text;
}

namespace xercesc_2_8
{
	//typedefs
	typedef void DOMTypeInfoImpl;

	// Forward declares:
	struct DOMNode;
	struct DOMNamedNodeMap;
	struct DOMUserDataHandler;
	struct DOMDocumentImpl;

	struct DOMNodeList
	{
		struct DOMNodeList_vtbl
		{
			void(__thiscall * DOMNodeListDtor)(DOMNodeList* This);
			DOMNode*  item(DOMNodeList* This, unsigned int);
			unsigned int getLength(DOMNodeList* This);
		};

		DOMNodeList_vtbl* __vftable /*VFT*/;
	};

	struct DOMNode
	{
		virtual ~DOMNode() = default;
		virtual const wchar_t* getNodeName();
		virtual const wchar_t* getNodeValue();
		virtual __int16 getNodeType();
		virtual DOMNode* getParentNode();
		virtual DOMNodeList* getChildNodes();
		virtual DOMNode* getFirstChild();
		virtual DOMNode* getLastChild();
		virtual DOMNode* getPreviousSibling();
		virtual DOMNode* getNextSibling();
		virtual DOMNamedNodeMap* getAttributes();
		virtual DOMDocumentImpl* getOwnerDocument();
		virtual DOMNode* cloneNode(bool);
		virtual DOMNode* insertBefore(DOMNode*, DOMNode*);
		virtual DOMNode* replaceChild(DOMNode*, DOMNode*);
		virtual DOMNode* removeChild(DOMNode*);
		virtual DOMNode* appendChild(DOMNode*);
		virtual bool hasChildNodes();
		virtual void setNodeValue(const wchar_t*);
		virtual void normalize();
		virtual bool isSupported(const wchar_t*, const wchar_t*);
		virtual const wchar_t* getNamespaceURI();
		virtual const wchar_t* getPrefix();
		virtual const wchar_t* getLocalName();
		virtual void setPrefix(const wchar_t*);
		virtual bool hasAttributes();
		virtual bool isSameNode(const DOMNode*);
		virtual bool isEqualNode(const DOMNode*);
		virtual void* setUserData(const wchar_t*, void*, DOMUserDataHandler*);
		virtual void* getUserData(const wchar_t*);
		virtual const wchar_t* getBaseURI();
		virtual __int16 compareTreePosition(const DOMNode*);
		virtual const wchar_t* getTextContent();
		virtual void setTextContent(const wchar_t*);
		virtual const wchar_t* lookupNamespacePrefix(const wchar_t*, bool);
		virtual bool isDefaultNamespace(const wchar_t*);
		virtual const wchar_t* lookupNamespaceURI(const wchar_t*);
		virtual DOMNode* getInterface(const wchar_t*);
		virtual void release();

		// Helper functions
		void setNodeValue(const Hedgehog::Base::CSharedString& in_string)
		{
			setNodeValue(to_wide(space2underscore(in_string.c_str())).c_str());
		}
		void setNodeValue(const char* in_string)
		{
			setNodeValue(to_wide(space2underscore(in_string)).c_str());
		}

		void setTextContent(const Hedgehog::Base::CSharedString& in_string)
		{
			setTextContent(to_wide(space2underscore(in_string.c_str())).c_str());
		}
		void setTextContent(const char* in_string)
		{
			setTextContent(to_wide(space2underscore(in_string)).c_str());
		}
	};

	//struct DOMAttrMapImpl : DOMNamedNodeMap
	//{
	//	DOMNodeVector* fNodes;
	//	DOMNode* fOwnerNode;
	//	bool attrDefaults;
	//};


	// /////////////////////////////////
	//
	// /////////////////////////////////

	/*
	struct DOMElementImpl : DOMElement
	{
		DOMNodeImpl fNode;
		DOMParentNode fParent;
		DOMChildNode fChild;
		DOMAttrMapImpl* fAttributes;
		DOMAttrMapImpl* fDefaultAttributes;
		const wchar_t* fName;
	};


	struct DOMElementNSImpl
		: DOMElementImpl
	{
		const wchar_t* fNamespaceURI;
		const wchar_t* fLocalName;
		const wchar_t* fPrefix;
		const DOMTypeInfoImpl* fSchemaType;
	};
	*/

	struct DOMAttrMapImpl;
	struct DOMAttr;
	struct DOMTypeInfo;

	typedef DOMNode DOMElement;

	struct DOMDocumentImpl
	{
		// temp
		typedef void DOMConfiguration;
		typedef void DOMEntity;
		typedef void DOMDocumentType;
		typedef void DOMNotation;
		typedef void RefVectorOf_DOMRangeImpl;
		typedef void RefVectorOf_DOMNodeIteratorImpl;

		typedef void DOMRange;
		typedef void DOMDocumentFragment;
		typedef void DOMText;
		typedef void DOMComment;
		typedef void DOMCDATASection;
		typedef void DOMProcessingInstruction;
		typedef void DOMEntityReference;
		typedef void DOMImplementation;

		typedef void DOMRangeImpl;
		typedef void DOMNodeIteratorImpl;
		typedef void DOMDocumentRange;


		virtual ~DOMDocumentImpl() = default;

		virtual DOMRange*  createRange();
		virtual DOMElement*  createElement(const wchar_t*);
		virtual DOMDocumentFragment*  createDocumentFragment();
		virtual DOMText*  createTextNode(const wchar_t*);
		virtual DOMComment*  createComment(const wchar_t*);
		virtual DOMCDATASection*  createCDATASection(const wchar_t*);
		virtual DOMProcessingInstruction*  createProcessingInstruction(const wchar_t*, const wchar_t*);
		virtual DOMAttr*  createAttribute(const wchar_t*);
		virtual DOMEntityReference*  createEntityReference(const wchar_t*);
		virtual DOMDocumentType*  getDoctype();
		virtual DOMImplementation*  getImplementation();
		virtual DOMElement*  getDocumentElement();
		virtual DOMNodeList*  getElementsByTagName(const wchar_t*);
		virtual DOMNode*  importNode(DOMNode*, bool);
		virtual DOMElement*  createElementNS(const wchar_t*, const wchar_t*, const int, const int);
		virtual DOMElement*  createElementNS_2(const wchar_t*, const wchar_t*);
		virtual DOMAttr*  createAttributeNS(const wchar_t*, const wchar_t*);
		virtual DOMNodeList*  getElementsByTagNameNS(const wchar_t*, const wchar_t*);
		virtual DOMElement*  getElementById(const wchar_t*);
		virtual const wchar_t*  getActualEncoding();
		virtual void setActualEncoding(const wchar_t*);
		virtual const wchar_t*  getEncoding();
		virtual void setEncoding(const wchar_t*);
		virtual bool getStandalone();
		virtual void setStandalone(bool);
		virtual const wchar_t*  getVersion();
		virtual void setVersion(const wchar_t*);
		virtual const wchar_t*  getDocumentURI();
		virtual void setDocumentURI(const wchar_t*);
		virtual bool getStrictErrorChecking();
		virtual void setStrictErrorChecking(bool);
		virtual DOMNode*  renameNode(DOMNode*, const wchar_t*, const wchar_t*);
		virtual DOMNode*  adoptNode(DOMNode*);
		virtual void normalizeDocument();

		virtual DOMConfiguration*  getDOMConfiguration();
		virtual DOMEntity*  createEntity(const wchar_t*);
		virtual DOMDocumentType*  createDocumentType(const wchar_t*, const wchar_t*, const wchar_t*);
		virtual DOMDocumentType*  createDocumentType_2(const wchar_t*);
		virtual DOMNotation*  createNotation(const wchar_t*);
		virtual DOMElement*  createElementNoCheck(const wchar_t*);
		virtual RefVectorOf_DOMRangeImpl*  getRanges();
		virtual RefVectorOf_DOMNodeIteratorImpl*  getNodeIterators();

		virtual void removeRange(DOMRangeImpl*);
		virtual void removeNodeIterator(DOMNodeIteratorImpl*);
		virtual void changed();
		virtual int changes();
		virtual DOMNode*  importNode_2(DOMNode*, bool, bool);
		virtual void setDOMConfiguration(DOMConfiguration*);


		// Helper funcs
		DOMElement* createElement(const Hedgehog::Base::CSharedString& in_string)
		{
			return createElement(to_wide(space2underscore(in_string.c_str())).c_str());
		}
		DOMElement* createElement(const char* in_string)
		{
			return createElement(to_wide(space2underscore(in_string)).c_str());
		}
	};

	struct DOMElementNSImpl
	{
		virtual ~DOMElementNSImpl() = default;
		virtual const wchar_t*  getNodeName();
		virtual const wchar_t* getNodeValue();
		virtual __int16 getNodeType();
		virtual DOMNode*  getParentNode();
		virtual DOMNodeList*  getChildNodes();
		virtual DOMNode*  getFirstChild();
		virtual DOMNode*  getLastChild();
		virtual DOMNode*  getPreviousSibling();
		virtual DOMNode*  getNextSibling();
		virtual DOMNamedNodeMap*  getAttributes();
		virtual DOMDocumentImpl*  getOwnerDocument();
		virtual DOMNode*  cloneNode(bool);
		virtual DOMNode*  insertBefore(DOMNode*, DOMNode*);
		virtual DOMNode*  replaceChild(DOMNode*, DOMNode*);
		virtual DOMNode*  removeChild(DOMNode*);
		virtual DOMNode*  appendChild(DOMNode*);
		virtual bool hasChildNodes();
		virtual void setNodeValue(const wchar_t*);
		virtual void normalize();
		virtual bool isSupported(const wchar_t*, const wchar_t*);
		virtual const wchar_t*  getNamespaceURI();
		virtual const wchar_t*  getPrefix();
		virtual const wchar_t*  getLocalName();
		virtual void setPrefix(const wchar_t*);
		virtual bool hasAttributes();
		virtual bool isSameNode(const DOMNode*);
		virtual bool isEqualNode(const DOMNode*);
		virtual void*  setUserData(const wchar_t*, void*, DOMUserDataHandler*);
		virtual void*  getUserData(const wchar_t*);
		virtual const wchar_t*  getBaseURI();
		virtual __int16 compareTreePosition(const DOMNode*);
		virtual const wchar_t*  getTextContent();
		virtual void setTextContent(const wchar_t*);
		virtual const wchar_t*  lookupNamespacePrefix(const wchar_t*, bool);
		virtual bool isDefaultNamespace(const wchar_t*);
		virtual const wchar_t*  lookupNamespaceURI(const wchar_t*);
		virtual DOMNode*  getInterface(const wchar_t*);
		virtual void release();
		virtual const wchar_t*  getTagName();
		virtual const wchar_t*  getAttribute(const wchar_t*);
		virtual DOMAttr*  getAttributeNode(const wchar_t*);
		virtual DOMNodeList*  getElementsByTagName(const wchar_t*);
		virtual void setAttribute(const wchar_t*, const wchar_t*);
		virtual DOMAttr*  setAttributeNode(DOMAttr*);
		virtual DOMAttr*  removeAttributeNode(DOMAttr*);
		virtual void removeAttribute(const wchar_t*);
		virtual const wchar_t*  getAttributeNS(const wchar_t*, const wchar_t*);
		virtual void setAttributeNS(const wchar_t*, const wchar_t*, const wchar_t*);
		virtual void removeAttributeNS(const wchar_t*, const wchar_t*);
		virtual DOMAttr*  getAttributeNodeNS(const wchar_t*, const wchar_t*);
		virtual DOMAttr*  setAttributeNodeNS(DOMAttr*);
		virtual DOMNodeList*  getElementsByTagNameNS(const wchar_t*, const wchar_t*);
		virtual bool hasAttribute(const wchar_t*);
		virtual bool hasAttributeNS(const wchar_t*, const wchar_t*);
		virtual void setIdAttribute(const wchar_t*);
		virtual void setIdAttributeNS(const wchar_t*, const wchar_t*);
		virtual void setIdAttributeNode(const DOMAttr*);
		virtual const DOMTypeInfo*  getTypeInfo();
		virtual void setReadOnly(bool, bool);
		virtual DOMAttr*  setDefaultAttributeNode(DOMAttr*);
		virtual DOMAttr*  setDefaultAttributeNodeNS(DOMAttr*);
		virtual DOMAttrMapImpl*  getDefaultAttributes();
		virtual DOMNode*  rename(const wchar_t*, const wchar_t*);
		virtual void setupDefaultAttributes();
		virtual void setTypeInfo(const DOMTypeInfoImpl*);
	};

}

// FIXME: put this elsewhere!!
namespace Sonic
{
	// TODO: Ask Skyth how to go about these...

	// No definition necessary as this type is only ever passed as a pointer for the game to handle.
	// Only declaring this in the first place to ensure type safety.
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
		void AddInt(const char* name, uint32_t* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3780, CXMLTypeSLTxt * This, const char* a2, uint32_t* a3);
			AddParam(this, name, value);
		}
		void AddVector(const char* name, Hedgehog::Math::CVector* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3A00, CXMLTypeSLTxt * This, const char* a2, Hedgehog::Math::CVector * a3);
			AddParam(this, name, value);
		}
		void AddRotation(const char* name, Hedgehog::Math::CQuaternion* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3820, CXMLTypeSLTxt * This, const char* a2, Hedgehog::Math::CQuaternion * a3);
			AddParam(this, name, value);
		}
		void AddBool(const char* name, bool* value)
		{
			BB_FUNCTION_PTR(void, __stdcall, AddParam, 0x00CE3B40, CXMLTypeSLTxt * This, const char* a2, bool* a3);
			AddParam(this, name, value);
		}

		void ApplyParamsToElement(xercesc_2_8::DOMElementNSImpl* element)
		{
			FUNCTION_PTR(void*, __stdcall, func, 0x00CE28B0, Sonic::CXMLTypeSLTxt * This, xercesc_2_8::DOMElementNSImpl** Elem);
			func(this, &element);
		}

		void ApplyParamsToElement(xercesc_2_8::DOMElement* element)
		{
			FUNCTION_PTR(void*, __stdcall, func, 0x00CE28B0, Sonic::CXMLTypeSLTxt * This, xercesc_2_8::DOMElement** Elem);
			func(this, &element);
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

		//int buffer[1030]{};
		BB_INSERT_PADDING(0x1018) {};
		CMember* m_pMember = nullptr;
	};
	BB_ASSERT_SIZEOF(CXMLTypeSLTxt, 0x101C);

	class CGameplayFlowStageAct : public Hedgehog::Universe::CStateMachineBase::CStateBase
	{
	public:

		int m_Field060;
		int m_Field064;
		int m_Field068;
		int m_Field06C;
		int m_Field070;
		int IUpdateCoordinator;
		int m_Field078;
		int m_Field07C;
		int m_Field080;
		int m_Field084;
		int m_Field088;
		int m_Field08C;
		int m_Field090;
		int m_Field094;
		int m_Field098;
		int m_Field09C;
		int m_Field0A0;
		int m_Field0A4;
		int m_Field0A8;
		int m_Field0AC;
		int m_Field0B0;
		int m_Field0B4;
		int m_Field0B8;
		int m_Field0BC;
		int m_Field0C0;
		int m_Field0C4;
		int m_Field0C8;
		int m_Field0CC;
		int m_Field0D0;
		int m_Field0D4;
		int m_Field0D8;
		int m_Field0DC;
		int m_Field0E0;
		int m_Field0E4;
		int m_Field0E8;
		int m_Field0EC;
		int m_IsPaused;
		int m_ModeID;
		int m_Field0F8;
		int m_Field0FC;
		int m_Field100;
		int m_Field104;
		int m_Field108;
		int m_Field10C;
		int m_Field110;
		int m_Field114;
		int m_Field118;
		int m_Field11C;
		int m_Field120;
		int m_Field124;
		int m_Field128;
		int m_Field12C;
		int m_Field130;
		int m_Field134;
		int m_Field138;
		int m_Field13C;
		int m_Field140;
		int m_Field144;
		int m_Field148;
		int m_Field14C;
		int m_Field150;
		int m_Field154;
		int m_Field158;
		int m_Field15C;
		int m_Field160;
		int m_Field164;
		int m_Field168;
		int m_Field16C;
		int m_Field170;
		int m_Field174;
		int m_Field178;
		int m_Field17C;
		int m_Field180;
		int m_Field184;
		int m_Field188;
		int m_Field18C;
		boost::shared_ptr<void> m_spStageLoaderXML;
		int m_Field198;
		int m_Field19C;
		int m_pGameActParameter;
		int m_Field1A4;
		int m_Field1A8;
		int m_Field1AC;
		int m_Field1B0;
		int m_Field1B4;
		int m_Field1B8;
		int m_Field1BC;
	};

}

namespace Sonic::Message
{
	class MsgStartSetEditor : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167EEDC);
	};

	class MsgStartInstanceBrush : public Hedgehog::Universe::MessageTypeSet
	{
	public:
		HH_FND_MSG_MAKE_TYPE(0x0167E8D8);
	};
}


namespace SetEditor
{
	// - Types
	//////////////////////

	struct SSetObjectCreationInfo
	{
		Eigen::Vector3f m_Position;
		float m_UnknownFloat; // Always 2.0????
		Hedgehog::Math::CQuaternion m_Rotation;
		Hedgehog::Base::CSharedString m_Name;
		int m_Field024;
		int m_MultiSetParam;
		Sonic::CEditParam* m_EditParam;
		float m_Range;
		int m_Field034;
		int m_Field038;
		int m_LayerID;
		int m_Field040;
		int m_Field044;
		int m_Field048;
		int m_Field04C;
	};

	struct SetMap
	{
		INSERT_PADDING(0x0C) {};
		uint32_t m_SetObjectID;
		boost::shared_ptr<SSetObjectCreationInfo> m_spSetObjectCreationInfo;
	};

	HOOK(void, __fastcall, _SetObjectManagerUpdate, 0x00EB3260, Sonic::CGameObject* This, void*, const hh::fnd::SUpdateInfo& updateInfo)
	{
		DebugDrawText::log("SetObject Manager Running", 0, 0, { 1,1,0.6,1 });

		//if (GetAsyncKeyState('I') & 1)
		if (false)
		{
			//FUNCTION_PTR(void, __thiscall, StartSetEditor, 0x00EB0B30, Sonic::CGameObject* _This, void* msg);
			//StartSetEditor(This, nullptr);

			//boost::shared_ptr<Sonic::Message::MsgStartSetEditor>
			//msg = boost::make_shared<Sonic::Message::MsgStartSetEditor>();

			This->SendMessage(This->m_ActorID, boost::make_shared<Sonic::Message::MsgStartSetEditor>(), 1.0f);
		}

		if (GetAsyncKeyState('Y') & 1)
		{
			//bool success = This->SendMessageImm(This, Sonic::Message::MsgSetObjectSave("work\\EatMyShorts.xml"));

			Sonic::Message::MsgSaveSetLayer msg(1);
			This->SendMessageImm(This, msg);

			if (msg.m_Success)
				DebugDrawText::log("Save succeeded! Check the work folder to see the result.",   10.0f, 0, {0.2, 1, 0.05, 1});
			else
				DebugDrawText::log("Save failure! Please check what caused this via debugging.", 10.0f, 0, {1, 0.2, 0.05, 1});
		}

		original_SetObjectManagerUpdate(This, nullptr, updateInfo);
	}

	// Set editor helper stuff
	Sonic::CRigidBody* QueryWorldHit(Sonic::CRayCastCollision* in_pRaycastCollider, float depth = 1.0f)
	{
		using namespace hh::math;

		const float WIDTH  = (float)*(uint32_t*)0x1DFDDDC;
		const float HEIGHT = (float)*(uint32_t*)0x1DFDDE0;

		POINT mousePos{};
		GetCursorPos(&mousePos);
		ScreenToClient(GetForegroundWindow(), &mousePos);

		Sonic::CCamera* camera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera().get();

		CVector2 screenCoord((float)mousePos.x / WIDTH, 1.0f - (float)mousePos.y / HEIGHT);

		CVector  ndc(screenCoord.x(), screenCoord.y(), 0.5f);
		ndc -= CVector(0.5f, 0.5f, 0.5f);
		ndc *= 2.0f;

		const CVector4 ViewCoordinates = camera->m_MyCamera.m_Projection.inverse() * CVector4(ndc.x(), ndc.y(), ndc.z(), 1.0f);
		const CVector4 PerspCoordinate = camera->m_MyCamera.m_View.inverse() * ViewCoordinates;

		const CVector worldPosition = CVector(PerspCoordinate.x(), PerspCoordinate.y(), PerspCoordinate.z()) / PerspCoordinate.w();
		const CVector direction = (worldPosition - camera->m_Position).normalized();
		const CVector rayEnd = direction * depth + camera->m_Position;

		Sonic::SCollisionHitPointInfo hitInfo;

		if (!in_pRaycastCollider->CastRay(*(uint32_t*)0x01E0AFB4, camera->m_Position, rayEnd, &hitInfo))
			return nullptr;

		return (Sonic::CRigidBody*)hitInfo.pPhysicsUnit;
	}

	HOOK(void, __fastcall, _GameplayFlowUpdate,     0x00D057F0, Sonic::CGameplayFlowStageAct* This)
	{
//#define _DEBUGSET
#ifdef _DEBUGSET
		//DebugDrawText::log(format("Active frame: %d", This->m_pStateMachine->m_UpdateInfo.Frame));
		FUNCTION_PTR(void, __thiscall, StartInstanceBrush,  0x00D07610, void* __This, void*);
		FUNCTION_PTR(void, __thiscall, FinishInstanceBrush, 0x00D04350, void* __This, void*);

		static bool forceOn = false;
		if (GetAsyncKeyState('M') & 1)
			forceOn = true;

		DebugDrawText::log(format("Is Paused: %d", This->m_IsPaused));

		if (forceOn)
		{
			This->m_ModeID = 5;
			This->m_IsPaused = 5;
		}

		DebugDrawText::log(format("ModeID: %d", This->m_ModeID));

		if (GetAsyncKeyState('I') & 1)
		{
			if (This->m_ModeID == 3)
				FinishInstanceBrush(This, nullptr);
			else
				StartInstanceBrush(This, nullptr);
		}
#endif



		original_GameplayFlowUpdate(This);

		//if (This->m_ModeID != This->m_IsPaused)
		//	DebugDrawText::log(format("CONFLICT 02: IS %d -- WAS %d", This->m_ModeID, This->m_IsPaused), 10.0f);
	}

	//void __fastcall GameplayFlowForcedUpdate(Hedgehog::Universe::CStateMachineBase::CStateBase* This)
	//{
	//	DebugDrawText::log("Flow Updating");
	//}
	

	// - Patch
	//////////////////////


	void __stdcall WriteNode(xercesc_2_8::DOMElementNSImpl** inNode, void* outNode, SetMap* setMap)
	{
		using namespace xercesc_2_8;

		//auto* pMap = (hh::map<uint32_t, boost::shared_ptr<SSetObjectCreationInfo>>*)setMap;
		//auto* whatever = &pMap->at(100);
		//DebugDrawText::log(format("%X", whatever), 10.0f);

		SSetObjectCreationInfo* setInfo = setMap->m_spSetObjectCreationInfo.get();
		DOMElementNSImpl* parentNode = *inNode;
		DOMDocumentImpl* document = parentNode->getOwnerDocument();

		DOMElement* currentElement = document->createElement(setInfo->m_Name);

		auto* paramList = &setInfo->m_EditParam->m_ParamList;
		Sonic::CXMLTypeSLTxt xmlTxt;

		hh::math::CVector     position(setInfo->m_Position);
		xmlTxt.AddVector("Position", &position);
		xmlTxt.AddRotation("Rotation", &setInfo->m_Rotation);
		xmlTxt.AddInt("SetObjectID", &setMap->m_SetObjectID);

		for (int i = 0; i < paramList->size(); ++i)
		{
			auto* param = paramList->at(i);
			param->WriteParametersToXML(&xmlTxt);
		}

		xmlTxt.ApplyParamsToElement(currentElement);

		// Commit to SetObject group
		parentNode->appendChild(currentElement);
	}

	static constexpr uint32_t writeNodeJmp   = 0x00EB4422;
	static constexpr uint32_t addrFindString = 0x00661590;
	ASMHOOK WriteNodeASM()
	{
		__asm
		{
			push ecx
			add ecx, 0x20
			call addrFindString
			pop ecx

			test eax, eax
			jz Jump


			//push ecx // Sonic::SSetObjetCreationInfo *

			mov ecx, [esp + 0x2C - 0x14] // Get map element directly so we access set object ID.
			push ecx

			lea ecx, [esp + 0x20]
			push ecx // xercesc_2_8::DOMElementNSImpl * -- OUT
			lea edx, [esp + 0x1C]
			push edx // xercesc_2_8::DOMElementNSImpl * -- IN

			call WriteNode

			inc ebx

			Jump:
			jmp writeNodeJmp
		}
	}


	// Debug text
	// ///////////

	// Restores debug text that sonic team broke
	HOOK(void, __cdecl, DebugDrawTextDraw,  0x750820, void*, float x, float y, void*, uint8_t* in_color, wchar_t* text, ...)
	{
		va_list va;
		va_start(va, text);

		wchar_t formatted[1024];
		_vsnwprintf_s(formatted, _countof(formatted), text, va);

		char formattedMultiByte[1024];
		WideCharToMultiByte(CP_UTF8, 0, formatted, -1, formattedMultiByte, sizeof(formattedMultiByte), 0, 0);

		const size_t WIDTH  = *(size_t*)0x1DFDDDC;
		const size_t HEIGHT = *(size_t*)0x1DFDDE0;

		static float textScale = (float)WIDTH / 1280.0f;


		DebugDrawText::Color color = 
		{
			(float)in_color[0] / 255.0f,
			(float)in_color[1] / 255.0f,
			(float)in_color[2] / 255.0f,
			1};

		//DebugDrawText::draw(formattedMultiByte,
		//	{
		//		(size_t)(x / *(size_t*)0x180C6B0 * WIDTH)  % WIDTH,
		//		(size_t)(y / *(size_t*)0x1B24560 * HEIGHT) % HEIGHT
		//	},
		//	//textScale,
		//	1.5f,
		//	color);

		//DebugDrawText::log(format("X Value FLT: %f", x));
		//DebugDrawText::log(format("X Value INT: %d", (size_t)(x / *(size_t*)0x180C6B0 * WIDTH) % WIDTH));

		DebugDrawText::draw(formattedMultiByte,
			{
				(size_t)(x / *(size_t*)0x180C6B0 * WIDTH)  % WIDTH,
				(size_t)(y / *(size_t*)0x1B24560 * HEIGHT) % HEIGHT
			},
			//textScale,
			1.0f,
			color);
	}

	HOOK(void, __cdecl, DebugDrawTextDraw2, 0x750670, void*, float x, float y, void*, uint8_t* in_color, void*, wchar_t* text, ...)
	{
		va_list va;
		va_start(va, text);

		wchar_t formatted[1024];
		_vsnwprintf_s(formatted, _countof(formatted), text, va);

		char formattedMultiByte[1024];
		WideCharToMultiByte(CP_UTF8, 0, formatted, -1, formattedMultiByte, sizeof(formattedMultiByte), 0, 0);

		const size_t WIDTH  = *(size_t*)0x1DFDDDC;
		const size_t HEIGHT = *(size_t*)0x1DFDDE0;

		static float textScale = (float)WIDTH / 1280.0f;

		DebugDrawText::log("TEXT_V_2", 10.0f, 0, { 1,0,0,1 });

		DebugDrawText::Color color = 
		{
			(float)in_color[0] / 255.0f,
			(float)in_color[1] / 255.0f,
			(float)in_color[2] / 255.0f,
			1};

		DebugDrawText::draw(formattedMultiByte,
			{
				(size_t)(x / *(size_t*)0x180C6B0 * WIDTH)  % WIDTH,
				(size_t)(y / *(size_t*)0x1B24560 * HEIGHT) % HEIGHT
			},
			//textScale,
			1.0f,
			color);
	}
}


void SetEditor::Init()
{
	INSTALL_HOOK(_SetObjectManagerUpdate)
	INSTALL_HOOK(_GameplayFlowUpdate)

	// Reimplement debug text
	INSTALL_HOOK(DebugDrawTextDraw)
	INSTALL_HOOK(DebugDrawTextDraw2)

	//WRITE_MEMORY(0x016E4190, void*, GameplayFlowForcedUpdate)
	WRITE_JUMP(0x00EB4405, WriteNodeASM)
}

// Thank you nextin.
void StageSelectTests()
{
	static bool done = false;
	if (done)
		return;

	//if (!Sonic::CInputState::GetInstance()->GetPadState().IsTapped(Sonic::eKeyState_Y))
	//	return;

	if (!(GetAsyncKeyState('B') & 1))
		return;

	Sonic::CGameDocument::GetInstance()->AddGameObject(boost::make_shared<Sonic::StageSelectMenu::CDebugStageSelectMenuXml>());
	done = true;
}

void SetEditor::OnFrame()
{
	//StageSelectTests();
}
#endif
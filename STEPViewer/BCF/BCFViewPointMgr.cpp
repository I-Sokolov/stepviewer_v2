#include "stdafx.h"
#include "BCFViewPointMgr.h"
#include "STEPViewerDoc.h"
#include "STEPViewerView.h"
#include "BCFTopicView.h"
#include "_ifc_instance.h"
#include "_ifc_model.h"
#include "_oglUtils.h"

#include <unordered_set>

#define IFC_ROOT "IfcRoot"
#define IFC_SPACE "IfcSpace"
#define IFC_OPENING "IfcOpeningElement"
#define GLOBAL_ID "GlobalId"

void CBCFViewPointMgr::SetViewFromComment(BCFComment& comment)
{
	if (auto commentViewPoint = comment.GetViewPoint()) {
		//set camera
		BCFCamera camera = commentViewPoint->GetCameraType();
		BCFPoint viewPoint;
		BCFPoint direction;
		BCFPoint upVector;
		commentViewPoint->GetCameraViewPoint(viewPoint);
		commentViewPoint->GetCameraDirection(direction);
		commentViewPoint->GetCameraUpVector(upVector);
		double viewToWorldScale = commentViewPoint->GetViewToWorldScale();
		double fieldOfView = commentViewPoint->GetFieldOfView();
		double aspectRatio = commentViewPoint->GetAspectRatio();
		m_view.GetViewerView()->SetBCFView(camera, viewPoint, direction, upVector, viewToWorldScale, fieldOfView, aspectRatio);
		
		ApplySelectionToViewer(commentViewPoint);
		ApplyColoringToViewer(commentViewPoint);
		ApplyVisibilityToViewer(commentViewPoint);
	}
	else {
		m_view.GetViewerView()->ResetBCFView();

		ApplySelectionToViewer(nullptr);
		ApplyColoringToViewer(nullptr);
		ApplyVisibilityToViewer(nullptr);
	}
}

bool CBCFViewPointMgr::SaveCurrentViewToComent(BCFComment&comment)
{
	bool ok = true;

	auto vp = comment.GetViewPoint();
	if (!vp) {
		vp = comment.GetTopic().AddViewPoint();
		if (vp) {
			ok = comment.SetViewPoint(vp) && ok;
		}
		else {
			ok = false;
		}
	}

	if (vp) {

		if (auto view = m_view.GetViewerView()) {

			//save camera
			BCFCamera camera = BCFCameraOrthogonal;
			BCFPoint viewPoint;
			BCFPoint direction;
			BCFPoint upVector;
			double viewToWorldScale = 1;
			double fieldOfView = 90;
			double aspectRatio = 1;

			view->GetBCFView(camera, viewPoint, direction, upVector, viewToWorldScale, fieldOfView, aspectRatio);

			ok = vp->SetCameraType(camera) && ok;
			ok = vp->SetCameraViewPoint(&viewPoint) && ok;
			ok = vp->SetCameraDirection(&direction) && ok;
			ok = vp->SetCameraUpVector(&upVector) && ok;
			ok = vp->SetViewToWorldScale(viewToWorldScale) && ok;
			ok = vp->SetFieldOfView(fieldOfView) && ok;
			ok = vp->SetAspectRatio(aspectRatio) && ok;
		}

		ok = SaveSelection(*vp) && ok;
		ok = SaveColoring(*vp) && ok;
		ok = SaveVisibility(*vp) && ok;
	}

	return ok;

}

void CBCFViewPointMgr::ApplySelectionToViewer(BCFViewPoint* vp)
{
	vector<_instance*> vecInstances;

	if (vp != nullptr) {
		int i = 0;
		while (auto comp = vp->GetSelection(i++)) {
			if (comp) {
				if (auto inst = SearchComponent(*comp)) {
					vecInstances.push_back(inst);
				}
			}
		}
	}

	m_view.GetViewerDoc().selectInstances(nullptr, vecInstances);
}

_instance* CBCFViewPointMgr::SearchComponent(BCFComponent& comp)
{
	auto ifcGuid = comp.GetIfcGuid();
	if (ifcGuid && *ifcGuid) {
		if (auto inst = SearchIfcComponent(ifcGuid)) {
			return inst;
		}
	}

	auto appId = comp.GetAuthoringToolId();
	if (appId && *appId) {
		//...
	}

	return NULL;
}

_instance* CBCFViewPointMgr::SearchIfcComponent(const char* ifcGuid)
{
	for (auto model : m_view.GetViewerDoc().getModels()) {
		if (auto ifcModel = dynamic_cast<_ifc_model*>(model)) {
			if (auto sdaiModel = ifcModel->getSdaiModel()) {
				if (auto rootEntity = sdaiGetEntity(sdaiModel, IFC_ROOT)) {
					if (auto globalIdAttr = sdaiGetAttrDefinition(rootEntity, GLOBAL_ID)) {

						for (auto inst : ifcModel->getInstances()) {
							if (auto apInst = dynamic_cast<_ap_instance*>(inst)) {
								if (auto sdaiInst = apInst->getSdaiInstance()) {

									const char* globalId = NULL;
									if (sdaiGetAttr(sdaiInst, globalIdAttr, sdaiSTRING, &globalId) && globalId) {

										if (0 == strcmp(ifcGuid, globalId)) {
											return inst;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return NULL;
}

bool CBCFViewPointMgr::SaveSelection(BCFViewPoint& vp)
{
	bool ok = true;

	int i = 0;
	while (auto sel = vp.GetSelection(i)) {
		if (!sel->Remove()) {
			ok = false;
			i++;
		}
	}

	for (auto inst : m_view.GetViewerDoc().getSelectedInstances()) {
		if (auto globalId = GetGlobalId(inst)) {
			ok = vp.AddSelection(globalId) && ok;
		}
	}
	
	return ok;
}

const char* CBCFViewPointMgr::GetGlobalId(_instance* inst)
{
	if (auto ifcInst = dynamic_cast<_ifc_instance*>(inst)) {
		if (auto sdaiInst = ifcInst->getSdaiInstance()) {
			const char* globalId = NULL;
			if (sdaiGetAttrBN(sdaiInst, GLOBAL_ID, sdaiSTRING, &globalId) && globalId && *globalId) {
				return globalId;
			}
		}
	}
	return NULL;
}

void CBCFViewPointMgr::ApplyColoringToViewer(BCFViewPoint* vp)
{
	auto pOGLView = m_view.GetViewerDoc().getViewAs<_oglView>();
	if (pOGLView == nullptr)
	{
		ASSERT(FALSE);

		return;
	}

	pOGLView->removeUserDefinedMaterials();

	if (vp != nullptr) {
		int i = 0;
		while (auto bcfcoloring = vp->GetColoring(i++)) {
			if (bcfcoloring) {
				if (auto strcolor = bcfcoloring->GetColor()) {
					auto clrref = GetColorRef(strcolor);

					int j = 0;
					vector<_instance*> vecInstances;
					while (auto comp = bcfcoloring->GetComponent(j++)) {
						if (auto inst = SearchComponent(*comp)) {
							vecInstances.push_back(inst);
						}
					}
					pOGLView->addUserDefinedMaterial(vecInstances, GetRValue(clrref), GetGValue(clrref), GetBValue(clrref));
				}
			}
		}
	}
}


bool CBCFViewPointMgr::SaveColoring(BCFViewPoint& vp)
{
	auto pOGLView = m_view.GetViewerDoc().getViewAs<_oglView>();
	if (pOGLView == nullptr)
	{
		ASSERT(FALSE);

		return false;
	}

	map<COLORREF, list<_instance*>> mapColoring;
	for (auto itUserDefinedMaterial : pOGLView->getUserDefinedMaterials())
	{
		COLORREF color = RGB(
			itUserDefinedMaterial.second->getDiffuseColor().r(), 
			itUserDefinedMaterial.second->getDiffuseColor().g(), 
			itUserDefinedMaterial.second->getDiffuseColor().b());

		auto itColoring = mapColoring.find(color);
		if (itColoring != mapColoring.end())
		{
			itColoring->second.push_back(itUserDefinedMaterial.first);
		}
		else
		{
			mapColoring[color] = list<_instance*>{ itUserDefinedMaterial.first };
		}
	}

	bool ok = true;

	int i = 0;
	while (auto bcfcoloring = vp.GetColoring(i)) {
		if (!bcfcoloring->Remove()) {
			ok = false;
			i++;
		}
	}

	for (auto& coloring : mapColoring) {

		auto clr = GetColorStr(coloring.first);
		
		if (auto bcfcoloring = vp.AddColoring(clr.c_str())) {

			for (auto inst : coloring.second) {
				if (auto globalId = GetGlobalId(inst)) {
					ok = bcfcoloring->AddComponent(globalId) && ok;
				}
			}
		}
		else {
			ok = false;
		}
	}

	return ok;
}

static BYTE GetByte(const char* str)
{
	BYTE v = 0;

	for (int i = 0; i < 2; i++) {
		v *= 16;

		if (str[i] >= '0' && str[i] <= '9') {
			v += str[i] - '0';
		}
		else if (str[i] >= 'A' && str[i] <= 'F') {
			v += str[i] - 'A' + 10;
		}
		else if (str[i] >= 'a' && str[i] <= 'f') {
			v += str[i] - 'a' + 10;
		}
	}

	return v;
}


COLORREF CBCFViewPointMgr::GetColorRef(const char* strcolor)
{
	COLORREF clr = 0;

	auto len = strlen(strcolor);
	
	int i = 0;

	if (len == 8) {
		BYTE b = GetByte(strcolor);
		clr = b << 24;
		i += 2;
	}

	BYTE shift = 0;
	while (i + 1 < (int)len) {
		BYTE b = GetByte(strcolor + i);

		clr |= (COLORREF)b << shift;

		i += 2;
		shift += 8;
	}

	return clr;
}

static COLORREF GetComponent(COLORREF clr, BYTE shift, char buff[3])
{
	auto comp = (clr >> shift) & 0xFF;
	sprintf_s(buff, 3, "%02X", comp);
	return comp;
}

std::string CBCFViewPointMgr::GetColorStr(COLORREF clr)
{
	char str[9];

	int i = 0;
	if (GetComponent(clr, 24, str)) {
		i += 2;
	}
	
	for (BYTE shift = 0; shift <= 16; shift+=8) {
		GetComponent(clr, shift, str + i);
		i += 2;
	}

	str[i] = 0;

	return str;
}


void CBCFViewPointMgr::ApplyVisibilityToViewer(BCFViewPoint* vp)
{
	if (vp != nullptr) {
		std::unordered_set<_instance*> exceptions;
		int i = 0;
		while (auto comp = vp->GetException(i++)) {
			if (auto inst = SearchComponent(*comp)) {
				exceptions.insert(inst);
			}
		}

		for (auto model : m_view.GetViewerDoc().getModels()) {
			if (model) {

				SdaiEntity space = NULL;
				SdaiEntity opening = NULL;
				if (auto ifcModel = dynamic_cast<_ifc_model*>(model)) {
					if (auto sdaiModel = ifcModel->getSdaiModel()) {
						space = sdaiGetEntity(sdaiModel, IFC_SPACE);
						opening = sdaiGetEntity(sdaiModel, IFC_OPENING);
					}
				}

				for (auto inst : model->getInstances()) {

					bool visible = vp->GetDefaultVisibility();

					if (auto ifcInst = dynamic_cast<_ifc_instance*>(inst)) {
						if (auto sdaiInst = ifcInst->getSdaiInstance()) {
							if (sdaiIsInstanceOf(sdaiInst, space)) {
								visible = vp->GetSpaceVisible();
							}
							else if (sdaiIsInstanceOf(sdaiInst, opening)) {
								visible = vp->GetOpeningsVisible();
							}
							else if (IsSpaceBoundary(sdaiInst)) {
								visible = vp->GetSpaceBoundariesVisible();
							}
						}
					}

					if (exceptions.find(inst) != exceptions.end()) {
						visible = !visible;
					}

					inst->setEnable(visible);
				}
			}
		}

		m_view.GetViewerDoc().onInstancesEnabledStateChanged(nullptr);
	}
	else {
		m_view.GetViewerDoc().resetInstancesEnabledState(nullptr);
	}
}

bool CBCFViewPointMgr::SaveVisibility(BCFViewPoint& vp)
{
	bool ok = true;

	int allInstances[2] = { 0,0 };
	int spaces[2] = { 0,0 };
	int boundaries[2] = { 0, 0 };
	int openings[2] = { 0, 0 };

	// calculate visible/invisible instances
	//
	for (auto model : m_view.GetViewerDoc().getModels()) {
		if (model) {

			SdaiEntity space = NULL;
			SdaiEntity opening = NULL;
			if (auto ifcModel = dynamic_cast<_ifc_model*>(model)) {
				if (auto sdaiModel = ifcModel->getSdaiModel()) {
					space = sdaiGetEntity(sdaiModel, IFC_SPACE);
					opening = sdaiGetEntity(sdaiModel, IFC_OPENING);
				}
			}

			for (auto inst : model->getInstances()) {

				int ind = inst->getEnable() ? 0 : 1;

				allInstances[ind]++;

				if (auto ifcInst = dynamic_cast<_ifc_instance*>(inst)) {
					if (auto sdaiInst = ifcInst->getSdaiInstance()) {
						if (sdaiIsInstanceOf(sdaiInst, space)) {
							spaces[ind]++;
						}
						else if (sdaiIsInstanceOf(sdaiInst, opening)) {
							openings[ind]++;
						}
						else if (IsSpaceBoundary(sdaiInst)) {
							boundaries[ind]++;
						}
					}
				}
			}
		}
	}

	// set default visibilities
	//
	ok = vp.SetDefaultVisibility(allInstances[0] >= allInstances[1]) && ok;
	ok = vp.SetSpaceVisible(spaces[0] > spaces[1]) && ok;
	ok = vp.SetSpaceBoundariesVisible(boundaries[0] > boundaries[1]) && ok;
	ok = vp.SetOpeningsVisible(openings[0] > openings[1]) && ok;

	//set exceptions
	//
	int i = 0;
	while (auto exception = vp.GetException(i)) {
		if (!exception->Remove()) {
			ok = false;
			i++;
		}
	}

	for (auto model : m_view.GetViewerDoc().getModels()) {
		if (model) {
			SdaiEntity space = NULL;
			SdaiEntity opening = NULL;
			SdaiAttr attrGlobalId = NULL;
			if (auto ifcModel = dynamic_cast<_ifc_model*>(model)) {
				if (auto sdaiModel = ifcModel->getSdaiModel()) {
					space = sdaiGetEntity(sdaiModel, IFC_SPACE);
					opening = sdaiGetEntity(sdaiModel, IFC_OPENING);

					if (auto root = sdaiGetEntity(sdaiModel, IFC_ROOT))
						attrGlobalId = sdaiGetAttrDefinition(root, GLOBAL_ID);
				}
			}

			for (auto inst : model->getInstances()) {

				bool visible = vp.GetDefaultVisibility();

				if (auto ifcInst = dynamic_cast<_ifc_instance*>(inst)) {
					if (auto sdaiInst = ifcInst->getSdaiInstance()) {
						if (sdaiIsInstanceOf(sdaiInst, space)) {
							visible = vp.GetSpaceVisible();
						}
						else if (sdaiIsInstanceOf(sdaiInst, opening)) {
							visible = vp.GetOpeningsVisible();
						}
						else if (IsSpaceBoundary(sdaiInst)) {
							visible = vp.GetSpaceBoundariesVisible();
						}

						if (visible != inst->getEnable()) {
							const char* globalId = NULL;
							if (sdaiGetAttr(sdaiInst, attrGlobalId, sdaiSTRING, &globalId) && globalId && *globalId)
								ok = vp.AddException(globalId) && ok;
						}
					}
				}
			}
		}
	}

	return ok;
}

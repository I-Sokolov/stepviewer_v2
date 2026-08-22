#pragma once

#include "bcfAPI.h"
#include <string>

class CBCFTopicView;
class _instance;

class CBCFViewPointMgr
{
public:
	CBCFViewPointMgr(CBCFTopicView& view) : m_view(view){}

	void SetViewFromComment(BCFComment& comment);

	bool SaveCurrentViewToComent(BCFComment& comment);

private:
	void ApplySelectionToViewer(BCFViewPoint* vp);
	bool SaveSelection(BCFViewPoint& vp);

	void ApplyColoringToViewer(BCFViewPoint* vp);
	bool SaveColoring(BCFViewPoint& vp);

	void ApplyVisibilityToViewer(BCFViewPoint* vp);
	bool SaveVisibility(BCFViewPoint& vp);

	_instance* SearchComponent(BCFComponent& comp);
	_instance* SearchIfcComponent(const char* ifcGuid);

	bool IsSpaceBoundary(SdaiInstance /*inst*/) { return false; /*TODO*/ }

	const char* GetGlobalId(_instance* inst);

	COLORREF GetColorRef(const char* strcolor);
	std::string GetColorStr(COLORREF clr);

private:
	CBCFTopicView& m_view;
};

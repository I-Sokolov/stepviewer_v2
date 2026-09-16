#include "stdafx.h"

#include "BCFCommentForm.h"
#include "BCFView.h"
#include "BCFViewPointMgr.h"

namespace
{
	CString FormatPoint(const BCFPoint& point)
	{
		CString value;
		value.Format(L"%.5f, %.5f, %.5f", point.xyz[0], point.xyz[1], point.xyz[2]);
		return value;
	}

	CString FormatDouble(double number)
	{
		CString value;
		value.Format(L"%.5f", number);
		return value;
	}

}

BEGIN_MESSAGE_MAP(CBCFCommentForm, CWnd)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PANE_COMMENT_VIEW_TOPIC, &CBCFCommentForm::OnViewTopic)
	ON_NOTIFY(TCN_SELCHANGE, IDC_PANE_COMMENT_TABS, &CBCFCommentForm::OnTabChanged)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SNAPSHOT_SELECT, &CBCFCommentForm::OnSelectSnapshot)
	ON_BN_CLICKED(IDC_PANE_COMMENT_GET_FROM_VIEW, &CBCFCommentForm::OnGetFromView)
	ON_BN_CLICKED(IDC_PANE_COMMENT_APPLY_VIEW, &CBCFCommentForm::OnApplyView)
	ON_BN_CLICKED(IDC_PANE_COMMENT_GRAB_SELECTED, &CBCFCommentForm::OnGrabSelected)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SELECT_COMPONENTS, &CBCFCommentForm::OnSelectComponents)
	ON_BN_CLICKED(IDC_PANE_COMMENT_GRAB_VISIBLE, &CBCFCommentForm::OnGrabVisible)
	ON_BN_CLICKED(IDC_PANE_COMMENT_VISIBLE_FROM_SELECTION, &CBCFCommentForm::OnVisibleFromSelection)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SET_VISIBLE, &CBCFCommentForm::OnSetVisible)
END_MESSAGE_MAP()

BOOL CBCFCommentForm::Create(CBCFView* pane)
{
	m_pane = pane;
	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}
	m_viewTopic.Create(L"Back to topic details", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_VIEW_TOPIC);
	SetBCFControlFont(m_viewTopic, this);
	CreateBCFStaticLabel(m_headerInfo, L"Comment to topic", this);
	m_tabs.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS,
		CRect(), this, IDC_PANE_COMMENT_TABS);
	SetBCFControlFont(m_tabs, this);
	m_tabs.InsertItem(0, L"General");
	m_tabs.InsertItem(1, L"Visualization");
	CreateBCFStaticLabel(m_textLabel, L"Comment:", this);
	m_text.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_WANTRETURN | WS_VSCROLL, CRect(), this, IDC_PANE_COMMENT_TEXT);
	SetBCFControlFont(m_text, this);
	m_snapshot.Create(L"", WS_CHILD,
		CRect(), this);
	m_selectSnapshot.Create(L"Select file...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SNAPSHOT_SELECT);
	SetBCFControlFont(m_selectSnapshot, this);
	m_cameraGroup.Create(L"Camera", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	SetBCFControlFont(m_cameraGroup, this);
	m_cameraDetails.Create(WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_READONLY | WS_VSCROLL, CRect(), this, 0);
	m_applyView.Create(L"Apply", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_APPLY_VIEW);
	m_getFromView.Create(L"Get From View", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_GET_FROM_VIEW);
	SetBCFControlFont(m_cameraDetails, this);
	SetBCFControlFont(m_applyView, this);
	SetBCFControlFont(m_getFromView, this);
	m_selectionGroup.Create(L"Selection", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	m_grabSelected.Create(L"Grab selected", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_GRAB_SELECTED);
	m_selectComponents.Create(L"Apply", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SELECT_COMPONENTS);
	m_selectedComponents.Create(WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_READONLY | WS_VSCROLL, CRect(), this, 0);
	m_visibilityGroup.Create(L"Visibility", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	m_grabVisible.Create(L"Grab visible", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_GRAB_VISIBLE);
	m_visibleFromSelection.Create(L"Grab selected", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_VISIBLE_FROM_SELECTION);
	m_setVisible.Create(L"Apply", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SET_VISIBLE);
	m_visibilityMode.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		CRect(), this, IDC_PANE_COMMENT_VISIBILITY_MODE);
	m_visibilityMode.AddString(L"Show all but exceptions");
	m_visibilityMode.AddString(L"Hide all but exceptions");
	CreateBCFStaticLabel(m_showLabel, L"Show", this);
	m_showSpaces.Create(L"Spaces", WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, CRect(), this, 0);
	m_showBoundaries.Create(L"Boundaries", WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, CRect(), this, 0);
	m_showOpenings.Create(L"Openings", WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, CRect(), this, 0);
	m_visibilityExceptions.Create(WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_READONLY | WS_VSCROLL, CRect(), this, 0);
	m_coloringGroup.Create(L"Coloring", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	m_coloringDetails.Create(WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_READONLY | WS_VSCROLL, CRect(), this, 0);
	CWnd* visualizationControls[] = {
		&m_selectionGroup, &m_grabSelected, &m_selectComponents,
		&m_selectedComponents, &m_visibilityGroup, &m_grabVisible, &m_visibleFromSelection, &m_setVisible,
		&m_visibilityMode, &m_showLabel, &m_showSpaces, &m_showBoundaries,
		&m_showOpenings, &m_visibilityExceptions, &m_coloringGroup, &m_coloringDetails
	};
	for (CWnd* control : visualizationControls) {
		SetBCFControlFont(*control, this);
	}
	m_tabs.SetCurSel(0);
	ShowTab(0);
	return TRUE;
}

void CBCFCommentForm::Load(BCFComment* comment)
{
	m_comment = comment;
	if (!IsWindow(GetSafeHwnd())) {
		return;
	}
	UpdateHeader();
	m_text.SetWindowText(comment ? FromUTF8(comment->GetText()) : CString());
	LoadViewPoint();
	if (IsWindow (GetSafeHwnd ())){
		ReloadSelection ();
		ReloadVisibility ();
		ReloadColoring ();
		}
}

void CBCFCommentForm::UpdateHeader()
{
	if (!m_comment) {
		m_headerInfo.SetWindowText(CString());
		return;
	}

	CString created = FormatBCFCommentCreated(*m_comment);
	created.SetAt(0, L'c');
	CString modified = FormatBCFCommentModified(*m_comment);
	if (!modified.IsEmpty()) {
		modified.SetAt(0, L'm');
	}

	CString header;
	header.Format(L"Comment to topic '%s' %s", FromUTF8(m_comment->GetTopic().GetTitle()).GetString(),
		created.GetString());
	if (!modified.IsEmpty()) {
		header += L", ";
		header += modified;
	}
	m_headerInfo.SetWindowText(header);
}

bool CBCFCommentForm::Commit()
{
	if (!m_comment) {
		return true;
	}
	CString text;
	m_text.GetWindowText(text);
	bool ok = m_comment->SetText(ToUTF8(text).c_str());
	ok = CommitVisibility() && ok;
	m_pane->ShowLog(!ok);
	if (ok) {
		UpdateHeader();
	}
	return ok;
}

void CBCFCommentForm::LoadViewPoint()
{
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	CString details;
	if (viewPoint) {
		BCFPoint point = {};
		viewPoint->GetCameraViewPoint(point);
		details.Format(L"Type: %s\r\nView point: %s",
			viewPoint->GetCameraType() == BCFCameraOrthogonal
				? L"Orthogonal"
				: L"Perspective",
			FormatPoint(point).GetString());
		viewPoint->GetCameraDirection(point);
		details.AppendFormat(L"\r\nDirection: %s", FormatPoint(point).GetString());
		viewPoint->GetCameraUpVector(point);
		details.AppendFormat(L"\r\nUp vector: %s", FormatPoint(point).GetString());
		details.AppendFormat(L"\r\nAspect ratio: %s",
			FormatDouble(viewPoint->GetAspectRatio()).GetString());
		details.AppendFormat(L"\r\nScale: %s",
			FormatDouble(viewPoint->GetViewToWorldScale()).GetString());
		details.AppendFormat(L"\r\nField of view: %s",
			FormatDouble(viewPoint->GetFieldOfView()).GetString());
		m_snapshot.Load(FromUTF8(viewPoint->GetSnapshot()));
	}
	else {
		m_snapshot.Clear();
	}
	m_cameraDetails.SetWindowText(details);
	m_applyView.EnableWindow(viewPoint != nullptr);
	m_getFromView.EnableWindow(m_comment != nullptr && m_pane->GetDocument() != nullptr);

	if (IsWindow(GetSafeHwnd ())) {
		CRect client;
		GetClientRect (client);
		OnSize (SIZE_RESTORED, client.Width (), client.Height ());
		if (IsWindow (GetSafeHwnd ())){
			RedrawWindow (nullptr, nullptr,
						  RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
			}
		}
}

void CBCFCommentForm::OnTabChanged(NMHDR*, LRESULT* result)
{
	ShowTab(m_tabs.GetCurSel());
	*result = 0;
}

void CBCFCommentForm::FocusInitialControl()
{
	if (m_tabs.GetCurSel() != 0) {
		m_tabs.SetCurSel(0);
		ShowTab(0);
	}
	m_text.SetFocus();
}

void CBCFCommentForm::ShowTab(int tab)
{
	const int generalCommand = tab == 0 ? SW_SHOW : SW_HIDE;
	m_textLabel.ShowWindow(generalCommand);
	m_text.ShowWindow(generalCommand);
	m_snapshot.ShowWindow(generalCommand);
	m_selectSnapshot.ShowWindow(generalCommand);
	m_cameraGroup.ShowWindow(generalCommand);
	m_cameraDetails.ShowWindow(generalCommand);
	m_getFromView.ShowWindow(generalCommand);
	m_applyView.ShowWindow(generalCommand);
	const int visualizationCommand = tab == 1 ? SW_SHOW : SW_HIDE;
	CWnd* visualizationControls[] = {
		&m_selectionGroup, &m_grabSelected, &m_selectComponents,
		&m_selectedComponents, &m_visibilityGroup, &m_grabVisible, &m_visibleFromSelection, &m_setVisible,
		&m_visibilityMode, &m_showLabel, &m_showSpaces, &m_showBoundaries,
		&m_showOpenings, &m_visibilityExceptions, &m_coloringGroup, &m_coloringDetails
	};
	for (CWnd* control : visualizationControls) {
		control->ShowWindow(visualizationCommand);
	}
	CRect client;
	GetClientRect(client);
	OnSize(SIZE_RESTORED, client.Width(), client.Height());
	RedrawWindow(nullptr, nullptr,
		RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CBCFCommentForm::OnSelectSnapshot()
{
	if (!m_comment) {
		return;
	}
	CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		L"Image files (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png||");
	if (dialog.DoModal() != IDOK) {
		return;
	}
	BCFViewPoint* viewPoint = m_comment->GetViewPoint();
	if (!viewPoint) {
		viewPoint = m_comment->GetTopic().AddViewPoint();
	}
	const bool ok = viewPoint &&
		m_comment->SetViewPoint(viewPoint) &&
		viewPoint->SetSnapshot(ToUTF8(dialog.GetPathName()).c_str());
	m_pane->ShowLog(!ok);
	if (ok) {
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnGetFromView()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CBCFViewPointMgr(*m_pane->GetDocument()).SaveCurrentViewToComent(*m_comment);
	m_pane->ShowLog(!ok);
	if (ok) {
		LoadViewPoint();
		ReloadSelection();
		ReloadVisibility();
		ReloadColoring();
	}
}

void CBCFCommentForm::OnApplyView()
{
	if (m_comment && m_comment->GetViewPoint() && m_pane->GetDocument()) {
		CBCFViewPointMgr(*m_pane->GetDocument()).SetViewFromComment(*m_comment);
		m_pane->ShowLog(false);
	}
}

void CBCFCommentForm::ReloadSelection()
{
	CString components;
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	if (viewPoint) {
		for (uint16_t i = 0; BCFComponent* component = viewPoint->GetSelection(i); ++i) {
			if (!components.IsEmpty()) {
				components += L"\r\n";
			}
			components += GetComponentText(*component);
		}
	}
	m_selectedComponents.SetWindowText(components);
	m_selectComponents.EnableWindow(viewPoint && !components.IsEmpty());
}

void CBCFCommentForm::OnGrabSelected()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CBCFViewPointMgr(*m_pane->GetDocument())
		.SaveCurrentSelectionToComment(*m_comment);
	m_pane->ShowLog(!ok);
	if (ok) {
		ReloadSelection();
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnSelectComponents()
{
	if (m_comment && m_pane->GetDocument()) {
		CBCFViewPointMgr(*m_pane->GetDocument()).SetSelectionFromComment(*m_comment);
		m_pane->ShowLog(false);
	}
}

void CBCFCommentForm::ReloadVisibility()
{
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	m_visibilityMode.SetCurSel(viewPoint && !viewPoint->GetDefaultVisibility() ? 1 : 0);
	m_showSpaces.SetCheck(viewPoint && viewPoint->GetSpaceVisible() ? BST_CHECKED : BST_UNCHECKED);
	m_showBoundaries.SetCheck(viewPoint && viewPoint->GetSpaceBoundariesVisible()
		? BST_CHECKED : BST_UNCHECKED);
	m_showOpenings.SetCheck(viewPoint && viewPoint->GetOpeningsVisible()
		? BST_CHECKED : BST_UNCHECKED);

	CString exceptions;
	if (viewPoint) {
		for (uint16_t i = 0; BCFComponent* component = viewPoint->GetException(i); ++i) {
			if (!exceptions.IsEmpty()) {
				exceptions += L"\r\n";
			}
			exceptions += GetComponentText(*component);
		}
	}
	if (!exceptions.IsEmpty()) {
		exceptions = L"Exceptions:\r\n" + exceptions;
	}
	m_visibilityExceptions.SetWindowText(exceptions);
	m_setVisible.EnableWindow(viewPoint != nullptr);
}

bool CBCFCommentForm::CommitVisibility()
{
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	if (!viewPoint) {
		return true;
	}
	bool ok = viewPoint->SetDefaultVisibility(m_visibilityMode.GetCurSel() != 1);
	ok = viewPoint->SetSpaceVisible(m_showSpaces.GetCheck() == BST_CHECKED) && ok;
	ok = viewPoint->SetSpaceBoundariesVisible(m_showBoundaries.GetCheck() == BST_CHECKED) && ok;
	ok = viewPoint->SetOpeningsVisible(m_showOpenings.GetCheck() == BST_CHECKED) && ok;
	return ok;
}

void CBCFCommentForm::OnGrabVisible()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CBCFViewPointMgr(*m_pane->GetDocument())
		.SaveCurrentVisibilityToComment(*m_comment);
	m_pane->ShowLog(!ok);
	if (ok) {
		ReloadVisibility();
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnSetVisible()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CommitVisibility();
	m_pane->ShowLog(!ok);
	if (ok) {
		CBCFViewPointMgr(*m_pane->GetDocument()).SetVisibilityFromComment(*m_comment);
		m_pane->ShowLog(false);
	}
}

void CBCFCommentForm::ReloadColoring()
{
	CString details;
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	if (viewPoint) {
		for (uint16_t i = 0; BCFColoring* coloring = viewPoint->GetColoring(i); ++i) {
			if (!details.IsEmpty()) {
				details += L"\r\n";
			}
			details += L"#";
			details += FromUTF8(coloring->GetColor());
			for (uint16_t j = 0; BCFComponent* component = coloring->GetComponent(j); ++j) {
				details += L"\r\n  ";
				details += GetComponentText(*component);
			}
		}
	}
	m_coloringDetails.SetWindowText(details);
}

CString CBCFCommentForm::GetComponentText(BCFComponent& component) const
{
	CString text;
	const char* ifcGuid = component.GetIfcGuid();
	if (ifcGuid && *ifcGuid && m_pane->GetDocument()) {
		const std::wstring displayName =
			CBCFViewPointMgr(*m_pane->GetDocument()).GetIfcComponentDisplayName(ifcGuid);
		text = displayName.c_str();
	}
	if (text.IsEmpty()) {
		text = FromUTF8(ifcGuid);
	}
	if (text.IsEmpty()) {
		text = FromUTF8(component.GetAuthoringToolId());
	}
	if (text.IsEmpty()) {
		text = FromUTF8(component.GetOriginatingSystem());
	}
	if (text.IsEmpty()) {
		text = L"(Unidentified component)";
	}
	return text;
}

void CBCFCommentForm::OnVisibleFromSelection()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CBCFViewPointMgr(*m_pane->GetDocument())
		.SaveSelectedAsVisibleToComment(*m_comment);
	m_pane->ShowLog(!ok);
	if (ok) {
		ReloadVisibility();
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnViewTopic()
{
	if (m_comment && Commit()) {
		m_pane->ShowTopic(&m_comment->GetTopic());
	}
}

void CBCFCommentForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_text.GetSafeHwnd()) {
		return;
	}

	CClientDC dc(this);
	CFont* font = m_headerInfo.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC metrics = {};
	dc.GetTextMetrics(&metrics);
	const int textHeight = metrics.tmAscent + metrics.tmDescent + metrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 5;
	const int margin = rowHeight / 3;
	const int rowSpacing = margin;
	const int labelOffset = (rowHeight - textHeight) / 2;
	CString buttonText;
	m_viewTopic.GetWindowText(buttonText);
	const int buttonWidth = static_cast<int>(dc.GetTextExtent(buttonText).cx) + 2 * margin;
	if (oldFont) {
		dc.SelectObject(oldFont);
	}

	m_viewTopic.MoveWindow(max(margin, cx - buttonWidth - margin),
		0, buttonWidth, rowHeight);
	m_headerInfo.MoveWindow(margin, labelOffset,
		max(rowHeight, cx - buttonWidth - 3 * margin), textHeight);
	const int tabsTop = rowHeight + rowSpacing;
	m_tabs.MoveWindow(margin, tabsTop, max(rowHeight, cx - 2 * margin),
		max(2 * rowHeight, cy - tabsTop - margin));
	CRect page(0, 0, max(rowHeight, cx - 2 * margin), max(2 * rowHeight, cy - tabsTop - margin));
	m_tabs.AdjustRect(FALSE, page);
	page.OffsetRect(margin, tabsTop);
	page.DeflateRect(margin, margin);

	if (m_tabs.GetCurSel() == 0) {
		CString applyText;
		CString getFromViewText;
		m_applyView.GetWindowText(applyText);
		m_getFromView.GetWindowText(getFromViewText);
		const int applyWidth =
			static_cast<int>(dc.GetTextExtent(applyText).cx) + 2 * margin;
		const int getFromViewWidth =
			static_cast<int>(dc.GetTextExtent(getFromViewText).cx) + 2 * margin;

		CString cameraText;
		m_cameraDetails.GetWindowText(cameraText);
		int cameraTextWidth = 0;
		int tokenPosition = 0;
		while (tokenPosition != -1) {
			const CString line = cameraText.Tokenize(L"\r\n", tokenPosition);
			cameraTextWidth = max(cameraTextWidth,
				static_cast<int>(dc.GetTextExtent(line).cx));
		}
		const int cameraWidth = max(
			cameraTextWidth + 2 * margin + ::GetSystemMetrics(SM_CXVSCROLL),
			applyWidth + getFromViewWidth + 3 * margin);
		const int remainingWidth = max(2 * rowHeight, page.Width() - cameraWidth - 2 * rowSpacing);
		const int firstWidth = remainingWidth / 2;
		const int secondWidth = remainingWidth - firstWidth;
		const int secondLeft = page.left + firstWidth + rowSpacing;
		const int cameraLeft = secondLeft + secondWidth + rowSpacing;

		m_textLabel.MoveWindow(page.left, page.top + labelOffset, firstWidth, textHeight);
		m_text.MoveWindow(page.left, page.top + rowHeight, firstWidth,
			max(rowHeight, page.Height() - rowHeight));

		m_snapshot.MoveWindow(secondLeft, page.top, secondWidth,
			max(rowHeight, page.Height() - rowHeight - rowSpacing));
		CString selectText;
		m_selectSnapshot.GetWindowText(selectText);
		const int selectWidth = static_cast<int>(dc.GetTextExtent(selectText).cx) + 2 * margin;
		const int snapshotButtonsTop = page.bottom - rowHeight;
		m_selectSnapshot.MoveWindow(secondLeft, snapshotButtonsTop, selectWidth, rowHeight);

		m_cameraGroup.MoveWindow(cameraLeft, page.top, cameraWidth, page.Height());
		const int cameraContentLeft = cameraLeft + margin;
		const int cameraButtonsTop = page.top + rowHeight;
		m_applyView.MoveWindow(
			cameraContentLeft, cameraButtonsTop, applyWidth, rowHeight);
		m_getFromView.MoveWindow(
			cameraLeft + cameraWidth - margin - getFromViewWidth,
			cameraButtonsTop, getFromViewWidth, rowHeight);
		const int cameraDetailsTop =
			cameraButtonsTop + rowHeight + rowSpacing;
		m_cameraDetails.MoveWindow(
			cameraContentLeft, cameraDetailsTop,
			max(rowHeight, cameraWidth - 2 * margin),
			max(rowHeight, static_cast<int>(page.bottom) -
				cameraDetailsTop - margin));
	}
	else if (m_tabs.GetCurSel() == 1) {
		const int groupWidth = max(rowHeight, (page.Width() - 2 * rowSpacing) / 3);
		const int selectionLeft = page.left;
		const int visibilityLeft = selectionLeft + groupWidth + rowSpacing;
		const int coloringLeft = visibilityLeft + groupWidth + rowSpacing;
		const int coloringWidth = max(rowHeight, static_cast<int>(page.right) - coloringLeft);
		m_selectionGroup.MoveWindow(selectionLeft, page.top, groupWidth, page.Height());
		m_visibilityGroup.MoveWindow(visibilityLeft, page.top, groupWidth, page.Height());
		m_coloringGroup.MoveWindow(coloringLeft, page.top, coloringWidth, page.Height());

		CString grabText;
		CString selectText;
		m_grabSelected.GetWindowText(grabText);
		m_selectComponents.GetWindowText(selectText);
		const int grabWidth = static_cast<int>(dc.GetTextExtent(grabText).cx) + 2 * margin;
		const int selectWidth = static_cast<int>(dc.GetTextExtent(selectText).cx) + 2 * margin;
		const int contentLeft = selectionLeft + margin;
		const int contentTop = page.top + rowHeight;
		m_selectComponents.MoveWindow(contentLeft, contentTop, selectWidth, rowHeight);
		m_grabSelected.MoveWindow(selectionLeft + groupWidth - margin - grabWidth,
			contentTop, grabWidth, rowHeight);
		const int listTop = contentTop + rowHeight + rowSpacing;
		m_selectedComponents.MoveWindow(contentLeft, listTop,
			max(rowHeight, groupWidth - 2 * margin),
			max(rowHeight, static_cast<int>(page.bottom) - listTop - margin));

		CString grabVisibleText;
		CString visibleFromSelectionText;
		CString setVisibleText;
		m_grabVisible.GetWindowText(grabVisibleText);
		m_visibleFromSelection.GetWindowText(visibleFromSelectionText);
		m_setVisible.GetWindowText(setVisibleText);
		const int grabVisibleWidth =
			static_cast<int>(dc.GetTextExtent(grabVisibleText).cx) + 2 * margin;
		const int visibleFromSelectionWidth =
			static_cast<int>(dc.GetTextExtent(visibleFromSelectionText).cx) + 2 * margin;
		const int setVisibleWidth =
			static_cast<int>(dc.GetTextExtent(setVisibleText).cx) + 2 * margin;
		const int visibilityContentLeft = visibilityLeft + margin;
		m_setVisible.MoveWindow(visibilityContentLeft, contentTop,
			setVisibleWidth, rowHeight);
		const int visibleFromSelectionLeft = visibilityLeft + groupWidth -
			margin - visibleFromSelectionWidth;
		m_visibleFromSelection.MoveWindow(visibleFromSelectionLeft, contentTop,
			visibleFromSelectionWidth, rowHeight);
		m_grabVisible.MoveWindow(visibleFromSelectionLeft - rowSpacing - grabVisibleWidth,
			contentTop, grabVisibleWidth, rowHeight);
		const int modeTop = contentTop + rowHeight + rowSpacing;
		m_visibilityMode.MoveWindow(visibilityContentLeft, modeTop,
			max(rowHeight, groupWidth - 2 * margin), 3 * rowHeight);
		const int showTop = modeTop + rowHeight + rowSpacing;
		const int showLabelWidth = static_cast<int>(dc.GetTextExtent(L"Show").cx) + margin;
		m_showLabel.MoveWindow(visibilityContentLeft, showTop + labelOffset,
			showLabelWidth, textHeight);
		int checkLeft = visibilityContentLeft + showLabelWidth;
		CButton* checks[] = { &m_showSpaces, &m_showBoundaries, &m_showOpenings };
		for (CButton* check : checks) {
			CString checkText;
			check->GetWindowText(checkText);
			const int checkWidth = ::GetSystemMetrics(SM_CXMENUCHECK) +
				static_cast<int>(dc.GetTextExtent(checkText).cx) + margin;
			check->MoveWindow(checkLeft, showTop, checkWidth, rowHeight);
			checkLeft += checkWidth + rowSpacing;
		}
		const int exceptionsTop = showTop + rowHeight + rowSpacing;
		m_visibilityExceptions.MoveWindow(visibilityContentLeft, exceptionsTop,
			max(rowHeight, groupWidth - 2 * margin),
			max(rowHeight, static_cast<int>(page.bottom) - exceptionsTop - margin));

		m_coloringDetails.MoveWindow(coloringLeft + margin, page.top + rowHeight,
			max(rowHeight, coloringWidth - 2 * margin),
			max(rowHeight, page.Height() - rowHeight - margin));
	}
}

#include "stdafx.h"

#include "BCFCommentForm.h"
#include "BCFView.h"
#include "BCFViewPointMgr.h"

namespace
{
	enum CameraValue
	{
		ViewPoint,
		Direction,
		UpVector,
		AspectRatio,
		Scale,
		FieldOfView
	};

	CString FormatPoint(const BCFPoint& point)
	{
		CString value;
		value.Format(L"%.5g, %.5g, %.5g", point.xyz[0], point.xyz[1], point.xyz[2]);
		return value;
	}

	bool ParsePoint(const CString& value, BCFPoint& point)
	{
		return swscanf_s(value, L" %lf , %lf , %lf ",
			&point.xyz[0], &point.xyz[1], &point.xyz[2]) == 3;
	}

	bool ParseDouble(const CString& value, double& result)
	{
		wchar_t tail = 0;
		return swscanf_s(value, L" %lf %c", &result, &tail, 1) == 1;
	}
}

BEGIN_MESSAGE_MAP(CBCFCommentForm, CWnd)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PANE_COMMENT_VIEW_TOPIC, &CBCFCommentForm::OnViewTopic)
	ON_NOTIFY(TCN_SELCHANGE, IDC_PANE_COMMENT_TABS, &CBCFCommentForm::OnTabChanged)
	ON_CBN_SELCHANGE(IDC_PANE_COMMENT_CAMERA, &CBCFCommentForm::OnCameraChanged)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SNAPSHOT_SELECT, &CBCFCommentForm::OnSelectSnapshot)
	ON_BN_CLICKED(IDC_PANE_COMMENT_FROM_VIEW, &CBCFCommentForm::OnFromView)
	ON_BN_CLICKED(IDC_PANE_COMMENT_TO_VIEW, &CBCFCommentForm::OnToView)
	ON_BN_CLICKED(IDC_PANE_COMMENT_GRAB_SELECTED, &CBCFCommentForm::OnGrabSelected)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SELECT_COMPONENTS, &CBCFCommentForm::OnSelectComponents)
	ON_BN_CLICKED(IDC_PANE_COMMENT_GRAB_VISIBLE, &CBCFCommentForm::OnGrabVisible)
	ON_BN_CLICKED(IDC_PANE_COMMENT_SET_VISIBLE, &CBCFCommentForm::OnSetVisible)
END_MESSAGE_MAP()

BOOL CBCFCommentForm::Create(CBCFView* pane)
{
	m_pane = pane;
	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}
	m_viewTopic.Create(L"<<", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
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
	m_snapshot.Create(L"No snapshot", WS_CHILD | SS_CENTER | SS_CENTERIMAGE | SS_SUNKEN,
		CRect(), this);
	SetBCFControlFont(m_snapshot, this);
	m_selectSnapshot.Create(L"Select file...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SNAPSHOT_SELECT);
	m_captureSnapshot.Create(L"Capture", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SNAPSHOT_CAPTURE);
	SetBCFControlFont(m_selectSnapshot, this);
	SetBCFControlFont(m_captureSnapshot, this);
	m_captureSnapshot.EnableWindow(FALSE);
	m_cameraGroup.Create(L"Camera", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	SetBCFControlFont(m_cameraGroup, this);
	const wchar_t* labels[] = {
		L"View Point:", L"Direction:", L"Up vector:", L"Aspect Ratio:", L"Scale:", L"Field of view:"
	};
	for (size_t i = 0; i < _countof(m_cameraLabels); ++i) {
		CreateBCFStaticLabel(m_cameraLabels[i], labels[i], this);
		m_cameraValues[i].Create(WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
			CRect(), this, 0);
		SetBCFControlFont(m_cameraValues[i], this);
	}
	m_camera.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		CRect(), this, IDC_PANE_COMMENT_CAMERA);
	SetBCFControlFont(m_camera, this);
	m_camera.AddString(L"No");
	m_camera.AddString(L"Orthogonal");
	m_camera.AddString(L"Perspective");
	m_fromView.Create(L"From View", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_FROM_VIEW);
	m_toView.Create(L"To View", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_TO_VIEW);
	SetBCFControlFont(m_fromView, this);
	SetBCFControlFont(m_toView, this);
	m_selectionGroup.Create(L"Selection", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	m_grabSelected.Create(L"Grab selected", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_GRAB_SELECTED);
	m_selectComponents.Create(L"Select", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_SELECT_COMPONENTS);
	m_selectedComponents.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
		LBS_NOINTEGRALHEIGHT, CRect(), this, 0);
	m_visibilityGroup.Create(L"Visibility", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	m_grabVisible.Create(L"Grab visible", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_GRAB_VISIBLE);
	m_setVisible.Create(L"Set visible", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
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
	CWnd* visualizationControls[] = {
		&m_selectionGroup, &m_grabSelected, &m_selectComponents,
		&m_selectedComponents, &m_visibilityGroup, &m_grabVisible, &m_setVisible,
		&m_visibilityMode, &m_showLabel, &m_showSpaces, &m_showBoundaries,
		&m_showOpenings, &m_visibilityExceptions, &m_coloringGroup
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
	if (!GetSafeHwnd()) {
		return;
	}
	UpdateHeader();
	m_text.SetWindowText(comment ? FromUTF8(comment->GetText()) : CString());
	LoadViewPoint();
	ReloadSelection();
	ReloadVisibility();
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
	ok = CommitViewPoint() && ok;
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
	m_camera.SetCurSel(viewPoint
		? (viewPoint->GetCameraType() == BCFCameraOrthogonal ? 1 : 2)
		: 0);
	if (viewPoint) {
		BCFPoint point = {};
		viewPoint->GetCameraViewPoint(point);
		m_cameraValues[ViewPoint].SetWindowText(FormatPoint(point));
		viewPoint->GetCameraDirection(point);
		m_cameraValues[Direction].SetWindowText(FormatPoint(point));
		viewPoint->GetCameraUpVector(point);
		m_cameraValues[UpVector].SetWindowText(FormatPoint(point));
		CString value;
		value.Format(L"%.5g", viewPoint->GetAspectRatio());
		m_cameraValues[AspectRatio].SetWindowText(value);
		value.Format(L"%.5g", viewPoint->GetViewToWorldScale());
		m_cameraValues[Scale].SetWindowText(value);
		value.Format(L"%.5g", viewPoint->GetFieldOfView());
		m_cameraValues[FieldOfView].SetWindowText(value);
		CString snapshot = FromUTF8(viewPoint->GetSnapshot());
		m_snapshot.SetWindowText(snapshot.IsEmpty() ? CString(L"No snapshot") : snapshot);
	}
	else {
		for (CBCFEdit& value : m_cameraValues) {
			value.SetWindowText(CString());
		}
		m_snapshot.SetWindowText(L"No snapshot");
	}
	UpdateCameraControls();
}

bool CBCFCommentForm::CommitViewPoint()
{
	if (!m_comment) {
		return true;
	}
	const int camera = m_camera.GetCurSel();
	if (camera <= 0) {
		return true;
	}
	BCFViewPoint* viewPoint = m_comment->GetViewPoint();
	if (!viewPoint) {
		viewPoint = m_comment->GetTopic().AddViewPoint();
		if (!viewPoint || !m_comment->SetViewPoint(viewPoint)) {
			return false;
		}
	}

	BCFPoint points[3] = {};
	double values[3] = {};
	CString text;
	for (int i = 0; i < 3; ++i) {
		m_cameraValues[i].GetWindowText(text);
		if (!ParsePoint(text, points[i])) {
			AfxMessageBox(L"Camera point must contain three numeric coordinates.", MB_ICONERROR);
			return false;
		}
	}
	for (int i = AspectRatio; i <= FieldOfView; ++i) {
		m_cameraValues[i].GetWindowText(text);
		if (!ParseDouble(text, values[i - AspectRatio])) {
			AfxMessageBox(L"Camera value must be numeric.", MB_ICONERROR);
			return false;
		}
	}

	bool ok = viewPoint->SetCameraType(camera == 1 ? BCFCameraOrthogonal : BCFCameraPerspective);
	ok = viewPoint->SetCameraViewPoint(&points[0]) && ok;
	ok = viewPoint->SetCameraDirection(&points[1]) && ok;
	ok = viewPoint->SetCameraUpVector(&points[2]) && ok;
	ok = viewPoint->SetAspectRatio(values[0]) && ok;
	ok = viewPoint->SetViewToWorldScale(values[1]) && ok;
	ok = viewPoint->SetFieldOfView(values[2]) && ok;
	return ok;
}

void CBCFCommentForm::OnTabChanged(NMHDR*, LRESULT* result)
{
	ShowTab(m_tabs.GetCurSel());
	*result = 0;
}

void CBCFCommentForm::ShowTab(int tab)
{
	const int generalCommand = tab == 0 ? SW_SHOW : SW_HIDE;
	m_textLabel.ShowWindow(generalCommand);
	m_text.ShowWindow(generalCommand);
	m_snapshot.ShowWindow(generalCommand);
	m_selectSnapshot.ShowWindow(generalCommand);
	m_captureSnapshot.ShowWindow(generalCommand);
	m_cameraGroup.ShowWindow(generalCommand);
	m_camera.ShowWindow(generalCommand);
	m_fromView.ShowWindow(generalCommand);
	m_toView.ShowWindow(generalCommand);
	for (size_t i = 0; i < _countof(m_cameraLabels); ++i) {
		m_cameraLabels[i].ShowWindow(generalCommand);
		m_cameraValues[i].ShowWindow(generalCommand);
	}
	const int visualizationCommand = tab == 1 ? SW_SHOW : SW_HIDE;
	CWnd* visualizationControls[] = {
		&m_selectionGroup, &m_grabSelected, &m_selectComponents,
		&m_selectedComponents, &m_visibilityGroup, &m_grabVisible, &m_setVisible,
		&m_visibilityMode, &m_showLabel, &m_showSpaces, &m_showBoundaries,
		&m_showOpenings, &m_visibilityExceptions, &m_coloringGroup
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

void CBCFCommentForm::UpdateCameraControls()
{
	const int camera = m_camera.GetCurSel();
	for (int i = 0; i < static_cast<int>(_countof(m_cameraValues)); ++i) {
		const bool enabled = camera > 0 &&
			(i != Scale || camera == 1) &&
			(i != FieldOfView || camera == 2);
		m_cameraLabels[i].EnableWindow(enabled);
		m_cameraValues[i].EnableWindow(enabled);
	}
	m_toView.EnableWindow(m_comment && m_comment->GetViewPoint() && camera > 0);
}

void CBCFCommentForm::OnCameraChanged()
{
	UpdateCameraControls();
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
		m_snapshot.SetWindowText(dialog.GetPathName());
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnFromView()
{
	if (!m_comment || !m_pane->GetDocument()) {
		return;
	}
	const bool ok = CBCFViewPointMgr(*m_pane->GetDocument()).SaveCurrentViewToComent(*m_comment);
	m_pane->ShowLog(!ok);
	if (ok) {
		LoadViewPoint();
	}
}

void CBCFCommentForm::OnToView()
{
	if (m_comment && m_comment->GetViewPoint() && m_pane->GetDocument()) {
		CBCFViewPointMgr(*m_pane->GetDocument()).SetViewFromComment(*m_comment);
		m_pane->ShowLog(false);
	}
}

void CBCFCommentForm::ReloadSelection()
{
	m_selectedComponents.ResetContent();
	BCFViewPoint* viewPoint = m_comment ? m_comment->GetViewPoint() : nullptr;
	if (viewPoint) {
		for (uint16_t i = 0; BCFComponent* component = viewPoint->GetSelection(i); ++i) {
			CString text = FromUTF8(component->GetIfcGuid());
			if (text.IsEmpty()) {
				text = FromUTF8(component->GetAuthoringToolId());
			}
			if (text.IsEmpty()) {
				text = FromUTF8(component->GetOriginatingSystem());
			}
			if (text.IsEmpty()) {
				text = L"(Unidentified component)";
			}
			m_selectedComponents.AddString(text);
		}
	}
	m_selectComponents.EnableWindow(viewPoint && m_selectedComponents.GetCount() > 0);
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
		UpdateCameraControls();
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
			CString text = FromUTF8(component->GetIfcGuid());
			if (text.IsEmpty()) {
				text = FromUTF8(component->GetAuthoringToolId());
			}
			if (text.IsEmpty()) {
				text = FromUTF8(component->GetOriginatingSystem());
			}
			if (text.IsEmpty()) {
				text = L"(Unidentified component)";
			}
			if (!exceptions.IsEmpty()) {
				exceptions += L"\r\n";
			}
			exceptions += text;
		}
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
		UpdateCameraControls();
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

	m_viewTopic.MoveWindow(0, 0, buttonWidth, rowHeight);
	m_headerInfo.MoveWindow(buttonWidth + margin, labelOffset,
		max(rowHeight, cx - buttonWidth - 2 * margin), textHeight);
	const int tabsTop = rowHeight + rowSpacing;
	m_tabs.MoveWindow(margin, tabsTop, max(rowHeight, cx - 2 * margin),
		max(2 * rowHeight, cy - tabsTop - margin));
	CRect page(0, 0, max(rowHeight, cx - 2 * margin), max(2 * rowHeight, cy - tabsTop - margin));
	m_tabs.AdjustRect(FALSE, page);
	page.OffsetRect(margin, tabsTop);
	page.DeflateRect(margin, margin);

	if (m_tabs.GetCurSel() == 0) {
		const int vectorLabelWidth = static_cast<int>(dc.GetTextExtent(L"View Point:").cx) + margin;
		const int vectorValueWidth = static_cast<int>(
			dc.GetTextExtent(L"-12345, -12345, -12345").cx) + 2 * margin;
		const int scalarLabelWidth = static_cast<int>(dc.GetTextExtent(L"Field of view:").cx) + margin;
		const int scalarValueWidth = static_cast<int>(dc.GetTextExtent(L"-12345").cx) + 2 * margin;
		const int cameraWidth = vectorLabelWidth + vectorValueWidth +
			scalarLabelWidth + scalarValueWidth + 3 * margin + rowSpacing;
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
		CString captureText;
		m_selectSnapshot.GetWindowText(selectText);
		m_captureSnapshot.GetWindowText(captureText);
		const int selectWidth = static_cast<int>(dc.GetTextExtent(selectText).cx) + 2 * margin;
		const int captureWidth = static_cast<int>(dc.GetTextExtent(captureText).cx) + 2 * margin;
		const int snapshotButtonsTop = page.bottom - rowHeight;
		m_selectSnapshot.MoveWindow(secondLeft, snapshotButtonsTop, selectWidth, rowHeight);
		m_captureSnapshot.MoveWindow(secondLeft + selectWidth + rowSpacing,
			snapshotButtonsTop, captureWidth, rowHeight);

		m_cameraGroup.MoveWindow(cameraLeft, page.top, cameraWidth, page.Height());
		const int cameraContentLeft = cameraLeft + margin;
		const int vectorControlLeft = cameraContentLeft + vectorLabelWidth;
		const int scalarLabelLeft = vectorControlLeft + vectorValueWidth + rowSpacing;
		const int scalarControlLeft = scalarLabelLeft + scalarLabelWidth;
		CString fromText;
		CString toText;
		m_fromView.GetWindowText(fromText);
		m_toView.GetWindowText(toText);
		const int fromWidth = static_cast<int>(dc.GetTextExtent(fromText).cx) + 2 * margin;
		const int toWidth = static_cast<int>(dc.GetTextExtent(toText).cx) + 2 * margin;
		const int cameraButtonsWidth = fromWidth + rowSpacing + toWidth;
		const int cameraTop = page.top + rowHeight;
		m_camera.MoveWindow(cameraContentLeft, page.top + rowHeight,
			max(rowHeight, cameraWidth - 2 * margin - cameraButtonsWidth - rowSpacing),
			3 * rowHeight);
		m_fromView.MoveWindow(cameraLeft + cameraWidth - margin - cameraButtonsWidth,
			cameraTop, fromWidth, rowHeight);
		m_toView.MoveWindow(cameraLeft + cameraWidth - margin - toWidth,
			cameraTop, toWidth, rowHeight);
		for (int i = 0; i < static_cast<int>(_countof(m_cameraLabels)); ++i) {
			const bool scalar = i >= AspectRatio;
			const int row = scalar ? i - AspectRatio : i;
			const int top = page.top + (row + 2) * (rowHeight + rowSpacing);
			const int labelLeft = scalar ? scalarLabelLeft : cameraContentLeft;
			const int controlLeft = scalar ? scalarControlLeft : vectorControlLeft;
			m_cameraLabels[i].MoveWindow(labelLeft, top + labelOffset,
				scalar ? scalarLabelWidth : vectorLabelWidth, textHeight);
			m_cameraValues[i].MoveWindow(controlLeft, top + labelOffset,
				scalar ? scalarValueWidth : vectorValueWidth, rowHeight - labelOffset);
		}
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
		m_grabSelected.MoveWindow(contentLeft, contentTop, grabWidth, rowHeight);
		m_selectComponents.MoveWindow(contentLeft + grabWidth + rowSpacing,
			contentTop, selectWidth, rowHeight);
		const int listTop = contentTop + rowHeight + rowSpacing;
		m_selectedComponents.MoveWindow(contentLeft, listTop,
			max(rowHeight, groupWidth - 2 * margin),
			max(rowHeight, static_cast<int>(page.bottom) - listTop - margin));

		CString grabVisibleText;
		CString setVisibleText;
		m_grabVisible.GetWindowText(grabVisibleText);
		m_setVisible.GetWindowText(setVisibleText);
		const int grabVisibleWidth =
			static_cast<int>(dc.GetTextExtent(grabVisibleText).cx) + 2 * margin;
		const int setVisibleWidth =
			static_cast<int>(dc.GetTextExtent(setVisibleText).cx) + 2 * margin;
		const int visibilityContentLeft = visibilityLeft + margin;
		m_grabVisible.MoveWindow(visibilityContentLeft, contentTop,
			grabVisibleWidth, rowHeight);
		m_setVisible.MoveWindow(visibilityContentLeft + grabVisibleWidth + rowSpacing,
			contentTop, setVisibleWidth, rowHeight);
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
	}
}

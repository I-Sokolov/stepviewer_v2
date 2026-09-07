#include "stdafx.h"

#include "BCFProjectForm.h"
#include "BCFProjectDlg.h"
#include "BCFView.h"
#include "BCFViewControls.h"

namespace
{
	const int COLUMN_COUNT = 10;
	const wchar_t* COLUMN_NAMES[COLUMN_COUNT] = {
		L"GUID", L"Title", L"Type", L"Stage", L"Status",
		L"Assigned", L"Priority", L"Due", L"Created", L"Modified"
	};
	const int COLUMN_WIDTHS[COLUMN_COUNT] = { 190, 180, 90, 90, 90, 130, 80, 90, 130, 130 };
}

BEGIN_MESSAGE_MAP(CBCFProjectForm, CWnd)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PANE_TOPICS, &CBCFProjectForm::OnTopicChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_PANE_TOPICS, &CBCFProjectForm::OnTopicDoubleClick)
END_MESSAGE_MAP()

BOOL CBCFProjectForm::Create(CBCFView* pane)
{
	m_pane = pane;

	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}

	CreateBCFStaticLabel(m_topicsLabel, L"Topics:", this);
	
	m_topics.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		CRect(), this, IDC_PANE_TOPICS);
	SetBCFControlFont(m_topics, this);
	m_topics.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	
	for (int column = 0; column < COLUMN_COUNT; ++column) {
		m_topics.InsertColumn(column, COLUMN_NAMES[column], LVCFMT_LEFT, COLUMN_WIDTHS[column]);
	}
	
	return TRUE;
}

void CBCFProjectForm::Load(BCFTopic* selectTopic)
{
	BCFTopic* selected = selectTopic ? selectTopic : GetSelectedTopic();
	BCFProject* project = m_pane->GetProject();
	m_topics.SetRedraw(FALSE);
	m_topics.DeleteAllItems();
	int selectedItem = -1;
	if (project) {
		for (uint16_t i = 0; BCFTopic* topic = project->GetTopic(i); ++i) {
			int item = m_topics.InsertItem(m_topics.GetItemCount(), FromUTF8(topic->GetGuid()));
			m_topics.SetItemText(item, 1, FromUTF8(topic->GetTitle()));
			m_topics.SetItemText(item, 2, FromUTF8(topic->GetTopicType()));
			m_topics.SetItemText(item, 3, FromUTF8(topic->GetStage()));
			m_topics.SetItemText(item, 4, FromUTF8(topic->GetTopicStatus()));
			m_topics.SetItemText(item, 5, FromUTF8(topic->GetAssignedTo()));
			m_topics.SetItemText(item, 6, FromUTF8(topic->GetPriority()));
			m_topics.SetItemText(item, 7, FromUTF8(topic->GetDueDate()));
			CString created = CBCFProjectDlg::FormatDateTime(topic->GetCreationDate());
			if (*topic->GetCreationAuthor()) {
				created.AppendFormat(L" - %s", FromUTF8(topic->GetCreationAuthor()).GetString());
			}
			m_topics.SetItemText(item, 8, created);
			CString modified = CBCFProjectDlg::FormatDateTime(topic->GetModifiedDate());
			if (*topic->GetModifiedAuthor()) {
				modified.AppendFormat(L" - %s", FromUTF8(topic->GetModifiedAuthor()).GetString());
			}
			m_topics.SetItemText(item, 9, modified);
			m_topics.SetItemData(item, reinterpret_cast<DWORD_PTR>(topic));
			if (topic == selected) {
				selectedItem = item;
			}
		}
	}
	m_topics.SetRedraw(TRUE);
	m_topics.Invalidate();
	if (selectedItem < 0 && m_topics.GetItemCount()) {
		selectedItem = 0;
	}
	if (selectedItem >= 0) {
		m_topics.SetItemState(selectedItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
	m_pane->RefreshCommandUI();
}

bool CBCFProjectForm::Commit()
{
	return m_pane->CommitProjectInfo();
}

BCFTopic* CBCFProjectForm::GetSelectedTopic() const
{
	POSITION position = m_topics.GetFirstSelectedItemPosition();
	if (!position) {
		return nullptr;
	}
	int item = const_cast<CListCtrl&>(m_topics).GetNextSelectedItem(position);
	return reinterpret_cast<BCFTopic*>(m_topics.GetItemData(item));
}

void CBCFProjectForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_topics.GetSafeHwnd()) {
		return;
	}
	const int margin = 8;
	m_topicsLabel.MoveWindow(margin, margin, 60, 18);
	m_topics.MoveWindow(margin, margin + 20, max(20, cx - 2 * margin),
		max(20, cy - 3 * margin - 12));
}

void CBCFProjectForm::OnTopicChanged(NMHDR*, LRESULT* result)
{
	m_pane->RefreshCommandUI();
	*result = 0;
}

void CBCFProjectForm::OnTopicDoubleClick(NMHDR*, LRESULT* result)
{
	if (BCFTopic* topic = GetSelectedTopic()) {
		m_pane->ShowTopic(topic);
	}
	*result = 0;
}

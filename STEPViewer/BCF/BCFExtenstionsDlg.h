#pragma once

#include "bcfAPI.h"

#include <memory>
#include <vector>

class CBCFExtensionUserPage : public CPropertyPage
{
public:
	explicit CBCFExtensionUserPage(const CString& user);

	CString GetUser() const { return m_user; }

protected:
	virtual void DoDataExchange(CDataExchange* dataExchange) override;

private:
	CString m_user;
};

class CBCFExtensionEnumerationPage : public CPropertyPage
{
public:
	CBCFExtensionEnumerationPage(BCFExtensions& extensions,
		BCFEnumeration enumeration, LPCTSTR title);

	bool Apply();
	bool IsEmpty();
	CString GetTitle() const { return m_title; }

protected:
	virtual BOOL OnInitDialog() override;
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnMoveUp();
	afx_msg void OnMoveDown();
	afx_msg void OnSelectionChanged();
	DECLARE_MESSAGE_MAP()

private:
	void MoveSelection(int offset);
	void UpdateButtons();

	BCFExtensions& m_extensions;
	BCFEnumeration m_enumeration;
	CString m_title;
	std::vector<CString> m_originalValues;
	CListBox m_values;
	CButton m_remove;
	CButton m_moveUp;
	CButton m_moveDown;
};

class CBCFExtenstionsDlg : public CPropertySheet
{
public:
	CBCFExtenstionsDlg(BCFProject& project, const CString& user, CWnd* parent);

	CString GetUser() const { return m_userPage.GetUser(); }

protected:
	afx_msg void OnApplySettings();
	DECLARE_MESSAGE_MAP()

private:
	CBCFExtensionUserPage m_userPage;
	std::vector<std::unique_ptr<CBCFExtensionEnumerationPage>> m_extensionPages;
};

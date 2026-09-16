#pragma once

#include <atlimage.h>

class CBCFSnapshotCtrl : public CStatic
{
public:
	bool Load(const CString& path);
	void Clear();
	int GetPreferredWidth(int height);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	CImage m_image;
};

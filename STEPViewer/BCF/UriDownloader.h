#pragma once

class CUriDownloader
{
public:
	explicit CUriDownloader(CWnd* parent);

	static bool IsUri(const CString& path);
	CString GetLocalPath(const CString& uri) const;

private:
	CWnd* m_parent;
};

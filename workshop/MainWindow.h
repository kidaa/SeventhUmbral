#pragma once

#include <map>
#include "win32/Dialog.h"
#include "win32/Tab.h"

class CMainWindow : public Framework::Win32::CDialog
{
public:
								CMainWindow();
	virtual						~CMainWindow();

protected:
	long						OnCommand(unsigned short, unsigned short, HWND) override;
	long						OnNotify(WPARAM, NMHDR*) override;
	long						OnSize(unsigned int, unsigned int, unsigned int) override;

private:
	typedef std::unique_ptr<Framework::Win32::CWindow> DocumentPtr;
	typedef std::map<uint32, DocumentPtr> DocumentMap;

	void						LoadWindowRect();
	void						SaveWindowRect();
	void						Destroy();
	void						ShowWelcomePage();
	void						ShowApperanceViewer();
	void						ShowWorldViewer(uint32);
	void						ShowSheetViewer(uint32);
	void						ShowAbout();

	void						InsertDocument(DocumentPtr&&);
	void						UpdateLayout();
	void						SelectTab(int);
	void						UnselectTab(int);

	Framework::Win32::CTab		m_tabs;
	DocumentMap					m_documents;
	uint32						m_nextDocumentId = 1;
};

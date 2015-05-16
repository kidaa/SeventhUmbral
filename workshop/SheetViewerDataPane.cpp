#include "SheetViewerDataPane.h"
#include "resource.h"
#include "../dataobjects/Sheet.h"
#include "../dataobjects/SheetData.h"
#include "../dataobjects/FileManager.h"
#include "../dataobjects/HalfFloat.h"
#include "string_cast.h"
#include "StdStreamUtils.h"
#include "Utf8.h"
#include "string_format.h"

CSheetViewerDataPane::CSheetViewerDataPane(HWND parentWnd)
: Framework::Win32::CDialog(MAKEINTRESOURCE(IDD_SHEETVIEWER_DATAPANE), parentWnd)
{
	try
	{
		auto definitionFilePath = CFileManager::GetServerDataPath() / "ffxivd_sheetcolumns.xml";
		auto definitionStream = Framework::CreateInputStdStream(definitionFilePath.native());
		m_sheetColumnsDefinitions = CSheetColumnsDefinition::CreateFromXml(definitionStream);
	}
	catch(...)
	{
		//Failed to read definition file
	}

	SetClassPtr();

	m_dataListView = GetItem(IDC_SHEETDATA_LIST);
	m_dataListView.SetExtendedListViewStyle(m_dataListView.GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	//Data View Columns
	{
		Framework::Win32::CRect columnHeaderRect(0, 0, 50, 72);
		MapDialogRect(m_hWnd, columnHeaderRect);

		LVCOLUMN col = {};
		col.pszText = _T("ID");
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = columnHeaderRect.Width();
		m_dataListView.InsertColumn(0, col);
	}
}

CSheetViewerDataPane::~CSheetViewerDataPane()
{

}

void CSheetViewerDataPane::SetSheetInfo(const std::string& sheetName, uint32 sheetId)
{
	m_sheetName = sheetName;
	m_sheetId = sheetId;
	PopulateDataView(sheetId);
	m_dataListView.EnsureItemVisible(0, false);
}

void CSheetViewerDataPane::SetLanguage(const char* language)
{
	m_language = language;
	if(m_sheetId != 0)
	{
		PopulateDataView(m_sheetId);
	}
}

long CSheetViewerDataPane::OnNotify(WPARAM, LPNMHDR nmhdr)
{
	if(nmhdr->idFrom == IDC_SHEETDATA_LIST)
	{
		switch(nmhdr->code)
		{
		case LVN_GETDISPINFO:
			OnDataViewGetDispInfo(reinterpret_cast<NMLVDISPINFO*>(nmhdr));
			break;
		}
	}
	return FALSE;
}

long CSheetViewerDataPane::OnSize(unsigned int, unsigned int, unsigned int)
{
	auto clientRect = GetClientRect();
	m_dataListView.SetSizePosition(clientRect);
	return FALSE;
}

void CSheetViewerDataPane::OnDataViewGetDispInfo(NMLVDISPINFO* dispInfo)
{
	const auto& displayRow = m_displayRows[dispInfo->item.iItem];
	const auto& displayText = displayRow[dispInfo->item.iSubItem];
	dispInfo->item.pszText = const_cast<TCHAR*>(displayText.c_str());
}

void CSheetViewerDataPane::PopulateDataView(uint32 sheetId)
{
	CSheet sheet;
	{
		auto sheetFilePath = CFileManager::GetResourcePath(sheetId);
		auto sheetStream = Framework::CreateInputStdStream(sheetFilePath.native());
		sheet = CSheet::Create(sheetStream);
	}

	unsigned int subSheetId = 0;
	for(unsigned int i = 0; i < sheet.GetSubSheets().size(); i++)
	{
		const auto& subSheet = sheet.GetSubSheets()[i];
		if(!subSheet.lang.compare(m_language))
		{
			subSheetId = i;
			break;
		}
	}

	const auto& subSheet = sheet.GetSubSheets()[subSheetId];

	auto sheetData = CSheetData::Create(sheet, subSheetId,
		[](uint32 fileId)
		{
			auto filePath = CFileManager::GetResourcePath(fileId);
			return new Framework::CStdStream(Framework::CreateInputStdStream(filePath.native()));
		}
	);

	m_displayRows.clear();
	m_displayRows.reserve(sheetData.GetRows().size());

	for(const auto& rowPair : sheetData.GetRows())
	{
		StringArray displayRow;
		displayRow.reserve(subSheet.columnCount + 1);

		auto rowId = string_cast<std::tstring>(std::to_string(rowPair.first));
		displayRow.push_back(rowId);

		assert(rowPair.second.size() == subSheet.columnCount);
		
		for(unsigned int i = 0; i < subSheet.columnCount; i++)
		{
			auto typeParam = subSheet.typeParams[i];
			auto cellValue = rowPair.second[i];
			std::tstring displayText;
			switch(typeParam)
			{
			case CSheet::TYPE_PARAM_U8:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<uint8>(cellValue.GetValue8())));
				break;
			case CSheet::TYPE_PARAM_S8:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<int8>(cellValue.GetValue8())));
				break;
			case CSheet::TYPE_PARAM_U16:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<uint16>(cellValue.GetValue16())));
				break;
			case CSheet::TYPE_PARAM_S16:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<int16>(cellValue.GetValue16())));
				break;
			case CSheet::TYPE_PARAM_U32:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<uint32>(cellValue.GetValue32())));
				break;
			case CSheet::TYPE_PARAM_S32:
				displayText = string_cast<std::tstring>(std::to_string(static_cast<int32>(cellValue.GetValue32())));
				break;
			case CSheet::TYPE_PARAM_F16:
				displayText = string_cast<std::tstring>(std::to_string(CHalfFloat::ToFloat(cellValue.GetValue16())));
				break;
			case CSheet::TYPE_PARAM_F32:
				displayText = string_cast<std::tstring>(std::to_string(cellValue.GetValueFloat()));
				break;
			case CSheet::TYPE_PARAM_BOOL:
				displayText = (cellValue.GetValue8() == 0) ? _T("false") : _T("true");
				break;
			case CSheet::TYPE_PARAM_STRING:
				displayText = string_cast<std::tstring>(Framework::Utf8::ConvertFrom(cellValue.GetStringValue()));
				break;
			default:
				assert(0);
				break;
			}
			displayRow.push_back(displayText);
		}
		m_displayRows.push_back(displayRow);
	}

	DeleteDataViewColumns();
	CreateDataViewColumns(subSheet.columnCount);
	m_dataListView.SetItemCount(sheetData.GetRows().size());
}

void CSheetViewerDataPane::CreateDataViewColumns(unsigned int colCount)
{
	Framework::Win32::CRect columnHeaderRect(0, 0, 70, 72);
	MapDialogRect(m_hWnd, columnHeaderRect);
	for(unsigned int i = 0; i < colCount; i++)
	{
		auto columnName = string_cast<std::tstring>(m_sheetColumnsDefinitions.GetColumnName(m_sheetName, i + 1));
		if(columnName.empty())
		{
			columnName = string_format(_T("Column %d"), i + 1);
		}
		LVCOLUMN col = {};
		col.pszText = const_cast<TCHAR*>(columnName.c_str());
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = columnHeaderRect.Width();
		m_dataListView.InsertColumn(i + 1, col);
	}
}

void CSheetViewerDataPane::DeleteDataViewColumns()
{
	while(m_dataListView.DeleteColumn(1))
	{

	}
}

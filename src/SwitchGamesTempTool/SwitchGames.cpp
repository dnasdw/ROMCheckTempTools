#include "SwitchGames.h"
#include <tinyxml2.h>

CSwitchGames::STextFileContent::STextFileContent()
	: EncodingOld(kEncodingUnknown)
	, EncodingNew(kEncodingUnknown)
	, LineTypeOld(kLineTypeUnknown)
	, LineTypeNew(kLineTypeUnknown)
{
}

CSwitchGames::CSwitchGames()
	: m_nCheckLevel(kCheckLevelCRLF)
{
	m_sModuleDirName = UGetModuleDirName();
}

CSwitchGames::~CSwitchGames()
{
}

void CSwitchGames::SetResultFileName(const UString& a_sResultFileName)
{
	m_sResultFileName = a_sResultFileName;
}

void CSwitchGames::SetOutputDirName(const UString& a_sOutputDirName)
{
	m_sOutputDirName = a_sOutputDirName;
}

void CSwitchGames::SetCheckLevel(n32 a_nCheckLevel)
{
	m_nCheckLevel = a_nCheckLevel;
}

int CSwitchGames::Check()
{
	if (readConfig() != 0)
	{
		return 1;
	}
	if (readResult() != 0)
	{
		return 1;
	}
	if (!makeDir(m_sOutputDirName))
	{
		return 1;
	}
	if (checkResult() != 0)
	{
		return 1;
	}
	return 0;
}

string CSwitchGames::trim(const string& a_sLine)
{
	return WToU8(trim(U8ToW(a_sLine)));
}

wstring CSwitchGames::trim(const wstring& a_sLine)
{
	wstring sTrimmed = a_sLine;
	wstring::size_type uPos = sTrimmed.find_first_not_of(L"\n\v\f\r \x85\xA0");
	if (uPos == wstring::npos)
	{
		return L"";
	}
	sTrimmed.erase(0, uPos);
	uPos = sTrimmed.find_last_not_of(L"\n\v\f\r \x85\xA0");
	if (uPos != wstring::npos)
	{
		sTrimmed.erase(uPos + 1);
	}
	return sTrimmed;
}

bool CSwitchGames::empty(const string& a_sLine)
{
	return trim(a_sLine).empty();
}

bool CSwitchGames::pathCompare(const UString& lhs, const UString& rhs)
{
	wstring sLhs = UToW(lhs);
	transform(sLhs.begin(), sLhs.end(), sLhs.begin(), ::towupper);
	wstring sRhs = UToW(rhs);
	transform(sRhs.begin(), sRhs.end(), sRhs.begin(), ::towupper);
	return sLhs < sRhs;
}

int CSwitchGames::readTextFile(const UString& a_sFilePath, STextFileContent& a_TextFileContent)
{
	FILE* fp = UFopen(a_sFilePath.c_str(), USTR("rb"), true);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uFileSize = ftell(fp);
	if (uFileSize == 0)
	{
		fclose(fp);
		UPrintf(USTR("file size == 0: %") PRIUS USTR("\n"), a_sFilePath.c_str());
		return 1;
	}
	fseek(fp, 0, SEEK_SET);
	char* pTemp = new char[uFileSize + 1];
	fread(pTemp, 1, uFileSize, fp);
	fclose(fp);
	pTemp[uFileSize] = 0;
	a_TextFileContent.TextOld = pTemp;
	if (strlen(pTemp) != uFileSize || a_TextFileContent.TextOld.size() != uFileSize || memcmp(a_TextFileContent.TextOld.c_str(), pTemp, uFileSize) != 0)
	{
		delete[] pTemp;
		UPrintf(USTR("unicode?: %") PRIUS USTR("\n"), a_sFilePath.c_str());
		return 1;
	}
	delete[] pTemp;
	bool bCP437 = false;
	bool bUTF8 = false;
	if (StartWith(a_TextFileContent.TextOld, "\xEF\xBB\xBF"))
	{
		try
		{
			wstring sTextW = U8ToW(a_TextFileContent.TextOld);
			a_TextFileContent.TextNew = WToU8(sTextW);
			if (a_TextFileContent.TextNew == a_TextFileContent.TextOld)
			{
				bUTF8 = true;
				a_TextFileContent.EncodingOld = kEncodingUTF8withBOM;
			}
		}
		catch (...)
		{
			// do nothing
		}
	}
	if (!bUTF8)
	{
		try
		{
			wstring sTextW = XToW(a_TextFileContent.TextOld, 437, "CP437");
			a_TextFileContent.TextNew = WToX(sTextW, 437, "CP437");
			if (a_TextFileContent.TextNew == a_TextFileContent.TextOld)
			{
				bCP437 = true;
				a_TextFileContent.EncodingOld = kEncodingCP437;
			}
		}
		catch (...)
		{
			// do nothing
		}
	}
	if (!bCP437 && !bUTF8)
	{
		try
		{
			wstring sTextW = U8ToW(a_TextFileContent.TextOld);
			a_TextFileContent.TextNew = WToU8(sTextW);
			if (a_TextFileContent.TextNew == a_TextFileContent.TextOld)
			{
				bUTF8 = true;
				if (StartWith(a_TextFileContent.TextOld, "\xEF\xBB\xBF"))
				{
					a_TextFileContent.EncodingOld = kEncodingUTF8withBOM;
				}
				else
				{
					a_TextFileContent.EncodingOld = kEncodingUTF8;
				}
			}
		}
		catch (...)
		{
			// do nothing
		}
	}
	if (bCP437)
	{
		a_TextFileContent.TextNew = a_TextFileContent.TextOld;
		a_TextFileContent.EncodingNew = kEncodingCP437;
	}
	else if (bUTF8)
	{
		string sTextU8 = a_TextFileContent.TextOld;
		if (a_TextFileContent.EncodingOld == kEncodingUTF8withBOM)
		{
			sTextU8.erase(0, 3);
		}
		bool bUTF8ToCP437 = false;
		try
		{
			wstring sTextW = U8ToW(sTextU8);
			string sTextX = WToX(sTextW, 437, "CP437");
			wstring sTextXW = XToW(sTextX, 437, "CP437");
			if (sTextXW == sTextW)
			{
				a_TextFileContent.TextNew = sTextX;
				a_TextFileContent.EncodingNew = kEncodingCP437;
				bUTF8ToCP437 = true;
			}
		}
		catch (...)
		{
			// do nothing
		}
		if (!bUTF8ToCP437)
		{
			a_TextFileContent.TextNew = a_TextFileContent.TextOld;
			if (a_TextFileContent.EncodingOld == kEncodingUTF8)
			{
				a_TextFileContent.TextNew.insert(0, "\xEF\xBB\xBF");
			}
			a_TextFileContent.EncodingNew = kEncodingUTF8withBOM;
		}
	}
	else
	{
		a_TextFileContent.TextNew = a_TextFileContent.TextOld;
	}
	string sTextNoCRLF = Replace(a_TextFileContent.TextOld, "\r\n", "");
	string sTextNoCR = Replace(a_TextFileContent.TextOld, "\r", "");
	string sTextNoLF = Replace(a_TextFileContent.TextOld, "\n", "");
	n32 nCRLFCount = (a_TextFileContent.TextOld.size() - sTextNoCRLF.size()) / 2;
	n32 nCROnlyCount = (a_TextFileContent.TextOld.size() - sTextNoCR.size()) - nCRLFCount;
	n32 nLFOnlyCount = (a_TextFileContent.TextOld.size() - sTextNoLF.size()) - nCRLFCount;
	if (nCROnlyCount > nCRLFCount && nCROnlyCount > nLFOnlyCount)
	{
		if (nCRLFCount == 0 && nLFOnlyCount == 0)
		{
			a_TextFileContent.LineTypeOld = kLineTypeCR;
		}
		else
		{
			a_TextFileContent.LineTypeOld = kLineTypeCRMix;
		}
	}
	else if (nCRLFCount > nLFOnlyCount)
	{
		if (nLFOnlyCount == 0)
		{
			if (nCROnlyCount == 0)
			{
				a_TextFileContent.LineTypeOld = kLineTypeCRLF;
			}
			else
			{
				a_TextFileContent.LineTypeOld = kLineTypeCRLF_CR;
			}
		}
		else
		{
			a_TextFileContent.LineTypeOld = kLineTypeCRLFMix;
		}
	}
	else if (nLFOnlyCount > nCRLFCount)
	{
		if (nCRLFCount == 0)
		{
			if (nCROnlyCount == 0)
			{
				a_TextFileContent.LineTypeOld = kLineTypeLF;
			}
			else
			{
				a_TextFileContent.LineTypeOld = kLineTypeLF_CR;
			}
		}
		else
		{
			a_TextFileContent.LineTypeOld = kLineTypeLFMix;
		}
	}
	if (EndWith(a_sFilePath, USTR(".nfo")) && a_TextFileContent.LineTypeOld != kLineTypeUnknown && nCROnlyCount == 0)
	{
		a_TextFileContent.TextNew = Replace(a_TextFileContent.TextNew, "\r", "");
		a_TextFileContent.LineTypeNew = kLineTypeLF;
	}
	else if (EndWith(a_sFilePath, USTR(".nfo")) && a_TextFileContent.LineTypeOld == kLineTypeCR)
	{
		a_TextFileContent.TextNew = Replace(a_TextFileContent.TextNew, "\r", "\n");
		a_TextFileContent.LineTypeNew = kLineTypeLF;
	}
	else if (EndWith(a_sFilePath, USTR(".sfv")) && a_TextFileContent.LineTypeOld != kLineTypeUnknown && nCROnlyCount == 0)
	{
		a_TextFileContent.TextNew = Replace(a_TextFileContent.TextNew, "\r", "");
		a_TextFileContent.TextNew = Replace(a_TextFileContent.TextNew, "\n", "\r\n");
		a_TextFileContent.LineTypeNew = kLineTypeCRLF;
	}
	else if (EndWith(a_sFilePath, USTR(".sfv")) && a_TextFileContent.LineTypeOld == kLineTypeCR)
	{
		a_TextFileContent.TextNew = Replace(a_TextFileContent.TextNew, "\r", "\r\n");
		a_TextFileContent.LineTypeNew = kLineTypeCRLF;
	}
	else
	{
		a_TextFileContent.LineTypeNew = a_TextFileContent.LineTypeOld;
	}
	return 0;
}

bool CSwitchGames::makeDir(const UString& a_sDirPath)
{
	UString sDirPath = a_sDirPath;
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	u32 uMaxPath = sDirPath.size() + MAX_PATH * 2;
	wchar_t* pDirPath = new wchar_t[uMaxPath];
	if (_wfullpath(pDirPath, UToW(sDirPath).c_str(), uMaxPath) == nullptr)
	{
		return false;
	}
	sDirPath = WToU(pDirPath);
	delete[] pDirPath;
	if (!StartWith(sDirPath, USTR("\\\\")))
	{
		sDirPath = USTR("\\\\?\\") + sDirPath;
	}
#endif
	UString sPrefix;
	if (StartWith(sDirPath, USTR("\\\\?\\")))
	{
		sPrefix = USTR("\\\\?\\");
		sDirPath.erase(0, 4);
	}
	else if (StartWith(sDirPath, USTR("\\\\")))
	{
		sPrefix = USTR("\\\\");
		sDirPath.erase(0, 2);
	}
	vector<UString> vDirPath = SplitOf(sDirPath, USTR("/\\"));
	UString sDirName = sPrefix;
	UString sSep = sPrefix.empty() ? USTR("/") : USTR("\\");
	for (n32 i = 0; i < static_cast<n32>(vDirPath.size()); i++)
	{
		sDirName += vDirPath[i];
		if (!sPrefix.empty() && i < 1)
		{
			// do nothing
		}
		else if (!UMakeDir(sDirName.c_str()))
		{
			return false;
		}
		sDirName += sSep;
	}
	return true;
}

int CSwitchGames::writeFileString(const UString& a_sFilePath, const string& a_sStringContent)
{
	UString sFilePath = a_sFilePath;
	UString sDirPath = USTR(".");
	UString::size_type uPos = a_sFilePath.find_last_of(USTR("/\\"));
	if (uPos != UString::npos)
	{
		sDirPath = a_sFilePath.substr(0, uPos);
	}
	if (!makeDir(sDirPath))
	{
		return 1;
	}
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	u32 uMaxPath = sDirPath.size() + MAX_PATH * 2;
	wchar_t* pFilePath = new wchar_t[uMaxPath];
	if (_wfullpath(pFilePath, UToW(a_sFilePath).c_str(), uMaxPath) == nullptr)
	{
		return 1;
	}
	sFilePath = WToU(pFilePath);
	delete[] pFilePath;
	if (!StartWith(sFilePath, USTR("\\\\")))
	{
		sFilePath = USTR("\\\\?\\") + sFilePath;
	}
#endif
	FILE* fp = UFopen(sFilePath.c_str(), USTR("wb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fwrite(a_sStringContent.c_str(), 1, a_sStringContent.size(), fp);
	fclose(fp);
	return 0;
}

int CSwitchGames::readConfig()
{
	UString sConfigXmlPath = m_sModuleDirName + USTR("/Switch Games.xml");
	FILE* fp = UFopen(sConfigXmlPath.c_str(), USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	tinyxml2::XMLDocument xmlDoc;
	tinyxml2::XMLError eError = xmlDoc.LoadFile(fp);
	fclose(fp);
	if (eError != tinyxml2::XML_SUCCESS)
	{
		return 1;
	}
	const tinyxml2::XMLElement* pDocConfig = xmlDoc.FirstChildElement("config");
	if (pDocConfig == nullptr)
	{
		return 1;
	}
	const tinyxml2::XMLElement* pConfigGreen = pDocConfig->FirstChildElement("green");
	if (pConfigGreen == nullptr)
	{
		return 1;
	}
	for (const tinyxml2::XMLElement* pGreenName = pConfigGreen->FirstChildElement("name"); pGreenName != nullptr; pGreenName = pGreenName->NextSiblingElement("name"))
	{
		wstring sNameText;
		const char* pNameText = pGreenName->GetText();
		if (pNameText != nullptr)
		{
			sNameText = U8ToW(pNameText);
		}
		if (sNameText.empty())
		{
			continue;
		}
		try
		{
			wregex rPattern(sNameText, regex_constants::ECMAScript | regex_constants::icase);
			m_vGreenStyleNamePattern.push_back(rPattern);
		}
		catch (regex_error& e)
		{
			UPrintf(USTR("%") PRIUS USTR(" regex error: %") PRIUS USTR("\n\n"), WToU(sNameText).c_str(), AToU(e.what()).c_str());
			continue;
		}
	}
	const tinyxml2::XMLElement* pGreenExclude = pConfigGreen->FirstChildElement("exclude");
	if (pGreenExclude != nullptr)
	{
		for (const tinyxml2::XMLElement* pExcludeName = pGreenExclude->FirstChildElement("name"); pExcludeName != nullptr; pExcludeName = pExcludeName->NextSiblingElement("name"))
		{
			wstring sNameText;
			const char* pNameText = pExcludeName->GetText();
			if (pNameText != nullptr)
			{
				sNameText = U8ToW(pNameText);
			}
			if (sNameText.empty())
			{
				continue;
			}
			try
			{
				wregex rPattern(sNameText, regex_constants::ECMAScript | regex_constants::icase);
				m_vNotGreenStyleNamePattern.push_back(rPattern);
			}
			catch (regex_error& e)
			{
				UPrintf(USTR("%") PRIUS USTR(" regex error: %") PRIUS USTR("\n\n"), WToU(sNameText).c_str(), AToU(e.what()).c_str());
				continue;
			}
		}
	}
	return 0;
}

int CSwitchGames::readResult()
{
	FILE* fp = UFopen(m_sResultFileName.c_str(), USTR("rb"), false);
	if (fp == nullptr)
	{
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	u32 uResultFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* pTemp = new char[uResultFileSize + 1];
	fread(pTemp, 1, uResultFileSize, fp);
	fclose(fp);
	pTemp[uResultFileSize] = 0;
	string sResultText = pTemp;
	delete[] pTemp;
	vector<string> vResultText = SplitOf(sResultText, "\r\n");
	for (vector<string>::iterator itResultText = vResultText.begin(); itResultText != vResultText.end(); ++itResultText)
	{
		*itResultText = trim(*itResultText);
	}
	vector<string>::const_iterator itResultText = remove_if(vResultText.begin(), vResultText.end(), empty);
	vResultText.erase(itResultText, vResultText.end());
	if (!vResultText.empty())
	{
		if (vResultText[0] == "[yes]\t[no]\t[year]\t[name]\t[path]")
		{
			vResultText.erase(vResultText.begin());
		}
	}
	for (itResultText = vResultText.begin(); itResultText != vResultText.end(); ++itResultText)
	{
		wstring sLine = U8ToW(*itResultText);
		vector<wstring> vLine = Split(sLine, L"\t");
		if (vLine.size() != 5)
		{
			return 1;
		}
		SResult result;
		if (vLine[0] == L"[v]" && trim(vLine[1]).empty())
		{
			result.Exist = true;
		}
		else if (vLine[1] == L"[x]" && trim(vLine[0]).empty())
		{
			result.Exist = false;
		}
		else
		{
			return 1;
		}
		result.Year = SToN32(vLine[2]);
		if (result.Year == 0)
		{
			continue;
		}
		result.Name = vLine[3];
		result.Path = vLine[4];
		if (!result.Path.empty())
		{
			result.Type = result.Path.substr(0, result.Path.size() - result.Name.size() - 1);
			wstring::size_type uPos = result.Type.find_last_of(L"/\\");
			if (uPos != wstring::npos)
			{
				result.Type.erase(0, uPos + 1);
			}
		}
		m_vResult.push_back(result);
	}
	return 0;
}

int CSwitchGames::checkResult()
{
	n32 nCheckCount = 0;
	for (vector<SResult>::const_iterator itResult = m_vResult.begin(); itResult != m_vResult.end(); ++itResult)
	{
		const SResult& result = *itResult;
		if (result.Exist)
		{
			nCheckCount++;
		}
	}
	n32 nCheckIndex = 1;
	for (vector<SResult>::iterator itResult = m_vResult.begin(); itResult != m_vResult.end(); ++itResult)
	{
		SResult& result = *itResult;
		if (!result.Exist)
		{
			continue;
		}
		result.Type = L"unknown";
		const wstring& sName = result.Name;
		UPrintf(USTR("\t%5d/%5d\t%4d %") PRIUS USTR("\n"), nCheckIndex, nCheckCount, result.Year, WToU(sName).c_str());
		nCheckIndex++;
		UString sDirPath = WToU(result.Path);
		wstring& sType = result.Type;
		n32 nRowStyle = kStyleIdNone;
		if (matchGreenStyle(sName))
		{
			nRowStyle = kStyleIdGreen;
		}
		bool bNsp = false;
		bool bUpdate = false;
		bool bDlc = false;
		wregex rNsp(L"eshop", regex_constants::ECMAScript | regex_constants::icase);
		wregex rUpdate(L"update", regex_constants::ECMAScript | regex_constants::icase);
		wregex rDlc(L"dlc", regex_constants::ECMAScript | regex_constants::icase);
		wregex rNotDlc(L"incl.*dlc", regex_constants::ECMAScript | regex_constants::icase);
		if (regex_search(sName, rDlc) && !regex_search(sName, rNotDlc))
		{
			bDlc = true;
		}
		else if (regex_search(sName, rUpdate))
		{
			bUpdate = true;
		}
		else if (regex_search(sName, rNsp))
		{
			bNsp = true;
		}
		if (bNsp)
		{
			sType = L"nsp";
		}
		else if (bUpdate)
		{
			sType = L"update";
		}
		else if (bDlc)
		{
			sType = L"dlc";
		}
		if (m_nCheckLevel <= kCheckLevelName)
		{
			updateOutput(result, nRowStyle, L"");
			continue;
		}
		UString::size_type uPrefixSize = sDirPath.size() + 1;
		vector<UString> vFile;
		queue<UString> qDir;
		qDir.push(sDirPath);
		while (!qDir.empty())
		{
			UString& sParent = qDir.front();
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
			WIN32_FIND_DATAW ffd;
			HANDLE hFind = INVALID_HANDLE_VALUE;
			wstring sPattern = sParent + L"/*";
			hFind = FindFirstFileW(sPattern.c_str(), &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
					{
						wstring sFileName = sParent + L"/" + ffd.cFileName;
						vFile.push_back(sFileName.substr(uPrefixSize));
					}
					else if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
					{
						wstring sDir = sParent + L"/" + ffd.cFileName;
						qDir.push(sDir);
					}
				} while (FindNextFileW(hFind, &ffd) != 0);
				FindClose(hFind);
			}
#else
			DIR* pDir = opendir(sParent.c_str());
			if (pDir != nullptr)
			{
				dirent* pDirent = nullptr;
				while ((pDirent = readdir(pDir)) != nullptr)
				{
					string sName = pDirent->d_name;
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
					sName = TSToS<string, string>(sName, "UTF-8-MAC", "UTF-8");
#endif
					// handle cases where d_type is DT_UNKNOWN
					if (pDirent->d_type == DT_UNKNOWN)
					{
						string sPath = sParent + "/" + sName;
						Stat st;
						if (UStat(sPath.c_str(), &st) == 0)
						{
							if (S_ISREG(st.st_mode))
							{
								pDirent->d_type = DT_REG;
							}
							else if (S_ISDIR(st.st_mode))
							{
								pDirent->d_type = DT_DIR;
							}
						}
					}
					if (pDirent->d_type == DT_REG)
					{
						string sFileName = sParent + "/" + sName;
						vFile.push_back(sFileName.substr(uPrefixSize));
					}
					else if (pDirent->d_type == DT_DIR && strcmp(pDirent->d_name, ".") != 0 && strcmp(pDirent->d_name, "..") != 0)
					{
						string sDir = sParent + "/" + sName;
						qDir.push(sDir);
					}
				}
				closedir(pDir);
			}
#endif
			qDir.pop();
		}
		sort(vFile.begin(), vFile.end(), pathCompare);
		for (vector<UString>::const_iterator itFile = vFile.begin(); itFile != vFile.end(); ++itFile)
		{
			const UString& sFile = *itFile;
			UString::size_type uPos = sFile.rfind(USTR('.'));
			if (uPos == UString::npos)
			{
				result.OtherFile.push_back(sFile);
				continue;
			}
			UString sExtName = sFile.substr(uPos + 1);
			if (sExtName == USTR("rar"))
			{
				if (m_nCheckLevel <= kCheckLevelCRLF)
				{
					result.RarFile[sFile] = 0;
				}
				else
				{
					UString sFilePath = sDirPath + USTR("/") + sFile;
					FILE* fp = UFopen(sFilePath.c_str(), USTR("rb"), false);
					if (fp == nullptr)
					{
						return 1;
					}
					Fseek(fp, 0, SEEK_END);
					n64 nFileSize = Ftell(fp);
					Fseek(fp, 0, SEEK_SET);
					static const u32 c_uBufferSize = 0x01000000;
					static vector<u8> c_vBin(c_uBufferSize);
					u32 uCRC32 = 0;
					while (nFileSize > 0)
					{
						u32 uSize = nFileSize >= c_uBufferSize ? c_uBufferSize : static_cast<u32>(nFileSize);
						fread(&*c_vBin.begin(), 1, uSize, fp);
						uCRC32 = UpdateCRC32(&*c_vBin.begin(), uSize, uCRC32);
						nFileSize -= uSize;
					}
					fclose(fp);
					result.RarFile[sFile] = uCRC32;
					UPrintf(USTR("%") PRIUS USTR(" %08x\n"), sFile.c_str(), uCRC32);
				}
			}
			else if (sExtName == USTR("nfo"))
			{
				result.NfoFile.push_back(sFile);
			}
			else if (sExtName == USTR("sfv"))
			{
				result.SfvFile.push_back(sFile);
			}
			else
			{
				if (sExtName.size() == 3)
				{
					if (sExtName[0] >= USTR('r') && sExtName[0] < USTR('|') && sExtName[1] >= USTR('0') && sExtName[1] <= USTR('9') && sExtName[2] >= USTR('0') && sExtName[2] <= USTR('9'))
					{
						if (m_nCheckLevel <= kCheckLevelCRLF)
						{
							result.RarFile[sFile] = 0;
						}
						else
						{
							UString sFilePath = sDirPath + USTR("/") + sFile;
							FILE* fp = UFopen(sFilePath.c_str(), USTR("rb"), false);
							if (fp == nullptr)
							{
								return 1;
							}
							Fseek(fp, 0, SEEK_END);
							n64 nFileSize = Ftell(fp);
							Fseek(fp, 0, SEEK_SET);
							static const u32 c_uBufferSize = 0x01000000;
							static vector<u8> c_vBin(c_uBufferSize);
							u32 uCRC32 = 0;
							while (nFileSize > 0)
							{
								u32 uSize = nFileSize >= c_uBufferSize ? c_uBufferSize : static_cast<u32>(nFileSize);
								fread(&*c_vBin.begin(), 1, uSize, fp);
								uCRC32 = UpdateCRC32(&*c_vBin.begin(), uSize, uCRC32);
								nFileSize -= uSize;
							}
							fclose(fp);
							result.RarFile[sFile] = uCRC32;
							UPrintf(USTR("%") PRIUS USTR(" %08x\n"), sFile.c_str(), uCRC32);
						}
					}
				}
				else
				{
					result.OtherFile.push_back(sFile);
				}
			}
		}
		if (result.NfoFile.size() > 1)
		{
			UPrintf(USTR("nfo COUNT %d > 1: %") PRIUS USTR("\n"), static_cast<n32>(result.NfoFile.size()), sDirPath.c_str());
			for (n32 i = 0; i < static_cast<n32>(result.NfoFile.size()); i++)
			{
				UPrintf(USTR("nfo[%d]: %") PRIUS USTR("\n"), i, result.NfoFile[i].c_str());
			}
			nRowStyle = kStyleIdRed;
		}
		if (result.SfvFile.size() > 1)
		{
			UPrintf(USTR("sfv COUNT %d > 1: %") PRIUS USTR("\n"), static_cast<n32>(result.SfvFile.size()), sDirPath.c_str());
			for (n32 i = 0; i < static_cast<n32>(result.SfvFile.size()); i++)
			{
				UPrintf(USTR("sfv[%d]: %") PRIUS USTR("\n"), i, result.SfvFile[i].c_str());
			}
			nRowStyle = kStyleIdRed;
		}
		if (result.OtherFile.size() > 1)
		{
			UPrintf(USTR("other COUNT %d > 1: %") PRIUS USTR("\n"), static_cast<n32>(result.OtherFile.size()), sDirPath.c_str());
			for (n32 i = 0; i < static_cast<n32>(result.OtherFile.size()); i++)
			{
				UPrintf(USTR("other[%d]: %") PRIUS USTR("\n"), i, result.OtherFile[i].c_str());
			}
			nRowStyle = kStyleIdRed;
		}
		bool bHasXci = false;
		bool bHasNsp = false;
		if (!result.RarFile.empty())
		{
			static UString c_sTempFileName = m_sOutputDirName + USTR("/7zl.txt");
			wstring sCommand = L"\"" + UToW(m_sModuleDirName) + L"/7z.exe\" l " + UToW(sDirPath) + L"/" + UToW(result.RarFile.begin()->first);
			sCommand = Replace(sCommand, L"/", L"\\");
			fflush(stdout);
			FILE* fp = UFreopen(c_sTempFileName.c_str(), USTR("wb+"), stdout);
			if (fp == nullptr)
			{
				return 1;
			}
			n32 nResult = USystem(sCommand.c_str());
			fflush(stdout);
			fclose(fp);
			UFreopen(USTR("CON"), USTR("w"), stdout);
			fp = UFopen(c_sTempFileName.c_str(), USTR("rb"), false);
			if (fp == nullptr)
			{
				return 1;
			}
			fseek(fp, 0, SEEK_END);
			u32 uTempSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			char* pTemp = new char[uTempSize + 1];
			fread(pTemp, 1, uTempSize, fp);
			fclose(fp);
			pTemp[uTempSize] = 0;
			string s7zl = pTemp;
			vector<string> v7zl = SplitOf(s7zl, "\r\n");
			for (vector<string>::iterator itList = v7zl.begin(); itList != v7zl.end(); ++itList)
			{
				*itList = trim(AToU8(*itList));
			}
			vector<string>::const_iterator itList = remove_if(v7zl.begin(), v7zl.end(), empty);
			v7zl.erase(itList, v7zl.end());
			bool bBegin = false;
			for (n32 i = 0; i < static_cast<n32>(v7zl.size()); i++)
			{
				const string& sLine = v7zl[i];
				if (sLine == "------------------- ----- ------------ ------------  ------------------------")
				{
					if (bBegin)
					{
						break;
					}
					else
					{
						bBegin = true;
					}
				}
				else if (bBegin)
				{
					static size_t c_uPos = strlen("------------------- ----- ------------ ------------  ");
					string sFileName = sLine.substr(c_uPos);
					if (EndWith(sFileName, ".xci"))
					{
						bHasXci = true;
					}
					else if (EndWith(sFileName, ".nsp"))
					{
						bHasNsp = true;
					}
				}
			}
		}
		if (sType == L"unknown")
		{
			if (bHasXci)
			{
				sType = L"xci";
			}
			else if (bHasNsp)
			{
				sType = L"nsp";
			}
		}
		else
		{
			if (bHasXci)
			{
				sType = L"unknown";
			}
		}
		wstring sComment;
		UString sPatchDirName = m_sOutputDirName + USTR("/") + WToU(sType + L"/" + sName);
		if (!result.NfoFile.empty())
		{
			UString sFilePath = sDirPath + USTR("/") + result.NfoFile[0];
			STextFileContent textFileContent;
			if (readTextFile(sFilePath, textFileContent) != 0)
			{
				UPrintf(USTR("read text file error: %") PRIUS USTR("\n"), sFilePath.c_str());
				nRowStyle = kStyleIdRed;
				updateOutput(result, nRowStyle, sComment);
				continue;
			}
			if (textFileContent.LineTypeNew == kLineTypeLF)
			{
				sComment += L"/nfo LF";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else if (textFileContent.LineTypeNew == kLineTypeLF_CR)
			{
				sComment += L"/nfo LF|CR";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else if (textFileContent.LineTypeNew == kLineTypeCRLF)
			{
				sComment += L"/nfo CRLF";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else
			{
				sComment += L"/nfo MIX";
				nRowStyle = kStyleIdRed;
			}
			if (textFileContent.TextNew != textFileContent.TextOld || textFileContent.EncodingNew != textFileContent.EncodingOld || textFileContent.LineTypeNew != textFileContent.LineTypeOld)
			{
				UString sPatchFileName = sPatchDirName + USTR("/") + result.NfoFile[0];
				if (writeFileString(sPatchFileName, textFileContent.TextNew) != 0)
				{
					return 1;
				}
			}
		}
		if (!result.SfvFile.empty())
		{
			UString sFilePath = sDirPath + USTR("/") + result.SfvFile[0];
			STextFileContent textFileContent;
			if (readTextFile(sFilePath, textFileContent) != 0)
			{
				UPrintf(USTR("read text file error: %") PRIUS USTR("\n"), sFilePath.c_str());
				nRowStyle = kStyleIdRed;
				updateOutput(result, nRowStyle, sComment.substr(1));
				continue;
			}
			if (textFileContent.LineTypeNew == kLineTypeLF)
			{
				sComment += L"/sfv LF";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else if (textFileContent.LineTypeNew == kLineTypeLF_CR)
			{
				sComment += L"/sfv LF|CR";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else if (textFileContent.LineTypeNew == kLineTypeCRLF)
			{
				sComment += L"/sfv CRLF";
				if (textFileContent.EncodingNew == kEncodingUTF8withBOM)
				{
					sComment += L" with BOM";
				}
			}
			else
			{
				sComment += L"/sfv MIX";
				nRowStyle = kStyleIdRed;
			}
			if (textFileContent.TextNew != textFileContent.TextOld || textFileContent.EncodingNew != textFileContent.EncodingOld || textFileContent.LineTypeNew != textFileContent.LineTypeOld)
			{
				UString sPatchFileName = sPatchDirName + USTR("/") + result.SfvFile[0];
				if (writeFileString(sPatchFileName, textFileContent.TextNew) != 0)
				{
					return 1;
				}
			}
		}
		if (!sComment.empty())
		{
			sComment.erase(0, 1);
		}
		if (m_nCheckLevel <= kCheckLevelCRLF)
		{
			updateOutput(result, nRowStyle, sComment);
			continue;
		}
		bool bCRC32Error = false;
		UString sFilePath = sDirPath + USTR("/") + result.SfvFile[0];
		FILE* fp = UFopen(sFilePath.c_str(), USTR("rb"), false);
		if (fp == nullptr)
		{
			bCRC32Error = true;
		}
		if (bCRC32Error)
		{
			nRowStyle = kStyleIdRed;
			updateOutput(result, nRowStyle, sComment);
			continue;
		}
		fseek(fp, 0, SEEK_END);
		u32 uSfvFileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* pTemp = new char[uSfvFileSize + 1];
		fread(pTemp, 1, uSfvFileSize, fp);
		fclose(fp);
		pTemp[uSfvFileSize] = 0;
		string sSfv = pTemp;
		delete[] pTemp;
		vector<string> vSfv = SplitOf(sSfv, "\r\n");
		for (vector<string>::iterator it = vSfv.begin(); it != vSfv.end(); ++it)
		{
			*it = trim(*it);
		}
		vector<string>::const_iterator itSfv = remove_if(vSfv.begin(), vSfv.end(), empty);
		vSfv.erase(itSfv, vSfv.end());
		for (vector<string>::const_iterator it = vSfv.begin(); it != vSfv.end(); ++it)
		{
			const string& sLine = *it;
			vector<string> vLine = Split(sLine, " ");
			if (vLine.size() != 2)
			{
				bCRC32Error = true;
				break;
			}
			UString sFile = AToU(vLine[0]);
			u32 uCRC32 = SToU32(vLine[1], 16);
			map<UString, u32>::const_iterator itRar = result.RarFile.find(sFile);
			if (itRar == result.RarFile.end())
			{
				bCRC32Error = true;
				break;
			}
			if (uCRC32 != itRar->second)
			{
				bCRC32Error = true;
				break;
			}
		}
		if (bCRC32Error)
		{
			nRowStyle = kStyleIdRed;
			updateOutput(result, nRowStyle, sComment);
			continue;
		}
		updateOutput(result, nRowStyle, sComment);
	}
	for (map<wstring, map<wstring, string>>::const_iterator it = m_mOutput.begin(); it != m_mOutput.end(); ++it)
	{
		UString sType = WToU(it->first);
		const map<wstring, string>& mOutput = it->second;
		UString sOutputFileName = m_sOutputDirName + USTR("/") + sType + USTR(".txt");
		FILE* fp = UFopen(sOutputFileName.c_str(), USTR("wb"), false);
		if (fp == nullptr)
		{
			return 1;
		}
		for (map<wstring, string>::const_iterator itOutput = mOutput.begin(); itOutput != mOutput.end(); ++itOutput)
		{
			fprintf(fp, "%s\r\n", itOutput->second.c_str());
		}
		fclose(fp);
	}
	return 0;
}

bool CSwitchGames::matchGreenStyle(const wstring& a_sName) const
{
	for (vector<wregex>::const_iterator itGreen = m_vGreenStyleNamePattern.begin(); itGreen != m_vGreenStyleNamePattern.end(); ++itGreen)
	{
		if (regex_search(a_sName, *itGreen))
		{
			for (vector<wregex>::const_iterator itNotGreen = m_vNotGreenStyleNamePattern.begin(); itNotGreen != m_vNotGreenStyleNamePattern.end(); ++itNotGreen)
			{
				if (regex_search(a_sName, *itNotGreen))
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

void CSwitchGames::updateOutput(const SResult& a_Result, n32 a_nRowSytle, const wstring& a_sComment)
{
	string sOutput;
	switch (a_nRowSytle)
	{
	case kStyleIdNoneNoBorder:
		sOutput += "nb";
		break;
	case kStyleIdNone:
		sOutput += "no";
		break;
	case kStyleIdRed:
		sOutput += "re";
		break;
	case kStyleIdBlueBold:
		sOutput += "bb";
		break;
	case kStyleIdBlue:
		sOutput += "bl";
		break;
	case kStyleIdGreen:
		sOutput += "gr";
		break;
	case kStyleIdYellow:
		sOutput += "ye";
		break;
	case kStyleIdGold:
		sOutput += "go";
		break;
	default:
		sOutput += "re";
		break;
	}
	sOutput += "\t" + WToU8(a_Result.Name);
	string sFileName;
	if (!a_Result.RarFile.empty())
	{
		sFileName = UToU8(a_Result.RarFile.begin()->first);
		static regex c_rRar("(.*)\\.part\\d+\\.rar", regex_constants::ECMAScript | regex_constants::icase);
		smatch match;
		if (regex_search(sFileName, match, c_rRar))
		{
			sFileName = match[1];
		}
		else
		{
			if (sFileName.size() >= 4)
			{
				sFileName.resize(sFileName.size() - 4);
			}
		}
	}
	else if (!a_Result.NfoFile.empty())
	{
		sFileName = UToU8(a_Result.NfoFile[0]);
		if (sFileName.size() >= 4)
		{
			sFileName.resize(sFileName.size() - 4);
		}
	}
	sOutput += "\t" + sFileName;
	sOutput += "\t" + WToU8(a_sComment) + "\t";
	m_mOutput[a_Result.Type][a_Result.Name] = sOutput;
}

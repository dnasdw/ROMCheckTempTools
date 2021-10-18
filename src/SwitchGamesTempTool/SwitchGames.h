#ifndef SWITCH_GAMES_H_
#define SWITCH_GAMES_H_

#include <sdw.h>

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define UFreopen _wfreopen
#define USystem _wsystem
#else
#define UFreopen freopen
#define USystem system
#endif

class CSwitchGames
{
public:
	enum ECheckLevel
	{
		kCheckLevelName = 0,
		kCheckLevelCRLF = 1,
		kCheckLevelSfv = 2,
		kCheckLevelMax
	};
	enum EStyleId
	{
		kStyleIdNoneNoBorder = 0,
		kStyleIdNone = 1,
		kStyleIdRed = 2,
		kStyleIdBlueBold = 3,
		kStyleIdBlue = 4,
		kStyleIdGreen = 5,
		kStyleIdYellow = 6,
		kStyleIdGold = 7
	};
	enum EEncoding
	{
		kEncodingUnknown,
		kEncodingCP437,
		kEncodingUTF8,
		kEncodingUTF8withBOM
	};
	enum ELineType
	{
		kLineTypeUnknown,
		kLineTypeLF,
		kLineTypeLF_CR,
		kLineTypeLFMix,
		kLineTypeCRLF,
		kLineTypeCRLF_CR,
		kLineTypeCRLFMix,
		kLineTypeCR,
		kLineTypeCRMix
	};
	struct SResult
	{
		n32 Year;
		wstring Name;
		wstring Path;
		bool Exist;
		wstring Type;
		map<UString, u32> RarFile;
		vector<UString> NfoFile;
		vector<UString> SfvFile;
		vector<UString> OtherFile;
	};
	struct STextFileContent
	{
		string TextOld;
		string TextNew;
		EEncoding EncodingOld;
		EEncoding EncodingNew;
		ELineType LineTypeOld;
		ELineType LineTypeNew;
		STextFileContent();
	};
	CSwitchGames();
	~CSwitchGames();
	void SetResultFileName(const UString& a_sResultFileName);
	void SetOutputDirName(const UString& a_sOutputDirName);
	void SetCheckLevel(n32 a_nCheckLevel);
	int Check();
private:
	static string trim(const string& a_sLine);
	static wstring trim(const wstring& a_sLine);
	static bool empty(const string& a_sLine);
	static bool pathCompare(const UString& lhs, const UString& rhs);
	static int readTextFile(const UString& a_sFilePath, STextFileContent& a_TextFileContent);
	static bool makeDir(const UString& a_sDirPath);
	static int writeFileString(const UString& a_sFilePath, const string& a_sStringContent);
	int readConfig();
	int readResult();
	int checkResult();
	bool matchGreenStyle(const wstring& a_sName) const;
	void updateOutput(const SResult& a_Result, n32 a_nRowSytle, const wstring& a_sComment);
	UString m_sResultFileName;
	UString m_sOutputDirName;
	n32 m_nCheckLevel;
	UString m_sModuleDirName;
	vector<wregex> m_vGreenStyleNamePattern;
	vector<wregex> m_vNotGreenStyleNamePattern;
	vector<SResult> m_vResult;
	map<wstring, map<wstring, string>> m_mOutput;
};

#endif	// SWITCH_GAMES_H_

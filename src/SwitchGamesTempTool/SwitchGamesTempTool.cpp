#include "SwitchGames.h"

int checkResult(const UString& a_sResultFileName, const UString& a_sOutputDirName, n32 a_nCheckLevel)
{
	if (a_nCheckLevel <= CSwitchGames::kCheckLevelName)
	{
		return 1;
	}
	if (a_nCheckLevel >= CSwitchGames::kCheckLevelMax)
	{
		return 1;
	}
	CSwitchGames switchGames;
	switchGames.SetResultFileName(a_sResultFileName);
	switchGames.SetOutputDirName(a_sOutputDirName);
	switchGames.SetCheckLevel(a_nCheckLevel);
	return switchGames.Check();
}

int UMain(int argc, UChar* argv[])
{
	if (argc < 3)
	{
		return 1;
	}
	if (UCslen(argv[1]) == 1)
	{
		switch (*argv[1])
		{
		case USTR('C'):
		case USTR('c'):
			if (argc == 4)
			{
				return checkResult(argv[2], argv[3], CSwitchGames::kCheckLevelCRLF);
			}
			else if (argc == 5)
			{
				return checkResult(argv[2], argv[3], SToN32(argv[4]));
			}
			else
			{
				return 1;
			}
		default:
			break;
		}
	}
	return 1;
}

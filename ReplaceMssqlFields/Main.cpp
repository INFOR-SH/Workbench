#include <windows.h>

#include "Technique/Code/MssqlCapturer.h"
#include "Technique/Code/MssqlScanner.h"
#include "Technique/Code/SqlDeclaration.h"
#include "Technique/Code/SqlProcedure.h"
#include "Technique/Code/SqlVariable.h"
#include "Technique/Code/SqlArgument.h"
#include "Technique/Ecotope/String.h"
#include "Technique/Ecotope/Path.h"

using namespace SyteLine::Technique;
using namespace SyteLine::Technique::Code;

struct SFilePath
{
    mstring InputPath;
    mstring OutputPath;

public:
    SFilePath()
    {
    }

    SFilePath(const SFilePath& oThat)
        :InputPath(oThat.InputPath)
        ,OutputPath(oThat.OutputPath)
    {
    }

    SFilePath(MSTRING& sInputPath, MSTRING& sOutputPath)
        :InputPath(sInputPath)
        ,OutputPath(sOutputPath)
    {
    }

    SFilePath(const SFilePath&& oThat)
    {
        *this = move(oThat);
    }

public:
    const SFilePath& operator=(const SFilePath& oRValue)
    {
        InputPath = oRValue.InputPath;
        OutputPath = oRValue.OutputPath;

        return *this;
    }
};

void ContinueWritting(char* csBuffer, ofstream& stlOuptuFileStream)
{
    stlOuptuFileStream.write(csBuffer, strlen(csBuffer));
    stlOuptuFileStream.write("\n", strlen("\n"));
}

bool AddParameter(char* csBuffer, ofstream& stlOuptuFileStream, MSTRING& sInsertParameter)
{
    mstring sLine(csBuffer);
    int nIndex = sLine.find_last_of(")");

    if(nIndex == -1)
    {
        sLine.append("\n");
        stlOuptuFileStream.write(sLine.c_str(), sLine.length());

        stlOuptuFileStream.write(sInsertParameter.c_str(), sInsertParameter.length());
    }
    else
    {
        mstring sFirstWritten = sLine.substr(0, nIndex);
        mstring sLastWritten = sLine.substr(nIndex, sLine.length()-nIndex);

        sFirstWritten.append("\n");
        stlOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());

        stlOuptuFileStream.write(sInsertParameter.c_str(), sInsertParameter.length());

        sLastWritten.append("\n");
        stlOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());
    }

    return true;
}

int ReplaceProcedureName(int nReadLine, char* csBuffer, ofstream& stlOuptuFileStream, CSqlProcedure& oProcedure, MSTRING& sOldProcedureName, MSTRING& sNewProcedureName)
{
    int nResult = 0;
    mstring sLine(csBuffer);
    int nIndex = sLine.find(sOldProcedureName);

    if(nIndex == -1)
    {
        if(nReadLine== oProcedure.EndingLine())
        {
            nResult = -1;
        }
    }
    else
    {
        mstring sFirstWritten = sLine.substr(0, nIndex);
        mstring sLastWritten = sLine.substr(nIndex + sOldProcedureName.length(), sLine.length() - nIndex - sOldProcedureName.length());

        stlOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());
        stlOuptuFileStream.write(sNewProcedureName.c_str(), sNewProcedureName.length());

        sLastWritten.append("\n");
        stlOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());

        nResult = 1;
    }

    return nResult;
}

void AddProcedureArgument(char* csBuffer, ofstream& stlOuptuFileStream, CSqlProcedure& oProcedure, const CSqlArgument& oLastArgument1, MSTRING& sInsertArgument1)
{
    mstring sLine(csBuffer);
    int nIndex = sLine.find_last_of("/");

    if(nIndex == -1)
    {
        sLine.append("\n");
        stlOuptuFileStream.write(sLine.c_str(), sLine.length());

        stlOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());
    }
    else
    {
        mstring sFirstWritten = sLine.substr(0, nIndex);
        mstring sLastWritten = sLine.substr(nIndex, sLine.length()-nIndex);

        sFirstWritten.append("\n");
        stlOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());

        stlOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());

        sLastWritten.append("\n");
        stlOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());
    }
}

bool ProcessFile(CMssqlCapturer& oCapturer, MSTRING& sInputFileName, MSTRING& sOutputFileName)
{
    bool bResult = true;
    mstring sOldProcedureName1("InitSessionContextSp");
    mstring sNewProcedureName1("InitSessionContextWithUserSp");
    mstring sProcedureName2("@EXTGEN_SpName");
    CSqlFile oSqlFile = oCapturer.GetSqlFile();
    auto aParameters = oSqlFile.Declaration().Parameters();
    CSqlProcedure oProcedure1 = oSqlFile.QueryProcedure(sOldProcedureName1);
    CSqlProcedure oProcedure2 = oSqlFile.QueryProcedure(sProcedureName2);
    
    if(oProcedure1.Name().size() == 0)
    {
        sOldProcedureName1 = "dbo.InitSessionContextSp";
        sNewProcedureName1 = "dbo.InitSessionContextWithUserSp";
        oProcedure1 = oSqlFile.QueryProcedure(sOldProcedureName1);

        if(oProcedure1.Name().size() == 0)
        {
            return false;
        }
    }

    CSqlVariable oLastParameter(aParameters.Last());
    CSqlArgument oLastArgument1(oProcedure1.QuoteArguments().Last());
    CSqlArgument oLastArgument2(oProcedure2.QuoteArguments().Last());
    ifstream stlInputFileStream(sInputFileName);
    ofstream stlOuptuFileStream(sOutputFileName, ios::trunc);

    char csBuffer[2048] = {0};
    size_t nReadLine = 1;
    mstring sInsertParameter = "   , @UserId   TokenType   =   NULL\n";
    mstring sInsertArgument1 = "   , @UserName = @UserId\n";
    mstring sInsertArgument2 = "   , @UserId\n";
    int nReplaced = 0;

    while(stlInputFileStream.getline(csBuffer,2048))
    {
        if(nReadLine == oLastParameter.EndingLine())
        {
            AddParameter(csBuffer, stlOuptuFileStream, sInsertParameter);
        }
        else if(nReadLine == oLastArgument2.EndingLine())
        {
            AddProcedureArgument(csBuffer, stlOuptuFileStream, oProcedure2, oLastArgument2, sInsertArgument2);
        }
        else if(nReadLine >= oProcedure1.StartingLine() && nReadLine <= oProcedure1.EndingLine())
        {
            if(nReplaced == 0)
            {
                int nResult = ReplaceProcedureName(nReadLine, csBuffer, stlOuptuFileStream, oProcedure1, sOldProcedureName1, sNewProcedureName1);
                
                if(nResult == 0)
                {
                    ContinueWritting(csBuffer, stlOuptuFileStream);
                }
                else if(nResult == -1)
                {
                    bResult = false;
                    goto RET;
                }
                else
                {
                    nReplaced = 1;
                }
            }
            else if(nReplaced == 1)
            {
                if(nReadLine == oLastArgument1.EndingLine())
                {
                    AddProcedureArgument(csBuffer, stlOuptuFileStream, oProcedure1, oLastArgument1, sInsertArgument1);

                    nReplaced = 2;
                }
                else
                {
                    ContinueWritting(csBuffer, stlOuptuFileStream);
                }
            }
            else
            {
                ContinueWritting(csBuffer, stlOuptuFileStream);
            }
        }
        else
        {
            ContinueWritting(csBuffer, stlOuptuFileStream);
        }

        memset(csBuffer, 0, sizeof(csBuffer));
        nReadLine++;
    }

RET:
    stlInputFileStream.close();
    stlOuptuFileStream.close();

    return bResult;
}

vector<SFilePath> GetFilePaths()
{
    vector<SFilePath> stlFilePaths;

    WIN32_FIND_DATA winFindData;
    wstring sBaseDirectory = L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RptSPs\\";
    wstring sInputDirectory = sBaseDirectory;
    wstring sInputDirectoryFiles(sBaseDirectory + L"*.sp");
    wstring sOutputDirectory(sBaseDirectory + L"Outputs\\");
    HANDLE nFind = INVALID_HANDLE_VALUE;

    nFind = FindFirstFile(sInputDirectoryFiles.c_str(), &winFindData);

    if(INVALID_HANDLE_VALUE != nFind)
    {
        do
        {
            if((winFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                char csInputFilePath[255+1] = {0};
                char csOutputFilePath[255+1] = {0};
                wstring sInputFilePath = sInputDirectory;
                wstring sOutputFilePath = sOutputDirectory;

                sInputFilePath.append(winFindData.cFileName);
                sOutputFilePath.append(winFindData.cFileName);

                wcstombs(csInputFilePath, sInputFilePath.c_str(), sInputFilePath.length());
                wcstombs(csOutputFilePath, sOutputFilePath.c_str(), sOutputFilePath.length());

                stlFilePaths.push_back(SFilePath(csInputFilePath, csOutputFilePath));
            }
        }
        while(FindNextFile(nFind,&winFindData));
    }

    return move(stlFilePaths);
}

void main()
{
    mstring sInputPath("D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RptSPs\\Rpt_JobExceptionSp.sp");
    mstring sOutputPath("D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RptSPs\\Outputs\\Rpt_JobExceptionSp.sp");
    ifstream stlInputStream(sInputPath);
    CMssqlCapturer oCapturer;
    CMssqlScanner oScanner(&stlInputStream);

    oScanner.Flex(&oCapturer);

    stlInputStream.close();

    ProcessFile(oCapturer, sInputPath, sOutputPath);

    /*auto aFilePath = GetFilePaths();
    vector<mstring> stlFailedFiles;
    vector<mstring> stlSuccessfulFiles;

    for(int i = 0; i < aFilePath.size(); i++)
    {
        ifstream stlInputStream(aFilePath[i].InputPath);
        CMssqlCapturer oCapturer;
        CMssqlScanner oScanner(&stlInputStream);

        oScanner.Flex(&oCapturer);

        stlInputStream.close();

        if(ProcessFile(oCapturer, aFilePath[i].InputPath, aFilePath[i].OutputPath) == false)
        {
            stlFailedFiles.push_back(aFilePath[i].InputPath);
        }
        else
        {
            stlSuccessfulFiles.push_back(aFilePath[i].InputPath);
        }
    }

    for(int j = 0; j < stlFailedFiles.size(); j++)
    {
        cout<<"FAIL TO PROCESS: "<<stlFailedFiles[j]<<endl;
    }

    for(int k = 0; k < stlSuccessfulFiles.size(); k++)
    {
        cout<<"Successfully PROCESS: "<<stlSuccessfulFiles[k]<<endl;
    }*/

}
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

    SFilePath(const SFilePath& that)
        :InputPath(that.InputPath)
        ,OutputPath(that.OutputPath)
    {
    }

    SFilePath(MSTRING& sInputPath, MSTRING& sOutputPath)
        :InputPath(sInputPath)
        ,OutputPath(sOutputPath)
    {
    }

    SFilePath(const SFilePath&& that)
    {
        *this = move(that);
    }

public:
    const SFilePath& operator=(const SFilePath& rvalue)
    {
        InputPath = rvalue.InputPath;
        OutputPath = rvalue.OutputPath;

        return *this;
    }
};

void ContinueWritting(char* csBuffer, ofstream& gOuptuFileStream)
{
    gOuptuFileStream.write(csBuffer, strlen(csBuffer));
    gOuptuFileStream.write("\n", strlen("\n"));
}

bool AddParameter(char* csBuffer, ofstream& gOuptuFileStream, MSTRING& sInsertParameter)
{
    mstring sLine(csBuffer);
    int nIndex = sLine.find_last_of(")");

    if(nIndex == -1)
    {
        sLine.append("\n");
        gOuptuFileStream.write(sLine.c_str(), sLine.length());

        gOuptuFileStream.write(sInsertParameter.c_str(), sInsertParameter.length());
    }
    else
    {
        mstring sFirstWritten = sLine.substr(0, nIndex);
        mstring sLastWritten = sLine.substr(nIndex, sLine.length()-nIndex);

        sFirstWritten.append("\n");
        gOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());

        gOuptuFileStream.write(sInsertParameter.c_str(), sInsertParameter.length());

        sLastWritten.append("\n");
        gOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());
    }

    return true;
}

int ReplaceProcedureName(int nReadLine, char* csBuffer, ofstream& gOuptuFileStream, CSqlProcedure& oProcedure, MSTRING& sOldProcedureName, MSTRING& sNewProcedureName)
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

        gOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());
        gOuptuFileStream.write(sNewProcedureName.c_str(), sNewProcedureName.length());

        sLastWritten.append("\n");
        gOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());

        nResult = 1;
    }

    return nResult;
}

void AddProcedureArgument(char* csBuffer, ofstream& gOuptuFileStream, CSqlProcedure& oProcedure, const CSqlArgument& oLastArgument1, MSTRING& sInsertArgument1)
{
    mstring sLine(csBuffer);
    int nIndex = sLine.find_last_of("/");

    if(nIndex == -1)
    {
        sLine.append("\n");
        gOuptuFileStream.write(sLine.c_str(), sLine.length());

        gOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());
    }
    else
    {
        mstring sFirstWritten = sLine.substr(0, nIndex);
        mstring sLastWritten = sLine.substr(nIndex, sLine.length()-nIndex);

        sFirstWritten.append("\n");
        gOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());

        gOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());

        sLastWritten.append("\n");
        gOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());
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
    ifstream gInputFileStream(sInputFileName);
    ofstream gOuptuFileStream(sOutputFileName, ios::trunc);

    char csBuffer[2048] = {0};
    size_t nReadLine = 1;
    mstring sInsertParameter = "   , @UserId   TokenType   =   NULL\n";
    mstring sInsertArgument1 = "   , @UserName = @UserId\n";
    mstring sInsertArgument2 = "   , @UserId\n";
    int nReplaced = 0;

    while(gInputFileStream.getline(csBuffer,2048))
    {
        if(nReadLine == oLastParameter.EndingLine())
        {
            AddParameter(csBuffer, gOuptuFileStream, sInsertParameter);
        }
        else if(nReadLine == oLastArgument2.EndingLine())
        {
            AddProcedureArgument(csBuffer, gOuptuFileStream, oProcedure2, oLastArgument2, sInsertArgument2);
        }
        else if(nReadLine >= oProcedure1.StartingLine() && nReadLine <= oProcedure1.EndingLine())
        {
            if(nReplaced == 0)
            {
                int nResult = ReplaceProcedureName(nReadLine, csBuffer, gOuptuFileStream, oProcedure1, sOldProcedureName1, sNewProcedureName1);
                
                if(nResult == 0)
                {
                    ContinueWritting(csBuffer, gOuptuFileStream);
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
                    AddProcedureArgument(csBuffer, gOuptuFileStream, oProcedure1, oLastArgument1, sInsertArgument1);

                    nReplaced = 2;
                }
                else
                {
                    ContinueWritting(csBuffer, gOuptuFileStream);
                }
            }
            else
            {
                ContinueWritting(csBuffer, gOuptuFileStream);
            }
        }
        else
        {
            ContinueWritting(csBuffer, gOuptuFileStream);
        }

        memset(csBuffer, 0, sizeof(csBuffer));
        nReadLine++;
    }

RET:
    gInputFileStream.close();
    gOuptuFileStream.close();

    return bResult;
}

vector<SFilePath> GetFilePaths()
{
    vector<SFilePath> gFilePaths;

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

                gFilePaths.push_back(SFilePath(csInputFilePath, csOutputFilePath));
            }
        }
        while(FindNextFile(nFind,&winFindData));
    }

    return move(gFilePaths);
}

void main()
{
    mstring sInputPath("D:\\Rpt_JobExceptionSp.sql");
    mstring sOutputPath("D:\\OUTPUT_Rpt_JobExceptionSp.sql");
    ifstream gInputStream(sInputPath);
    CMssqlCapturer oCapturer;
    CMssqlScanner oScanner(&gInputStream);

    oScanner.Flex(&oCapturer);

    gInputStream.close();

    ProcessFile(oCapturer, sInputPath, sOutputPath);

    /*auto aFilePath = GetFilePaths();
    vector<mstring> gFailedFiles;
    vector<mstring> gSuccessfulFiles;

    for(int i = 0; i < aFilePath.size(); i++)
    {
        ifstream gInputStream(aFilePath[i].InputPath);
        CMssqlCapturer oCapturer;
        CMssqlScanner oScanner(&gInputStream);

        oScanner.Flex(&oCapturer);

        gInputStream.close();

        if(ProcessFile(oCapturer, aFilePath[i].InputPath, aFilePath[i].OutputPath) == false)
        {
            gFailedFiles.push_back(aFilePath[i].InputPath);
        }
        else
        {
            gSuccessfulFiles.push_back(aFilePath[i].InputPath);
        }
    }

    for(int j = 0; j < gFailedFiles.size(); j++)
    {
        cout<<"FAIL TO PROCESS: "<<gFailedFiles[j]<<endl;
    }

    for(int k = 0; k < gSuccessfulFiles.size(); k++)
    {
        cout<<"Successfully PROCESS: "<<gSuccessfulFiles[k]<<endl;
    }*/

}
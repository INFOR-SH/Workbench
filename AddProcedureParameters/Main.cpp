#include <windows.h>

#include "Technique/Code/MssqlCapturer.h"
#include "Technique/Code/MssqlScanner.h"

using namespace SyteLine::Technique;
using namespace SyteLine::Technique::Code;

void PrintDuplicatedParametersFiles(const map<wstring, vector<wstring>>& gFiles)
{
    if(!gFiles.empty())
    {
        wcout<<L"[DUPLICATED PARAMETERS FILES WOULD BE IGNORED]"<<endl;

        for(auto a = gFiles.begin(); a != gFiles.end(); a++)
        {
            wcout<<L"File:"<<a->first.c_str()<<L";Parameter:";

            for(int i = 0; i <a->second.size(); i++)
            {
                wcout<<a->second[i]<<L","<<endl;
            }

            wcout<<endl;
        }
    }
}

void PrintDuplicatedProceduresFiles(const vector<wstring>& gFiles)
{
    if(!gFiles.empty())
    {
        wcout<<L"[DUPLICATED PROCEDURES FILES WOULD BE IGNORED]"<<endl;

        for(int i = 0; i < gFiles.size(); i++)
        {
            wcout<<L"File:"<<gFiles[i]<<endl;
        }
    }
}

vector<wstring> CheckDuplicatedParameterName(CSqlDeclaration& oDeclaration, const vector<wstring>& gNames)
{
    vector<wstring> gDuplication;

    for(int i = 0; i < gNames.size(); i++)
    {
        auto aFoundParameters = oDeclaration.QueryParameter(gNames[i]);

        if(aFoundParameters.Size() > 0 )
        {
            gDuplication.push_back(gNames[i]);
        }
    }

    return move(gDuplication);
}

vector<wstring> GetInputPaths(WSTRING& sDirectory, WSTRING& sFilter)
{
    vector<wstring> gPaths;
    WIN32_FIND_DATA winFindData;
    wstring sInputDirectory = sDirectory;
    wstring sFileFilter(sDirectory + sFilter);
    HANDLE nFind = INVALID_HANDLE_VALUE;

    nFind = FindFirstFile(sFileFilter.c_str(), &winFindData);

    if(INVALID_HANDLE_VALUE != nFind)
    {
        do
        {
            if((winFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                char csInputFilePath[255+1] = {0};
                wstring sInputFilePath = sInputDirectory;

                sInputFilePath.append(winFindData.cFileName);

                gPaths.push_back(sInputFilePath);
            }
        }
        while(FindNextFile(nFind,&winFindData));
    }

    return move(gPaths);
}

void AddParameter(wchar_t* wcsBuffer, wofstream& gOuptutStream, WSTRING& sInsertion)
{
    wstring sLine(wcsBuffer);
    int nFound = sLine.find(L")");

    if(nFound == -1)
    {
        sLine.append(L"\n");

        gOuptutStream.write(sLine.c_str(), sLine.length());
        gOuptutStream.write(sInsertion.c_str(), sInsertion.length());
    }
    else
    {
        wstring sFirstWritten = sLine.substr(0, nFound);
        wstring sLastWritten = sLine.substr(nFound, sLine.length()-nFound);

        sFirstWritten.append(L"\n");

        gOuptutStream.write(sFirstWritten.c_str(), sFirstWritten.length());
        gOuptutStream.write(sInsertion.c_str(), sInsertion.length());

        sLastWritten.append(L"\n");

        gOuptutStream.write(sLastWritten.c_str(), sLastWritten.length());
    }
}

int CheckDeclarationRear(WSTRING& sLine, const CSqlVariable& oParameter)
{
    wstring sLowerLine = UString::ToLower(sLine);
    wstring sLastParameterString;

    if(oParameter.Output())
    {
        sLastParameterString = L"output";
    }
    else if(!oParameter.Value().empty())
    {
        sLastParameterString = UString::ToLower(oParameter.Value());
    }
    else
    {
        sLastParameterString = UString::ToLower(oParameter.Type());
    }

    int nFound = sLowerLine.find(sLastParameterString);

    return nFound + sLastParameterString.size();
}

int CheckProcedureRear(WSTRING& sLine, const CSqlArgument& oParameter)
{
    wstring sLowerLine = UString::ToLower(sLine);
    wstring sLastArgumentString;

    if(oParameter.Output())
    {
        sLastArgumentString = L"output";
    }
    else
    {
        sLastArgumentString = UString::ToLower(oParameter.RightValue());
    }

    int nFound = sLowerLine.find(sLastArgumentString);

    return nFound + sLastArgumentString.size();
}

void AddParameters(wofstream& gOutputStream,
                   WSTRING& sLine,
                   const CSqlVariable& oParameter,
                   const map<wstring, wstring>& gInsertion)
{
    int nIndex = CheckDeclarationRear(sLine, oParameter);
    wstring sLineHead = sLine.substr(0, nIndex).append(L"\n");
    wstring sLineRear = sLine.substr(nIndex);

    gOutputStream.write(sLineHead.c_str(), sLineHead.length());

    for(auto a = gInsertion.begin(); a != gInsertion.end(); a++)
    {
        wstring sInsertion = a->second;

        sInsertion.append(L"\n");

        gOutputStream.write(sInsertion.c_str(), sInsertion.length());
    }

    if(sLineRear.length() > 0)
    {
        sLineRear.append(L"\n");

        gOutputStream.write(sLineRear.c_str(), sLineRear.length());
    }
}

void AddArguments(wofstream& gOutputStream,
                  WSTRING& sLine,
                  const CSqlArgument& oArgument,
                  const vector<wstring>& gInsertion)
{
    int nIndex = CheckProcedureRear(sLine, oArgument);
    wstring sLineHead = sLine.substr(0, nIndex).append(L"\n");
    wstring sLineRear = sLine.substr(nIndex);

    gOutputStream.write(sLineHead.c_str(), sLineHead.length());

    for(int i = 0; i < gInsertion.size(); i++)
    {
        wstring sInsertion = gInsertion[i];

        sInsertion.append(L"\n");

        gOutputStream.write(sInsertion.c_str(), sInsertion.length());
    }

    if(sLineRear.length() > 0)
    {
        sLineRear.append(L"\n");

        gOutputStream.write(sLineRear.c_str(), sLineRear.length());
    }
}

bool ProcessFile(CSqlDeclaration& oDeclaration,
                 CSqlProcedure& oProcedure,
                 WSTRING& sInputPath,
                 WSTRING& sOutputPath,
                 const map<wstring, wstring>& gParametersInsertion,
                 const vector<wstring>& gArgumentsInsertion)
{
    CSqlVariable oLastParameter = oDeclaration.QuoteParameters().Last();
    CSqlArgument oLastArgument = oProcedure.QuoteArguments().Last();
    wifstream gInputStream(sInputPath);
    wofstream gOutputStream(sOutputPath, ios::trunc);
    size_t nReadingLine = 1;
    wchar_t wcsBuffer[2048] = {0};
    int nReplaced = 0;
    bool bResult = true;

    while(gInputStream.getline(wcsBuffer, 2048))
    {
        wstring sLine(wcsBuffer);

        if(nReadingLine == oLastParameter.EndingLine())
        {
            AddParameters(gOutputStream, sLine, oLastParameter, gParametersInsertion);
            nReadingLine++;
            continue;
        }
        else if(nReadingLine == oLastArgument.EndingLine())
        {
            AddArguments(gOutputStream, sLine, oLastArgument, gArgumentsInsertion);
            nReadingLine++;
            continue;
        }
        else
        {
            sLine.append(L"\n");
            gOutputStream.write(sLine.c_str(), sLine.length());
        }

        nReadingLine++;
    }

    gInputStream.close();
    gOutputStream.close();

    return bResult;
}

int main(int argc, char* argv[])
{
    wstring sDirectory = L"D:\\";
    map<wstring, wstring> gParametersInertion;
    vector<wstring> gInsertedParameterNames;
    vector<wstring> gArgumentsInsertion;
    vector<wstring> gInputPaths = GetInputPaths(sDirectory, L"*.sql");
    map<wstring, vector<wstring>> gDuplicatedParametersFiles;
    vector<wstring> gDuplicatedProceduresFiles;

    gParametersInertion[L"@UserId"] = L"   , @UserId   TokenType   =   NULL";
    gArgumentsInsertion.push_back(L"         , @UserId");

    for(auto a = gParametersInertion.begin(); a != gParametersInertion.end(); a++)
    {
        gInsertedParameterNames.push_back(a->first);
    }

    for(int i = 0; i < gInputPaths.size(); i++)
    {
        CPath oInputPath(gInputPaths[i]);
        CMssqlCapturer oCapturer;
        ifstream gScannerStream(gInputPaths[i]);
        CMssqlScanner oScanner(&gScannerStream);

        oScanner.Flex(&oCapturer);

        gScannerStream.close();

        CSqlDeclaration oDeclaration = oCapturer.QuoteSqlFile().Declaration();
        auto aDuplication = CheckDuplicatedParameterName(oDeclaration, gInsertedParameterNames);

        if(aDuplication.size() > 0)
        {
            gDuplicatedParametersFiles[oInputPath.FileName()] = aDuplication;

            continue;
        }

        wstring sOutputPath = sDirectory + L"Outputs\\" + oInputPath.FileName();
        const CSqlFile& oFile = oCapturer.QuoteSqlFile();
        CQueried<CSqlProcedure> oProcedures = oFile.QueryProcedure(L"@EXTGEN_SpName");

        if(oProcedures.Size() != 1)
        {
            gDuplicatedProceduresFiles.push_back(oInputPath.FileName());

            continue;
        }

        ProcessFile(oDeclaration,
            oProcedures[0],
            gInputPaths[i],
            sOutputPath,
            gParametersInertion,
            gArgumentsInsertion);

    }

    PrintDuplicatedParametersFiles(gDuplicatedParametersFiles);
    PrintDuplicatedProceduresFiles(gDuplicatedProceduresFiles);

    return 0;
}
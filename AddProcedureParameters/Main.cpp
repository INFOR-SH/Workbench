#include <windows.h>

#include "Technique/Code/MssqlCapturer.h"
#include "Technique/Code/MssqlScanner.h"

using namespace SyteLine::Technique;
using namespace SyteLine::Technique::Code;

typedef map<wstring, vector<wstring>> GDuplicatedFiles;

struct SFilePath
{
    wstring InputPath;
    wstring OutputPath;

public:
    SFilePath()
    {
    }

    SFilePath(const SFilePath& that)
        :InputPath(that.InputPath)
        ,OutputPath(that.OutputPath)
    {
    }

    SFilePath(WSTRING& sInputPath, WSTRING& sOutputPath)
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

void PrintDuplicatedParameterFiles(const GDuplicatedFiles& gFiles)
{
    wcout<<L"[DUPLICATED PARAMETER FILES WOULD BE IGNORED]"<<endl;

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

bool ProcessFile(CSqlDeclaration& oDeclaration, WSTRING& sInputPath, WSTRING& sOutputPath, const vector<wstring>& sInsertions)
{
    CSqlVariable oLastParameter = oDeclaration.QuoteParameters().Last();
    wifstream gInputStream(sInputPath);
    wofstream gOuptutStream(sOutputPath, ios::trunc);
    size_t nReadingLine = 1;
    wchar_t wcsBuffer[2048] = {0};

    while(gInputStream.getline(wcsBuffer,2048))
    {
        if(nReadingLine == oLastParameter.EndingLine())
        {
            wstring sLine(wcsBuffer);
            int nFound = sLine.find(L")");
            /*int nRightBraces = CWStringHelper(sLine).Split(')').size();
            int nLeftBraces = CWStringHelper(sLine).Split('(').size();
            bool bSameLine = nRightBraces>nLeftBraces;*/

            for(int i = 0; i < sInsertions.size(); i++)
            {
                /*if(0 == i && bSameLine)
                {
                    wstring sFirstWritten = sLine.substr(0, nFound);
                    wstring sLastWritten = sLine.substr(nFound, sLine.length()-nFound);

                    sFirstWritten.append("\n");

                    gOuptutStream.write(sFirstWritten.c_str(), sFirstWritten.length());
                    gOuptutStream.write(sInsertions[i].c_str(), sInsertions[i].length());

                    sLastWritten.append("\n");

                    gOuptutStream.write(sLastWritten.c_str(), sLastWritten.length());
                }
                else
                {
                }*/
            }
        }
    }

    gInputStream.close();
    gOuptutStream.close();

    return true;
}

int main(int argc, char* argv[])
{
    /*wstring sInputPath("D:\\Rpt_JobExceptionSp.sql");
    wstring sOutputPath("D:\\OUTPUT_Rpt_JobExceptionSp.sql");
    
    ifstream gInputStream(sInputPath);
    CMssqlCapturer oCapturer;
    CMssqlScanner oScanner(&gInputStream);

    oScanner.Flex(&oCapturer);

    gInputStream.close();*/

    wstring sDirectory = L"D:\\";
    vector<wstring> gInsertions;
    vector<wstring> gInputPaths = GetInputPaths(sDirectory, L"*.sql");
    GDuplicatedFiles gDuplicatedFiles;

    gInsertions.push_back(L"@UserId");

    for(int i = 0; i < gInputPaths.size(); i++)
    {
        CPath oInputPath(gInputPaths[i]);
        CMssqlCapturer oCapturer;
        ifstream gScannerStream(gInputPaths[i]);
        CMssqlScanner oScanner(&gScannerStream);

        oScanner.Flex(&oCapturer);

        gScannerStream.close();

        CSqlDeclaration oDeclaration = oCapturer.QuoteSqlFile().Declaration();
        auto aDuplication = CheckDuplicatedParameterName(oDeclaration, gInsertions);

        if(aDuplication.size() > 0)
        {
            gDuplicatedFiles[oInputPath.FileName()] = aDuplication;

            continue;
        }

        wstring sOutputPath = sDirectory + oInputPath.FileName();

        //ProcessFile(oDeclaration, gInputPaths[i], sOutputPath, gInsertions);

    }

    PrintDuplicatedParameterFiles(gDuplicatedFiles);

    //auto gDuplication = CheckParameterName(oCapturer.GetSqlFile().Declaration());

    //ProcessFile(oCapturer, sInputPath, sOutputPath);

    return 0;
}
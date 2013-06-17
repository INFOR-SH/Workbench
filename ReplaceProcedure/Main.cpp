#include <windows.h>

#include "Technique/Code/MssqlCapturer.h"
#include "Technique/Code/MssqlScanner.h"

using namespace SyteLine::Technique;
using namespace SyteLine::Technique::Code;

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

bool ReplaceProcedureName(int nReadLine,
                         wchar_t* wcsBuffer,
                         wofstream& gOuptuFileStream,
                         const CSqlProcedure& oProcedure,
                         WSTRING& sOldName,
                         WSTRING& sNewName)
{
    bool bResult = false;
    wstring sLine(wcsBuffer);
    int nIndex = sLine.find(sOldName);

    if(nIndex != -1)
    {
        wstring sFirstWritten = sLine.substr(0, nIndex);
        wstring sLastWritten = sLine.substr(nIndex + sOldName.length(), sLine.length() - nIndex - sOldName.length());

        gOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());
        gOuptuFileStream.write(sNewName.c_str(), sNewName.length());

        sLastWritten.append(L"\n");
        gOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());

        bResult = true;
    }

    return bResult;
}

void AddProcedureArgument(wchar_t* wcsBuffer, wofstream& gOuptuFileStream, CSqlProcedure& oProcedure, const CSqlArgument& oLastArgument1, WSTRING& sInsertArgument1)
{
    wstring sLine(wcsBuffer);
    int nIndex = sLine.find(L"/");

    if(nIndex == -1)
    {
        sLine.append(L"\n");
        gOuptuFileStream.write(sLine.c_str(), sLine.length());

        gOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());
    }
    else
    {
        wstring sFirstWritten = sLine.substr(0, nIndex);
        wstring sLastWritten = sLine.substr(nIndex, sLine.length()-nIndex);

        sFirstWritten.append(L"\n");
        gOuptuFileStream.write(sFirstWritten.c_str(), sFirstWritten.length());

        gOuptuFileStream.write(sInsertArgument1.c_str(), sInsertArgument1.length());

        sLastWritten.append(L"\n");
        gOuptuFileStream.write(sLastWritten.c_str(), sLastWritten.length());
    }
}

void PrintMissingProcedureFiles(const vector<wstring>& gFiles)
{
    if(!gFiles.empty())
    {
        wcout<<L"[MISSING PROCEDURE FILE WOULD BE IGNORED]"<<endl;

        for(int i = 0; i < gFiles.size(); i++)
        {
            wcout<<L"File:"<<gFiles[i]<<endl;
        }
    }
}

void main()
{
    string sScannerOutputFile = "./Scanner.output.txt";
    wstring sDirectory = L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RptSPs\\Outputs\\";
    map<wstring, wstring> gParametersInertion;
    vector<wstring> gInputPaths = GetInputPaths(sDirectory, L"*.sp");
    vector<wstring> gProcessedFiles;
    vector<wstring> gMissingProcedureFiles;
    ofstream gScannerOutputStream(sScannerOutputFile, ios::trunc);

    for(int i = 0; i < gInputPaths.size(); i++)
    {
        CPath oInputPath(gInputPaths[i]);
        CMssqlCapturer oCapturer;
        ifstream gScannerStream(gInputPaths[i]);
        CMssqlScanner oScanner(&gScannerStream, &gScannerOutputStream);

        oScanner.Flex(&oCapturer);

        gScannerStream.close();

        wstring sOutputPath = sDirectory + L"Results\\" + oInputPath.FileName();
        const CSqlFile& oFile = oCapturer.QuoteSqlFile();
        wstring sOldSpName(L"InitSessionContextSp");
        wstring sNewSpName(L"InitSessionContextWithUserSp");
        CQueried<CSqlProcedure> aQueried = oFile.QueryProcedure(sOldSpName);
        vector<CSqlProcedure> gProcedures = aQueried.Array();

        if(gProcedures.size() == 0)
        {
            gMissingProcedureFiles.push_back(oInputPath.FileName());
            wcout<<"Ignored:"<<oInputPath.FileName()<<endl;
            continue;
        }
        
        wifstream gInputStream(gInputPaths[i]);
        wofstream gOutputStream(sOutputPath, ios::trunc);
        size_t nReadingLine = 1;
        wchar_t wcsBuffer[2048] = {0};
        bool nReplaced = 0;
        bool bResult = false;
        bool bContinue = false;

        while(gInputStream.getline(wcsBuffer, 2048))
        {
            for(int j = 0; j < gProcedures.size(); j++)
            {
                CSqlArgument oLastArgument = gProcedures[j].QuoteArguments().Last();

                if(nReadingLine >= gProcedures[j].StartingLine() && nReadingLine <= gProcedures[j].EndingLine())
                {
                    bContinue = false;

                    if(bResult == false)
                    {
                        bResult = ReplaceProcedureName(nReadingLine,
                            wcsBuffer,
                            gOutputStream,
                            gProcedures[j],
                            L"InitSessionContextSp",
                            L"InitSessionContextWithUserSp");

                        if(bResult == false)
                        {
                            gOutputStream.write(wcsBuffer, wcslen(wcsBuffer));
                            gOutputStream.write(L"\n", wcslen(L"\n"));
                        }
                    }
                    else
                    {
                        if(nReadingLine == oLastArgument.EndingLine())
                        {
                            AddProcedureArgument(wcsBuffer, gOutputStream, gProcedures[j], oLastArgument, L"   , @UserName = @BGUserId\n");
                            bResult = false;
                        }
                        else
                        {
                            gOutputStream.write(wcsBuffer, wcslen(wcsBuffer));
                            gOutputStream.write(L"\n", wcslen(L"\n"));
                        }
                    }
                }
                else
                {
                    bContinue = true;
                }
            }

            if(bContinue)
            {
                gOutputStream.write(wcsBuffer, wcslen(wcsBuffer));
                gOutputStream.write(L"\n", wcslen(L"\n"));
            }

            nReadingLine++;
        }

        gInputStream.close();
        gOutputStream.close();

        gProcessedFiles.push_back(oInputPath.FileName());
        wcout<<"Processed:"<<oInputPath.FileName()<<endl;
    }
    gScannerOutputStream.close();

    PrintMissingProcedureFiles(gMissingProcedureFiles);

    wcout<<"Total Processed:"<<gProcessedFiles.size()<<" Total Ignored:"<<gMissingProcedureFiles.size()<<endl;
}
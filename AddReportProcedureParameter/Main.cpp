#include <windows.h>

#include "Technique/Ecotope/Base.h"
#include "Technique/Ecotope/Path.h"

using namespace SyteLine::Technique;

struct SProcedure
{
    wstring Name;
    wstring Path;
    size_t InsertPoint;

    SProcedure()
    {}

    SProcedure(const SProcedure& oThat)
        :Name(oThat.Name)
        ,Path(oThat.Path)
        ,InsertPoint(oThat.InsertPoint)
    {}

    SProcedure(WSTRING& sPath, WSTRING& sName, size_t nInsertPoint)
        :Name(sName)
        ,Path(sPath)
        ,InsertPoint(nInsertPoint)
    {
    }

    const SProcedure& operator=(const SProcedure& oRValue)
    {
        Name = oRValue.Name;
        Path = oRValue.Path;
        InsertPoint = oRValue.InsertPoint;

        return *this;
    }
};

vector<wstring> GetFilePaths()
{
    vector<wstring> stlPaths;

    WIN32_FIND_DATA winFindData;
    wstring sDirectory = L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RPT_RDLs\\";
    
    wstring sDirectoryFiles(sDirectory + L"*.rdl");
    HANDLE nFind = INVALID_HANDLE_VALUE;

    nFind = FindFirstFile(sDirectoryFiles.c_str(), &winFindData);

    if(INVALID_HANDLE_VALUE != nFind)
    {
        do
        {
            if((winFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
               
                char csFilePath[255+1] = {0};
                wstring sFilePath = sDirectory;

                sFilePath.append(winFindData.cFileName);

                
                stlPaths.push_back(sFilePath);
            }
        }
        while(FindNextFile(nFind,&winFindData));
    }

    return move(stlPaths);
}

SProcedure FindDatssetProcedueName(WSTRING& sFilePath)
{
    SProcedure oResult;
    wifstream stlInputStream(sFilePath);
    wchar_t wcsBuffer[1024] = {0};
    size_t nReadLine = 1;
    size_t nDatasetLine = 0;
    size_t nEndParameterLine = 0;
    size_t nDatssetProcedureLine = 0;
    wstring sHead = L"<CommandText>";
    wstring sRear = L"</CommandText>";

    while(stlInputStream.getline(wcsBuffer,1024))
    {
        wstring sLine(wcsBuffer);

        if(nDatasetLine == 0)
        {
            if(sLine.find(L"<DataSet") != -1)
            {
                nDatasetLine = nReadLine;
            }
        }
        else
        {
            if(sLine.find(L"</QueryParameter>") != -1)
            {
                nEndParameterLine = nReadLine;
            }

            int nHeadIndex = sLine.find(sHead);

            if(nHeadIndex != -1)
            {
                wstring sTemp = sLine.substr(nHeadIndex+sHead.length(), sLine.length()-nHeadIndex);
                int nRearIndex = sTemp.find(sRear);

                if(nRearIndex != -1)
                {
                    sTemp = sTemp.substr(0, nRearIndex);
                }

                transform(sTemp.begin(), sTemp.end(), sTemp.begin(), tolower);

                if(sTemp.substr(0, 4) == L"rpt_")
                {
                    oResult.Name = sTemp;
                    oResult.Path = sFilePath;
                    oResult.InsertPoint = nEndParameterLine;
                }

                break;
            }
        }

        memset(wcsBuffer, 0, sizeof(wcsBuffer));
        nReadLine++;
    }

    stlInputStream.close();

    return oResult;
}

vector<wstring> ReadKeys(wstring sFilePath)
{
    vector<wstring> stlResult;
    wifstream stlInputStream(sFilePath);
    wchar_t wcsBuffer[256] = {0};

    while(stlInputStream.getline(wcsBuffer,256))
    {
        wstring sKey(wcsBuffer);
        transform(sKey.begin(), sKey.end(), sKey.begin(), tolower);

        stlResult.push_back(sKey);
    }

    stlInputStream.close();

    return move(stlResult);
}

vector<SProcedure> GetRequiredRDLs(const vector<wstring>& stlPaths)
{
    wstring sKeysFilePath(L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RPT_RDLs\\Keys.txt");
    map<wstring, SProcedure> stlAllMapping;
    vector<SProcedure> stlRequiredProcedures;

    for(int i = 0; i <stlPaths.size(); i++)
    {
        SProcedure oProcedure = FindDatssetProcedueName(stlPaths[i]);

        if(oProcedure.Name.length() > 0)
        {
            stlAllMapping[oProcedure.Name] = oProcedure;
        }
    }

    vector<wstring> stlKeys = ReadKeys(sKeysFilePath);
    vector<wstring> stlFailToFindKeys;

    for(int i = 0; i < stlKeys.size(); i++)
    {
        auto a = stlAllMapping.find(stlKeys[i]);

        if(a != stlAllMapping.end())
        {
            stlRequiredProcedures.push_back(a->second);
            wcout<<a->second.Path<<endl;
        }
        else
        {
            stlFailToFindKeys.push_back(stlKeys[i]);
        }
    }

    for(int j = 0; j < stlFailToFindKeys.size(); j++)
    {
        wcout<<L"Fail to find the key in RDL file:"<<stlFailToFindKeys[j]<<endl;
    }

    return move(stlRequiredProcedures);
}

void AddParameter(vector<SProcedure>& oProcedures)
{
    wstring sOutputDirectory = L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RPT_RDLs\\Outputs\\";
    wstring sInserted =
        L"         <QueryParameter Name=\"@UserId\">\n"\
        L"            <Value>=Parameters!BG_USERID.Value<\/Value>\n"\
        L"            <rd:UserDefined>true<\/rd:UserDefined>\n"\
        L"          <\/QueryParameter>\n";

    for(auto a = oProcedures.begin(); a != oProcedures.end(); a++)
    {
        size_t nReadLine = 1;
        CPath oPath(a->Path);
        wstring sOutputPath = sOutputDirectory;

        sOutputPath.append(oPath.FileName());

        wifstream stlInputStream(a->Path);
        wofstream stlOutputStream(sOutputPath, ios::trunc);
        wchar_t wcsBuffer[4096] = {0};
        bool bDone = false;

        while(stlInputStream.getline(wcsBuffer,4096))
        {
            if(nReadLine == a->InsertPoint + 1 && bDone == false)
            {
                stlOutputStream.write(sInserted.c_str(), sInserted.length());
                
                bDone = true;
            }
            
            stlOutputStream.write(wcsBuffer, wcslen(wcsBuffer));
            stlOutputStream.write(L"\n", wcslen(L"\n"));

            memset(wcsBuffer, 0, sizeof(wcsBuffer));
            nReadLine++;
        }

        stlInputStream.close();
        stlOutputStream.close();

        wcout<<"Finished file:"<<sOutputPath.c_str()<<endl;
    }
}

void main(int argc, wchar_t* argv[])
{
    vector<wstring> stlPaths = GetFilePaths();
    auto aProcedures = GetRequiredRDLs(stlPaths);

    wcout<<L"Total "<<aProcedures.size()<<L" files need to be processed"<<endl;

    //AddParameter(aProcedures);

    return;
}
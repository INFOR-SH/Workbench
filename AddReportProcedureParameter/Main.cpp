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

    SProcedure(const SProcedure& that)
        :Name(that.Name)
        ,Path(that.Path)
        ,InsertPoint(that.InsertPoint)
    {}

    SProcedure(WSTRING& sPath, WSTRING& sName, size_t nInsertPoint)
        :Name(sName)
        ,Path(sPath)
        ,InsertPoint(nInsertPoint)
    {
    }

    const SProcedure& operator=(const SProcedure& rvalue)
    {
        Name = rvalue.Name;
        Path = rvalue.Path;
        InsertPoint = rvalue.InsertPoint;

        return *this;
    }
};

vector<wstring> GetFilePaths()
{
    vector<wstring> gPaths;

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

                
                gPaths.push_back(sFilePath);
            }
        }
        while(FindNextFile(nFind,&winFindData));
    }

    return move(gPaths);
}

SProcedure FindDatssetProcedueName(WSTRING& sFilePath)
{
    SProcedure oResult;
    wifstream gInputStream(sFilePath);
    wchar_t wcsBuffer[1024] = {0};
    size_t nReadLine = 1;
    size_t nDatasetLine = 0;
    size_t nEndParameterLine = 0;
    size_t nDatssetProcedureLine = 0;
    wstring sHead = L"<CommandText>";
    wstring sRear = L"</CommandText>";

    while(gInputStream.getline(wcsBuffer,1024))
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

    gInputStream.close();

    return oResult;
}

vector<wstring> ReadKeys(wstring sFilePath)
{
    vector<wstring> gResult;
    wifstream gInputStream(sFilePath);
    wchar_t wcsBuffer[256] = {0};

    while(gInputStream.getline(wcsBuffer,256))
    {
        wstring sKey(wcsBuffer);
        transform(sKey.begin(), sKey.end(), sKey.begin(), tolower);

        gResult.push_back(sKey);
    }

    gInputStream.close();

    return move(gResult);
}

vector<SProcedure> GetRequiredRDLs(const vector<wstring>& gPaths)
{
    wstring sKeysFilePath(L"D:\\INFOR\\StyeLine\\Issues\\TRK142850\\RPT_RDLs\\Keys.txt");
    map<wstring, SProcedure> gAllMapping;
    vector<SProcedure> gRequiredProcedures;

    for(int i = 0; i <gPaths.size(); i++)
    {
        SProcedure oProcedure = FindDatssetProcedueName(gPaths[i]);

        if(oProcedure.Name.length() > 0)
        {
            gAllMapping[oProcedure.Name] = oProcedure;
        }
    }

    vector<wstring> gKeys = ReadKeys(sKeysFilePath);
    vector<wstring> gFailToFindKeys;

    for(int i = 0; i < gKeys.size(); i++)
    {
        auto a = gAllMapping.find(gKeys[i]);

        if(a != gAllMapping.end())
        {
            gRequiredProcedures.push_back(a->second);
            wcout<<a->second.Path<<endl;
        }
        else
        {
            gFailToFindKeys.push_back(gKeys[i]);
        }
    }

    for(int j = 0; j < gFailToFindKeys.size(); j++)
    {
        wcout<<L"Fail to find the key in RDL file:"<<gFailToFindKeys[j]<<endl;
    }

    return move(gRequiredProcedures);
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

        wifstream gInputStream(a->Path);
        wofstream gOutputStream(sOutputPath, ios::trunc);
        wchar_t wcsBuffer[4096] = {0};
        bool bDone = false;

        while(gInputStream.getline(wcsBuffer,4096))
        {
            if(nReadLine == a->InsertPoint + 1 && bDone == false)
            {
                gOutputStream.write(sInserted.c_str(), sInserted.length());
                
                bDone = true;
            }
            
            gOutputStream.write(wcsBuffer, wcslen(wcsBuffer));
            gOutputStream.write(L"\n", wcslen(L"\n"));

            memset(wcsBuffer, 0, sizeof(wcsBuffer));
            nReadLine++;
        }

        gInputStream.close();
        gOutputStream.close();

        wcout<<"Finished file:"<<sOutputPath.c_str()<<endl;
    }
}

void main(int argc, wchar_t* argv[])
{
    vector<wstring> gPaths = GetFilePaths();
    auto aProcedures = GetRequiredRDLs(gPaths);

    wcout<<L"Total "<<aProcedures.size()<<L" files need to be processed"<<endl;

    //AddParameter(aProcedures);

    return;
}
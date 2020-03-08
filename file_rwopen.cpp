/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2020 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef PGE_FILES_QT
#include <QFileInfo>
#include <QDir>
#endif

#include "file_formats.h"
#include "pge_file_lib_private.h"

bool FileFormats::OpenLevelFile(PGESTRING filePath, LevelData &FileData)
{
    PGE_FileFormats_misc::TextFileInput file;
    if(!file.open(filePath, true))
    {
        FileData.meta.ReadFileValid = false;
        FileData.meta.ERROR_info = "Can't open file";
        FileData.meta.ERROR_linedata = "";
        FileData.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenLevelFileT(file, FileData);
}

bool FileFormats::OpenLevelRaw(PGESTRING &rawdata, PGESTRING filePath, LevelData &FileData)
{
    PGE_FileFormats_misc::RawTextInput file;
    if(!file.open(&rawdata, filePath))
    {
        FileData.meta.ReadFileValid = false;
        FileData.meta.ERROR_info = "Can't open file";
        FileData.meta.ERROR_linedata = "";
        FileData.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenLevelFileT(file, FileData);
}

bool FileFormats::OpenLevelFileT(PGE_FileFormats_misc::TextInput &file, LevelData &FileData)
{
    PGESTRING firstLine;

    FileData.meta.ERROR_info.clear();
    firstLine = file.read(8);
    file.seek(0, PGE_FileFormats_misc::TextInput::begin);

    if(PGE_StartsWith(firstLine, "SMBXFile"))
    {
        //Read SMBX65-38A LVL File
        if(!ReadSMBX38ALvlFile(file, FileData))
            return false;
    }
    else if(PGE_FileFormats_misc::PGE_DetectSMBXFile(firstLine))
    {
        //Disable UTF8 for SMBX64 files
        if(!file.reOpen(false))
            return false;
        //Read SMBX LVL File
        if(!ReadSMBX64LvlFile(file, FileData))
            return false;
    }
    else
    {
        //Read PGE LVLX File
        if(!ReadExtendedLvlFile(file, FileData))
            return false;
    }

    if(PGE_FileFormats_misc::TextFileInput::exists(file.getFilePath() + ".meta"))
    {
        if(!ReadNonSMBX64MetaDataF(file.getFilePath() + ".meta", FileData.metaData))
            FileData.meta.ERROR_info = "Can't open meta-file";
    }

    return true;
}

bool FileFormats::OpenLevelFileHeader(PGESTRING filePath, LevelData &data)
{
    PGE_FileFormats_misc::TextFileInput file;
    data.meta.ERROR_info.clear();

    if(!file.open(filePath, true))
    {
        data.meta.ReadFileValid = false;
        data.meta.ERROR_info = "Can't open file";
        data.meta.ERROR_linedata = "";
        data.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenLevelFileHeaderT(file, data);
}

bool FileFormats::OpenLevelFileHeaderRaw(PGESTRING &rawdata, PGESTRING filePath, LevelData &data)
{
    PGE_FileFormats_misc::RawTextInput file;
    data.meta.ERROR_info.clear();

    if(!file.open(&rawdata, filePath))
    {
        data.meta.ReadFileValid = false;
        data.meta.ERROR_info = "Can't open file";
        data.meta.ERROR_linedata = "";
        data.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenLevelFileHeaderT(file, data);
}

bool FileFormats::OpenLevelFileHeaderT(PGE_FileFormats_misc::TextInput &file, LevelData &data)
{
    PGESTRING firstLine;
    firstLine = file.readLine();
    file.seek(0, PGE_FileFormats_misc::TextInput::begin);

    if(PGE_StartsWith(firstLine, "SMBXFile"))
    {
        //Read SMBX65-38A LVL File
        return ReadSMBX38ALvlFileHeaderT(file, data);
    }
    else if(PGE_FileFormats_misc::PGE_DetectSMBXFile(firstLine))
    {
        //Disable UTF8 for SMBX64 files
        if(!file.reOpen(false))
            return false;
        //Read SMBX LVL File
        return ReadSMBX64LvlFileHeaderT(file, data);
    }
    else
    {
        //Read PGE LVLX File
        return ReadExtendedLvlFileHeaderT(file, data);
    }
}


bool FileFormats::SaveLevelFile(LevelData &FileData, PGESTRING filePath, LevelFileFormat format, unsigned int FormatVersion)
{
    FileData.meta.ERROR_info.clear();
    switch(format)
    {
    case LVL_PGEX:
    {
        smbx64CountStars(FileData);
        if(!FileFormats::WriteExtendedLvlFileF(filePath, FileData))
        {
            FileData.meta.ERROR_info += "Cannot save file " + filePath + ".";
            return false;
        }
        return true;
    }
    //break;
    case LVL_SMBX64:
    {
        //Apply SMBX64-specific things to entire array
        smbx64LevelPrepare(FileData);

        if(!FileFormats::WriteSMBX64LvlFileF(filePath, FileData, FormatVersion))
        {
            FileData.meta.ERROR_info += "Cannot save file " + filePath + ".";
            return false;
        }

        //save additional meta data
        if(!FileData.metaData.bookmarks.empty())
        {
            if(!FileFormats::WriteNonSMBX64MetaDataF(filePath + ".meta", FileData.metaData))
            {
                FileData.meta.ERROR_info += "Cannot save file " + filePath + ".meta.";
                return false;
            }
        }
        return true;
    }
    //break;
    case LVL_SMBX38A:
    {
        if(!FileFormats::WriteSMBX38ALvlFileF(filePath, FileData))
        {
            FileData.meta.ERROR_info = "Cannot save file " + filePath + ".";
            return false;
        }
        return true;
    }
        //break;
    }
    FileData.meta.ERROR_info = "Unsupported file type";
    return false;
}

bool FileFormats::SaveLevelData(LevelData &FileData, PGESTRING &RawData, LevelFileFormat format, unsigned int FormatVersion)
{
    FileData.meta.ERROR_info.clear();
    switch(format)
    {
    case LVL_PGEX:
    {
        smbx64CountStars(FileData);
        WriteExtendedLvlFileRaw(FileData, RawData);
        return true;
    }
    //break;
    case LVL_SMBX64:
    {
        smbx64LevelPrepare(FileData);
        WriteSMBX64LvlFileRaw(FileData, RawData, FormatVersion);
        return true;
    }
    //break;
    case LVL_SMBX38A:
    {
        FileFormats::WriteSMBX38ALvlFileRaw(FileData, RawData);
        return true;
    }
        //break;
    }
    FileData.meta.ERROR_info = "Unsupported file type";
    FileData.meta.ReadFileValid = false;
    return false;
}



bool FileFormats::OpenWorldFile(PGESTRING filePath, WorldData &data)
{
    PGE_FileFormats_misc::TextFileInput file;
    if(!file.open(filePath, true))
    {
        data.meta.ReadFileValid = false;
        data.meta.ERROR_info = "Can't open file";
        data.meta.ERROR_linedata = "";
        data.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenWorldFileT(file, data);
}

bool FileFormats::OpenWorldRaw(PGESTRING &rawdata, PGESTRING filePath, WorldData &FileData)
{
    PGE_FileFormats_misc::RawTextInput file;
    if(!file.open(&rawdata, filePath))
    {
        FileData.meta.ReadFileValid = false;
        FileData.meta.ERROR_info = "Can't open file";
        FileData.meta.ERROR_linedata = "";
        FileData.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenWorldFileT(file, FileData);
}

bool FileFormats::OpenWorldFileT(PGE_FileFormats_misc::TextInput &file, WorldData &data)
{
    PGESTRING firstLine;

    data.meta.ERROR_info.clear();
    firstLine = file.read(8);
    file.seek(0, PGE_FileFormats_misc::TextInput::begin);

    if(PGE_StartsWith(firstLine, "SMBXFile"))
    {
        //Read SMBX-38A WLD File
        if(!ReadSMBX38AWldFile(file, data))
            return false;
    }
    else if(PGE_FileFormats_misc::PGE_DetectSMBXFile(firstLine))
    {
        //Disable UTF8 for SMBX64 files
        if(!file.reOpen(false))
            return false;
        //Read SMBX WLD File
        if(!ReadSMBX64WldFile(file, data))
            return false;
    }
    else
    {
        //Read PGE WLDX File
        if(!ReadExtendedWldFile(file, data))
            return false;
    }

    if(PGE_FileFormats_misc::TextFileInput::exists(file.getFilePath() + ".meta"))
    {
        if(!ReadNonSMBX64MetaDataF(file.getFilePath() + ".meta", data.metaData))
            data.meta.ERROR_info = "Can't open meta-file";
    }

    return true;
}

bool FileFormats::OpenWorldFileHeader(PGESTRING filePath, WorldData &data)
{
    PGE_FileFormats_misc::TextFileInput file;
    data.meta.ERROR_info.clear();

    if(!file.open(filePath, true))
    {
        data.meta.ReadFileValid = false;
        data.meta.ERROR_info = "Can't open file";
        data.meta.ERROR_linedata = "";
        data.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenWorldFileHeaderT(file, data);
}

bool FileFormats::OpenWorldFileHeaderRaw(PGESTRING &rawdata, PGESTRING filePath, WorldData &data)
{
    PGE_FileFormats_misc::RawTextInput file;
    data.meta.ERROR_info.clear();

    if(!file.open(&rawdata, filePath))
    {
        data.meta.ReadFileValid = false;
        data.meta.ERROR_info = "Can't open file";
        data.meta.ERROR_linedata = "";
        data.meta.ERROR_linenum = -1;
        return false;
    }
    return OpenWorldFileHeaderT(file, data);
}

bool FileFormats::OpenWorldFileHeaderT(PGE_FileFormats_misc::TextInput &file, WorldData &data)
{
    PGESTRING firstLine;
    firstLine = file.readLine();
    file.seek(0, PGE_FileFormats_misc::TextInput::begin);

    if(PGE_StartsWith(firstLine, "SMBXFile"))
    {
        //Read SMBX-38A WLD File
        return ReadSMBX38AWldFileHeaderT(file, data);
    }
    else if(PGE_FileFormats_misc::PGE_DetectSMBXFile(firstLine))
    {
        //Disable UTF8 for SMBX64 files
        if(!file.reOpen(false))
            return false;
        //Read SMBX WLD File
        return ReadSMBX64WldFileHeaderT(file, data);
    }
    else
    {
        //Read PGE WLDX File
        return ReadExtendedWldFileHeaderT(file, data);
    }
}

bool FileFormats::SaveWorldFile(WorldData &FileData, PGESTRING filePath, FileFormats::WorldFileFormat format, unsigned int FormatVersion)
{
    FileData.meta.ERROR_info.clear();
    switch(format)
    {
    case WLD_PGEX:
    {
        if(!FileFormats::WriteExtendedWldFileF(filePath, FileData))
        {
            FileData.meta.ERROR_info += "Cannot save file " + filePath + ".";
            return false;
        }
        return true;
    }
    //break;
    case WLD_SMBX64:
    {
        if(!FileFormats::WriteSMBX64WldFileF(filePath, FileData, FormatVersion))
        {
            FileData.meta.ERROR_info += "Cannot save file " + filePath + ".";
            return false;
        }

        //save additional meta data
        if(!FileData.metaData.bookmarks.empty())
        {
            if(!FileFormats::WriteNonSMBX64MetaDataF(filePath + ".meta", FileData.metaData))
            {
                FileData.meta.ERROR_info += "Cannot save file " + filePath + ".meta.";
                return false;
            }
        }
        return true;
    }
    //break;
    case WLD_SMBX38A:
    {
        if(!FileFormats::WriteSMBX38AWldFileF(filePath, FileData))
        {
            FileData.meta.ERROR_info += "Cannot save file " + filePath + ".";
            return false;
        }
        return true;
    }
        //break;
    }
    FileData.meta.ERROR_info = "Unsupported file type";
    return false;
}

bool FileFormats::SaveWorldData(WorldData &FileData, PGESTRING &RawData, FileFormats::WorldFileFormat format, unsigned int FormatVersion)
{
    FileData.meta.ERROR_info.clear();
    switch(format)
    {
    case WLD_PGEX:
    {
        WriteExtendedWldFileRaw(FileData, RawData);
        return true;
    }
    //break;
    case WLD_SMBX64:
    {
        WriteSMBX64WldFileRaw(FileData, RawData, FormatVersion);
        return true;
    }
    //break;
    case WLD_SMBX38A:
    {
        WriteSMBX38AWldFileRaw(FileData, RawData);
        return true;
    }
        //break;
    }
    FileData.meta.ERROR_info = "Unsupported file type";
    return false;
}

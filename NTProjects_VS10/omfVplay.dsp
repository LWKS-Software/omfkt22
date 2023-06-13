# Microsoft Developer Studio Project File - Name="omfVplay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=omfVplay - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "omfVplay.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "omfVplay.mak" CFG="omfVplay - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "omfVplay - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "omfVplay - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "omfVplay - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\ReleaseOutput"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "..\include" /I "..\portinc" /I "..\kitomfi" /I "..\avidjpg" /I "..\bento" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "OMFI_NEED_ULONG" /D "HAVE_STDC" /D AVID_CODEC_SUPPORT=1 /D "OMF_USE_DLL" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "omfVplay - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Output"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\bento" /I "..\include" /I "..\portinc" /I "..\kitomfi" /I "..\avidjpg" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "OMFI_NEED_ULONG" /D "HAVE_STDC" /D AVID_CODEC_SUPPORT=1 /D "OMF_USE_DLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /incremental:no

!ENDIF 

# Begin Target

# Name "omfVplay - Win32 Release"
# Name "omfVplay - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\apps\omfVplayNT\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVplay.cpp
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVplayDoc.cpp
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVPlayView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\apps\omfVplayNT\MainFrm.h
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVplay.h
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVplayDoc.h
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVPlayView.h
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\apps\omfVplayNT\res\omfVplay.ico
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\omfVplay.rc
# End Source File
# Begin Source File

SOURCE=..\apps\omfVplayNT\res\omfVplayDoc.ico
# End Source File
# End Group
# End Target
# End Project

# Microsoft Developer Studio Project File - Name="unittest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=unittest - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak" CFG="unittest - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unittest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unittest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unittest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WinRel"
# PROP BASE Intermediate_Dir ".\WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\ReleaseOutput"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\portinc" /I "..\compat" /I "..\kitomfi" /I "..\avidjpg" /I "..\bento" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "OMFI_NEED_ULONG" /D "MAKE_TEST_HARNESS" /D "HAVE_STDC" /D AVID_CODEC_SUPPORT=1 /D "OMF_USE_DLL" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "unittest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WinDebug"
# PROP BASE Intermediate_Dir ".\WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Output"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\portinc" /I "..\compat" /I "..\kitomfi" /I "..\avidjpg" /I "..\bento" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "OMFI_NEED_ULONG" /D "MAKE_TEST_HARNESS" /D "HAVE_STDC" /D AVID_CODEC_SUPPORT=1 /D "OMF_USE_DLL" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "unittest - Win32 Release"
# Name "unittest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\unittest\ChgMob.c
# End Source File
# Begin Source File

SOURCE=..\unittest\CloneX.c
# End Source File
# Begin Source File

SOURCE=..\unittest\CompIter.c
# End Source File
# Begin Source File

SOURCE=..\unittest\Convert.c
# End Source File
# Begin Source File

SOURCE=..\unittest\CopyMob.c
# End Source File
# Begin Source File

SOURCE=..\unittest\CopyMobX.c
# End Source File
# Begin Source File

SOURCE=..\unittest\DelMob.c
# End Source File
# Begin Source File

SOURCE=..\unittest\DumpDK.c
# End Source File
# Begin Source File

SOURCE=..\unittest\DumpFXD.c
# End Source File
# Begin Source File

SOURCE=..\unittest\DupMobs.c
# End Source File
# Begin Source File

SOURCE=..\unittest\ERCvt.c
# End Source File
# Begin Source File

SOURCE=..\unittest\FndSrc.c
# End Source File
# Begin Source File

SOURCE=..\unittest\IterSrch.c
# End Source File
# Begin Source File

SOURCE=..\unittest\ManyObjs.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkComp2x.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkCplx.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkDesc.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkFilm.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkIlleg.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkMedia.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkOMFLoc.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkRawLoc.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkSimpFX.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkSimple.c
# End Source File
# Begin Source File

SOURCE=..\unittest\MkTape.c
# End Source File
# Begin Source File

SOURCE=..\unittest\NgCloneX.c
# End Source File
# Begin Source File

SOURCE=..\unittest\NgFndSrc.c
# End Source File
# Begin Source File

SOURCE=..\unittest\ObjIter.c
# End Source File
# Begin Source File

SOURCE=..\unittest\OpenMod.c
# End Source File
# Begin Source File

SOURCE=..\unittest\Patch1x.c
# End Source File
# Begin Source File

SOURCE=..\unittest\PrivReg.c
# End Source File
# Begin Source File

SOURCE=..\unittest\ReadComp.c
# End Source File
# Begin Source File

SOURCE=..\unittest\TCCvt.c
# End Source File
# Begin Source File

SOURCE=..\unittest\tstattr.c
# End Source File
# Begin Source File

SOURCE=..\unittest\TstCodec.c
# End Source File
# Begin Source File

SOURCE=..\unittest\UnitTest.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\unittest\MkMed1x.h
# End Source File
# Begin Source File

SOURCE=..\unittest\MkMedia.h
# End Source File
# Begin Source File

SOURCE=..\unittest\MkTape.h
# End Source File
# Begin Source File

SOURCE=..\unittest\UnitTest.h
# End Source File
# End Group
# End Target
# End Project

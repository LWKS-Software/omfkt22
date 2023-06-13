# Microsoft Developer Studio Project File - Name="omfToolkit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=omfToolkit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "omfToolkit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "omfToolkit.mak" CFG="omfToolkit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "omfToolkit - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "omfToolkit - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "omfToolkit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\ReleaseOutput"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OMFTOOLKIT_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\portinc" /I "..\compat" /I "..\bento" /I "..\jpeg" /I "..\avidjpg" /I "..\kitomfi" /D "OMF_BUILDING_TOOLKIT_DLL" /D "OMFI_NEED_ULONG" /D "HAVE_STDC" /D "OMFI_JPEG_CODEC" /D "OMFI_ENABLE_SEMCHECK" /D "OMFI_ERROR_TRACE" /D "OMFI_ENABLE_STREAM_CACHE" /D "OMF_USE_DLL" /D "INCLUDES_ARE_ANSI" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "omfToolkit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Output"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OMFTOOLKIT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\portinc" /I "..\compat" /I "..\bento" /I "..\jpeg" /I "..\avidjpg" /I "..\kitomfi" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OMF_BUILDING_TOOLKIT_DLL" /D "OMFI_NEED_ULONG" /D "HAVE_STDC" /D "OMFI_JPEG_CODEC" /D "OMFI_ENABLE_SEMCHECK" /D "OMFI_ERROR_TRACE" /D "OMFI_ENABLE_STREAM_CACHE" /D "OMF_USE_DLL" /D "INCLUDES_ARE_ANSI" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:".\Output/omfToolkitd.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "omfToolkit - Win32 Release"
# Name "omfToolkit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\bento\BufferIO.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMCntOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMDbgOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMErrOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMHndOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMObjOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMRefOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMSesOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMTPOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMValOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\CMVHOps.c
# End Source File
# Begin Source File

SOURCE=..\bento\DynValus.c
# End Source File
# Begin Source File

SOURCE=..\bento\FreeSpce.c
# End Source File
# Begin Source File

SOURCE=..\bento\GlbNames.c
# End Source File
# Begin Source File

SOURCE=..\bento\Handlers.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jbsmooth.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcarith.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jccolor.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcdeflts.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcexpand.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jchuff.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcmaster.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcmcu.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcpipe.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jcsample.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdarith.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jddeflts.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdmcu.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdpipe.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jerror.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jfwddct.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jmemsys.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jrevdct.c
# End Source File
# Begin Source File

SOURCE=..\jpeg\jutils.c
# End Source File
# Begin Source File

SOURCE=..\bento\ListMgr.c
# End Source File
# Begin Source File

SOURCE=..\bento\oldtoc.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omAcces.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcAIFF.c
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAvidJFIF.c
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAvJPED.c
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAVR.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcCDCI.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCheck.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcJPEG.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCodec.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCompos.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcRGBA.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcStd.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcStrm.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcTIFF.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCvt.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcWAVE.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcXlate.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omEffect.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omErr.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omfansic.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omFile.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omFndSrc.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omfsess.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omIEEE.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omJFIF.c
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omJPED.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omJPEG.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omLocate.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omMedia.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omMedObj.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omMobBld.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omMobGet.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omMobMgt.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omSearch.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omTable.c
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omUtils.c
# End Source File
# Begin Source File

SOURCE=..\bento\SymTbMgr.c
# End Source File
# Begin Source File

SOURCE=..\bento\TOCEnts.c
# End Source File
# Begin Source File

SOURCE=..\bento\TOCIO.c
# End Source File
# Begin Source File

SOURCE=..\bento\TOCObjs.c
# End Source File
# Begin Source File

SOURCE=..\bento\Update.c
# End Source File
# Begin Source File

SOURCE=..\bento\Utility.c
# End Source File
# Begin Source File

SOURCE=..\bento\Values.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\avidjpg\AUNC.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AUNCp.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR1.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR12.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR1e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR2.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR25.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR26.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR27.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR2e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR2m.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR2s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR3.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR3e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR3m.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR3s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR4.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR4e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR4m.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR4s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR5.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR5e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR5m.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR6e.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR6m.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR6s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR70.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR71.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR75.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR77.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR8s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVR9s.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF100S.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF12S.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF200.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF200p.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF20p.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF25p.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF300.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF300p.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF35.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF50p.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF50S.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\AVRJFIF70.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\avrPvt.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\avrs.h
# End Source File
# Begin Source File

SOURCE=..\portinc\masterhd.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcAIFF.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAvidJFIF.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAvJPED.h
# End Source File
# Begin Source File

SOURCE=..\avidjpg\omcAVR.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcCDCI.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcJPEG.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCntPvt.h
# End Source File
# Begin Source File

SOURCE=..\include\omCodCmn.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omCodec.h
# End Source File
# Begin Source File

SOURCE=..\include\omCodId.h
# End Source File
# Begin Source File

SOURCE=..\include\omCompos.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcRGBA.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcStd.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcStrm.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcTIFF.h
# End Source File
# Begin Source File

SOURCE=..\include\omCvt.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omcWAVE.h
# End Source File
# Begin Source File

SOURCE=..\include\omDefs.h
# End Source File
# Begin Source File

SOURCE=..\include\omEffect.h
# End Source File
# Begin Source File

SOURCE=..\include\omErr.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\OMFAnsiC.h
# End Source File
# Begin Source File

SOURCE=..\include\omFile.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omJPEG.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omLocate.h
# End Source File
# Begin Source File

SOURCE=..\include\omMedia.h
# End Source File
# Begin Source File

SOURCE=..\include\omMobBld.h
# End Source File
# Begin Source File

SOURCE=..\include\omMobGet.h
# End Source File
# Begin Source File

SOURCE=..\include\omMobMgt.h
# End Source File
# Begin Source File

SOURCE=..\include\omPublic.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omPvt.h
# End Source File
# Begin Source File

SOURCE=..\kitomfi\omTable.h
# End Source File
# Begin Source File

SOURCE=..\include\omTypes.h
# End Source File
# Begin Source File

SOURCE=..\include\omUtils.h
# End Source File
# Begin Source File

SOURCE=..\portinc\portkey.h
# End Source File
# Begin Source File

SOURCE=..\portinc\portmacs.h
# End Source File
# Begin Source File

SOURCE=..\portinc\portvars.h
# End Source File
# End Group
# End Target
# End Project

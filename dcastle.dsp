# Microsoft Developer Studio Project File - Name="dcastle" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=dcastle - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dcastle.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dcastle.mak" CFG="dcastle - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dcastle - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "dcastle - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dcastle - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "dcastle - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "src\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "dcastle - Win32 Release"
# Name "dcastle - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\act.cpp
# End Source File
# Begin Source File

SOURCE=.\src\act_comm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\act_info.cpp
# End Source File
# Begin Source File

SOURCE=.\src\alias.cpp
# End Source File
# Begin Source File

SOURCE=.\src\arena.cpp
# End Source File
# Begin Source File

SOURCE=.\src\assign_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ban.cpp
# End Source File
# Begin Source File

SOURCE=.\src\board.cpp
# End Source File
# Begin Source File

SOURCE=.\src\channel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_barbarian.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_mage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_monk.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_paladin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_ranger.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_thief.cpp
# End Source File
# Begin Source File

SOURCE=.\src\class\cl_warrior.cpp
# End Source File
# Begin Source File

SOURCE=.\src\clan.cpp
# End Source File
# Begin Source File

SOURCE=.\src\comm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\const.cpp
# End Source File
# Begin Source File

SOURCE=.\src\db.cpp
# End Source File
# Begin Source File

SOURCE=.\src\fight.cpp
# End Source File
# Begin Source File

SOURCE=.\src\fount.cpp
# End Source File
# Begin Source File

SOURCE=.\src\game_portal.cpp
# End Source File
# Begin Source File

SOURCE=.\src\group.cpp
# End Source File
# Begin Source File

SOURCE=.\src\guild.cpp
# End Source File
# Begin Source File

SOURCE=.\src\handler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\info.cpp
# End Source File
# Begin Source File

SOURCE=.\src\interp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\inventory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ki.cpp
# End Source File
# Begin Source File

SOURCE=.\src\limits.cpp
# End Source File
# Begin Source File

SOURCE=.\src\magic.cpp
# End Source File
# Begin Source File

SOURCE=.\src\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mob_act.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mob_commands.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mob_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mob_proc2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\mob_prog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\modify.cpp
# End Source File
# Begin Source File

SOURCE=.\src\move.cpp
# End Source File
# Begin Source File

SOURCE=.\src\nanny.cpp
# End Source File
# Begin Source File

SOURCE=.\src\nlog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\non_off.cpp
# End Source File
# Begin Source File

SOURCE=.\src\nullfile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\obj_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\objects.cpp
# End Source File
# Begin Source File

SOURCE=.\src\offense.cpp
# End Source File
# Begin Source File

SOURCE=.\src\save.cpp
# End Source File
# Begin Source File

SOURCE=.\src\shop.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sing.cpp
# End Source File
# Begin Source File

SOURCE=.\src\social.cpp
# End Source File
# Begin Source File

SOURCE=.\src\spells.cpp
# End Source File
# Begin Source File

SOURCE=.\src\token.cpp
# End Source File
# Begin Source File

SOURCE=.\src\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\src\weather.cpp
# End Source File
# Begin Source File

SOURCE=.\src\who.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_101.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_102.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_103.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_104.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_105.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_106.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_107.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_108.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_109.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wiz_110.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wizard.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\include\act.h
# End Source File
# Begin Source File

SOURCE=.\src\include\affect.h
# End Source File
# Begin Source File

SOURCE=.\src\include\alias.h
# End Source File
# Begin Source File

SOURCE=.\src\include\arena.h
# End Source File
# Begin Source File

SOURCE=.\src\include\bandwidth.h
# End Source File
# Begin Source File

SOURCE=.\src\include\character.h
# End Source File
# Begin Source File

SOURCE=.\src\include\clan.h
# End Source File
# Begin Source File

SOURCE=.\src\include\comm.h
# End Source File
# Begin Source File

SOURCE=.\src\include\connect.h
# End Source File
# Begin Source File

SOURCE=.\src\include\db.h
# End Source File
# Begin Source File

SOURCE=.\src\include\event.h
# End Source File
# Begin Source File

SOURCE=.\src\include\fight.h
# End Source File
# Begin Source File

SOURCE=.\src\include\fileinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\include\game_portal.h
# End Source File
# Begin Source File

SOURCE=.\src\include\handler.h
# End Source File
# Begin Source File

SOURCE=.\src\include\ident.h
# End Source File
# Begin Source File

SOURCE=.\src\include\interp.h
# End Source File
# Begin Source File

SOURCE=.\src\include\isr.h
# End Source File
# Begin Source File

SOURCE=.\src\include\ki.h
# End Source File
# Begin Source File

SOURCE=.\src\include\language.h
# End Source File
# Begin Source File

SOURCE=.\src\include\levels.h
# End Source File
# Begin Source File

SOURCE=.\src\include\machine.h
# End Source File
# Begin Source File

SOURCE=.\src\include\magic.h
# End Source File
# Begin Source File

SOURCE=.\src\include\memory.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mobile.h
# End Source File
# Begin Source File

SOURCE=.\src\include\obj.h
# End Source File
# Begin Source File

SOURCE=.\src\include\player.h
# End Source File
# Begin Source File

SOURCE=.\src\include\punish.h
# End Source File
# Begin Source File

SOURCE=.\src\include\race.h
# End Source File
# Begin Source File

SOURCE=.\src\include\returnvals.h
# End Source File
# Begin Source File

SOURCE=.\src\include\room.h
# End Source File
# Begin Source File

SOURCE=.\src\include\shop.h
# End Source File
# Begin Source File

SOURCE=.\src\include\sing.h
# End Source File
# Begin Source File

SOURCE=.\src\include\social.h
# End Source File
# Begin Source File

SOURCE=.\src\include\socket.h
# End Source File
# Begin Source File

SOURCE=.\src\include\spells.h
# End Source File
# Begin Source File

SOURCE=.\src\include\structs.h
# End Source File
# Begin Source File

SOURCE=.\src\include\terminal.h
# End Source File
# Begin Source File

SOURCE=.\src\include\timeinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\include\token.h
# End Source File
# Begin Source File

SOURCE=.\src\include\utility.h
# End Source File
# Begin Source File

SOURCE=.\src\include\weather.h
# End Source File
# Begin Source File

SOURCE=.\src\wizard\wizard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

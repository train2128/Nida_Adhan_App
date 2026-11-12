; Nida installer - installs to Program Files, registers uninstall entry
; (Programs & Features remove/modify) and an optional Windows startup entry.
;
; Build:
;   makensis installer.nsi
;   makensis /DDEPLOY_DIR="C:\path\to\deploy" installer.nsi
;
; Expected DEPLOY_DIR content (windeployqt output):
;   Nida.exe + Qt6*.dll, MinGW runtimes, platforms/, styles/, iconengines/,
;   imageformats/, multimedia/, networkinformation/, sqldrivers/, tls/,
;   generic/, translations/

Unicode True
SetCompressor lzma
XPStyle on

!define PRODUCT_NAME "Nida"
!define PRODUCT_VERSION "1.1.0"
!define PRODUCT_PUBLISHER "Nida"
!define PRODUCT_WEB_SITE "https://github.com/train2128/Nida_Adhan_App"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Nida.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_RUN_KEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define PRODUCT_UNINST_ROOT_KEY HKLM

!ifndef DEPLOY_DIR
  !define DEPLOY_DIR "C:\Users\TECBOX\Desktop\Nida"
!endif

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

; MUI settings
!define MUI_ABORTWARNING
!define MUI_ICON "resources\win\icon.ico"
!define MUI_UNICON "resources\win\icon.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\Nida.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Nida now"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Nida-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\Nida"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} - Prayer time notification and adhan player"
VIAddVersionKey "LegalCopyright" "MIT License"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"

; --- Main application (required) ---
Section "Application (required)" SEC_APP
  SectionIn RO

  ; Close a running instance so files can be overwritten.
  ; nsExec is bundled with NSIS, no extra plugin needed.
  nsExec::ExecToStack 'taskkill /f /im Nida.exe'
  Pop $0 ; exit code, ignored
  Pop $1 ; output, ignored
  Sleep 500

  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Copy everything windeployqt produced (exe, Qt/MinGW DLLs, plugins).
  ; Recursive on purpose: immune to Qt DLL renames (avcodec-61, etc.)
  ; and to plugin sets changing between Qt versions.
  File /r "${DEPLOY_DIR}\*.*"

  ; Uninstaller + Programs & Features (Add/Remove Programs) entry.
  ; This is what gives you the remove/modify entry.
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Nida.exe,0"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninst.exe" /S'
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"

  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Nida.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "Path" "$INSTDIR"
SectionEnd

; --- Startup entry (optional, checked by default) ---
Section "Start with Windows" SEC_STARTUP
  ; Machine-wide Run entry (needs admin - we have it).
  ; Quoted so "Program Files" paths work. The app's own
  ; Settings toggle manages the per-user (HKCU) entry separately.
  WriteRegStr HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}" '"$INSTDIR\Nida.exe"'
SectionEnd

Section "Desktop Shortcut" SEC_DESKTOP
  CreateShortCut "$DESKTOP\Nida.lnk" "$INSTDIR\Nida.exe" "" "$INSTDIR\Nida.exe" 0
SectionEnd

Section "Start Menu Shortcuts" SEC_STARTMENU
  CreateDirectory "$SMPROGRAMS\Nida"
  CreateShortCut "$SMPROGRAMS\Nida\Nida.lnk" "$INSTDIR\Nida.exe" "" "$INSTDIR\Nida.exe" 0
  CreateShortCut "$SMPROGRAMS\Nida\Uninstall Nida.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
SectionEnd

; Section descriptions
LangString DESC_APP ${LANG_ENGLISH} "Nida application files (Qt runtimes, plugins) installed under Program Files."
LangString DESC_STARTUP ${LANG_ENGLISH} "Launch Nida automatically when Windows starts (machine-wide Run entry)."
LangString DESC_DESKTOP ${LANG_ENGLISH} "Add a Nida shortcut to the desktop."
LangString DESC_STARTMENU ${LANG_ENGLISH} "Add Nida and Uninstall shortcuts to the Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} $(DESC_APP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTUP} $(DESC_STARTUP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} $(DESC_DESKTOP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} $(DESC_STARTMENU)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Default state: startup + start-menu on, desktop off
Function .onInit
  ; Pre-select sections
  SectionSetFlags ${SEC_STARTUP} 1
  SectionSetFlags ${SEC_DESKTOP} 0
  SectionSetFlags ${SEC_STARTMENU} 1

  ; If already installed, offer to remove the old version first
  ReadRegStr $R0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
  IfErrors done
  IfSilent uninstall_old
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
    "${PRODUCT_NAME} is already installed.$\nClick OK to remove the previous version or Cancel to abort." \
    IDOK uninstall_old
  Abort
uninstall_old:
  ExecWait '$R0 /S _?=$INSTDIR'
done:
FunctionEnd

Section "Uninstall"
  ; Stop a running instance first
  nsExec::ExecToStack 'taskkill /f /im Nida.exe'
  Pop $0
  Pop $1
  Sleep 500

  ; Remove startup entries (machine-wide from installer + per-user from app Settings)
  DeleteRegValue HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"
  DeleteRegValue HKCU "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"

  ; Remove Programs & Features entry + App Paths
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\Nida\Nida.lnk"
  Delete "$SMPROGRAMS\Nida\Uninstall Nida.lnk"
  RMDir "$SMPROGRAMS\Nida"
  Delete "$DESKTOP\Nida.lnk"

  ; Remove installed files (recursive: covers all Qt/plugin files
  ; regardless of version-specific DLL names)
  RMDir /r "$INSTDIR"
SectionEnd

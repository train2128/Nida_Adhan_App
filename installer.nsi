; ─────────────────────────────────────────────────────────────────────────────
; Nida installer (NSIS 3)
;
; - Fresh install: normal wizard.
; - Older version installed: asks to update, removes the old version, then
;   installs the new one in the same folder. Settings (in %APPDATA%) are kept.
; - Same version installed: offers to reinstall/repair.
; - Newer version installed: warns before downgrading.
; - Registers in "Apps & features" / "Programs and Features" with uninstaller.
;
; Build:
;   makensis installer.nsi
;   makensis /DDEPLOY_DIR="C:\path\to\deploy" installer.nsi
;
; DEPLOY_DIR = folder with Nida.exe + everything windeployqt copied next to it.
; The version is read from Nida.exe itself (set by project(VERSION) in
; CMakeLists.txt), so the installer and the app can never disagree.
; ─────────────────────────────────────────────────────────────────────────────

Unicode True
SetCompressor /SOLID lzma
ManifestDPIAware true

!ifndef DEPLOY_DIR
  !define DEPLOY_DIR "C:\Users\TECBOX\Desktop\Nida"
!endif

!if /FileExists "${DEPLOY_DIR}\Nida.exe"
!else
  !error "Nida.exe not found in DEPLOY_DIR (${DEPLOY_DIR}). Pass /DDEPLOY_DIR=..."
!endif

; Version straight from the exe's VERSIONINFO (e.g. 1.2.0.0 -> 1.2.0).
!getdllversion "${DEPLOY_DIR}\Nida.exe" NIDA_V_
!define PRODUCT_VERSION "${NIDA_V_1}.${NIDA_V_2}.${NIDA_V_3}"
!if "${PRODUCT_VERSION}" == "0.0.0"
  !error "Nida.exe has no version info; rebuild it with the current CMakeLists.txt"
!endif

!define PRODUCT_NAME "Nida"
!define PRODUCT_PUBLISHER "Nida"
!define PRODUCT_WEB_SITE "https://github.com/train2128/Nida_Adhan_App"
!define PRODUCT_EXE "Nida.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_EXE}"
!define PRODUCT_RUN_KEY "Software\Microsoft\Windows\CurrentVersion\Run"

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "WordFunc.nsh"
!include "x64.nsh"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Nida-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show
BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION}"

VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "FileDescription" "${PRODUCT_NAME} ${PRODUCT_VERSION} Setup"
VIAddVersionKey "LegalCopyright" "MIT License"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"

; State of an existing installation (filled in .onInit)
Var OldVersion      ; "" when not installed
Var OldUninstaller  ; full path to the old uninst.exe
Var OldDir          ; old install folder
Var OldRegView      ; 32 or 64: registry view the old entry lives in
Var IsUpgrade       ; 1 when replacing an existing install

; ── Wizard pages ────────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON "resources\win\icon.ico"
!define MUI_UNICON "resources\win\icon.ico"
!define MUI_WELCOMEPAGE_TITLE "${PRODUCT_NAME} ${PRODUCT_VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT "$(WELCOME_TEXT)"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME} now"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchNida

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipDirOnUpgrade
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

LangString WELCOME_TEXT ${LANG_ENGLISH} "This will install ${PRODUCT_NAME} ${PRODUCT_VERSION}, the prayer times and adhan app.$\r$\n$\r$\nIf ${PRODUCT_NAME} is running, it will be closed during setup.$\r$\n$\r$\nClick Next to continue."

; ── Helpers ─────────────────────────────────────────────────────────────────
!macro CloseNida
  ; Close a running Nida so its files can be replaced/removed.
  nsExec::ExecToStack 'taskkill /f /im ${PRODUCT_EXE}'
  Pop $0
  Pop $0
  Sleep 800
!macroend

; Reads the existing install from one registry view into the Old* variables.
!macro ReadOldInstall VIEW
  SetRegView ${VIEW}
  ReadRegStr $0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
  ${If} $0 != ""
  ${AndIf} $OldVersion == ""
    ReadRegStr $OldVersion HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion"
    ReadRegStr $OldDir HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
    ${If} $OldVersion == ""
      StrCpy $OldVersion "0.0.0" ; registered but without a version: treat as old
    ${EndIf}
    ; UninstallString is quoted: "C:\...\uninst.exe"
    StrCpy $OldUninstaller $0
    StrCpy $1 $OldUninstaller 1
    ${If} $1 == '"'
      StrCpy $OldUninstaller $OldUninstaller "" 1
      StrCpy $OldUninstaller $OldUninstaller -1
    ${EndIf}
    ${If} $OldDir == ""
      ${GetParent} $OldUninstaller $OldDir
    ${EndIf}
    StrCpy $OldRegView ${VIEW}
  ${EndIf}
!macroend

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires 64-bit Windows."
    Abort
  ${EndIf}

  StrCpy $IsUpgrade 0
  StrCpy $OldVersion ""
  ; Current installs register in the 64-bit view; 1.1.0 and older were
  ; registered by a 32-bit installer in the 32-bit (WOW6432Node) view.
  !insertmacro ReadOldInstall 64
  !insertmacro ReadOldInstall 32
  SetRegView 64

  ${If} $OldVersion != ""
    StrCpy $IsUpgrade 1
    StrCpy $INSTDIR $OldDir
    ${VersionCompare} "$OldVersion" "${PRODUCT_VERSION}" $2
    ; $2: 0 = same, 1 = installed is newer, 2 = installed is older
    ${If} $2 == 2
      IfSilent +3
      MessageBox MB_YESNO|MB_ICONQUESTION "${PRODUCT_NAME} $OldVersion is installed.$\r$\n$\r$\nUpdate to version ${PRODUCT_VERSION}? The old version will be removed and replaced. Your settings are kept." IDYES +2
      Abort
    ${ElseIf} $2 == 0
      IfSilent +3
      MessageBox MB_YESNO|MB_ICONQUESTION "${PRODUCT_NAME} ${PRODUCT_VERSION} is already installed.$\r$\n$\r$\nReinstall it? Your settings are kept." IDYES +2
      Abort
    ${Else}
      IfSilent +3
      MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 "A newer version of ${PRODUCT_NAME} ($OldVersion) is installed.$\r$\n$\r$\nReplace it with the older version ${PRODUCT_VERSION}?" IDYES +2
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

Function SkipDirOnUpgrade
  ; An update goes into the existing folder; don't ask again.
  ${If} $IsUpgrade == 1
    Abort
  ${EndIf}
FunctionEnd

Function LaunchNida
  ; Setup runs elevated; start Nida through Explorer so it runs as the normal
  ; user (its autostart and settings belong to that user, not to admin).
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\${PRODUCT_EXE}"'
FunctionEnd

Function RemoveOldVersion
  DetailPrint "Removing ${PRODUCT_NAME} $OldVersion..."
  ${If} ${FileExists} "$OldUninstaller"
    ; _?= runs the old uninstaller in place and waits for it to finish.
    ExecWait '"$OldUninstaller" /S _?=$OldDir' $0
    DetailPrint "Old uninstaller finished (code $0)"
    Delete "$OldUninstaller"
  ${EndIf}
  ; Leftovers (only if this really is a Nida folder).
  ${If} ${FileExists} "$OldDir\${PRODUCT_EXE}"
    RMDir /r "$OldDir"
  ${EndIf}
  RMDir "$OldDir"
  ; Drop the old registry entry in whichever view it lived.
  ${If} $OldRegView == 32
    SetRegView 32
  ${Else}
    SetRegView 64
  ${EndIf}
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetRegView 64
FunctionEnd

; ── Sections ────────────────────────────────────────────────────────────────
Section "Nida (required)" SEC_APP
  SectionIn RO
  !insertmacro CloseNida

  ${If} $IsUpgrade == 1
    Call RemoveOldVersion
  ${EndIf}

  ; Older installers added a machine-wide autostart entry; together with the
  ; app's own per-user entry that started Nida twice. The app now manages
  ; autostart itself (Settings > General), so remove the old one.
  SetRegView 32
  DeleteRegValue HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"
  SetRegView 64
  DeleteRegValue HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"

  SetOutPath "$INSTDIR"
  SetOverwrite on
  ; Everything windeployqt produced (exe, Qt/MinGW DLLs, plugin folders).
  File /r "${DEPLOY_DIR}\*.*"

  WriteUninstaller "$INSTDIR\uninst.exe"

  ; Apps & features / Programs and Features entry
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "HelpLink" "${PRODUCT_WEB_SITE}/issues"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${PRODUCT_EXE},0"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninst.exe" /S'
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "VersionMajor" ${NIDA_V_1}
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "VersionMinor" ${NIDA_V_2}
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"

  ; Lets "Nida" be started from Win+R
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\${PRODUCT_EXE}"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "Path" "$INSTDIR"
SectionEnd

Section "Start Menu shortcut" SEC_STARTMENU
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
SectionEnd

Section /o "Desktop shortcut" SEC_DESKTOP
  SetShellVarContext all
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0
SectionEnd

LangString DESC_APP ${LANG_ENGLISH} "The ${PRODUCT_NAME} app and the Qt runtime it needs."
LangString DESC_STARTMENU ${LANG_ENGLISH} "Add ${PRODUCT_NAME} and its uninstaller to the Start menu."
LangString DESC_DESKTOP ${LANG_ENGLISH} "Add a ${PRODUCT_NAME} shortcut to the desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} $(DESC_APP)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} $(DESC_STARTMENU)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} $(DESC_DESKTOP)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ── Uninstaller ─────────────────────────────────────────────────────────────
Function un.onInit
  SetRegView 64
FunctionEnd

Section "Uninstall"
  !insertmacro CloseNida

  ; Autostart entries: per-user (set by the app) and old machine-wide ones.
  DeleteRegValue HKCU "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"
  DeleteRegValue HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"
  SetRegView 32
  DeleteRegValue HKLM "${PRODUCT_RUN_KEY}" "${PRODUCT_NAME}"
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  SetRegView 64

  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  ; Shortcuts (current all-users ones and older per-user ones)
  SetShellVarContext all
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  SetShellVarContext current
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"

  ; Program files. Recursive because Qt plugin/DLL names vary by version;
  ; guarded so a broken path can never wipe an unrelated folder.
  ${If} ${FileExists} "$INSTDIR\${PRODUCT_EXE}"
    RMDir /r "$INSTDIR"
  ${Else}
    Delete "$INSTDIR\uninst.exe"
    RMDir "$INSTDIR"
  ${EndIf}
  ; User settings in %APPDATA%\Nida are left in place on purpose.
SectionEnd

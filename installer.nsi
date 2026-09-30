!define PRODUCT_NAME "Nida"
!define PRODUCT_VERSION "1.1.0"
!define PRODUCT_PUBLISHER "Nida"
!define PRODUCT_WEB_SITE "https://github.com/train2128/Nida_Adhan_App"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Nida.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_STARTUP_KEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define PRODUCT_UNINST_ROOT_KEY HKLM

; Point this to the folder containing Nida.exe + all deployed Qt DLLs
!define DEPLOY_DIR "C:\Users\Train-Windows\Desktop\DEV\Nida"

SetCompressor lzma

!include "MUI2.nsh"
!include "FileFunc.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${DEPLOY_DIR}\..\Nida_Adhan_App\resources\win\icon.ico"
!define MUI_UNICON "${DEPLOY_DIR}\..\Nida_Adhan_App\resources\win\icon.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${DEPLOY_DIR}\..\Nida_Adhan_App\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Nida-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES64\Nida"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin

; Sections
Section "Application (required)" SEC01
  SectionIn RO
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Root files
  File "${DEPLOY_DIR}\Nida.exe"
  File "${DEPLOY_DIR}\Qt6Core.dll"
  File "${DEPLOY_DIR}\Qt6Gui.dll"
  File "${DEPLOY_DIR}\Qt6Widgets.dll"
  File "${DEPLOY_DIR}\Qt6Multimedia.dll"
  File "${DEPLOY_DIR}\Qt6Network.dll"
  File "${DEPLOY_DIR}\Qt6Sql.dll"
  File "${DEPLOY_DIR}\Qt6Svg.dll"
  File "${DEPLOY_DIR}\Qt6Pdf.dll"
  File "${DEPLOY_DIR}\libc++.dll"
  File "${DEPLOY_DIR}\libunwind.dll"
  File "${DEPLOY_DIR}\libgcc_s_seh-1.dll"
  File "${DEPLOY_DIR}\libstdc++-6.dll"
  File "${DEPLOY_DIR}\libwinpthread-1.dll"
  File "${DEPLOY_DIR}\D3Dcompiler_47.dll"
  File "${DEPLOY_DIR}\opengl32sw.dll"
  File "${DEPLOY_DIR}\avcodec-61.dll"
  File "${DEPLOY_DIR}\avformat-61.dll"
  File "${DEPLOY_DIR}\avutil-59.dll"
  File "${DEPLOY_DIR}\swresample-5.dll"
  File "${DEPLOY_DIR}\swscale-8.dll"

  ; Subdirectories
  SetOutPath "$INSTDIR\platforms"
  File "${DEPLOY_DIR}\platforms\qwindows.dll"

  SetOutPath "$INSTDIR\styles"
  File "${DEPLOY_DIR}\styles\qmodernwindowsstyle.dll"

  SetOutPath "$INSTDIR\iconengines"
  File "${DEPLOY_DIR}\iconengines\qsvgicon.dll"

  SetOutPath "$INSTDIR\imageformats"
  File "${DEPLOY_DIR}\imageformats\qgif.dll"
  File "${DEPLOY_DIR}\imageformats\qicns.dll"
  File "${DEPLOY_DIR}\imageformats\qico.dll"
  File "${DEPLOY_DIR}\imageformats\qjpeg.dll"
  File "${DEPLOY_DIR}\imageformats\qpdf.dll"
  File "${DEPLOY_DIR}\imageformats\qsvg.dll"
  File "${DEPLOY_DIR}\imageformats\qtga.dll"
  File "${DEPLOY_DIR}\imageformats\qtiff.dll"
  File "${DEPLOY_DIR}\imageformats\qwbmp.dll"
  File "${DEPLOY_DIR}\imageformats\qwebp.dll"

  SetOutPath "$INSTDIR\multimedia"
  File "${DEPLOY_DIR}\multimedia\ffmpegmediaplugin.dll"
  File "${DEPLOY_DIR}\multimedia\windowsmediaplugin.dll"

  SetOutPath "$INSTDIR\networkinformation"
  File "${DEPLOY_DIR}\networkinformation\qnetworklistmanager.dll"

  SetOutPath "$INSTDIR\sqldrivers"
  File "${DEPLOY_DIR}\sqldrivers\qsqlite.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlibase.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlmimer.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqloci.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlodbc.dll"
  File "${DEPLOY_DIR}\sqldrivers\qsqlpsql.dll"

  SetOutPath "$INSTDIR\tls"
  File "${DEPLOY_DIR}\tls\qcertonlybackend.dll"
  File "${DEPLOY_DIR}\tls\qschannelbackend.dll"

  SetOutPath "$INSTDIR\generic"
  File "${DEPLOY_DIR}\generic\qtuiotouchplugin.dll"

  SetOutPath "$INSTDIR\translations"
  File "${DEPLOY_DIR}\translations\qt_ar.qm"
  File "${DEPLOY_DIR}\translations\qt_bg.qm"
  File "${DEPLOY_DIR}\translations\qt_ca.qm"
  File "${DEPLOY_DIR}\translations\qt_cs.qm"
  File "${DEPLOY_DIR}\translations\qt_da.qm"
  File "${DEPLOY_DIR}\translations\qt_de.qm"
  File "${DEPLOY_DIR}\translations\qt_en.qm"
  File "${DEPLOY_DIR}\translations\qt_es.qm"
  File "${DEPLOY_DIR}\translations\qt_fa.qm"
  File "${DEPLOY_DIR}\translations\qt_fi.qm"
  File "${DEPLOY_DIR}\translations\qt_fr.qm"
  File "${DEPLOY_DIR}\translations\qt_gd.qm"
  File "${DEPLOY_DIR}\translations\qt_he.qm"
  File "${DEPLOY_DIR}\translations\qt_hr.qm"
  File "${DEPLOY_DIR}\translations\qt_hu.qm"
  File "${DEPLOY_DIR}\translations\qt_it.qm"
  File "${DEPLOY_DIR}\translations\qt_ja.qm"
  File "${DEPLOY_DIR}\translations\qt_ka.qm"
  File "${DEPLOY_DIR}\translations\qt_ko.qm"
  File "${DEPLOY_DIR}\translations\qt_lg.qm"
  File "${DEPLOY_DIR}\translations\qt_lv.qm"
  File "${DEPLOY_DIR}\translations\qt_nl.qm"
  File "${DEPLOY_DIR}\translations\qt_nn.qm"
  File "${DEPLOY_DIR}\translations\qt_pl.qm"
  File "${DEPLOY_DIR}\translations\qt_pt_BR.qm"
  File "${DEPLOY_DIR}\translations\qt_ru.qm"
  File "${DEPLOY_DIR}\translations\qt_sk.qm"
  File "${DEPLOY_DIR}\translations\qt_sv.qm"
  File "${DEPLOY_DIR}\translations\qt_tr.qm"
  File "${DEPLOY_DIR}\translations\qt_uk.qm"
  File "${DEPLOY_DIR}\translations\qt_zh_CN.qm"
  File "${DEPLOY_DIR}\translations\qt_zh_TW.qm"

  ; Windows startup (current user)
  WriteRegStr HKCU "${PRODUCT_STARTUP_KEY}" "${PRODUCT_NAME}" "$INSTDIR\Nida.exe"

  ; Uninstaller + Programs & Features entry
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Nida.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" "$INSTDIR\uninst.exe /S"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Nida.exe"
SectionEnd

Section "Desktop Shortcut" SEC02
  CreateShortCut "$DESKTOP\Nida.lnk" "$INSTDIR\Nida.exe" "" "$INSTDIR\Nida.exe" 0
SectionEnd

Section "Start Menu Shortcut" SEC03
  CreateDirectory "$SMPROGRAMS\Nida"
  CreateShortCut "$SMPROGRAMS\Nida\Nida.lnk" "$INSTDIR\Nida.exe" "" "$INSTDIR\Nida.exe" 0
SectionEnd

; Section descriptions
LangString DESC_SEC01 ${LANG_ENGLISH} "Nida application files, Qt & MinGW runtimes, plugins, and startup registry entry."
LangString DESC_SEC02 ${LANG_ENGLISH} "Add a shortcut to Nida on your desktop."
LangString DESC_SEC03 ${LANG_ENGLISH} "Add a shortcut to Nida in the Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} $(DESC_SEC01)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} $(DESC_SEC02)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} $(DESC_SEC03)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; .onInit - check for existing install
Function .onInit
  ReadRegStr $R0 HKLM "${PRODUCT_UNINST_KEY}" "UninstallString"
  IfErrors done
  IfSilent uninstall_old
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
    "${PRODUCT_NAME} is already installed. Click OK to remove the previous version or Cancel to abort." \
    IDOK uninstall_old
  Abort
uninstall_old:
  ExecWait '"$R0" /S _?=$INSTDIR'
done:
FunctionEnd

Section "Uninstall"
  ; Remove startup entry
  DeleteRegValue HKCU "${PRODUCT_STARTUP_KEY}" "${PRODUCT_NAME}"

  ; Remove Programs & Features entry
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\Nida\Nida.lnk"
  RMDir "$SMPROGRAMS\Nida"
  Delete "$DESKTOP\Nida.lnk"

  ; Remove files
  Delete "$INSTDIR\Nida.exe"
  Delete "$INSTDIR\Qt6Core.dll"
  Delete "$INSTDIR\Qt6Gui.dll"
  Delete "$INSTDIR\Qt6Widgets.dll"
  Delete "$INSTDIR\Qt6Multimedia.dll"
  Delete "$INSTDIR\Qt6Network.dll"
  Delete "$INSTDIR\Qt6Sql.dll"
  Delete "$INSTDIR\Qt6Svg.dll"
  Delete "$INSTDIR\Qt6Pdf.dll"
  Delete "$INSTDIR\libc++.dll"
  Delete "$INSTDIR\libunwind.dll"
  Delete "$INSTDIR\libgcc_s_seh-1.dll"
  Delete "$INSTDIR\libstdc++-6.dll"
  Delete "$INSTDIR\libwinpthread-1.dll"
  Delete "$INSTDIR\D3Dcompiler_47.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\avcodec-61.dll"
  Delete "$INSTDIR\avformat-61.dll"
  Delete "$INSTDIR\avutil-59.dll"
  Delete "$INSTDIR\swresample-5.dll"
  Delete "$INSTDIR\swscale-8.dll"
  Delete "$INSTDIR\uninst.exe"

  Delete "$INSTDIR\platforms\qwindows.dll"
  Delete "$INSTDIR\styles\qmodernwindowsstyle.dll"
  Delete "$INSTDIR\iconengines\qsvgicon.dll"
  Delete "$INSTDIR\imageformats\qgif.dll"
  Delete "$INSTDIR\imageformats\qicns.dll"
  Delete "$INSTDIR\imageformats\qico.dll"
  Delete "$INSTDIR\imageformats\qjpeg.dll"
  Delete "$INSTDIR\imageformats\qpdf.dll"
  Delete "$INSTDIR\imageformats\qsvg.dll"
  Delete "$INSTDIR\imageformats\qtga.dll"
  Delete "$INSTDIR\imageformats\qtiff.dll"
  Delete "$INSTDIR\imageformats\qwbmp.dll"
  Delete "$INSTDIR\imageformats\qwebp.dll"
  Delete "$INSTDIR\multimedia\ffmpegmediaplugin.dll"
  Delete "$INSTDIR\multimedia\windowsmediaplugin.dll"
  Delete "$INSTDIR\networkinformation\qnetworklistmanager.dll"
  Delete "$INSTDIR\sqldrivers\qsqlite.dll"
  Delete "$INSTDIR\sqldrivers\qsqlibase.dll"
  Delete "$INSTDIR\sqldrivers\qsqlmimer.dll"
  Delete "$INSTDIR\sqldrivers\qsqloci.dll"
  Delete "$INSTDIR\sqldrivers\qsqlodbc.dll"
  Delete "$INSTDIR\sqldrivers\qsqlpsql.dll"
  Delete "$INSTDIR\tls\qcertonlybackend.dll"
  Delete "$INSTDIR\tls\qschannelbackend.dll"
  Delete "$INSTDIR\generic\qtuiotouchplugin.dll"
  Delete "$INSTDIR\translations\qt_ar.qm"
  Delete "$INSTDIR\translations\qt_bg.qm"
  Delete "$INSTDIR\translations\qt_ca.qm"
  Delete "$INSTDIR\translations\qt_cs.qm"
  Delete "$INSTDIR\translations\qt_da.qm"
  Delete "$INSTDIR\translations\qt_de.qm"
  Delete "$INSTDIR\translations\qt_en.qm"
  Delete "$INSTDIR\translations\qt_es.qm"
  Delete "$INSTDIR\translations\qt_fa.qm"
  Delete "$INSTDIR\translations\qt_fi.qm"
  Delete "$INSTDIR\translations\qt_fr.qm"
  Delete "$INSTDIR\translations\qt_gd.qm"
  Delete "$INSTDIR\translations\qt_he.qm"
  Delete "$INSTDIR\translations\qt_hr.qm"
  Delete "$INSTDIR\translations\qt_hu.qm"
  Delete "$INSTDIR\translations\qt_it.qm"
  Delete "$INSTDIR\translations\qt_ja.qm"
  Delete "$INSTDIR\translations\qt_ka.qm"
  Delete "$INSTDIR\translations\qt_ko.qm"
  Delete "$INSTDIR\translations\qt_lg.qm"
  Delete "$INSTDIR\translations\qt_lv.qm"
  Delete "$INSTDIR\translations\qt_nl.qm"
  Delete "$INSTDIR\translations\qt_nn.qm"
  Delete "$INSTDIR\translations\qt_pl.qm"
  Delete "$INSTDIR\translations\qt_pt_BR.qm"
  Delete "$INSTDIR\translations\qt_ru.qm"
  Delete "$INSTDIR\translations\qt_sk.qm"
  Delete "$INSTDIR\translations\qt_sv.qm"
  Delete "$INSTDIR\translations\qt_tr.qm"
  Delete "$INSTDIR\translations\qt_uk.qm"
  Delete "$INSTDIR\translations\qt_zh_CN.qm"
  Delete "$INSTDIR\translations\qt_zh_TW.qm"

  ; Remove subdirectories
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR\styles"
  RMDir "$INSTDIR\iconengines"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR\multimedia"
  RMDir "$INSTDIR\networkinformation"
  RMDir "$INSTDIR\sqldrivers"
  RMDir "$INSTDIR\tls"
  RMDir "$INSTDIR\generic"
  RMDir "$INSTDIR\translations"
  RMDir "$INSTDIR"
SectionEnd

@echo off
setlocal
pushd "%~dp0"

.\deploy_exe\kuincl.exe -i "%~dp0../src/kuin_editor/kuin_editor.kn" -o "%~dp0deploy_exe/kuin" -s "%~dp0deploy_exe/sys/" -e exe -r -x wnd
copy /Y ".\output\x64\Release\kuin_dll\kuin_dll.dll" "%~dp0deploy_exe\data\d0917.knd"
pause

popd

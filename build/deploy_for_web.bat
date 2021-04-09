@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
if exist "%~dp0deploy_web_res" rd /s /q "%~dp0deploy_web_res"
mkdir "%~dp0deploy_web_res"
mkdir "%~dp0deploy_web_res\sys"
copy /Y "..\src\sys\common.h" ".\deploy_web_res\sys\"
xcopy /s /e /q /i /y "..\src\sys\cpp" ".\deploy_web_res\sys\cpp"
xcopy /s /e /q /i /y "..\src\sys\web" ".\deploy_web_res\sys\web"
if exist "%~dp0deploy_web" rd /s /q "%~dp0deploy_web"
mkdir "%~dp0deploy_web"
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0deploy_web/kuin" -s "%~dp0../src/sys/" -p "%~dp0./deploy_web_res/" -e web -x static -r

pause

popd

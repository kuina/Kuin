@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe -x lang=ja

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
if exist "%~dp0deploy_web_res" rd /s /q "%~dp0deploy_web_res"
mkdir "%~dp0deploy_web_res"
mkdir "%~dp0deploy_web_res\sys"
copy /Y "..\src\sys\common.h" ".\deploy_web_res\sys\"
xcopy /s /e /q /i /y "..\src\sys\cpp" ".\deploy_web_res\sys\cpp"
xcopy /s /e /q /i /y "..\src\sys\web" ".\deploy_web_res\sys\web"
if exist "%~dp0deploy_web_ja" rd /s /q "%~dp0deploy_web_ja"
mkdir "%~dp0deploy_web_ja"
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0deploy_web_ja/kuin" -s "%~dp0../src/sys/" -p "%~dp0./deploy_web_res/" -e web -x static -r -x lang=ja

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
if exist "%~dp0deploy_web_en" rd /s /q "%~dp0deploy_web_en"
mkdir "%~dp0deploy_web_en"
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0deploy_web_en/kuin" -s "%~dp0../src/sys/" -p "%~dp0./deploy_web_res/" -e web -x static -r -x lang=en

pause

popd

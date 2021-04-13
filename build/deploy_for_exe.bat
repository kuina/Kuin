@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.cpp
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp" -s "%~dp0../src/sys/" -e cpp -r -x nogc
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll" -s "%~dp0../src/sys/" -e cpp -r -x nogc -x nogcdb -x lib

rem Kuin2.cpp ->[Visual Studio]->Kuin2.exe
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m
if exist "%~dp0deploy_exe" rd /s /q "%~dp0deploy_exe"
mkdir "%~dp0deploy_exe"
copy /Y ".\output\x64\Release\kuin_cpp\kuin_cpp.exe" "%~dp0deploy_exe\kuincl.exe"
xcopy /s /e /q /i /y "..\src\sys" "%~dp0deploy_exe\sys"
mkdir ".\deploy_exe\sys\data"
mkdir ".\deploy_exe\sys\data\dbg"
mkdir ".\deploy_exe\sys\data\rls"
copy /Y ".\libs\Release_dbg\*.knd" ".\deploy_exe\sys\data\dbg\"
copy /Y ".\libs\Release_rls\*.knd" ".\deploy_exe\sys\data\rls\"
pause

popd

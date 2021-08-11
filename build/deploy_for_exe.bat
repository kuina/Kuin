@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe -x lang=ja

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.cpp
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp_ja" -s "%~dp0../src/sys/" -e cpp -r -x nogc -x lang=ja
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll_ja" -s "%~dp0../src/sys/" -e cpp -r -x nogc -x nogcdb -x lib -x lang=ja
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp_en" -s "%~dp0../src/sys/" -e cpp -r -x nogc -x lang=en
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll_en" -s "%~dp0../src/sys/" -e cpp -r -x nogc -x nogcdb -x lib -x lang=en

rem Kuin2.cpp ->[Visual Studio]->Kuin2.exe
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m
pause

if exist "%~dp0deploy_exe_ja" rd /s /q "%~dp0deploy_exe_ja"
mkdir "%~dp0deploy_exe_ja"
copy /Y ".\output\x64\Release\kuin_cpp_ja\kuin_cpp_ja.exe" "%~dp0deploy_exe_ja\kuincl.exe"
xcopy /s /e /q /i /y "..\src\sys" "%~dp0deploy_exe_ja\sys"
mkdir ".\deploy_exe_ja\sys\data"
mkdir ".\deploy_exe_ja\sys\data\dbg"
mkdir ".\deploy_exe_ja\sys\data\rls"
copy /Y ".\libs\Release_dbg\*.knd" ".\deploy_exe_ja\sys\data\dbg\"
copy /Y ".\libs\Release_rls\*.knd" ".\deploy_exe_ja\sys\data\rls\"

.\deploy_exe_ja\kuincl.exe -i "%~dp0../src/kuin_editor/kuin_editor.kn" -o "%~dp0deploy_exe_ja/kuin" -s "%~dp0deploy_exe_ja/sys/" -e exe -r -x wnd -x lang=ja
copy /Y ".\output\x64\Release\kuin_dll_ja\kuin_dll_ja.dll" "%~dp0deploy_exe_ja\data\d0917.knd"

if exist "%~dp0deploy_exe_en" rd /s /q "%~dp0deploy_exe_en"
mkdir "%~dp0deploy_exe_en"
copy /Y ".\output\x64\Release\kuin_cpp_en\kuin_cpp_en.exe" "%~dp0deploy_exe_en\kuincl.exe"
xcopy /s /e /q /i /y "..\src\sys" "%~dp0deploy_exe_en\sys"
mkdir ".\deploy_exe_en\sys\data"
mkdir ".\deploy_exe_en\sys\data\dbg"
mkdir ".\deploy_exe_en\sys\data\rls"
copy /Y ".\libs\Release_dbg\*.knd" ".\deploy_exe_en\sys\data\dbg\"
copy /Y ".\libs\Release_rls\*.knd" ".\deploy_exe_en\sys\data\rls\"

.\deploy_exe_en\kuincl.exe -i "%~dp0../src/kuin_editor/kuin_editor.kn" -o "%~dp0deploy_exe_en/kuin" -s "%~dp0deploy_exe_en/sys/" -e exe -r -x wnd -x lang=en
copy /Y ".\output\x64\Release\kuin_dll_en\kuin_dll_en.dll" "%~dp0deploy_exe_en\data\d0917.knd"

copy /Y ".\output\kuin_cpp_ja.cpp" "%~dp0package_src_ja\kuin.cpp"
copy /Y ".\output\kuin_cpp_en.cpp" "%~dp0package_src_en\kuin.cpp"

pause

popd

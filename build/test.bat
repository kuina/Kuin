@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe

if exist ".\output\test_res" rd /s /q ".\output\test_res"
mkdir ".\output\test_res"
xcopy /s /e /q /i /y "..\src\sys" ".\output\test_res\sys"
copy /Y "..\src\compiler\*.kn" ".\output\test_res\"
xcopy /s /e /q /i /y "..\src\compiler\cpp" ".\output\test_res\cpp"
xcopy /s /e /q /i /y "..\src\compiler\web" ".\output\test_res\web"
xcopy /s /e /q /i /y "..\src\compiler\exe" ".\output\test_res\exe"
copy /Y ".\test_data\test.kn" ".\output\test_res\"
mkdir ".\output\test_res\sys\data"
mkdir ".\output\test_res\sys\data\dbg"
copy /Y ".\libs\Debug\*.knd" ".\output\test_res\sys\data\dbg\"

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.cpp
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp" -s "%~dp0output/test_res/sys/" -e cpp -x nogc
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll" -s "%~dp0output/test_res/sys/" -e cpp -x nogc -x nogcdb -x lib

rem Kuin2.cpp ->[Visual Studio]->Kuin2.exe
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.cpp
.\output\x64\Release\kuin_cpp\kuin_cpp.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp2" -s "%~dp0output/test_res/sys/" -e cpp -x nogc

rem Compare Kuin2.cpp
fc /N "%~dp0output\kuin_cpp.cpp" "%~dp0output\kuin_cpp2.cpp"
pause

rem Test Kuin2.exe
.\output\x64\Release\kuin_cpp\kuin_cpp.exe -i "%~dp0test_data/test.kn" -o "%~dp0output/test_kuin_cpp" -s "%~dp0output/test_res/sys/" -e cpp
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/test_kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m
.\output\x64\Release\test_kuin_cpp.exe > .\output\test_kuin_cpp_output.txt
fc /N "%~dp0output\test_kuin_cpp_output.txt" "%~dp0output\test_kuin_cpp_correct.txt"
pause

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
.\output\x64\Release\kuin_cpp\kuin_cpp.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_js" -s "%~dp0output/test_res/sys/" -p "%~dp0output/test_res/" -e web

rem Kuin2.js ->[Kuin2.js]-> Kuin2.js
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\kuin_js.html?-i&res/main.kn&-o&kuin_js2&-s&res/sys/&-e&web"

rem Test Kuin2.js
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\kuin_js.html?-i&res/test.kn&-o&test_kuin_js&-s&res/sys/&-e&web&-x&static"
echo "Place the files..."
pause
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\test_kuin_js2.html"

rem ---------------------------------------------------------------------------

.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/exe_output/kuin_exe" -s "%~dp0output/test_res/sys/" -e exe

.\output\exe_output\kuin_exe.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/exe_output/kuin_exe2" -s "%~dp0output/test_res/sys/" -e exe

.\output\exe_output\kuin_exe2.exe -i "%~dp0test_data/test.kn" -o "%~dp0output/exe_output/test_kuin_exe" -s "%~dp0output/test_res/sys/" -e exe

.\output\exe_output\test_kuin_exe.exe > .\output\test_kuin_exe_output.txt
fc /N "%~dp0output\test_kuin_exe_output.txt" "%~dp0output\test_kuin_exe_correct.txt"
pause
pause
pause

popd

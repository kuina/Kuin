@echo off
setlocal
pushd "%~dp0"

rem Kuin2.kn ->[Kuin1.exe]-> Kuin2.exe
.\kuincl.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin.exe" -s "%~dp0./sys/" -e exe -x lang=ja

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
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp_ja" -s "%~dp0output/test_res/sys/" -e cpp -r -x nogc -x lang=ja
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll_ja" -s "%~dp0output/test_res/sys/" -e cpp -r -x nogc -x nogcdb -x lib -x lang=ja
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp_en" -s "%~dp0output/test_res/sys/" -e cpp -r -x nogc -x lang=en
.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_dll_en" -s "%~dp0output/test_res/sys/" -e cpp -r -x nogc -x nogcdb -x lib -x lang=en

rem Kuin2.cpp ->[Visual Studio]->Kuin2.exe
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m
pause

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.cpp
.\output\x64\Release\kuin_cpp_ja\kuin_cpp_ja.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_cpp_ja2" -s "%~dp0output/test_res/sys/" -e cpp -r -x nogc -x lang=ja

rem Compare Kuin2.cpp
fc /N "%~dp0output\kuin_cpp_ja.cpp" "%~dp0output\kuin_cpp_ja2.cpp"
pause

rem Test Kuin2.exe
.\output\x64\Release\kuin_cpp_ja\kuin_cpp_ja.exe -i "%~dp0test_data/test.kn" -o "%~dp0output/test_kuin_cpp" -s "%~dp0output/test_res/sys/" -e cpp -x lang=ja
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0output/test_kuin_cpp.sln" /t:clean;rebuild /p:Configuration="Release" /p:Platform="x64" /m
.\output\x64\Release\test_kuin_cpp.exe > .\output\test_kuin_cpp_output.txt
fc /N "%~dp0output\test_kuin_cpp_output.txt" "%~dp0output\test_kuin_cpp_correct.txt"
pause

rem ---------------------------------------------------------------------------

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
.\output\x64\Release\kuin_cpp_ja\kuin_cpp_ja.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_js" -s "%~dp0output/test_res/sys/" -p "%~dp0output/test_res/" -e web -x lang=ja

rem Kuin2.js ->[Kuin2.js]-> Kuin2.js
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\kuin_js.html?-i&res/main.kn&-o&kuin_js2&-s&res/sys/&-e&web"

rem Test Kuin2.js
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\kuin_js.html?-i&res/test.kn&-o&test_kuin_js&-s&res/sys/&-e&web&-x&static"
echo "Place the files..."
pause
"C:\Program Files\Mozilla Firefox\firefox.exe" ".\output\test_kuin_js2.html"

rem ---------------------------------------------------------------------------

.\output\kuin.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/exe_output/kuin_exe" -s "%~dp0output/test_res/sys/" -e exe -x lang=ja

.\output\exe_output\kuin_exe.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/exe_output/kuin_exe2" -s "%~dp0output/test_res/sys/" -e exe -x lang=ja

.\output\exe_output\kuin_exe2.exe -i "%~dp0test_data/test.kn" -o "%~dp0output/exe_output/test_kuin_exe" -s "%~dp0output/test_res/sys/" -e exe -x lang=ja

.\output\exe_output\test_kuin_exe.exe > .\output\test_kuin_exe_output.txt
fc /N "%~dp0output\test_kuin_exe_output.txt" "%~dp0output\test_kuin_exe_correct.txt"
pause
pause
pause

popd

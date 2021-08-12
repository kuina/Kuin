@echo off
setlocal
pushd "%~dp0"

rem ---------------------------------------------------------------------------

set browser_path=C:\Program Files\Mozilla Firefox\firefox.exe

rem Kuin2.kn ->[Kuin2.exe]-> Kuin2.js
:: .\output\x64\Release\kuin_cpp_ja\kuin_cpp_ja.exe -i "%~dp0../src/compiler/main.kn" -o "%~dp0output/kuin_js" -s "%~dp0output/test_res/sys/" -p "%~dp0output/test_res/" -e web -x lang=ja

rem Kuin2.js ->[Kuin2.js]-> Kuin2.js
"%browser_path%" "file:///%~dp0output\kuin_js.html?-i&res/main.kn&-o&kuin_js2&-s&res/sys/&-e&web"

rem Test Kuin2.js
"%browser_path%" "file:///%~dp0output\kuin_js.html?-i&res/test.kn&-o&test_kuin_js&-s&res/sys/&-e&web&-x&static"
echo "Place the files..."
pause
"%browser_path%" "file:///%~dp0output\test_kuin_js2.html"

popd

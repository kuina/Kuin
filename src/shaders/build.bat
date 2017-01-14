@echo off
set fxc_path="C:\Program Files (x86)\Windows Kits\8.0\bin\x64\fxc.exe"
%fxc_path% tri_vs.fx /T vs_4_0 /Fo tri.vs
%fxc_path% tri_ps.fx /T ps_4_0 /Fo tri.ps
pause

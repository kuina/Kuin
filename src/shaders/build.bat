@echo off
set fxc_path="C:\Program Files (x86)\Windows Kits\8.1\bin\x64\fxc.exe"
%fxc_path% tri_vs.fx /T vs_4_0 /Fo tri.vs
%fxc_path% tri_ps.fx /T ps_4_0 /Fo tri.ps
%fxc_path% rect_vs.fx /T vs_4_0 /Fo rect.vs
%fxc_path% circle_vs.fx /T vs_4_0 /Fo circle.vs
%fxc_path% circle_ps.fx /T ps_4_0 /Fo circle.ps
pause

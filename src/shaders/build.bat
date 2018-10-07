@echo off
set fxc_path="C:\Program Files (x86)\Windows Kits\8.1\bin\x64\fxc.exe"
%fxc_path% tri_vs.fx /T vs_4_0 /Fo tri.vs
%fxc_path% tri_ps.fx /T ps_4_0 /Fo tri.ps
%fxc_path% font_ps.fx /T ps_4_0 /Fo font.ps
%fxc_path% rect_vs.fx /T vs_4_0 /Fo rect.vs
%fxc_path% circle_vs.fx /T vs_4_0 /Fo circle.vs
%fxc_path% circle_ps.fx /T ps_4_0 /Fo circle.ps
%fxc_path% circle_line_ps.fx /T ps_4_0 /Fo circle_line.ps
%fxc_path% tex_vs.fx /T vs_4_0 /Fo tex.vs
%fxc_path% tex_rot_vs.fx /T vs_4_0 /Fo tex_rot.vs
%fxc_path% tex_ps.fx /T ps_4_0 /Fo tex.ps
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj.vs
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj_joint.vs /DJOINT
%fxc_path% obj_ps.fx /T ps_4_0 /Fo obj.ps
%fxc_path% obj_toon_ps.fx /T ps_4_0 /Fo obj_toon.ps
%fxc_path% obj_outline_vs.fx /T vs_4_0 /Fo obj_outline.vs
%fxc_path% obj_outline_vs.fx /T vs_4_0 /Fo obj_outline_joint.vs /DJOINT
%fxc_path% obj_outline_ps.fx /T ps_4_0 /Fo obj_outline.ps
%fxc_path% filter_vs.fx /T vs_4_0 /Fo filter.vs
%fxc_path% filter_none_ps.fx /T ps_4_0 /Fo filter_none.ps
%fxc_path% filter_monotone_ps.fx /T ps_4_0 /Fo filter_monotone.ps
%fxc_path% particle_2d_vs.fx /T vs_4_0 /Fo particle_2d.vs
%fxc_path% particle_2d_ps.fx /T ps_4_0 /Fo particle_2d.ps
%fxc_path% particle_updating_vs.fx /T vs_4_0 /Fo particle_updating.vs
%fxc_path% particle_updating_ps.fx /T ps_4_0 /Fo particle_updating.ps
pause

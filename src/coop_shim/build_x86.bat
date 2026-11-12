@echo off
rem build_x86.bat - compile coop_shim.dll (x86) with MSVC. Output stays in THIS folder (NOT the game).
rem NOTE: target is x86 (/MACHINE:X86, /DWIN32); use the 32-bit toolchain (vcvars32).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat" >vc.log 2>&1
where cl >nul 2>&1 || (echo NO_CL & exit /b 1)
if not exist obj mkdir obj
cl /nologo /W3 /O1 /MT /LD /DWIN32 /D_CRT_SECURE_NO_WARNINGS /Fo"obj\\" coop_shim.cpp /Fe"coop_shim.dll" /link /MACHINE:X86 user32.lib >build.log 2>&1
echo BUILD_DONE %errorlevel%
echo (scaffold only - NOT deployed, NOT tested)

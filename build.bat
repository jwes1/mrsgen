@echo off
echo resource.rc
rc resource.rc >NUL
if %errorlevel% neq 0 (
  pause
	exit /b %errorlevel%
)
echo mrsgen.c
cl /nologo mrsgen.c /link /nologo resource.res >nul
if %errorlevel% neq 0 (
  pause
	exit /b %errorlevel%
)

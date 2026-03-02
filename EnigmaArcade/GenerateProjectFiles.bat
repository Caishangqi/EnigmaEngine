@echo off
echo ========================================
echo  EnigmaEngine - Generate Project Files
echo ========================================
echo.
dotnet run --project "%~dp0..\Engine\Source\Programs\BuildTool\Source\BuildTool\BuildTool.csproj" -- generate-project-files --project "%~dp0EnigmaArcade.eproject"
if %ERRORLEVEL% NEQ 0 (
echo.
echo [ERROR] Project file generation failed.
pause
exit /b 1
)
echo.
echo Project files generated successfully.
pause
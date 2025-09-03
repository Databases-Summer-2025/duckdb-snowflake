@echo off
REM Download ADBC headers from official releases

REM Create directory for headers
set HEADER_DIR=%~dp0..\src\include\arrow-adbc
if not exist "%HEADER_DIR%" mkdir "%HEADER_DIR%"

echo Downloading ADBC headers...

REM Download the main adbc.h header
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/apache/arrow-adbc/main/c/include/arrow-adbc/adbc.h' -OutFile '%HEADER_DIR%\adbc.h'"

REM Download the driver manager header
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/apache/arrow-adbc/main/c/include/arrow-adbc/adbc_driver_manager.h' -OutFile '%HEADER_DIR%\adbc_driver_manager.h'"

if exist "%HEADER_DIR%\adbc.h" (
    if exist "%HEADER_DIR%\adbc_driver_manager.h" (
        echo ADBC headers downloaded successfully to: %HEADER_DIR%
    ) else (
        echo Failed to download adbc_driver_manager.h
        exit /b 1
    )
) else (
    echo Failed to download adbc.h
    exit /b 1
)
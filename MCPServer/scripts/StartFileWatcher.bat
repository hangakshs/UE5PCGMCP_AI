@echo off
REM UE5 Forest Generation - File Watcher Service Startup Script
REM 이 스크립트를 실행하면 UE5와 통신하는 파일 감시 서비스가 시작됩니다.

echo ================================================================
echo    UE5 Forest Generation - File Watcher Service
echo    Starting standalone file communication service...
echo ================================================================
echo.

REM Python 가상환경 활성화 (있는 경우)
if exist ".venv\Scripts\activate.bat" (
    echo Activating virtual environment...
    call .venv\Scripts\activate.bat
) else if exist "venv\Scripts\activate.bat" (
    echo Activating virtual environment...
    call venv\Scripts\activate.bat
) else (
    echo No virtual environment found, using system Python
)

REM Python 버전 확인
python --version
echo.

REM 프로젝트 루트 경로 계산 (scripts 폴더의 2단계 상위 = 프로젝트 루트)
set PROJECT_ROOT=%~dp0..\..
pushd %PROJECT_ROOT%
set PROJECT_ROOT=%CD%
popd

echo Project Root: %PROJECT_ROOT%
echo.

REM 서비스 시작
echo Starting File Watcher Service...
echo Press Ctrl+C to stop the service
echo.
python src/file_watcher_service.py --project-root "%PROJECT_ROOT%"

REM 종료 시
echo.
echo Service stopped.
pause

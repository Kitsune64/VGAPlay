@echo off
setlocal

mode con: cols=80 lines=25

:: Set working directory to the directory containing this script
cd /d "%~dp0"

set "PNG_EXTRACT_BIN=PNGExtract.exe"
set "MPG2WAV_BIN=mpg2wav.exe"
set "FRAMEPACK_BIN=FramePack.exe"
set "VERIFICATOR_BIN=Verificator.exe"

echo ===============================================================================
echo                         FramePack Suite Pipeline 1.4
echo ===============================================================================

:: Check executables
if not exist "%PNG_EXTRACT_BIN%" goto msg_err_bin
if not exist "%MPG2WAV_BIN%" goto msg_err_bin
if not exist "%FRAMEPACK_BIN%" goto msg_err_bin
if not exist "%VERIFICATOR_BIN%" goto msg_err_bin
goto bin_ok

:msg_err_bin
echo Error: One or more executables (.exe) are missing in this directory.
goto error

:bin_ok

:: -------------------------------------------------------------------------------
:: Ask for video file
:: -------------------------------------------------------------------------------

:ask_mpg
set "mpg_file="
set /p "mpg_file=Enter video file name with extension (ex: video.mp4): "

if not defined mpg_file goto err_mpg_empty
if exist "%mpg_file%" goto mpg_exists

echo Warning: The file "%mpg_file%" does not exist.
set "response="
set /p "response=Do you want to continue anyway? (y/N): "

if /i "%response%"=="y" goto mpg_exists

echo Cancelled.
goto error

:err_mpg_empty
echo Error: File name cannot be empty.
goto ask_mpg

:mpg_exists

:: -------------------------------------------------------------------------------
:: Ask for PNG directory
:: -------------------------------------------------------------------------------

:ask_dir
set "png_dir="
set /p "png_dir=Enter target directory name for PNGs: "

if not defined png_dir goto err_dir_empty
goto dir_ok

:err_dir_empty
echo Error: Directory name cannot be empty.
goto ask_dir

:dir_ok

:: -------------------------------------------------------------------------------
:: Ask for output prefix
:: -------------------------------------------------------------------------------

:ask_prefix
set "output_prefix="
set /p "output_prefix=Enter output prefix (1-8 characters, without extension): "

if not defined output_prefix goto err_prefix

if not "%output_prefix:~8,1%"=="" goto err_prefix

echo(%output_prefix%| findstr /r "^[A-Za-z0-9_-][A-Za-z0-9_-]*$" >nul
if errorlevel 1 goto err_prefix

goto prefix_ok

:err_prefix
echo Invalid prefix.
echo Use 1 to 8 characters: letters, numbers, underscore or hyphen.
goto ask_prefix

:prefix_ok

:: -------------------------------------------------------------------------------
:: Ask for FPS
:: -------------------------------------------------------------------------------

:ask_fps
set "fps="
set /p "fps=Enter FPS rate (1 to 60): "

if not defined fps goto err_fps

echo(%fps%| findstr /r "^[0-9][0-9]*$" >nul
if errorlevel 1 goto err_fps

set /a fps_test=1%fps%-100 >nul 2>&1

if %fps_test% LSS 1 goto err_fps
if %fps_test% GTR 60 goto err_fps

set "fps=%fps_test%"
goto fps_ok

:err_fps
echo Invalid FPS. Please enter a number between 1 and 60.
goto ask_fps

:fps_ok

:: -------------------------------------------------------------------------------
:: Ask for maximum colors
:: -------------------------------------------------------------------------------

:ask_colors
set "max_colors="
set /p "max_colors=Enter max colors (1 to 256): "

if not defined max_colors goto err_colors

echo(%max_colors%| findstr /r "^[0-9][0-9]*$" >nul
if errorlevel 1 goto err_colors

set /a colors_test=max_colors >nul 2>&1

if %colors_test% LSS 1 goto err_colors
if %colors_test% GTR 256 goto err_colors

goto colors_ok

:err_colors
echo Invalid input. Please enter a number between 1 and 256.
goto ask_colors

:colors_ok

:: -------------------------------------------------------------------------------
:: Ask for padding mode
:: -------------------------------------------------------------------------------

:ask_padding
set "padding_mode="
set /p "padding_mode=Enter padding mode (0: disabled, 1: enabled): "

if "%padding_mode%"=="0" goto padding_ok
if "%padding_mode%"=="1" goto padding_ok

echo Invalid input. Please enter exactly 0 or 1.
goto ask_padding

:padding_ok

:: -------------------------------------------------------------------------------
:: Confirmation
:: -------------------------------------------------------------------------------

echo.
echo Warning: PNGExtract will write into the specified directory:
echo "%png_dir%"
echo.
echo Output files:
echo "%output_prefix%.WAV"
echo "%output_prefix%.BIN"
echo.

set "response="
set /p "response=Do you want to continue? (y/N): "

if /i "%response%"=="y" goto run_pipeline

echo Cancelled.
goto error

:: -------------------------------------------------------------------------------
:: STEP 1 - PNGExtract
:: -------------------------------------------------------------------------------

:run_pipeline

echo -------------------------------------------------------------------------------
echo Running PNGExtract...
echo -------------------------------------------------------------------------------

"%PNG_EXTRACT_BIN%" "%mpg_file%" "%png_dir%" "%fps%" "%max_colors%" "%padding_mode%"

if errorlevel 1 goto err_png_exec
goto step1_ok

:err_png_exec
echo Error during PNGExtract execution.
goto error

:step1_ok

:: -------------------------------------------------------------------------------
:: STEP 2 - mpg2wav
:: -------------------------------------------------------------------------------

echo -------------------------------------------------------------------------------
echo Running mpg2wav...
echo Output file: %output_prefix%.WAV
echo -------------------------------------------------------------------------------

"%MPG2WAV_BIN%" "%mpg_file%" "%output_prefix%"

if errorlevel 1 goto err_mpg2wav_exec
goto step2_ok

:err_mpg2wav_exec
echo Error during mpg2wav execution.
goto error

:step2_ok

:: -------------------------------------------------------------------------------
:: STEP 3 - FramePack
:: -------------------------------------------------------------------------------

echo -------------------------------------------------------------------------------
echo Running FramePack...
echo FramePack will automatically detect all PNG files.
echo Maximum colors per image: %max_colors%
echo Output file: %output_prefix%.BIN
echo -------------------------------------------------------------------------------

:: FramePack syntax:
:: FramePack <DIRECTORY> <PREFIX> <FPS> <MAX_COLORS>

"%FRAMEPACK_BIN%" "%png_dir%" "%output_prefix%" "%fps%" "%max_colors%"

if errorlevel 1 goto err_framepack_exec
goto step3_ok

:err_framepack_exec
echo Error during FramePack execution.
goto error

:step3_ok

:: -------------------------------------------------------------------------------
:: STEP 4 - Verificator
:: -------------------------------------------------------------------------------

echo -------------------------------------------------------------------------------
echo Running Verificator on %output_prefix%.BIN...
echo -------------------------------------------------------------------------------

"%VERIFICATOR_BIN%" "%output_prefix%.BIN"

if errorlevel 1 goto err_verif_exec
goto pipeline_success

:err_verif_exec
echo Error during Verificator execution.
goto error

:: -------------------------------------------------------------------------------
:: SUCCESS
:: -------------------------------------------------------------------------------

:pipeline_success

echo ===============================================================================
echo Pipeline completed successfully from start to finish!
echo ===============================================================================
goto end

:: -------------------------------------------------------------------------------
:: ERROR
:: -------------------------------------------------------------------------------

:error

echo ===============================================================================
echo Pipeline aborted due to an error.
echo ===============================================================================
pause
exit /b 1

:end
pause
exit /b 0

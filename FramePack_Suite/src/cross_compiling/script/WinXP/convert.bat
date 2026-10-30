@echo off
mode con: cols=80 lines=25

:: Force working directory to local to bypass space issues without quotes
cd /d %~dp0

set PNG_EXTRACT_BIN=PNGExtract.exe
set MPG2WAV_BIN=mpg2wav.exe
set FRAMEPACK_BIN=FramePack.exe
set VERIFICATOR_BIN=Verificator.exe

echo ===============================================================================
echo                           FramePack Suite Pipeline
echo ===============================================================================

if not exist %PNG_EXTRACT_BIN% goto msg_err_bin
if not exist %MPG2WAV_BIN% goto msg_err_bin
if not exist %FRAMEPACK_BIN% goto msg_err_bin
if not exist %VERIFICATOR_BIN% goto msg_err_bin
goto bin_ok

:msg_err_bin
echo Error: One or more executables (.exe) are missing in this directory.
goto error

:bin_ok

:ask_mpg
set mpg_file=
set /p mpg_file=Enter video file name with extension (ex:video.mp4): 
if "%mpg_file%"== "" goto err_mpg_empty
if exist %mpg_file% goto mpg_exists
echo Warning: The file %mpg_file% does not exist.
set response=
set /p response=Do you want to continue anyway? (y/N): 
if /i %response%==y goto mpg_exists
echo Cancelled.
goto error

:err_mpg_empty
echo Error: File name cannot be empty.
goto ask_mpg

:mpg_exists

:ask_dir
set png_dir=
set /p png_dir=Enter target directory name for PNGs: 
if "%png_dir%"== "" goto err_dir_empty


goto dir_ok

:err_dir_empty
echo Error: Directory name cannot be empty.
goto ask_dir

:dir_ok

:ask_fps
set fps=
set /p fps=Enter FPS rate (e.g., 15, 24, 30): 
if "%fps%"== "" goto err_fps_empty
goto fps_ok

:err_fps_empty
echo Error: FPS cannot be empty.
goto ask_fps

:fps_ok

:ask_colors
set max_colors=
set /p max_colors=Enter max colors (1 to 256): 
if "%max_colors%"== "" goto err_colors_invalid
set /a test_num=%max_colors% >nul 2>&1
if not %test_num%==%max_colors% goto err_colors_num
if %max_colors% LSS 1 goto err_colors_range
if %max_colors% GTR 256 goto err_colors_range
goto ask_padding

:err_colors_invalid
echo Invalid input.
goto ask_colors

:err_colors_num
echo Invalid number.
goto ask_colors

:err_colors_range
echo Must be between 1 and 256.
goto ask_colors

:ask_padding
set padding_mode=
set /p padding_mode=Enter padding mode (0: disabled, 1: enabled): 
if %padding_mode%==0 goto ask_padding_done
if %padding_mode%==1 goto ask_padding_done
echo Invalid input. Please enter exactly 0 or 1.
goto ask_padding
:ask_padding_done

echo Warning: PNGExtract will write into the specified directory.
set response=
set /p response=Do you want to continue anyway? (y/N): 
if /i %response%==y goto run_pipeline
echo Cancelled.
goto error

:run_pipeline

:: --- STEP 1: Run PNGExtract ---
echo -------------------------------------------------------------------------------
echo Running PNGExtract...
echo -------------------------------------------------------------------------------
%PNG_EXTRACT_BIN% %mpg_file% %png_dir% %fps% %max_colors% %padding_mode%
if errorlevel 1 goto err_png_exec
goto step1_ok

:err_png_exec
echo Error during PNGExtract execution.
goto error

:step1_ok

:: --- STEP 2: Run mpg2wav ---
echo -------------------------------------------------------------------------------
echo Running mpg2wav (Generating AUDIO.WAV)...
echo -------------------------------------------------------------------------------
%MPG2WAV_BIN% %mpg_file%
if errorlevel 1 goto err_mpg2wav_exec
goto step2_ok

:err_mpg2wav_exec
echo Error during mpg2wav execution.
goto error

:step2_ok

:: --- STEP 3: Auto-count PNGs in background ---
if exist %png_dir%\ goto png_dir_exists
echo Error: Target PNG directory %png_dir% not found.
goto error

:png_dir_exists
set fc=0
for /r %png_dir% %%f in (*.PNG) do (
    set /a fc+=1
)
if %fc% EQU 0 goto err_no_png
goto step3_ok

:err_no_png
echo Error: No PNG files found in %png_dir%.
goto error

:step3_ok

echo -------------------------------------------------------------------------------
echo Packager: Processing %fc% frames...
echo Running FramePack...
echo -------------------------------------------------------------------------------
%FRAMEPACK_BIN% %png_dir% VIDEO PNG %fc% %fps% %max_colors%
if errorlevel 1 goto err_framepack_exec
goto step4_ok

:err_framepack_exec
echo Error during FramePack execution.
goto error

:step4_ok

:: --- STEP 4: Run Verificator ---
echo -------------------------------------------------------------------------------
echo Running Verificator on VIDEO.BIN...
echo -------------------------------------------------------------------------------
%VERIFICATOR_BIN% VIDEO.BIN
if errorlevel 1 goto err_verif_exec
goto pipeline_success

:err_verif_exec
echo Error during Verificator execution.
goto error

:pipeline_success
echo ===============================================================================
echo Pipeline completed successfully from start to finish!
echo ===============================================================================
goto end

:error
echo ===============================================================================
echo Pipeline aborted due to an error.
echo ===============================================================================
pause
exit /b 1

:end
pause

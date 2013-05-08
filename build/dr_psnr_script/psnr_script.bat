@echo off

@echo ...

if not exist BasketballDrive_1920x1080_50.yuv GOTO OVER1

@echo Running encoder for BasketBallDrive: config all_I ...
@echo Basketball_all_I > dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c ..\..\cfg\per-sequence\BasketballDrive.cfg -c ..\..\cfg\encoder_all_I.cfg -b script.out > encoder_output.txt
@echo Running decoder for BasketBallDrive...
TAppDecoder.exe -b script.out -o script.yuv > decoder_output.txt
@echo Running dr_psnr for BasketBallDrive...
dr_psnr.exe -r BasketballDrive_1920x1080_50.yuv -c script.yuv -w 1920 -h 1080 -e 30 >> dr_psnr_output.txt

@echo Running encoder for BasketBallDrive: config I_15P ...
@echo Basketball_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c ..\..\cfg\per-sequence\BasketballDrive.cfg -c ..\..\cfg\encoder_I_15P.cfg -b script.out >> encoder_output.txt
@echo Running decoder for BasketBallDrive...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for BasketBallDrive...
dr_psnr.exe -r BasketballDrive_1920x1080_50.yuv -c script.yuv -w 1920 -h 1080 -e 30 >> dr_psnr_output.txt

@echo ...
GOTO FOURPEOPLE

:OVER1
@echo To run this script, BasketballDrive_1920x1080_50.yuv file should be available in \build\dr_psnr_script directory.
@echo ...

:FOURPEOPLE
if not exist FourPeople_1280x720_60.yuv GOTO OVER2

@echo Running encoder for FourPeople: config all_I ...
@echo Fourpeople_all_I >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c ..\..\cfg\per-sequence\FourPeople.cfg -c ..\..\cfg\encoder_all_I.cfg -b script.out >> encoder_output.txt
@echo Running decoder for FourPeople...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for FourPeople...
dr_psnr.exe -r FourPeople_1280x720_60.yuv -c script.yuv -w 1280 -h 720 -e 30 >> dr_psnr_output.txt

@echo Running encoder for FourPeople: config I_15P ...
@echo Fourpeople_I_15P >> dr_psnr_output.txt
..\vc11-x86_64\Release\x265-cli.exe -c ..\..\cfg\per-sequence\FourPeople.cfg -c ..\..\cfg\encoder_I_15P.cfg -b script.out >> encoder_output.txt
@echo Running decoder for FourPeople...
TAppDecoder.exe -b script.out -o script.yuv >> decoder_output.txt
@echo Running dr_psnr for FourPeople...
dr_psnr.exe -r FourPeople_1280x720_60.yuv -c script.yuv -w 1280 -h 720 -e 30 >> dr_psnr_output.txt

@echo ...
GOTO DONE

:OVER2
@echo To run this script, FourPeople_1280x720_60.yuv file should be available in \build\dr_psnr_script directory.
@echo ...

:DONE
@echo Done...

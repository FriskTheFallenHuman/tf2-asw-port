@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /tf +everything /mksln Game_TF.sln /2013
popd
@pause
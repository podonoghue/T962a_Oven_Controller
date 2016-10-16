@echo off

set STORE=My

set INF_2_CAT=C:\Apps\WinDDK\7600.16385.1\bin\selfsign\Inf2Cat.exe
set SIGNTOOL=C:\Apps\WinDDK\7600.16385.1\bin\amd64\SignTool.exe
set TIMESTAMP_URL=http://timestamp.verisign.com/scripts/timstamp.dll
set SHA=19B5A37F5AF7C74D9909519F16ED2B5AA53F1C10

echo Creating CAT files
%INF_2_CAT% /driver:%~dp0\Drivers /os:XP_X86,XP_X64,7_X86,7_X64,Vista_X86,Vista_X64

echo Signing CAT files
for %%f in (%~dp0\Drivers\*.cat) do %SIGNTOOL% sign /s %STORE% /t %TIMESTAMP_URL% /sha1 %SHA% %%f

pause
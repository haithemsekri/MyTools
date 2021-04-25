@echo off
 
set ip="xxxxx"

set mask="xxxxx"

set usr="xxxxx"

set pass="xxxxx"

@echo on 

echo %ip%  %mask%  %usr%  %pass% 

C:

cd "C:\Program Files (x86)\NirSoft\wakemeonlan-x64"


:repeat

WakeMeOnLan.exe  /wakeup %ip%  20000 %mask%

(ping -n 1 %ip%  | find "TTL=") || goto :repeat

echo Success!

cmdkey /delete:%ip%

cmdkey /generic:%ip% /user:%usr% /pass:%pass% 

cmdkey /list:%ip%

mstsc /v:%ip%  /control


@echo off
tcc.exe  -o QLForth.exe ..\src\main.c ..\src\qlforth.c ..\src\compiler.c ..\src\code.c ..\src\simulator.c
QLForth.exe ..\testcase\QLForth.QLF
@echo on
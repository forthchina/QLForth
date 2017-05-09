@echo off
tcc.exe -Iinc -o bin\QLForth.exe src\main.c src\qlforth.c src\compiler.c src\code.c src\simulator.c
bin\QLForth.exe testcase\QLForth.QLF
@echo on
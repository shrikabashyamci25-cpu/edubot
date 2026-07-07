@echo off
C:\msys64\mingw64\bin\gcc.exe -Wall -std=c99 -o edubot_sim.exe main.c nlp_engine.c knowledge_base.c dialogue_manager.c simulation.c -lm > compile_sim_out.txt 2>&1
echo Exit code: %ERRORLEVEL% >> compile_sim_out.txt

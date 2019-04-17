# Aimbot
First attempt at reading and manipulating game data through process memory.
First game was using AssaultCube, got the basics of finding player memory, locations , math to get vectors.
Currently working on a CSGO external hack.


MemReader.cpp 

Game: AssaultCube
Aimbot - Auto target closest enemy Entity player's head
ESP    - Projects square around target players using World To Screen calcuations
Godmode- Infinite HP + Ammo by writing to players memory.


baba_yaga.cpp 

A Mmuch more refined aimbot.

External hack for "Counterstrike: Global Offensive"
Current Capabilitys:
ESP: 
-Illuminate all enemy entites
-Illuminate all friendly entites

Aimbot:
- Smooth lerp between vectors to stop snapping
- Visibility check, only targets entitys in vision of player

Working on 
- Recoil manager

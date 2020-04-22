# Blind Jump Portable

## Introduction

Blind Jump is an action/adventure game. Evan Bowman started the project in late 2015 while in college, and resumed work on the code after a long hiatus, during which he was teaching himself traditional oil painting.

The game uses procedural algorithms to generate the levels, so the game changes each time you play. While the game is designed to be difficult, it will also be possible to beat the whole game in under an hour. At time of writing, the game has one boss after the first ten levels, but there will be a boss fight after every set of ten levels. Enemies and environments change after each boss fight.

<p align="center">
  <img src="screen1.png"/>
</p>

## Implementation

This repository contains a substantial re-write of the original BlindJump code. In the current iteration, the core parts of the code have no external dependencies, and target a [theoretically] platform-neutral hardware abstraction layer. The embedded version of BlindJump runs on Gameboy Advance, and the game can easily support desktop OSs, although most of the graphics reoutines are yet unimplemented for desktop platforms. When porting the game to a new platform, one only needs to re-implement the Platform class in source/platform/.

## Building

The project should be pretty easy to compile, but if you have trouble, please try using the attached docker container before emailing me with operating system questions.
```
docker pull ubuntu:18.04
sudo docker build -t blind_jump_build .
sudo docker run -it blind_jump_build
make
```

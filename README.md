<a href="https://scan.coverity.com/projects/evanbowman-blind-jump-portable">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/21790/badge.svg"/>
</a>

# Blind Jump (portable)

You should find this readme mosty up-to-date, but as the game is under active development, everything is subject to change, and sometimes edits to the readme lag significantly behind changes to the game itself.

<details>
  <summary> <b>Game Preview GIF</b> (warning: contains flashing lights and colors)</summary>
  <img src="imgs_for_readme/header.gif"/>
</details>


## Contents
<!--ts-->
   * [Contents](#contents)
   * [Introduction](#introduction)
   * [Gameplay](#gameplay)
      * [Controls](#controls)
      * [Multiplayer](#multiplayer)
   * [Implementation](#implementation)
      * [Scripting](#scripting)
   * [Building](#building)
<!--te-->

## Introduction

Blind Jump is a simple action/adventure game. Evan Bowman started the project in late 2015 while in college, and resumed work on the code after a several year long hiatus, during which he was teaching himself oil painting. The game is designed to run on a wide variety of platforms, from macOS to the Nintendo Gameboy Advance.

The game uses procedural algorithms to generate the levels, so the level designs change each time you play. While the game is designed to be difficult, it will also be possible to beat the whole game in under an hour. At time of writing, the game has two bosses after each of the first ten levels, with twenty more levels currently in development. Enemies and environments change after each boss fight.

<p align="center">
  <img src="imgs_for_readme/s1.png"/>
</p>

<p align="center">
  <img src="imgs_for_readme/s2.png"/>
</p>

<p align="center">
  <img src="imgs_for_readme/s5.png"/>
</p>

<p align="center">
  <img src="imgs_for_readme/s3.png"/>
</p>

<p align="center">
  <img src="imgs_for_readme/s4.png"/>
</p>

## Gameplay

### Controls

On the gameboy advance, use the d-pad to move, the A button to shoot, and the B button to interact with things and to select items and options in the game's menus. If you hold the A button while walking, you will shoot repeatedly, and also the player will strafe.

Hold the right shoulder button, or the left shoulder button, to open up a quick item selector menu, or a minimap, respectively.

<img src="imgs_for_readme/item_quick_select.gif"/>
<img src="imgs_for_readme/quick_map.gif"/>

To access all for your items, press select. Press start for more options.

### Multiplayer

<img src="imgs_for_readme/multiplayer_connect.gif"/>

The game supports multiplayer over the gameboy advance's serial port. While certainly incomplete, you should find the multiplayer mode to be more-or-less playable.
To enable, press the start button on both devices, and then select the "Connect Peer" option within twenty seconds. You need to be on the very first level to connect a peer, otherwise the option will be grayed-out. If running on an actual GAMBOY ADVANCE, you may need to select "Connect Peer" on the device plugged into the gray end of the link cable first, followed by the device connected to the smaller purple end of the link cable. This is a known issue, and I am still working on resolving this, at time of writing.

## Implementation

This repository contains a substantial re-write of the original BlindJump code. In the current iteration, the core parts of the code have no external dependencies, and target a [theoretically] platform-neutral hardware abstraction layer. The embedded version of BlindJump runs on Gameboy Advance, and has partial support for Desktop OSs via SFML. When porting the game to a new platform, one only needs to re-implement the Platform class in source/platform/.

The game is written almost entirely in C++, along with a small amount of C, a custom dialect of LISP, as well as a teeny bit of ARM assembly.

### Scripting

BlindJump uses a custom LISP dialect for lightweight scripting. The init.lisp script offers some further usage tips, but generally, our LISP implementation supports functions as first class values, functional currying, most of the common builtins, like map, cons, list, etc., variadic functions, and many more features. Our interpreter does not support lambdas, although technically I think you could create your own lambdas via abuse of the builtin functional currying, and the eval keyword. But custom function definitions are not a goal of the script interface. BlindJump executes a number of scripts for various scenarios--you can think of the scripts as analogous to git hooks. In the settings menu, the game allows you to launch a repl while the game is running, to manipulate game state (for gameboy builds, the game will render an onscreen keyboard and console, for desktop builds, the game will pause, and read from the command line).

## Building

The project should be pretty easy to compile, but if you have trouble, please try using the attached docker container before emailing me with operating system questions.
```
docker pull ubuntu:18.04
sudo docker build -t blind_jump_build .
sudo docker run -it blind_jump_build
make
```

NOTE: you can also get a containerized build environment from the link below, although you'll have to remember to run `git pull` when entering the container, because I built the container with a frozen version of the repository. If this is inconvenient for you, feel free to build the container yourself using the steps above.

https://hub.docker.com/r/evanbowman/blind_jump_build

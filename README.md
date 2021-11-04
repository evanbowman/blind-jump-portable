<a href="https://scan.coverity.com/projects/evanbowman-blind-jump-portable">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/21790/badge.svg"/>
</a>  

<p align="center">
  <img src="imgs_for_readme/heading.png"/>
</p>

<p align="center">
  <img src="imgs_for_readme/s7.png"/>
</p>


# Blind Jump (portable)


You should find this readme mosty up-to-date, but as the game is under active development, everything is subject to change, and sometimes edits to the readme lag significantly behind changes to the game itself.

<b>Game Preview GIF</b>


<img src="imgs_for_readme/header.gif"/>

## Contents
<!--ts-->
   * [Contents](#contents)
   * [Introduction](#introduction)
   * [Gameplay](#gameplay)
      * [Controls](#controls)
      * [Multiplayer](#multiplayer)
      * [Secrets](#secrets)
   * [Implementation](#implementation)
   * [Building](#building)
   * [Contributing](#contributing)
      * [Music](#music)
   * [Localization](#localization)
   * [Security](#security)
   * [Downloads](#downloads)
   * [Feedback](#feedback)
   * [Licence](#licence)
   * [LISP](#lisp)
      * [Library](#library)
      * [API](#api)
      * [Example](#example)
<!--te-->

## Introduction

<p align="center">
  <img src="imgs_for_readme/s8.png"/>
</p>

Blind Jump is a simple action/adventure game, featuring procedurally generated level maps, collectible items, fluid animation, and meticulously designed color palettes. Evan Bowman started the project in late 2015 while in college, and resumed work on the code after a several year long hiatus, during which he was teaching himself oil painting. Originally developed for macOS, the game was rebuilt from scratch during the 2020 pandemic, with a new custom engine. Blind Jump now compiles for various other platforms, including Linux, Windows, Nintendo Gameboy Advance, and Sony PlayStation Portable.

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

[Contents↑](#contents)

## Gameplay

### Controls

Use the directional buttons or the analog stick to move, the B/X button to shoot, and the A/O button to interact with things and to select items and options in the game's menus. If you hold the B button while walking, you will shoot repeatedly, and also the player will strafe. Tap A while walking to dodge.

Hold the right shoulder button, or the left shoulder button, to open up a quick item selector menu, or a minimap, respectively.

Some people prefer to have the strafe and shoot mapped to different keys. For an alternate button mapping, where A=strafe, B=shoot, and Left_Bumper=dodge, switch the strafe key setting to separate in the settings menu.

<img src="imgs_for_readme/item_quick_select.gif"/>
<img src="imgs_for_readme/quick_map.gif"/>

To access all for your items, press select. Press start for more options.

### Multiplayer

<img src="imgs_for_readme/multiplayer_connect.gif"/>

The GBA edition of the game supports multiplayer over the gameboy advance's serial port. While certainly incomplete, you should find the multiplayer mode to be more-or-less playable. The PSP edition does not support multiplayer at this time.
To enable, press the start button on both devices, and then select the "Connect Peer" option within twenty seconds. You need to be on the very first level to connect a peer, otherwise the option will be grayed-out. If running on an actual GAMBOY ADVANCE, you may need to select "Connect Peer" on the device plugged into the gray end of the link cable first, followed by the device connected to the smaller purple end of the link cable. This is a known issue, and I am still working on resolving this, at time of writing.

### Secrets

#### Boss Rush Mode

With the title screen open, press select fifteen times, and the game will enter a boss-rush mode. Note that entering boss-rush mode is equivalent to starting a new game, and you will lose any items that you may have collected.

Technically, you may also enter boss-rush mode at any time, by opening the script console (see below), and executing the command: `(set 'debug-mode 7)`. The next transporter will then take the player to a boss.

#### Script Console

Open the pause screen, and press the left bumper repeatedly, and a menu option for a script console will appear in the pause menu. The console gives you access to the game's lisp interpreter, allowing you to manipulate certain variables, add items, jump to levels, etc. See [LISP](#lisp) for more info about the lisp dialect. Press A to enter text, and B for backspace (B will also exit the console if the text entry is empty). Press start to enter a command, and press L to open a list of variable autocomplete options. If the completion list is open, you may press A to select a completion, or B to cancel out of the autocomplete. The intrepreter will highlight excessive closing parentheses in red, making it easier to keep track of parens for really long lines where you cannot see the beginning of the command.

<img src="imgs_for_readme/console.gif"/>

#### UART Interface

In addition to providing an on-screen script console, the Blind Jump GBA edition also supports a scripting interface via UART. To connect your personal computer to the Gameboy Advance's serial port, you will need to splice an RS232 cable into a Gameboy Advance link cable, such that:
```
SO  --------> 5 RxD
SI  <-------- 4 TxD
GND <-------> 1 GND
```

Connect the two devices, set your PC's baud rate the standard 9600 Hz, and turn on the gameboy advance. If you've never used a UART console, try picocom, which is known to work.


[Contents↑](#contents)

## Implementation

This repository contains a substantial re-write of the original BlindJump code. In the current iteration, the core parts of the code have no external dependencies, and target a [theoretically] platform-neutral hardware abstraction layer. This embedded version of BlindJump runs on Gameboy Advance, PSP, and has partial support for Desktop OSs via SFML. When porting the game to a new platform, one only needs to re-implement the Platform class in source/platform/.

The game is written almost entirely in C++, along with a small amount of C, a custom dialect of LISP, as well as a teeny bit of ARM assembly.

[Contents↑](#contents)

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

[Contents↑](#contents)

## Contributing

The best way to help this project along, would be to download and play the game. All suggestions for improvements are welcome.

### Music

Because adding new music to the GBA version of the game is tedious/complicated, we should take a moment to describe the methodology for formatting and compiling sound files. The Gameboy Advance hardware more-or-less requires digital audio data to be formatted as 8-bit signed wav files. To add new sound files to the project, the data will need to be converted to headerless 8-bit signed PCM, at 16kHz. Then, the data will need to be converted to an ASM file, and compiled into the project. Evan Bowman hacked together a tool for this, available here: https://github.com/evanbowman/raw_to_asm. The raw_to_asm tool should be run from the BlindJump project root. The music/sound file should be exported to the required raw format, with a .raw extension, and placed in the sounds/ directory. Then, you should run raw_to_asm, with the name of the sound file, minus the .raw extension (`raw_to_asm music`, where music.raw exists in sounds). The program will write header and source files containing an assembler representation of the music data to the source/data/ directory. You will need to add the new source file to CMakeLists.txt, and add some new declarations for the sound or music data to gba_platform.cpp. Also, remember that GBA cartridges should be no larger than 32mb

[Contents↑](#contents)

## Localization

The BlindJump text engine supports localized character sets via utf-8 encoding; however, the game does not include every possible utf-8 codepoint in its character map (see the charset image in images/). BlindJump currently supports alpha-numeric glyphs for English, a few hundred glyphs for Chinese words, some extended glyphs for Italian and French, Cyrillic characters for Russian, and a minimal Japanese Katakana charset (including accent marks). To add a new language to the game, you would need make a couple of changes:
1) Create a new file, <language_name>.txt, in the strings/ directory. For the sake of simplicity, you may want to just make a copy of one of the existing files in the strings/ folder.
2) Define a translation, by translating each string in the newly created file to the desired language. Make sure you test the translation thoroughly by actually playing the game, to verify that you are not accidentally using strings that are too long, which may cause graphical glitches. Remember that the Gameboy Advance can display 30 characters horizontally, so for any strings that the game's UI does not reflow onto the next line, your translated text may be either truncated, or in some cases, the game will write an error to the logs indicating that the program was unable to display the excessively long text, and halt execution (freeze).
3) Next, find the definition of the `languages` list variable in scripts/init.lisp, and add a symbol corresponding to the name of the newly added language file (minus the .txt extension, see init.lisp for examples).
4) Lastly, you will need to explicitly link your strings file to the build system in CMakeLists.txt. Because the Gameboy Advance version of the game does not run in hosted environments, there is no filesystem, so the build system needs to know about your file, in order to copy its contents into the ROM.

Currently, Blind Jump includes built-in support for the following languages:
* English
* Chinese
* Russian
* Italian
* French

[Contents↑](#contents)

## Security

I periodically submit all of this project's source code to the Coverity static analyzer, which checks for buffer overruns, memory corruption, etc. Currently, BlindJump is 100% defect free. Let's keep it that way.

[Contents↑](#contents)

## Downloads

See the [releases section](https://github.com/evanbowman/blind-jump-portable/releases) of this github remote repo, where you may download the Gameboy Advance ROM directly (files with a .gba extension), and play the game with a flash cartridge (e.g. Everdrive). You could also use an emulator, although I personally prefer to play gameboy advance games on the actual device. You will find ROMs attached to each release, going all the way back to 2019, so you could also download earlier ROMs to see how the project changed as new code was introduced. I used the Linux/Windows/Mac versions of the game during development, but the desktop releases have fallen a bit out of date and may no longer work. I have not put much effort into maintaining the Desktop PC build targets, as gba emulators exist for so many different platforms, making the GBA rom very portable on its own.

[Contents↑](#contents)

## Feedback

Have an opinion about the game, or suggestions for improvements? I encourage you to create an issue, or to leave a comment over [here](https://evanbowman.itch.io/blindjump).

[Contents↑](#contents)

## Licence

Most of the code in this project is available under the MIT licence. The Gameboy Advance and PlayStation Portable builds of the project reference code from external/libgba and glib2d (respectively), which are GPL projects. Consequently, the Gameboy Advance and PSP builds must be distributed under the terms of the GPL licence. The game is already open source, so this makes little difference, just worth mentioning, for people reading the code. As the Desktop builds of the project do not reference the gpl'd code in any way, the desktop builds are not GPL.

All of images and character designs belong to Evan Bowman, and should not be used for commercial purposes without permission. The music belongs to various composers. Most of the music tracks are Creative Commons and merely require attribution (see the game's ending credits), but a few of the music tracks may not allow distribution in a commercial context. These tracks would need to be replaced, if copies of the game were to be sold.

## LISP

BlindJump uses a custom LISP dialect for lightweight scripting. In the settings menu, the game allows you to launch a repl while the game is running, to manipulate game state (for gameboy builds, the game will render an onscreen keyboard and console, for desktop builds, the game will pause, and read from the command line). See below for a brief overview of the language. If you've used Scheme, many things will feel familiar, although you'll also find some notable differences.

### Language

#### S-Expressions
Like All LISP languages, our interpreter represents code using a notation called S-Expressions. Each S-Expression consists of a list of space-separated tokens, enclosed with parentheses. For example:
```LISP
(dog cat bird) ;; This is an S-Expression

(1 2 cat 5 pigeon) ;; This is also an S-Expression

() ;; So is this.
```
When you enter an S-Expression, the first element in the list will be interpreted as a function, and the remaining elements will be supplied to the function as arguments.
```
(+ 1 2 3) ;; Result: 6
```
In some scenarios, you do not want an S-Expression to be evaluated as a function. For example, you may simply want a list of data. To prevent evaluation of an S-Expression, prefix the expression with a `'` character.
```
;; NOTE: without the quote character, the interpreter would have tried to call a function called dog,
;; with the arguments cat and 3.
'(dog cat 3) ;; Result: '(dog cat 3).
```

#### Variables

To declare a variable, use the `set` function, along with a variable name symbol, followed by a value:
```
(set 'i 5)
```
Now, variable `i` stores the value `5`, so `5` will be substituted wherever you type the variable `i`:

```
(+ i i) ; -> 10
```


#### Data types

##### Nil
The simplest data type in our language. Represents nothing. Represented by the `nil` keyword, or by the empty list `'()`.

##### Integer
A 32 bit signed integer. For example; `0`, `1`, or `2`. Due to the way that our interpreter tokenizes variable names, there is currently no way to specify negative numbers, although you can work around the issue by subtracting from zero, e.g.: `(- 0 5)` for -5.

##### Cons Pair
Arguably the most important data type in our language. Implements a pair of two values. To create a pair, invoke the `cons` function, with two arguments:
```
(cons 1 2) ;; -> '(1 . 2)
```
LISP interpreters represent lists as chains of cons pairs, terminated by nil. For example:
```
(cons 1 (cons 2 (cons 3 nil))) ;; -> '(1 2 3)
```
Or more simply, you could just use the `list` function:
```
(list 1 2 3) ;; -> '(1 2 3)
```

##### Functions (lambdas)
A function, or lambda, contains re-usable code, to be executed later. To declare a lambda, use the `lambda` keyword:
```
(lambda
  (cons 1 2))
```
You may want to store the lambda in a variable, so that you can use it later:
```
(set 'my-function 
  (lambda
    (cons 1 2)))
```
Now, when you invoke my-function, the code you wrote within the lambda expression will be evaluated:
```
(my-function) ;; -> '(1 . 2)
```
But for a function to be really useful, you'll want to be able to pass parameters to it. Within a function, you may refer to function arguments with the $ character, followed by the argument number, starting from zero. For example:
```
(set 'temp
  (lambda
    (cons $0 $1)))
    
(temp 5 6) ;; -> '(5 . 6)
```
You may also refer to an argument with the `arg` function, e.g. `(arg 0)` or `(arg 5)`. To ask the interpreter how many arguments the user passed to the current function, you may use the `argc` function.

To refer to the current function, invoke the `this` function.
```
((lambda) ((this))) ;; Endless loop, because the function repeatedly invokes itself.
```
(above snippet): In mathematical theory, we call a function defined in terms of itself a recursive function. After cons pairs, you might call recursion the second most import concept in understanding LISP.

##### Strings

A sequence of characters. Declare a string with by enclosing characters in quotes "".

##### Symbols

Declare a symbol by prefixing a word with the quote character, `'abc`. 

### Library

Blind Jump LISP provides two sets of functions:
* Builtin library functions
* Builtin engine API functions

The builtin library functions implement useful generic algorithms, while the engine API functions provide methods for altering game state. Often, you'll find yourself using the builtin library functions in conjunction with the engine API functions. First, we'll cover the builtin library functions.

#### set
`(set <symbol> <value>)`
Set a global variable described by `<symbol>` to `<value>`. 
For example:
```
(set 'a 10) ;; a now holds 10.
```
#### cons
`(cons <first> <second>)`
Construct a pair, from parameters `<first>` and `<second>`.
```
(cons 1 'fish) ;; -> '(1 . fish)
```
#### car
`(car <pair>)`
Retrieve the first element in a cons pair.
```
(car '(1 . fish)) ;; -> 1
```
#### cdr
`(cdr <pair>)`
Retrieve the second element in a cons pair.
```
(cdr '(1 . fish)) ;; -> 'fish
```

#### list
`(list ...)`
Construct a list containing all parameters passed to the list function.
```
(list 1 2 3 4) ;; -> '(1 2 3 4)
```

#### arg
`(arg <integer>)`
Retrieve an arument passed to the current function.

#### progn
`(progn ...)`
Return the value of the last parameter passed to progn.

#### not
`(not <value>)`
Return `1` if `<value>` is `0` or `nil`. Return `0` otherwise.

#### equal
`(equal <value> <value)`
Return `1` if both parameters are equivalent. Return `0` otherwise.

#### +
`(+ ...)`
Add all integers passed to `+`.

#### apply
`(apply <function> <list>)`
Invoke `<function>`, passing each argument in `<list>` as a parameter to `<function>`.
```
(apply + '(1 2 3 4 5)) ;; -> 15
(apply cons '(1 2)) ;; '(1 . 2)
```

#### fill
`(fill <integer> <value>)`
Create a list with `<integer>` repetitions of `<value>`.
```
(fill 6 5) ;; -> '(5 5 5 5 5 5)
(fill 4 'cat) ;; -> '(cat cat cat cat)
```

#### gen
`(gen <integer> <function>)`
Generate a list of `<integer>` numbers, by invoking `<function>` on each number from `0` to `<integer>`.
```
(gen 5 (lambda (* 2 $0)) ;; -> '(0 2 4 6 8)
```

####  length
`(length <list>)`
Returns the length of a list.
```
(length (list 0 0)) ;; -> 2
```

#### <
`(< <integer_1> <integer_2>)`
Return `1` if `<integer_1>` < `<integer_2>`. Return `0` otherwise.

#### >
See `<`

#### -
`(- <integer> <integer>)`
Subtract two integers.

#### * 
`(* ...)`
Multiply all integer arguments together.

#### /
`(/ <integer> <integer>)`
Divide two integers.

#### interp-stat
`(interp-stat)`
Return a lisp representing the LISP interpreter's internal state variables.

#### range
`(range <integer:begin> <integer:end> <integer:incr>)`
Generate a list of integers from begin to end, incrementing by incr.

#### unbind
`(unbind <symbol>)`
Delete a global varaible. Be careful!

#### symbol
`(symbol <string>)`
Create a symbol from a string.

#### string
`(string ...)`
Create a string by converting all arguments to strings and concatenating them.

#### bound
`(bound <symbol>)`
Return `1` if a variable exists for a symbol, `0` otherwise.

#### filter
`(filter <function> <list>)`
Return the subset of `<list>` where `<function>` returns 1 when invoked with elements of `<list>`.
```
(filter (lambda (< $0 5)) '(1 2 3 4 5 6 7 8)) ;; '(1 2 3 4)
```

#### map
`(map <function> <list>)`
Return a list representing the result of calling `<function>` for each element of `<list>`.
```
(map (lambda (+ $0 3)) (range 0 10)) ;; '(3 4 5 6 7 8 9 10 11 12)
```
NOTE: You may pass multiple lists to map, and multiple elements, one from each list, will be passed to `<function>`
```
(map (lambda (+ $0 $1)) (range 0 10) (range 10 20)) ;; '(10 12 14 16 18 20 22 24 26 28)
```

#### reverse
`(reverse <list>)`
Reverse a list.

#### gc
`(gc)`
Manually run the garbage collector. If you do not know what garbage collection is, do not worry about this function, as it runs automatically anyway.

#### get
`(get <list> <integer>)`
Retrieve element from `<list>` at index `<integer>`.

#### eval
`(eval <list>)`
Evaluate data as code.
```
(eval '(+ 1 2 3)) ;; 6

(eval (list '+ 1 2 3)) ;; 6
```

#### this
`(this)`
Return the currently executing function. Only makes sense to call `this` within a lambda.

#### compile
`(compile <lambda>)`
Compiles a lisp function to bytecode, and returns the new function. Bytecode functions run faster and take up less space than lisp functions.

#### disassemble
`(disassemble <lambda>)`
Disassemble a bytecode function, allowing you to inspect the bytecode. By default, writes the output to UART, you you will not see anything if you run this function in the on-screen script console.

[Contents↑](#contents)

### API

#### make-enemy
`(make-enement <integer:enemy-type> <integer:x> <integer:y>)`
Create enemy at x, y. See enemy- variables in the autocomplete window for available options.

#### level
`(level <integer>)`
When invoked with an integer parameter, sets the current level number. When invoked with no parameters, returns the current level number.

#### add-items
`(add-items ...)`
Adds each parameter to the inventory.
```
(add-items item-accelerator item-lethargy)

(apply add-items (range 0 30)) ;; add item ids from range 0 to 30 to the inventory.
```

#### get-hp
`(get-hp <entity>)`
Return health for an entity. (NOTE: call `(enemies)` to list enemy ids, or specify `player`).

#### set-hp
`(set-hp <entity> <integer>)`
Set entity's health to `<integer>`.

#### kill
`(kill <entity>)`
Set an entity's health to zero.

#### get-pos
`(get-pos <entity>)`
Return a pair of `'(x . y)`, representing an entity's position.

#### set-pos
`(set-pos <entity> <integer> <integer>)`
Set an entity's position.

#### enemies
`(enemies)`
Return a list of all enemies in the game.
```
;; For example
(map kill (enemies)) ;; kill all enemies
(map get-pos (enemies)) ;; get positions for all enemies
```

[Contents↑](#contents)


### Example
Here's a concise little implemenation of merge sort, using the language features described above.
```lisp
(set 'bisect-impl
     (compile
      (lambda
        (if (not $1)
            (cons (reverse $2) $0)
          (if (not (cdr $1))
              (cons (reverse $2) $0)
            ((this)
             (cdr $0)
             (cdr (cdr $1))
             (cons (car $0) $2)))))))

(set 'bisect (lambda (bisect-impl $0 $0 '())))

(set 'merge
     (compile
      (lambda
        (if (not $0)
            $1
          (if (not $1)
              $0
            (if (< (car $0) (car $1))
                (cons (car $0) ((this) (cdr $0) $1))
              (cons (car $1) ((this) $0 (cdr $1)))))))))


(set 'sort
     (lambda
       (if (not (cdr $0))
           $0
         (let ((temp (bisect $0)))
           (merge (sort (car temp))
                  (sort (cdr temp)))))))
```
[Contents↑](#contents)

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


Ce fichier devrait être à jour, mais comme le jeu est en cours de développement, tout est susceptible d'être modifié. La mise à jour de ce fichier peut parfois être très en retard par rapport aux changements apportés au jeu lui-même.

<b>GIF d'aperçu du jeu</b>


<img src="imgs_for_readme/header.gif"/>

## Contents
<!--ts-->
   * [Contenu](#contents)
   * [Introduction](#introduction)
   * [Gameplay](#gameplay)
      * [Commandes](#controls)
      * [Multijoueur](#multiplayer)
      * [Secrets](#secrets)
   * [Implémentation](#implementation)
   * [Synthèse](#building)
   * [Contribuer](#contributing)
      * [Musique](#music)
   * [Traduction](#localization)
   * [Sécurité](#security)
   * [Téléchargements](#downloads)
   * [Commentaires](#feedback)
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

Blind Jump est un jeu d'action/aventure simple, comportant des cartes générées de manière procédurale, des objets à collectionner, une animation fluide et des palettes de couleurs choisies méticuleusement. Vous êtes appelé à reprendre la main sur des stations en orbite autour de la Terre qui ont été décimées sans prévenir... Evan Bowman a commencé le projet à la fin de l'année 2015 alors qu'il était à l'université, et a repris le travail sur le code après une pause de plusieurs années (au cours de laquelle il apprenait la peinture à l'huile par lui-même). Développé à l'origine pour macOS, le jeu a été reconstruit de zéro pendant la pandémie de 2020, avec un nouveau moteur personnalisé. Blind Jump peut désormais être compilé pour diverses autres plateformes, notamment Linux, Windows, Nintendo Gameboy Advance et Sony PlayStation Portable.

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

[Contenu↑](#contents)

## Gameplay

### Commandes

Utilisez la croix directionnelle ou le stick analogique pour vous déplacer, le bouton B/X pour tirer et le bouton A/O pour interagir avec les objets et pour sélectionner des éléments et des options dans les menus du jeu. Si vous maintenez le bouton B enfoncé tout en marchant, vous tirerez de manière répétée et le joueur pourra également faire des pas de côté. Appuyez sur A en marchant pour esquiver.

On peut également changer les commandes dans le menu Options pour permettre de séparer le pas de côté et le tir. En choisissant « Touche Visée : séparée » : A=esquive, B=tir, et Bouton L=pas de côté. 

Maintenez le bouton R enfoncé pour ouvrir un menu de sélection rapide d’objets, ou le bouton L enfoncé pour ouvrir une carte mini map. Dans le cas où le bouton L est assigné au pas de côté, la carte est accessible par le menu Select.

<img src="imgs_for_readme/item_quick_select.gif"/>
<img src="imgs_for_readme/quick_map.gif"/>

Pour accéder à votre inventaire, appuyez sur Select. Appuyez sur Start pour accéder aux options.

### Multijoueur

<img src="imgs_for_readme/multiplayer_connect.gif"/>

L'édition GBA du jeu prend en charge le multijoueur via le port série de la Gameboy Advance. Le mode multijoueur devrait être plus ou moins jouable bien qu’il soit encore expérimental. L'édition PSP ne supporte pas le multijoueur pour le moment.
Pour l'activer, appuyez sur Start sur les deux appareils, puis sélectionnez l'option "Connexion Coéquipier" dans les vingt secondes. Vous devez être au tout premier niveau pour connecter un coéquipier, sinon l'option sera grisée et inaccessible. Attention, si vous utilisez un câble officier Gameboy Advance, il se peut que vous deviez d’abord sélectionner l’option « Connexion Coéquipier » sur la console connectée à l’extrémité grise, puis ensuite sur la console connectée à l’extrémité violette. Je suis toujours en train de travailler sur ce problème. 

### Secrets

#### Mode Boss Rush

Au niveau de l’écran titre, appuyez quinze fois sur la touche Select pour entrer dans le mode Boss Rush. Notez que lancer le mode Boss Rush équivaut à commencer une nouvelle partie, et vous perdrez tous les objets que vous avez pu ramasser.

Techniquement, vous pouvez aussi passer en mode boss-rush à tout moment, en ouvrant la console de script (voir ci-dessous), et en exécutant la commande : (set 'debug-mode 7). La prochaine porte emmènera alors le joueur vers un boss.

#### Console de Script

Mettez le jeu en pause (Start), et appuyez sur le bouton L à plusieurs reprises : une nouvelle entrée apparaît dans le menu permettant de lancer la console de script. La console vous donne accès à l'interpréteur LISP du jeu, ce qui vous permet de manipuler certaines variables, d'ajouter des objets, de sauter des niveaux, etc. Voir [LISP](#lisp) pour plus d'informations sur le dialecte lisp. Appuyez sur A pour saisir du texte, et sur B pour le retour arrière (B quittera également la console si l'entrée de texte est vide). Appuyez sur Start pour entrer une commande, et sur L pour ouvrir une liste d'options de complétion automatique des variables. Si la liste de complétion est ouverte, vous pouvez appuyer sur A pour sélectionner une complétion, ou sur B pour annuler la complétion automatique. L'interpréteur met en évidence les parenthèses fermantes excessives en rouge, ce qui facilite le suivi des parenthèses pour les lignes très longues où vous ne pouvez pas voir le début de la commande.

<img src="imgs_for_readme/console.gif"/>

#### Interface UART

En plus de fournir une console de script à l'écran, l'édition GBA de Blind Jump supporte également une interface de script via UART. Pour connecter votre ordinateur personnel au port série de la Gameboy Advance, vous devrez raccorder un câble RS232 à un cable link de la Gameboy Advance, de la manière suivante :
```
SO  --------> 5 RxD
SI  <-------- 4 TxD
GND <-------> 1 GND
```

Connectez les deux appareils, réglez le débit en bauds de votre PC sur le standard 9600 Hz, et allumez la Gameboy Advance. Si vous n'avez jamais utilisé une console UART, essayez picocom, qui fonctionne bien.


[Contenu↑](#contents)

## Implémentation

Ce dépôt contient une réécriture substantielle du code original de BlindJump. Dans l'itération actuelle, les parties centrales du code n'ont pas de dépendances externes, et visent une couche d'abstraction matérielle [théoriquement] indépendante de la plate-forme. Cette version embarquée de BlindJump fonctionne sur Gameboy Advance, PSP, et a un support partiel pour les OS de bureau via SFML. Lors du portage du jeu sur une nouvelle plateforme, il suffit de réimplémenter la classe Platform dans source/platform/. 

Le jeu est écrit presque entièrement en C++, avec une petite quantité de C, un dialecte personnalisé de LISP, ainsi qu'un tout petit peu d'assembleur ARM.

[Contenu↑](#contents)

## Synthèse

Le projet devrait être assez facile à compiler, mais si vous avez des difficultés, essayez d'utiliser le conteneur docker ci-joint avant de m'envoyer un mail avec des questions sur le système d'exploitation.
```
docker pull ubuntu:18.04
sudo docker build -t blind_jump_build .
sudo docker run -it blind_jump_build
make
```

NOTE : vous pouvez également obtenir un environnement de construction conteneurisé à partir du lien ci-dessous, bien que vous deviez vous rappeler d'exécuter `git pull` lorsque vous entrez dans le conteneur, puisque j'ai construit le conteneur avec une version gelée du référentiel. Si cela n'est pas pratique pour vous, n'hésitez pas à construire le conteneur vous-même en suivant les étapes ci-dessus.

https://hub.docker.com/r/evanbowman/blind_jump_build

[Contenu↑](#contents)

## Contribuer

La meilleure façon d'aider ce projet est de télécharger le jeu et d'y jouer. Toutes les suggestions d'amélioration sont les bienvenues.

### Musique

L'ajout de nouvelles musiques à la version GBA du jeu étant fastidieux/compliqué, il faut bien comprendre la méthodologie de formatage et de compilation des fichiers audio. Le matériel de la Gameboy Advance exige plus ou moins que les données audio numériques soient formatées sous forme de fichiers wav 8 bits signés. Pour ajouter de nouveaux fichiers sonores au projet, les données devront être converties en PCM 8 bits signé sans en-tête, à 16 kHz. Ensuite, les données devront être converties en un fichier ASM, et compilées dans le projet. Evan Bowman a créé un outil pour cela, disponible ici : https://github.com/evanbowman/raw_to_asm. L'outil raw_to_asm doit être lancé depuis la racine du projet BlindJump. Le fichier musical/son doit être exporté au format brut requis, avec une extension .raw, et placé dans le répertoire sounds/. Ensuite, vous devez exécuter raw_to_asm, avec le nom du fichier son, moins l'extension .raw (`raw_to_asm music`, où music.raw existe dans sounds). Le programme écrira les fichiers d'en-tête et source contenant une représentation en assembleur des données musicales dans le répertoire source/data/. Vous devrez ajouter le nouveau fichier source à CMakeLists.txt, et ajouter de nouvelles déclarations pour les données sonores ou musicales à gba_platform.cpp. Rappelez-vous également que les cartouches GBA ne doivent pas dépasser 32 Mo.

[Contenu↑](#contents)

## Traduction

Le moteur de texte de BlindJump supporte tous types de caractères via l'encodage utf-8 ; cependant, le jeu n'inclut pas tous les codes utf-8 possibles dans sa carte de caractères (voir l'image charset dans images/). BlindJump supporte actuellement les glyphes alpha-numériques pour l'anglais, quelques centaines de glyphes pour les mots chinois, quelques glyphes étendus pour l'italien et le français, les caractères cyrilliques pour le russe, et un jeu de caractères Katakana japonais minimal (y compris les marques d'accent). Pour ajouter une nouvelle langue au jeu, vous devez effectuer quelques modifications :
1) Créez un nouveau fichier, <nom_de_la_langue>.txt, dans le répertoire strings/. Par souci de simplicité, vous pouvez vous contenter de faire une copie de l'un des fichiers existants dans le dossier strings/.
2) Ecrivez votre traduction, en traduisant chaque ligne du fichier nouvellement créé dans la langue souhaitée. Veillez à tester la traduction en jouant au jeu, afin de vérifier que les lignes de texte ne sont pas trop longues, ce qui pourrait provoquer des problèmes graphiques. Rappelez-vous que la Gameboy Advance peut afficher 30 caractères horizontalement : pour toute chaîne de caractères que l'interface utilisateur du jeu ne fait pas passer à la ligne suivante, votre texte traduit peut être tronqué, ou dans certains cas, le jeu écrira une erreur dans les logs indiquant que le programme n'a pas pu afficher le texte excessivement long, et arrêtera l'exécution (freeze).
3) Ensuite, trouvez la définition de la liste de variables `languages` dans scripts/init.lisp, et ajoutez un symbole correspondant au nom du fichier de langue nouvellement ajouté (sans l'extension .txt, voir init.lisp pour des exemples).
4) Enfin, vous devrez lier explicitement votre fichier de chaînes au système de construction dans CMakeLists.txt. Etant donné que la version Gameboy Advance du jeu ne fonctionne pas dans des environnements hébergés, il n'y a pas de système de fichiers, donc le système de construction doit connaître votre fichier, afin de copier son contenu dans la ROM.

Blind Jump est aujourd’hui jouable dans les langues suivantes :
* Anglais
* Chinois
* Russe
* Italien
* Français

[Contenu↑](#contents)

## Sécurité

Je soumets périodiquement tout le code source de ce projet à l'analyseur statique Coverity, qui vérifie les dépassements de tampon, la corruption de mémoire, etc. Actuellement, BlindJump est 100% sans défaut. Continuons dans cette voie.

[Contenu↑](#contents)

## Téléchargements

Voir la [section releases](https://github.com/evanbowman/blind-jump-portable/releases) de ce repo github, où vous pouvez télécharger la ROM Gameboy Advance directement (fichiers avec une extension .gba), et lancer le jeu à l'aide d'une flashcart (par exemple Everdrive). Vous pouvez également utiliser un émulateur, bien que je préfère personnellement jouer aux jeux Gameboy Advance sur la console elle-même. Vous trouverez des ROMs jointes à chaque version, remontant jusqu'en 2019, donc vous pourriez aussi télécharger des ROMs antérieures pour voir comment le projet a évolué au fur et à mesure des ajouts. J'ai utilisé les versions Linux/Windows/Mac du jeu pendant le développement, mais les versions de bureau sont tombées un peu en désuétude et peuvent ne plus fonctionner. Je n'ai pas fait beaucoup d'efforts pour maintenir les cibles de construction pour PC de bureau, car il existe des émulateurs GBA pour de nombreuses plates-formes différentes, ce qui rend la rom GBA très portable en soi.

[Contenu↑](#contents)

## Commentaires

Vous avez une opinion sur le jeu, ou des suggestions d'amélioration ? Je vous encourage à créer un problème, ou à laisser un commentaire [ici](https://evanbowman.itch.io/blindjump).

[Contenu↑](#contents)

## Licence

La plupart du code de ce projet est disponible sous la licence MIT. Les versions Gameboy Advance et PlayStation Portable du projet font référence au code de external/libgba et glib2d (respectivement), qui sont des projets GPL. Par conséquent, les builds Gameboy Advance et PSP doivent être distribués sous les termes de la licence GPL. Le jeu est déjà open source, donc ça ne fait pas beaucoup de différence mais vaut la peine d’être mentionné pour ceux qui veulent s’amuser avec le code. Comme les versions de bureau du projet ne font pas référence au code GPL de quelque manière que ce soit, elles ne sont pas sous licence GPL.

Toutes les images et tous les dessins de personnages appartiennent à Evan Bowman, et ne doivent pas être utilisés à des fins commerciales sans autorisation. La musique appartient à divers compositeurs. La plupart des pistes musicales sont sous licence Creative Commons et nécessitent simplement une attribution (voir le générique de fin du jeu), mais quelques pistes musicales peuvent ne pas permettre une distribution dans un contexte commercial. Ces pistes devront être remplacées si des copies du jeu sont vendues.

## LISP

BlindJump uses a custom LISP dialect for lightweight scripting. In the settings menu, the game allows you to launch a repl while the game is running, to manipulate game state (for gameboy builds, the game will render an onscreen keyboard and console, for desktop builds, the game will pause, and read from the command line). See below for a brief overview of the language. If you've used Scheme, many things will feel familiar, although you'll also find some notable differences.

* LISP-1
* Supports Tail Call Optimization (for compiled bytecode)
* Dynamically scoped
* Includes a bytecode compiler, with a small set of optimization passes

### Language

#### S-Expressions
Like All LISP languages, our interpreter represents code using a notation called S-Expressions. Each S-Expression consists of a list of space-separated tokens, enclosed with parentheses. For example:
```LISP
(dog cat bird) ;; This is an S-Expression

(1 2 cat 5 pigeon) ;; This is also an S-Expression

() ;; So is this.
```
When you enter an S-Expression, the first element in the list will be interpreted as a function, and the remaining elements will be supplied to the function as arguments.
```LISP
(+ 1 2 3) ;; Result: 6
```
In some scenarios, you do not want an S-Expression to be evaluated as a function. For example, you may simply want a list of data. To prevent evaluation of an S-Expression, prefix the expression with a `'` character.
```LISP
;; NOTE: without the quote character, the interpreter would have tried to call a function called dog,
;; with the arguments cat and 3.
'(dog cat 3) ;; Result: '(dog cat 3).
```

#### Variables

To declare a variable, use the `set` function, along with a variable name symbol, followed by a value:
```LISP
(set 'i 5)
```
Now, variable `i` stores the value `5`, so `5` will be substituted wherever you type the variable `i`:

```LISP
(+ i i) ; -> 10
```


#### Data types

##### Nil
The simplest data type in our language. Represents nothing. Represented by the `nil` keyword, or by the empty list `'()`.

##### Integer
A 32 bit signed integer. For example; `0`, `1`, or `2`. Due to the way that our interpreter tokenizes variable names, there is currently no way to specify negative numbers, although you can work around the issue by subtracting from zero, e.g.: `(- 0 5)` for -5.

##### Cons Pair
Arguably the most important data type in our language. Implements a pair of two values. To create a pair, invoke the `cons` function, with two arguments:
```LISP
(cons 1 2) ;; -> '(1 . 2)
```
LISP interpreters represent lists as chains of cons pairs, terminated by nil. For example:
```LISP
(cons 1 (cons 2 (cons 3 nil))) ;; -> '(1 2 3)
```
Or more simply, you could just use the `list` function:
```LISP
(list 1 2 3) ;; -> '(1 2 3)
```

##### Functions (lambdas)
A function, or lambda, contains re-usable code, to be executed later. To declare a lambda, use the `lambda` keyword:
```LISP
(lambda
  (cons 1 2))
```
You may want to store the lambda in a variable, so that you can use it later:
```LISP
(set 'my-function 
  (lambda
    (cons 1 2)))
```
Now, when you invoke my-function, the code you wrote within the lambda expression will be evaluated:
```LISP
(my-function) ;; -> '(1 . 2)
```
But for a function to be really useful, you'll want to be able to pass parameters to it. Within a function, you may refer to function arguments with the $ character, followed by the argument number, starting from zero. For example:
```LISP
(set 'temp
  (lambda
    (cons $0 $1)))
    
(temp 5 6) ;; -> '(5 . 6)
```
You may also refer to an argument with the `arg` function, e.g. `(arg 0)` or `(arg 5)`. To ask the interpreter how many arguments the user passed to the current function, you may use the `argc` function.

To refer to the current function, invoke the `this` function.
```LISP
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
```LISP
(set 'a 10) ;; a now holds 10.
```
You may be wondering why `set` accepts a symbol as an argument. I did not implement `set` as a macro or a special form, but instead, as a regular function. The whole dialect only has three special forms, `lambda`, `if`, and `let`. I implemented everything else as a builtin function, so unlike other lisp dialects, you need to specify a quoted symbol when assigning a variable.

#### cons
`(cons <first> <second>)`
Construct a pair, from parameters `<first>` and `<second>`.
```LISP
(cons 1 'fish) ;; -> '(1 . fish)
```
#### car
`(car <pair>)`
Retrieve the first element in a cons pair.
```LISP
(car '(1 . fish)) ;; -> 1
```
#### cdr
`(cdr <pair>)`
Retrieve the second element in a cons pair.
```LISP
(cdr '(1 . fish)) ;; -> 'fish
```

#### list
`(list ...)`
Construct a list containing all parameters passed to the list function.
```LISP
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
```LISP
(apply + '(1 2 3 4 5)) ;; -> 15
(apply cons '(1 2)) ;; '(1 . 2)
```

#### fill
`(fill <integer> <value>)`
Create a list with `<integer>` repetitions of `<value>`.
```LISP
(fill 6 5) ;; -> '(5 5 5 5 5 5)
(fill 4 'cat) ;; -> '(cat cat cat cat)
```

#### gen
`(gen <integer> <function>)`
Generate a list of `<integer>` numbers, by invoking `<function>` on each number from `0` to `<integer>`.
```LISP
(gen 5 (lambda (* 2 $0)) ;; -> '(0 2 4 6 8)
```

####  length
`(length <list>)`
Returns the length of a list.
```LISP
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
```LISP
(filter (lambda (< $0 5)) '(1 2 3 4 5 6 7 8)) ;; '(1 2 3 4)
```

#### map
`(map <function> <list>)`
Return a list representing the result of calling `<function>` for each element of `<list>`.
```LISP
(map (lambda (+ $0 3)) (range 0 10)) ;; '(3 4 5 6 7 8 9 10 11 12)
```
NOTE: You may pass multiple lists to map, and multiple elements, one from each list, will be passed to `<function>`
```LISP
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
```LISP
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
```LISP
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
```LISP
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

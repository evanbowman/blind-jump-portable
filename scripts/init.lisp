;;;
;;; Blind Jump init script, invoked during startup.
;;;
;;; For scripting, the game uses a minimal lisp dialect. All variables are
;;; global, and there is no real syntax for custom (user defined) lambdas
;;; yet. The platform provides a rich set of builtin functions, which should be
;;; more than sufficient for lightweight scripting. I do not intend for much of
;;; the game logic to actually be written in lisp, I just wanted a more flexible
;;; language for startup configuration, and I wanted to support an in-game lisp
;;; repl for debugging/testing purposes. Our interpreter, which needs to run
;;; with very limited memory on embedded systems, is designed for low memory
;;; usage, rather than speed. While the language does support syntax for loops,
;;; you should stick to the builtin functions, which execute faster.
;;;
;;; We do not fully support quoted expressions yet. For now, use the # character
;;; to refer to a symbol.
;;;
;;; Note: This lisp dialect supports almost no special forms--almost everything
;;; is a first class function. For example, 'set' is not implemented as a
;;; keyword, but as a builtin function, which is why set expressions require a
;;; symbol object rather than a language-level identifier.
;;;
;;; The interpreter supports a builtin syntax for functional currying. In
;;; addition to functions, you can invoke lists--if the car of the list is a
;;; function, the cdr of the list will be passed to that function as
;;; arguments. For example:
;;;
;;; >> (set #temp (list list 1 2 3))
;;; nil
;;; >> temp
;;; (<lambda> 1 2 3)
;;; >> (temp 4 5)
;;; (1 2 3 4 5)
;;;
;;; Because we do not support lambda expressions, curried functions should come
;;; in handy, although technically, one could often achieve the same result with
;;; a combination of the map and fill builtins. e.g.:
;;;
;;; >> (set #curried-cons (list cons 1))
;;; nil
;;; >> (map curried-cons (list 1 2 3))
;;; ((1 . 1) (1 . 2) (1 . 3))
;;; >> (map cons (fill 3 1) (list 1 2 3))
;;; ((1 . 1) (1 . 2) (1 . 3))
;;;
;;; So, technically, currying is unnecessary if you're using the variadic
;;; version of the map builtin, but you can save a lot of extra memory in some
;;; cases by using curried functions, rather than generating big lists full of
;;; repetative data.
;;;
;;;
;;; Some gotchas, and other special tips:
;;;
;;; 1) Be careful when declaring symbols in lists passed to the eval
;;; function. For example, this code:
;;;
;;; (eval (list #set #a 8)) ;; i.e.: (set a 8)
;;;
;;; Will not work. Do you see why? The set function accepts a symbol, not an
;;; identifier, as an argument. This code will try to load whatever is currently
;;; bound to the variable a, and pass the value to set, which expects a
;;; symbol. So, instead, we'll want to explicitly include the symbol modifier
;;; character as a symbol within the evaluation list:
;;;
;;; (eval (list #set ## #a 8)) ;; i.e.: (set # a 8), aka: (set #a 8)
;;;


;; For our onscreen keyboard, which does not include +, *, or /
(set #add +)
(set #mul *)
(set #div /)


(set #languages
     (list #null
           #english ;; The engine uses index 1 of the list as the default
                    ;; language, when a save file does not yet exist. To change
                    ;; the default, move another language above this line.
           #spanish))


(if (equal (platform) #Desktop)
    (progn

      ;; Hint:
      ;; (register-controller <vendor-id>
      ;;                      <product-id>
      ;;                      <action-1>
      ;;                      <action-2>
      ;;                      <start>
      ;;                      <l-trigger>
      ;;                      <r-trigger>)

      (register-controller 1356 616 1 0 9 4 5) ;; Sony PS3 Controller
      (register-controller 1356 1476 2 1 9 4 5)   ;; Sony PS4 Controller
      (set #network-port 50001)))


;; Parameters to control the cellular automata for level generation
(set #cell-thresh (cons 4 5))
(set #cell-iters 2)


;;
;; Below: some configurable settings for testing the game. Use the pre-defined
;; modes to jump to a specific level, or hard-code your own parameters for
;; level, health, items, etc.
;;
;; We only set debug-mode if unbound in the environment, in case someone passed
;; an eval string to the application through the commandline in hosted
;; environments, to define their own version of the variable:
;; ./BlindJump -e "(set #debug-mode 4)"
;;
(if (not (bound #debug-mode))
    ;; 0: normal mode
    ;; 1: debug boss 0
    ;; 2: debug boss 1
    ;; 3: debug zone 2
    ;; 4: debug zone 3
    ;; 5: debug boss 2
    ;; 6: debug zone 4
    ;; 7: boss rush mode
    ;; 8: debug boss 3
    (set #debug-mode 0))


(if (not (equal debug-mode 0))
    (progn
      (log-severity 0)

      ;; let's give ourselves lots of health and powerups
      (set #temp
           (list
            item-accelerator
            item-accelerator
            item-accelerator
            item-accelerator
            item-explosive_rounds_2
            item-explosive_rounds_2
            item-explosive_rounds_2
            item-explosive_rounds_2)

      (set-hp player 20)

      (if (equal debug-mode 7) (level boss-0-level))

      (if (equal debug-mode 1)
          (progn
            (level boss-0-level)
            (apply add-items temp)))

      (if (equal debug-mode 2)
          (progn
            (level boss-1-level)
            (apply add-items (cons item-signal_jammer temp))))

      (if (equal debug-mode 3)
          (progn
            (level (+ 1 boss-0-level))
            ;; These params should be sort of reasonable for someone who just
            ;; beat the first boss...
            (set-hp player 5)
            (add-items item-accelerator
                       item-explosive_rounds_2)))

      (if (equal debug-mode 4)
          (progn
            (level (+ 1 boss-1-level))
            (set-hp player 8)
            (add-items item-accelerator
                       item-explosive_rounds_2)))

      (if (equal debug-mode 5)
          (progn
            (level boss-2-level)
            (set-hp player 7)
            (apply add-items temp)))

      (if (equal debug-mode 6)
          (progn
            (level (+ 1 boss-2-level))
            (set-hp player 4)
            (add-items item-accelerator
                       item-explosive_rounds_2)))

      (unbind #temp)))


;;;
;;; Some useful little snippets:
;;;
;;; Unlock all items:
;;; (apply add-items (range 0 50))
;;;
;;; Clear enemies from level:
;;; (apply kill (enemies))
;;;

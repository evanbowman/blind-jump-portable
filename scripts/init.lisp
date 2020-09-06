;;;
;;; Blind Jump init script, invoked during startup.
;;;
;;; For scripting, the game uses a minimal lisp dialect. All variables are
;;; global, and there is no syntax for custom (user defined) lambdas yet. The
;;; platform provides a rich set of builtin functions, which should be more than
;;; sufficient for lightweight scripting. I do not intend for much of the game
;;; logic to actually be written in lisp, I just wanted a more flexible language
;;; for startup configuration, and I wanted to support an in-game lisp repl for
;;; debugging/testing purposes. Our interpreter, which needs to run with very
;;; limited memory on embedded systems, is designed for low memory usage, rather
;;; than speed. While the language does support syntax for loops, you should
;;; stick to the builtin functions, which execute faster.
;;;


;; For our onscreen keyboard, which does not include + or /
(set 'add +)
(set 'div /)


(set 'default-lang 'english)

(if (equal (platform) 'Desktop)
    (progn
      (register-controller 1356 616 11 14 16 4 5) ;; Sony PS3 Controller
      (register-controller 1356 1476 2 1 9 4 5)   ;; Sony PS4 Controller
      (set 'network-port 50001)))


;; Parameters to control the cellular automata for level generation
(set 'cell-thresh (cons 4 5))
(set 'cell-iters 2)


;;
;; Below: some configurable settings for testing the game. Use the pre-defined
;; modes to jump to a specific level, or hard-code your own parameters for
;; level, health, items, etc.
;;
(if (not (bound 'debug-mode))
    ;; 0: normal mode
    ;; 1: debug boss 0
    ;; 2: debug boss 1
    ;; 3: debug zone 2
    ;; 4: debug zone 3
    ;; 5: debug boss 2
    (set 'debug-mode 0))


(if (not (equal debug-mode 0))
    (progn
      ;; let's give ourselves lots of health and powerups
      (set 'debug-items-lat (list 5 5 5 5 9 9 9 9))
      (set-hp 0 20)

      (if (equal debug-mode 1)
          (progn
            (level 10)
            (apply add-items debug-items-lat)))

      (if (equal debug-mode 2)
          (progn
            (level 21)
            (apply add-items (cons 12 debug-items-lat))))

      (if (equal debug-mode 3)
          (progn
            (level 11)
            ;; These params should be sort of reasonable for someone who just
            ;; beat the first boss...
            (set-hp 0 7)
            (add-items 5 9)))

      (if (equal debug-mode 4)
          (progn
            (level 22)
            (set-hp 0 8)
            (add-items 5 9)))

      (if (equal debug-mode 5)
          (progn
            (level 27)
            (set-hp 0 7)
            ;; (apply add-items debug-items-lat)
            (add-items 5 9)))

      (unbind 'debug-items-lat)))

(unbind 'debug-mode)


;;;
;;; Some useful little snippets:
;;;
;;; Unlock all items:
;;; (apply add-items (range 0 50))
;;;
;;; Clear enemies from level:
;;; (apply kill (enemies))
;;;

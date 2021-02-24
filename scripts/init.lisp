
;; For our onscreen keyboard, which does not include +, *, or /
(set 'add +)
(set 'mul *)
(set 'div /)


(set 'languages
     '(null
       english ;; The engine uses index 1 of the list as the default language,
               ;; when a save file does not yet exist. To change the default,
               ;; move another language above this line.
       spanish))


(if (equal (platform) 'Desktop)
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
      (set 'network-port 50001)))


;; Parameters to control the cellular automata for level generation
(set 'cell-thresh (cons 4 5))
(set 'cell-iters 2)



(set 'pre-levelgen-hooks nil)
(set 'post-levelgen-hooks nil)
(set 'waypoint-clear-hooks nil)
(set 'boss-defeated-hooks nil)


(set 'add-hook
     (compile (lambda
                ;; (add-hook hooks-sym hook)
                (set (arg 1) (cons (arg 0) (eval (arg 1)))))))

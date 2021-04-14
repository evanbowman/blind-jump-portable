
;; For our onscreen keyboard, which does not include +, *, or /
(set 'add +)
(set 'mul *)
(set 'div /)


(set 'languages
     '(null
       (english 1) ;; Language name, required font size (1:normal or 2:double)
       (chinese 2)
       ))


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


(add-hook
 'waypoint-clear-hooks
 (compile
  (lambda
    (if (all-true (not (peer-conn)) (bound 'swarm))
        (progn
          (alert 137)
          (set 'n (if (equal swarm 0) 10 5))

          (set 'temp
               (lambda (map
                        +
                        (fill n (- ((arg 0) (get-pos gate)) 15))
                        (gen n (lambda (cr-choice 15))))))

          (map make-enemy (fill n swarm) (temp car) (temp cdr))

          (unbind 'swarm))))))

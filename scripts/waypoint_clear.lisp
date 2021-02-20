;;;
;;; This hook will be evaluated when the player clears a waypoint.
;;;


(if (all-true (not (peer-conn)) (bound 'swarm))
    (progn
      (alert 136)
      (set 'n (if (equal swarm 0) 10 5))

      ;; Give the enemies a bit of a random offset when spawning
      (set 'temp
           (cons
            (map + (fill n (- (car (get-pos gate)) 15)) (gen n (lambda (cr-choice 15))))
            (map + (fill n (- (cdr (get-pos gate)) 15)) (gen n (lambda (cr-choice 15))))))

      (map make-enemy (fill n swarm) (car temp) (cdr temp))

      (unbind 'swarm)))


(map (lambda ((arg 0))) waypoint-clear-hooks)

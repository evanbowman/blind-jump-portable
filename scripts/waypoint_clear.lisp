;;;
;;; This hook will be evaluated when the player clears a waypoint.
;;;


(if (bound #swarm)
    (progn
      (alert 136)
      (set #n (if (equal swarm 0) 10 5))

      ;; Give the enemies a bit of a random offset when spawning
      (set #temp
           (cons
            (map + (fill n (- (car (get-pos 2)) 15)) (gen n (list cr-choice 30)))
            (map + (fill n (- (cdr (get-pos 2)) 15)) (gen n (list cr-choice 30)))))

      (map make-enemy (fill n swarm) (car temp) (cdr temp))

      (unbind #swarm)))

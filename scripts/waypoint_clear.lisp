;;;
;;; This hook will be evaluated when the player clears a waypoint.
;;;


(if (bound #swarm)
    (progn
      (alert 136)
      (set #n (if (equal swarm 0) 10 5))
      (map make-enemy
           (fill n swarm)
           (fill n (car (get-pos 2)))
           (fill n (cdr (get-pos 2)))) ;; NOTE: The transporter will always have
                                       ;; entity id 2.
      (unbind #swarm)))

;;;
;;; This hook will be evaluated when the game finishes generating a new level.
;;;


;; Sometime, spawn a swarm of enemies after clearing a waypoint. The
;; waypoint_clear hook will look for the swarm variable in the environment.
(if (not (peer-conn)) ;; For now, due to the way entity ids are synchronized (or
                      ;; rather, the way that they aren't), we cannot spawn
                      ;; enemies with ids that are guaranteed to match across
                      ;; consoles, except during level generation.
    (if (equal (cr-choice 4) 0)
        (if (all-true
             (> (level) 1)
             (not (equal (level) boss-0-level))
             (not (equal (level) boss-1-level))
             (not (equal (level) boss-2-level))
             (not (equal (level) boss-3-level)))
            (set #swarm (if (all-true (> (level) boss-0-level)
                                      (cr-choice 2))
                            6
                            0)))))

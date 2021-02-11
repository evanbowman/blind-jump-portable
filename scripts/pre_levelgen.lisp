;;;
;;; This hook will be evaluated when the game commences level generation.
;;;


;; Indices in the tilemap which the game will treat as walls
(set 'wall-tiles-list (list 0 3 8 9 10 11 12 13))

;; The game makes a distinction between which tiles are part of the center of
;; the map, and which tiles are edges.
(set 'edge-tiles-list (list 1 4 14 15 16 17))


(if (< (level) (+ boss-0-level 1))
    (set 'wall-tiles-list (cons 18 (cons 19 wall-tiles-list))))


;; Boss rush hack
(if (equal debug-mode 7)
    (progn

      (add-items item-accelerator
                 item-explosive_rounds_2)

      (if (< (level) boss-0-level)
          (level boss-0-level)
          (if (< (level) boss-1-level)
              (level boss-1-level)
               (if (< (level) boss-2-level)
                   (level boss-2-level)
                   (if (< (level) boss-3-level)
                       (level boss-3-level)))))))


(map (lambda ((arg 0))) pre-levelgen-hooks)

;;;
;;; This hook will be evaluated when the game commences level generation.
;;;


;; Indices in the tilemap which the game will treat as walls
(set #wall-tiles-list (list 0 3 8 9 10 11 12 13))

;; The game makes a distinction between which tiles are part of the center of
;; the map, and which tiles are edges.
(set #edge-tiles-list (list 1 4 14 15 16 17))

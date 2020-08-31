
(set default-lang 'english)

(if (equal (platform) 'Desktop)
    (progn
      (register-controller 1356 616 11 14 16 4 5)
      (register-controller 1356 1476 2 1 9 4 5))
  nil)


(set cell-thresh (cons 4 5))
(set cell-iters 2)

(set sound-dir '../sounds/)
(set image-dir '../images/)
(set network-port 50001)

(define map (lambda (fn lst) (if (null? lst) (quote ()) (cons (fn (car lst)) (map fn (cdr lst))))))
(map (lambda (x) (* x x)) (list 1 2 3))

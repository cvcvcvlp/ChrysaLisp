(include 'sys/class.inc)

;;;;;;;;;;;;;;;;;;
; mem static class
;;;;;;;;;;;;;;;;;;

(def-class 'sys_mem)
(dec-method 'statics 'sys/mem/statics)
(dec-method 'init 'sys/mem/init)
(dec-method 'deinit 'sys/mem/deinit)
(dec-method 'alloc 'sys/mem/alloc 'static '(r0) '(r0 r1))
(dec-method 'calloc 'sys/mem/calloc 'static '(r0) '(r0 r1))
(dec-method 'free 'sys/mem/free 'static '(r0))
(dec-method 'clear 'sys/mem/clear 'static '(r0 r1) '(r0))
(dec-method 'fill 'sys/mem/fill 'static '(r0 r1 r2) '(r0))
(dec-method 'copy 'sys/mem/copy 'static '(r0 r1 r2) '(r0 r1))
(dec-method 'realloc 'sys/mem/realloc 'static '(r0 r1 r2) '(r0 r1))
(dec-method 'recalloc 'sys/mem/recalloc 'static '(r0 r1 r2) '(r0 r1))
(dec-method 'collect 'sys/mem/collect)
(dec-method 'used 'sys/mem/used 'static nil '(r0))

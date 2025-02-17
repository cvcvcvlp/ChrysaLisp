;;;;;;;;;;;;;
; Emit Buffer
;;;;;;;;;;;;;

(defcvar '*emit-buffer* (list) '*distance* (list) '*pc* nil '*pass* nil
	'*out-buffer* (cat "") '*old-out-buffer* (cat "") '*out-stream* nil)

(defmacro-bind emit (&rest _)
	`(push *emit-buffer* ~_))

(defcfun-bind emit-reset ()
	(push (clear *distance* *emit-buffer*) progn))

(defcfun-bind emit-passes ()
	(defq b nil)
	(setq *pass* 0 *pc* 0 *out-stream* (string-stream *out-buffer*))
	(eval (bind-fun *emit-buffer*) *compile-env*)
	(setq *out-buffer* (str *out-stream*) *pass* (inc *pass*))
	(until (eql *out-buffer* *old-out-buffer*)
		(setq b *old-out-buffer* *old-out-buffer* *out-buffer* *out-stream* (string-stream b) *pc* 0)
		(eval *emit-buffer* *compile-env*)
		(setq *out-buffer* (str *out-stream*) *pass* (inc *pass*))))

(defcfun print-emit (_)
	(and (lst? _) (not (eql (elem 0 _) 'lambda)) (print _)))

(defcfun-bind emit-label (_)
	(set (env) _ *pc*))

(defcfun-bind emit-byte (&rest _)
	(write-char *out-stream* _ 1)
	(setq *pc* (+ *pc* (length _))))

(defcfun-bind emit-short (&rest _)
	(write-char *out-stream* _ 2)
	(setq *pc* (+ *pc* (* (length _) 2))))

(defcfun-bind emit-int (&rest _)
	(write-char *out-stream* _ 4)
	(setq *pc* (+ *pc* (* (length _) 4))))

(defcfun-bind emit-long (&rest _)
	(write-char *out-stream* _ 8)
	(setq *pc* (+ *pc* (* (length _) 8))))

(defcfun-bind emit-string (_)
	(write *out-stream* _)
	(setq *pc* (+ *pc* (length _))))

(defcfun-bind emit-align (a &optional b)
	(when (/= *pass* 0)
		(defq p *pc* b (opt b 0) s *out-stream*)
		(setq *pc* (align p a))
		(while (<= (setq p (inc p)) *pc*)
			(write-char s b))))

;;;;;;;;;;;;;;;;;;;;
; VP level optimizer
;;;;;;;;;;;;;;;;;;;;

(defmacro opt-emit-read-ops ()
	''(emit-cpy-ir-ui emit-cpy-ir-us emit-cpy-ir-ub emit-cpy-ir-i emit-cpy-ir-s emit-cpy-ir-b emit-cpy-ir))

(defmacro opt-emit-write-ops ()
	''(emit-cpy-ri-i emit-cpy-ri-s emit-cpy-ri-b emit-cpy-ri-i emit-cpy-ri-s emit-cpy-ri-b emit-cpy-ri))

(defmacro opt-emit-write-ops1 ()
	''(emit-cpy-ri-i emit-cpy-ri-s emit-cpy-ri-b emit-cpy-ri))

(defmacro opt-emit-read-ops1 ()
	''(emit-cpy-ir-i emit-cpy-ir-s emit-cpy-ir-b emit-cpy-ir))

(defmacro opt-emit-read-ops2 ()
	''(emit-cpy-ir-ui emit-cpy-ir-us emit-cpy-ir-ub emit-cpy-ir))

(defmacro opt-emit-call-ops ()
	''(emit-label emit-call emit-call-abi emit-call-i emit-call-r emit-call-p))

(defmacro opt-emit-kills-ops ()
	''(emit-beq emit-bne emit-blt emit-ble emit-bgt emit-bge emit-jmp emit-jmp-i emit-jmp-r emit-jmp-p
		emit-alloc emit-free))

(defmacro opt-emit-two-out-ops ()
	''(emit-land-rr emit-lnot-rr emit-swp-rr emit-div-rrr emit-div-rrr-u))

(defmacro opt-emit-multi-out-ops ()
	''(emit-pop))

(defmacro opt-trashes? (r e)
	`(cond
		((eql ,r (elem -2 ,e)) t)
		((find (elem 0 ,e) (opt-emit-two-out-ops)) (eql ,r (elem -3 ,e)))
		((find (elem 0 ,e) (opt-emit-multi-out-ops)) (find ,r ,e))))

(defcfun-bind opt-emit-trashes-without-calls? (r e)
	(opt-trashes? r e))

(defcfun-bind opt-emit-trashes-with-calls? (r e)
	(cond
		((find (elem 0 e) (opt-emit-call-ops)) t)
		(t (opt-trashes? r e))))

(defcfun-bind opt-emit-trashes-with-kills? (r e)
	(cond
		((find (elem 0 e) (opt-emit-kills-ops)) t)
		((find (elem 0 e) (opt-emit-call-ops)) t)
		(t (opt-trashes? r e))))

(defcfun-bind opt-emit-find-rw (k _ r w b i)
	(defq _ (some! _ 0 t (lambda (e)
		(cond
			((and (eql w (elem 0 e))
				(eql b (elem 2 e))
				(= i (eval (elem 3 e))))
				(setq c 1) _)
			((and (eql r (elem 0 e))
				(eql b (elem 1 e))
				(not (eql b (elem 3 e)))
				(= i (eval (elem 2 e))))
				(setq c 3) _)
			((k b e) -1))) (list *emit-buffer*)))
	(and _ (/= _ -1) _))

(defcfun-bind opt-emit-find-rw1 (k _ ra rb w b i)
	(defq _ (some! _ 0 t (lambda (e)
		(cond
			((and (eql w (elem 0 e))
				(eql b (elem 2 e))
				(= i (eval (elem 3 e))))
				(setq c 1) _)
			((and (or (eql ra (elem 0 e)) (eql rb (elem 0 e)))
				(eql b (elem 1 e))
				(not (eql b (elem 3 e)))
				(= i (eval (elem 2 e))))
				(setq c 3) _)
			((k b e) -1))) (list *emit-buffer*)))
	(and _ (/= _ -1) _))

(defcfun-bind opt-emit-find-rr (k _ rs rd)
	(defq _ (some! _ 0 t (lambda (e)
		(cond
			((match? e `(emit-cpy-rr ,rs ,rd)) _)
			((opt-trashes? rs e) -1)
			((k rd e) -1))) (list *emit-buffer*)))
	(and _ (/= _ -1) _))

(defcfun-bind opt-emit-buffer ()
	(each! 2 -1 (lambda (e)
		(cond
			;read after read/write
			((defq c (find (defq o (elem 0 e)) (opt-emit-read-ops)))
				(when (defq w (opt-emit-find-rw opt-emit-trashes-with-calls? _
								(elem c (opt-emit-read-ops))
								(elem c (opt-emit-write-ops))
								(elem 1 e) (eval (elem 2 e))))
					(defq r (elem c (elem w *emit-buffer*)))
					(when (not (some! (inc w) _ t (lambda (_)
							(opt-emit-trashes-without-calls? r _)) (list *emit-buffer*)))
						(elem-set _ *emit-buffer*
							(if (eql r (elem 3 e))
								(lambda)
								`(emit-cpy-rr ,r ,(elem 3 e)))))))
			;write after write
			((defq c (find o (opt-emit-write-ops1)))
				(when (defq w (opt-emit-find-rw1 opt-emit-trashes-with-kills? _
								(elem c (opt-emit-read-ops1))
								(elem c (opt-emit-read-ops2))
								(elem c (opt-emit-write-ops1))
								(elem 2 e) (eval (elem 3 e))))
					(when (= c 1)
						(elem-set w *emit-buffer* (lambda)))))
			;redundant copies
			((eql o 'emit-cpy-rr)
				(when (opt-emit-find-rr opt-emit-trashes-with-calls? _ (elem 2 e) (elem 1 e))
					(elem-set _ *emit-buffer* (lambda))))
			))
	(list *emit-buffer*)))

;;;;;;;;;;;;;;;;;
; VP Instructions
;;;;;;;;;;;;;;;;;

(each (lambda (_)
	(defcvar _ _)) '(r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 rsp))

(defcmacro vp-def (_ &optional l)
	(defq v '(r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14))
	(setq l (apply cat (map (lambda (x y)
		(list (list 'quote x) y)) _ (if l (merge (cat (eval l)) v) v))))
	`(progn (defcvar ~l) (apply push (cat (list *func-syms*) ',_))))

(defcfun-bind vp-type (o)
	(defq i (type-of o))
	(cond
		((= i lisp_type_sym)
			(cond
				((reg o) 'r)
				(t 'c)))
		((= i lisp_type_int) 'c)
		((= i lisp_type_lst)
			(setq i (elem 0 o))
			(cond
				((reg i)
					(if (reg (elem 1 o)) 'd
						(if (= 2 (length o)) 'i
							(sym (cat "i" (elem 2 o))))))
				((eql i '@) i)
				((eql i '$) i)
				((eql i '&)
					(if (reg (elem 1 o))
						(if (reg (elem 2 o)) '&d '&i) nil))
				(t 'c)))
		((= i lisp_type_str) 's)
		(t nil)))

(defcfun-bind label-sym (s) (sym (cat "_l_" s)))
(defcfun-bind vp-label (l) (setq l (label-sym l))
	(emit (list 'emit-label (list 'quote l))) (push *func-syms* l) (defcvar l 0))
(defcfun-bind vp-align (a &optional b) (emit `(emit-align ,a ,b)))
(defcfun-bind vp-string (s) (emit `(emit-string ,s)))
(defcfun-bind vp-byte (&rest b) (emit `(emit-byte ~b)))
(defcfun-bind vp-short (&rest b) (emit `(emit-short ~b)))
(defcfun-bind vp-int (&rest b) (emit `(emit-int ~b)))
(defcfun-bind vp-long (&rest b) (emit `(emit-long ~b)))

(defcfun-bind vp-beq-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-beq ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bne-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-bne ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-blt-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-blt ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-ble-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-ble ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bgt-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-bgt ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bge-cr (c d l)
	(emit `(emit-cmp-cr ,c ,d) `(emit-bge ,(label-sym l) ,(dec (length (push *distance* 0))))))

(defcfun vp-beq-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-beq ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bne-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-bne ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-blt-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-blt ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-ble-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-ble ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bgt-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-bgt ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-bge-rr (s d l)
	(emit `(emit-cmp-rr ,s ,d) `(emit-bge ,(label-sym l) ,(dec (length (push *distance* 0))))))

(defcfun vp-seq-cr (c d) (emit `(emit-seq-cr ,c ,d)))
(defcfun vp-sne-cr (c d) (emit `(emit-sne-cr ,c ,d)))
(defcfun vp-slt-cr (c d) (emit `(emit-slt-cr ,c ,d)))
(defcfun vp-sle-cr (c d) (emit `(emit-sle-cr ,c ,d)))
(defcfun vp-sgt-cr (c d) (emit `(emit-sgt-cr ,c ,d)))
(defcfun vp-sge-cr (c d) (emit `(emit-sge-cr ,c ,d)))

(defcfun vp-seq-rr (s d) (emit `(emit-seq-rr ,s ,d)))
(defcfun vp-sne-rr (s d) (emit `(emit-sne-rr ,s ,d)))
(defcfun vp-slt-rr (s d) (emit `(emit-slt-rr ,s ,d)))
(defcfun vp-sle-rr (s d) (emit `(emit-sle-rr ,s ,d)))
(defcfun vp-sgt-rr (s d) (emit `(emit-sgt-rr ,s ,d)))
(defcfun vp-sge-rr (s d) (emit `(emit-sge-rr ,s ,d)))

(defcfun vp-push (&rest b) (emit `(emit-push ~b)))
(defcfun vp-pop (&rest b) (emit `(emit-pop ~b)))

(defcfun vp-call-abi (r b i x) (emit (cat `(emit-call-abi ,r ,b ,i) x)))
(defcfun vp-call (l) (emit `(emit-call ,(label-sym l))))
(defcfun vp-call-r (d) (emit `(emit-call-r ,d)))
(defcfun vp-call-i (b i) (emit `(emit-call-i ,b ,i)))
(defcfun-bind vp-call-p (l) (emit `(emit-call-p ,(label-sym l))))

(defcfun vp-jmp (l) (emit `(emit-jmp ,(label-sym l) ,(dec (length (push *distance* 0))))))
(defcfun vp-jmp-r (d) (emit `(emit-jmp-r ,d)))
(defcfun vp-jmp-i (b i) (emit `(emit-jmp-i ,b ,i)))
(defcfun vp-jmp-p (l) (emit `(emit-jmp-p ,(label-sym l))))

(defcfun vp-lea-i (b i d) (emit `(emit-lea-i ,b ,i ,d)))
(defcfun vp-lea-d (b i d) (emit `(emit-lea-d ,b ,i ,d)))
(defcfun vp-lea-p (l r) (emit `(emit-lea-p ,(label-sym l) ,r)))

(defcfun vp-cpy-cr (c d) (emit `(emit-cpy-cr ,c ,d)))
(defcfun-bind vp-cpy-rr (s d) (emit `(emit-cpy-rr ,s ,d)))
(defcfun-bind vp-cpy-ir (b i d) (emit `(emit-cpy-ir ,b ,i ,d)))
(defcfun vp-cpy-dr (b i d) (emit `(emit-cpy-dr ,b ,i ,d)))
(defcfun vp-cpy-pr (l d) (emit `(emit-cpy-pr ,(label-sym l) ,d)))
(defcfun-bind vp-cpy-ri (s b i) (emit `(emit-cpy-ri ,s ,b ,i)))
(defcfun vp-cpy-rd (s b i) (emit `(emit-cpy-rd ,s ,b ,i)))
(defcfun vp-cpy-rp (s l) (emit `(emit-cpy-rp ,s ,(label-sym l))))

(defcfun vp-cpy-ir-b (b i d) (emit `(emit-cpy-ir-b ,b ,i ,d)))
(defcfun vp-cpy-dr-b (b i d) (emit `(emit-cpy-dr-b ,b ,i ,d)))
(defcfun vp-cpy-ir-ub (b i d) (emit `(emit-cpy-ir-ub ,b ,i ,d)))
(defcfun vp-cpy-dr-ub (b i d) (emit `(emit-cpy-dr-ub ,b ,i ,d)))
(defcfun vp-cpy-ir-s (b i d) (emit `(emit-cpy-ir-s ,b ,i ,d)))
(defcfun vp-cpy-dr-s (b i d) (emit `(emit-cpy-dr-s ,b ,i ,d)))
(defcfun vp-cpy-ir-us (b i d) (emit `(emit-cpy-ir-us ,b ,i ,d)))
(defcfun vp-cpy-dr-us (b i d) (emit `(emit-cpy-dr-us ,b ,i ,d)))
(defcfun-bind vp-cpy-ir-i (b i d) (emit `(emit-cpy-ir-i ,b ,i ,d)))
(defcfun vp-cpy-dr-i (b i d) (emit `(emit-cpy-dr-i ,b ,i ,d)))
(defcfun-bind vp-cpy-ir-ui (b i d) (emit `(emit-cpy-ir-ui ,b ,i ,d)))
(defcfun vp-cpy-dr-ui (b i d) (emit `(emit-cpy-dr-ui ,b ,i ,d)))

(defcfun vp-cpy-ri-b (s b i) (emit `(emit-cpy-ri-b ,s ,b ,i)))
(defcfun vp-cpy-rd-b (s b i) (emit `(emit-cpy-rd-b ,s ,b ,i)))
(defcfun vp-cpy-ri-s (s b i) (emit `(emit-cpy-ri-s ,s ,b ,i)))
(defcfun vp-cpy-rd-s (s b i) (emit `(emit-cpy-rd-s ,s ,b ,i)))
(defcfun-bind vp-cpy-ri-i (s b i) (emit `(emit-cpy-ri-i ,s ,b ,i)))
(defcfun vp-cpy-rd-i (s b i) (emit `(emit-cpy-rd-i ,s ,b ,i)))

(defcfun vp-add-cr (c d) (emit `(emit-add-cr ,c ,d)))
(defcfun vp-add-rr (s d) (emit `(emit-add-rr ,s ,d)))
(defcfun vp-sub-cr (c d) (emit `(emit-sub-cr ,c ,d)))
(defcfun vp-sub-rr (s d) (emit `(emit-sub-rr ,s ,d)))
(defcfun vp-mul-cr (c d) (emit `(emit-mul-cr ,c ,d)))
(defcfun vp-mul-rr (s d) (emit `(emit-mul-rr ,s ,d)))
(defcfun vp-and-cr (c d) (emit `(emit-and-cr ,c ,d)))
(defcfun vp-and-rr (s d) (emit `(emit-and-rr ,s ,d)))
(defcfun vp-or-cr (c d) (emit `(emit-or-cr ,c ,d)))
(defcfun vp-or-rr (s d) (emit `(emit-or-rr ,s ,d)))
(defcfun vp-xor-cr (c d) (emit `(emit-xor-cr ,c ,d)))
(defcfun vp-xor-rr (s d) (emit `(emit-xor-rr ,s ,d)))

(defcfun vp-lnot-rr (r d) (emit `(emit-lnot-rr ,r ,d)))
(defcfun vp-land-rr (s d) (emit `(emit-land-rr ,s ,d)))

(defcfun vp-shl-cr (c d) (emit `(emit-shl-cr ,c ,d)))
(defcfun vp-shl-rr (s d) (emit `(emit-shl-rr ,s ,d)))
(defcfun vp-shr-cr (c d) (emit `(emit-shr-cr ,c ,d)))
(defcfun vp-shr-rr (s d) (emit `(emit-shr-rr ,s ,d)))
(defcfun vp-asr-cr (c d) (emit `(emit-asr-cr ,c ,d)))
(defcfun vp-asr-rr (s d) (emit `(emit-asr-rr ,s ,d)))

(defcfun vp-ret () (emit '(emit-ret)))
(defcfun vp-swp-rr (s d) (emit `(emit-swp-rr ,s ,d)))
(defcfun vp-ext-rr (s d) (emit `(emit-ext-rr ,s ,d)))
(defcfun vp-div-rrr (d r q) (emit `(emit-div-rrr ,d ,r ,q)))
(defcfun vp-div-rrr-u (d r q) (emit `(emit-div-rrr-u ,d ,r ,q)))

(defcfun vp-alloc (c) (emit `(emit-alloc ,c)))
(defcfun vp-free (c) (emit `(emit-free ,c)))

;;;;;;;;;;;;;;;;;;;;;
; Arch Emit Functions
;;;;;;;;;;;;;;;;;;;;;

(case *cpu*
(x86_64 (include 'sys/x64.inc))
(aarch64 (include 'sys/arm.inc))
(otherwise (throw "No such CPU !" *cpu*)))

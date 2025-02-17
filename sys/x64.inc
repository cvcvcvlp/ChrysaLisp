;;;;;;;;;;;;;;;;;;;;
; x64 Emit Functions
;;;;;;;;;;;;;;;;;;;;

(defcfun-bind reg (r)
	(find r '(r0 r1 r2 r3 rsp r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14)))

(defcvar 'stack_align 8 'stack_state '(r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14))

(defcfun stack-init ()
	(defq tk_state_size (* ptr_size (length stack_state)))
	(vp-sub-cr (+ tk_state_size (* ptr_size 2)) r1)
	(f-bind 'sys_task 'stop r2)
	(vp-cpy-ri r2 r1 (+ tk_state_size ptr_size))
	(vp-cpy-ri r4 r1 tk_state_size))

(defcfun-bind emit-rr (o s d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl sh 2) dh) o (+ 0xc0 (shl sl 3) dl)))

(defcfun emit-dr (o s1 s2 d)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) (shl s2h 1) s1h) o)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl dl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl dl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun emit-dr-bs (o s1 s2 d)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) (shl s2h 1) s1h) 0xf o)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl dl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl dl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun-bind emit-ir (o s c d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) sh) o)
	(cond
		((and (= c 0) (/= s 5) (/= s 13))
			(emit-byte (+ (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-ir constant out of range !" c))))

(defcfun emit-ir-bs (o s c d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) sh) 0x0f o)
	(cond
		((and (= c 0) (/= s 5) (/= s 13))
			(emit-byte (+ (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-ir-bs constant out of range !" c))))

(defcfun-bind emit-pr (o c d)
	(defq d (reg d) dl (logand 7 d) dh (shr d 3) c (- c 7))
	(cond
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x48 (shl dh 2)) o (+ 0x5 (shl dl 3)))
			(emit-int c))
		(t (throw "emit-pr constant out of range !" c))))

(defcfun-bind emit-shift-cr (o c d)
	(unless (= c 0)
		(defq d (reg d) dl (logand 7 d) dh (shr d 3))
		(emit-byte (+ 0x48 dh))
		(cond
			((= c 1)
				(emit-byte 0xd1 (+ o dl)))
			((<= c 0xff)
				(emit-byte 0xc1 (+ o dl) c))
			(t (throw "emit-shift-cr constant out of range !" c)))))

(defcfun emit-shift-rr (o s d)
	(cond
		((eql s r1)
			(defq d (reg d) dl (logand 7 d) dh (shr d 3))
			(emit-byte (+ 0x48 dh) 0xd3 (+ o dl)))
		((eql d r1)
			(defq sn (reg s) sl (logand 7 sn) sh (shr sn 3))
			(emit-swp-rr s d)
			(emit-byte (+ 0x48 sh) 0xd3 (+ o sl))
			(emit-swp-rr s d))
		(t
			(emit-push d s r0 r1)
			(emit-cpy-ir rsp 24 r0)
			(emit-cpy-ir rsp 16 r1)
			(emit-byte 0x48 0xd3 o)
			(emit-cpy-ri r0 rsp 24)
			(emit-pop d s r0 r1))))

(defcfun-bind emit-call-jump-p (o c)
	(emit-byte 0xff o)
	(emit-int (- c 6)))

(defcfun emit-call-jump-r (o d)
	(defq d (reg d) dl (logand 7 d))
	(if (>= d 8) (emit-byte 0x41))
	(emit-byte 0xff (+ o dl)))

(defcfun-bind emit-call-jump-i (o d c)
	(defq d (reg d) dl (logand 7 d))
	(if (>= d 8) (emit-byte 0x41))
	(emit-byte 0xff)
	(cond
		((and (= c 0) (/= d 5) (/= d 13))
			(emit-byte (+ o dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 o dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 o dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-call-jump-i constant out of range !" c))))

(defcfun-bind emit-push (&rest b)
	(each (lambda (r)
		(if (>= (setq r (reg r)) 8)
			(emit-byte 0x41 (+ 0x48 r))
			(emit-byte (+ 0x50 r)))) b))

(defcfun-bind emit-pop (&rest b)
	(each-rev (lambda (r)
		(if (>= (setq r (reg r)) 8)
			(emit-byte 0x41 (+ 0x50 r))
			(emit-byte (+ 0x58 r)))) b))

(defcfun-bind emit-branch (o c d)
	(defq m (elem d *distance*))
	(and (/= *pass* 0) (> (abs c) (abs m)) (elem-set d *distance* (setq m c)))
	(cond
		((<= -0x80 (- m 2) 0x7f)
			(emit-byte o (- c 2)))
		((<= -0x80000000 (- m 6) 0x7fffffff)
			(emit-byte 0xf (+ 0x10 o))
			(emit-int (- c 6)))
		(t (throw "emit-branch constant out of range !" c))))

(defcfun-bind emit-beq (l d) (emit-branch 0x74 (- l *pc*) d))
(defcfun-bind emit-bne (l d) (emit-branch 0x75 (- l *pc*) d))
(defcfun-bind emit-bge (l d) (emit-branch 0x7d (- l *pc*) d))
(defcfun emit-blt (l d) (emit-branch 0x7c (- l *pc*) d))
(defcfun emit-ble (l d) (emit-branch 0x7e (- l *pc*) d))
(defcfun emit-bgt (l d) (emit-branch 0x7f (- l *pc*) d))

(defcfun-bind emit-xor-rr (s d) (emit-rr 0x31 s d))

(defcfun-bind emit-or-cr (c r)
	(unless (= c 0)
		(defq r (reg r) rl (logand 7 r) rh (shr r 3))
		(emit-byte (+ 0x48 rh))
		(cond
			((<= -0x80 c 0x7f)
				(emit-byte 0x83 (+ 0xc8 rl) c))
			((<= -0x80000000 c 0x7fffffff)
				(if (= r 0)
					(emit-byte 0x0d)
					(emit-byte 0x81 (+ 0xc8 rl)))
				(emit-int c))
			(t (throw "emit-or-cr constant out of range !" c)))))

(defcfun-bind emit-cmp-cr (c r)
	(if (= c 0) (emit-rr 0x85 r r)
		(progn
			(defq r (reg r) rl (logand 7 r) rh (shr r 3))
			(emit-byte (+ 0x48 rh))
			(cond
				((<= -0x80 c 0x7f)
					(emit-byte 0x83 (+ 0xf8 rl) c))
				((<= -0x80000000 c 0x7fffffff)
					(if (= r 0)
						(emit-byte 0x3d)
						(emit-byte 0x81 (+ 0xf8 rl)))
					(emit-int c))
				(t (throw "emit-cmp-cr constant out of range !" c))))))

(defcfun-bind emit-cmp-rr (s d) (emit-rr 0x39 s d))

(defcfun emit-seq-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x74 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sne-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x75 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-slt-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x7c 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sle-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x7e 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sgt-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x7f 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sge-cr (c d)
	(emit-cmp-cr c d) (emit-byte 0x7d 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))

(defcfun emit-seq-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x74 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sne-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x75 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-slt-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x7c 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sle-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x7e 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sgt-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x7f 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))
(defcfun emit-sge-rr (s d)
	(emit-cmp-rr s d) (emit-byte 0x7d 5) (emit-xor-rr d d) (emit-byte 0xeb 4) (emit-or-cr -1 d))

(defcfun emit-call (l)
	(cond
		((<= -0x80000000 (defq c (- l *pc* 5)) 0x7fffffff)
			(emit-byte 0xe8)
			(emit-int c))
		(t (throw "emit-call constant out of range !" c))))

(defcfun emit-call-r (r) (emit-call-jump-r 0xd0 r))
(defcfun emit-call-i (d c) (emit-call-jump-i 0x10 d c))
(defcfun emit-call-p (l) (emit-call-jump-p 0x15 (- l *pc*)))

(defcfun-bind emit-jmp (l d)
	(defq m (elem d *distance*) c (- l *pc*))
	(and (/= *pass* 0) (> (abs c) (abs m)) (elem-set d *distance* (setq m c)))
	(cond
		((<= -0x80 (- m 2) 0x7f)
			(emit-byte (+ 0x2 0xe9) (- c 2)))
		((<= -0x80000000 (- m 5) 0x7fffffff)
			(emit-byte 0xe9)
			(emit-int (- c 5)))
		(t (throw "emit-jmp constant out of range !" c))))

(defcfun emit-jmp-r (r) (emit-call-jump-r 0xe0 r))
(defcfun emit-jmp-i (d c) (emit-call-jump-i 0x20 d c))
(defcfun emit-jmp-p (l) (emit-call-jump-p 0x25 (- l *pc*)))

(defcfun emit-lea-i (s c d) (unless (and (= c 0) (eql s d)) (emit-ir 0x8d s c d)))
(defcfun emit-lea-d (s1 s2 d) (emit-dr 0x8d s1 s2 d))
(defcfun-bind emit-lea-p (l d) (emit-pr 0x8d (- l *pc*) d))

(defcfun-bind emit-cpy-cr (c r)
	(if (= c 0)
		(emit-xor-rr r r)
		(progn
			(defq rn (reg r) rl (logand rn 7) rh (shr rn 3))
			(cond
				((= -1 c)
					(emit-or-cr -1 r))
				((<= 0 c 0xffffffff)
					(if (>= rn 8) (emit-byte 0x41))
					(emit-byte (+ 0xb8 rl))
					(emit-int c))
				((<= -0x80000000 c -1)
					(emit-byte (+ 0x48 rh) 0xc7 (+ 0xc0 rl))
					(emit-int c))
				(t
					(emit-byte (+ 0x48 rh) (+ 0xb8 rl))
					(emit-long c))))))

(defcfun-bind emit-cpy-rr (s d) (unless (eql s d) (emit-rr 0x89 s d)))
(defcfun-bind emit-cpy-ir (s c d) (emit-ir 0x8b s c d))
(defcfun emit-cpy-dr (s1 s2 d) (emit-dr 0x8b s1 s2 d))
(defcfun-bind emit-cpy-pr (l d) (emit-pr 0x8b (- l *pc*) d))

(defcfun-bind emit-cpy-ri (s d c)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(cond
		((and (= c 0) (/= d 5) (/= d 13))
			(emit-byte (+ 0x48 (shl sh 2) dh) 0x89 (+ (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x48 (shl sh 2) dh) 0x89 (+ 0x40 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x48 (shl sh 2) dh) 0x89 (+ 0x80 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ri constant out of range !" c))))

(defcfun emit-cpy-rd (d s1 s2)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) (shl s2h 1) s1h) 0x89)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl dl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl dl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun emit-cpy-rp (d l)
	(defq d (reg d) dl (logand 7 d) dh (shr d 3) c (- l *pc* 7))
	(cond
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x48 (shl dh 2)) 0x89 (+ 0x5 (shl dl 3)))
			(emit-int c))
		(t (throw "emit-cpy-rp constant out of range !" c))))

(defcfun emit-cpy-ir-b (s c d) (emit-ir-bs 0xbe s c d))
(defcfun emit-cpy-dr-b (s1 s2 d) (emit-dr-bs 0xbe s1 s2 d))
(defcfun emit-cpy-ir-ub (s c d) (emit-ir-bs 0xb6 s c d))
(defcfun emit-cpy-dr-ub (s1 s2 d) (emit-dr-bs 0xb6 s1 s2 d))
(defcfun emit-cpy-ir-s (s c d) (emit-ir-bs 0xbf s c d))
(defcfun emit-cpy-dr-s (s1 s2 d) (emit-dr-bs 0xbf s1 s2 d))
(defcfun emit-cpy-ir-us (s c d) (emit-ir-bs 0xb7 s c d))
(defcfun emit-cpy-dr-us (s1 s2 d) (emit-dr-bs 0xb7 s1 s2 d))

(defcfun-bind emit-cpy-ir-i (s c d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) sh) 0x63)
	(cond
		((and (= c 0) (/= s 5) (/= s 13))
			(emit-byte (+ (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ir-i constant out of range !" c))))

(defcfun emit-cpy-dr-i (s1 s2 d)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) (shl s2h 1) s1h) 0x63)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl dl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl dl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun-bind emit-cpy-ir-ui (s c d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(if (or (>= s 8) (>= d 8)) (emit-byte (+ 0x40 (shl dh 2) sh)))
	(emit-byte 0x8b)
	(cond
		((and (= c 0) (/= s 5) (/= s 13))
			(emit-byte (+ (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl dl 3) sl))
			(if (or (= s 4) (= s 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ir-ui constant out of range !" c))))

(defcfun emit-cpy-dr-ui (s1 s2 d)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(if (or (>= s1 8) (>= s2 8) (>= d 8)) (emit-byte (+ 0x40 (shl dh 2) (shl s2h 1) s1h)))
	(emit-byte 0x8b)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl dl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl dl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun emit-cpy-ri-b (s d c)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(if (or (>= s 4) (>= d 8)) (emit-byte (+ 0x40 (shl sh 2) dh)))
	(emit-byte 0x88)
	(cond
		((and (= c 0) (/= d 5) (/= d 13))
			(emit-byte (+ (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ri-b constant out of range !" c))))

(defcfun emit-cpy-rd-b (r s1 s2)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		r (reg r) rl (logand 7 r) rh (shr r 3))
	(if (or (>= s1 8) (>= s2 8) (>= r 4)) (emit-byte (+ 0x40 (shl rh 2) (shl s2h 1) s1h)))
	(emit-byte 0x88)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl rl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl rl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun emit-cpy-ri-s (s d c)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte 0x66)
	(if (or (>= s 8) (>= d 8)) (emit-byte (+ 0x40 (shl sh 2) dh)))
	(emit-byte 0x89)
	(cond
		((and (= c 0) (/= d 5) (/= d 13))
			(emit-byte (+ (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ri-s constant out of range !" c))))

(defcfun emit-cpy-rd-s (r s1 s2)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		r (reg r) rl (logand 7 r) rh (shr r 3))
	(emit-byte 0x66)
	(if (or (>= s1 8) (>= s2 8) (>= r 8)) (emit-byte (+ 0x40 (shl rh 2) (shl s2h 1) s1h)))
	(emit-byte 0x89)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl rl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl rl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun-bind emit-cpy-ri-i (s d c)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(if (or (>= s 8) (>= d 8)) (emit-byte (+ 0x40 (shl sh 2) dh)))
	(emit-byte 0x89)
	(cond
		((and (= c 0) (/= d 5) (/= d 13))
			(emit-byte (+ (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24)))
		((<= -0x80 c 0x7f)
			(emit-byte (+ 0x40 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-byte c))
		((<= -0x80000000 c 0x7fffffff)
			(emit-byte (+ 0x80 (shl sl 3) dl))
			(if (or (= d 4) (= d 12)) (emit-byte 0x24))
			(emit-int c))
		(t (throw "emit-cpy-ri-i constant out of range !" c))))

(defcfun emit-cpy-rd-i (r s1 s2)
	(defq s1 (reg s1) s1l (logand 7 s1) s1h (shr s1 3)
		s2 (reg s2) s2l (logand 7 s2) s2h (shr s2 3)
		r (reg r) rl (logand 7 r) rh (shr r 3))
	(if (or (>= s1 8) (>= s2 8) (>= r 8)) (emit-byte (+ 0x40 (shl rh 2) (shl s2h 1) s1h)))
	(emit-byte 0x89)
	(cond
		((and (/= s1 5) (/= s1 13))
			(emit-byte (+ 0x04 (shl rl 3)) (+ (shl s2l 3) s1l)))
		(t
			(emit-byte (+ 0x44 (shl rl 3)) (+ (shl s2l 3) s1l) 0))))

(defcfun-bind emit-add-cr (c r)
	(unless (= c 0)
		(defq r (reg r) rl (logand 7 r) rh (shr r 3))
		(emit-byte (+ 0x48 rh))
		(cond
			((= 1 c)
				(emit-byte 0xff (+ 0xc0 rl)))
			((<= -0x80 c 0x7f)
				(emit-byte 0x83 (+ 0xc0 rl) c))
			((<= -0x80000000 c 0x7fffffff)
				(if (= r 0)
					(emit-byte 0x05)
					(emit-byte 0x81 (+ 0xc0 rl)))
				(emit-int c))
			(t (throw "emit-add-cr constant out of range !" c)))))

(defcfun-bind emit-add-rr (s d) (emit-rr 0x01 s d))

(defcfun-bind emit-sub-cr (c r)
	(unless (= c 0)
		(defq r (reg r) rl (logand 7 r) rh (shr r 3))
		(emit-byte (+ 0x48 rh))
		(cond
			((= 1 c)
				(emit-byte 0xff (+ 0xc8 rl)))
			((<= -0x80 c 0x7f)
				(emit-byte 0x83 (+ 0xe8 rl) c))
			((<= -0x80000000 c 0x7fffffff)
				(if (= r 0)
					(emit-byte 0x2d)
					(emit-byte 0x81 (+ 0xe8 rl)))
				(emit-int c))
			(t (throw "emit-sub-cr constant out of range !" c)))))

(defcfun-bind emit-sub-rr (s d) (emit-rr 0x29 s d))

(defcfun-bind emit-and-cr (c r)
	(unless (= c -1)
		(if (= c 0)
			(emit-xor-rr r r)
			(progn
				(defq r (reg r) rl (logand 7 r) rh (shr r 3))
				(emit-byte (+ 0x48 rh))
				(cond
					((<= -0x80 c 0x7f)
						(emit-byte 0x83 (+ 0xe0 rl) c))
					((<= -0x80000000 c 0x7fffffff)
						(if (= r 0)
							(emit-byte 0x25)
							(emit-byte 0x81 (+ 0xe0 rl)))
						(emit-int c))
					(t (throw "emit-and-cr constant out of range !" c)))))))

(defcfun emit-and-rr (s d) (unless (eql s d) (emit-rr 0x21 s d)))

(defcfun emit-or-rr (s d) (unless (eql s d) (emit-rr 0x09 s d)))

(defcfun emit-xor-cr (c r)
	(unless (= c 0)
		(defq r (reg r) rl (logand 7 r) rh (shr r 3))
		(emit-byte (+ 0x48 rh))
		(cond
			((<= -0x80 c 0x7f)
				(emit-byte 0x83 (+ 0xf0 rl) c))
			((<= -0x80000000 c 0x7fffffff)
				(if (= r 0)
					(emit-byte 0x35)
					(emit-byte 0x81 (+ 0xf0 rl)))
				(emit-int c))
			(t (throw "emit-xor-cr constant out of range !" c)))))

(defcfun emit-lnot-rr (r d)
	(defq dlh (reg d))
	(if (>= dlh 4)
		(throw "emit-lnot-rr illegal register !" d))
	(emit-cpy-rr d r)
	(emit-xor-rr d d)
	(emit-cmp-cr 0 r)
	(emit-byte 0x0f 0x94 (+ 0xc0 dlh)))

(defcfun emit-land-rr (s d)
	(defq slh (reg s) dlh (reg d))
	(if (or (>= slh 4) (>= dlh 4))
		(throw "emit-land-rr illegal register !" (list s d)))
	(emit-cmp-cr 0 d)
	(emit-byte 0x0f 0x95 (+ 0xc0 dlh))
	(emit-cmp-cr 0 s)
	(emit-byte 0x0f 0x95 (+ 0xc0 slh))
	(emit-and-rr s d))

(defcfun-bind emit-shl-cr (c r) (emit-shift-cr 0xe0 c r))
(defcfun emit-shl-rr (s d) (emit-shift-rr 0xe0 s d))
(defcfun emit-shr-cr (c r) (emit-shift-cr 0xe8 c r))
(defcfun emit-shr-rr (s d) (emit-shift-rr 0xe8 s d))
(defcfun emit-asr-cr (c r) (emit-shift-cr 0xf8 c r))
(defcfun emit-asr-rr (s d) (emit-shift-rr 0xf8 s d))

(defcfun emit-mul-cr (c r)
	(if (= c 0)
		(emit-xor-rr r r)
		(progn
			(defq r (reg r) rl (logand r 7) rh (shr r 3))
			(cond
				((= 1 c))
				((= -1 c)
					(emit-byte (+ 0x48 rh) 0xf7 (+ 0xd8 rl)))
				((<= -0x80 c 0x7f)
					(emit-byte (+ 0x48 (shl rh 2) rh) 0x6b (+ 0xc0 rl (shl rl 3)) c))
				((<= -0x80000000 c 0x7fffffff)
					(emit-byte (+ 0x48 (shl rh 2) rh) 0x69 (+ 0xc0 rl (shl rl 3)))
					(emit-int c))
				(t (throw "emit-mul-cr constant out of range !" c))))))

(defcfun-bind emit-mul-rr (s d)
	(defq s (reg s) sl (logand 7 s) sh (shr s 3)
		d (reg d) dl (logand 7 d) dh (shr d 3))
	(emit-byte (+ 0x48 (shl dh 2) sh) 0x0f 0xaf (+ 0xc0 (shl dl 3) sl)))

(defcfun emit-swp-rr (s d)
	(unless (eql s d)
		(if (eql s r0) (setq s d d r0))
		(defq s (reg s) sl (logand 7 s) sh (shr s 3)
			d (reg d) dl (logand 7 d) dh (shr d 3))
		(if (= d 0)
			(emit-byte (+ 0x48 sh) (+ 0x90 sl))
			(emit-byte (+ 0x48 (shl sh 2) dh) 0x87 (+ 0xc0 (shl sl 3) dl)))))

(defcfun emit-ext-rr (s d)
	(cond
		((and (eql s r0) (eql d r2))
			(emit-byte 0x48 0x99))
		((eql s d)
			(emit-asr-cr 63 d))
		(t
			(emit-cpy-rr s d)
			(emit-asr-cr 63 d))))

(defcfun emit-div-rrr (s d1 d2)
	(cond
		((and (eql d1 r2) (eql d2 r0))
			(defq s (reg s) sl (logand 7 s) sh (shr s 3))
			(emit-byte (+ 0x48 sh) 0xF7 (+ 0xF8 sl)))
		((and (eql d1 r0) (eql d2 r2) (not (eql s r0)) (not (eql s r2)))
			(defq s (reg s) sl (logand 7 s) sh (shr s 3))
			(emit-byte 0x48 0x92 (+ 0x48 sh) 0xF7 (+ 0xF8 sl) 0x48 0x92))
		(t
			(emit-push d2 d1 s r2 r1 r0)
			(emit-cpy-ir rsp 24 r1)
			(emit-cpy-ir rsp 32 r2)
			(emit-cpy-ir rsp 40 r0)
			(emit-byte 0x48 0xF7 0xF9)
			(emit-cpy-ri r2 rsp 32)
			(emit-cpy-ri r0 rsp 40)
			(emit-pop d2 d1 s r2 r1 r0))))

(defcfun emit-div-rrr-u (s d1 d2)
	(cond
		((and (eql d1 r2) (eql d2 r0))
			(defq s (reg s) sl (logand 7 s) sh (shr s 3))
			(emit-byte (+ 0x48 sh) 0xF7 (+ 0xF0 sl)))
		((and (eql d1 r0) (eql d2 r2) (not (eql s r0)) (not (eql s r2)))
			(defq s (reg s) sl (logand 7 s) sh (shr s 3))
			(emit-byte 0x48 0x92 (+ 0x48 sh) 0xF7 (+ 0xF0 sl) 0x48 0x92))
		(t
			(emit-push d2 d1 s r2 r1 r0)
			(emit-cpy-ir rsp 24 r1)
			(emit-cpy-ir rsp 32 r2)
			(emit-cpy-ir rsp 40 r0)
			(emit-byte 0x48 0xF7 0xF1)
			(emit-cpy-ri r2 rsp 32)
			(emit-cpy-ri r0 rsp 40)
			(emit-pop d2 d1 s r2 r1 r0))))

(defcfun-bind emit-alloc (c) (emit-sub-cr (align c stack_align) rsp))
(defcfun-bind emit-free (c) (emit-add-cr (align c stack_align) rsp))
(defcfun-bind emit-ret () (emit-byte 0xc3))

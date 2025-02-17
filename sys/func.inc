(include 'sys/pii/class.inc)
(include 'sys/task/class.inc)
(include 'sys/vp.inc)

;;;;;;;;;;;;;;;;;
; func structures
;;;;;;;;;;;;;;;;;

(def-struct 'fn_header 'ln_fnode)
	(ushort 'length)
	(ushort 'entry)
	(ushort 'links)
	(ushort 'paths)
	(ushort 'stack)
	(offset 'pathname)
(def-struct-end)

;;;;;;;;;;;
; functions
;;;;;;;;;;;

(defcvar '*func-name* nil '*strings* (list) '*paths* (list) '*links* (list)
	'*func-align* nil '*func-syms* (list))

(defcfun-bind link-sym (_)
	(str "rl_" _))

(defcfun-bind path-sym (_)
	(str "rp_" _))

(defcfun string-sym (_)
	(str "rs_" _))

(defcfun-bind def-func (*name* &optional falign fstack)
	(print "-> " (cat "obj/" *cpu* "/" *abi* "/" (str *name*)))
	(emit-reset)
	(setq *func-name* *name* *switch-nxt* 0 *func-align* (opt falign stack_align))
	(clear *strings* *paths* *links* *func-syms*)
(vp-label 'func_start)
	(vp-long -1)
	(vp-short `(- ,(label-sym 'func_end) ,(label-sym 'func_start))
		`(- ,(label-sym 'func_entry) ,(label-sym 'func_start))
		`(- ,(label-sym 'func_links) ,(label-sym 'func_start))
		`(- ,(label-sym 'func_paths) ,(label-sym 'func_start))
		(opt fstack tk_stack_size))
(vp-label 'func_name_start)
	(vp-string (str *name*))
	(vp-byte 0 `(- ,(label-sym 'func_entry) ,(label-sym 'func_name_start)))
	(vp-align ptr_size `(- ,(label-sym 'func_entry) ,(label-sym 'func_name_start)))
(vp-label 'func_entry)
	(push-scope))

(defcfun-bind def-func-end ()
	(pop-scope-checked)
	(each (lambda (s)
	(vp-label (string-sym _))
		(vp-string s) (vp-byte 0)) *strings*)
	(vp-align ptr_size)
(vp-label 'func_links)
	(each (lambda (s)
	(vp-label (link-sym _))
		(vp-long `(- ,(label-sym (path-sym s)) *pc*))) *links*)
(vp-label 'func_paths)
	(each (lambda (s)
	(vp-label (path-sym _))
		(vp-string (str s)) (vp-byte 0)) *paths*)
	(vp-align ptr_size)
(vp-label 'func_end)
	(opt-emit-buffer)
	(when debug_emit
		(each print-emit *emit-buffer*))
	(emit-passes)
	(apply undef (cat (list *compile-env*) *func-syms*))
	(save *out-buffer* (cat "obj/" *cpu* "/" *abi* "/" (str *func-name*))))

(defcfun-bind fn-add-string (s)
	(defq i -1)
	(while (and (< (setq i (inc i)) (length *strings*)) (not (eql s (elem i *strings*)))))
	(if (= i (length *strings*)) (push *strings* s)) i)

(defcfun-bind fn-add-path (p)
	(if (defq i (find p *paths*)) i
		(dec (length (push *paths* p)))))

(defcfun-bind fn-add-link (p)
	(push *links* (fn-add-path p)))

(defcfun-bind fn-find-link (p)
	(defq i -1)
	(while (and (< (setq i (inc i)) (length *links*)) (not (eql p (elem (elem i *links*) *paths*)))))
	(if (= i (length *links*)) (fn-add-link p)) i)

(defcfun-bind fn-string (s r)
	(vp-lea-p (string-sym (fn-add-string s)) r))

(defcfun-bind fn-bind (p r)
	(if (def? p) (throw "Can't bind to inline function !" p)
		(vp-cpy-pr (link-sym (fn-find-link p)) r)))

(defcfun-bind fn-call (p)
	(if (def? p) ((eval p))
		(vp-call-p (link-sym (fn-find-link p)))))

(defcfun-bind fn-jump (p)
	(if (def? p) (throw "Can't jump to inline function !" p)
		(vp-jmp-p (link-sym (fn-find-link p)))))

(defcfun abort (&optional s)
	(call 'sys_pii 'write_str (list 2 (opt s "Abort !")))
	(jump 'sys_pii 'exit '(1)))

(defcfun assert (b &optional d)
	(when debug_mode
		(vpifnot b)
			(abort (opt d "Assert Failure !"))
		(endif)))

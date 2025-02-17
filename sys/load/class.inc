(include 'sys/class.inc)
(include 'sys/abi.inc)

;;;;;;;;;;;;;;;;;;;
; load static class
;;;;;;;;;;;;;;;;;;;

(def-class 'sys_load)
(dec-method 'statics 'sys/load/statics)
(dec-method 'init 'sys/load/init)
(dec-method 'bind 'sys/load/bind 'static '(r0) '(r0))

;;;;;;;;;;;;;;;;;
; load structures
;;;;;;;;;;;;;;;;;

(defcvar 'ld_page_size 4096)
(defcvar 'ld_block_size (* 64 1024))
(defcvar 'ld_reloc_buf_size (* ld_page_size 2))
(defcvar 'ld_name_size 128)
(defcvar 'ld_reloc_size (- ld_reloc_buf_size ld_name_size stat_size))

(def-struct 'ld_statics)
	(ptr 'host_funcs)
	(ptr 'function_flist)
	(ptr 'block_flist)
	(ptr 'block_start)
	(ptr 'block_end)
	(ptr 'reloc_stack)
	(ptr 'reloc_buffer)
	(ptr 'stat_buffer)
	(ptr 'name_buffer)
(def-struct-end)

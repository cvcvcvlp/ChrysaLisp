(include 'sys/heap/class.inc)
(include 'sys/list/class.inc)
(include 'sys/mail/class.inc)

(defcvar 'SCREEN_WIDTH 1280)
(defcvar 'SCREEN_HEIGHT 960)

;;;;;;;;;;;;;;;;;;
; gui static class
;;;;;;;;;;;;;;;;;;

(def-class 'gui)
(dec-method 'statics 'gui/gui/statics)
(dec-method 'init 'gui/gui/init)
(dec-method 'update 'gui/gui/update 'static '(r0))
(dec-method 'add 'gui/gui/add 'static '(r0) '(r0))
(dec-method 'gui 'gui/gui/gui)

(dec-method 'lisp_add 'gui/gui/lisp_add 'static '(r0 r1) '(r0 r1))

;;;;;;;;;;;;;;;;
; gui structures
;;;;;;;;;;;;;;;;

(def-struct 'gui_statics)
	(struct 'rect_heap 'hp_heap)
	(ptr 'window)
	(ptr 'screen)
	(ulong 'renderer)
	(ptr 'old_region)
	(ulong 'next_id)
	(uint 'dirty_flag)
(def-struct-end)

;;;;;;;;;;;;;;;;;;;;
; gui event msg data
;;;;;;;;;;;;;;;;;;;;

(def-enum 'ev_type)
	(enum 'mouse 'key 'action)
(def-enum-end)

(def-struct 'ev_msg)
	(ulong 'target_id)
	(ulong 'type)
(def-struct-end)

(def-struct 'ev_msg_mouse 'ev_msg)
	(uint 'buttons)
	(int 'x)
	(int 'y)
	(int 'rx)
	(int 'ry)
(def-struct-end)

(def-struct 'ev_msg_key 'ev_msg)
	(uint 'keycode)
	(uint 'key)
(def-struct-end)

(def-struct 'ev_msg_action 'ev_msg)
	(ulong 'source_id)
(def-struct-end)

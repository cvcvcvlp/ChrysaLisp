;import settings
(run 'sys/lisp.inc)
(run 'gui/lisp.inc)

(structure 'event 0
	(byte 'win_close)
	(byte 'win_next)
	(byte 'win_prev))

(defq images '(apps/images/frill.cpm apps/images/magicbox.cpm
	apps/images/captive.cpm apps/images/balls.cpm apps/images/banstand.cpm
	apps/images/bucky.cpm apps/images/circus.cpm apps/images/cyl_test.cpm
	apps/images/logo.cpm apps/images/mice.cpm apps/images/molecule.cpm
	apps/images/nippon3.cpm apps/images/piramid.cpm apps/images/rings.cpm
	apps/images/sharpend.cpm apps/images/stairs.cpm apps/images/temple.cpm
	apps/images/vermin.cpm) index 0 id t)

(ui-tree window (create-window window_flag_close) nil
	(ui-element image_flow (create-flow) ('flow_flags (bit-or flow_flag_down flow_flag_fillw))
		(ui-element _ (create-flow) ('flow_flags (bit-or flow_flag_right flow_flag_fillh) 'color 0xff00ff00)
			(button-connect-click (ui-element _ (create-button) ('text "Prev")) event_win_prev)
			(button-connect-click (ui-element _ (create-button) ('text "Next")) event_win_next))
		(ui-element frame (canvas-load (elem index images)))))

(window-set-title window (elem index images))
(window-connect-close window event_win_close)
(bind '(w h) (view-pref-size window))
(gui-add (view-change window 64 64 w h))

(defun win-refresh (_)
	(view-sub frame)
	(setq index _ frame (canvas-load (elem index images)))
	(view-layout (view-add-back image_flow frame))
	(view-dirty (window-set-title window (elem index images)))
	(bind '(x y _ _) (view-get-bounds window))
	(bind '(w h) (view-pref-size window))
	(view-dirty-all (view-change window x y w h)))

(while id
	(cond
		((eq (setq id (read-long ev_msg_target_id (defq msg (mail-mymail)))) event_win_close)
			(setq id nil))
		((eq id event_win_next)
			(win-refresh (mod (inc index) (length images))))
		((eq id event_win_prev)
			(win-refresh (mod (add (dec index) (length images)) (length images))))
		(t (view-event window msg))))

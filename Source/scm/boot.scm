;;; Copyright 2023 Alkaline Games, LLC.
;;;
;;; This Source Code Form is subject to the terms of the Mozilla Public
;;; License, v. 2.0. If a copy of the MPL was not distributed with this
;;; file, You can obtain one at https://mozilla.org/MPL/2.0/.

;;; AlkalineScemeUE boot program

; TODO: @@@ NEEDS TO BE A LIST OF EXPRESSIONS FOR NOW
(
(ue-log "TRACE SCHEME >>> BEGIN booting AlkalineSchemeUE")

(define (hook-input device action durations func)
  ; TODO: @@@ ASSUMING 'pointing DEVICE
  (ue-log "TRACE SCHEME >>> BEGIN hook-input")
  (ue-bind-input-touch 'pressed
    (lambda (finger location) (ue-print-string "pointing pressed")))
  (ue-bind-input-touch 'released
    (lambda (finger location) (ue-print-string "pointing released")))
  (ue-bind-input-touch 'repeated
    (lambda (finger location) (ue-print-string "pointing repeated")))
  (open-scheme-editor '(800 450))
  (ue-log "TRACE SCHEME <<< END hook-input"))

(define (open-scheme-editor screen-pos)
  ; TODO: ### IMPLEMENT
  (ue-log (format #f "TRACE SCHEME (open-scheme-editor ~S)" screen-pos)))

(ue-hook-on-world-added
  (lambda (world)
    (ue-log "TRACE SCHEME lambda from (ue-hook-on-world-added))")
    (ue-hook-on-world-begin-play
      world
      (lambda (world)
        (ue-log "TRACE SCHEME lambda from (ue-hook-on-world-begin-play)")
        (ue-print-string "Alkaline Scheme is alive")
        (hook-input 'pointing 'press '(200 0 200 0)
          (lambda (screen-pos) (open-scheme-editor screen-pos)))))))

; TODO: ### NEED TO HOOK ON EXISTING WORLDS AS WELL

;(ue-print-string "Alkaline Scheme is alive")

(ue-log "TRACE SCHEME <<< END booting AlkalineSchemeUE")
)
; TODO: ### s7_eval_c_string(...) is reporting "syntax error"
; here after the last expression successfully executes ?!?

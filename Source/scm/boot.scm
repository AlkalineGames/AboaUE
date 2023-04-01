;;; Copyright 2023 Alkaline Games, LLC.
;;;
;;; This Source Code Form is subject to the terms of the Mozilla Public
;;; License, v. 2.0. If a copy of the MPL was not distributed with this
;;; file, You can obtain one at https://mozilla.org/MPL/2.0/.

;;; AlkalineScemeUE boot program

; TODO: @@@ NEEDS TO BE A LIST OF EXPRESSIONS FOR NOW
(
(ue-log "BEGIN booting AlkalineSchemeUE")

(define (hook-ue-input device action durations func)
  ; TODO: ### IMPLEMENT
  (ue-log (format #f "called (hook-ue-input ~S ~S ~S ~S)" device action durations func)))

(define (open-scheme-editor screen-pos)
  ; TODO: ### IMPLEMENT
  (ue-log (format #f "called (open-scheme-editor ~S)" screen-pos)))

(define (print-ue text)
  ; TODO: ### IMPLEMENT
  (ue-log (format #f "called (print-ue ~S)" text)))

(hook-ue-input 'pointing 'press '(200 0 200 0)
  (lambda (screen-pos) (open-scheme-editor screen-pos)))

(open-scheme-editor '(800 450))

(print-ue "something")

(ue-log "..END booting AlkalineSchemeUE")
)
; TODO: ### s7_eval_c_string(...) is reporting "syntax error"
; here after the last expression successfully executes ?!?

;; Project-wide Emacs settings
;; We use clang-format for the whole project
;; see misc/fmt.py  - but this sets up the basic
;; environment for c++
(
 (c++-mode
  (indent-tabs-mode . nil)
  (c-basic-offset . 2))
 (c-mode
  (indent-tabs-mode . nil)
  (c-basic-offset . 2))
 )

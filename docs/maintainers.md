# How to apply patches from smf-dev@googlegroups.com?

The gist of it is that you need to integrate it with an editor so you can do

```
git am file.txt
```

First step is to set up ` mutt`



```bash

# ~/.muttrc

set ssl_starttls=yes
set ssl_force_tls=yes
set imap_user = '...@gmail.com'
set imap_pass = '.....password
set from='...@gmail.com'
set realname='Alexander Gallego'
set folder = '~/.mail/'
set spoolfile = 'imaps://imap.gmail.com/smf-dev'
set postponed="imaps://imap.gmail.com/[Gmail]/Drafts"
set header_cache = "~/.mutt/cache/headers"
set message_cachedir = "~/.mutt/cache/bodies"
set certificate_file = "~/.mutt/certificates"
set smtp_url = 'smtps://...@gmail.com@smtp.gmail.com:465/'
set smtp_pass = '...password'
set move = no
set imap_keepalive = 900
set editor='emacsclient -nw %s'
set pager='emacsclient -nw %s'

```

# Select the region and save it.

It's rather simple if you use something like vim or emacs.

Simply highlight the region of the patch. copy and paste it and name it file.txt

then just invoke:

```

git am file.txt

```


# Extra: emacs for mutt

```
; Do not cut words
(global-visual-line-mode t)

; open mail-mode when emacs is invoked by mutt
(add-to-list 'auto-mode-alist '("/mutt" . mail-mode))

; wrap email body
(add-hook 'mail-mode-hook 'turn-on-auto-fill)
(add-hook 'mail-mode-hook 'turn-on-filladapt-mode)
```

# References:

[Linux Kernel docs on mail clients](https://www.kernel.org/doc/Documentation/email-clients.txt)


# Reminders:

* Ensure there are docs & tests
* Make sure clang-format is ran on all the cpp buffers.
* Check the log levels
* Make sure the patch was submitted with --sign-off

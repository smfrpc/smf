Contributing to smf (/smɝf/)
======================================

# Sending Patches

`smf` follows a patch submission similar to Linux. 
Send patches to smf-dev, with a 
[DCO](http://elinux.org/Developer_Certificate_Of_Origin) 
signed-off-message. 

Use `git send-email` to send your patch.

## Example:

1. When you commit, use `"-s"` in your git commit command, which adds a DCO
signed off message. DCO is a 
[Developer's Certificate of Origin](http://elinux.org/Developer_Certificate_Of_Origin) 
. For the commit message, you can prefix a tag for an area of the codebase the patch is addressing

```
git commit -s -m "core: some descriptive commit message"
```

2. then send an email to the google group

```
git send-email <revision>..<final_revision> --to smf-dev@googlegroups.com
```

NOTE: for sending replies to patches, use --in-reply-to with the message ID of
the original message. Also, if you are sending out a new version of the change,
use git rebase and then a `git send-email` with a `-v2`, for instance, to
denote that it is a second version.

## Testing and Approval

Run `$ROOT/release` and ensure tests are passing (at least) as well as before the
patch.

 
-------------------


# Maintainers


## How to apply patches from smf-dev@googlegroups.com?

The gist of it is that you need to integrate it with an editor so you can do

```
git am --interactive PATCH_FILE

```

See details `$ROOT/misc/git_merge_patch.sh`



To send an email notification that you've merged it into /dev please use:

```

git send-email HEAD^..HEAD --compose --subject "$subject_line" --to smf-dev@googlegroups.com


```

See details `$ROOT/misc/git_email_merge_confirmation.sh`

## Using mutt

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

## Select the region and save it.

It's rather simple if you use something like vim or emacs.

Simply highlight the region of the patch. copy and paste it and name it  PATCH_FILE
and see steps above for doing a

`git am --interactive ...`


## Extra: emacs for mutt

```
; Do not cut words
(global-visual-line-mode t)

; open mail-mode when emacs is invoked by mutt
(add-to-list 'auto-mode-alist '("/mutt" . mail-mode))

; wrap email body
(add-hook 'mail-mode-hook 'turn-on-auto-fill)
(add-hook 'mail-mode-hook 'turn-on-filladapt-mode)
```

## References:

[Linux Kernel docs on mail clients](https://www.kernel.org/doc/Documentation/email-clients.txt)


## Reminders:

* Ensure there are docs & tests
* Make sure clang-format is ran on all the cpp buffers, via `$ROOT/release`
* Check the log levels
* Make sure the patch was submitted with --sign-off

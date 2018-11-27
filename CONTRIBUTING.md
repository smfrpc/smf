Contributing to smf (/smɝf/)
======================================

# Sending Patches

#### Note on corporate contributions. 
We do not have the bandwith to deal w/ the legalities from corporate contributions. Please 
make all your contributions as an individual with DCO. 
Copy right should be `Copyright (c) 2016 SMF Authors. All rights reserved.` going forward.

## tl;dr

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

Run `$ROOT/tools/build.sh -rt` and ensure tests are passing (at least) as well as before the
patch.



## References:

[Linux Kernel docs on mail clients](https://www.kernel.org/doc/Documentation/email-clients.txt)


## Reminders:

* Ensure there are docs & tests
* Make sure clang-format is ran on all the cpp buffers
* Check the log levels. Use `LOG_INFO` with tender loving care.
* Make sure the patch was submitted with --sign-off




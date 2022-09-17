The bundled .pxd files aren't magic
===================================

This is a really simple point: Cython includes a number of .pxd files for your convenience
exposing chunks of the C standard library, the C++ standard library, the Python C API, and
a few other things.

There's nothing specially about these files. They do nothing that you as a user can't do
yourself. Therefore if there is a feature missing you can write it yourself - there's no
need to wait for it to be implemented in Cython itself (please do submit a PR for it
if you can though!).

Similarly if a feature only exists in the latest version of Cython and you want to use
an earlier version then just copy it from the source on Github and it'll likely work.
(The one exception to this is features that depend on C++ forwarding references, which
are only available on Cython 3+).

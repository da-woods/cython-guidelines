You probably don't need to release the GIL
==========================================

.. Note::

  This article is copied from `a Stack Overflow Q/A that I wrote <https://stackoverflow.com/a/65834961/>`_. It was originally written in response to a lot of questions from people trying
  increasingly contorted ways of trying to release the GIL while still wanting to use Python
  types, mostly for no good reason.

The basic function of the GIL (global interpreter lock) is to ensure that Python's internal mechanisms are not subject to race conditions, by ensuring that only one Python thread is able to run at once. However, simply holding the GIL does not slow your code down.

The two (related) occasions when you should release the GIL are:

#. Using `Cython's parallelism mechanism <https://cython.readthedocs.io/en/latest/src/userguide/parallelism.html>`_. The contents of a ``prange`` loop for example are required to be ``nogil``.

#. If you want other (external) Python threads to be able to run at the same time.

    #. if you have a large computationally/IO-intensive block that doesn't need the GIL then it may be "polite" to release it, just to benefit users of your code who want to do multi-threading. However, this is mostly useful rather than necessary.

    #. (very, very occasionally) it's sometimes useful to briefly release the GIL with a short with ``nogil: pass`` block. This is because Cython doesn't release it spontaneously (unlike Python) so if you're waiting on another Python thread to complete a task, this can avoid deadlocks. This sub-point probably doesn't apply to you unless you're compiling GUI code with Cython.

The sort of Cython code that can run without the GIL (no calls to Python, purely C-level numeric operations) is often the sort of code that runs efficiently. This sometimes gives people the impression that the inverse is true and the trick is releasing the GIL, rather than the actual code they're running. Don't be misled by this - your (single-threaded) code will run the same speed with or without the GIL.

Therefore, if you have a nice fast Numpy function that does exactly what you want quickly on a big chunk of data, but can only be called with the GIL, then just call it - no harm is done!

As a final point: even within a ``nogil`` block (for example a ``prange`` loop) you can always get the GIL back if you need it::

  with gil:
      ... # small block of GIL requiring code goes here

Try not to do this too often (getting/releasing it takes time, and of course only one thread can be running this block at once) but equally it's a good way of doing small Python operations where needed.

Extra corollary
---------------

Not everything benefits from (or ever can!) being put in a ``prange`` loop. Some algorithms are
simply linear (and so a ``prange`` loop will just get the wrong answers). There's also a fixed
overhead from starting a ``prange`` loop which means it's only worthwhile on large problems.
And also that it may be worthwhile writing one extended parallel section rather than multiple
short ``prange`` loops. Finally, bits of Numpy and Scipy are parallelized internalize (usually
the bits that call BLAS), so Cython may end up competing with them for limited CPU cores.

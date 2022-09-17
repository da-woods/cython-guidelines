Themes
======

.. _separate-cy-interface:

Have a separate "Python" and "Cython" interface
-----------------------------------------------

I'm not a big fan of ``cpdef``. Generally a function should either be designed to be used
from Python (use ``def``) or designed to be used only from Cython (perhaps use ``cdef``).
``cpdef`` has all the restrictions of both and is often a sign that you have not planned
your interface correctly.

``cdef public`` attributes on classes (which are turned into properties) are
*often* a sign of the the same problem (although not universally).

If you're using C++ classes it's also worth thinking "do I need to expose these to Python".
Your life will probably be simpler if you don't have to write ``cdef class`` wrappers for
them and potentially work out the complex ownership issues involved.



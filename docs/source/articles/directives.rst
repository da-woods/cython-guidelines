Consider your compiler directives carefully
===========================================

A lot of people produce code that decorates every function with ``cython.boundscheck(False)``,
``cython.wraparound(False)``, and possibly ``cython.cdivision(True)``. I typically
regard this as `cargo-cult programming <https://en.wikipedia.org/wiki/Cargo_cult_programming>`_ 
(i.e. they've copied the code from somewhere without really understanding it).

A subset of the people with this code go on to complain about mystery segmentation faults
due to writing beyond the end of their arrays (which could be diagnosed by turning
``boundscheck`` on). I have fairly little sympathy for those people.

Try to use these directives by knowing what they do:

* ``boundscheck`` means that you won't receive an ``IndexError`` when indexing beyond the
  end of the array. It applies to typed memoryviews, ``np.ndarray`` and also indexing of
  most Python objects. Indexing of C arrays or C++ containers isn't guarded though, so
  ``boundscheck`` makes no difference to them. It is often "fairly" safe since most
  people are indexing based on ``len``::
  
    for idx in range(len(some_list)):
        val = some_list[idx]
        
  In this case you're guaranteed not to go beyond the list end. But you could often 
  just write ``for val in some_list:``.
  
* ``wraparound`` disables use of negative indices to gain a bit of speed.
  They're fairly rarely used so it's often a sensible thing to do.
  
* ``cdivision`` again gains a bit of speed by not correcting for a few differences
  between Python and C integer division, and not raising a ``ZeroDivisionError`` on
  division by zero. Often a sensible optimization but if you aren't dividing any
  numbers then it kind of looks like you're copying blindly.
  
I'm personally like to use these directives as locally as possible, just as
an indication and a reminder to myself that I've thought about whether they're actually
useful in a particular context::

  for idx in range(len(some_list)):
    with cython.boundscheck(False), cython.wraparound(False):
      val = some_list[idx]
      
I realize this is very much a matter of taste.

``cdef`` and ``cpdef`` functions
--------------------------------

Mentioned here because the thought is similar but I plan to write a more detailed
description elsewhere. The *contents* of a ``cdef`` function are no quicker or slower
than the contents of a Cython-defined ``def`` function. Therefore there is usually
no benefit to making all your functions ``cdef`` or ``cpdef``. The *call* to a ``c[p]def``
function is quicker and this can be worthwhile when you have lots of calls to a
small function. Additionally ``cdef`` functions can take C parameters like pointers.

``cpdef`` functions are also often a sign that
:ref:`you have not decided where to separate your Python and Cython interface<separate-cy-interface>`.

I believe a lot of people use ``c[p]def`` by default because they don't understand
what parts it actually speeds up.

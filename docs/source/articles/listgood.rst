.. _list-is-good:

The Python list is surprisingly good
====================================

A fairly common complaint is that Cython provides no way of exposing an efficient
list of a specific ``cdef class``. The answer to this is mainly: a list of one
type of ``cdef class`` would be implemented almost exactly like a Python list!

``list`` has a number of nice properties:

* it resizes efficiently (by overallocation). This does not apply to
  some of the competing options (e.g. ``ndarray`` with ``dtype=object``).
* iteration is pretty efficient - two pointer lookups. This is especially
  true if you specify the variable type as ``list`` but essentially any
  untyped for-loop in Cython has a fast path for ``list``.
* Element access can also be efficient for the same reason. Consider
  turning off ``boundscheck`` if you know you're indexing inside the
  valid length and ``wraparound`` if you know all the indices are
  positive.
* Needs no conversion to/from Python and is absolutely native to Python.

Technical details about why you don't need a special cdef-class-list:
---------------------------------------------------------------------

Consider the ``cdef class`` ``Child``::

    cdef class Child:
       cdef some_cdef_function():
           pass

It wouldn't be possible to have a direct array (allocated in a single chunk) of ``Child``s. Partly because, if somewhere else ever gets a reference to a ``Child`` in the array, that ``Child`` has to be kept alive (but not the whole array) which wouldn't be possible to ensure if they were all allocated in the same chunk of memory. Additionally, were the array to be resized (if this is a requirement) then it would invalidate any other references to the objects within the array.

Therefore you're left with having an array of pointers to ``Child``. Such a structure would be fine, but internally is basically just a Python list (but with a different pointer type). Therefore, 
there's really no benefit to doing something more complicated in Cython...).

What to do if you really want such a typed container
----------------------------------------------------

There are a few sensible workarounds:

#. Just to use a python list. You could also use a numpy array with dtype=object.
   If you need to to access a cdef function in the class you can do a cast first::

    cdef Child c = <Child?>a[0] # omit the ? if you don't want
                                # the overhead of checking the type.
    c.some_cdef_function()

   Internally both these options are stored as an C array of ``PyObject`` pointers to the 
   ``Child`` objects so are pretty efficient and pretty comparable. I recommend typing the
   ``list``.

#. Store your data as a C struct (``cdef struct ChildStruct: ....``) which can be readily stored 
   as an array. When you need a Python interface to that struct you can either define ``Child``
   so that it contains a copy of ``ChildStruct`` (but modifications won't propagate back to your
   original array), or a pointer to ``ChildStruct`` (but you need to be careful with ensuring that
   the memory is not freed which the Child pointing to it is alive). Really this point is
   about questioning the assumption that you need a ``cdef class``.

#. You could use a Numpy structured array - this is pretty similar to using an array of C ``struct`` except Numpy handles the memory, and provides a Python interface.

#. Finally, you can use a memoryview: ``cdef Child[:] array_of_child``. This can be initialized from a numpy array of dtype object::

    array_of_child = np.array([(Child() for i in range(100)])

   In terms of data-structure, this is an array of pointers (i.e. the same as a Python ``list``, but can be multi-dimensional). It avoids the need for ``<Child>`` casting. The important thing it doesn't do is any kind of type-checking - if you feed an object that isn't ``Child`` into the array then Cython won't notice (because the underlying ``dtype`` is ``object``), but will give nonsense answers or segmentation faults.

   In my view this approach gives you a false sense of security about two things: first that you have made a more efficient data structure (you haven't, it's basically the same as a ``list``); second that you have any kind of type safety. However, the ``cdef class``-typed memoryview option does exist. (If you want to use memoryviews, e.g. for multi-dimensional arrays, it would probably be better to use a memoryview of type ``object`` - this is honest about the underlying ``dtype``)

.. note::
    Much of this is reworded slightly from `a Stack Overflow answer that I wrote a few years ago <https://stackoverflow.com/a/33853634>`_. It's not plagiarism because I wrote it! 

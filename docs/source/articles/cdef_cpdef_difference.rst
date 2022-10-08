What's the difference between ``def``, ``cdef``, and ``cpdef`` functions
========================================================================

.. note::

  This article is mostly copied from `a stack overflow answer which I wrote <https://stackoverflow.com/a/41976772/>`_.
  
The key difference is in where the function can be called from: ``def`` functions can be called from Python and Cython while ``cdef`` functions can be called from Cython and C.

Both types of functions can be declared with any mixture of typed and untyped arguments, and in both cases the internals are compiled to C by Cython (and the compiled code should be very, very similar)::

    # A Cython class for illustrative purposes
    cdef class C:
        pass

    def f(int arg1, C arg2, arg3):
        # takes an integer, a "C" and an untyped generic python object
        pass

    cdef g(int arg1, C arg2, arg3):
        pass

In the example above, ``f`` will be visible to Python (once it has imported the Cython module) and ``g`` will not be and cannot be called from Python. ``g`` will translate into a C signature of::

  PyObject* some_name(int, struct __pyx_obj_11name_of_module_C *, PyObject*)

(where ``struct __pyx_obj_11name_of_module_C *`` is just the C struct that our class ``C`` is translated into). This allows it to be passed to C functions as a function pointer for example. In contrast ``f`` cannot (easily) be called from C.

Restrictions on ``cdef`` functions
----------------------------------

``cdef`` functions cannot be defined inside other functions - this is because there is no way of storing any captured variables in a C function pointer. E.g. the following code is illegal::

    # WON'T WORK!
    def g(a):
    cdef (int b):
        return a+b

cdef functions cannot take ``*args``- and ``**kwargs``-type arguments. This is because they cannot easily be translated into a C signature.

Advantages of ``cdef`` functions
--------------------------------

``cdef`` functions can take any type of argument, including those that have no Python equivalent (for example pointers). ``def`` functions cannot have these, since they must be callable from Python.

``cdef`` functions can also specify a return type (if it is not specified then they return a Python object, ``PyObject*`` in C). ``def`` functions always return a Python object, so cannot specify a return type::

    cdef int h(int* a):
        # specify a return type and take a non-Python compatible argument
        return a[0]

(you can, of course, specify a Python type annotation with ``->`` syntax, but Cython does not
make specific use of that.)
        
``cdef`` functions are quicker to call than ``def`` functions because they translate to a simple C function call.

cpdef functions
---------------

``cpdef`` functions cause Cython to generate a ``cdef`` function (that allows a quick function call from Cython) and a ``def`` function (which allows you to call it from Python). Internally the ``def`` function just calls the ``cdef`` function. In terms of the types of arguments allowed, ``cpdef`` functions have all the restrictions of both ``cdef`` and ``def`` functions.

Summary (or when to use a ``cdef`` function?)
---------------------------------------------

Once the function has been called there is no difference in the speed that the code inside a ``cdef`` and a ``def`` function runs at. Therefore, only use a ``cdef`` function if:

- You need to pass non-Python types in or out, or
- You need to pass it to C as a function pointer, or
- You are calling it often (so the sped-up function call is important) and you don't need to call it from Python.

Use a ``cpdef`` function when you are calling it often (so the sped-up function call is important) but you do need to call it from Python. I'm generally not a big fan of ``cpdef`` functions because I think
it should usually be clear whether a function is designed to be used by Cython or by Python. However
if you're writing a library for others to use then this may not always be the case.

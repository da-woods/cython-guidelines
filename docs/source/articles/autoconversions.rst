Beware automatic type conversions
=================================

Cython automatically generates type conversions between certain C/C++ types and Python types.
These are often undesirable.

First we should look at what conversions Cython generates:

* C ``struct`` to/from Python ``dict`` - if all elements of a ``struct`` are themselves 
  convertible to a Python object, then the ``struct`` will be converted to a Python 
  ``dict`` if returned from a function that returns a Python object::
  
    # taken from the Cython documentation
    cdef struct Grail:
        int age
        float volume
       
    def get_grail():
        cdef Grail g
        g.age = 100
        g.volume = 2.5
        return g
        
    print(get_grail())
    # prints something similar to:
    # {'age': 100, 'volume': 2.5}
    
* C++ standard library containers
  `to/from their Python equivalent <https://cython.readthedocs.io/en/latest/src/userguide/wrapping_CPlusPlus.html#standard-library>`_. A common pattern is to use
  a ``def`` function with an argument typed as ``std::vector``. This will be auto-converted
  from a Python list::
  
    from libcpp vector cimport vector
  
    def print_list(vector[int] x):
        for xi in x:
            print(x)

Most of these conversions should work both ways.

They have a couple of non-obvious downsides.

The conversion isn't free
-------------------------

Especially for the C++ container conversions. Consider the ``print_list`` function above. The
function is appealing because iteration over the vector is faster than iteration over a Python
list. However, Cython must iterate over *each element* of your input list, checking that it is
something convertible to a C integer. Therefore, you haven't actually saved yourself any time -
you've just hidden the "expensive" loop in a function signature.

These conversions may be worthwhile if you're doing sufficient work inside your function.
Consider also keeping the type as the C++ type and working on it across multiple Cython
functions (i.e. :ref:`minimize your "Python" interface and keep as much of the work in Cython
as possible<separate-cy-interface>`).

In many cases it might be better to type your function with a 1D typed memoryview (``int[:]``)
and pass in an ``array.array`` or a Numpy array. See also :ref:`list-is-good`.

Changes to not propagate back
-----------------------------

Especially to attributes of ``cdef class``es exposed to Python via properties (including
via ``cdef public`` attributes.

For example::

    from libcpp.vector cimport vector

    cdef class VecHolder:
        def __init__(self, max):
             self.value = list(range(max))  # just fill it for demo purposes
    
        cdef public vector[double] values

then from Python::

    vh = VecHolder(5)
    print(vh.values)
    # Output: [ 0, 1, 2, 3, 4 ]
    
    vh.values[0] = 100
    print(vh.values)
    # Output: [ 0, 1, 2, 3, 4 ]
    
    # However you can re-assign it completely
    vh.values = []
    print(vh.values)
    # Output: []
    
Essentially your Python code modifies the ``list`` that is returned to it an not the underlying
``vector`` used to generate the ``list``. This is sufficiently non-intuitive that I really
recommend against exposing convertible types as attributes!

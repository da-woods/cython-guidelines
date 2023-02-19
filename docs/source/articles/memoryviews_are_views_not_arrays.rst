.. _memviews-are-views:

Memoryviews are views of *existing memory*
==========================================

The clue is in the name!

This is a common source of confusion to new users. Typed memoryviews don't
themselves allocate any memory. They simply provide fast access to memory
that already exists.

Allocation
----------

This means that you must create something for them to view. The most
common thing to use is a Numpy array::

    cdef double[:] x = np.zeros((5,), dtype=np.double)
    
However you can also use any other Python interface that supports the
buffer protocol. Convenient examples are ``array.array``, ``bytearray``
and ``bytes`` (the latter requires a ``const`` memoryview because
you can't alter a bytes object).

A commonly asked question is "what's the fastest way to allocate
memory for a memoryview?". Usually this is a sign that you should
reconsider your design. Allocating memory isn't fast, allocating
a memory for a memoryview always involves create a Python object
(although the exact details may be hidden from you). See if you
can refactor your memory allocation to happen once, outside your
tight inner loop.

If the lifetime of the memory is tightly scoped you can get
reasonable performance by simply pointing the memoryview at a
C/C++ array. The following three examples provide quick demonstrations::

    def fixed_sized_array():
        cdef double[10] arr
        cdef double[::1] mview = <double[:10:1]>arr
        
        # do something ...
        
        # arr ceases to exist at the end of the function... don't
        # use the memoryview afterwards
    
    
    from libc.stdlib cimport malloc, free
    
    def variable_sized_array(int N):
        cdef double[::1] mview
        cdef double *arr = malloc(sizeof(double)*N)
        try:
            mview = <double[:N:1]>arr
            # do something...
        finally:
            free(arr)
            # don't use the memoryview after this!
            
    # Code below requires C++!
    from libcpp.vector cimport vector
    
    def cpp_variably_sized(int N):
        cdef vector[double] vec = vector[double](N)
        cdef double[::1] mview = <double[:N:1]>vec.data()
        
        # do something ...
        
        # again, don't use the memoryview after the end of the function
        
The vector version is nice because it gives you a variable sized array without
the need for manual clean-up of the memory. But practically it's basically
the same as the ``malloc/free`` version.

Be aware in all cases a lightweight ``PyObject`` is created to provide a buffer
protocol interface, so the GIL will be needed.
            
If you really want a quick and dynamically scoped way to allocate memory the
fastest way appears to be to use ``array.array`` by the non-portable internal
interface exposed by ``from cpython cimport array``. Cython's own 
``cython.views.array`` doesn't seem particularly competitive. I don't plan to
detail this hacky method here....

Re-assignment
-------------

As a side effect::

    x = np.ones((5,), dtype=np.double)
    
simply re-targets the view so that it now views a different object. It
doesn't change the modify whatever ``x`` previously viewed. If you
want to copy into ``x`` instead then do::

    x[:] = np.ones((5,), dtype=np.double)
    
As a full example::

    def retarget_view(double[:] x):
        x = np.ones((5,), dtype=np.double)
    
    def change_existing_data(double[:] x):
        x[:] = np.ones((5,), dtype=np.double)
        
    arr1 = np.zeros((5,), dtype=np.double)
    print(arr1[0])  # 0
    
    # calling retarget_view has no external visible side-effects
    retarget_view(arr1)
    print(arr1[0])  # 0
    
    # actually modify the data in arr1
    change_existing_data(arr1)
    print(arr1[0])  # 1

Slicing is quick
----------------

Note that slicing a memoryview to get a "subview" is very quick, doesn't
require the GIL, and doesn't need to allocate Python objects::

    cdef void f(double[:] x):
        ... # do something
        
    def call_f(double[:,:] x2D):
        for i in range(x2D.shape[0]):
            f(x[i, :])  # this is fast!
            
Take advantage of this!
        
Understanding the underlying data structure
-------------------------------------------

Technical details follow - you may want to skip this.

.. Note::

  This section is mostly copied from `a Stack Overflow answer that I wrote <https://stackoverflow.com/a/37497998/4657412>`_.

When you write in a function::

    cdef double[:] a

you end up with a `__Pyx_memviewslice` object::

    typedef struct {
      struct __pyx_memoryview_obj *memview;
      char *data;
      Py_ssize_t shape[8];
      Py_ssize_t strides[8];
      Py_ssize_t suboffsets[8];
    } __Pyx_memviewslice;

The memoryview contains a C pointer some some data which it (usually) doesn't directly own. It also contains a pointer to an underlying Python object (``struct __pyx_memoryview_obj *memview;``). If the data is owned by a Python object then ``memview`` holds a reference to that and ensures the Python object that holds the data is kept alive as long as the memoryview is around.

The combination of the pointer to the raw data, and information of how to index it (``shape``, ``strides`` and ``suboffsets``) allows Cython to do indexing the using the raw data pointers and some simple C maths (which is very efficient). e.g.::

    x=a[0]

gives something like::

    (*((double *) ( /* dim=0 */ (__pyx_v_a.data + __pyx_t_2 * __pyx_v_a.strides[0]) )));

In contrast, if you work with untyped objects and write something like::

    a = np.array([1,2,3]) # note no typedef
    x = x[0]

the indexing is done as::

    __Pyx_GetItemInt(__pyx_v_a, 0, long, 1, __Pyx_PyInt_From_long, 0, 0, 1);

which itself expands to a whole bunch of Python C-api calls (so is slow). Ultimately it calls ``a``'s ``__getitem__`` method.

Comparison with the old ``np.ndarray`` syntax
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There really isn't a huge difference.

If you do something like::

    cdef np.ndarray[np.int32_t, ndim=1] new_arr

it works practically very like a memoryview, with access to raw pointers and the speed should be very similar.

The advantage to using memoryviews is that you can use a wider range of array types with them (such as the `standard library array <https://docs.python.org/3/library/array.html>`_), so you're more flexible about the types your functions can be called with. This fits in with the general Python idea of "duck-typing" - that your code should work with any parameter that behaves the right way (rather than checking the type). 

A second (small) advantage is that you don't need the numpy headers to build your module.

A third (possibly larger) advantage is that memoryviews can be sliced without the GIL while ``cdef np.ndarray`` s can't (`See the Cython docs <http://docs.cython.org/src/userguide/memoryviews.html#comparison-to-the-old-buffer-support>`_)

A slight disadvantage to memoryviews is that they seem to be slightly slower to set up.

Memoryviews vs ``malloc`` ed pointers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You won't get any speed advantage (but neither will you get too much speed loss). Raw pointers
are about the most direct way to access data, but memoryviews are not a huge overhead on top
of that.

The minor advantages accessing the your data via a memoryview rather than a pointer are:

1. You can write functions that can be used either from Python or internally within Cython::

        cpdef do_something_useful(double[:] x):
            # can be called from Python with any array type or from Cython
            # with something that's already a memoryview
            ....

2. You can let Cython handle the freeing of memory for this type of array, which could simplify your life for things that have an unknown lifetime. See http://docs.cython.org/src/userguide/memoryviews.html#cython-arrays and especially ``.callback_free_data``.

3. You can pass your data back to python python code (it'll get the underlying ``__pyx_memoryview_obj`` or something similar). Be very careful of memory management here (i.e. see point 2!).

4. The other thing you can do is handle things like 2D arrays defined as pointer to pointer (e.g. ``double**``). See http://docs.cython.org/src/userguide/memoryviews.html#specifying-more-general-memory-layouts. I generally don't like this type of array, but if you have existing C code that already uses if then you can interface with that (and pass it back to Python so your Python code can also use it).

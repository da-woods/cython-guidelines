.. _dont-return-memoryviews:

Try not to return typed memoryviews
===================================

Typed memoryviews are the recommended way to quickly accessing array data in Cython.
However, they are usually not what the users of your function want::

    import numpy as np

    def my_arange(int start, int stop):
        cdef int[::1] values = np.empty((stop-start), dtype=np.int)
        for i in range(start, stop):
            values[i-start] = i
        return values
        
This will work, but give your users a slightly opaque object that they can index
(although not as efficiently as in Cython), pass to any Cython function that
takes a typed memoryview, and not a whole lot else.

It's reasonably likely your users would rather have the Numpy object that the
memoryview is a view of - this has a far richer interface. There's a few
ways of doing this. One way that's sometimes recommended is::

    return values.obj
    
In most cases, I don't really recommend this. Especially if the memoryview has been
sliced (e.g. ``values = values[2:]``) then ``obj`` will point to the whole underlying
object rather than what you're taking a view of.

I usually recommend::

    return np.asarray(values)
    
This is *fairly* cheap. It doesn't allocate a new block of memory, but instead creates a
Numpy array wrapper that indexes into the view (efficiently).

Another good option for cases like ``my_arange`` where the underlying object is allocated 
within the function itself is to keep an object version in parallel -
remember that the memoryview "views" the memory in the object so a change to one will change
both. That also lets you use the rich Numpy whole-array arithmetic easily, and mix it in
with fast Cython element-by-element access::

    import numpy as np

    def my_arange(int start, int stop):
        valuesO = np.empty((stop-start), dtype=np.int)
        cdef int[::1] values = valuesO
        
        for i in range((stop-start)):
            values[i] = i
            
        # an alternative approach just to illustrate that you can do it
        valuesO += start
        
        return valuesO

Your Python interface need not match what it's wrapping
=======================================================

As a trivial example lets take a C function that multiplies
two matrices are wrap it in Cython (practically you'd just use
Numpy of course...):

.. code-block:: C

    /* calculate A@B, where A in l by m, B is m by n,
    and C already allocated space for the l by n output.
    Returns
     0 on success,
     1 if A contains a NaN,
     2 if B contains a NaN,
     3 if the matrices are too big
    */
    int mat_mul(double *A, int l, int m, double *B, int n, double *C);
    
As an aside, this interface gets even more complicated if it needs to
handle strides or similar.

Fairly frequently people will see this and try to replicate it in the
Python interface they expose via Cython. What is a suitable Python
version of a ``double*`` (to an array)? The really wrong answer involves ctypes, or
hiding it in a ``PyCapsule``. A slightly better answer is that you could
use 1D contiguous typed memoryview to give::

    def py_mat_mul(double[::1] A, int l, int m, double[::1] B, int n, double[::1] C):
        return mat_mul(&A[0], l, m, &B[0], n, &C[0])
        
This is not a very useful wrapping. The first thing to note is that ``l``, ``m`` and ``n``
are pretty redundant here - we know the sizes from the memoryviews. Importantly it
also provides an opportunity for the user to feed it wrong information. The second thing
is that the function is *really* operating on 2D arrays, not 1D arrays. The 1D
representation is just how C treats it (the contiguousness is still important though)::

    def py_mat_mul(double[:,::1] A, double[:, ::1] B, double[:, ::1] C):
        cdef int l = A.shape[0]
        cdef int m = A.shape[1]
        cdef int n = B.shape[1]
        if B.shape[0] != m:
            raise ValueError("B has the wrong first dimension")
        if C.shape[0] != l and C.shape[1] != n:
            raise ValueError("C has the wrong dimensions")
        return mat_mul(&A[0,0], l, m, &B[0,0], n, &C[0])
        
The next problem is that C is really an output - we shouldn't be expecting our Python
users to pass it (we might choose to let them as an optimization of course, similar to
how Numpy uses ``out`` parameters, but I don't do this example). Creating the array
ourself also eliminates a source of user error::

    def py_mat_mul(double[:, ::1] A, double[:, ::1] B):
        cdef int l = A.shape[0]
        cdef int m = A.shape[1]
        cdef int n = B.shape[1]
        if B.shape[0] != m:
            raise ValueError("B has the wrong first dimension")
        C = np.empty((l, n), dtype=np.float64)
        cdef double[:,::1] C_view = C
        
        c_res = mat_mul(&A[0,0], l, m, &B[0,0], n, &C[0])
        return c_res, C
        
I've assumed here that our users will probably want a Numpy array back. This is a
good assumption, but make it match your use-case.

The final issue - a C error code likely doesn't mean much a Python user (and a tuple
of an error code and a matrix is probably error-prone). Raise a Python exception
instead::

    def py_mat_mul(double[:, ::1] A, double[:, ::1] B):
        cdef int l = A.shape[0]
        cdef int m = A.shape[1]
        cdef int n = B.shape[1]
        if B.shape[0] != m:
            raise ValueError("B has the wrong first dimension")
        C = np.empty((l, n), dtype=np.float64)
        cdef double[:,::1] C_view = C
        
        c_res = mat_mul(&A[0,0], l, m, &B[0,0], n, &C[0])
        if c_res == 1:
            raise RuntimeError("A contained a NaN")
        elif c_res == 2:
            raise RuntimeError("B contained a NaN")
        elif c_res == 3:
            raise RuntimeError("matricies were too big")
        elif c_res != 0:
            raise RuntimeError("Unknown error in matrix multiplication")
        return C
        
This at least presents a moderately intuitive Python interface.

``double**`` matrices
---------------------

Some C interfaces take the unhelpful decision to store their matrices
as indirect pointers. This is rarely a good design decision (it makes
much more sense to do what Numpy does and store an underlying 1D matrix
and then calculate a 1D index from a 2D index).

This section therefore has two pieces of advice:

Don't write this yourself
^^^^^^^^^^^^^^^^^^^^^^^^^

If you've decided that memoryviews are too high level for what you want
and that you want to "unleash the raw power of C" and use pointers
directly in Cython, don't
use the ``double**`` representation yourself. (You're also probably
wrong, and memoryviews are probably fine, but that's a side-issue).

How to handle this interface in external code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Although Cython *does* support indirect memoryviews, very few
external libraries can provide matrices that match them (possibly
PIL can, I think?). Therefore the underlying advice still
applies - provide your data as a big underlying 1D matrix.
Then use Cython to create the indirection array of pointers
that C needs::

    from libc.stdlib cimport malloc, free
    
    cdef extern from "something.h":
        double calculate_from_matrix(double **mat, int m, int n)
        
    def py_calculate_from_matrix(double[:,::1] mat):
        cdef double** ptrs = <double**>malloc(sizeof(double*)*mat.shape[0])
        try:
            # Fill in your pointer array
            for i in range(mat.shape[0]):
                ptrs[i] = &mat[i,0]
            return calculate_from_matrix(ptrs, mat.shape[0], mat.shape[1])
        finally:
            free(ptrs)

This is much more satisfactory that the alternative - having to copy *all*
your data, including a separate allocation and deallocation for each slice
of the array.

There's a couple of small enhancements to consider here: first mat need
not actually be contiguous. Instead only the second dimension must
be contiguous. There's no direct way of expressing a memoryview like this
but you could always except a generic 2D memoryview and validate it yourself.
Second, you could avoid the ``malloc`` and ``free`` with a Numpy
array of ``dtype=np.uintp`` (an unsigned int big enough to hold a pointer).
This is likely slightly slower and involves a bit of casting, but it
removes the need for manual memory management.

``vector<double>/vector<vector<double>>`` matrices
--------------------------------------------------

Some C++ code uses ``std::vector`` as a representation of a matrix/array type.
This is a bit of a pain from Cython - you basically have to copy the data
into the vector (since there's no way of making a vector "point" at some
data).

Nested vectors have many of the same disadvantages as ``double**`` matrices.
The only thing that can't go wrong is memory management.

Essentially if you have code like this there isn't a good solution except
copying.

Most proper C++ matrix algebra libraries (e.g. Eigen) provide a way of
wrapping existing data, so you should use that where possible.

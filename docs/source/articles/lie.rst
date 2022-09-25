Don't be afraid to lie to Cython
================================

Including lies by omission!

If you don't use certain information in Cython then it doesn't need to know it.

* This may mean omitting wrapped functions,

* Omitting template parameters of C++ classes/functions - use the "cname"
  feature here to tell Cython to expose it under one name, and generate the 
  specific template under another name::

   cdef extern from "some_header.hpp":
       cdef cppclass IntContainer "Container<int>":
           ...
       IntContainer getIC "getContainer<int>"(int a, int b) 
       
  A common use of this is for wrapping classes with numeric template parameters
  (which Cython doesn't properly support), for example::
  
    cdef extern from "<array>":
        cdef cppclass ArrayDbl4 "std::array<double, 4>":
            int size() noexcept
            double operator[]() noexcept
            # etc
          
  As an aside, another way to approach the same problem is to use the "cname"
  feature to declare numbers as if they're classes - it's hacky but it works::
  
    cdef extern from "<array>" namespace std:
        cdef cppclass array[T, Size]:
            # ..
            
        cdef cppclass Four "4":
            pass
            
    cdef array[double, Four] arr
  
* Omit class hierarchies from C++ code. Who cares what base classes your
  class has if you are not converting it into a pointer or reference to
  those base classes? Note that you can just declare inherited methods in
  the class you're using.
  
  This might be useful if you're trying to wrap something like 
  ``std::ostringstream`` to write to a C++ string. Although it has a
  quite complex inheritance hierarchy you don't need most of it. The
  chances are you only need ``operator<<()`` (and possibly only for a
  few types), and ``str()``.
  
* For complex template functions it's often easier to leave the template
  deduction to C++ rather than let Cython work out the template parameters.
  To do this declare it as a variadic C function::

   from libcpp.vector cimport vector

   cdef extern from "<algorithm>" namespace "std":
       void sort(...)

   cdef example(vector[double]& v):
       sort(v.begin(), v.end())  # your compiler works this out!
       
  The sort is particularly useful if you want to pass it a comparison
  function. Certainly on the clang standard library, trying to
  specify a function pointer as a template argument (like the default
  wrapper that Cython provides does) causes compile errors while letting
  the compile deduce the types itself works well::
  
    cdef bool cmp_gt(double x, double y):
        # bool is cimported from libcpp
        return x > y
      
    sort(v.begin(), v.end(), cmp_gt)
  
  Unfortunately this does not work as well for anything with a return type.
  Still - you don't need to declare *all* the parameters.

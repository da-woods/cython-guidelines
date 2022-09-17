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
  
* Omit class hierarchies from C++ code. Who cares what base classes your
  class has if you are not converting it into a pointer or reference to
  those base classes? Note that you can just declare inherited methods in
  the class you're using.
  
* For complex template functions it's often easier to leave the template
  deduction to C++ rather than let Cython work out the template parameters.
  To do this declare it as a variadic C function::

   from libcpp.vector cimport vector

   cdef extern from "<algorithm>" namespace "std":
       void sort(...)

   cdef example(vector[double]& v):
       sort(v.begin(), v.end())  # your compiler works this out!
  
  Unfortunately this does not work as well for anything with a return type.
  Still - you don't need to declare *all* the parameters.

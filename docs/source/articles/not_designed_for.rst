What Cython is not designed for
===============================

Obfuscation
-----------

Cython was not designed to help obfuscate (hide) your Python code and therefore 
little support is available to people who want to use it for this. If you do 
decide to use it for this it's worth knowing what its weaknesses are:

* Many *Python* operations take place through string look-ups 
  using functions such as ``PyObject_GetAttrString``. These strings will be 
  easily visible in the generated compiled C files. Using ``cdef classes`` and 
  C structs reduces the number of string lookups and may make your code harder 
  to reverse engineer.
  
* Names of ``def`` and ``cpdef`` functions and any class will be available by 
  running ``dir(your_module)``. Classes can be imperfectly hidden with the 
  ``@cython.internal`` decorator. ``def`` functions can be hidden by declaring 
  them ``cdef`` although they are not exactly equal in functionality so this 
  change may have other consequences (to be discussed elsewhere!).
  
* The automatic pickling mechanism can accidentally expose a small amount of 
  information about classes you define. Disable it with the ``auto_pickle`` 
  directive.
  
* Any Python exception that reaches calling code will have a traceback 
  including the names of functions that it has propagated through. It does not 
  contain the actual lines of code, which are only visible when the source file 
  is present.

Creating Executables
--------------------

Cython includes a small tool called ``cython_freeze`` which is slightly 
unfortunate since it gives people that idea that Cython can be used to turn 
their Python application into an executable. Cython is basically unsuitable for 
this for any non-trivial project.

The main problems are:

* You need to link to (and bundle) libpython. This can be partly solved by 
  static linking, but it's still worth remembering that your application does 
  depend on the Python runtime.
  
* Any modules you import (including standard library modules, third party 
  libraries, and also if your program includes  more than one module). These 
  are not built into your executable an so you must bundle them separately. 
  There are tricks to try to bundle them together, but the rapidly fall apart 
  from anything complicated so I don't recommend them and won't detail them 
  here.
  
There are a number of tools designed to build executables from Python scripts. 
I haven't used any of them seriously so do not make a recommendation. However, 
`Nuitka <https://github.com/Nuitka/Nuitka/>`_ is probably closest to what most 
people want from Cython. `Py2exe <https://www.py2exe.org/>`_,
`PyInstaller <http://www.pyinstaller.org/>`_ and 
`cx_Freeze <https://pypi.org/project/cx-Freeze/>`_ are other options.

Wrapping C++ classes
--------------------

This may be a controversial opinion, since Cython is actually pretty good for 
interfacing with C++. However, if *all* you want to do is to wrap C++ classes 
to expose them to Python then I personally think
`PyBind11 <https://github.com/pybind/>`_ is a better tool - it works 
directly in C++ so it has much better visibility of C++ types and 
C++ ownership semantics and so you'll probably find it more natural for this.

Where Cython comes into its own is if you want to use C++ classes from within 
Cython code - deeply intermingling C++ logic and Python logic. I don't know of 
anything else that can do this as naturally so I really do recommend this use 
case. I'll discuss recommendations for this elsewhere is this guide.

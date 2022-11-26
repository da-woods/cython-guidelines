Issues when embedding
=====================

General I :ref:`recommend against using Cython's "embedding" feature to create executables <dont-create-exes>`. For anything except the simplest of programs they aren't self-contained and so
people are usually disappointed that they haven't made something that they can distribute.
With this recommendation re-iterated, here's some of the things that can go wrong.

Not initializing the Python interpreter
---------------------------------------

Cython doesn't compile to "pure stand-alone C code". Instead Cython compiles to a bunch of
Python C API calls that depend on the Python interpreter. Therefore, in your main function
you *must* initialize the Python interpreter with ``Py_Initialize()``. You should do this
as early as possible in your ``main()`` function.

Very occasionally you may get away without it, for exceptionally simple programs. This
is pure luck, and you should not rely on it. There is no "safe subset" of Cython that's
designed to run without the interpreter.

The consequence of not initializing the Python interpreter is likely to be crashes.

You should only initialize the interpreter once - a lot of modules, including most Cython
modules and Numpy don't currently like being imported multiple times. Therefore if you're
doing occasional Python/Cython calculations in a larger program what you *don't do* is::

   void run_calcuation() {
        Py_Initialize();
        // Use Python/Cython code
        Py_Finalize();
   }
   
The chances are you will get mystery unexplained crashes.

Not setting the Python path
---------------------------

If your module imports anything (and possibly even if it doesn't) then it'll need
the Python path set so it knows where to look for modules. Unlikely the standalone
interpreter, embedded Python doesn't set this up automatically.

``PySys_SetPath(...)`` is the easiest way of doing this (just after ``Py_Initialize()`
ideally). You could also use ``PySys_GetObject("path")`` and then append to the
list that it returns.

if you forget to do this you will likely see import errors.

Not importing the Cython module
-------------------------------

Cython doesn't create standalone C code - it creates C code that's designed to be
imported as a Cython module. The "import" function sets up a lot of the basic
infrastructure necessary for you code to run. For example, strings are initialized
at import time, and built-ins like ``print`` are found and stashed within your
Cython module.

Therefore, if you decide to skip the initialization and just go straight to
running your public functions you will likely experience crashes (even for
something as simple as using a string).

InitTab
^^^^^^^

The preferred way to do imports in modern Python (>=3.5) is to use the
`inittab mechanism <https://docs.python.org/3/c-api/import.html#c._inittab>`_
which is detailed in `the Cython documentation <http://docs.cython.org/en/latest/src/userguide/external_C_code.html#public-declarations>`_. This should be done
before ``Py_Initialize()``.

Forcing single-phase
^^^^^^^^^^^^^^^^^^^^

If for some reason you aren't able to add your module to the inittab before
Python is initialized (a common reason is trying to import another Cython
module built into a single shared library) then you can disable
multi-phase initialization by defining ``CYTHON_PEP489_MULTI_PHASE_INIT=0``
for your C compiler (for gcc this would be ``-DCYTHON_PEP489_MULTI_PHASE_INIT=0``
at the command line). If you do this then you can run the module init
function directly (``PyInit_<module_name>`` on Python 3). *This really
isn't the preferred option*.

Working with multi-phase
^^^^^^^^^^^^^^^^^^^^^^^^

It is possible to run the multi-phase initialization manually yourself.
First call your
``PyInit_<module_name>`` function (which'll return a ``PyModuleDef`` object
and then call ``PyModule_FromDefAndSpec`` and ``PyModule_ExecDef``.
The module spec can be created with `importlib <https://docs.python.org/3/library/importlib.html#importlib.machinery.ModuleSpec>`_.

As an example, consider this Cython module (``embed_example.pyx``):

.. literalinclude:: ../../../examples/embed_example/embed_example.pyx

I've printed a module global just to make absolutely sure that the
module can't be used without being imported.

The C file to import it (``main.c``):

.. literalinclude:: ../../../examples/embed_example/main.c
   :language: C
   
and run the following commands to build it:

.. literalinclude:: ../../../examples/embed_example/to_build.txt
   :language: bash

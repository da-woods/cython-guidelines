#include <Python.h>

#include "embed_example.h"

int main() {
    int result = 1;
    PyObject *spec = NULL, *spec_globals = NULL, *mod = NULL;
    Py_Initialize();
    PyObject *maybe_mod = PyInit_embed_example();
    if (!maybe_mod) goto bad;
    if (Py_IS_TYPE(maybe_mod, &PyModuleDef_Type)) {
        // multi-phase init
        spec_globals = PyDict_New();
        if (!spec_globals) goto bad;
        PyObject *res = PyRun_String(
            "import importlib.machinery as im\n"
            // Note that Cython doesn't actually use the loader
            // so it can be None. It'd be better to
            // provide something more useful though.
            "spec = im.ModuleSpec('embed_example', None)\n",
            Py_file_input, spec_globals, spec_globals);
        Py_XDECREF(res); // don't use res whether or not it's set
        if (!res) goto bad;
        spec = PyDict_GetItemString(spec_globals, "spec");
        if (!spec) goto bad;               
        
        mod = PyModule_FromDefAndSpec(
            (PyModuleDef*)maybe_mod,
            spec);
        if (!mod) goto bad;
        int execRes = PyModule_ExecDef(mod, (PyModuleDef*)maybe_mod);
        if (execRes) goto bad;
    } else {
        mod = maybe_mod;
    }
    
    func();
    
    result = 0;
    if (0) {
        bad:
        PyErr_Print();
    }
    // The moduledef isn't an owned reference so doesn't get decref'd
    Py_XDECREF(mod);
    Py_XDECREF(spec);
    Py_XDECREF(spec_globals);
    Py_Finalize();
    return result;    
}

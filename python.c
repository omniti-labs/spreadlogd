#include "Python.h"

void python_startup() {
    Py_SetProgramName("spreadlogd");
    Py_Initialize();
}

void python_shutdown() {
    Py_Exit(0);
}

int python_path(char *path) {
    PyObject *sys_dict, *sys_path;
    sys_dict = PyModule_GetDict(PyImport_Import(PyString_FromString("sys")));
    sys_path = PyDict_GetItemString(sys_dict, "path");
    PyList_Append(sys_path,PyString_FromString(path));
    return 1;
}

int python_import(char *module) {
    PyObject *pModule, *pName;
    pName = PyString_FromString(module);
    pModule = PyImport_Import(pName);
    if(pModule == NULL) {
        fprintf(stderr, "Failed to load \"%s\"\n", module);
        return NULL;
    }
    return 1;
}

int python_log(char *func, char *sender, char *group, char *message) {
    int retval;
    char *ptr;
    PyObject *pFunction, *pFunctionName, *pSender, *pGroup, *pMessage;
    PyObject *pModuleName, *pDict, *pArgs;


    pSender = PyString_FromString(sender);
    pGroup = PyString_FromString(group);
    pMessage = PyString_FromString(message);
    ptr = strrchr(func, '.');
    if(ptr == NULL) {
        fprintf(stderr, "Function call must be <package>.<name>\n");
        return NULL;
    }
    pFunctionName = PyString_FromString(ptr + 1);
    pModuleName = PyString_FromStringAndSize(func, ptr - func);
    pDict = PyModule_GetDict(PyImport_Import(pModuleName));
    Py_DECREF(pModuleName);
    if(pDict == NULL) {
        return NULL;
    }
    pFunction = PyDict_GetItem(pDict, pFunctionName);
    if(pFunction == NULL) {
        fprintf(stderr, "Function not found\n");
        return NULL;
    }
    pArgs = PyTuple_New(2);
    PyTuple_SetItem(pArgs, 0, pSender);
    PyTuple_SetItem(pArgs, 1, pGroup);
    PyTuple_SetItem(pArgs, 2, pMessage);
    PyObjectCallObject(pFunction, pArgs);
    Py_DECREF(pArgs);
    return 1;
}    


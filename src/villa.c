/* Villa module using dictionary interface */
/* Author: Li Guangming */

#include <Python.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <depot.h>
#include <cabin.h>
#include <villa.h>

typedef struct {
    char *dptr;
    int dsize;
} datum;

typedef struct {
    PyObject_HEAD
    VILLA *villa;
    char *prefix;
    int jmode;
} villaobject;

static PyTypeObject VillaType;

#define is_villaobject(v) ((v)->ob_type == &VillaType)
#define check_villaobject_open(v)                                            \
    if ((v)->villa == NULL) {                                                \
        PyErr_SetString(VillaError, "VILLA object has already been closed"); \
        return NULL;                                                         \
    }

typedef int (*vlcurpos)(VILLA *villa);
static PyObject *VillaError;

extern PyTypeObject PyVillaIterKey_Type; /* Forward */
extern PyTypeObject PyVillaIterItem_Type; /* Forward */
extern PyTypeObject PyVillaIterValue_Type; /* Forward */
static PyObject *villaiter_new(villaobject *, PyTypeObject *);

static PyObject *
new_villa_object(char *file, int flags, int size)
{
    villaobject *dp;

    dp = PyObject_New(villaobject, &VillaType);
    dp->jmode = -1;

    if (dp == NULL) {
        return NULL;
    }

    if (!(dp->villa = vlopen(file, flags, VL_CMPLEX))) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        Py_DECREF(dp);
        return NULL;
    }

    return (PyObject *)dp;
}

/* Methods */

void _villa_close(villaobject *self)
{
    if (self->villa) {
        vlclose(self->villa);
        self->villa = NULL;
    }
}

static void
villa_dealloc(villaobject *self)
{
    _villa_close(self);
    PyObject_Del(self);
}

static int
villa_length(villaobject *dp)
{
    if (dp->villa == NULL) {
        PyErr_SetString(VillaError, "VILLA object has already been closed");
        return -1;
    }
    return vlrnum(dp->villa);
}

static PyObject *
villa_subscript(villaobject *dp, register PyObject *key)
{
    datum drec, krec;
    int tmp_size;
    PyObject *ret;

    if (!PyArg_Parse(key, "s#", &krec.dptr, &tmp_size)) {
        PyErr_SetString(PyExc_TypeError,
            "villa mappings have string indices only");
        return NULL;
    }

    krec.dsize = tmp_size;
    check_villaobject_open(dp);
    drec.dptr = vlget(dp->villa, krec.dptr, krec.dsize, &tmp_size);
    drec.dsize = tmp_size;
    if (!drec.dptr) {
        if (dpecode == DP_ENOITEM) {
            PyErr_SetString(PyExc_KeyError,
                PyString_AS_STRING((PyStringObject *)key));
        }
        else {
            PyErr_SetString(VillaError, dperrmsg(dpecode));
        }
        return NULL;
    }

    ret = PyString_FromStringAndSize(drec.dptr, drec.dsize);
    free(drec.dptr);
    return ret;
}

static int
villa_ass_sub(villaobject *dp, PyObject *v, PyObject *w)
{
    datum krec, drec;
    int tmp_size;

    if (!PyArg_Parse(v, "s#", &krec.dptr, &tmp_size)) {
        PyErr_SetString(PyExc_TypeError,
            "villa mappings have string indices only");
        return -1;
    }
    krec.dsize = tmp_size;
    if (dp->villa == NULL) {
        PyErr_SetString(VillaError, "VILLA object has already been closed");
        return -1;
    }
    if (w == NULL) {
        if (vlout(dp->villa, krec.dptr, krec.dsize) == 0) {
            if (dpecode == DP_ENOITEM) {
                PyErr_SetString(PyExc_KeyError,
                    PyString_AS_STRING((PyStringObject *)v));
            }
            else {
                PyErr_SetString(VillaError, dperrmsg(dpecode));
            }
            return -1;
        }
    }
    else {
        if (!PyArg_Parse(w, "s#", &drec.dptr, &tmp_size)) {
            PyErr_SetString(PyExc_TypeError,
                "villa mappings have string elements only");
            return -1;
        }
        drec.dsize = tmp_size;
        if (vlput(dp->villa, krec.dptr, krec.dsize, drec.dptr, drec.dsize, VL_DDUP) == 0) {
            PyErr_SetString(VillaError, dperrmsg(dpecode));
            return -1;
        }
    }
    return 0;
}

static PyMappingMethods villa_as_mapping = {
    (lenfunc)villa_length, /*mp_length*/
    (binaryfunc)villa_subscript, /*mp_subscript*/
    (objobjargproc)villa_ass_sub, /*mp_ass_subscript*/
};

static PyObject *
villa__close(register villaobject *dp, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":close")) {
        return NULL;
    }
    _villa_close(dp);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
villa_keys(register villaobject *dp, PyObject *args)
{
    register PyObject *v, *item;
    datum key;
    int err, tmp_size;

    if (!PyArg_ParseTuple(args, ":keys")) {
        return NULL;
    }

    check_villaobject_open(dp);
    v = PyList_New(0);
    if (v == NULL) {
        return NULL;
    }

    /* Init Iterator */
    if (!vlcurfirst(dp->villa)) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
    }

    /* Scan Iterator */
    while ((key.dptr = vlcurkey(dp->villa, &tmp_size)) != NULL) {
        key.dsize = tmp_size;
        item = PyString_FromStringAndSize(key.dptr, key.dsize);
        free(key.dptr);
        if (item == NULL) {
            Py_DECREF(v);
            return NULL;
        }
        err = PyList_Append(v, item);
        Py_DECREF(item);
        if (err != 0) {
            Py_DECREF(v);
            return NULL;
        }
        if (!vlcurnext(dp->villa)) {
            break;
        }
    }
    return v;
}

static PyObject *
villa_has_key(register villaobject *dp, PyObject *args)
{
    datum key;
    int val;
    int tmp_size;

    if (!PyArg_ParseTuple(args, "s#:has_key", &key.dptr, &tmp_size)) {
        return NULL;
    }

    key.dsize = tmp_size;
    check_villaobject_open(dp);
    val = vlvsiz(dp->villa, key.dptr, key.dsize);
    if (val == -1) {
        if (dpecode == DP_ENOITEM) {
            Py_INCREF(Py_False);
            return Py_False;
        }
        else {
            PyErr_SetString(VillaError, dperrmsg(dpecode));
            return NULL;
        }
    }
    else {
        Py_INCREF(Py_True);
        return Py_True;
    }
}

static PyObject *
villa_get(register villaobject *dp, PyObject *args)
{
    datum key, val;
    PyObject *defvalue = Py_None, *ret;
    int tmp_size;

    if (!PyArg_ParseTuple(args, "s#|O:get", &key.dptr, &tmp_size, &defvalue)) {
        return NULL;
    }

    key.dsize = tmp_size;
    check_villaobject_open(dp);
    val.dptr = vlget(dp->villa, key.dptr, key.dsize, &tmp_size);
    val.dsize = tmp_size;

    if (val.dptr != NULL) {
        ret = PyString_FromStringAndSize(val.dptr, val.dsize);
        free(val.dptr);
    }
    else {
        Py_INCREF(defvalue);
        ret = defvalue;
    }

    return ret;
}

static PyObject *
villa_getlist(register villaobject *dp, PyObject *args)
{
    register PyObject *v, *item;
    datum key, val;
    CBLIST *list;
    int i, err;

    if (!PyArg_ParseTuple(args, "s#:getlist", &key.dptr, &key.dsize)) {
        return NULL;
    }

    check_villaobject_open(dp);
    v = PyList_New(0);
    if (v == NULL) {
        return NULL;
    }

    list = vlgetlist(dp->villa, key.dptr, key.dsize);
    if (!list) {
        return v;
    }

    for (i = 0; i < cblistnum(list); i++) {
        val.dptr = (char *)cblistval(list, i, &val.dsize);
        item = PyString_FromStringAndSize(val.dptr, val.dsize);

        if (item == NULL) {
            Py_DECREF(v);
            cblistclose(list);
            return NULL;
        }

        err = PyList_Append(v, item);
        Py_DECREF(item);
        if (err != 0) {
            Py_DECREF(v);
            cblistclose(list);
            return NULL;
        }
    }

    cblistclose(list);
    return v;
}

static PyObject *
villa_put(register villaobject *dp, PyObject *args)
{
    datum key, val;
    PyObject *value = NULL;
    int tmp_size, mode;

    if (!PyArg_ParseTuple(args, "s#|Si:put",
            &key.dptr, &tmp_size, &value, &mode)) {
        return NULL;
    }

    key.dsize = tmp_size;
    check_villaobject_open(dp);

    if (value == NULL) {
        value = PyString_FromStringAndSize(NULL, 0);
        if (value == NULL) {
            return NULL;
        }
    }
    else {
        Py_INCREF(value);
    }

    val.dptr = PyString_AS_STRING(value);
    val.dsize = PyString_GET_SIZE(value);
    if (!vlput(dp->villa, key.dptr, key.dsize, val.dptr, val.dsize, mode)) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        return NULL;
    }

    return value;
}

static PyObject *
villa_iterprefix(register villaobject *dp, PyObject *args)
{
    datum prefix;
    int tmp_size, mode;

    if (!PyArg_ParseTuple(args, "s#|i:iterprefix",
            &prefix.dptr, &tmp_size, &mode)) {
        return NULL;
    }

    prefix.dsize = tmp_size;
    check_villaobject_open(dp);
    dp->jmode = mode;

    if (!vlcurjump(dp->villa, prefix.dptr, prefix.dsize, mode)) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        return NULL;
    }

    return villaiter_new(dp, &PyVillaIterItem_Type);
}

static PyObject *
villa_trunprefix(register villaobject *dp, PyObject *args)
{
    datum prefix;
    char *key;
    int tmp_size, mode;

    if (!PyArg_ParseTuple(args, "s#|i:trunprefix",
            &prefix.dptr, &tmp_size, &mode)) {
        return NULL;
    }

    prefix.dsize = tmp_size;
    check_villaobject_open(dp);

    if (!vlcurjump(dp->villa, prefix.dptr, prefix.dsize, mode)) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        Py_RETURN_FALSE;
    }

    while (vlcurout(dp->villa)) {
        key = vlcurkey(dp->villa, NULL);
        if (key){
            if (strncmp(prefix.dptr, key, prefix.dsize)) {
                free(key);
                break;
            }
            free(key);
        }
    }

    Py_RETURN_TRUE;
}

static PyObject *
villa_iterkeys(villaobject *dp)
{
    return villaiter_new(dp, &PyVillaIterKey_Type);
}

static PyObject *
villa_iteritems(villaobject *dp)
{
    return villaiter_new(dp, &PyVillaIterItem_Type);
}

static PyObject *
villa_itervalues(villaobject *dp)
{
    return villaiter_new(dp, &PyVillaIterValue_Type);
}

static PyMethodDef villa_methods[] = {
    { "close", (PyCFunction)villa__close, METH_VARARGS,
        "close()\nClose the database." },
    { "keys", (PyCFunction)villa_keys, METH_VARARGS,
        "keys() -> list\nReturn a list of all keys in the database." },
    { "has_key", (PyCFunction)villa_has_key, METH_VARARGS,
        "has_key(key} -> boolean\nReturn true if key is in the database." },
    { "put", (PyCFunction)villa_put, METH_VARARGS,
        "put(key, value, mode) -> value\n"
        "Return the value for key if present, otherwie null" },
    { "iterprefix", (PyCFunction)villa_iterprefix, METH_VARARGS,
        "D.iterprefix(prefix, mode) -> an iterator over the (key, value) items of D" },
    { "trunprefix", (PyCFunction)villa_trunprefix, METH_VARARGS,
        "trunprefix(prefix) -> remove all values key has prefix" },
    { "getlist", (PyCFunction)villa_getlist, METH_VARARGS,
        "getlist(key) -> list\n"
        "Return the list for key" },
    { "get", (PyCFunction)villa_get, METH_VARARGS,
        "get(key[, default]) -> value\n"
        "Return the value for key if present, otherwise default." },
    { "iterkeys", (PyCFunction)villa_iterkeys, METH_NOARGS,
        "D.iterkeys() -> an iterator over the keys of D" },
    { "iteritems", (PyCFunction)villa_iteritems, METH_NOARGS,
        "D.iteritems() -> an iterator over the (key, value) items of D" },
    { "itervalues", (PyCFunction)villa_itervalues, METH_NOARGS,
        "D.itervalues() -> an iterator over the values of D" },
    { NULL, NULL } /* sentinel */
};

static PyObject *
villa_getattr(villaobject *dp, char *name)
{
    return Py_FindMethod(villa_methods, (PyObject *)dp, name);
}

static PyTypeObject VillaType = {
    PyObject_HEAD_INIT(NULL)0,
    "villa.villa",
    sizeof(villaobject),
    0,
    (destructor)villa_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)villa_getattr, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_compare*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    &villa_as_mapping, /*tp_as_mapping*/
};

/* ----------------------------------------------------------------- */
/* Villa iterator                                                    */
/* ----------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
        villaobject *villa; /* Set to NULL when iterator is exhausted */
    PyObject *di_result; /* reusable result tuple for iteritems */
} villaiterobject;

static PyObject *
villaiter_new(villaobject *dp, PyTypeObject *itertype)
{
    villaiterobject *di;
    di = PyObject_New(villaiterobject, itertype);

    if (di == NULL) {
        return NULL;
    }

    Py_INCREF(dp);
    di->villa = dp;

    if (dp->jmode == -1) {
        vlcurfirst(dp->villa);
    }

    if (itertype == &PyVillaIterItem_Type) {
        di->di_result = PyTuple_Pack(2, Py_None, Py_None);
        if (di->di_result == NULL) {
            Py_DECREF(di);
            return NULL;
        }
    }
    else {
        di->di_result = NULL;
    }

    return (PyObject *)di;
}

static void
villaiter_dealloc(villaiterobject *di)
{
    Py_XDECREF(di->villa);
    Py_XDECREF(di->di_result);
    PyObject_Del(di);
}

static PySequenceMethods villaiter_as_sequence = {
    0, /* sq_concat */
};

static PyObject *villaiter_iternextkey(villaiterobject *di)
{
    datum key;
    villaobject *d = di->villa;
    int tmp_size;
    PyObject *ret;

    if (d == NULL) {
        return NULL;
    }

    assert(is_villaobject(d));
    if (vlcurnext(d->villa)) {
        key.dptr = vlcurkey(d->villa, &tmp_size);
    }
    else {
        if (dpecode != DP_ENOITEM) {
            PyErr_SetString(VillaError, dperrmsg(dpecode));
        }
        Py_DECREF(d);
        di->villa = NULL;
        return NULL;
    }

    key.dsize = tmp_size;

    ret = PyString_FromStringAndSize(key.dptr, key.dsize);
    free(key.dptr);

    return ret;
}

static PyObject *villaiter_iternextitem(villaiterobject *di)
{
    datum key, val;
    PyObject *pykey, *pyval, *result = di->di_result;
    int tmp_size;
    villaobject *d = di->villa;

    if (d == NULL) {
        return NULL;
    }

    assert(is_villaobject(d));
    if (d->jmode == VL_JFORWARD){
        key.dptr = vlcurkey(d->villa, &tmp_size);
        if (key.dptr) {
            vlcurnext(d->villa);
        }
        else {
            if (dpecode != DP_ENOITEM) {
                PyErr_SetString(VillaError, dperrmsg(dpecode));
            }
            goto fail;
        }
    }
    else{
        if (vlcurnext(d->villa)) {
            key.dptr = vlcurkey(d->villa, &tmp_size);
        }
        else {
            if (dpecode != DP_ENOITEM) {
                PyErr_SetString(VillaError, dperrmsg(dpecode));
            }
            goto fail;
        }
    }

    key.dsize = tmp_size;
    pykey = PyString_FromStringAndSize(key.dptr, key.dsize);

    if (!(val.dptr = vlget(d->villa, key.dptr, key.dsize, &tmp_size))) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        free(key.dptr);
        Py_DECREF(pykey);
        goto fail;
    }
    val.dsize = tmp_size;
    pyval = PyString_FromStringAndSize(val.dptr, val.dsize);
    free(key.dptr);
    free(val.dptr);

    if (result->ob_refcnt == 1) {
        Py_INCREF(result);
        Py_DECREF(PyTuple_GET_ITEM(result, 0));
        Py_DECREF(PyTuple_GET_ITEM(result, 1));
    }
    else {
        result = PyTuple_New(2);
        if (result == NULL) {
            return NULL;
        }
    }

    PyTuple_SET_ITEM(result, 0, pykey);
    PyTuple_SET_ITEM(result, 1, pyval);
    return result;

fail:
    Py_DECREF(d);
    di->villa = NULL;
    return NULL;
}

static PyObject *villaiter_iternextvalue(villaiterobject *di)
{
    datum key, val;
    PyObject *pyval;
    int tmp_size;
    villaobject *d = di->villa;

    if (d == NULL) {
        return NULL;
    }
    assert(is_villaobject(d));

    if (vlcurnext(d->villa)) {
        key.dptr = vlcurkey(d->villa, &tmp_size);
    }
    else {
        if (dpecode != DP_ENOITEM) {
            PyErr_SetString(VillaError, dperrmsg(dpecode));
        }
        goto fail;
    }
    key.dsize = tmp_size;

    if (!(val.dptr = vlget(d->villa, key.dptr, key.dsize, &tmp_size))) {
        PyErr_SetString(VillaError, dperrmsg(dpecode));
        free(key.dptr);
        goto fail;
    }
    val.dsize = tmp_size;
    pyval = PyString_FromStringAndSize(val.dptr, val.dsize);
    free(key.dptr);
    free(val.dptr);

    return pyval;

fail:
    Py_DECREF(d);
    di->villa = NULL;
    return NULL;
}

PyTypeObject PyVillaIterKey_Type = {
    PyObject_HEAD_INIT(&PyType_Type)0, /* ob_size */
    "villay-keyiterator", /* tp_name */
    sizeof(villaiterobject), /* tp_basicsize */
    0, /* tp_itemsize */
    /* methods */
    (destructor)villaiter_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    0, /* tp_repr */
    0, /* tp_as_number */
    &villaiter_as_sequence, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    PyObject_GenericGetAttr, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    0, /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    PyObject_SelfIter, /* tp_iter */
    (iternextfunc)villaiter_iternextkey, /* tp_iternext */
};

PyTypeObject PyVillaIterItem_Type = {
    PyObject_HEAD_INIT(&PyType_Type)0, /* ob_size */
    "villa-itemiterator", /* tp_name */
    sizeof(villaiterobject), /* tp_basicsize */
    0, /* tp_itemsize */
    /* methods */
    (destructor)villaiter_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    0, /* tp_repr */
    0, /* tp_as_number */
    &villaiter_as_sequence, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    PyObject_GenericGetAttr, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    0, /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    PyObject_SelfIter, /* tp_iter */
    (iternextfunc)villaiter_iternextitem, /* tp_iternext */
};

PyTypeObject PyVillaIterValue_Type = {
    PyObject_HEAD_INIT(&PyType_Type)0, /* ob_size */
    "villa-valueiterator", /* tp_name */
    sizeof(villaiterobject), /* tp_basicsize */
    0, /* tp_itemsize */
    /* methods */
    (destructor)villaiter_dealloc, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_compare */
    0, /* tp_repr */
    0, /* tp_as_number */
    &villaiter_as_sequence, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash */
    0, /* tp_call */
    0, /* tp_str */
    PyObject_GenericGetAttr, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    0, /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    PyObject_SelfIter, /* tp_iter */
    (iternextfunc)villaiter_iternextvalue, /* tp_iternext */
};

/* ----------------------------------------------------------------- */
/* villa module                                                      */
/* ----------------------------------------------------------------- */

static PyObject *
villaopen(PyObject *self, PyObject *args)
{
    char *name;
    char *flags = "r";
    int size = -1;
    int iflags;

    if (!PyArg_ParseTuple(args, "s|si:open", &name, &flags, &size)) {
        return NULL;
    }

    switch (flags[0]) {
    case 'r':
        iflags = VL_OREADER;
        break;
    case 'w':
        iflags = VL_OWRITER;
        break;
    case 'c':
        iflags = VL_OWRITER | VL_OCREAT;
        break;
    case 'n':
        iflags = VL_OWRITER | VL_OCREAT | VL_OTRUNC;
        break;
    default:
        PyErr_SetString(VillaError,
            "arg 2 to open should be 'r', 'w', 'c', or 'n'");
        return NULL;
    }
    return new_villa_object(name, iflags, size);
}

static PyMethodDef villamodule_methods[] = {
    { "open", (PyCFunction)villaopen, METH_VARARGS,
        "open(path[, flag[, size]]) -> mapping\n"
        "Return a database object." },
    { 0, 0 },
};

PyMODINIT_FUNC
initvilla(void)
{
    PyObject *m, *d, *s;

    VillaType.ob_type = &PyType_Type;

    m = Py_InitModule("villa", villamodule_methods);
    if (m == NULL) {
        return;
    }

    d = PyModule_GetDict(m);
    if (VillaError == NULL) {
        VillaError = PyErr_NewException("villa.error", NULL, NULL);
    }

    PyModule_AddIntMacro(m, VL_DOVER);
    PyModule_AddIntMacro(m, VL_DKEEP);
    PyModule_AddIntMacro(m, VL_DCAT);
    PyModule_AddIntMacro(m, VL_DDUP);
    PyModule_AddIntMacro(m, VL_DDUPR);
    PyModule_AddIntMacro(m, VL_JFORWARD);
    PyModule_AddIntMacro(m, VL_JBACKWARD);

    s = PyString_FromString("villa");
    if (s != NULL) {
        PyDict_SetItemString(d, "library", s);
        Py_DECREF(s);
    }

    if (VillaError != NULL) {
        PyDict_SetItemString(d, "error", VillaError);
    }
}

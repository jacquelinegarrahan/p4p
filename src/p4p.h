#ifndef P4P2_H
#define P4P2_H

#include <stdexcept>
#include <sstream>

#include <epicsMutex.h>
#include <epicsGuard.h>

#include <pvxs/data.h>
#include <pvxs/server.h>
#include <pvxs/source.h>
#include <pvxs/sharedpv.h>
#include <pvxs/client.h>

#ifdef READONLY
// don't want def from shareLib.h
#  undef READONLY
#endif

#include <Python.h>

#if PY_MAJOR_VERSION < 3
// quiet some warnings about implict const char* -> char* cast
// for API functions.  These are corrected in py >= 3.x
#ifndef PyObject_CallFunction
#  define PyObject_CallFunction(O, FMT, ...) PyObject_CallFunction(O, (char*)(FMT), ##__VA_ARGS__)
#  define PyObject_CallMethod(O, METH, FMT, ...) PyObject_CallMethod(O, (char*)(METH), (char*)(FMT), ##__VA_ARGS__)
#endif
#endif

namespace p4p {

using namespace pvxs;

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

struct SB {
    std::ostringstream strm;
    operator std::string() { return strm.str(); }
    template<typename T>
    SB& operator<<(const T& v) {
        strm<<v;
        return *this;
    }
};

struct PyRef {
    PyObject *obj = nullptr;

    PyRef() = default;
    PyRef(const PyRef& o)
        :obj(o.obj)
    {
        Py_XINCREF(o.obj);
    }
    PyRef& operator=(const PyRef& o)
    {
        if(this!=&o) {
            Py_XINCREF(o.obj);
            Py_XDECREF(obj);
            obj = o.obj;
        }
        return *this;
    }

    PyRef(PyRef&& o) noexcept
        :obj(o.obj)
    {
        o.obj = nullptr;
    }
    PyRef& operator=(PyRef&& o) noexcept {
        Py_XDECREF(obj);
        obj = o.obj;
        o.obj = nullptr;
        return *this;
    }

    ~PyRef() {
        Py_CLEAR(obj);
    }

    explicit PyRef(PyObject* o)
        :obj(o)
    {
        if(!obj)
            throw std::logic_error("Alloc failed");
    }

    static
    PyRef allownull(PyObject *o) {
        PyRef ret;
        ret.obj = o;
        return ret;
    }

    static
    PyRef borrow(PyObject* o) {
        PyRef ret(o);
        Py_INCREF(o);
        return ret;
    }

    static
    PyRef iternext(const PyRef& iter) {
        auto item = PyIter_Next(iter.obj);
        auto ret(PyRef::allownull(item));
        if(!item && PyErr_Occurred())
            throw std::runtime_error("XXX"); // exception already set
        return ret;
    }

    PyObject* release() {
        auto ret = obj;
        obj = nullptr;
        return ret;
    }

    void swap(PyRef& o) {
        std::swap(obj, o.obj);
    }

    void reset(PyObject *o) {
        (*this) = PyRef(o);
    }

    constexpr explicit operator bool() const { return obj; }
    constexpr explicit operator PyObject*() const { return obj; }
};

// release GIL
struct PyUnlock
{
    PyThreadState *state;
    PyUnlock() :state(PyEval_SaveThread()) {}
    ~PyUnlock() { PyEval_RestoreThread(state); }
};

// acquire GIL
struct PyLock
{
    PyGILState_STATE state;
    PyLock() :state(PyGILState_Ensure()) {}
    ~PyLock() { PyGILState_Release(state); }
};

TypeDef startPrototype(const std::string& id, const Value& base);
void appendPrototype(TypeDef& def, PyObject* spec);
PyObject* asPySpec(const Value& v, bool fakearray=false);

int except_map();
PyObject* asPy(const Value& v, bool unpackstruct, bool unpackrecurse, PyObject *wrapper);
void storePy(Value& v, PyObject* py);
PyObject* tostr(const Value& v, size_t limit=0, bool showval=true);

/******* Server *******/

std::string toString(const server::Server& serv, int detail=0);
void attachHandler(server::SharedPV& pv, PyObject *handler);
void detachHandler(server::SharedPV& pv);

std::shared_ptr<server::Source> createDynamic(PyObject* handler);
void disconnectDynamic(const std::shared_ptr<server::Source>& src);

/******* client *******/

std::function<void (client::Result &&)> opHandler(PyObject *handler);
template<typename Builder>
void opHandler(Builder& builder, PyObject *handler) {
    builder.result(opHandler(handler));
}
std::function<Value (Value &&)> opBuilder(PyObject *handler);
template<typename Builder>
void opBuilder(Builder& builder, PyObject *handler) {
    builder.build(opBuilder(handler));
}
void opEvent(client::MonitorBuilder& builder, PyObject *handler);

PyObject* monPop(const std::shared_ptr<client::Subscription>& mon);

} // namespace p4p

#endif // P4P2_H

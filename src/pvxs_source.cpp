#include <sstream>

#include "p4p.h"
#include <_p4p.h>

namespace p4p {

namespace {

struct DynamicSource : public server::Source
{
    PyObject* handler;

    mutable epicsMutex lock;

    explicit DynamicSource(PyObject* handler)
        :handler(handler)
    {}

    // Source interface
public:
    virtual void onSearch(Search &op) override final
    {
        for(auto& chan : op) {
            // test neg cache

            PyLock L;

            if(!handler)
                break;

            auto ret(PyRef::allownull(PyObject_CallMethod(handler, "testChannel", "s", chan.name())));
            if(!ret.obj) {
                PySys_WriteStderr("Unhandled Exception %s:%d", __FILE__, __LINE__);
                PyErr_Print();
                PyErr_Clear();

            } else if(PyObject_IsTrue(ret.obj)) {
                chan.claim();
                continue;

            } else if(PyBytes_Check(ret.obj) && strcmp(PyBytes_AsString(ret.obj), "nocache")==0) {
                continue;
            }

            // add to neg cache
        }
    }
    virtual void onCreate(std::unique_ptr<server::ChannelControl> &&op) override final
    {
        PyLock L;

        auto ret(PyRef::allownull(PyObject_CallMethod(handler, "makeChannel", "ss",
                                                      op->name().c_str(),
                                                      op->peerName().c_str())));
        if(!ret.obj) {
            PySys_WriteStderr("Unhandled Exception %s:%d", __FILE__, __LINE__);
            PyErr_Print();
            PyErr_Clear();

        } else if(auto pv = SharedPV_unwrap(ret.obj)) {
            pv.attach(std::move(op));

        } else {
            PyErr_Format(PyExc_TypeError, "makeChannel(\"%s\") must return SharedPV, not %s",
                         op->name().c_str(),
                         Py_TYPE(ret.obj)->tp_name);
            PyErr_Print();
            PyErr_Clear();
        }
    }
};

} // namespace

std::shared_ptr<server::Source> createDynamic(PyObject* handler)
{
    return std::make_shared<DynamicSource>(handler);
}

void disconnectDynamic(const std::shared_ptr<server::Source>& src)
{
    if(!src)
        return;

    auto dynsrc = dynamic_cast<DynamicSource*>(src.get());

    dynsrc->handler = nullptr;
}

}

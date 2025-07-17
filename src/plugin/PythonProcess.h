#ifndef CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#define CNOID_JUPYTER_PLUGIN_PYTHON_PROCESS_H
#include <QObject>
#include <cnoid/OptionManager> //option
#include <sstream>

#include "JupyterPlugin.h"
#include <xeus/xmessage.hpp>

namespace cnoid {

class PythonProcess : public QObject
{
    Q_OBJECT;
public:
    PythonProcess(JupyterPlugin *_self) : self(_self)
    { self = _self; }
    std::string connection_file;

#ifdef USE_OLD_OPTION
    void onSigOptionsParsed(boost::program_options::variables_map& variables);
#else
    void onSigOptionsParsed(OptionManager *_om);
#endif
    bool initialize();
    bool finalize();

    using hoge = std::vector<xeus::xmessage>;
public Q_SLOTS:
    void procRequest(hoge &msg);
Q_SIGNALS:
    void sendRequest(hoge &msg);

private:
    JupyterPlugin *self;
    bool setupPython();
    bool start();
    void kernelThread();

    class Impl;
    Impl *impl;
};

}

#endif

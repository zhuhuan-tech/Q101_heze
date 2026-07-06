
/*
 * Copyright (c) 2023-2024 MLOptic Corp. All rights reserved.
 * This document is property of MLOptic Corp.
 * It is considered confidential and proprietary.
 *
 * Project:ML_AddinManger
 * Author:jason.huang
 * Date:2023.10.26
 * Version:0.0.1
 * Class description:
 * ML_AddinManger is a plugin manager primarily responsible for scanning,
 * loading, and uninstalling plugins.We can create a singleton object through
 * the ML_AddinManger class and manage all plugins through this object, such as
 * creating, closing, and accessing functions in plugins, etc.
 */

#pragma once
#include <qobject.h>
#include <map>
#include <string>
#include <list>
// #include "ml_addinmanger_global.h"
#include "ML_addInInterface.h"

class ADDINMANGER_API ML_AddinManger : public QObject {
    Q_OBJECT

 public:
    ML_AddinManger(QObject* parent = nullptr);

 public:
    // Create a singleton for Manger
    static ML_AddinManger& getinstance();

    // Retrieve available plugins in the given directory
    pluginException loadPluginPath(std::string pluginPath);

    // return list of all dataIO plugins
    std::list<QObject*> getDataIOList(void);

    // return list of actuator plugins
    std::list<QObject*> getActList(void);

    // return list of algorithm plugins
    std::list<QObject*> getAlgList(void);

    // return list of filterWheel plugins
    std::list<QObject*> getFilterWheelList(void);

    // return list of RxFilterWheel plugins
    std::list<QObject*> getRxFilterWheelList(void);
   
    // return list of reflector plugins
    std::list<QObject*> getReflectorList(void);

    // return list of spectrometer plugins
    std::list<QObject*> getSpectrometerList(void);

    // return list of specbos plugins
    std::list<QObject*> getSpecbosList(void);

    /*
     *	get plugin pointer
     *	pluginType includes (typeDataIO, typeActuator, typeAlgo, typeGrabber,
     *typeADDA, typeRawIO). You can obtain the corresponding plugin pointer
     *based on the plugin type and plugin name or plugin ID.
     */
    ML_addInInterfaceBase* getPluginPtr(pluginType type, pluginName pluginName);
    ML_addInInterfaceBase* getPluginPtr(pluginType type, int pluginID);

    // get ID in plugin list based on plugin pointer
    int getPluginIDByPtr(void* pluginPtr);

    // get plugin info(type, version, author, description, license, name)
    PluginInfo getPluginInfo(pluginType type, pluginName pluginName);
    PluginInfo getPluginInfo(pluginType type, int pluginID);

    /**
     *   Create plugin instance
     *   Retrieve the plugin list based on the plugin name,
     *   plugin id, and plugin pointer, and create a plugin
     *   instance through pluginInstanceName. If the pluginInstanceName
     *   string is empty, the name of the plugin instance is consistent
     *   with the plugin name.
     **/
    ML_addInBase* newPluginInstance(pluginType type,
                                    pluginName pluginName,
                                    std::string pluginInstanceName = "");
    ML_addInBase* newPluginInstance(pluginType type,
                                    int pluginID,
                                    std::string pluginInstanceName = "");
    ML_addInBase* newPluginInstance(void* pluginPtr,
                                    std::string pluginInstanceName = "");

    /*
     *	Get plugin instance
     *	Because multiple instances of the plugin have the same name,
     *	if the plugin instance passes in a name, it will receive an
     *	instance of type map. Meanwhile, if the plugin instance name
     *	is not assigned a value, the plugin instance name will default
     *	to the plugin name.
     *   Obtain a pointer to a unique plugin instance based on its ID.
     */
    std::map<int, ML_addInBase*> getPluginInstance(
        pluginType type,
        pluginName pluginName,
        std::string pluginInstanceName = "");
    ML_addInBase* getPluginInstance(pluginType type,
                                    pluginName pluginName,
                                    int pluginInstanceID);

    // close plugin instance
    pluginException closePluginInstance(pluginType type,
                                        pluginName pluginName,
                                        std::string pluginInstanceName);
    pluginException closePluginInstance(pluginType type,
                                        pluginName pluginName,
                                        int pluginInstanceID);
    pluginException closePluginInstance(pluginType type,
                                        int pluginID,
                                        std::string pluginInstanceName);
    pluginException closePluginInstance(pluginType type,
                                        int pluginID,
                                        int pluginInstanceID);

    // close all plugin instances
    pluginException closeAllPluginInstances();

 private:
    std::list<QObject*> m_addInListDataIO;
    std::list<QObject*> m_addInListAct;
    std::list<QObject*> m_addInListAlgo;
    std::list<QObject*> m_addInListFilterWheel;
    std::list<QObject*> m_addInListRxFilterWheel;
    std::list<QObject*> m_addInListReflector;
    std::list<QObject*> m_addInListSpectrometer;
    std::list<QObject*> m_addInListSpecbos;
};

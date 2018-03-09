//******************************************************************
//
// Copyright 2017 Open Connectivity Foundation
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#include <signal.h>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <functional>
#include <string>
#include <iostream>
#include <memory>
#include <exception>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "ocstack.h"
#include "OCPlatform.h"
#include "OCApi.h"
#include "ocpayload.h"

using namespace OC;
namespace PH = std::placeholders;

/*
 tool_version          : 20171123
 input_file            : ../device_test/out_codegeneration_merged.swagger.json
 version of input_file : v1.1.0-20160519
 title of input_file   : Binary Switch
*/

#define INTERFACE_KEY "if"

/*
* default class, so that we have to define less variables/functions.
*/
class Resource
{
    protected:
    OCResourceHandle m_resourceHandle;
    OC::OCRepresentation m_rep;
    virtual OCEntityHandlerResult entityHandler(std::shared_ptr<OC::OCResourceRequest> request)=0;
};


/*
 * class definition for class that handles /mnt
 *
 * The resource through which a Device is maintained and can be used for diagnostic purposes.
 * fr (Factory Reset) is a boolean.
 *   The value 0 means No action (Default), the value 1 means Start Factory Reset
 * After factory reset, this value shall be changed back to the default value
 * rb (Reboot) is a boolean.
 *   The value 0 means No action (Default), the value 1 means Start Reboot
 * After Reboot, this value shall be changed back to the default value
 * Retrieve the maintenance action status

*/
class MntResource : public Resource
{
    public:
        /*
         * constructor
         */
        MntResource();

        /*
         * destructor
         */
         virtual ~MntResource();
    private:

        /*
         * function to parse the payload for the update function (e.g. POST) /mnt
         * Set the maintenance action(s)

         * @param queries  the query parameters for this call
         * @param rep  the response to get the property values from
         * @return OCEntityHandlerResult ok or not ok indication
         */
        OCEntityHandlerResult post(OC::QueryParamsMap queries, const OC::OCRepresentation& rep);

        /*
         * function to make the payload for the retrieve function (e.g. GET) /mnt
         * The resource through which a Device is maintained and can be used for diagnostic purposes.
         * fr (Factory Reset) is a boolean.
         *   The value 0 means No action (Default), the value 1 means Start Factory Reset
         * After factory reset, this value shall be changed back to the default value
         * rb (Reboot) is a boolean.
         *   The value 0 means No action (Default), the value 1 means Start Reboot
         * After Reboot, this value shall be changed back to the default value
         * Retrieve the maintenance action status

         * @param queries  the query parameters for this call
         */
        OCRepresentation get(OC::QueryParamsMap queries);

        
        std::atomic<bool> m_notify_thread_active;       // signal if the thread should run 
        std::thread* m_notify_thread;                   // the thread running in the background to update the observe listeners
        std::atomic<bool> m_send_notification_flag;     // signal if the thread should update all registered listeners
        std::mutex m_cv_mutex;                          // mutex to lock the sending of the notification
        std::condition_variable m_cv;

        /*
         * Function responsible for notifying all observers when a property
         * on the resource has been changed.
         * this function is running in a thread. Send a notification the
         * m_send_notification_flag must be set to true and then m_cv.notify_all()
         * must be called. This should automatically be handled by the code when
         * PUT or POST is sent to the entityHandler.
         */
        void notifyObservers(void);

        // resource types and interfaces as array..
        std::string m_RESOURCE_TYPE[1] = {"oic.wk.mnt"}; // rt value (as an array)
        std::string m_RESOURCE_INTERFACE[2] = {"oic.if.baseline","oic.if.rw"}; // interface if (as an array)
        std::string m_IF_UPDATE[3] = {"oic.if.a", "oic.if.rw", "oic.if.baseline"}; // updateble interfaces
        int m_nr_resource_types = 1;
        int m_nr_resource_interfaces = 2;
        ObservationIds m_interestedObservers;

        // member variables for path: "/mnt"
        std::vector<std::string>  m_var_value_if; // the value for the array attribute "if": The interface set supported by this resource
        std::string m_var_name_if = "if"; // the name for the attribute "if" 
        bool m_var_value_rb; // the value for the attribute "rb": Reboot Action
        std::string m_var_name_rb = "rb"; // the name for the attribute "rb" 
        std::string m_var_value_n; // the value for the attribute "n": Friendly name of the resource
        std::string m_var_name_n = "n"; // the name for the attribute "n" 
        bool m_var_value_fr; // the value for the attribute "fr": Factory Reset
        std::string m_var_name_fr = "fr"; // the name for the attribute "fr" 
        std::vector<std::string>  m_var_value_rt; // the value for the array attribute "rt": Resource Type of the Resource
        std::string m_var_name_rt = "rt"; // the name for the attribute "rt" 
        
    protected:
        /*
         * function to check if the interface is
         * @param  interface_name the interface name used during the request
         * @return true: updatable interface
         */
        bool in_updatable_interfaces(std::string interface_name);

        /*
         * the entity handler for this resource
         * @param request the incoming request to handle
         * @return OCEntityHandlerResult ok or not ok indication
         */
        virtual OCEntityHandlerResult entityHandler(std::shared_ptr<OC::OCResourceRequest> request);
};

/*
* Constructor code
*/
MntResource::MntResource():
    m_notify_thread_active(true),
    m_notify_thread(nullptr),
    m_send_notification_flag(false),
    m_cv_mutex{},
    m_cv{}
{
    std::cout << "- Running: MntResource constructor" << std::endl;
    std::string resourceURI = "/mnt";

    // initialize member variables /mnt
    // initialize vector if  The interface set supported by this resource
    
    m_var_value_if.push_back("oic.if.baseline");
    m_var_value_if.push_back("oic.if.rw");m_var_value_rb = false; // current value of property "rb" Reboot Action
    m_var_value_n = "";  // current value of property "n" Friendly name of the resource
    m_var_value_fr = false; // current value of property "fr" Factory Reset
    // initialize vector rt  Resource Type of the Resource
    
    m_var_value_rt.push_back("oic.wk.mnt");EntityHandler cb = std::bind(&MntResource::entityHandler, this,PH::_1);
    OCStackResult result = OCPlatform::registerResource(m_resourceHandle,
                                                        resourceURI,
                                                        m_RESOURCE_TYPE[0],
                                                        m_RESOURCE_INTERFACE[0],
                                                        cb,
                                                        OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE );

    // add the additional resource types
    for( int a = 1; a < m_nr_resource_types; a++ )
    {
        OCStackResult result1 = OCPlatform::bindTypeToResource(m_resourceHandle, m_RESOURCE_TYPE[a].c_str());
        if (result1 != OC_STACK_OK)
            std::cerr << "Could not bind resource type:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }
    // add the additional interfaces
    for( int a = 1; a < m_nr_resource_interfaces; a++)
    {
        OCStackResult result2 = OCPlatform::bindInterfaceToResource(m_resourceHandle, m_RESOURCE_INTERFACE[a].c_str());
        if (result2 != OC_STACK_OK)
            std::cerr << "Could not bind interface:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }

    std::cout << "\t" << "# resource interfaces: " << m_nr_resource_interfaces << std::endl;
    std::cout << "\t" << "# resource types     : " << m_nr_resource_types << std::endl;

    if(OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("MntResource failed to start")+std::to_string(result));
    }

    m_send_notification_flag = false;
    m_notify_thread_active = true;
    m_notify_thread = new std::thread(&MntResource::notifyObservers, this);
}

/*
* Destructor code
*/
MntResource::~MntResource()
{
    m_notify_thread_active = false;
    m_cv.notify_all();
    if(m_notify_thread->joinable())
    {
        m_notify_thread->join();
    }
}

/*
* function to make the payload for the retrieve function (e.g. GET) /mnt
* @param queries  the query parameters for this call
*/
OCRepresentation MntResource::get(QueryParamsMap queries)
{
    OC_UNUSED(queries);
    m_rep.setValue(m_var_name_if,  m_var_value_if ); 
    m_rep.setValue(m_var_name_rb, m_var_value_rb ); 
    m_rep.setValue(m_var_name_n, m_var_value_n ); 
    m_rep.setValue(m_var_name_fr, m_var_value_fr ); 
    m_rep.setValue(m_var_name_rt,  m_var_value_rt ); 

    return m_rep;
}

/*
* function to parse the payload for the update function (e.g. POST) /mnt
* @param queries  the query parameters for this call
* @param rep  the response to get the property values from
* @return OCEntityHandlerResult ok or not ok indication
*/
OCEntityHandlerResult MntResource::post(QueryParamsMap queries, const OCRepresentation& rep)
{
    OCEntityHandlerResult ehResult = OC_EH_OK;
    OC_UNUSED(queries);
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_if))
        {
            // value exist in payload
            
            // check if "if" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'if' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    try {
        if (rep.hasAttribute(m_var_name_rb))
        {
            // value exist in payload
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    
    try {
        if (rep.hasAttribute(m_var_name_n))
        {
            // value exist in payload
            
            // check if "n" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'n' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    try {
        if (rep.hasAttribute(m_var_name_fr))
        {
            // value exist in payload
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_rt))
        {
            // value exist in payload
            
            // check if "rt" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'rt' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }if (ehResult == OC_EH_OK)
    {
        // no error: assign the variables
        // array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_if))
            {
                rep.getValue(m_var_name_if, m_var_value_if);
                int first = 1;
                std::cout << "\t\t" << "property 'if' UPDATED: " ;
                for(auto myvar: m_var_value_if)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'if' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_rb, m_var_value_rb ))
            {
                std::cout << "\t\t" << "property 'rb' UPDATED: " << ((m_var_value_rb) ? "true" : "false") << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'rb' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_n, m_var_value_n ))
            {
                std::cout << "\t\t" << "property 'n' UPDATED: " << m_var_value_n << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'n' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_fr, m_var_value_fr ))
            {
                std::cout << "\t\t" << "property 'fr' UPDATED: " << ((m_var_value_fr) ? "true" : "false") << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'fr' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }// array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_rt))
            {
                rep.getValue(m_var_name_rt, m_var_value_rt);
                int first = 1;
                std::cout << "\t\t" << "property 'rt' UPDATED: " ;
                for(auto myvar: m_var_value_rt)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'rt' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        if (m_send_notification_flag)
        {
            m_cv.notify_all();
        }
    }
    return ehResult;
}
/*
* function to check if the interface name is an registered interface name
*/
bool MntResource::in_updatable_interfaces(std::string interface_name)
{
    for (int i=0; i<3; i++)
    {
        if (m_IF_UPDATE[i].compare(interface_name) == 0)
            return true;
    }
    return false;
}

/*
* the entity handler
*/
OCEntityHandlerResult MntResource::entityHandler(std::shared_ptr<OCResourceRequest> request)
{
    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    //std::cout << "In entity handler for MntResource " << std::endl;

    if(request)
    {
        std::cout << "In entity handler for MntResource, URI is : "
                  << request->getResourceUri() << std::endl;

        // Check for query params (if any)
        QueryParamsMap queries = request->getQueryParameters();
        if (!queries.empty())
        {
            std::cout << "\nQuery processing up to entityHandler" << std::endl;
        }
        for (auto it : queries)
        {
            std::cout << "Query key: " << it.first << " value : " << it.second
                    << std::endl;
        }
        // get the value, so that we can AND it to check which flags are set
        int requestFlag = request->getRequestHandlerFlag();

        if(requestFlag & RequestHandlerFlag::RequestFlag)
        {
            // request flag is set
            auto pResponse = std::make_shared<OC::OCResourceResponse>();
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());

            if(request->getRequestType() == "GET")
            {
                std::cout<<"MntResource Get Request"<< std::endl;

                pResponse->setResourceRepresentation(get(queries), "");
                if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                {
                    ehResult = OC_EH_OK;
                }
            }

            else if(request->getRequestType() == "POST")
            {
                std::cout <<"MntResource Post Request"<<std::endl;
                bool  handle_post = true;

                if (queries.size() > 0)
                {
                    for (const auto &eachQuery : queries)
                    {
                        std::string key = eachQuery.first;
                        if (key.compare(INTERFACE_KEY) == 0)
                        {
                            std::string value = eachQuery.second;
                            if (in_updatable_interfaces(value) == false)
                            {
                                std::cout << "Update request received via interface: " << value
                                            << " . This interface is not authorized to update resource!!" << std::endl;
                                pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_FORBIDDEN);
                                handle_post = false;
                                ehResult = OC_EH_ERROR;
                                break;
                            }
                        }
                    }
                }
                if (handle_post)
                {
                    ehResult = post(queries, request->getResourceRepresentation());
                    if (ehResult == OC_EH_OK)
                    {
                        pResponse->setResourceRepresentation(get(queries), "");
                    }
                    else
                    {
                         pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_ERROR);
                    }
                    OC_VERIFY(OCPlatform::sendResponse(pResponse) == OC_STACK_OK);
                }
            }
            else
            {
                std::cout << "MntResource unsupported request type (delete,put,..)"
                    << request->getRequestType() << std::endl;
                pResponse->setResponseResult(OC_EH_ERROR);
                OCPlatform::sendResponse(pResponse);
                ehResult = OC_EH_ERROR;
            }
        }

        if(requestFlag & RequestHandlerFlag::ObserverFlag)
        {
            // observe flag is set
            std::cout << "\t\trequestFlag : Observer\n" << std::endl;
            ObservationInfo observationInfo = request->getObservationInfo();
            if(ObserveAction::ObserveRegister == observationInfo.action)
            {
                // add observer
                m_interestedObservers.push_back(observationInfo.obsId);
            }
            else if(ObserveAction::ObserveUnregister == observationInfo.action)
            {
                // delete observer
                m_interestedObservers.erase(std::remove(
                                                            m_interestedObservers.begin(),
                                                            m_interestedObservers.end(),
                                                            observationInfo.obsId),
                                                            m_interestedObservers.end());
            }
            ehResult = OC_EH_OK;
        }
    }
    return ehResult;
}

/*
* the function running in the thread to update the registered observers
*/
void MntResource::notifyObservers(void)
{
    std::unique_lock<std::mutex> cv_lock(m_cv_mutex);
    while (m_notify_thread_active)
    {
        m_cv.wait(cv_lock);
        if (m_send_notification_flag)
        {
            m_send_notification_flag = false;
            std::cout << "Send NOTIFICATION to all observers" << std::endl;

            OCPlatform::notifyAllObservers(this->m_resourceHandle);
        }
    }
}


/*
 * class definition for class that handles /nmon
 *
 * The resource through which a Device can monitor network traffic.
 * Retrieve the network monitor action status

*/
class NmonResource : public Resource
{
    public:
        /*
         * constructor
         */
        NmonResource();

        /*
         * destructor
         */
         virtual ~NmonResource();
    private:

        /*
         * function to parse the payload for the update function (e.g. POST) /nmon
         * Start/Stop collecting and reset the networking monitor resource

         * @param queries  the query parameters for this call
         * @param rep  the response to get the property values from
         * @return OCEntityHandlerResult ok or not ok indication
         */
        OCEntityHandlerResult post(OC::QueryParamsMap queries, const OC::OCRepresentation& rep);

        /*
         * function to make the payload for the retrieve function (e.g. GET) /nmon
         * The resource through which a Device can monitor network traffic.
         * Retrieve the network monitor action status

         * @param queries  the query parameters for this call
         */
        OCRepresentation get(OC::QueryParamsMap queries);

        
        std::atomic<bool> m_notify_thread_active;       // signal if the thread should run 
        std::thread* m_notify_thread;                   // the thread running in the background to update the observe listeners
        std::atomic<bool> m_send_notification_flag;     // signal if the thread should update all registered listeners
        std::mutex m_cv_mutex;                          // mutex to lock the sending of the notification
        std::condition_variable m_cv;

        /*
         * Function responsible for notifying all observers when a property
         * on the resource has been changed.
         * this function is running in a thread. Send a notification the
         * m_send_notification_flag must be set to true and then m_cv.notify_all()
         * must be called. This should automatically be handled by the code when
         * PUT or POST is sent to the entityHandler.
         */
        void notifyObservers(void);

        // resource types and interfaces as array..
        std::string m_RESOURCE_TYPE[1] = {"oic.wk.nmon"}; // rt value (as an array)
        std::string m_RESOURCE_INTERFACE[2] = {"oic.if.baseline","oic.if.rw"}; // interface if (as an array)
        std::string m_IF_UPDATE[3] = {"oic.if.a", "oic.if.rw", "oic.if.baseline"}; // updateble interfaces
        int m_nr_resource_types = 1;
        int m_nr_resource_interfaces = 2;
        ObservationIds m_interestedObservers;

        // member variables for path: "/nmon"
        int m_var_value_ianaifType; // the value for the attribute "ianaifType": The type of the network connection, as defined by iana https://www.iana.org/assignments/ianaiftype-mib/ianaiftype-mib
        std::string m_var_name_ianaifType = "ianaifType"; // the name for the attribute "ianaifType" 
        int m_var_value_tx; // the value for the attribute "tx": Amount of transmitted kilo bytes from collection start time 'time'
        std::string m_var_name_tx = "tx"; // the name for the attribute "tx" 
        std::vector<std::string>  m_var_value_rt; // the value for the array attribute "rt": Resource Type of the Resource
        std::string m_var_name_rt = "rt"; // the name for the attribute "rt" 
        int m_var_value_mmsrx; // the value for the attribute "mmsrx": Maximum received message size in bytes (rx) in the collection period
        std::string m_var_name_mmsrx = "mmsrx"; // the name for the attribute "mmsrx" 
        int m_var_value_rx; // the value for the attribute "rx": Amount of received kilo bytes from collection start time 'time'
        std::string m_var_name_rx = "rx"; // the name for the attribute "rx" 
        std::vector<std::string>  m_var_value_if; // the value for the array attribute "if": The interface set supported by this resource
        std::string m_var_name_if = "if"; // the name for the attribute "if" 
        int m_var_value_amstx; // the value for the attribute "amstx": Average transmitted message size in bytes (tx) in the collection period
        std::string m_var_name_amstx = "amstx"; // the name for the attribute "amstx" 
        bool m_var_value_col; // the value for the attribute "col": True: Device is collecting values
        std::string m_var_name_col = "col"; // the name for the attribute "col" 
        int m_var_value_amsrx; // the value for the attribute "amsrx": Average received message size in bytes (rx) in the collection period
        std::string m_var_name_amsrx = "amsrx"; // the name for the attribute "amsrx" 
        bool m_var_value_reset; // the value for the attribute "reset": True: reset the collected values
        std::string m_var_name_reset = "reset"; // the name for the attribute "reset" 
        int m_var_value_mmstx; // the value for the attribute "mmstx": Maximum transmitted message size in bytes (tx) in the collection period
        std::string m_var_name_mmstx = "mmstx"; // the name for the attribute "mmstx" 
        
    protected:
        /*
         * function to check if the interface is
         * @param  interface_name the interface name used during the request
         * @return true: updatable interface
         */
        bool in_updatable_interfaces(std::string interface_name);

        /*
         * the entity handler for this resource
         * @param request the incoming request to handle
         * @return OCEntityHandlerResult ok or not ok indication
         */
        virtual OCEntityHandlerResult entityHandler(std::shared_ptr<OC::OCResourceRequest> request);
};

/*
* Constructor code
*/
NmonResource::NmonResource():
    m_notify_thread_active(true),
    m_notify_thread(nullptr),
    m_send_notification_flag(false),
    m_cv_mutex{},
    m_cv{}
{
    std::cout << "- Running: NmonResource constructor" << std::endl;
    std::string resourceURI = "/nmon";

    // initialize member variables /nmon
    m_var_value_ianaifType = 71; // current value of property "ianaifType" The type of the network connection, as defined by iana https://www.iana.org/assignments/ianaiftype-mib/ianaiftype-mib
    m_var_value_tx = 10; // current value of property "tx" Amount of transmitted kilo bytes from collection start time 'time'
    // initialize vector rt  Resource Type of the Resource
    
    m_var_value_rt.push_back("oic.wk.nmon");m_var_value_mmsrx = 35; // current value of property "mmsrx" Maximum received message size in bytes (rx) in the collection period
    m_var_value_rx = 15; // current value of property "rx" Amount of received kilo bytes from collection start time 'time'
    // initialize vector if  The interface set supported by this resource
    
    m_var_value_if.push_back("oic.if.baseline");
    m_var_value_if.push_back("oic.if.rw");m_var_value_amstx = 35; // current value of property "amstx" Average transmitted message size in bytes (tx) in the collection period
    m_var_value_col = false; // current value of property "col" True: Device is collecting values
    m_var_value_amsrx = 20; // current value of property "amsrx" Average received message size in bytes (rx) in the collection period
    m_var_value_reset = false; // current value of property "reset" True: reset the collected values
    m_var_value_mmstx = 50; // current value of property "mmstx" Maximum transmitted message size in bytes (tx) in the collection period
    EntityHandler cb = std::bind(&NmonResource::entityHandler, this,PH::_1);
    OCStackResult result = OCPlatform::registerResource(m_resourceHandle,
                                                        resourceURI,
                                                        m_RESOURCE_TYPE[0],
                                                        m_RESOURCE_INTERFACE[0],
                                                        cb,
                                                        OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE );

    // add the additional resource types
    for( int a = 1; a < m_nr_resource_types; a++ )
    {
        OCStackResult result1 = OCPlatform::bindTypeToResource(m_resourceHandle, m_RESOURCE_TYPE[a].c_str());
        if (result1 != OC_STACK_OK)
            std::cerr << "Could not bind resource type:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }
    // add the additional interfaces
    for( int a = 1; a < m_nr_resource_interfaces; a++)
    {
        OCStackResult result2 = OCPlatform::bindInterfaceToResource(m_resourceHandle, m_RESOURCE_INTERFACE[a].c_str());
        if (result2 != OC_STACK_OK)
            std::cerr << "Could not bind interface:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }

    std::cout << "\t" << "# resource interfaces: " << m_nr_resource_interfaces << std::endl;
    std::cout << "\t" << "# resource types     : " << m_nr_resource_types << std::endl;

    if(OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("NmonResource failed to start")+std::to_string(result));
    }

    m_send_notification_flag = false;
    m_notify_thread_active = true;
    m_notify_thread = new std::thread(&NmonResource::notifyObservers, this);
}

/*
* Destructor code
*/
NmonResource::~NmonResource()
{
    m_notify_thread_active = false;
    m_cv.notify_all();
    if(m_notify_thread->joinable())
    {
        m_notify_thread->join();
    }
}

/*
* function to make the payload for the retrieve function (e.g. GET) /nmon
* @param queries  the query parameters for this call
*/
OCRepresentation NmonResource::get(QueryParamsMap queries)
{
    OC_UNUSED(queries);
    
    if (m_var_value_col == true)
    {
        m_var_value_rx++;
        m_var_value_amsrx++;
        m_var_value_mmsrx++:
        
        m_var_value_tx++;
        m_var_value_amsrx++;
        m_var_value_mmstx++;
    }
    if (m_var_name_reset == true)
    {
        m_var_value_rx=0;
        m_var_value_amsrx=0;
        m_var_value_mmsrx=0;
        
        m_var_value_tx=0;
        m_var_value_amsrx=0;
        m_var_value_mmstx=0;
        m_var_name_reset = false;
    }
    
    m_rep.setValue(m_var_name_ianaifType, m_var_value_ianaifType ); 
    m_rep.setValue(m_var_name_tx, m_var_value_tx ); 
    m_rep.setValue(m_var_name_rt,  m_var_value_rt ); 
    m_rep.setValue(m_var_name_mmsrx, m_var_value_mmsrx ); 
    m_rep.setValue(m_var_name_rx, m_var_value_rx ); 
    m_rep.setValue(m_var_name_if,  m_var_value_if ); 
    m_rep.setValue(m_var_name_amstx, m_var_value_amstx ); 
    m_rep.setValue(m_var_name_col, m_var_value_col ); 
    m_rep.setValue(m_var_name_amsrx, m_var_value_amsrx ); 
    m_rep.setValue(m_var_name_reset, m_var_value_reset ); 
    m_rep.setValue(m_var_name_mmstx, m_var_value_mmstx ); 

    return m_rep;
}

/*
* function to parse the payload for the update function (e.g. POST) /nmon
* @param queries  the query parameters for this call
* @param rep  the response to get the property values from
* @return OCEntityHandlerResult ok or not ok indication
*/
OCEntityHandlerResult NmonResource::post(QueryParamsMap queries, const OCRepresentation& rep)
{
    OCEntityHandlerResult ehResult = OC_EH_OK;
    OC_UNUSED(queries);
    try {
        if (rep.hasAttribute(m_var_name_ianaifType))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_ianaifType, value);
            
            // check if "ianaifType" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'ianaifType' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_tx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_tx, value);
            
            // check if "tx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'tx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_rt))
        {
            // value exist in payload
            
            // check if "rt" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'rt' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    try {
        if (rep.hasAttribute(m_var_name_mmsrx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_mmsrx, value);
            
            // check if "mmsrx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'mmsrx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_rx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_rx, value);
            
            // check if "rx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'rx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_if))
        {
            // value exist in payload
            
            // check if "if" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'if' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    try {
        if (rep.hasAttribute(m_var_name_amstx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_amstx, value);
            
            // check if "amstx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'amstx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_col))
        {
            // value exist in payload
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_amsrx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_amsrx, value);
            
            // check if "amsrx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'amsrx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_reset))
        {
            // value exist in payload
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_mmstx))
        {
            // value exist in payload
            // allocate the variable
            int value;
            // get the actual value from the payload
            rep.getValue(m_var_name_mmstx, value);
            
            // check if "mmstx" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'mmstx' is readOnly "<< std::endl;
            
            
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    if (ehResult == OC_EH_OK)
    {
        // no error: assign the variables
        
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_ianaifType, m_var_value_ianaifType ))
            {
                std::cout << "\t\t" << "property 'ianaifType' UPDATED: " << m_var_value_ianaifType << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'ianaifType' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_tx, m_var_value_tx ))
            {
                std::cout << "\t\t" << "property 'tx' UPDATED: " << m_var_value_tx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'tx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }// array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_rt))
            {
                rep.getValue(m_var_name_rt, m_var_value_rt);
                int first = 1;
                std::cout << "\t\t" << "property 'rt' UPDATED: " ;
                for(auto myvar: m_var_value_rt)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'rt' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_mmsrx, m_var_value_mmsrx ))
            {
                std::cout << "\t\t" << "property 'mmsrx' UPDATED: " << m_var_value_mmsrx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'mmsrx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_rx, m_var_value_rx ))
            {
                std::cout << "\t\t" << "property 'rx' UPDATED: " << m_var_value_rx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'rx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }// array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_if))
            {
                rep.getValue(m_var_name_if, m_var_value_if);
                int first = 1;
                std::cout << "\t\t" << "property 'if' UPDATED: " ;
                for(auto myvar: m_var_value_if)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'if' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_amstx, m_var_value_amstx ))
            {
                std::cout << "\t\t" << "property 'amstx' UPDATED: " << m_var_value_amstx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'amstx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_col, m_var_value_col ))
            {
                std::cout << "\t\t" << "property 'col' UPDATED: " << ((m_var_value_col) ? "true" : "false") << std::endl;
                
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'col' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_amsrx, m_var_value_amsrx ))
            {
                std::cout << "\t\t" << "property 'amsrx' UPDATED: " << m_var_value_amsrx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'amsrx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_reset, m_var_value_reset ))
            {
                std::cout << "\t\t" << "property 'reset' UPDATED: " << ((m_var_value_reset) ? "true" : "false") << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'reset' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            // value exist in payload
            if (rep.getValue(m_var_name_mmstx, m_var_value_mmstx ))
            {
                std::cout << "\t\t" << "property 'mmstx' UPDATED: " << m_var_value_mmstx << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'mmstx' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        if (m_send_notification_flag)
        {
            m_cv.notify_all();
        }
    }
    return ehResult;
}
/*
* function to check if the interface name is an registered interface name
*/
bool NmonResource::in_updatable_interfaces(std::string interface_name)
{
    for (int i=0; i<3; i++)
    {
        if (m_IF_UPDATE[i].compare(interface_name) == 0)
            return true;
    }
    return false;
}

/*
* the entity handler
*/
OCEntityHandlerResult NmonResource::entityHandler(std::shared_ptr<OCResourceRequest> request)
{
    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    //std::cout << "In entity handler for NmonResource " << std::endl;

    if(request)
    {
        std::cout << "In entity handler for NmonResource, URI is : "
                  << request->getResourceUri() << std::endl;

        // Check for query params (if any)
        QueryParamsMap queries = request->getQueryParameters();
        if (!queries.empty())
        {
            std::cout << "\nQuery processing up to entityHandler" << std::endl;
        }
        for (auto it : queries)
        {
            std::cout << "Query key: " << it.first << " value : " << it.second
                    << std::endl;
        }
        // get the value, so that we can AND it to check which flags are set
        int requestFlag = request->getRequestHandlerFlag();

        if(requestFlag & RequestHandlerFlag::RequestFlag)
        {
            // request flag is set
            auto pResponse = std::make_shared<OC::OCResourceResponse>();
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());

            if(request->getRequestType() == "GET")
            {
                std::cout<<"NmonResource Get Request"<< std::endl;

                pResponse->setResourceRepresentation(get(queries), "");
                if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                {
                    ehResult = OC_EH_OK;
                }
            }

            else if(request->getRequestType() == "POST")
            {
                std::cout <<"NmonResource Post Request"<<std::endl;
                bool  handle_post = true;

                if (queries.size() > 0)
                {
                    for (const auto &eachQuery : queries)
                    {
                        std::string key = eachQuery.first;
                        if (key.compare(INTERFACE_KEY) == 0)
                        {
                            std::string value = eachQuery.second;
                            if (in_updatable_interfaces(value) == false)
                            {
                                std::cout << "Update request received via interface: " << value
                                            << " . This interface is not authorized to update resource!!" << std::endl;
                                pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_FORBIDDEN);
                                handle_post = false;
                                ehResult = OC_EH_ERROR;
                                break;
                            }
                        }
                    }
                }
                if (handle_post)
                {
                    ehResult = post(queries, request->getResourceRepresentation());
                    if (ehResult == OC_EH_OK)
                    {
                        pResponse->setResourceRepresentation(get(queries), "");
                    }
                    else
                    {
                         pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_ERROR);
                    }
                    OC_VERIFY(OCPlatform::sendResponse(pResponse) == OC_STACK_OK);
                }
            }
            else
            {
                std::cout << "NmonResource unsupported request type (delete,put,..)"
                    << request->getRequestType() << std::endl;
                pResponse->setResponseResult(OC_EH_ERROR);
                OCPlatform::sendResponse(pResponse);
                ehResult = OC_EH_ERROR;
            }
        }

        if(requestFlag & RequestHandlerFlag::ObserverFlag)
        {
            // observe flag is set
            std::cout << "\t\trequestFlag : Observer\n" << std::endl;
            ObservationInfo observationInfo = request->getObservationInfo();
            if(ObserveAction::ObserveRegister == observationInfo.action)
            {
                // add observer
                m_interestedObservers.push_back(observationInfo.obsId);
            }
            else if(ObserveAction::ObserveUnregister == observationInfo.action)
            {
                // delete observer
                m_interestedObservers.erase(std::remove(
                                                            m_interestedObservers.begin(),
                                                            m_interestedObservers.end(),
                                                            observationInfo.obsId),
                                                            m_interestedObservers.end());
            }
            ehResult = OC_EH_OK;
        }
    }
    return ehResult;
}

/*
* the function running in the thread to update the registered observers
*/
void NmonResource::notifyObservers(void)
{
    std::unique_lock<std::mutex> cv_lock(m_cv_mutex);
    while (m_notify_thread_active)
    {
        m_cv.wait(cv_lock);
        if (m_send_notification_flag)
        {
            m_send_notification_flag = false;
            std::cout << "Send NOTIFICATION to all observers" << std::endl;

            OCPlatform::notifyAllObservers(this->m_resourceHandle);
        }
    }
}


/*
 * class definition for class that handles /binaryswitch
 *
 * This resource describes a binary switch (on/off).
 * The value is a boolean.
 * A value of 'true' means that the switch is on.
 * A value of 'false' means that the switch is off.

*/
class BinaryswitchResource : public Resource
{
    public:
        /*
         * constructor
         */
        BinaryswitchResource();

        /*
         * destructor
         */
         virtual ~BinaryswitchResource();
    private:

        /*
         * function to parse the payload for the update function (e.g. POST) /binaryswitch

         * @param queries  the query parameters for this call
         * @param rep  the response to get the property values from
         * @return OCEntityHandlerResult ok or not ok indication
         */
        OCEntityHandlerResult post(OC::QueryParamsMap queries, const OC::OCRepresentation& rep);

        /*
         * function to make the payload for the retrieve function (e.g. GET) /binaryswitch
         * This resource describes a binary switch (on/off).
         * The value is a boolean.
         * A value of 'true' means that the switch is on.
         * A value of 'false' means that the switch is off.

         * @param queries  the query parameters for this call
         */
        OCRepresentation get(OC::QueryParamsMap queries);

        
        std::atomic<bool> m_notify_thread_active;       // signal if the thread should run 
        std::thread* m_notify_thread;                   // the thread running in the background to update the observe listeners
        std::atomic<bool> m_send_notification_flag;     // signal if the thread should update all registered listeners
        std::mutex m_cv_mutex;                          // mutex to lock the sending of the notification
        std::condition_variable m_cv;

        /*
         * Function responsible for notifying all observers when a property
         * on the resource has been changed.
         * this function is running in a thread. Send a notification the
         * m_send_notification_flag must be set to true and then m_cv.notify_all()
         * must be called. This should automatically be handled by the code when
         * PUT or POST is sent to the entityHandler.
         */
        void notifyObservers(void);

        // resource types and interfaces as array..
        std::string m_RESOURCE_TYPE[1] = {"oic.r.switch.binary"}; // rt value (as an array)
        std::string m_RESOURCE_INTERFACE[2] = {"oic.if.baseline","oic.if.a"}; // interface if (as an array)
        std::string m_IF_UPDATE[3] = {"oic.if.a", "oic.if.rw", "oic.if.baseline"}; // updateble interfaces
        int m_nr_resource_types = 1;
        int m_nr_resource_interfaces = 2;
        ObservationIds m_interestedObservers;

        // member variables for path: "/binaryswitch"
        std::vector<std::string>  m_var_value_if; // the value for the array attribute "if": The interface set supported by this resource
        std::string m_var_name_if = "if"; // the name for the attribute "if" 
        bool m_var_value_value; // the value for the attribute "value": Status of the switch
        std::string m_var_name_value = "value"; // the name for the attribute "value" 
        std::vector<std::string>  m_var_value_rt; // the value for the array attribute "rt": Resource Type
        std::string m_var_name_rt = "rt"; // the name for the attribute "rt" 
        std::string m_var_value_n; // the value for the attribute "n": Friendly name of the resource
        std::string m_var_name_n = "n"; // the name for the attribute "n" 
        
    protected:
        /*
         * function to check if the interface is
         * @param  interface_name the interface name used during the request
         * @return true: updatable interface
         */
        bool in_updatable_interfaces(std::string interface_name);

        /*
         * the entity handler for this resource
         * @param request the incoming request to handle
         * @return OCEntityHandlerResult ok or not ok indication
         */
        virtual OCEntityHandlerResult entityHandler(std::shared_ptr<OC::OCResourceRequest> request);
};

/*
* Constructor code
*/
BinaryswitchResource::BinaryswitchResource():
    m_notify_thread_active(true),
    m_notify_thread(nullptr),
    m_send_notification_flag(false),
    m_cv_mutex{},
    m_cv{}
{
    std::cout << "- Running: BinaryswitchResource constructor" << std::endl;
    std::string resourceURI = "/binaryswitch";

    // initialize member variables /binaryswitch
    // initialize vector if  The interface set supported by this resource
    
    m_var_value_if.push_back("oic.if.baseline");
    m_var_value_if.push_back("oic.if.a");m_var_value_value = false; // current value of property "value" Status of the switch
    // initialize vector rt  Resource Type
    
    m_var_value_rt.push_back("oic.r.switch.binary");m_var_value_n = "";  // current value of property "n" Friendly name of the resource
    EntityHandler cb = std::bind(&BinaryswitchResource::entityHandler, this,PH::_1);
    OCStackResult result = OCPlatform::registerResource(m_resourceHandle,
                                                        resourceURI,
                                                        m_RESOURCE_TYPE[0],
                                                        m_RESOURCE_INTERFACE[0],
                                                        cb,
                                                        OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE );

    // add the additional resource types
    for( int a = 1; a < m_nr_resource_types; a++ )
    {
        OCStackResult result1 = OCPlatform::bindTypeToResource(m_resourceHandle, m_RESOURCE_TYPE[a].c_str());
        if (result1 != OC_STACK_OK)
            std::cerr << "Could not bind resource type:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }
    // add the additional interfaces
    for( int a = 1; a < m_nr_resource_interfaces; a++)
    {
        OCStackResult result2 = OCPlatform::bindInterfaceToResource(m_resourceHandle, m_RESOURCE_INTERFACE[a].c_str());
        if (result2 != OC_STACK_OK)
            std::cerr << "Could not bind interface:" << m_RESOURCE_INTERFACE[a] << std::endl;
    }

    std::cout << "\t" << "# resource interfaces: " << m_nr_resource_interfaces << std::endl;
    std::cout << "\t" << "# resource types     : " << m_nr_resource_types << std::endl;

    if(OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("BinaryswitchResource failed to start")+std::to_string(result));
    }

    m_send_notification_flag = false;
    m_notify_thread_active = true;
    m_notify_thread = new std::thread(&BinaryswitchResource::notifyObservers, this);
}

/*
* Destructor code
*/
BinaryswitchResource::~BinaryswitchResource()
{
    m_notify_thread_active = false;
    m_cv.notify_all();
    if(m_notify_thread->joinable())
    {
        m_notify_thread->join();
    }
}

/*
* function to make the payload for the retrieve function (e.g. GET) /binaryswitch
* @param queries  the query parameters for this call
*/
OCRepresentation BinaryswitchResource::get(QueryParamsMap queries)
{
    OC_UNUSED(queries);
    m_rep.setValue(m_var_name_if,  m_var_value_if ); 
    m_rep.setValue(m_var_name_value, m_var_value_value ); 
    m_rep.setValue(m_var_name_rt,  m_var_value_rt ); 
    m_rep.setValue(m_var_name_n, m_var_value_n ); 

    return m_rep;
}

/*
* function to parse the payload for the update function (e.g. POST) /binaryswitch
* @param queries  the query parameters for this call
* @param rep  the response to get the property values from
* @return OCEntityHandlerResult ok or not ok indication
*/
OCEntityHandlerResult BinaryswitchResource::post(QueryParamsMap queries, const OCRepresentation& rep)
{
    OCEntityHandlerResult ehResult = OC_EH_OK;
    OC_UNUSED(queries);
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_if))
        {
            // value exist in payload
            
            // check if "if" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'if' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    try {
        if (rep.hasAttribute(m_var_name_value))
        {
            // value exist in payload
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    
    // TODO add check on array contents out of range, etc..
    try {
        if (rep.hasAttribute(m_var_name_rt))
        {
            // value exist in payload
            
            // check if "rt" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'rt' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    try {
        if (rep.hasAttribute(m_var_name_n))
        {
            // value exist in payload
            
            // check if "n" is read only
            ehResult = OC_EH_ERROR;
            std::cout << "\t\t" << "property 'n' is readOnly "<< std::endl;
            
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }if (ehResult == OC_EH_OK)
    {
        // no error: assign the variables
        // array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_if))
            {
                rep.getValue(m_var_name_if, m_var_value_if);
                int first = 1;
                std::cout << "\t\t" << "property 'if' UPDATED: " ;
                for(auto myvar: m_var_value_if)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'if' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_value, m_var_value_value ))
            {
                std::cout << "\t\t" << "property 'value' UPDATED: " << ((m_var_value_value) ? "true" : "false") << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'value' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }// array only works for integer, boolean, numbers and strings
        // TODO: make it also work with array of objects
        try {
            if (rep.hasAttribute(m_var_name_rt))
            {
                rep.getValue(m_var_name_rt, m_var_value_rt);
                int first = 1;
                std::cout << "\t\t" << "property 'rt' UPDATED: " ;
                for(auto myvar: m_var_value_rt)
                {
                    if(first)
                    {
                        std::cout << myvar;
                        first = 0;
                    }
                    else
                    {
                        std::cout << "," << myvar;
                    }
                }
                std::cout <<  std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'rt' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
        try {
            if (rep.getValue(m_var_name_n, m_var_value_n ))
            {
                std::cout << "\t\t" << "property 'n' UPDATED: " << m_var_value_n << std::endl;
                m_send_notification_flag = true;
            }
            else
            {
                std::cout << "\t\t" << "property 'n' not found in the representation" << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }

        if (m_send_notification_flag)
        {
            m_cv.notify_all();
        }
    }
    return ehResult;
}
/*
* function to check if the interface name is an registered interface name
*/
bool BinaryswitchResource::in_updatable_interfaces(std::string interface_name)
{
    for (int i=0; i<3; i++)
    {
        if (m_IF_UPDATE[i].compare(interface_name) == 0)
            return true;
    }
    return false;
}

/*
* the entity handler
*/
OCEntityHandlerResult BinaryswitchResource::entityHandler(std::shared_ptr<OCResourceRequest> request)
{
    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    //std::cout << "In entity handler for BinaryswitchResource " << std::endl;

    if(request)
    {
        std::cout << "In entity handler for BinaryswitchResource, URI is : "
                  << request->getResourceUri() << std::endl;

        // Check for query params (if any)
        QueryParamsMap queries = request->getQueryParameters();
        if (!queries.empty())
        {
            std::cout << "\nQuery processing up to entityHandler" << std::endl;
        }
        for (auto it : queries)
        {
            std::cout << "Query key: " << it.first << " value : " << it.second
                    << std::endl;
        }
        // get the value, so that we can AND it to check which flags are set
        int requestFlag = request->getRequestHandlerFlag();

        if(requestFlag & RequestHandlerFlag::RequestFlag)
        {
            // request flag is set
            auto pResponse = std::make_shared<OC::OCResourceResponse>();
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());

            if(request->getRequestType() == "GET")
            {
                std::cout<<"BinaryswitchResource Get Request"<< std::endl;

                pResponse->setResourceRepresentation(get(queries), "");
                if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                {
                    ehResult = OC_EH_OK;
                }
            }

            else if(request->getRequestType() == "POST")
            {
                std::cout <<"BinaryswitchResource Post Request"<<std::endl;
                bool  handle_post = true;

                if (queries.size() > 0)
                {
                    for (const auto &eachQuery : queries)
                    {
                        std::string key = eachQuery.first;
                        if (key.compare(INTERFACE_KEY) == 0)
                        {
                            std::string value = eachQuery.second;
                            if (in_updatable_interfaces(value) == false)
                            {
                                std::cout << "Update request received via interface: " << value
                                            << " . This interface is not authorized to update resource!!" << std::endl;
                                pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_FORBIDDEN);
                                handle_post = false;
                                ehResult = OC_EH_ERROR;
                                break;
                            }
                        }
                    }
                }
                if (handle_post)
                {
                    ehResult = post(queries, request->getResourceRepresentation());
                    if (ehResult == OC_EH_OK)
                    {
                        pResponse->setResourceRepresentation(get(queries), "");
                    }
                    else
                    {
                         pResponse->setResponseResult(OCEntityHandlerResult::OC_EH_ERROR);
                    }
                    OC_VERIFY(OCPlatform::sendResponse(pResponse) == OC_STACK_OK);
                }
            }
            else
            {
                std::cout << "BinaryswitchResource unsupported request type (delete,put,..)"
                    << request->getRequestType() << std::endl;
                pResponse->setResponseResult(OC_EH_ERROR);
                OCPlatform::sendResponse(pResponse);
                ehResult = OC_EH_ERROR;
            }
        }

        if(requestFlag & RequestHandlerFlag::ObserverFlag)
        {
            // observe flag is set
            std::cout << "\t\trequestFlag : Observer\n" << std::endl;
            ObservationInfo observationInfo = request->getObservationInfo();
            if(ObserveAction::ObserveRegister == observationInfo.action)
            {
                // add observer
                m_interestedObservers.push_back(observationInfo.obsId);
            }
            else if(ObserveAction::ObserveUnregister == observationInfo.action)
            {
                // delete observer
                m_interestedObservers.erase(std::remove(
                                                            m_interestedObservers.begin(),
                                                            m_interestedObservers.end(),
                                                            observationInfo.obsId),
                                                            m_interestedObservers.end());
            }
            ehResult = OC_EH_OK;
        }
    }
    return ehResult;
}

/*
* the function running in the thread to update the registered observers
*/
void BinaryswitchResource::notifyObservers(void)
{
    std::unique_lock<std::mutex> cv_lock(m_cv_mutex);
    while (m_notify_thread_active)
    {
        m_cv.wait(cv_lock);
        if (m_send_notification_flag)
        {
            m_send_notification_flag = false;
            std::cout << "Send NOTIFICATION to all observers" << std::endl;

            OCPlatform::notifyAllObservers(this->m_resourceHandle);
        }
    }
}



class IoTServer
{
    public:
        /**
         *  constructor
         *  creates all resources from the resource classes.
         */
        IoTServer();

        /**
         *  destructor
         */
        ~IoTServer();

    private:
        MntResource  m_mntInstance;
        NmonResource  m_nmonInstance;
        BinaryswitchResource  m_binaryswitchInstance;};

IoTServer::IoTServer()
    :m_mntInstance(),
     m_nmonInstance(),
     m_binaryswitchInstance()
{
    std::cout << "Running IoTServer constructor" << std::endl;
}

IoTServer::~IoTServer()
{
    std::cout << "Running IoTServer destructor" << std::endl;
}

class Platform
{
public:
    /*
     * Platform constructor
     */
    Platform(void);

    /*
     * Platform destructor
     */
    virtual ~Platform();

    /*
     * Start the platform
     */
    OCStackResult start();

    /*
     * Register all platform info
     * This will register the platformId, manufaturerName, manufacturerUrl,
     * modelNumber, dateOfManufacture, platformVersion, operatingSystemVersion,
     * hardwareVersion, firmwareVersion, supportUrl, and systemTime
     */
    OCStackResult registerPlatformInfo(void);

    /*
     * Get OCPlatformInfo pointer
     */
    OCPlatformInfo* getPlatformInfo(void);

    /*
     * Stop the platform
     */
    OCStackResult stop(void);

    /*
     * SetDeviceInfo
     * Sets the device information ("/oic/d")
     *
     * @return OC_STACK_OK on success OC_STACK_ERROR on failure
     */
    OCStackResult setDeviceInfo(void);

    // Set of strings for each of device info fields
    std::string  deviceName = "Binary Switch";
    std::string  deviceType = "oic.d.light";
    std::string  specVersion = "ocf.1.0.0";
    std::vector<std::string> dataModelVersions;

    std::string  protocolIndependentID = "fa008167-3bbf-4c9d-8604-c9bcb96cb712";

private:
    // Set of strings for each of platform Info fields
    std::string m_platformId = "0A3E0D6F-DBF5-404E-8719-D6880042463A";
    std::string m_manufacturerName = "ocf";
    std::string m_manufacturerLink = "https://ocf.org/";
    std::string m_modelNumber = "ModelNumber";
    std::string m_dateOfManufacture = "2017-12-01";
    std::string m_platformVersion = "1.0";
    std::string m_operatingSystemVersion = "myOS";
    std::string m_hardwareVersion = "1.0";
    std::string m_firmwareVersion = "1.0";
    std::string m_supportLink = "https://ocf.org/";
    std::string m_systemTime = "2017-12-01T12:00:00.52Z";

    /*
    * duplicateString
    *
    * @param targetString  destination string, will be allocated
    * @param sourceString  source string, e.g. will be copied
    *  TODO: don't use strncpy
    */
    void duplicateString(char ** targetString, std::string sourceString);

    /**
     *  SetPlatformInfo
     *  Sets the platform information ("oic/p")
     *
     * @param platformID the platformID
     * @param manufacturerName the manufacturerName
     * @param manufacturerUrl the manufacturerUrl
     * @param modelNumber the modelNumber
     * @param platformVersion the platformVersion
     * @param operatingSystemVersion the operatingSystemVersion
     * @param hardwareVersion the hardwareVersion
     * @param firmwareVersion the firmwareVersion
     * @param supportUrl the supportUrl
     * @param systemTime the systemTime
     * @return OC_STACK_ERROR or OC_STACK_OK
     */
    void setPlatformInfo(std::string platformID,
                         std::string manufacturerName,
                         std::string manufacturerUrl,
                         std::string modelNumber,
                         std::string dateOfManufacture,
                         std::string platformVersion,
                         std::string operatingSystemVersion,
                         std::string hardwareVersion,
                         std::string firmwareVersion,
                         std::string supportUrl,
                         std::string systemTime);

    /*
     * deletePlatformInfo
     * Deleted the allocated platform information
     */
    void deletePlatformInfo(void);

    // OCPlatformInfo Contains all the platform info
    OCPlatformInfo platformInfo;
};

/**
*  server_fopen
*  opens file
*  implements redirection to open:
* - initial security settings
* - introspection file
* @param path path+filename of the file to open
* @param mode mode of the file to open
* @return the filehandle of the opened file (or error)
*/
FILE* server_fopen(const char* path, const char* mode)
{
    FILE* fileptr = NULL;

    if (0 == strcmp(path, OC_SECURITY_DB_DAT_FILE_NAME))
    {
        // reading the security initial setup file
        fileptr = fopen("server_security.dat", mode);
        std::cout << "reading security file 'server_security.dat' ptr: " << fileptr << std::endl;
        return fileptr;
    }
    else if (0 == strcmp(path, OC_INTROSPECTION_FILE_NAME))
    {
        // reading the introspection file
        fileptr = fopen("server_introspection.dat", mode);
        std::cout << "reading introspection file  'server_introspection.dat' ptr: " << fileptr << std::endl;
        return fileptr;
    }
    else
    {
        std::cout << "persistent storage - server_fopen: " << path << std::endl;
        return fopen(path, mode);
    }
}

// Create persistent storage handlers
OCPersistentStorage ps{server_fopen, fread, fwrite, fclose, unlink};

/*
* Platform Constructor
*/
Platform::Platform(void)
{
    std::cout << "Running Platform constructor" << std::endl;
    dataModelVersions.push_back("ocf.res.1.3.0");
    dataModelVersions.push_back("ocf.sh.1.3.0");

    // create the platform
    PlatformConfig cfg
    {
        ServiceType::InProc,
        ModeType::Server,
        &ps
    };
    OCPlatform::Configure(cfg);
    setPlatformInfo(m_platformId, m_manufacturerName, m_manufacturerLink,
                    m_modelNumber, m_dateOfManufacture, m_platformVersion,
                    m_operatingSystemVersion, m_hardwareVersion,
                    m_firmwareVersion, m_supportLink, m_systemTime);
}

/*
* Platform Destructor
*/
Platform::~Platform(void)
{
    std::cout << "Running Platform destructor" << std::endl;
    deletePlatformInfo();
}

OCStackResult Platform::start(void)
{
    return OCPlatform::start();
}

OCStackResult Platform::registerPlatformInfo(void)
{
    OCStackResult result = OC_STACK_ERROR;
    result = OCPlatform::registerPlatformInfo(platformInfo);
    return result;
}

OCPlatformInfo* Platform::getPlatformInfo(void)
{
    return &platformInfo;
}

OCStackResult Platform::stop(void)
{
    return OCPlatform::stop();
}

void Platform::duplicateString(char ** targetString, std::string sourceString)
{
    *targetString = new char[sourceString.length() + 1];
    strncpy(*targetString, sourceString.c_str(), (sourceString.length() + 1));
}

void Platform::setPlatformInfo(std::string platformID,
                               std::string manufacturerName,
                               std::string manufacturerUrl,
                               std::string modelNumber,
                               std::string dateOfManufacture,
                               std::string platformVersion,
                               std::string operatingSystemVersion,
                               std::string hardwareVersion,
                               std::string firmwareVersion,
                               std::string supportUrl,
                               std::string systemTime)
{
    duplicateString(&platformInfo.platformID, platformID);
    duplicateString(&platformInfo.manufacturerName, manufacturerName);
    duplicateString(&platformInfo.manufacturerUrl, manufacturerUrl);
    duplicateString(&platformInfo.modelNumber, modelNumber);
    duplicateString(&platformInfo.dateOfManufacture, dateOfManufacture);
    duplicateString(&platformInfo.platformVersion, platformVersion);
    duplicateString(&platformInfo.operatingSystemVersion, operatingSystemVersion);
    duplicateString(&platformInfo.hardwareVersion, hardwareVersion);
    duplicateString(&platformInfo.firmwareVersion, firmwareVersion);
    duplicateString(&platformInfo.supportUrl, supportUrl);
    duplicateString(&platformInfo.systemTime, systemTime);
}

void Platform::deletePlatformInfo()
{
    delete[] platformInfo.platformID;
    delete[] platformInfo.manufacturerName;
    delete[] platformInfo.manufacturerUrl;
    delete[] platformInfo.modelNumber;
    delete[] platformInfo.dateOfManufacture;
    delete[] platformInfo.platformVersion;
    delete[] platformInfo.operatingSystemVersion;
    delete[] platformInfo.hardwareVersion;
    delete[] platformInfo.firmwareVersion;
    delete[] platformInfo.supportUrl;
    delete[] platformInfo.systemTime;
}

/**
*  SetDeviceInfo
*  Sets the device information ("oic/d"), from the globals

* @return OC_STACK_ERROR or OC_STACK_OK
*/
OCStackResult Platform::setDeviceInfo()
{
    OCStackResult result = OC_STACK_ERROR;

    OCResourceHandle handle = OCGetResourceHandleAtUri(OC_RSRVD_DEVICE_URI);
    if (handle == NULL)
    {
        std::cout << "Failed to find resource " << OC_RSRVD_DEVICE_URI << std::endl;
        return result;
    }
    result = OCBindResourceTypeToResource(handle, deviceType.c_str());
    if (result != OC_STACK_OK)
    {
        std::cout << "Failed to add device type" << std::endl;
        return result;
    }
    result = OCPlatform::setPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DEVICE_NAME, deviceName);
    if (result != OC_STACK_OK)
    {
        std::cout << "Failed to set device name" << std::endl;
        return result;
    }
    result = OCPlatform::setPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_DATA_MODEL_VERSION,
                                          dataModelVersions);
    if (result != OC_STACK_OK)
    {
        std::cout << "Failed to set data model versions" << std::endl;
        return result;
    }
    result = OCPlatform::setPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_SPEC_VERSION, specVersion);
    if (result != OC_STACK_OK)
    {
        std::cout << "Failed to set spec version" << std::endl;
        return result;
    }
    result = OCPlatform::setPropertyValue(PAYLOAD_TYPE_DEVICE, OC_RSRVD_PROTOCOL_INDEPENDENT_ID,
                                          protocolIndependentID);
    if (result != OC_STACK_OK)
    {
        std::cout << "Failed to set piid" << std::endl;
        return result;
    }

    return OC_STACK_OK;
}

#ifdef __unix__
// global needs static, otherwise it can be compiled out and then Ctrl-C does not work
static int quit = 0;
// handler for the signal to stop the application
void handle_signal(int signal)
{
    OC_UNUSED(signal);
    quit = 1;
}
#endif

// main application
// starts the server
int main()
{
    Platform platform;
    OC_VERIFY(platform.start() == OC_STACK_OK);

    std::cout << "/oic/p" << std::endl;
    // initialize "/oic/p"
    if (platform.registerPlatformInfo() != OC_STACK_OK)
    {
        std::cout << "Platform Registration (/oic/p) failed" << std::endl;
    }
    // initialize "/oic/d"
    std::cout << "/oic/d" << std::endl;
    if (platform.setDeviceInfo() != OC_STACK_OK)
    {
        std::cout << "Device Registration (/oic/d) failed" << std::endl;
    }

    std::cout << "device type: " <<  platform.deviceType << std::endl;
    std::cout << "platformID: " <<  platform.getPlatformInfo()->platformID << std::endl;
    std::cout << "platform independent: " <<  platform.protocolIndependentID << std::endl;

#ifdef __unix__
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    std::cout << "Press Ctrl-C to quit...." << std::endl;
    // create the server
    IoTServer server;
    do
    {
        usleep(2000000);
    }
    while (quit != 1);

#endif


#if defined(_WIN32)
    IoTServer server;
    std::cout << "Press Ctrl-C to quit...." << std::endl;
    // we will keep the server alive for at most 30 minutes
    std::this_thread::sleep_for(std::chrono::minutes(30));
    OC_VERIFY(OCPlatform::stop() == OC_STACK_OK);
#endif

    return 0;
}
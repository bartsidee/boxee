/*****************************************************************
|
|      Platinum - AV Media Connect Device
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "Platinum.h"
#include "PltMediaConnect.h"

NPT_SET_LOCAL_LOGGER("platinum.devices.mediaconnect")

/*----------------------------------------------------------------------
|       forward references
+---------------------------------------------------------------------*/
extern NPT_UInt8 X_MS_MediaReceiverRegistrarSCPD[];
extern NPT_UInt8 MS_ContentDirectorySCPD[];

/*----------------------------------------------------------------------
|       PLT_MediaConnect::PLT_MediaConnect
+---------------------------------------------------------------------*/
PLT_MediaConnect::PLT_MediaConnect(const char*  friendly_name, 
                                   bool         add_hostname     /* = true */, 
                                   const char*  uuid             /* = NULL */, 
                                   NPT_UInt16   port             /* = 0 */,
                                   bool         port_rebind      /* = false */) :	
    PLT_MediaServer(friendly_name, false, uuid, port, port_rebind),
    m_RegistrarService(NULL),
    m_AddHostname(add_hostname)
{
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::~PLT_MediaConnect
+---------------------------------------------------------------------*/
PLT_MediaConnect::~PLT_MediaConnect()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::SetupServices()
{
	{
		m_RegistrarService = new PLT_Service(
			this,
			"urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1", 
			"urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar",
			"X_MS_MediaReceiverRegistrar");

		NPT_CHECK_FATAL(m_RegistrarService->SetSCPDXML((const char*) X_MS_MediaReceiverRegistrarSCPD));
		NPT_CHECK_FATAL(AddService(m_RegistrarService));

		m_RegistrarService->SetStateVariable("AuthorizationGrantedUpdateID", "0");
		m_RegistrarService->SetStateVariable("AuthorizationDeniedUpdateID", "0");
		m_RegistrarService->SetStateVariable("ValidationSucceededUpdateID", "0");
		m_RegistrarService->SetStateVariable("ValidationRevokedUpdateID", "0");
	}

    return PLT_MediaServer::SetupServices();
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::ProcessGetDescription
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaConnect::ProcessGetDescription(NPT_HttpRequest&              request,
                                        const NPT_HttpRequestContext& context,
                                        NPT_HttpResponse&             response)
{
	// lock to make sure another request is not modifying the device while we are already
	NPT_AutoLock lock(m_Lock);

	NPT_Result res				   = NPT_SUCCESS;
    NPT_String oldModelName        = m_ModelName;
    NPT_String oldModelNumber      = m_ModelNumber;
    NPT_String oldModelURL         = m_ModelURL;
    NPT_String oldManufacturerURL  = m_ManufacturerURL;
    NPT_String oldDlnaDoc          = m_DlnaDoc;
    NPT_String oldDlnaCap          = m_DlnaCap;
    NPT_String oldAggregationFlags = m_AggregationFlags;
    NPT_String oldFriendlyName     = m_FriendlyName;
    
    NPT_String hostname;
    NPT_System::GetMachineName(hostname);

    if (PLT_HttpHelper::GetDeviceSignature(request) == PLT_XBOX) {
        // XBox needs to see something behind a ':'
        if (m_AddHostname && hostname.GetLength() > 0) {
            m_FriendlyName += ": " + hostname;
        } else if (m_FriendlyName.Find(":") == -1) {
            m_FriendlyName += ": 1";
        }
    } else if (m_AddHostname && hostname.GetLength() > 0) {
        m_FriendlyName += " (" + hostname + ")";
    }

    // change some things based on device signature from request
    if (PLT_HttpHelper::GetDeviceSignature(request) == PLT_XBOX || 
        PLT_HttpHelper::GetDeviceSignature(request) == PLT_WMP) {
        m_ModelName        = "Windows Media Player Sharing";
        m_ModelNumber      = "12.0";
        m_ModelURL         = "http://go.microsoft.com/fwlink/?LinkId=105926";
        m_Manufacturer     = "Microsoft Corporation";
        m_ManufacturerURL  = "http://www.microsoft.com/";
        m_DlnaDoc          = "DMS-1.50";
        m_DlnaCap          = "";
        m_AggregationFlags = "";
        // TODO: http://msdn.microsoft.com/en-us/library/ff362657(PROT.10).aspx
        // TODO: <serialNumber>GUID</serialNumber>

        // return description with modified params
        res = PLT_MediaServer::ProcessGetDescription(request, context, response);
    } else {
        if (PLT_HttpHelper::GetDeviceSignature(request) == PLT_PS3) {
           m_DlnaDoc = "DMS-1.50";
           m_DlnaCap = "av-upload,image-upload,audio-upload";
           m_AggregationFlags = "10";
        }

        // return description with modified params
        res = PLT_MediaServer::ProcessGetDescription(request, context, response);
    }
    
    // reset to old values now
    m_FriendlyName     = oldFriendlyName;
    m_ModelName        = oldModelName;
    m_ModelNumber      = oldModelNumber;
    m_ModelURL         = oldModelURL;
    m_ManufacturerURL  = oldManufacturerURL;
    m_DlnaDoc          = oldDlnaDoc;
    m_DlnaCap          = oldDlnaCap;
    m_AggregationFlags = oldAggregationFlags;
    
    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::ProcessGetSCPD
+---------------------------------------------------------------------*/
NPT_Result 
PLT_MediaConnect::ProcessGetSCPD(PLT_Service*                  service,
                                 NPT_HttpRequest&              request,
                                 const NPT_HttpRequestContext& context,
                                 NPT_HttpResponse&             response)
{
    // Override SCPD response by providing an SCPD without a Search action
    // to all devices except XBox or WMP which need it
    if (service->GetServiceType() == "urn:schemas-upnp-org:service:ContentDirectory:1" &&
        PLT_HttpHelper::GetDeviceSignature(request) != PLT_XBOX &&
        PLT_HttpHelper::GetDeviceSignature(request) != PLT_WMP) {
        NPT_HttpEntity* entity;
        PLT_HttpHelper::SetBody(response, (const char*) MS_ContentDirectorySCPD, &entity);    
        entity->SetContentType("text/xml; charset=\"utf-8\"");
        return NPT_SUCCESS;
    }

    return PLT_MediaServer::ProcessGetSCPD(service, request, context, response);
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::Authorize
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::Authorize(PLT_MediaConnectInfo* info, bool state)
{
    info->m_Authorized = state;
    if (state == true) {
        return m_RegistrarService->IncStateVariable("AuthorizationGrantedUpdateID");
    }

    return m_RegistrarService->IncStateVariable("AuthorizationDeniedUpdateID");
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::Validate
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::Validate(PLT_MediaConnectInfo* info, bool state)
{
    info->m_Validated = state;
    if (state == true) {
        return m_RegistrarService->IncStateVariable("ValidationSucceededUpdateID");
    }

    return m_RegistrarService->IncStateVariable("ValidationRevokedUpdateID");
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnAction(PLT_ActionReference&          action, 
                           const PLT_HttpRequestContext& context)
{
    PLT_MediaConnectInfo* mc_info = NULL;

    /* parse the action name */
    NPT_String name = action->GetActionDesc().GetName();

    /* handle X_MS_MediaReceiverRegistrar actions here */
    if (name.Compare("IsAuthorized") == 0) {
        return OnIsAuthorized(action, mc_info);
    }
    if (name.Compare("RegisterDevice") == 0) {
        return OnRegisterDevice(action, mc_info);
    }
    if (name.Compare("IsValidated") == 0) {
        return OnIsValidated(action, mc_info);
    }  

    return PLT_MediaServer::OnAction(action, context);
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::LookUpMediaConnectInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::LookUpMediaConnectInfo(NPT_String             deviceID, 
                                         PLT_MediaConnectInfo*& mc_info)
{
    mc_info = NULL;

    if (deviceID.GetLength()) {
        /* lookup the MAC from the UDN */
        NPT_String* MAC;
        if (NPT_SUCCEEDED(m_MediaConnectUDNMap.Get(deviceID, MAC))) {
            /* lookup the PLT_MediaConnectInfo from the MAC now */
            return m_MediaConnectDeviceInfoMap.Get(*MAC, mc_info);
        }
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnIsAuthorized
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnIsAuthorized(PLT_ActionReference&  action, 
                                 PLT_MediaConnectInfo* mc_info)
{
    bool authorized = true;

    NPT_String deviceID;
    action->GetArgumentValue("DeviceID", deviceID);

    /* is there a device ID passed ? */
    if (deviceID.GetLength()) {
        /* lookup the MediaConnectInfo from the UDN */
        NPT_String MAC;
        PLT_MediaConnectInfo* device_info;
        if (NPT_FAILED(LookUpMediaConnectInfo(deviceID, device_info))) {
            authorized = false;
        } else {
            authorized = device_info->m_Authorized;
        }
    } else {
        authorized = mc_info?mc_info->m_Authorized:true;
    }

    action->SetArgumentValue("Result", authorized?"1":"0");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnRegisterDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnRegisterDevice(PLT_ActionReference&  action, 
                                   PLT_MediaConnectInfo* mc_info)
{
    NPT_COMPILER_UNUSED(mc_info);

    NPT_String reqMsgBase64;
    action->GetArgumentValue("RegistrationReqMsg", reqMsgBase64);

    NPT_String respMsgBase64;
    action->SetArgumentValue("RegistrationRespMsg", respMsgBase64);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       PLT_MediaConnect::OnIsValidated
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::OnIsValidated(PLT_ActionReference&  action, 
                                PLT_MediaConnectInfo* mc_info)
{
    bool validated = true;

    NPT_String deviceID;
    action->GetArgumentValue("DeviceID", deviceID);

    /* is there a device ID passed ? */
    if (deviceID.GetLength()) {
        /* lookup the MediaConnectInfo from the UDN */
        NPT_String MAC;
        PLT_MediaConnectInfo* device_info;
        if (NPT_FAILED(LookUpMediaConnectInfo(deviceID, device_info))) {
            validated = false;
        } else {
            validated = device_info->m_Validated;
        }
    } else {
        validated = mc_info?mc_info->m_Validated:true;
    }

    action->SetArgumentValue("Result", validated?"1":"0");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaConnect::GetMappedObjectId
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaConnect::GetMappedObjectId(const char* object_id, NPT_String& mapped_object_id) 
{
    if (!object_id) return NPT_ERROR_INVALID_PARAMETERS;
    
    // Reroute XBox 360 and WMP requests to our route
    if (NPT_StringsEqual(object_id, "15")) {
        mapped_object_id = "0/Videos"; // Videos
    } else if (NPT_StringsEqual(object_id, "16")) {
        mapped_object_id = "0/Photos"; // Photos
    } else {
        mapped_object_id = object_id;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaConnectDelegate::GetFilePath
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaConnectDelegate::GetFilePath(const char* object_id, NPT_String& filepath) 
{
    if (!object_id) return NPT_ERROR_INVALID_PARAMETERS;

    // Reroute XBox 360 and WMP requests to our route
    if (NPT_StringsEqual(object_id, "15")) {
        return PLT_FileMediaServerDelegate::GetFilePath("", filepath); // Videos
    } else if (NPT_StringsEqual(object_id, "16")) {
        return PLT_FileMediaServerDelegate::GetFilePath("", filepath); // Photos
    } else if (NPT_StringsEqual(object_id, "13") || NPT_StringsEqual(object_id, "4")) {
        return PLT_FileMediaServerDelegate::GetFilePath("", filepath); // Music
    }

    return PLT_FileMediaServerDelegate::GetFilePath(object_id, filepath);;
}

/*----------------------------------------------------------------------
|   PLT_FileMediaConnectDelegate::OnSearchContainer
+---------------------------------------------------------------------*/
NPT_Result
PLT_FileMediaConnectDelegate::OnSearchContainer(PLT_ActionReference&          action, 
                                                const char*                   object_id, 
                                                const char*                   search_criteria,
                                                const char*                   filter,
                                                NPT_UInt32                    starting_index,
                                                NPT_UInt32                    requested_count,
                                                const char*                   sort_criteria,
                                                const PLT_HttpRequestContext& context)
{
    /* parse search criteria */
    
    /* TODO: HACK TO PASS DLNA */
    if (search_criteria && NPT_StringsEqual(search_criteria, "Unknownfieldname")) {
        /* error */
        NPT_LOG_WARNING_1("Unsupported or invalid search criteria %s", search_criteria);
        action->SetError(708, "Unsupported or invalid search criteria");
        return NPT_FAILURE;
    }
    
    /* locate the file from the object ID */
    NPT_String dir;
    if (NPT_FAILED(GetFilePath(object_id, dir))) {
        /* error */
        NPT_LOG_WARNING("ObjectID not found.");
        action->SetError(710, "No Such Container.");
        return NPT_FAILURE;
    }
    
    /* retrieve the item type */
    NPT_FileInfo info;
    NPT_Result res = NPT_File::GetInfo(dir, &info);
    if (NPT_FAILED(res) || (info.m_Type != NPT_FileInfo::FILE_TYPE_DIRECTORY)) {
        /* error */
        NPT_LOG_WARNING("No such container");
        action->SetError(710, "No such container");
        return NPT_FAILURE;
    }
    
    /* hack for now to return something back to XBox 360 */
    return OnBrowseDirectChildren(action, 
                                  object_id, 
                                  filter, 
                                  starting_index, 
                                  requested_count, 
                                  sort_criteria, 
                                  context);
}

/*****************************************************************
|
|   Platinum - HTTP Client Tasks
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
|   includes
+---------------------------------------------------------------------*/
#include "PltHttpClientTask.h"

NPT_SET_LOCAL_LOGGER("platinum.core.http.clienttask")

/*----------------------------------------------------------------------
|   PLT_HttpTcpConnector::PLT_HttpTcpConnector
+---------------------------------------------------------------------*/
PLT_HttpTcpConnector::PLT_HttpTcpConnector() :
    NPT_HttpClient::Connector(),
    m_Socket(new NPT_TcpClientSocket()),
    m_Valid(false)
{
}

/*----------------------------------------------------------------------
|   PLT_HttpTcpConnector::~PLT_HttpTcpConnector
+---------------------------------------------------------------------*/
PLT_HttpTcpConnector::~PLT_HttpTcpConnector()
{
}

/*----------------------------------------------------------------------
|   PLT_HttpTcpConnector::Connect
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpTcpConnector::Connect(const char*                hostname, 
                              NPT_UInt16                 port, 
                              bool                       secure,
                              NPT_Timeout                connection_timeout,
                              NPT_Timeout                io_timeout,
                              NPT_Timeout                name_resolver_timeout,
                              NPT_InputStreamReference&  input_stream, 
                              NPT_OutputStreamReference& output_stream)
{
    NPT_COMPILER_UNUSED(secure);
    
    /* shortcut to return previous streams if valid and same target */
    if (m_Valid && m_HostName == hostname && m_Port == port) {
        input_stream  = m_InputStream;
        output_stream = m_OutputStream;
        return NPT_SUCCESS;
    }

    /* reset output params first */
    input_stream  = NULL;
    output_stream = NULL;

    // get the address and port to which we need to connect
    NPT_IpAddress address;
    NPT_CHECK_FATAL(address.ResolveName(hostname, name_resolver_timeout));

    // connect to the server
    NPT_LOG_FINER_2("NPT_HttpTcpConnector::Connect - will connect to %s:%d\n", hostname, port);
    m_Socket->SetReadTimeout(io_timeout);
    m_Socket->SetWriteTimeout(io_timeout);

    NPT_SocketAddress socket_address(address, port);
    NPT_CHECK_FATAL(m_Socket->Connect(socket_address, connection_timeout));

    // get and keep the streams
    NPT_CHECK(m_Socket->GetInputStream(m_InputStream));
    NPT_CHECK(m_Socket->GetOutputStream(m_OutputStream));
    NPT_CHECK(m_Socket->GetInfo(m_SocketInfo));

    m_HostName    = hostname;
    m_Port        = port;
    m_Valid       = true;

    input_stream  = m_InputStream;
    output_stream = m_OutputStream;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::PLT_HttpClientSocketTask(NPT_HttpRequest* request /* = NULL */,
                                                   bool             wait_forever /* = false */) :
    m_WaitForever(wait_forever),
    m_Connector(NULL)
{
    if (request) m_Requests.Push(request);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask
+---------------------------------------------------------------------*/
PLT_HttpClientSocketTask::~PLT_HttpClientSocketTask()
{
    // delete any outstanding requests
    NPT_HttpRequest* request;
    while (NPT_SUCCEEDED(m_Requests.Pop(request, false))) {
        delete request;
    }
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::AddRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::AddRequest(NPT_HttpRequest* request)
{
    return m_Requests.Push(request);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::GetNextRequest
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::GetNextRequest(NPT_HttpRequest*& request, NPT_Timeout timeout_ms)
{
    return m_Requests.Pop(request, timeout_ms);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::SetConnector
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::SetConnector(PLT_HttpTcpConnector* connector)
{
    if (IsAborting(0)) return NPT_ERROR_CONNECTION_ABORTED;
    
    // NPT_HttpClient will delete old connector and own the new one
    m_Client.SetConnector(connector);
    m_Connector = connector;
    return NPT_SUCCESS;
}


/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::SetConnector
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::SetConnector()
{
    NPT_AutoLock autolock(m_ConnectorLock);
    return (m_Connector?NPT_SUCCESS:SetConnector(new PLT_HttpTcpConnector()));
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::ResetConnector
+---------------------------------------------------------------------*/
NPT_Result
PLT_HttpClientSocketTask::ResetConnector()
{
    NPT_AutoLock autolock(m_ConnectorLock);
    return SetConnector(NULL);
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::DoAbort
+---------------------------------------------------------------------*/
void
PLT_HttpClientSocketTask::DoAbort()
{
    NPT_AutoLock autolock(m_ConnectorLock);
    if (m_Connector) m_Connector->Abort();
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::DoRun
+---------------------------------------------------------------------*/
void
PLT_HttpClientSocketTask::DoRun()
{
    NPT_HttpRequest*       request = NULL;
    NPT_HttpRequestContext context;
    bool                   using_previous_connector = false;
    NPT_Result             res;
    NPT_HttpResponse*      response = NULL;
    NPT_TimeStamp          watchdog;

    NPT_System::GetCurrentTimeStamp(watchdog);

    do {
        // pop next request or wait for one for 100ms
        while (NPT_SUCCEEDED(GetNextRequest(request, 100))) {
            response = NULL;
retry:
            // if body is not seekable, don't even try to
            // reuse previous connector since in case it fails because
            // server closed connection, we won't be able to
            // rewind the body to resend the request
            if (!PLT_HttpHelper::IsBodyStreamSeekable(*request) && using_previous_connector) {
                using_previous_connector = false;
                NPT_CHECK_LABEL_WARNING(ResetConnector(), abort);
            }

                // assign a new connector if needed
            NPT_CHECK_LABEL_WARNING(SetConnector(), abort);
            
            if (IsAborting(0)) goto abort;

            // send request
            res = m_Client.SendRequest(*request, response);

            // retry only if we were reusing a previous connector
            if (NPT_FAILED(res) && using_previous_connector) {
                using_previous_connector = false;
                NPT_CHECK_LABEL_WARNING(ResetConnector(), abort);

                // server may have closed socket on us
                NPT_HttpEntity* entity = request->GetEntity();
                NPT_InputStreamReference input_stream;

                // rewind request body if any to be able to resend it
                if (entity && NPT_SUCCEEDED(entity->GetInputStream(input_stream)) && !input_stream.IsNull()) {
                    input_stream->Seek(0);
                }

                goto retry;
            }

            NPT_LOG_FINER_1("PLT_HttpClientSocketTask receiving: res = %d", res);
            PLT_LOG_HTTP_MESSAGE(NPT_LOG_LEVEL_FINER, response);

            // callback to process response
            NPT_SocketInfo info;
            m_Connector->GetInfo(info);
            context.SetLocalAddress(info.local_address);
            context.SetRemoteAddress(info.remote_address);
            ProcessResponse(res, *request, context, response);

            // check if server says keep-alive to keep our connector
            if (response && PLT_HttpHelper::IsConnectionKeepAlive(*response)) {
                using_previous_connector = true;
            } else {
                using_previous_connector = false;
                NPT_CHECK_LABEL_WARNING(ResetConnector(), abort);
            }

            // cleanup
            delete response;
            response = NULL;
            delete request;
            request = NULL;
        }

        // DLNA requires that we abort unanswered/unused sockets after 60 secs
        NPT_TimeStamp now;
        NPT_System::GetCurrentTimeStamp(now);
        if (now > watchdog + NPT_TimeInterval(60.)) {    
            using_previous_connector = false;
            NPT_CHECK_LABEL_WARNING(ResetConnector(), abort);
            watchdog = now;
        }

    } while (m_WaitForever && !IsAborting(0));
    
abort:
    if (request) delete request;
    if (response) delete response;
}

/*----------------------------------------------------------------------
|   PLT_HttpClientSocketTask::ProcessResponse
+---------------------------------------------------------------------*/
NPT_Result 
PLT_HttpClientSocketTask::ProcessResponse(NPT_Result                    res, 
                                          const NPT_HttpRequest&        request,  
                                          const NPT_HttpRequestContext& context, 
                                          NPT_HttpResponse*             response) 
{
    NPT_COMPILER_UNUSED(request);
    NPT_COMPILER_UNUSED(context);

    NPT_LOG_FINE_1("PLT_HttpClientSocketTask::ProcessResponse (result=%d)", res);
    NPT_CHECK_WARNING(res);

    NPT_CHECK_POINTER_WARNING(response);

    // check if there's a body to read
    NPT_HttpEntity* entity;
    NPT_InputStreamReference body;
    if (!(entity = response->GetEntity()) || 
        NPT_FAILED(entity->GetInputStream(body)) ||
        body.IsNull()) {
        return NPT_SUCCESS;
    }

    // dump body into ether  
    // (if no content-length specified, read until disconnected)
    NPT_NullOutputStream output;
    NPT_CHECK_SEVERE(NPT_StreamToStreamCopy(*body, 
                                            output,
                                            0, 
                                            entity->GetContentLength()));

    return NPT_SUCCESS;
}

#!/usr/bin/env python

# Copyright (C) 2005-2007, Glenn Washburn
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
#
# You must obey the GNU General Public License in all respects for
# all of the code used other than OpenSSL.  If you modify file(s)
# with this exception, you may extend this exception to your version
# of the file(s), but you are not obligated to do so.  If you do not
# wish to do so, delete this exception statement from your version.
# If you delete this exception statement from all source files in the
# program, then also delete it here.
#
# Contact:  Glenn Washburn <crass@berlios.de>

import sys, cStringIO as StringIO
import xmlrpclib, urllib, urlparse, socket

# this allows us to parse scgi urls just like http ones
from urlparse import uses_netloc
uses_netloc.append('scgi')

def encode_netstring(string):
    "Encode string as netstring"
    return '%d:%s,'%(len(string), string)

def make_headers(headers):
    "Make scgi header list"
    return '\x00'.join(headers)+'\x00'

def convert_xmlrpc2sgi_req(xmlreq):
    "Wrap xmlrpc request in an scgi request,\nsee spec at: http://python.ca/scgi/protocol.txt"
    # See spec at: http://python.ca/scgi/protocol.txt
    headers = make_headers([
        'CONTENT_LENGTH', str(len(xmlreq)),
        'SCGI', '1',
    ])
    
    enc_headers = encode_netstring(headers)
    
    return enc_headers+xmlreq

def send_scgi(url, scgireq):
    "Send request and get response from url"
    scheme, netloc, path, query, frag = urlparse.urlsplit(url)
    host, port = urllib.splitport(netloc)
    #~ print '>>>', (netloc, host, port)
    
    addrinfo = socket.getaddrinfo(host, port, socket.AF_INET, socket.SOCK_STREAM)
    
    assert len(addrinfo) == 1, "There's more than one? %r"%addrinfo
    #~ print addrinfo
    
    sock = socket.socket(*addrinfo[0][:3])
    
    sock.connect(addrinfo[0][4])
    sock.send(scgireq)
    recvdata = resp = sock.recv(1024)
    while recvdata != '':
        recvdata = sock.recv(1024)
        #~ print 'Trying to receive more: %r'%recvdata
        resp += recvdata
    sock.close()
    return resp

def gen_headers(file):
    "Get header lines from scgi response"
    line = file.readline().rstrip()
    while line.strip():
        yield line
        line = file.readline().rstrip()

def get_scgi_resp(resp):
    "Get xmlrpc response from scgi response"
    fresp = StringIO.StringIO(resp)
    for line in gen_headers(fresp):
        #~ print "Header: %s"%line
        pass
    
    xmlresp = fresp.read()
    return xmlresp

def convert_params_to_native(params):
    "Parse xmlrpc-c command line arg syntax"
    #~ print 'convert_params_to_native', params
    cparams = []
    # parse parameters
    for param in params:
        if param[1] != '/':
            cparams.append(param)
            continue
        
        if param[0] == 'i':
            ptype = int
        elif param[0] == 'b':
            ptype = bool
        elif param[0] == 's':
            ptype = str
        
        
        cparams.append(ptype(param[2:]))
    
    return tuple(cparams)

def do_scgi_xmlrpc_request(host, methodname, params=()):
    """
        Send an xmlrpc request over scgi to host.
        host: scgi://host:port/path
        methodname: xmlrpc method name
        params: tuple of simple python objects
        returns xmlrpc response
    """
    xmlreq = xmlrpclib.dumps(params, methodname)
    
    scgireq = convert_xmlrpc2sgi_req(xmlreq)
    
    #~ print xmlreq, params
    #~ print repr(scgireq)
    #~ sys.stdout.write(scgireq)
    
    resp = send_scgi(host, scgireq)
    #~ print resp
    
    respxml = get_scgi_resp(resp)
    #~ print respxml
    
    return respxml

def do_scgi_xmlrpc_request_py(host, methodname, params=()):
    """
        Send an xmlrpc request over scgi to host.
        host: scgi://host:port/path
        methodname: xmlrpc method name
        params: tuple of simple python objects
        returns xmlrpc response converted to python
    """
    xmlresp = do_scgi_xmlrpc_request(host, methodname, params)
    return xmlrpclib.loads(xmlresp)[0][0]

class RTorrentXMLRPCClient(object):
    """
    The following is an exmple of how to use this class.
    rtorrent_host='http://localhost:33000'
    rtc = RTorrentXMLRPCClient(rtorrent_host)
    for infohash in rtc.download_list('complete'):
        if rtc.d.get_ratio(infohash) > 500:
            print "%s has a ratio of over 0.5"%(rtc.d.get_name(infohash))
    """
    
    def __init__(self, url, methodname=''):
        self.url = url
        self.methodname = methodname
    
    def __call__(self, *args):
        #~ print "%s%r"%(self.methodname, args)
        return do_scgi_xmlrpc_request_py(self.url, self.methodname, args)
    
    def __getattr__(self, attr):
        methodname = self.methodname and '.'.join([self.methodname,attr]) or attr
        return RTorrentXMLRPCClient(self.url, methodname)


def main(argv):
    output_python=False
    
    if argv[0] == '-p':
        output_python=True
        argv.pop(0)
    
    host, methodname = argv[:2]
    
    respxml = do_scgi_xmlrpc_request(host, methodname,
                                    convert_params_to_native(argv[2:]))
    
    if not output_python:
        print respxml
    else:
        print xmlrpclib.loads(respxml)[0][0]

if __name__ == "__main__":
    main(sys.argv[1:])


#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Copyright (c) 2010-2019 The Freicoin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the conjunctive terms of BOTH version 3 of the GNU Affero
# General Public License as published by the Free Software Foundation
# AND the MIT/X11 software license.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License and the MIT/X11 software license for
# more details.
#
# You should have received a copy of both licenses along with this
# program.  If not, see <https://www.gnu.org/licenses/> and
# <http://www.opensource.org/licenses/mit-license.php>

#
# Test REST interface
#

from test_framework.test_framework import FreicoinTestFramework
from test_framework.util import *
import base64

try:
    import http.client as httplib
except ImportError:
    import httplib
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

class HTTPBasicsTest (FreicoinTestFramework):        
    def setup_nodes(self):
        return start_nodes(4, self.options.tmpdir, extra_args=[['-rpckeepalive=1'], ['-rpckeepalive=0'], [], []])

    def run_test(self):        
        
        #################################################
        # lowlevel check for http persistent connection #
        #################################################
        url = urlparse.urlparse(self.nodes[0].url)
        authpair = url.username + ':' + url.password
        headers = {"Authorization": "Basic " + base64.b64encode(authpair)}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        
        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out2 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True) #must also response with a correct json-rpc message
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        conn.close()
        
        #same should be if we add keep-alive because this should be the std. behaviour
        headers = {"Authorization": "Basic " + base64.b64encode(authpair), "Connection": "keep-alive"}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        
        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out2 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True) #must also response with a correct json-rpc message
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        conn.close()
        
        #now do the same with "Connection: close"
        headers = {"Authorization": "Basic " + base64.b64encode(authpair), "Connection":"close"}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, False) #now the connection must be closed after the response        
        
        #node1 (2nd node) is running with disabled keep-alive option
        urlNode1 = urlparse.urlparse(self.nodes[1].url)
        authpair = urlNode1.username + ':' + urlNode1.password
        headers = {"Authorization": "Basic " + base64.b64encode(authpair)}
                
        conn = httplib.HTTPConnection(urlNode1.hostname, urlNode1.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, False) #connection must be closed because keep-alive was set to false
        
        #node2 (third node) is running with standard keep-alive parameters which means keep-alive is off
        urlNode2 = urlparse.urlparse(self.nodes[2].url)
        authpair = urlNode2.username + ':' + urlNode2.password
        headers = {"Authorization": "Basic " + base64.b64encode(authpair)}
                
        conn = httplib.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, True) #connection must be closed because freicoind should use keep-alive by default
        
if __name__ == '__main__':
    HTTPBasicsTest ().main ()

# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

import sys
from test import test_support
import _testcapi

def test_main():

    for name in dir(_testcapi):
        if name.startswith('test_'):
            test = getattr(_testcapi, name)
            if test_support.verbose:
                print "internal", name
            try:
                test()
            except _testcapi.error:
                raise test_support.TestFailed, sys.exc_info()[1]

    # some extra thread-state tests driven via _testcapi
    def TestThreadState():
        import thread
        import time

        if test_support.verbose:
            print "auto-thread-state"

        idents = []

        def callback():
            idents.append(thread.get_ident())

        _testcapi._test_thread_state(callback)
        a = b = callback
        time.sleep(1)
        # Check our main thread is in the list exactly 3 times.
        if idents.count(thread.get_ident()) != 3:
            raise test_support.TestFailed, \
                  "Couldn't find main thread correctly in the list"

    def TestPendingCalls_Submit(l, n):
        def callback():
            l[0] += 1
            
        _testcapi._pending_threadfunc(callback, n);
        
    def TestPendingCalls_Wait(l, n):
        #now, stick around until l[0] has grown to 10
        count = 0;
        while l[0] != n:
            #this busy loop is where we expect to be interrupted to
            #run our callbacks.  Note that callbacks are only run on the
            #main thread
            if test_support.verbose:
                print "(%i)"%(l[0],),
            for i in xrange(1000):
                a = i*i
            count += 1
            if count > 10000:
                raise test_support.TestFailed, \
                    "timeout waiting for %i callbacks, got %i"%(n, count)
        if test_support.verbose:
            print "(%i)"%(l[0],)

    def TestPendingCalls():
        if test_support.verbose:
            print "pending-calls"
        l = [0]
        n = 5
        import threading
        t = threading.Thread(target=TestPendingCalls_Submit, args = (l, n))
        t.start()
        TestPendingCalls_Wait(l, n)
        t.join()
        
        #again, just using the main thread, likely they will all be dispathced at
        #once.  We must be careful not to ask for too many.  The queue can handle
        #at most 32 and since we are on a single thread, we would wait forever
        #for the queue to become free
        l[0] = 0
        TestPendingCalls_Submit(l, n)
        TestPendingCalls_Wait(l, n)
        
    try:
        _testcapi._test_thread_state
        have_thread_state = True
    except AttributeError:
        have_thread_state = False

    if have_thread_state:
        TestThreadState()
        import threading
        t=threading.Thread(target=TestThreadState)
        t.start()
        t.join()

if __name__ == "__main__":
    test_main()

from boxee import *;
import time;

if __name__ == "__main__":
	Boxee.GetInstance().Start()
	bx_handle = Boxee.GetInstance()
	time.sleep(2);
	feed = BXBoxeeFeed()

	print "getting boxee feed..."
	if bx_handle.GetBoxeeFeed(feed):
		print "boxee feed: timestamp - " + str(feed.GetTimeStamp())
		print "there are " + str(feed.GetNumOfActions()) + " actions in feed";
		
		for i in range(0,feed.GetNumOfActions(),1):
			print "action " + str(i) + " : " + feed.GetAction(i).GetMessageType() 
			print "   desc: " + feed.GetAction(i).GetValue("description")
			print "   objects: " + str(feed.GetAction(i).GetObjectCount())
			for o in range(0,feed.GetAction(i).GetObjectCount()):
				obj = feed.GetAction(i).GetObject(o)
				print "       object type: " + obj.GetType() + " name: " + obj.GetName() + " id: "+obj.GetID()

		print "testing object retrieve by type: "
		obj = feed.GetAction(0).GetObject("movie")
		if obj.IsValid() :
			print "got movie object. name: " + obj.GetName() 
		else:
			print "failed to get movie object" 

	else:
		print "failed to get boxee feed!"

	print "getting friends list..."
	list = BXFriendsList()
	if bx_handle.RetrieveFriendsList(list):
		print "friends list retrieved. total of: " + str(list.GetCount()) + " friends."
		iter = list.Iterate()
		while not iter.IsEndOfList() :
			friend = iter.GetFriend()
			print friend.GetFriendId() + " (" + friend.GetAttribute("display_name") + ") : "
			print "presence: " + friend.GetAttribute("presence")
				
			actions = friend.GetActionCount();
			for a in range(0,actions,1):
				action = friend.GetAction(a)
				print "action " + str(a) + " : " + action.GetMessageType() 
				print "   desc: " + action.GetValue("description")
				print "   objects: " + str(action.GetObjectCount())
				for o in range(0,action.GetObjectCount()):
					obj = action.GetObject(o)
					print "       object type: " + obj.GetType() + " name: " + obj.GetName() + " id: "+obj.GetID()

			iter.Next()

	

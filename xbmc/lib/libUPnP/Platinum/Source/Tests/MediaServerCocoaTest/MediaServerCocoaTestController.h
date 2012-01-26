//
//  MediaServerCocoaTestController.h
//  Platinum
//
//  Created by Sylvain on 9/14/10.
//  Copyright 2010 Plutinosoft LLC. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PltUPnPObject.h"
#import "PltMediaServerObject.h"

@interface MediaServerCocoaTestController : NSObject <PLT_MediaServerDelegateObject> {
    IBOutlet NSWindow*	window;
    IBOutlet NSButton*  mainButton;
    
    PLT_UPnPObject*     upnp;
}

@end

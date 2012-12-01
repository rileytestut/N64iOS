//
//  N64AppDelegate.h
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "../../src/SDL/src/video/uikit/SDL_uikitappdelegate.h"
//#import "../../src/SDL/src/video/uikit/SDL_uikitopenglview.h"
//#import "../../src/SDL/src/events/SDL_events_c.h"
//#import "../../src/SDL/src/video/uikit/jumphack.h"

@interface N64AppDelegate : UIResponder <UIApplicationDelegate> {
    unsigned int windowID;
}

@property (strong, nonatomic) UIWindow *window;
@property (nonatomic) unsigned int windowID;

@end

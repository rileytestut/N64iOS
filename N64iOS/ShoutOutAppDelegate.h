//
//  n64ios
//
//  Created by Spookysoft on 9/6/08.
//  Copyright Spookysoft 2008. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "BTInquiryViewController.h"
#import "SDL_uikitappdelegate.h"
#import "SDL_uikitopenglview.h"
#import "SDL_events_c.h"
#import "jumphack.h"
#ifdef WITH_ADS
#import "AltAds.h"
#endif

#define NUMBER_OF_AD_PAGES 1

@interface ShoutOutAppDelegate : NSObject <UIApplicationDelegate, BTInquiryDelegate> {
	
	IBOutlet UIWindow*					window;
	IBOutlet UINavigationController*	navigationController;
	IBOutlet UITabBar*					tabBar;
#ifdef WITH_ADS
	AltAds*	 altAds[NUMBER_OF_AD_PAGES];
#endif
	BTInquiryViewController *inqViewControl;
  SDL_WindowID windowID;
}

+ (ShoutOutAppDelegate *)sharedAppDelegate;
- (void)switchToBrowse;
- (void)switchToSaveStates;
//- (void)switchToBookmarks;
- (void)switchToNowPlaying;
- (void)switchToRecent;
- (void)switchToOptions;
- (void)switchToWebBrowserView;
- (void)reloadROMs;
#ifdef WITH_ADS
- (AltAds*)getAdViewWithIndex:(int)index;
- (void)pauseAdViews;
- (void)unpauseAdViews;
#endif

@property (readwrite, assign) SDL_WindowID windowID;
@property (nonatomic, retain) UIWindow *window;
@property (nonatomic, retain) UINavigationController *navigationController;
@property (nonatomic, retain) BTInquiryViewController *inqViewControl;

@end


#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import "Frameworks/CoreSurface.h"
#import "helpers.h"
#ifdef WITH_ADS
#import "AltAds.h"
#endif
#import "TabBar.h"
#import "RomController.h"
#import "ShoutOutAppDelegate.h"
#import "NowPlayingController.h"
#import "SaveStatesController.h"
#import "RecentController.h"
#import "OptionsController.h"
#import "WebBrowserViewController.h"

#define SOApp ((SOApplication *)[UIApplication sharedApplication])
@interface SOApplication : UIApplication 
{
	IBOutlet	RomController             * romView;
	IBOutlet	NowPlayingController		  * nowPlayingView;
	IBOutlet	SaveStatesController		  * saveStatesView;
	IBOutlet	RecentController          * recentView;
	IBOutlet	OptionsController         * optionsView;
	IBOutlet	WebBrowserViewController  * webBrowserView;
}

@property(assign)	RomController             * romView;
@property(assign)	NowPlayingController	    * nowPlayingView;
@property(assign)	SaveStatesController	    * saveStatesView;
@property(assign)	RecentController		    	* recentView;
@property(assign)	OptionsController		    	* optionsView;
@property(assign)	WebBrowserViewController  * webBrowserView;

@end

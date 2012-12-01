//
//  NowPlayingController.h
//  ShoutOut
//
//  Created by ME on 9/13/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import <UIKit/UIView-Geometry.h>
//#import <GraphicsServices/GraphicsServices.h>
#import <Foundation/Foundation.h>
#import "Frameworks/CoreSurface.h"
#import <QuartzCore/CALayer.h>

#import <pthread.h>
#import <sched.h>
#import <unistd.h>
#import <sys/time.h>

struct myGSPathPoint {
	char unk0;
	char unk1;
	char unk2;
	char unk3;
	char unk4;
	char unk5;
	char status;
	char unk7;
	char unk8;
	char unk9;
	char unk10;
	char unk11;
	char unk12;
	char unk13;
	char unk14;
	char unk15;
	float x;
	float y;
};

typedef struct {
	char unk0;
	char unk1;
	char unk2;
	char unk3;
	char unk4;
	char unk5;
	char unk6;
	char unk7;
	
	char unk8;
	char unk9;
	char unk10;
	char unk11;
	char unk12;
	char unk13;
	char unk14;
	char unk15;
	
	char unk16;
	char unk17;
	char unk18;
	char unk19;
	char unk20;
	char unk21;
	char unk22;
	char unk23;
	
	char unk24;
	char unk25;
	char unk26;
	char unk27;
	char unk28;
	char unk29;
	char unk30;
	char unk31;
	
	char unk32;
	char unk33;
	char unk34;
	char unk35;
	char unk36;
	char unk37;
	char unk38;
	char unk39;
	
	char unk40;
	char unk41;
	char unk42;
	char unk43;
	char unk44;
	char unk45;
	char unk46;
	char unk47;
	
	char unk48;
	char unk49;
	char unk50;
	char unk51;
	char unk52;
	char unk53;
	char unk54;
	char unk55;
	
	char type;
	char unk57;
	char unk58;
	char unk59;
	char unk60;
	char unk61;
	char fingerCount;
	char unk62;	
	char unk63;
	char unk64;
	char unk65;
	char unk66;
	char unk67;
	char unk68;
	char unk69;
	char unk70;
	char unk71;
	char unk72;
	char unk73;
	char unk74;
	char unk75;
	char unk76;
	char unk77;
	char unk78;
	char unk79;
	char unk80;
	char unk81;
	char unk82;
	char unk83;
	char unk84;
	char unk85;
	char unk86;
	struct myGSPathPoint points[100];
} myGSEvent;

@interface ScreenView : UIView < UIAlertViewDelegate >
{
	CoreSurfaceBufferRef			_screenSurface;
	CALayer						*	screenLayer;
	NSTimer						*	timer;
}

- (id)initWithFrame:(CGRect)frame;
- (void)drawRect:(CGRect)rect;
- (CoreSurfaceBufferRef)getSurface;
- (void)npUpdateScreen;
- (void)dummy;
- (void)setTrialAlert;

@end

@interface NowPlayingController : UIViewController < UIAlertViewDelegate, UIActionSheetDelegate > 
{
	//IBOutlet	UISlider		* volumeSlider;
	IBOutlet	UIView			* screenView;
	IBOutlet	UITabBar		* tabBar;
	IBOutlet	UIWindow		* window;
	IBOutlet	UIButton			* ButtonL1;
	IBOutlet	UIButton			* ButtonL2;
	IBOutlet	UIButton			* ButtonR1;
	IBOutlet	UIButton			* ButtonR2;
	IBOutlet	UIButton			* ButtonUP;
	IBOutlet	UIButton			* ButtonDOWN;
	IBOutlet	UIButton			* ButtonLEFT;
	IBOutlet	UIButton			* ButtonRIGHT;
	IBOutlet	UIButton			* ButtonSTART;
	IBOutlet	UIButton			* ButtonSELECT;
	IBOutlet	UIButton			* ButtonUPLEFT;
	IBOutlet	UIButton			* ButtonUPRIGHT;
	IBOutlet	UIButton			* ButtonDOWNLEFT;
	IBOutlet	UIButton			* ButtonDOWNRIGHT;
	//IBOutlet	UIButton			* ButtonSAVE;
	//IBOutlet	UIButton			* ButtonBOOKMARK;
	IBOutlet	UIButton			* ButtonTRIANGLE;
	IBOutlet	UIButton			* ButtonSQUARE;
	IBOutlet	UIButton			* ButtonCIRCLE;
	IBOutlet	UIButton			* ButtonCROSS;
	IBOutlet	UIButton			* ButtonEXIT;
	NSString					* currentId;
	NSString					* currentTunein;
	NSString					* currentTitle;
	NSString					* currentPath;
	NSString					* currentFile;
	NSString					* currentDir;
	//ScreenView					* screenView;
	UIImageView					* controllerImageView;
  CGRect A;
  CGRect B;
  CGRect AB;
  CGRect Up;
  CGRect Left;
  CGRect Down;
  CGRect Right;
  CGRect UpLeft;
  CGRect DownLeft;
  CGRect UpRight;
  CGRect DownRight;
  CGRect Select;
  CGRect Start;
  CGRect LPad;
  CGRect RPad;
  CGRect LPad2;
  CGRect RPad2;
  CGRect Menu;
  CGRect ButtonUp;
  CGRect ButtonLeft;
  CGRect ButtonDown;
  CGRect ButtonRight;
  CGRect ButtonUpLeft;
  CGRect ButtonDownLeft;
  CGRect ButtonUpRight;
  CGRect ButtonDownRight;
	
	CGPoint fingers[5];
    int numFingers;
	
	CoreSurfaceBufferRef			_screenSurface;
	CALayer						*	screenLayer;
	NSTimer						*	timer;	
}

- (void)getControllerCoords:(int)orientation;
- (void)fixRects;
- (void)volumeChanged:(id)sender;
//- (void)setBookmark:(id)sender;
- (void)setSaveState:(id)sender;
- (void)startEmu:(char*)path;
- (void)runSound;
- (void)runProgram;
- (void)runMenu;
- (void)runMainMenu;
- (void)showTrialAlert;

@end

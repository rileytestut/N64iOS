//
//  NowPlayingController.m
//  ShoutOut
//
//  Created by ME on 9/13/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SOApplication.h"
#import <pthread.h>

#include <mach/mach_port.h>

#import "Frameworks/CoreSurface.h"
#import "Frameworks/IOKit/IOTypes.h"
#import "Frameworks/IOKit/IOKitLib.h"

#include <stdio.h> // For mprotect
#include <sys/mman.h>

extern unsigned long gp2x_timer_read(void);

extern unsigned long gp2x_pad_status;

#define IPHONE_MENU_DISABLED            0
#define IPHONE_MENU_MAIN                1
#define IPHONE_MENU_SAVE_CURRENT        2
#define IPHONE_MENU_SAVE_NEW            3
#define IPHONE_MENU_QUIT_LOAD           4
#define IPHONE_MENU_QUIT                5
#define IPHONE_MENU_MAIN_LOAD           6


char showfps[2][64];
char* ourArgs[32];
char ourArgsParam[32][1024];
int ourArgsCnt = 0;
unsigned short* screenbuffer;
int iphone_touches = 0;
int iphone_menu = IPHONE_MENU_DISABLED;
long long iphone_last_upd_ticks = 0;
int iphone_controller_opacity = 100;
int iphone_is_landscape = 0;
int iphone_soundon = 0;
pthread_t sound_tid;
static unsigned long newtouches[10];
static unsigned long oldtouches[10];
unsigned long gp2x_pad_status = 0;
long gp2x_pad_x_axis = 0;
long gp2x_pad_y_axis = 0;
long gp2x_pad_z_axis = 0;

enum  { GP2X_UP=0x1,       GP2X_LEFT=0x4,       GP2X_DOWN=0x10,  GP2X_RIGHT=0x40,
	      GP2X_START=1<<8,   GP2X_SELECT=1<<9,    GP2X_L=1<<10,    GP2X_R=1<<11,
	      GP2X_A=1<<12,      GP2X_B=1<<13,        GP2X_X=1<<14,    GP2X_Y=1<<15,
        GP2X_VOL_UP=1<<23, GP2X_VOL_DOWN=1<<22, GP2X_PUSH=1<<27 };

extern unsigned long btUsed;      
extern unsigned long gp2x_fps_debug;
extern float __audioVolume;
extern volatile unsigned short BaseAddress[240*160];
extern volatile int __emulation_run;
extern volatile int __emulation_saving;
extern volatile int __emulation_paused;
extern int tArgc;
extern char** tArgv;
extern pthread_t main_tid;
extern pthread_attr_t main_attr;
extern unsigned char gamepak_filename[512];
extern char test_print_buffer[128];
extern unsigned short* videobuffer;
extern unsigned char *vrambuffer;

extern void save_state(char *savestate_filename, unsigned short *screen_capture);
extern void set_save_state(void);
extern int SDL_main(int argc, char *argv[]);

static ScreenView *sharedInstance = nil;

#if 0
void npUpdateScreen()
{
  //usleep(100);
  //sched_yield();
	[sharedInstance performSelectorOnMainThread:@selector(setNeedsDisplay) withObject:nil waitUntilDone:NO];
/*
	if(screenbuffer && vrambuffer)
	{
	  //unsigned long Timer=gp2x_timer_read();
    unsigned long* screen = screenbuffer;
    unsigned short* vram = vrambuffer;
    int x, y;
    screen += ((320 - 256) / 2);
    for(y = 0; y < 224; y++)
    {
      for(x = 0; x < 256; x++)
      {
        unsigned long pixel = *vram++;
        screen[x] = (255 << 24) | ((pixel & 0xF800) << 8) | ((pixel & 0x7E0) << 5) | ((pixel & 0x1F) << 3);
      }
      screen += 320;
		}
    //fprintf(stderr, "Diff: %d\n", gp2x_timer_read() - Timer);
	}
*/
}
#endif

void* app_Thread_Start(void* args)
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	__emulation_run = 1;
	SDL_main(ourArgsCnt, ourArgs);
	__emulation_run = 0;
	__emulation_saving = 0;
  [pool release];

	return NULL;
}

@implementation ScreenView
- (id)initWithFrame:(CGRect)frame {
	if ((self = [super initWithFrame:frame])!=nil) 
	{
  		CFMutableDictionaryRef dict;
  		int w = 320; //rect.size.width;
  		int h = 240; //rect.size.height;

    	int pitch = w * 2, allocSize = 2 * w * h;
    	char *pixelFormat = "565L";

  		self.opaque = YES;
  		self.clearsContextBeforeDrawing = NO;
  		self.userInteractionEnabled = NO;
  		self.multipleTouchEnabled = NO;
  		self.contentMode = UIViewContentModeTopLeft;

      if([SOApp.optionsView getCurrentSmoothScaling])
      {
    	  [[self layer] setMagnificationFilter:kCAFilterLinear];
    	  [[self layer] setMinificationFilter:kCAFilterLinear];      
      }
      else
      {
    	  [[self layer] setMagnificationFilter:kCAFilterNearest];
    	  [[self layer] setMinificationFilter:kCAFilterNearest];
  	  }
  		dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
  										 &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  		CFDictionarySetValue(dict, kCoreSurfaceBufferGlobal, kCFBooleanTrue);
  		CFDictionarySetValue(dict, kCoreSurfaceBufferMemoryRegion,
  							 @"IOSurfaceMemoryRegion");
  		CFDictionarySetValue(dict, kCoreSurfaceBufferPitch,
  							 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pitch));
  		CFDictionarySetValue(dict, kCoreSurfaceBufferWidth,
  							 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &w));
  		CFDictionarySetValue(dict, kCoreSurfaceBufferHeight,
  							 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &h));
  		CFDictionarySetValue(dict, kCoreSurfaceBufferPixelFormat,
  							 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, pixelFormat));
  		CFDictionarySetValue(dict, kCoreSurfaceBufferAllocSize,
  							 CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &allocSize));

  		_screenSurface = CoreSurfaceBufferCreate(dict);
  		CoreSurfaceBufferLock(_screenSurface, 3);

      	//CALayer* screenLayer = [CALayer layer];  
  		screenLayer = [[CALayer layer] retain];
  		/*
  		CGAffineTransform affineTransform = CGAffineTransformIdentity;
  		affineTransform = CGAffineTransformConcat( affineTransform, CGAffineTransformMakeRotation(90));
  		self.transform = affineTransform;
  		*/
  		if(!iphone_is_landscape)
  		{
  			if([SOApp.optionsView getCurrentScaling])
  			{
  				screenLayer.frame = CGRectMake(0.0f, 0.0f, 320.0f, 238.0f);
  				[ screenLayer setOpaque: YES ];
  			}
  			else
  			{
  				screenLayer.frame = CGRectMake(0.0f, 0.0f, 320.0f, 238.0f);
  				[ screenLayer setOpaque: YES ];				
  			}
  		}
  		else
  		{
  			if([SOApp.optionsView getCurrentScaling])
  			{
  				CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI / 2.0f); // = CGAffineTransformMakeTranslation(1.0, 1.0);
  				[screenLayer setAffineTransform:transform];
  				screenLayer.frame = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
  				//[screenLayer setCenter:CGPointMake(240.0f,160.0f)];
  				[ screenLayer setOpaque:YES ];
  			}
  			else
  			{
  				CGAffineTransform transform = CGAffineTransformMakeRotation(M_PI / 2.0f); // = CGAffineTransformMakeTranslation(1.0, 1.0);
  				[screenLayer setAffineTransform:transform];
  				screenLayer.frame = CGRectMake(20.0f, 80.0f, 238.0f, 320.0f);
  				//[screenLayer setCenter:CGPointMake(240.0f,160.0f)];
  				[ screenLayer setOpaque:YES ];				
  			}
  		}
      if([SOApp.optionsView getCurrentSmoothScaling])
      {
    	  [screenLayer setMagnificationFilter:kCAFilterLinear];
    	  [screenLayer setMinificationFilter:kCAFilterLinear];      
      }
      else
      {
    	  [screenLayer setMagnificationFilter:kCAFilterNearest];
    	  [screenLayer setMinificationFilter:kCAFilterNearest];
  	  }
  		screenLayer.contents = (id)_screenSurface;
  		[[self layer] addSublayer:screenLayer];

  		/*
  		 screenLayer = [CALayer layer];
  		 screenLayer.doubleSided = NO;
  		 screenLayer.bounds = rect;
  		 screenLayer.contents = (id)_screenSurface;
  		 screenLayer.anchorPoint = CGPointMake(0, 0); // set anchor point to top-left
  		 [self.layer addSublayer: screenLayer];
  		 */
      	CoreSurfaceBufferUnlock(_screenSurface);

  		screenbuffer = CoreSurfaceBufferGetBaseAddress(_screenSurface);
  		//[NSThread setThreadPriority:0.0];
  		//[NSThread detachNewThreadSelector:@selector(npUpdateScreen) toTarget:self withObject:nil];

      iphone_last_upd_ticks = 0;
  		/*
  		timer = [NSTimer scheduledTimerWithTimeInterval:(1.0f / 50.0f)
  												 target:self
  											   selector:@selector(setNeedsDisplay)
  											   userInfo:nil
  												repeats:YES];
  		*/
  	}

  	sharedInstance = self;

  	return self;
}

- (void)dealloc
{
	//[timer invalidate];
	[ screenLayer release ];
	[ super dealloc ];
}

- (CoreSurfaceBufferRef)getSurface
{
	return _screenSurface;
}


- (void)drawRect:(CGRect)rect
{
/*  if(screenbuffer && vrambuffer)
	{
    memcpy(screenbuffer, vrambuffer, 256*224*2);
	}
*/
}

- (void)dummy
{
	
}

- (void)npUpdateScreen
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  //[NSThread setThreadPriority:0.9];
/*
	struct sched_param    param;
  param.sched_priority = 46;
  if(pthread_setschedparam(pthread_self(), SCHED_RR, &param) != 0)
  {
    fprintf(stderr, "Error setting pthread priority\n");
  }
*/
/*
	struct sched_param    param;
  param.sched_priority = -47;
  if(pthread_setschedparam(pthread_self(), SCHED_OTHER, &param) != 0)
  {
    fprintf(stderr, "Error setting pthread priority\n");
  }
*/
	while(__emulation_run)
	{
	  [self performSelectorOnMainThread:@selector(setNeedsDisplay) withObject:nil waitUntilDone:YES];
    //usleep(16666);
    //sched_yield();
	}
  [pool release];
}

-(void)setTrialAlert
{
  [SOApp.nowPlayingView showTrialAlert];
}

@end

@implementation NowPlayingController

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
	// the user clicked one of the OK/Cancel buttons
	if(__emulation_run)
	{
	  if(iphone_menu == IPHONE_MENU_MAIN)
	  {
	    if(buttonIndex == 0)
	    {
        	iphone_menu = IPHONE_MENU_SAVE_CURRENT;
	    }
	    else if(buttonIndex == 1)
	    {
        	iphone_menu = IPHONE_MENU_SAVE_NEW;
	    }
	    else if(buttonIndex == 2)
	    {
      	  	iphone_menu = IPHONE_MENU_QUIT_LOAD;
	    }
	    else
	    {
       	 	iphone_menu = IPHONE_MENU_DISABLED;
	    }
	  }
	  
	  if(iphone_menu == IPHONE_MENU_QUIT_LOAD)
	  {
      		iphone_menu = IPHONE_MENU_QUIT;
			UIActionSheet *alert = [[UIActionSheet alloc] initWithTitle:@"Save your state before quitting? You can overwrite a currently loaded savestate, save to a new file, or cancel." delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:@"Save [Currently Loaded] State",@"Save State To New File",@"Don't Save",@"Cancel", nil];
			[alert showInView:self.view];
			[alert release];
      	  	return;
	  }

	  if(iphone_menu == IPHONE_MENU_QUIT)
	  {			
	   	if (buttonIndex == 0)
  		{
  		  	[SOApp.delegate switchToBrowse];
  			[tabBar didMoveToWindow];
  			__emulation_saving = 2;
  			__emulation_run = 0;
  			while(__emulation_saving)
  			{
         	   usleep(16666);
          		//sched_yield();
        	}
  			//pthread_join(main_tid, NULL);
  			//[screenView removeFromSuperview];
  			//[screenView release];
  			if(!btUsed || !iphone_is_landscape)
            {
  			  [controllerImageView removeFromSuperview];
  			  [controllerImageView release];
  			}
  			__emulation_run = 0;
  			__emulation_saving = 0;	
#ifdef WITH_ADS
  			[SOApp.delegate unpauseAdViews];		
#endif
       	 	[SOApp.saveStatesView startSaveData:0.1f];	
  			[SOApp.delegate switchToSaveStates];
  			[tabBar didMoveToWindowSaveStates];
  		}
  		else if (buttonIndex == 1)
  		{
  		  [SOApp.delegate switchToBrowse];
  			[tabBar didMoveToWindow];
  			
  			__emulation_saving = 1;
  			__emulation_run = 0;
  			while(__emulation_saving)
  			{
          usleep(1000000);
          sched_yield();
        }
  			//pthread_join(main_tid, NULL);
  			//[screenView removeFromSuperview];
  			//[screenView release];
  			if(!btUsed || !iphone_is_landscape)
            {
  			  [controllerImageView removeFromSuperview];
  			  [controllerImageView release];
  			}
  			__emulation_run = 0;
  			__emulation_saving = 0;
#ifdef WITH_ADS
  			[SOApp.delegate unpauseAdViews];
#endif
        [SOApp.saveStatesView startSaveData:0.1f];	
  			[SOApp.delegate switchToSaveStates];
  			[tabBar didMoveToWindowSaveStates];
  		}
  		else if (buttonIndex == 2)
  		{
  		  [SOApp.delegate switchToBrowse];
  			[tabBar didMoveToWindow];
  	    
  			__emulation_saving = 0;
  			__emulation_run = 0;
  			//pthread_join(main_tid, NULL);
  			//[screenView removeFromSuperview];
  			//[screenView release];
            if(!btUsed || !iphone_is_landscape)
            {
  			  [controllerImageView removeFromSuperview];
  			  [controllerImageView release];
  			}
  			__emulation_run = 0;
  			__emulation_saving = 0;
#ifdef WITH_ADS
  			[SOApp.delegate unpauseAdViews];
#endif
  		}
  		iphone_menu = IPHONE_MENU_DISABLED;
	  }
	  else if(iphone_menu == IPHONE_MENU_SAVE_CURRENT)
	  {
      __emulation_saving = 2;
      iphone_menu = IPHONE_MENU_DISABLED;
	  }
	  else if(iphone_menu == IPHONE_MENU_SAVE_NEW)
	  {
      __emulation_saving = 1;
      iphone_menu = IPHONE_MENU_DISABLED;
	  }
	  else
	  {
      iphone_menu = IPHONE_MENU_DISABLED;
	  }
	}
	else
	{
		if (buttonIndex == 0)
		{
      iphone_is_landscape = 0;
			iphone_soundon = 1;
			[ self getControllerCoords:0 ];
			[ self fixRects ];
			numFingers = 0;
			__emulation_run = 1;
			//screenView = [ [ScreenView alloc] initWithFrame: CGRectMake(0, 0, 320, 480)];
			//[self.view addSubview: screenView];
			controllerImageView = [ [ UIImageView alloc ] initWithImage:[UIImage imageNamed:[NSString stringWithFormat:@"controller_hs%d.png", [SOApp.optionsView getCurrentSkin]]]];
			controllerImageView.frame = CGRectMake(0.0f, 240.0f, 320.0f, 240.0f); // Set the frame in which the UIImage should be drawn in.
			controllerImageView.userInteractionEnabled = NO;
			controllerImageView.multipleTouchEnabled = NO;
			controllerImageView.clearsContextBeforeDrawing = YES;
			[controllerImageView setOpaque:YES];
			[controllerImageView setAlpha:((float)iphone_controller_opacity / 100.0f)];
			[self.view addSubview: controllerImageView]; // Draw the image in self.view.
		}
		else if (buttonIndex == 1)
		{
		  iphone_is_landscape = 0;
			iphone_soundon = 0;
			[ self getControllerCoords:0 ];
			[ self fixRects ];
			numFingers = 0;
			__emulation_run = 1;
			//screenView = [ [ScreenView alloc] initWithFrame: CGRectMake(0, 0, 320, 480)];
			//[self.view addSubview: screenView];
			controllerImageView = [ [ UIImageView alloc ] initWithImage:[UIImage imageNamed:[NSString stringWithFormat:@"controller_hs%d.png", [SOApp.optionsView getCurrentSkin]]]];
			controllerImageView.frame = CGRectMake(0.0f, 240.0f, 320.0f, 240.0f); // Set the frame in which the UIImage should be drawn in.
			controllerImageView.userInteractionEnabled = NO;
			controllerImageView.multipleTouchEnabled = NO;
			controllerImageView.clearsContextBeforeDrawing = YES;
			[controllerImageView setOpaque:YES];
			[controllerImageView setAlpha:((float)iphone_controller_opacity / 100.0f)];
			[self.view addSubview: controllerImageView]; // Draw the image in self.view.
		}
		else if (buttonIndex == 2)
		{
		  iphone_is_landscape = 1;
			iphone_soundon = 1;
			[ self getControllerCoords:1 ];
			[ self fixRects ];
			numFingers = 0;
			__emulation_run = 1;
			screenView = [ [ScreenView alloc] initWithFrame: CGRectMake(0, 0, 320, 480)];
			[self.view addSubview: screenView];
			if(!btUsed)
            {
                controllerImageView = [ [ UIImageView alloc ] initWithImage:[UIImage imageNamed:[NSString stringWithFormat:@"controller_fs%d.png", [SOApp.optionsView getCurrentSkin]]]];
                controllerImageView.frame = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f); // Set the frame in which the UIImage should be drawn in.
                controllerImageView.userInteractionEnabled = NO;
                controllerImageView.multipleTouchEnabled = NO;
                controllerImageView.clearsContextBeforeDrawing = YES;
                [controllerImageView setOpaque:YES];
                [controllerImageView setAlpha:((float)iphone_controller_opacity / 100.0f)];
                [self.view addSubview: controllerImageView]; // Draw the image in self.view.
            }
		}
		else
		{
            iphone_is_landscape = 1;
			iphone_soundon = 0;
			[ self getControllerCoords:1 ];
			[ self fixRects ];
			numFingers = 0;
			__emulation_run = 1;
			screenView = [ [ScreenView alloc] initWithFrame: CGRectMake(0, 0, 320, 480)];
			[self.view addSubview: screenView];
            if(!btUsed)
            {
                controllerImageView = [ [ UIImageView alloc ] initWithImage:[UIImage imageNamed:[NSString stringWithFormat:@"controller_fs%d.png", [SOApp.optionsView getCurrentSkin]]]];
                controllerImageView.frame = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f); // Set the frame in which the UIImage should be drawn in.
                [controllerImageView setOpaque:YES];
                [controllerImageView setAlpha:((float)iphone_controller_opacity / 100.0f)];
                controllerImageView.userInteractionEnabled = NO;
                controllerImageView.multipleTouchEnabled = NO;
                controllerImageView.clearsContextBeforeDrawing = YES;
                [self.view addSubview: controllerImageView]; // Draw the image in self.view.			
            }
		}
#ifdef WITH_ADS		
    [SOApp.delegate pauseAdViews];
#endif

/*
    pthread_attr_init(&main_attr);
    pthread_attr_setdetachstate(&main_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&main_tid, &main_attr, app_Thread_Start, NULL);
    pthread_attr_destroy(&main_attr);
*/
    [self performSelector:@selector(runProgram) withObject:nil afterDelay:0.0];
		//pthread_create(&main_tid, NULL, app_Thread_Start, NULL);

/*
		struct sched_param    param;
    param.sched_priority = 46;
    if(pthread_setschedparam(main_tid, SCHED_RR, &param) != 0)
    {
      fprintf(stderr, "Error setting pthread priority\n");
    }
*/
/*
		struct sched_param    param;
    param.sched_priority = 46;
    if(pthread_setschedparam(main_tid, SCHED_OTHER, &param) != 0)
    {
      fprintf(stderr, "Error setting pthread priority\n");
    }
*/
    //[NSThread detachNewThreadSelector:@selector(runProgram) toTarget:self withObject:nil];
    //[NSThread detachNewThreadSelector:@selector(runSound) toTarget:self withObject:nil];  
	}
}

- (void)runProgram
{
  [NSThread setThreadPriority:0.95];
	__emulation_run = 1;
	SDL_main(ourArgsCnt, ourArgs);
	__emulation_run = 0;
	__emulation_saving = 0;	
}

- (void)startEmu:(char*)path {
	int i;
	
  iphone_menu = IPHONE_MENU_DISABLED;
	
	ourArgsCnt = 0;
	/* faked executable path */
	ourArgs[ourArgsCnt]=get_resource_path("n64ios"); ourArgsCnt++;

	sprintf(ourArgsParam[ourArgsCnt], "%s", path);	
	ourArgs[ourArgsCnt]=ourArgsParam[ourArgsCnt]; ourArgsCnt++;

	ourArgs[ourArgsCnt]=NULL;

	for (i=0; i<ourArgsCnt; i++)
	{
		fprintf(stderr, "%s ", ourArgs[i]);
	}
	fprintf(stderr, "\n");
	
	self.view.frame = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
	
	UIActionSheet *alert = [[UIActionSheet alloc] initWithTitle:@"Choose Your Screen Orientation & Sound Options" delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:@"Portrait & Sound", @"Portrait & No Sound", /*@"Landscape & Sound", @"Landscape & No Sound",*/ nil];
	[alert showInView:self.view];
	[alert release];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
		// Initialization code
    /*[ self getControllerCoords:0 ];
    [ self fixRects ];
    numFingers = 0;
    iphone_touches = 0;
    self.navigationItem.hidesBackButton = YES;
    
    self.view.opaque = YES;
    self.view.clearsContextBeforeDrawing = YES;
    self.view.userInteractionEnabled = YES;
    self.view.multipleTouchEnabled = YES;
    self.view.contentMode = UIViewContentModeTopLeft;
    
    [[self.view layer] setMagnificationFilter:kCAFilterNearest];
    [[self.view layer] setMinificationFilter:kCAFilterNearest];
	   */
  }
	return self;
}

- (void)awakeFromNib {
	//[ self setTapDelegate: self ];
	//[ self setGestureDelegate: self ];
	//[ self setMultipleTouchEnabled: YES ];
	
	[ self getControllerCoords:0 ];
	[ self fixRects ];
	numFingers = 0;
	iphone_touches = 0;
	self.navigationItem.hidesBackButton = YES;
	
  self.view.opaque = YES;
	self.view.clearsContextBeforeDrawing = YES;
	self.view.userInteractionEnabled = YES;
	self.view.multipleTouchEnabled = YES;
	self.view.contentMode = UIViewContentModeTopLeft;
	
	[[self.view layer] setMagnificationFilter:kCAFilterNearest];
	[[self.view layer] setMinificationFilter:kCAFilterNearest];
	//[NSThread setThreadPriority:1.0];
/*
	struct sched_param    param;
  param.sched_priority = 40;
  if(pthread_setschedparam(pthread_self(), SCHED_RR, &param) != 0)
  {
    fprintf(stderr, "Error setting pthread priority\n");
  }
*/
/*
	struct sched_param    param;
  param.sched_priority = 47;
  if(pthread_setschedparam(pthread_self(), SCHED_RR, &param) != 0)
  {
    fprintf(stderr, "Error setting pthread priority\n");
  }
*/
}

- (void)drawRect:(CGRect)rect
{
}

- (void)fixRects {
/*
    UpLeft    	= [ self convertRect: UpLeft toView: self ];
    DownLeft  	= [ self convertRect: DownLeft toView: self ];
    UpRight   	= [ self convertRect: UpRight toView: self ];
    DownRight  	= [ self convertRect: DownRight toView: self ];
    Up     = [ self convertRect: Up toView: self ];
    Down   = [ self convertRect: Down toView: self ];
    Left   = [ self convertRect: Left toView: self ];
    Right  = [ self convertRect: Right toView: self ];
    Select = [ self convertRect: Select toView: self ];
    Start  = [ self convertRect: Start toView: self ];
	
    ButtonUpLeft    	= [ self convertRect: ButtonUpLeft toView: self ];
    ButtonDownLeft  	= [ self convertRect: ButtonDownLeft toView: self ];
    ButtonUpRight   	= [ self convertRect: ButtonUpRight toView: self ];
    ButtonDownRight  	= [ self convertRect: ButtonDownRight toView: self ];
    ButtonUp     = [ self convertRect: ButtonUp toView: self ];
    ButtonDown   = [ self convertRect: ButtonDown toView: self ];
    ButtonLeft   = [ self convertRect: ButtonLeft toView: self ];
    ButtonRight  = [ self convertRect: ButtonRight toView: self ];
	
    LPad   = [ self convertRect: LPad toView: self ];
    RPad   = [ self convertRect: RPad toView: self ];
	
    LPad2   = [ self convertRect: LPad2 toView: self ];
    RPad2   = [ self convertRect: RPad2 toView: self ];
	
    Menu   = [ self convertRect: Menu toView: self ];
*/
}

- (void)runMenu
{
  if(__emulation_paused)
  {
    return;
  }
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
  __emulation_paused = 1;
  iphone_menu = IPHONE_MENU_MAIN_LOAD;
  while(iphone_menu != IPHONE_MENU_DISABLED)
  {
    if(iphone_menu == IPHONE_MENU_MAIN_LOAD)
    {
      iphone_menu = IPHONE_MENU_MAIN;
      [NSThread detachNewThreadSelector:@selector(runMainMenu) toTarget:self withObject:nil];
		}
		else
		{
      sched_yield();
      usleep(1000000);
		}
	}
	__emulation_paused = 0;
	
  [pool release];
}

- (void)runMainMenu
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  //[NSThread setThreadPriority:0.5];
    
  /*
  struct sched_param    param;
  param.sched_priority = 45;
  if(pthread_setschedparam(pthread_self(), SCHED_RR, &param) != 0)
  {
    fprintf(stderr, "Error setting pthread priority\n");
  }
	*/
	UIActionSheet *alert = [[UIActionSheet alloc] initWithTitle:@"Choose an option from the menu. Press cancel to go back." delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:@"Save [Currently Loaded] State",@"Save State To New File",@"Quit To Menu",@"Cancel", nil];
	[alert showInView:self.view];
	[alert release];
  [pool release];
}

#define MyCGRectContainsPoint(rect, point)						  \
	(((point.x >= rect.origin.x) &&								        \
		(point.y >= rect.origin.y) &&							          \
		(point.x <= rect.origin.x + rect.size.width) &&			\
		(point.y <= rect.origin.y + rect.size.height)) ? 1 : 0)

#if 1
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{	    
  int touchstate[10];
  //Get all the touches.
  int i;
  NSSet *allTouches = [event allTouches];
  int touchcount = [allTouches count];
	
  if(!__emulation_run)
  {
    return;
  }
	
  for (i = 0; i < 10; i++) 
  {
    touchstate[i] = 0;
    oldtouches[i] = newtouches[i];
  }

  for (i = 0; i < touchcount; i++) 
  {
	  UITouch *touch = [[allTouches allObjects] objectAtIndex:i];
    
		if( touch != nil && 
		    ( touch.phase == UITouchPhaseBegan ||
			  touch.phase == UITouchPhaseMoved ||
  			  touch.phase == UITouchPhaseStationary) )
		{
			struct CGPoint point;
			point = [touch locationInView:self.view];
			
      	  	touchstate[i] = 1;
			
			if (MyCGRectContainsPoint(Left, point)) 
			{
				gp2x_pad_status |= GP2X_LEFT;
        		newtouches[i] = GP2X_LEFT;
			}
			else if (MyCGRectContainsPoint(Right, point)) 
			{
				gp2x_pad_status |= GP2X_RIGHT;
				newtouches[i] = GP2X_RIGHT;
			}
			else if (MyCGRectContainsPoint(Up, point)) 
			{
				gp2x_pad_status |= GP2X_UP;
				newtouches[i] = GP2X_UP;
			}
			else if (MyCGRectContainsPoint(Down, point))
			{
				gp2x_pad_status |= GP2X_DOWN;
				newtouches[i] = GP2X_DOWN;
			}
			else if (MyCGRectContainsPoint(A, point)) 
			{
				gp2x_pad_status |= GP2X_B;
				newtouches[i] = GP2X_B;
			}
			else if (MyCGRectContainsPoint(B, point)) 
			{
				gp2x_pad_status |= GP2X_X;
				newtouches[i] = GP2X_X;
			}
			else if (MyCGRectContainsPoint(AB, point)) 
			{
				gp2x_pad_status |= GP2X_X | GP2X_B;
				newtouches[i] = GP2X_X | GP2X_B;
			}
			else if (MyCGRectContainsPoint(UpLeft, point)) 
			{
				gp2x_pad_status |= GP2X_UP | GP2X_LEFT;
				newtouches[i] = GP2X_UP | GP2X_LEFT;
			} 
			else if (MyCGRectContainsPoint(DownLeft, point)) 
			{
				gp2x_pad_status |= GP2X_DOWN | GP2X_LEFT;
				newtouches[i] = GP2X_DOWN | GP2X_LEFT;
			}
			else if (MyCGRectContainsPoint(UpRight, point)) 
			{
				gp2x_pad_status |= GP2X_UP | GP2X_RIGHT;
				newtouches[i] = GP2X_UP | GP2X_RIGHT;
			}
			else if (MyCGRectContainsPoint(DownRight, point)) 
			{
				gp2x_pad_status |= GP2X_DOWN | GP2X_RIGHT;
				newtouches[i] = GP2X_DOWN | GP2X_RIGHT;
			}			
			else if (MyCGRectContainsPoint(LPad, point)) 
			{
				gp2x_pad_status |= GP2X_VOL_DOWN;
				newtouches[i] = GP2X_VOL_DOWN;
			}
			else if (MyCGRectContainsPoint(RPad, point)) 
			{
				gp2x_pad_status |= GP2X_VOL_UP;
				newtouches[i] = GP2X_VOL_UP;
			}
			else if (MyCGRectContainsPoint(Select, point)) 
			{
				gp2x_pad_status |= GP2X_SELECT;
				newtouches[i] = GP2X_SELECT;
			}
			else if (MyCGRectContainsPoint(Start, point)) 
			{
				gp2x_pad_status |= GP2X_START;
				newtouches[i] = GP2X_START;
			}
			else if (MyCGRectContainsPoint(Menu, point)) 
			{
				if(touch.phase == UITouchPhaseBegan || touch.phase == UITouchPhaseStationary)
				{
					if(__emulation_run)
					{
#if 0 // TEMP!
            [NSThread detachNewThreadSelector:@selector(runMenu) toTarget:self withObject:nil];
#endif
					}
					else
					{
  						[SOApp.delegate switchToBrowse];
  						[tabBar didMoveToWindow];
					}
					
          //return;
				}
			}
			
			if(oldtouches[i] != newtouches[i])
      	  	{
        	  gp2x_pad_status &= ~(oldtouches[i]);
		  	}
		}	
	} 

  for (i = 0; i < 10; i++) 
  {
    if(touchstate[i] == 0)
    {
      gp2x_pad_status &= ~(newtouches[i]);
      newtouches[i] = 0;
      oldtouches[i] = 0;
    }
  }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	[self touchesBegan:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	[self touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	[self touchesBegan:touches withEvent:event];
}
#endif
/*
- (void)view:(UIView *)view handleTapWithCount:(int)count event:(myGSEvent *)event {
	//NSLog(@"handleTapWithCount: %d", count);
	[self dumpEvent: event];
}
*/
/*
- (double)viewTouchPauseThreshold:(UIView *)view {
	NSLog(@"TouchPause!");
	return 0.0;
}
*/

#if 0
- (void)mouseEvent:(myGSEvent*)event {
	int i;
	int touchcount = event->fingerCount;
	
	gp2x_pad_status = 0;
	
	for (i = 0; i < touchcount; i++) 
	{
		struct CGPoint point = CGPointMake(event->points[i].x, event->points[i].y);
		
		if (MyCGRectContainsPoint(Left, point)) {
			gp2x_pad_status |= GP2X_LEFT;
		}
		else if (MyCGRectContainsPoint(Right, point)) {
			gp2x_pad_status |= GP2X_RIGHT;
		}
		else if (MyCGRectContainsPoint(Up, point)) {
			gp2x_pad_status |= GP2X_UP;
		}
		else if (MyCGRectContainsPoint(Down, point)) {
			gp2x_pad_status |= GP2X_DOWN;
		}
		else if (MyCGRectContainsPoint(A, point)) {
			gp2x_pad_status |= GP2X_B;
		}
		else if (MyCGRectContainsPoint(B, point)) {
			gp2x_pad_status |= GP2X_X;
		}
		else if (MyCGRectContainsPoint(AB, point)) {
			gp2x_pad_status |= GP2X_B | GP2X_X;
		}			
		else if (MyCGRectContainsPoint(UpLeft, point)) {
			gp2x_pad_status |= GP2X_UP | GP2X_LEFT;
		} 
		else if (MyCGRectContainsPoint(DownLeft, point)) {
			gp2x_pad_status |= GP2X_DOWN | GP2X_LEFT;
		}
		else if (MyCGRectContainsPoint(UpRight, point)) {
			gp2x_pad_status |= GP2X_UP | GP2X_RIGHT;
		}
		else if (MyCGRectContainsPoint(DownRight, point)) {
			gp2x_pad_status |= GP2X_DOWN | GP2X_RIGHT;
		}			
		else if (MyCGRectContainsPoint(LPad, point)) {
			gp2x_pad_status |= GP2X_L;
		}
		else if (MyCGRectContainsPoint(RPad, point)) {
			gp2x_pad_status |= GP2X_R;
		}			
		else if (MyCGRectContainsPoint(Select, point)) {
			gp2x_pad_status |= GP2X_SELECT;
		}
		else if (MyCGRectContainsPoint(Start, point)) {
			gp2x_pad_status |= GP2X_START;
		}
		else if (MyCGRectContainsPoint(Menu, point)) {
			//if(touch.phase == UITouchPhaseBegan || touch.phase == UITouchPhaseStationary)
			{
				[SOApp.delegate switchToBrowse];
				[tabBar didMoveToWindow];
				if(__emulation_run)
				{
					UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Save Game State" message:@"Save Game State?" delegate:self cancelButtonTitle:nil otherButtonTitles:@"Save Game!",@"Don't Save?", nil];
					[alert show];
					[alert release];
				}
			}
		}
	}
}

- (void)mouseDown:(myGSEvent*)event {
	//NSLog(@"mouseDown:");
	[self mouseEvent: event];
}

- (void)mouseDragged:(myGSEvent*)event {
	//NSLog(@"mouseDragged:");
	[self mouseEvent: event];
}

- (void)mouseEntered:(myGSEvent*)event {		
	//NSLog(@"mouseEntered:");
	[self mouseEvent: event];
}

- (void)mouseExited:(myGSEvent*)event {		
	//NSLog(@"mouseExited:");
	[self mouseEvent: event];
}

- (void)mouseMoved:(myGSEvent*)event {
	//NSLog(@"mouseMoved:");
	[self mouseEvent: event];
}

- (void)mouseUp:(myGSEvent*)event {
	[self mouseEvent: event];
}
#endif

/*
- (BOOL)isFirstResponder {
	return YES;
}
*/

- (void)getControllerCoords:(int)orientation {
    char string[256];
    FILE *fp;
	
	if(!orientation)
	{
		fp = fopen([[NSString stringWithFormat:@"%scontroller_hs%d.txt", get_resource_path("/"), [SOApp.optionsView getCurrentSkin]] UTF8String], "r");
  }
	else
	{
		fp = fopen([[NSString stringWithFormat:@"%scontroller_fs%d.txt", get_resource_path("/"), [SOApp.optionsView getCurrentSkin]] UTF8String], "r");
	}
	
	if (fp) 
	{
		int i = 0;
        while(fgets(string, 256, fp) != NULL && i < 17) {
			char* result = strtok(string, ",");
			int coords[4];
			int i2 = 1;
			while( result != NULL && i2 < 5 )
			{
				coords[i2 - 1] = atoi(result);
				result = strtok(NULL, ",");
				i2++;
			}
			
			switch(i)
			{
  			case 0:    DownLeft   	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 1:    Down   		= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 2:    DownRight    = CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 3:    Left  		= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 4:    Right  		= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 5:    UpLeft     	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 6:    Up     		= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 7:    UpRight  	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 8:    Select		= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 9:	   Start	   	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 10:   A     	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 11:   B     	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 12:   AB    	= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 13:   LPad			= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 14:   RPad			= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
  			case 15:   Menu			= CGRectMake( coords[0], coords[1], coords[2], coords[3] ); break;
        case 16:   iphone_controller_opacity = coords[0]; break;
			}
           	i++;
        }
        fclose(fp);
    }
}

/*
 Implement loadView if you want to create a view hierarchy programmatically
- (void)loadView {
}
 */

/*
 If you need to do additional setup after loading the view, override viewDidLoad.
- (void)viewDidLoad {
}
 */


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
	// Release anything that's not essential, such as cached data
}


- (void)dealloc {	
	[super dealloc];
}

- (void)volumeChanged:(id)sender
{
#if 0
	__audioVolume = volumeSlider.value;
#endif
}

- (void)setSaveState:(id)sender 
{
#if 0
	if(__emulation_run)
	{
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Save Game State" message:@"Save Game State?" delegate:self cancelButtonTitle:@"Dont Save?" otherButtonTitles:@"Save Game!", nil];
		[alert show];
		[alert release];
		__emulation_saving = 1;
		__emulation_run = 0;
		pthread_join(main_tid, NULL);
		[screenView release];
	}	
	[SOApp.delegate switchToBrowse];
	[tabBar didMoveToWindow];	
#endif
}

@end

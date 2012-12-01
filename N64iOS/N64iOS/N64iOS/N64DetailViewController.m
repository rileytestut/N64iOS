//
//  N64DetailViewController.m
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import "N64DetailViewController.h"

char showfps[2][64];
char* ourArgs[32];
char ourArgsParam[32][1024];
int ourArgsCnt = 0;
unsigned short* screenbuffer;
int iphone_touches = 0;
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
int __emulation_run;
int __emulation_saving;
int __emulation_paused;
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

@interface N64DetailViewController ()
@property (strong, nonatomic) UIPopoverController *masterPopoverController;
- (void)configureView;
@end

@implementation N64DetailViewController

@synthesize detailDescriptionLabel = _detailDescriptionLabel;
@synthesize masterPopoverController = _masterPopoverController;

#pragma mark - Managing the detail item

- (void)setRomName:(NSString *)romName {

    if (_romName != romName) {
        _romName = romName;
        
        // Update the view.
        [self configureView];
    }

    if (self.masterPopoverController != nil) {
        [self.masterPopoverController dismissPopoverAnimated:YES];
    }        
}

- (void)configureView
{
    // Update the user interface for the detail item.

    if (self.romName) {
        self.detailDescriptionLabel.text = self.romName;
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    [self configureView];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    self.detailDescriptionLabel = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    double delayInSeconds = 1.0;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, delayInSeconds * NSEC_PER_SEC);
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Loading..." message:@"\n" delegate:nil cancelButtonTitle:nil otherButtonTitles:@"OK", nil];
        [alert addSubview:[[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge]];
        [alert show];
        
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectoryPath = [paths objectAtIndex:0];
        
        NSString *listingsPath = [documentsDirectoryPath stringByAppendingPathComponent:self.romName];
        char *cListingsPath = (char*)[listingsPath UTF8String];
        [self startEmu:cListingsPath];
        
    });
}

- (void)startEmu:(char*)path {
	int i;
    
	ourArgsCnt = 0;
	/* faked executable path */
    static char resource_path[1024];
    const char* fakepath = [[[NSBundle mainBundle] resourcePath] UTF8String];
    sprintf(resource_path, "%s/%s", fakepath, "n64ios");
	ourArgs[ourArgsCnt]=path; ourArgsCnt++;
    
	sprintf(ourArgsParam[ourArgsCnt], "%s", path);	
	ourArgs[ourArgsCnt]=ourArgsParam[ourArgsCnt]; ourArgsCnt++;
    
	ourArgs[ourArgsCnt]=NULL;
    
	for (i=0; i<ourArgsCnt; i++)
	{
		fprintf(stderr, "%s ", ourArgs[i]);
	}
	fprintf(stderr, "\n");
	
	self.view.frame = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
    
    iphone_is_landscape = 0;
    iphone_soundon = 1;
    __emulation_run = 1;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self performSelector:@selector(runProgram) withObject:nil afterDelay:0.0];
    });
}

- (void)runProgram
{
    [NSThread setThreadPriority:0.95];
	__emulation_run = 1;
	SDL_main(ourArgsCnt, ourArgs);
	__emulation_run = 0;
	__emulation_saving = 0;	
}

void* app_Thread_Start(void* args)
{
    @autoreleasepool {
        __emulation_run = 1;
        SDL_main(ourArgsCnt, ourArgs);
        __emulation_run = 0;
        __emulation_saving = 0;
    }
    
	return NULL;
}

#pragma mark - Split view

- (void)splitViewController:(UISplitViewController *)splitController willHideViewController:(UIViewController *)viewController withBarButtonItem:(UIBarButtonItem *)barButtonItem forPopoverController:(UIPopoverController *)popoverController
{
    barButtonItem.title = NSLocalizedString(@"Master", @"Master");
    [self.navigationItem setLeftBarButtonItem:barButtonItem animated:YES];
    self.masterPopoverController = popoverController;
}

- (void)splitViewController:(UISplitViewController *)splitController willShowViewController:(UIViewController *)viewController invalidatingBarButtonItem:(UIBarButtonItem *)barButtonItem
{
    // Called when the view is shown again in the split view, invalidating the button and popover controller.
    [self.navigationItem setLeftBarButtonItem:nil animated:YES];
    self.masterPopoverController = nil;
}

@end

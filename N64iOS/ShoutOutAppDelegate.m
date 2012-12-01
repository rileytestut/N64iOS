//
//  n64ios
//
//  Created by Spookysoft on 9/6/08.
//  Copyright Spookysoft 2008. All rights reserved.
//

#import "SOApplication.h"

#import "BTDevice.h"
#import "BTInquiryViewController.h"

#import <btstack/btstack.h>
#import <btstack/run_loop.h>
#import <btstack/hci_cmds.h>

#import <dlfcn.h>

void *btlibHandle;
int (*bt_open)();
int (*bt_close)();
int (*bt_send_cmd)(const hci_cmd_t *cmd, ...);
void (*bt_send_l2cap)(uint16_t local_cid, uint8_t *data, uint16_t len);
btstack_packet_handler_t (*bt_register_packet_handler)(btstack_packet_handler_t handler);
void (*bt_flip_addr)(bd_addr_t dest, bd_addr_t src);
void (*run_loop_init)(RUN_LOOP_TYPE type);
hci_cmd_t *p_hci_disconnect;
hci_cmd_t *p_l2cap_create_channel;
hci_cmd_t *p_hci_remote_name_request;
hci_cmd_t *p_hci_inquiry;
hci_cmd_t *p_btstack_set_power_mode;
hci_cmd_t *p_hci_inquiry_cancel;
hci_cmd_t *p_hci_remote_name_request_cancel;

int BluetoothManagerEnabled();
int BluetoothManagerSetEnabled(int newState);

unsigned long btUsed = 0;
bool btOK = false;
BTDevice *device;
uint16_t wiiMoteConHandle = 0;

extern int __disableaccel;
extern int __acceldigital;
extern int __wiimote;
extern unsigned long gp2x_pad_status;
extern long gp2x_pad_x_axis;
extern long gp2x_pad_y_axis;
extern long gp2x_pad_z_axis;

enum  { GP2X_UP=0x1,       GP2X_LEFT=0x4,       GP2X_DOWN=0x10,  GP2X_RIGHT=0x40,
	      GP2X_START=1<<8,   GP2X_SELECT=1<<9,    GP2X_L=1<<10,    GP2X_R=1<<11,
	      GP2X_A=1<<12,      GP2X_B=1<<13,        GP2X_X=1<<14,    GP2X_Y=1<<15,
        GP2X_VOL_UP=1<<23, GP2X_VOL_DOWN=1<<22, GP2X_PUSH=1<<27 };
  
void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
        bd_addr_t event_addr;

        switch (packet_type) {

                case L2CAP_DATA_PACKET:
                        if (packet[0] == 0xa1 && packet[1] == (__disableaccel ? 0x30 : 0x31))
                        {
/*
  0 	 0x01 	 D-Pad Left 	 Two
  1 	0x02 	D-Pad Right 	One
  2 	0x04 	D-Pad Down 	B
  3 	0x08 	D-Pad Up 	A
  4 	0x10 	Plus 	Minus
  5 	0x20 	Other uses 	Other uses
  6 	0x40 	Other uses 	Other uses
  7 	0x80 	Unknown 	Home
*/
                                gp2x_pad_status = 0;
                                if(__acceldigital || __disableaccel)
                                {
                                  int temp_val;
                                  long x_axis = 0;
                                  long y_axis = 0;
                                                                    
                                  if(packet[2] & 0x01) 
                                  {
                                    x_axis = -80;
                                    if(__disableaccel)
                                      gp2x_pad_status |= GP2X_DOWN;
                                  }
                                  if(packet[2] & 0x02) 
                                  {
                                    x_axis = 80;
                                    if(__disableaccel)
                                      gp2x_pad_status |= GP2X_UP;
                                  }
                                  if(packet[2] & 0x04)
                                  {
                                    y_axis = -80;
                                    if(__disableaccel)
                                      gp2x_pad_status |= GP2X_RIGHT;
                                  }
                                  if(packet[2] & 0x08)
                                  {
                                    y_axis = 80;
                                    if(__disableaccel)
                                      gp2x_pad_status |= GP2X_LEFT;
                                  }
                                  gp2x_pad_x_axis = x_axis;
                                  gp2x_pad_y_axis = y_axis;
                                  gp2x_pad_z_axis = 0;
                                  
                                  
                                  if(!__disableaccel)
                                  {
                                    temp_val = packet[5];
                                    temp_val = (int)(((float)temp_val / 1.6) - 80.0);
                                    temp_val *= 8;
                                    if(temp_val > 32)
                                      gp2x_pad_status |= GP2X_LEFT;
                                    if(temp_val < -32)
                                      gp2x_pad_status |= GP2X_RIGHT;      
                                  
                                    temp_val = packet[4];
                                    temp_val = (int)(((float)temp_val / 1.6) - 80.0);
                                    temp_val *= 8;
                                    if(temp_val > 24)
                                      gp2x_pad_status |= GP2X_UP;
                                    if(temp_val < -24)
                                      gp2x_pad_status |= GP2X_DOWN;
                                  }
                                }
                                else
                                {
                                  if(packet[2] & 0x01) 
                                  {
                                    gp2x_pad_status |= GP2X_DOWN;
                                  }
                                  if(packet[2] & 0x02) 
                                  {
                                    gp2x_pad_status |= GP2X_UP;
                                  }
                                  if(packet[2] & 0x04) 
                                  {
                                    gp2x_pad_status |= GP2X_RIGHT;
                                  }
                                  if(packet[2] & 0x08) 
                                  {
                                    gp2x_pad_status |= GP2X_LEFT;
                                  }
                                  
                                  gp2x_pad_x_axis = packet[4];
                                  gp2x_pad_y_axis = packet[5];
                                  gp2x_pad_z_axis = packet[6];
                                }
                                if(packet[2] & 0x10)
                                {
                                  gp2x_pad_status |= GP2X_START;
                                }
                                if(packet[3] & 0x01)
                                {
                                  gp2x_pad_status |= GP2X_B;
                                }
                                if(packet[3] & 0x02)
                                {
                                  gp2x_pad_status |= GP2X_X;
                                }
                                if(packet[3] & 0x04)
                                {
                                  gp2x_pad_status |= GP2X_L;
                                }
                                if(packet[3] & 0x08)
                                {
                                  gp2x_pad_status |= GP2X_START;
                                }
                                if(packet[3] & 0x10)
                                {
                                  gp2x_pad_status |= GP2X_SELECT;
                                }
                                if(packet[3] & 0x80)
                                {
                                  gp2x_pad_status |= GP2X_R;
                                }
                                                                 
                                //fprintf(stderr, "x: %d y: %d z: %d packet 4 %d 5 %d 6 %d\n", gp2x_pad_x_axis, gp2x_pad_y_axis, gp2x_pad_z_axis, packet[4], packet[5], packet[6]);
                                //fflush(stderr);
                        }
                        break;

                case HCI_EVENT_PACKET:

                        switch (packet[0]){

                                case L2CAP_EVENT_CHANNEL_OPENED:
                                        if (packet[2] == 0) {
                                                // inform about new l2cap connection
                                                bt_flip_addr(event_addr, &packet[3]);
                                                uint16_t psm = READ_BT_16(packet, 11);
                                                uint16_t source_cid = READ_BT_16(packet, 13);
                                                wiiMoteConHandle = READ_BT_16(packet, 9);
                                                NSLog(@"Channel successfully opened: handle 0x%02x, psm 0x%02x, source cid 0x%02x, dest cid 0x%02x",
                                                           wiiMoteConHandle, psm, source_cid,  READ_BT_16(packet, 15));
                                                if (psm == 0x13) {
                                                        // interupt channel openedn succesfully, now open control channel, too.
                                                        bt_send_cmd(p_l2cap_create_channel, event_addr, 0x11);
                                                } else {

                                                        // request acceleration data.. probably has to be sent to control channel 0x11 instead of 0x13
                                                        if(!__disableaccel)
                                                        {
                                                          uint8_t setMode31[] = { 0x52, 0x12, 0x00, 0x31 };
                                                          bt_send_l2cap( source_cid, setMode31, sizeof(setMode31));
                                                        }
                                                        uint8_t setLEDs[] = { 0x52, 0x11, 0x10 };
                                                        bt_send_l2cap( source_cid, setLEDs, sizeof(setLEDs));

                                                        btUsed = 1;
                                                        
                                                        // start app
                                                        [SOApp.delegate switchToBrowse];

                                                      	// Load ROMs
                                                        [SOApp.romView startRootData:2.0f];
                                                }
                                        }
                                        break;

                                default:
                                        break;
                        }
                        break;

                default:
                        break;
        }
}

@implementation ShoutOutAppDelegate

@synthesize window;
@synthesize windowID;
@synthesize navigationController;
@synthesize inqViewControl;

/* convenience method */
+(ShoutOutAppDelegate *)sharedAppDelegate {
	/* the delegate is set in UIApplicationMain(), which is garaunteed to be called before this method */
	return (ShoutOutAppDelegate *)[[UIApplication sharedApplication] delegate];
}

- (id)init
{
	if (self = [super init])
    {
        window = nil;
        windowID = 0;
	}
	return self;
}


- (void)applicationDidFinishLaunching:(UIApplication *)application {
    int i;
  
    /* Set working directory to resource path */
    [[NSFileManager defaultManager] changeCurrentDirectoryPath: [[NSBundle mainBundle] resourcePath]];

    btUsed = 0;

    [[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    application.delegate = self;

#ifdef WITH_ADS
  //MainScreenWindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  //Blocked = [AltAds isAdBlockingEnabled];
  for(i = 0; i < NUMBER_OF_AD_PAGES; i++)
  {
    altAds[i] = [[AltAds alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 55.0f) andWindow:nil];
  }
#endif

	CGRect frame = window.frame;
	frame.origin.y = window.frame.origin.y+window.frame.size.height-49;
	frame.size.height = 49;
	tabBar.frame = frame;
	[window addSubview:tabBar];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
  
  //[navigationController pushViewController:SOApp.romView animated:NO];
  
  //[self switchToBrowse];
  // Load ROMs
  [SOApp.romView startRootData:2.0f];
  
	// Load Saves
  [SOApp.saveStatesView startSaveData:4.0f];


//	tabBar.selectedItem = 0;
	// Configure and show the window
	//[SOApp.nowPlayingView startEmu:"/var/mobile/Media/ROMs/GBA/Pokemon - Emerald Version (U) [f1] (Save Type).gba"];
	//[window addSubview:[SOApp.nowPlayingView view]];
	//[window makeKeyAndVisible];
	
	[window addSubview:[navigationController view]];

	[window makeKeyAndVisible];

  //[ShoutOutAppDelegate sharedAppDelegate].window = window;

  //[self switchToBrowse];

    if(__wiimote)
    {
        btlibHandle = dlopen("/usr/local/lib/libBTstack.dylib", RTLD_NOW);
        
        if(btlibHandle != NULL)
        {
            bt_open = dlsym(btlibHandle, "bt_open");
            bt_close = dlsym(btlibHandle, "bt_close");
            bt_send_cmd = dlsym(btlibHandle, "bt_send_cmd");
            bt_send_l2cap = dlsym(btlibHandle, "bt_send_l2cap");
            bt_register_packet_handler = dlsym(btlibHandle, "bt_register_packet_handler");
            bt_flip_addr = dlsym(btlibHandle, "bt_flip_addr");
            run_loop_init = dlsym(btlibHandle, "run_loop_init");
            
            p_hci_disconnect = dlsym(btlibHandle, "hci_disconnect");
            p_l2cap_create_channel = dlsym(btlibHandle, "l2cap_create_channel");
            p_hci_remote_name_request = dlsym(btlibHandle, "hci_remote_name_request");
            p_hci_inquiry = dlsym(btlibHandle, "hci_inquiry");
            p_btstack_set_power_mode = dlsym(btlibHandle, "btstack_set_power_mode");
            p_hci_inquiry_cancel = dlsym(btlibHandle, "hci_inquiry_cancel");
            p_hci_remote_name_request_cancel = dlsym(btlibHandle, "hci_remote_name_request_cancel");
            
            // turn Apple's Bluetooth off
            if (BluetoothManagerEnabled())
            {
                printf("Apple's Bluetooth is active, forcing it off!\n");
                BluetoothManagerSetEnabled(0);
                // wait a bit
                sleep(2);
            }
            
            run_loop_init(RUN_LOOP_COCOA);
            if ( bt_open() )
            {
              // Alert user?
            }
            else
            {
                    bt_register_packet_handler(packet_handler);
                    btOK = true;
            }
        }
    }
  
  if (__wiimote && btOK) 
  {
    // create inq controller
    inqViewControl = [[BTInquiryViewController alloc] init];
    [inqViewControl setTitle:@"WiiMote Device Selector"];
    [inqViewControl setAllowSelection:NO];

    UITextView *footer = [[UITextView alloc] initWithFrame:CGRectMake(10,00,300,100)];
    footer.text = @"Make your WiiMote discoverable by pressing the 1+2 buttons at the same time. Use the navigation bar back button to exit.";
    footer.textColor = [UIColor blackColor];
    footer.font = [UIFont fontWithName:@"Arial" size:18];
    // footer.textAlignment = UITextAlignmentCenter;
    footer.backgroundColor = [UIColor whiteColor];
    footer.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    footer.editable = false;
    [[inqViewControl tableView] setTableFooterView:footer];
    
  	if ([[[self navigationController] viewControllers] containsObject:inqViewControl]) {
  		[[self navigationController] popToViewController:inqViewControl animated:NO];
  	} else {
  		[[self navigationController] pushViewController:inqViewControl animated:NO];
  	}

    [inqViewControl setDelegate:self];
    [inqViewControl startInquiry];
  }
  else
  {
    [self switchToBrowse];
  }
}

#ifdef WITH_ADS
- (AltAds*)getAdViewWithIndex:(int)index {
  if(index < NUMBER_OF_AD_PAGES)
  {
    return altAds[index];
  }
  return altAds[0];
}

- (void)pauseAdViews {
/*
  int i;
  
  for(i = 0; i < NUMBER_OF_AD_PAGES; i++)
  {
    [altAds[i] stopAdTimers];
	}
*/
}

- (void)unpauseAdViews {
  int i;
  
  for(i = 0; i < NUMBER_OF_AD_PAGES; i++)
  {
    [altAds[i] RefreshAd];
  }
}
#endif

#pragma mark TabBar Actions
- (void)switchToBrowse {
	[[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
	[tabBar setHidden:NO];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = FALSE;
	navigationController.navigationBar.hidden = FALSE;

	[SOApp.romView reload];
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.romView]) {
		[[self navigationController] popToViewController:SOApp.romView animated:NO];
	} else {
		[[self navigationController] pushViewController:SOApp.romView animated:NO];
	}
}
- (void)switchToSaveStates {
	[[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
	[tabBar setHidden:NO];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = FALSE;
	navigationController.navigationBar.hidden = FALSE;
	
	[SOApp.saveStatesView reload];
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.saveStatesView]) {
		[[self navigationController] popToViewController:SOApp.saveStatesView animated:NO];
	} else {
		[[self navigationController] pushViewController:SOApp.saveStatesView animated:NO];
	}
}
- (void)switchToNowPlaying {
	[[UIApplication sharedApplication] setStatusBarHidden:YES animated:NO];
	[tabBar setHidden:YES];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 0;
	navFrame.size.height = 480;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = TRUE;
	navigationController.navigationBar.hidden = TRUE;

	gp2x_pad_status = 0;
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.nowPlayingView]) {
		[[self navigationController] popToViewController:SOApp.nowPlayingView animated:NO];
	} else {
		[[self navigationController] pushViewController:SOApp.nowPlayingView animated:NO];
	}
}
- (void)switchToRecent {
	[[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
	[tabBar setHidden:NO];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = FALSE;
	navigationController.navigationBar.hidden = FALSE;
	
	[SOApp.recentView reload];
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.recentView]) {
		[[self navigationController] popToViewController:SOApp.recentView animated:NO];
	} else {
		[[self navigationController] pushViewController:SOApp.recentView animated:NO];
	}
}
- (void)switchToOptions {
	[[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
	[tabBar setHidden:NO];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = FALSE;
	navigationController.navigationBar.hidden = FALSE;
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.optionsView]) {
		[[self navigationController] popToViewController:SOApp.optionsView animated:NO];
	} else {
		[[self navigationController] pushViewController:SOApp.optionsView animated:NO];
	}
}

- (void)switchToWebBrowserView
{
	[[UIApplication sharedApplication] setStatusBarHidden:NO animated:NO];
	[tabBar setHidden:NO];
	CGRect navFrame = [navigationController view].frame;
	navFrame.origin.y = 20;
	navFrame.size.height = 460 - 49;
	[navigationController view].frame = navFrame;
	navigationController.navigationBarHidden = FALSE;
	navigationController.navigationBar.hidden = FALSE;
	
	if ([[[self navigationController] viewControllers] containsObject:SOApp.webBrowserView]) 
	{
		[[self navigationController] popToViewController:SOApp.webBrowserView animated:NO];
	} 
	else
	{
		[[self navigationController] pushViewController:SOApp.webBrowserView animated:NO];
	}
}

- (void)reloadROMs
{
  [SOApp.romView startRootData:2.0f];
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // disconnect
    if (__wiimote && wiiMoteConHandle && bt_send_cmd) {
        bt_send_cmd(p_hci_disconnect, wiiMoteConHandle, 0x13); // remote closed connection
    }
    
    if(__wiimote && bt_send_cmd)
    {
        bt_close();
    }

    SDL_SendQuit();
    /* hack to prevent automatic termination.  See SDL_uikitevents.m for details */
	longjmp(*(jump_env()), 1);
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
}

- (void) applicationWillResignActive:(UIApplication*)application
{
	SDL_SendWindowEvent(self.windowID, SDL_WINDOWEVENT_MINIMIZED, 0, 0);
}

- (void) applicationDidBecomeActive:(UIApplication*)application
{
	SDL_SendWindowEvent(self.windowID, SDL_WINDOWEVENT_RESTORED, 0, 0);
}

-(void) deviceChoosen:(BTInquiryViewController *) inqView device:(BTDevice*) selectedDevice{
        NSLog(@"deviceChoosen %@", [device toString]);
}

- (void) deviceDetected:(BTInquiryViewController *) inqView device:(BTDevice*) selectedDevice {
    NSLog(@"deviceDetected %@", [device toString]);
    if ([selectedDevice name] && [[selectedDevice name] caseInsensitiveCompare:@"Nintendo RVL-CNT-01"] == NSOrderedSame){
        NSLog(@"WiiMote found with address %@", [BTDevice stringForAddress:[selectedDevice address]]);
        device = selectedDevice;
        [inqViewControl stopInquiry];
        [inqViewControl showConnecting:device];
        
        // connect to device
        [device setConnectionState:kBluetoothConnectionConnecting];
        [[[SOApp.delegate inqViewControl] tableView] reloadData];
        bt_send_cmd(p_l2cap_create_channel, [device address], 0x13);
    }
}

- (void) inquiryStopped{
}

- (void) disconnectDevice:(BTInquiryViewController *) inqView device:(BTDevice*) selectedDevice {
}

- (void)dealloc {
#ifdef WITH_ADS
  int i;
  for(i = 0; i < NUMBER_OF_AD_PAGES; i++)
  {
    [altAds[i] release];
  }
#endif

	[ navigationController release ];
	[ window release ];
	[ super dealloc ];
}

@end

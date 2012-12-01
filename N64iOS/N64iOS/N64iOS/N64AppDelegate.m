//
//  N64AppDelegate.m
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import "N64AppDelegate.h"

@implementation N64AppDelegate

@synthesize window = _window;
@synthesize windowID;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    
    [self loadPlugins];
    
    // Override point for customization after application launch.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        UISplitViewController *splitViewController = (UISplitViewController *)self.window.rootViewController;
        UINavigationController *navigationController = [splitViewController.viewControllers lastObject];
        splitViewController.delegate = (id)navigationController.topViewController;
    }
    
    return YES;
}

- (void)loadPlugins {
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
    
    NSString *pluginsDirectory = [documentsDirectory stringByAppendingPathComponent:@"plugins"];
    NSString *configDirectory = [documentsDirectory stringByAppendingPathComponent:@"config"];
    
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    
    if (![[NSUserDefaults standardUserDefaults] objectForKey:@"firstRun"]) {
        
        NSString *configDirectory = [documentsDirectory stringByAppendingPathComponent:@"config"];
        
        [fileManager createDirectoryAtPath:configDirectory withIntermediateDirectories:YES attributes:NULL error:NULL];
        [fileManager createDirectoryAtPath:pluginsDirectory withIntermediateDirectories:YES attributes:NULL error:NULL];
        
        NSString *confFilePath = [[NSBundle mainBundle] pathForResource:@"mupen64plus" ofType:@"conf"];
        NSData *confFile = [NSData dataWithContentsOfFile:confFilePath];
        [confFile writeToFile:[NSString stringWithFormat:@"%@/%@", configDirectory, @"mupen64plus.conf"] atomically:YES];
    }
    
    NSString *configFileFilepath = [[NSBundle mainBundle] pathForResource:@"mupen64plus" ofType:@"conf"];
    NSData *configFile = [NSData dataWithContentsOfFile:configFileFilepath];
    
    NSString *originalConfigFileFilepath = [configDirectory stringByAppendingPathComponent:@"mupen64plus.conf"];
    NSData *originalConfigFile = [NSData dataWithContentsOfFile:originalConfigFileFilepath];
    
    if (![configFile isEqualToData: originalConfigFile]) {
        [fileManager removeItemAtPath:originalConfigFileFilepath error:NULL];
        [configFile writeToFile:originalConfigFileFilepath atomically:YES];
    }
    
    // Dummy Audio
    
    NSString *pluginFilepath = [[NSBundle mainBundle] pathForResource:@"dummyaudio" ofType:@"dylib"];
    NSData *plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    NSString *originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"dummyaudio.dylib"];
    NSData *originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
    
    // Dummy Input
    
    pluginFilepath = [[NSBundle mainBundle] pathForResource:@"dummyinput" ofType:@"dylib"];
    plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"dummyinput.dylib"];
    originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
    
    // Dummy Video
    
    pluginFilepath = [[NSBundle mainBundle] pathForResource:@"dummyvideo" ofType:@"dylib"];
    plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"dummyvideo.dylib"];
    originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
    
    // gles2n64
    
    pluginFilepath = [[NSBundle mainBundle] pathForResource:@"gles2n64" ofType:@"dylib"];
    plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"gles2n64.dylib"];
    originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
    
    configFileFilepath = [[NSBundle mainBundle] pathForResource:@"gles2n64" ofType:@"conf"];
    configFile = [NSData dataWithContentsOfFile:configFileFilepath];
    
    originalConfigFileFilepath = [configDirectory stringByAppendingPathComponent:@"gles2n64.conf"];
    originalConfigFile = [NSData dataWithContentsOfFile:originalConfigFileFilepath];
    
    if (![configFile isEqualToData: originalConfigFile]) {
        [fileManager removeItemAtPath:originalConfigFileFilepath error:NULL];
        [configFile writeToFile:originalConfigFileFilepath atomically:YES];
    }
    
    // jttl_audio
    
    pluginFilepath = [[NSBundle mainBundle] pathForResource:@"jttl_audio" ofType:@"dylib"];
    plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"jttl_audio.dylib"];
    originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
    
    configFileFilepath = [[NSBundle mainBundle] pathForResource:@"jttl_audio" ofType:@"conf"];
    configFile = [NSData dataWithContentsOfFile:configFileFilepath];
    
    originalConfigFileFilepath = [configDirectory stringByAppendingPathComponent:@"jttl_audio.conf"];
    originalConfigFile = [NSData dataWithContentsOfFile:originalConfigFileFilepath];
    
    if (![configFile isEqualToData: originalConfigFile]) {
        [fileManager removeItemAtPath:originalConfigFileFilepath error:NULL];
        [configFile writeToFile:originalConfigFileFilepath atomically:YES];
    }
        
    // mupen64_hle_rsp_azimer
    
    pluginFilepath = [[NSBundle mainBundle] pathForResource:@"mupen64_hle_rsp_azimer" ofType:@"dylib"];
    plugin = [NSData dataWithContentsOfFile:pluginFilepath];
    
    originalPluginFilepath = [pluginsDirectory stringByAppendingPathComponent:@"mupen64_hle_rsp_azimer.dylib"];
    originalPlugin = [NSData dataWithContentsOfFile:originalPluginFilepath];
    
    if (![plugin isEqualToData: originalPlugin]) {
        [fileManager removeItemAtPath:originalPluginFilepath error:NULL];
        [plugin writeToFile:originalPluginFilepath atomically:YES];
    }
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end

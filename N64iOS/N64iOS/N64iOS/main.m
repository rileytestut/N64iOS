//
//  main.m
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "N64AppDelegate.h"

char *applicationDirectory = nil;

int main(int argc, char *argv[])
{
    @autoreleasepool {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectoryPath = [paths objectAtIndex:0];
        applicationDirectory = (char *)[documentsDirectoryPath UTF8String];
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([N64AppDelegate class]));
    }
}

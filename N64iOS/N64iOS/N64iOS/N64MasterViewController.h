//
//  N64MasterViewController.h
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import <UIKit/UIKit.h>

@class N64DetailViewController;

@interface N64MasterViewController : UITableViewController

@property (strong, nonatomic) N64DetailViewController *detailViewController;

@end

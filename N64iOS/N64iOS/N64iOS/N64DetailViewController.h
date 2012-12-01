//
//  N64DetailViewController.h
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface N64DetailViewController : UIViewController <UISplitViewControllerDelegate>

@property (copy, nonatomic) NSString *romName;

@property (strong, nonatomic) IBOutlet UILabel *detailDescriptionLabel;

@end

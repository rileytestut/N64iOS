#import <UIKit/UIKit.h>


@interface OptionsController : UIViewController 
{
	IBOutlet UISegmentedControl*	segmentedSkin;
	IBOutlet UISwitch*				switchScaling;
	IBOutlet UISwitch*				switchAutosave;
	IBOutlet UISwitch*				switchAccelDigital;
	IBOutlet UISwitch*				switchFramerate;
	IBOutlet UISwitch*				switchWiiMote;
	IBOutlet UISegmentedControl*	segmentedFrameskip;
	IBOutlet UISwitch*				switchDisableAccel;
	NSMutableArray*					optionsArray;
}

- (void)optionChanged:(id)sender;
- (NSString*)getDocumentsDirectory;
- (void)getOptions;
- (int)getCurrentSkin;

@end

#import "SOApplication.h"

BOOL blocking = NO;

@implementation TabBar

-(void)resetButton
{
	blocking = YES;
	
	[self setSelectedItem:[self.items objectAtIndex:0]];
}

- (void)didMoveToWindow
{
	[self setSelectedItem:[self.items objectAtIndex:0]];
}

/*
- (void)didMoveToWindowNowPlaying
{
	[self setSelectedItem:[self.items objectAtIndex:3]];
}
*/

/*
- (void)didMoveToWindowBookmarks
{
	[self setSelectedItem:[self.items objectAtIndex:1]];
}
*/

- (void)didMoveToWindowSaveStates
{
	[self setSelectedItem:[self.items objectAtIndex:2]];
}

- (void)setSelectedItem:(id)item {
	[super setSelectedItem:item];
	int itemIndex = [self.items indexOfObject:self.selectedItem];

		if (!blocking)
		{
			if (itemIndex == 0) 
			{
				[SOApp.delegate switchToBrowse];
			} 
			else if (itemIndex == 1) 
			{
				[SOApp.delegate switchToRecent];
			} 
			else if (itemIndex == 2) 
			{
				[SOApp.delegate switchToOptions];
			} 
			else 
			{
				[SOApp.delegate switchToSaveStates];
			}
		}
		blocking = NO;
}

- (void)dealloc {
	[super dealloc];
}


@end

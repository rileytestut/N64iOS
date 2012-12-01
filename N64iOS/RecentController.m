#import "SOApplication.h"


@implementation RecentController

- (void)dealloc {
	[ super dealloc ];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	self.navigationItem.hidesBackButton = YES;
}

-(void)awakeFromNib
{
	self.navigationItem.hidesBackButton = YES;
	
	// always put any sort of initializations in here. They will only be called once.
	adNotReceived = 0;

	[self getRecent];
}

- (void)reload
{
  [tableview reloadData];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.navigationItem.hidesBackButton = YES;
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)viewDidDisappear:(BOOL)animated {
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
}

- (NSString*)getDocumentsDirectory
{
#ifdef APPSTORE_BUILD
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex: 0];
	return documentsDirectory;
#else
	return @"/Applications/n64ios.app";
#endif
}

- (void)addRecent:(NSString*)thisPath {
	if(thisPath)
	{
		[recentArray addObject:[NSDictionary dictionaryWithObjectsAndKeys:
								thisPath, @"path",
								nil]];
		if([recentArray count] > 7)
		{
			[recentArray removeObjectAtIndex:1];
		}
		[tableview reloadData];
		
	}
	else
	{
		return;
	}
	
	NSString *path=[[self getDocumentsDirectory] stringByAppendingPathComponent:@"recent.bin"];
	NSData *plistData;
	
	NSString *error;
	
	
	
	plistData = [NSPropertyListSerialization dataFromPropertyList:recentArray
				 
												format:NSPropertyListBinaryFormat_v1_0
				 
												errorDescription:&error];
	
	if(plistData)
		
	{
		
		NSLog(@"No error creating plist data.");
		
		[plistData writeToFile:path atomically:NO];
		
	}
	else
	{
		
		NSLog(error);
		
		[error release];
		
	}
}

- (void)getRecent {
	NSString *path=[[self getDocumentsDirectory] stringByAppendingPathComponent:@"recent.bin"];
	
	NSData *plistData;
	id plist;
	NSString *error;
	
	NSPropertyListFormat format;
	
	plistData = [NSData dataWithContentsOfFile:path];
	
	
	
	plist = [NSPropertyListSerialization propertyListFromData:plistData
			 
											 mutabilityOption:NSPropertyListImmutable
			 
													   format:&format
			 
											 errorDescription:&error];
	
	if(!plist)
	{
		
		NSLog(error);
		
		[error release];
		
		recentArray = [[NSMutableArray alloc] init];
	}
	else
	{
		recentArray = [[NSMutableArray alloc] initWithArray:plist];
	}
}

// *** Tablesource stuffs.
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	// Number of sections is the number of region dictionaries
#ifdef WITH_ADS
	if(recentArray==nil) {
		return 1;
	} else {
		return 2;
	}
#else
  return 1;
#endif
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if(recentArray==nil) {
		return 1;
	}

#ifdef WITH_ADS	
	if(section < 1)
	{
		return 1;
	}
#endif
	return [recentArray count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
#ifdef WITH_ADS
	if(indexPath.section < 1) {
		return 55.0; // this is the height of the ad
	}
#endif
	return 44.0; // this is the generic cell height
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell;
#ifdef WITH_ADS
	if(indexPath.section < 1) 
	{
		cell = [tableview dequeueReusableCellWithIdentifier:@"adCell"];
		/*
		if(cell != nil)
		{
			[cell release];
			cell = nil;
		}
    */
		if(cell == nil)
		{
			cell = [[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"adCell"];
			// Request an ad for this table view cell
		  altAds = [SOApp.delegate getAdViewWithIndex:2];
			[cell.contentView addSubview:altAds];
		}
		else
		{
		  altAds = [SOApp.delegate getAdViewWithIndex:2];
			[cell.contentView addSubview:altAds];
		}
		
		cell.text = @"";
	}
	else
#endif
	{
		cell = [tableview dequeueReusableCellWithIdentifier:@"cell"];
		if (cell == nil) 
		{
			cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"cell"] autorelease];
			UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 320, 44)];
			label.numberOfLines = 1;
			label.adjustsFontSizeToFitWidth = YES;
			label.minimumFontSize = 9.0f;
			label.lineBreakMode = UILineBreakModeMiddleTruncation;
			label.tag = 1;
			[cell.contentView addSubview:label];
			[label release];
		}
		cell.accessoryType = UITableViewCellAccessoryNone;
		if([[[recentArray objectAtIndex:indexPath.row] objectForKey:@"path"] compare:@""] != NSOrderedSame)
		{
			UILabel* tempLabel = (UILabel *)[cell viewWithTag:1];
      tempLabel.text = [[[recentArray objectAtIndex:indexPath.row] objectForKey:@"path"] lastPathComponent];
		}
	}
	
	// Set up the cell
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
#ifdef WITH_ADS
	if( indexPath.section < 1 )
	{
		return;
	}
#endif

  [tableview deselectRowAtIndexPath:indexPath animated:YES];

	char *cListingsPath;
	if([[[recentArray objectAtIndex:indexPath.row] objectForKey:@"path"] compare:@""] != NSOrderedSame )
	{
		cListingsPath = (char*)[[[recentArray objectAtIndex:indexPath.row] objectForKey:@"path"] UTF8String];
	}
	
	
	[SOApp.nowPlayingView startEmu:cListingsPath];
	[SOApp.delegate switchToNowPlaying];
	//[tabBar didMoveToWindowNowPlaying];
}

@end
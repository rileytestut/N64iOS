#import "SOApplication.h"
#import <pthread.h>

/* Globals */
int				tArgc;
char**			tArgv;
pthread_t		main_tid;
pthread_attr_t  main_attr;
volatile int				__emulation_run = 0;
volatile int				__emulation_saving = 0;
volatile int       __emulation_paused = 0;

#ifdef WITH_ADS	
static const int section_offset = 1;
#else
static const int section_offset = 0;
#endif

@implementation RomController

- (void)awakeFromNib 
{
	self.navigationItem.hidesBackButton = YES;
	self.navigationItem.prompt = @"www.zodttd.com";
	
	UIBarButtonItem *moreButton = [[UIBarButtonItem alloc] initWithTitle:@"Search" style:UIBarButtonItemStylePlain target:self action:@selector(searchButtonClicked)];
	self.navigationItem.rightBarButtonItem = moreButton;
	[moreButton release];
	
	adNotReceived = 0;
  arrayOfCharacters = [[NSMutableArray alloc] init];
  objectsForCharacters = [[NSMutableDictionary alloc] init];
  
  alphabetIndex = [[NSArray arrayWithArray:
     [@"A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|#"
           componentsSeparatedByString:@"|"]] retain];
}

- (void)reload
{
  [tableview reloadData];
}

- (void)purchaseButtonClicked
{
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://www.rockyourphone.com/index.php/checkout/cart/add/uenc/,/product/207/"]]; 
}

- (void)searchButtonClicked
{
  if(![[NSFileManager defaultManager] fileExistsAtPath:@"/Applications/n64ios.app/searched"])
  {
    FILE* fp = fopen("/Applications/n64ios.app/searched", "w");
    if(fp)
    {
      fclose(fp);
    }
  	UIAlertView* alertView=[[UIAlertView alloc] initWithTitle:nil
                      message:@"This interface allows you to download .zip and N64 ROM files directly from the web into the correct directory for n64ios. You may only download ROMS you legally own. n64ios and ZodTTD are not affiliated/associated with any of these sites. Please do not contact us for support downloading roms. Simply browse a site, find a ROM you own, and press download - the file will download and will be available to play."
  										delegate:self cancelButtonTitle:@"OK"
                      otherButtonTitles:nil];
  	[alertView show];
  	[alertView release];
  }
  [SOApp.delegate switchToWebBrowserView];
}

- (void)startRootData:(float)secs
{
 	//[NSThread detachNewThreadSelector:@selector(initRootData) toTarget:self withObject:nil]; 

	NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval:secs
											 target:self
										   selector:@selector(initRootData)
										   userInfo:nil
											 repeats:NO];
}

- (void)initRootData
{
  //NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	currentPath = [[NSString alloc] initWithCString:get_documents_path("")]; // initWithString:@"/var/mobile/Media/ROMs/GBA/"];
	[ self refreshData:currentPath ];
	
  //[pool release];
}

- (void)refreshData:(NSString*)path 
{
  unsigned long numFiles = 0;
  NSString* prevPath;
	[arrayOfCharacters removeAllObjects];
	[objectsForCharacters removeAllObjects];
	prevPath = [[NSString alloc] initWithString:path];
	[currentPath release];
	if([[prevPath substringWithRange:NSMakeRange([prevPath length]-1,1)] compare:@"/"] == NSOrderedSame)
	{
    currentPath = [[NSString alloc] initWithFormat:@"%@",prevPath];
	}
	else
	{
		currentPath = [[NSString alloc] initWithFormat:@"%@/",prevPath];
	}

	int i;
	NSFileManager *fileManager = [NSFileManager defaultManager];
  NSArray* dirContents = [fileManager directoryContentsAtPath: currentPath];
	NSInteger entries = [dirContents count];
  int characterLUT[27];
  NSMutableArray* arrayOfIndexedFiles[27];
  
  for(i = 0; i < 27; i++)
  {
    characterLUT[i] = 0;
    arrayOfIndexedFiles[i] = [[NSMutableArray alloc] init];
  }
  
  for ( i = 0; i < entries; i++ ) 
  {
		if(
      ([[dirContents  objectAtIndex: i] length] < 4) ||
      (
		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-3,3)] caseInsensitiveCompare:@".7z"] != NSOrderedSame &&
		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-4,4)] caseInsensitiveCompare:@".bin"] != NSOrderedSame &&
		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-4,4)] caseInsensitiveCompare:@".zip"] != NSOrderedSame &&
		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-4,4)] caseInsensitiveCompare:@".n64"] != NSOrderedSame &&
		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-4,4)] caseInsensitiveCompare:@".v64"] != NSOrderedSame &&
 		   [[[dirContents  objectAtIndex: i ] substringWithRange:NSMakeRange([[dirContents  objectAtIndex: i ] length]-4,4)] caseInsensitiveCompare:@".z64"] != NSOrderedSame 
		  )
		)
		{
      // Do nothing currently.
		}
		else
		{
      NSString* objectTitle = [dirContents  objectAtIndex: i ];
      NSString* objectIndexer = [[objectTitle substringWithRange:NSMakeRange(0,1)] uppercaseString];
      NSUInteger objectIndex = [objectIndexer rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZ"]].location;

		  if(objectIndex == NSNotFound)
		  {
        objectIndex = 26;
		  }
		  else
		  {
        char* objectLetter = [objectIndexer UTF8String];
        objectIndex = objectLetter[0] - 'A';
  		  if(objectIndex > 25)
  		  {
          objectIndex = 25;
  		  }
		  }

		  if(characterLUT[objectIndex] == 0)
		  {
        characterLUT[objectIndex] = 1;
		  }
		  
      [arrayOfIndexedFiles[objectIndex] addObject:[dirContents objectAtIndex:i]];
      numFiles++;
		}
  }

  for(i = 0; i < 27; i++)
  {
    if(characterLUT[i] == 1)
    {
      NSString* characters = [NSString stringWithString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZ#"];
      NSString* characterIndex = [characters substringWithRange:NSMakeRange(i,1)];
      [arrayOfCharacters addObject:characterIndex];
      [objectsForCharacters setObject:arrayOfIndexedFiles[i] forKey:characterIndex];
    }
    [arrayOfIndexedFiles[i] release];
  }
  
  if(numFiles == 0)
  {
  	UIAlertView* alertView=[[UIAlertView alloc] initWithTitle:nil
                      message:@"No ROMs found.\nWould you like to find them?\n\n"
  										delegate:self cancelButtonTitle:nil
                      otherButtonTitles:@"NO",@"YES",nil];
  	[alertView show];
  	[alertView release];
  }
  
  [tableview reloadData];
    
  self.navigationItem.prompt = currentPath;  

  [prevPath release];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if(buttonIndex == 1)
  {
    // Yes
    [self searchButtonClicked];
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath 
{
	UITableViewCell*      cell;
  NSMutableDictionary*  objects;
  
  objects = objectsForCharacters;
  
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
		
		cell.accessoryType = UITableViewCellAccessoryNone;  	
		cell.text = @"";
	}
	else
#endif
  {
  	cell = [tableview dequeueReusableCellWithIdentifier:@"labelCell"];
  	if (cell == nil) 
  	{
  		cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:@"labelCell"] autorelease];
  		UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 320, 44)];
    	label.numberOfLines = 1;
    	label.adjustsFontSizeToFitWidth = YES;
    	label.minimumFontSize = 9.0f;
      label.tag = 1;
    	label.lineBreakMode = UILineBreakModeMiddleTruncation;
    	[cell.contentView addSubview:label];
    	[label release];
  	}
				
  	cell.accessoryType = UITableViewCellAccessoryNone;				
	
    if([arrayOfCharacters count] <= 0)
    {
      cell.text = @"";
      return cell;
    }
  
    UILabel* tempLabel = (UILabel *)[cell viewWithTag:1];
    tempLabel.text = [[objects objectForKey:[arrayOfCharacters objectAtIndex:indexPath.section-section_offset]] objectAtIndex:indexPath.row];
  }
	// Set up the cell
	return cell;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath 
{
#ifdef WITH_ADS
	if( indexPath.section < 1 )
	{
		return;
	}
#endif

  if([arrayOfCharacters count] <= 0)
  {
    return;
  }

  NSMutableDictionary*  objects;
  
  [tableview deselectRowAtIndexPath:indexPath animated:YES];
  
  objects = objectsForCharacters;

  NSMutableString *listingsPath = [[NSMutableString alloc] initWithCString:get_documents_path("")];
  if([[listingsPath substringWithRange:NSMakeRange([listingsPath length]-1,1)] compare:@"/"] == NSOrderedSame)
  {
    [listingsPath appendString:[[objects objectForKey:[arrayOfCharacters objectAtIndex:indexPath.section-section_offset]] objectAtIndex:indexPath.row]];
  }
  else
  {
    [listingsPath stringByAppendingPathComponent:[[objects objectForKey:[arrayOfCharacters objectAtIndex:indexPath.section-section_offset]] objectAtIndex:indexPath.row]];
  }

  [SOApp.recentView addRecent:listingsPath];

	char *cListingsPath = (char*)[listingsPath UTF8String];
  [SOApp.nowPlayingView startEmu:cListingsPath];

  [SOApp.delegate switchToNowPlaying];

  [listingsPath release];
}

// Override if you support editing the list
/*
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
		
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		// Delete the row from the data source
		[tableview deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
	}	
	if (editingStyle == UITableViewCellEditingStyleInsert) {
		// Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
	}	
}
*/


// Override if you support conditional editing of the list
/*
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	// Return NO if you do not want the specified item to be editable.
	return YES;
}
*/

// Override if you support rearranging the list
/*
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/

// Override if you support conditional rearranging of the list
/*
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
	// Return NO if you do not want the item to be re-orderable.
	return YES;
}
*/
/*
- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
}

- (void)viewDidDisappear:(BOOL)animated {
}
*/

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	// Return YES for supported orientations
	return NO; //YES; //(interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
	// Release anything that's not essential, such as cached data
}


- (void)dealloc {
	[currentPath release];
	[arrayOfCharacters release];
	[objectsForCharacters release];
  [alphabetIndex release];
	[super dealloc];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
#ifdef WITH_ADS
  if([arrayOfCharacters count] <= 0)
  {
    return 1;
  }
  return [arrayOfCharacters count] + 1;
#else
  if([arrayOfCharacters count] <= 0)
  {
    return 1;
  }
  return [arrayOfCharacters count];
#endif
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
#ifdef WITH_ADS	
	if(section < 1)
	{
		return 1;
	}
#endif

  if([arrayOfCharacters count] <= 0)
  {
    return 0;
  }

  NSMutableDictionary*  objects;
  
  objects = objectsForCharacters;

  return [[objects objectForKey:[arrayOfCharacters objectAtIndex:section-section_offset]] count];
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section 
{
#ifdef WITH_ADS
  if(section < 1)
  {
    return @"";
  }
#endif

  if([arrayOfCharacters count] <= 0)
    return @"";

  return [arrayOfCharacters objectAtIndex:section-section_offset];
}

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView 
{
  return alphabetIndex;
}


- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {	
  NSInteger count = 0;
  
  for(NSString *character in arrayOfCharacters)
  {
    if([character isEqualToString:title])
      return count;
    count++;
  }

  return 0; // in case of some eror donot crash d application
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
#ifdef WITH_ADS
	if(indexPath.section < 1) {
		return 55.0; // this is the height of the ad
	}
#endif
	return 44.0; // this is the generic cell height
}

@end


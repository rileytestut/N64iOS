//
//  N64MasterViewController.m
//  N64iOS
//
//  Created by Riley Testut on 6/7/12.
//  Copyright (c) 2012 Testut Tech. All rights reserved.
//

#import "N64MasterViewController.h"

#import "N64DetailViewController.h"

@interface N64MasterViewController () {
    NSInteger _currentSection;
}

@property (copy, nonatomic) NSMutableDictionary *roms;
@property (copy, nonatomic) NSArray *romSections;

@end

@implementation N64MasterViewController

@synthesize detailViewController = _detailViewController;

- (void)awakeFromNib
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        self.clearsSelectionOnViewWillAppear = NO;
        self.contentSizeForViewInPopover = CGSizeMake(320.0, 600.0);
    }
    [super awakeFromNib];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    self.navigationItem.leftBarButtonItem = self.editButtonItem;
    
    self.detailViewController = (N64DetailViewController *)[[self.splitViewController.viewControllers lastObject] topViewController];
    
    [self refreshROMsList];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

#pragma mark - ROMs

- (NSString *)romFilepathForIndexPath:(NSIndexPath *)indexPath {
    NSString *section = [self.romSections objectAtIndex:indexPath.section];
    return [[self.roms objectForKey:section] objectAtIndex:indexPath.row];
}

- (IBAction)refreshROMsList {
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectoryPath = [paths objectAtIndex:0];
    
    NSMutableDictionary *roms = [NSMutableDictionary dictionary];
    
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSArray *contents = [fileManager contentsOfDirectoryAtPath:documentsDirectoryPath error:nil];
    
    self.romSections = [NSArray arrayWithArray:[@"A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|#" componentsSeparatedByString:@"|"]];
    
    for (int i = 0; i < contents.count; i++) {
        NSString *filename = [contents objectAtIndex:i];
        if ([filename hasSuffix:@".Z64"] || [filename hasSuffix:@".z64"]) {
            NSString* characterIndex = [filename substringWithRange:NSMakeRange(0,1)];
            
            BOOL matched = NO;
            for (int i = 0; i < self.romSections.count && !matched; i++) {
                NSString *section = [self.romSections objectAtIndex:i];
                if ([section isEqualToString:characterIndex]) {
                    matched = YES;
                }
            }
            
            if (!matched) {
                characterIndex = @"#";
            }
            
            NSMutableArray *sectionArray = [roms objectForKey:characterIndex];
            if (sectionArray == nil) {
                sectionArray = [[NSMutableArray alloc] init];
            }
            [sectionArray addObject:filename];
            [roms setObject:sectionArray forKey:characterIndex];
        }
    }
    
    self.roms = roms;
    
    [self.tableView reloadData];
    
}

#pragma mark - Table View Data Source

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Cell"];
    
    NSString *romName = [self romFilepathForIndexPath:indexPath];
    romName = [romName stringByDeletingPathExtension];
    cell.textLabel.text = romName;
    
    return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    NSInteger numberOfSections = self.romSections.count;
    return numberOfSections > 0 ? numberOfSections : 1;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *sectionTitle = nil;
    if(self.romSections.count) {
        NSInteger numberOfRows = [self tableView:tableView numberOfRowsInSection:section];
        if (numberOfRows > 0) {
            sectionTitle = [self.romSections objectAtIndex:section];
        }
    }
    return sectionTitle;
}

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    NSMutableArray *sectionIndexTitles = nil;
    if(self.romSections.count) {
        sectionIndexTitles = [NSMutableArray arrayWithArray:[@"A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|#" componentsSeparatedByString:@"|"]];
    }
    return  sectionIndexTitles;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    _currentSection = index;
    return index;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    NSInteger numberOfRows = self.roms.count;
    if(self.romSections.count) {
        numberOfRows = [[self.roms objectForKey:[self.romSections objectAtIndex:section]] count];
    }
    return numberOfRows;
}


- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = ([paths count] > 0) ? [paths objectAtIndex:0] : nil;
        
        NSString *romName = [self romFilepathForIndexPath:indexPath];
        
        NSFileManager *fileManager = [[NSFileManager alloc] init];
        NSError *error = nil;
        [fileManager removeItemAtPath:[documentsDirectory stringByAppendingPathComponent:romName] error:&error];
        
        if (error) {
            NSLog(@"%@ %@", error, [error userInfo]);
        }
        
        [self refreshROMsList];
        
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }
}

/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
 {
 }
 */

/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"showDetail"]) {
        NSIndexPath *indexPath = [self.tableView indexPathForSelectedRow];
        NSString *romName = [self romFilepathForIndexPath:indexPath];
        [[segue destinationViewController] setRomName:romName];
    }
}

@end

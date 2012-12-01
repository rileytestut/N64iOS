#import "AltAds.h"
#include <sys/stat.h>

#define MY_OWN_URL		@""

@implementation AltAds

//*******************************************************************************************
// initWithFrame: This initializes the AltAds view.
//*******************************************************************************************
- (id) initWithFrame:(CGRect)frame andWindow:(UIWindow*)_window
{
	if (self = [super initWithFrame:frame]) 
	{
		UIView* SpacerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 320.0f, 55.0f)];
		SpacerView.backgroundColor = [UIColor clearColor];
		[self addSubview:SpacerView];
		[SpacerView release];
	
    [self startMyOwnAds];	

		if(_window != nil)
		{
			[_window addSubview:self];
		}
	}
	
  return self;
}

//*******************************************************************************************
// dealloc - class destructor.
//*******************************************************************************************
- (void) dealloc 
{
	if(AdView != nil)
	  [AdView release];
  [super dealloc];
}

//*******************************************************************************************
// RefreshAd - Call this to get a new ad.
//*******************************************************************************************
- (void) RefreshAd 
{
	if(AdView.loading == NO) 
	{
		NSURL* Url = [NSURL URLWithString:MY_OWN_URL];
		loadAdInFrame = YES;
		[AdView loadRequest:[NSURLRequest requestWithURL:Url]];
	}
}

//*******************************************************************************************
// webViewDidFinishLoad - This is the UIWebView Delegate for when a page has finished loading.
//                        Here we will add the view if its not added, we will reset the
//                        refresh timer to fetch the next ad.
//*******************************************************************************************
- (void) webViewDidFinishLoad:(UIWebView*) webView 
{
	static BOOL AddedAlready = NO;
	loadAdInFrame = NO;
	NSLog(@"AltAds received an ad\n");
	if(AddedAlready == NO) 
	{
		AddedAlready = YES;
		[self addSubview:AdView];
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSLog(@"Clicked\n");
}

//*******************************************************************************************
// didFailLoadWithError - Occurs when the page could not load.
//*******************************************************************************************
- (void) webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error 
{
	NSLog(@"Fail to fetch ad\n");
}

//*******************************************************************************************
// shouldStartLoadWithRequest - When the webview is going to load a page, this function runs.
//                              We'll use this to handle the "click".
//*******************************************************************************************
- (BOOL) webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType 
{
	BOOL ShouldLoad = NO;
		
	if(loadAdInFrame == YES)
	{
		ShouldLoad = YES;
	}
	else 
	{
		[[UIApplication sharedApplication] openURL: [request URL]];
	}
	
	return ShouldLoad;
}

//*******************************************************************************************
// Starts my own ads.
//*******************************************************************************************
- (void) startMyOwnAds
{
	NSLog(@"Starting my own\n");
	AdView = [[UIWebView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 55.0f)];
	[AdView setDelegate:self];
	[self RefreshAd];
}

@end

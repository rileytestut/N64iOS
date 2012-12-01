#import <UIKit/UIKit.h>

@interface AltAds : UIView <UIWebViewDelegate> 
{
	UIWebView*	AdView;
	BOOL				loadAdInFrame;
}
- (id)   initWithFrame:(CGRect)frame andWindow:(UIWindow*)_window;
- (void) webViewDidFinishLoad:(UIWebView*) webView;
- (void) webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (void) RefreshAd;
- (void) startMyOwnAds;

@end

#import <UIKit/UIKit.h>


@interface WebBrowserViewController : UIViewController < UIAlertViewDelegate >
{
	IBOutlet UIWebView*	      webView;
  UIAlertView*              downloadWaitAlertView;
  int                       isDownloading;
  NSURLRequest*             downloadRequest;
  NSString*                 downloadType;  
}

-(void)reloadButtonClicked;
-(void)startingDownload:(NSURLRequest*)request withType:(NSString*)type;
-(void)startDownload;

@end

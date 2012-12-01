#import <UIKit/UIKit.h>

@interface UIKeyboard : UIView
{
    UIImage *m_snapshot;
    UITextInputTraits *m_defaultTraits;
}

+ (UIKeyboard *)activeKeyboard;

- (void)updateLayout;

- (UIInterfaceOrientation)orientation;

- (void)removeAutocorrectPrompt;
- (void)acceptAutocorrection;

- (void)setCaretBlinks:(BOOL)newBlinks;
- (void)setCaretVisible:(BOOL)isVisible;

- (BOOL)returnKeyEnabled;
- (void)setReturnKeyEnabled:(BOOL)isEnabled;

- (id<UITextInputTraits>)defaultTextInputTraits;
- (void)setDefaultTextInputTraits:(id<UITextInputTraits>)defaultTraits;

- (id)delegate;

- (void)clearSnapshot;
- (void)takeSnapshot;

- (void)activate;
- (void)deactivate;

- (int)textEffectsVisibilityLevel;
@end
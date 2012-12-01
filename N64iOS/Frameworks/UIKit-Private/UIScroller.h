#import <UIKit/UIKit.h>

@class UIScrollerIndicator;

typedef struct {
    double width;
    double height;
} CGSizeDouble;


@interface UIScroller : UIView
{
    CGSize _contentSize;
    id _delegate;
    UIScrollerIndicator *_verticalScrollerIndicator;
    UIScrollerIndicator *_horizontalScrollerIndicator;
    struct {
        unsigned int bounceEnabled:1;
        unsigned int rubberBanding:1;
        unsigned int scrollingDisabled:1;
        unsigned int scrollingDisabledOnMouseDown:1;
        unsigned int directionalLockEnabled:1;
        unsigned int eventMode:3;
        unsigned int dragging:1;
        unsigned int mouseDragged:1;
        unsigned int scrollTriggered:1;
        unsigned int dontSelect:1;
        unsigned int contentHighlighted:1;
        unsigned int lockVertical:1;
        unsigned int lockHorizontal:1;
        unsigned int keepLocked:1;
        unsigned int bouncedVertical:1;
        unsigned int bouncedHorizontal:1;
        unsigned int mouseUpGuard:1;
        unsigned int pushedTrackingMode:1;
        unsigned int delegateScrollerDidScroll:1;
        unsigned int delegateScrollerAdjustSmoothScrollEndVelocity:1;
        unsigned int delegateScrollerShouldAdjustSmoothScrollEndForVelocity:1;
        unsigned int offsetIgnoresContentSize:1;
        unsigned int usingThumb:1;
        unsigned int thumbDetectionEnabled:1;
        unsigned int showScrollerIndicators:1;
        unsigned int indicatorSubrect:1;
        unsigned int indicatorHideInGesture:1;
        unsigned int pinIndicatorToContent:1;
        unsigned int indicatorStyle:2;
        unsigned int multipleDrag:1;
        unsigned int showBackgroundShadow:1;
        unsigned int cancelNextContentMouseUp:1;
        unsigned int displayingScrollIndicators:1;
        unsigned int dontResetStartTouchPosition:1;
        unsigned int verticalIndicatorShrunk:1;
        unsigned int horizontalIndicatorShrunk:1;
        unsigned int highlightContentImmediately:1;
        unsigned int adjustedEndOffset:1;
        unsigned int ignoreNextMouseDrag:1;
        unsigned int contentFitDisableScrolling:1;
        unsigned int dontScrollToTop:1;
        unsigned int scrollingToTop:1;
        unsigned int delegateScrollerAdjustScrollToTopEnd:1;
        unsigned int reserved:18;
    } _scrollerFlags;
    float _scrollHysteresis;
    float _scrollDecelerationFactor;
    double _scrollToPointAnimationDuration;
    float _directionalScrollingAngle;
    float _farthestDistance;
    float _leftRubberBandWidth;
    float _rightRubberBandWidth;
    float _topRubberBandHeight;
    float _bottomRubberBandHeight;
    float _bottomBufferHeight;
    CGPoint _initialTouchPosition;
    CGPoint _startTouchPosition;
    double _startTouchTime;
    CGPoint _startOffset;
    CGPoint _lastTouchPosition;
    double _lastTouchTime;
    double _lastUpdateTime;
    CGPoint _lastUpdateOffset;
    UIView *_lastHighlightedView;
    CGSizeDouble _velocity;
    CGSizeDouble _previousVelocity;
    CGSizeDouble _decelerationFactor;
    CGSizeDouble _decelerationLnFactor;
    CGPoint _stopOffset;
    void *_scrollHeartbeat;
    CGRect _indicatorSubrect;
    UIView *_scrollerShadows[2];
    UIView *_contentShadows[8];
    id _scrollNotificationViews;
    CGSize _gridSize;
    CGSizeDouble _gridBounceLnFactor;
}

- (void)setContentSize:(CGSize)size;
- (CGSize)contentSize;

- (void)setAdjustForContentSizeChange:(BOOL)adjustForContentSizeChange;
- (BOOL)adjustForContentSizeChange;

- (void)setOffset:(CGPoint)offset;
- (CGPoint)offset;

- (void)setBottomBufferHeight:(float)bottomBufferHeight;
- (float)bottomBufferHeight;

- (void)scrollByDelta:(CGSize)delta;
- (void)scrollByDelta:(CGSize)delta animated:(BOOL)animated;

- (void)_scrollAnimationEnded;
- (CGPoint)_pinnedScrollPointForPoint:(CGPoint)point;

- (void)scrollPointVisibleAtTopLeft:(CGPoint)point;
- (void)scrollPointVisibleAtTopLeft:(CGPoint)point animated:(BOOL)animated;
- (void)scrollRectToVisible:(CGRect)rect animated:(BOOL)animated;
- (void)scrollRectToVisible:(CGRect)rect;

- (void)setScrollToPointAnimationDuration:(double)duration;
- (double)scrollToPointAnimationDuration;

- (void)setScrollHysteresis:(float)hysteresis;
- (float)scrollHysteresis;

- (void)setEventMode:(int)eventMode;
- (int)eventMode;

- (void)setAllowsRubberBanding:(BOOL)allowsRubberBanding;
- (void)setAllowsVerticalRubberBanding:(BOOL)allowsVerticalRubberBanding;
- (void)setAllowsHorizontalRubberBanding:(BOOL)allowsHorizonalRubberBanding;
- (void)setAllowsFourWayRubberBanding:(BOOL)allowsFourWayRubberBanding;

- (void)setDirectionalScrolling:(BOOL)directionalScrolling;
- (BOOL)directionalScrolling;

- (void)setDirectionalScrollingAngle:(float)directionalScrollingAngle;
- (float)directionalScrollingAngle;

- (void)setScrollingEnabled:(BOOL)scrollingEnabled;
- (BOOL)scrollingEnabled;

- (void)setScrollDecelerationFactor:(float)scrollDecelerationFactor;
- (float)scrollDecelerationFactor;

- (void)setBounces:(BOOL)bounces;
- (BOOL)bounces;

- (void)setGridSize:(CGSize)gridSize;
- (CGSize)gridSize;

- (void)setThumbDetectionEnabled:(BOOL)thumbDetectionEnabled;
- (BOOL)thumbDetectionEnabled;

- (void)setShowScrollerIndicators:(BOOL)showScrollerIndicators;
- (BOOL)showScrollerIndicators;

- (void)setScrollerIndicatorSubrect:(CGRect)scrollerIndicatorSubrect;
- (CGRect)scrollerIndicatorSubrect;

- (void)setScrollerIndicatorsPinToContent:(BOOL)pinToContent;
- (BOOL)scrollerIndicatorsPinToContent;

- (void)setScrollerIndicatorStyle:(int)indicatorStyle;
- (int)scrollerIndicatorStyle;

- (void)displayScrollerIndicators;

- (void)setRubberBand:(float)rubberBand forEdges:(unsigned int)edges;
- (float)rubberBandValueForEdge:(unsigned int)edge;
- (BOOL)releaseRubberBandIfNecessary;

- (void)setDelegate:(id)delegate;
- (id)delegate;

- (void)setShowBackgroundShadow:(BOOL)showBackgroundShadow;
- (BOOL)showBackgroundShadow;

- (void)setOffsetForDragOffset:(CGPoint)dragOffset withEvent:(/*GSEvent*/ id *)gsEvent duration:(float)duration;
- (CGPoint)dragStartOffset;

- (void)_popTrackingRunLoopMode;

- (void)cancelNextContentMouseUp;
- (void)contentMouseUpInView:(UIView *)view withEvent:(void *)event;

- (void)highlightView:(UIView *)view state:(BOOL)highlighted;
- (void)setHighlightContentImmediately:(BOOL)immediately;

- (CGSizeDouble)velocity;

- (BOOL)isScrolling;
- (BOOL)isDecelerating;

- (BOOL)adjustSmoothScrollEnd:(CGSizeDouble)smoothScrollEnd;

- (BOOL)scrollsToTop;
- (void)setScrollsToTop:(BOOL)scrollsToTop;

- (BOOL)_scrollToTop;

@end
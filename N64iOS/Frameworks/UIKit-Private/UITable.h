#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "UIScroller.h"

typedef struct { // TODO: figure out which margin is which
    float _field1;
    float _field2;
    float _field3;
    float _field4;
} UIMargins;


@class UIFloatArray, UITableCountView, _UITableDeleteAnimationSupport, _UITableReorderingSupport;

@interface UITable : UIScroller
{
    id _dataSource;
    NSMutableArray *_tableColumns;
    SEL _doubleAction;
    UIFloatArray *_rowHeights;
    struct _NSRange _visibleRows;
    struct _NSRange _visibleCols;
    NSMutableArray *_visibleCells;
    _UITableDeleteAnimationSupport *_deleteAnimationSupport;
    unsigned int _selectedRow;
    unsigned int _lastHighlightedRow;
    int _rowCount;
    int _tableReloadingSuspendedCount;
    float _padding;
    UIView *_accessoryView;
    UITableCountView *_countLabel;
    NSMutableArray *_reusableTableCells;
    int _reusableCapacity;
    NSMutableArray *_extraSeparators;
    int _swipeToDeleteRow;
    struct {
        unsigned int separatorStyle:3;
        unsigned int rowDeletionEnabled:1;
        unsigned int allowSelectionDuringRowDeletion:1;
        unsigned int dataSourceHeightForRow:1;
        unsigned int dataSourceSetObjectValue:1;
        unsigned int dataShowDisclosureForRow:1;
        unsigned int dataDisclosureClickableForRow:1;
        unsigned int dataSourceWantsHints:1;
        unsigned int dataSourceCanDeleteRow:1;
        unsigned int dataSourceConfirmDeleteRow:1;
        unsigned int delegateTableSelectionDidChange:1;
        unsigned int scrollsToSelection:1;
        unsigned int reloadSkippedDuringSuspension:1;
        unsigned int reuseTableCells:1;
        unsigned int delegateUpdateVisibleCellsNote:1;
        unsigned int delegateTableRowSelected:1;
        unsigned int rowAlreadyHighlighted:1;
        unsigned int needsReload:1;
        unsigned int delegateCanSwipe:1;
        unsigned int updatingVisibleCellsManually:1;
        unsigned int scheduledUpdateVisibleCells:1;
        unsigned int warnForForcedCellUpdateDisabled:1;
        unsigned int delaySendingSelectionChanged:1;
        unsigned int dataSourceCanInsertAtRow:1;
        unsigned int shouldDisplayTopSeparator:1;
        unsigned int displayTopSeparator:1;
        unsigned int needToAdjustExtraSeparators:1;
        unsigned int ignoreDragSwipe:1;
        unsigned int lastHighlightedRowActive:1;
        unsigned int reloading:1;
        unsigned int countStringInsignificantRowCount:4;
        unsigned int dataSourceCanReuseCell:1;
        unsigned int reserved:27;
    } _tableFlags;
    _UITableReorderingSupport *_reorderingSupport;
}

- (BOOL)validateDataSource;
- (void)setDataSource:(id)dataSource;
- (id)dataSource;

- (void)setDelegate:(id)delegate;

- (NSInteger)dataSourceGetRowCount;
- (BOOL)dataSourceSupportsVariableRowHeights;
- (NSInteger)numberOfRows;

- (NSMutableArray *)tableColumns;
- (NSInteger)numberOfColumns;
- (void)addTableColumn:(id)tableColumn;
- (void)removeTableColumn:(id)tableColumn;
- (int)columnWithIdentifier:(id)identifier;
- (id)tableColumnWithIdentifier:(id)identifier;

- (void)setRowHeight:(float)rowHeight;
- (float)rowHeight;

- (void)scrollRowToVisible:(NSInteger)row;

- (void)clearAllData;
- (void)_updateOriginOfCells:(NSRange)range;
- (void)reloadDataForInsertionOfRows:(NSRange)range;
- (void)reloadDataForInsertionOfRows:(NSRange)range animated:(BOOL)animated;
- (void)reloadCellAtRow:(NSInteger)row column:(NSInteger)column animated:(BOOL)animated;
- (float)animationDuration;
- (void)reloadData;
- (void)noteNumberOfRowsChanged;
- (void)setNeedsDisplayInRowRange:(NSRange)range;

- (void)setDoubleAction:(SEL)doubleAction;
- (SEL)doubleAction;

- (void)setCountStringInsignificantRowCount:(NSUInteger)count;
- (void)setCountString:(NSString *)countString;
- (void)animateDeletionOfCellAtRow:(NSInteger)row column:(NSInteger)column viaEdge:(NSInteger)edge;
- (void)animateDeletionOfRowWithCell:(id)cell viaEdge:(int)edge animatingOthersUp:(BOOL)animated;
- (void)animateDeletionOfRowWithCell:(id)cell viaEdge:(int)edge;
- (void)animateDeletionOfRowWithCell:(id)cell;
- (void)completeRowDeletionAnimation;
- (BOOL)canDeleteRow:(NSInteger)row;
- (BOOL)canInsertAtRow:(NSInteger)row;
- (void)enableRowDeletion:(BOOL)rowDeletion;
- (BOOL)_userCanDeleteRows;
- (void)_enableRowDeletion:(BOOL)rowDeletion forCell:(id)cell atRow:(int)row allowInsert:(BOOL)allowInsert allowReorder:(BOOL)allowReorder animated:(BOOL)animated;

// TODO: finish this
- (int)deleteConfirmationRow;
- (void)setDeleteConfirmationRow:(int)fp8;
- (void)removeControlWillHideRemoveConfirmation:(id)fp8;
- (void)enableRowDeletion:(BOOL)fp8 animated:(BOOL)fp12;
- (void)_removeContextFromSuperview:(id)fp8 finished:(id)fp12 context:(id)fp16;
- (void)_disableInteraction;
- (void)_enableInteraction;
- (BOOL)isRowDeletionEnabled;
- (void)setAllowSelectionDuringRowDeletion:(BOOL)fp8;
- (void)updateDisclosures;
- (struct CGRect)frameOfCellAtRow:(int)fp8 column:(int)fp12;
- (struct CGRect)frameOfCellAtColumn:(int)fp8 row:(int)fp12;
- (struct CGRect)rectOfViewAtColumn:(int)fp8 row:(int)fp12;
- (id)viewAtColumn:(int)fp8 row:(int)fp12;
- (id)cellAtRow:(int)fp8 column:(int)fp12;
- (void)setAllowsReordering:(BOOL)fp8;
- (id)dataSourceCreateCellForRow:(int)fp8 column:(int)fp12 reusing:(id)fp16;
- (BOOL)shouldIndentRow:(int)fp8;
- (id)createPreparedCellForRow:(int)fp8 column:(int)fp12;
- (struct CGRect)rectOfColumn:(int)fp8;
- (struct CGRect)_rectOfRow:(int)fp8 usingRowHeights:(id)fp12;
- (struct CGRect)rectOfRow:(int)fp8;
- (struct _NSRange)columnsInRect:(struct CGRect)fp8;
- (struct _NSRange)rowsInRect:(struct CGRect)fp8;
- (int)columnAtPoint:(struct CGPoint)fp8;
- (int)rowAtPoint:(struct CGPoint)fp8;
- (id)visibleCellsWithoutUpdatingLayout;
- (id)visibleCells;
- (id)visibleCellForRow:(int)fp8 column:(int)fp12;
- (BOOL)getRow:(int *)fp8 column:(int *)fp12 ofTableCell:(id)fp16;
- (struct _NSRange)visibleRowsInRect:(struct CGRect)fp8;
- (void)setOffset:(struct CGPoint)fp8;
- (void)setFrame:(struct CGRect)fp8;
- (void)_userSelectRow:(int)fp8;
- (void)selectRow:(int)fp8 byExtendingSelection:(BOOL)fp12;
- (void)_sendSelectionDidChange;
- (void)_delaySendSelectionDidChange;
- (void)removeFromSuperview;
- (BOOL)cancelMouseTracking;
- (BOOL)cancelTouchTracking;
- (void)selectCell:(id)fp8 inRow:(int)fp12 column:(int)fp16 withFade:(BOOL)fp20;
- (void)_selectRow:(int)fp8 byExtendingSelection:(BOOL)fp12 withFade:(BOOL)fp16 scrollingToVisible:(BOOL)fp20 withSelectionNotifications:(BOOL)fp24;
- (void)selectRow:(int)fp8 byExtendingSelection:(BOOL)fp12 withFade:(BOOL)fp16 scrollingToVisible:(BOOL)fp20;
- (void)selectRow:(int)fp8 byExtendingSelection:(BOOL)fp12 withFade:(BOOL)fp16;
- (BOOL)shouldDelaySendingSelectionChanged;
- (int)selectedRow;
- (int)lastHighlightedRow;
- (BOOL)highlightRow:(int)fp8;
- (BOOL)highlightNextRowByDelta:(int)fp8;
- (BOOL)selectHighlightedRow;
- (void)setScrollsToSelection:(BOOL)fp8;
- (void)setSeparatorStyle:(int)fp8;
- (int)separatorStyle;
- (void)setPadding:(float)fp8;
- (UIMargins)adornmentMargins;
- (UIMargins)pressedAdornmentMargins;
- (id)hitTest:(struct CGPoint)fp8 forEvent:(struct __GSEvent *)fp16;
- (BOOL)canSelectRow:(int)fp8;
- (BOOL)_checkCanSelectRow:(int)fp8 view:(id)fp12;
- (void)contentMouseUpInView:(id)fp8 withEvent:(struct __GSEvent *)fp12;
- (void)highlightView:(id)fp8 state:(BOOL)fp12;
- (void)setAccessoryView:(id)fp8;
- (id)accessoryView;
- (void)drawExtraSeparator:(struct CGRect)fp8;
- (void)setResusesTableCells:(BOOL)fp8;
- (void)setReusesTableCells:(BOOL)fp8;
- (void)scrollAndCenterTableCell:(id)fp8 animated:(BOOL)fp12;
- (void)_updateContentSize;
- (BOOL)floatArray:(id)fp8 loadValues:(float *)fp12 count:(int)fp16;
- (void)floatArray:(id)fp8 getValueCount:(int *)fp12 gapIndexCount:(int *)fp16;
- (void)_reloadRowHeights;
- (void)_addSubview:(id)fp8 atTop:(BOOL)fp12;
- (void)layoutSubviews;
- (void)_stopAutoscrollTimer;
- (void)_beginReorderingForCell:(id)fp8;
- (void)_autoscroll:(id)fp8;
- (void)_reorderPositionChangedForCell:(id)fp8;
- (void)_finishedAnimatingCellReorder:(id)fp8 finished:(id)fp12 context:(id)fp16;
- (void)_endCellReorderAnimation;
- (void)_tableCellAnimationDidStop:(id)fp8 finished:(id)fp12;
- (void)_endReorderingForCell:(id)fp8;
- (void)_setNeedsVisibleCellsUpdate:(BOOL)fp8;
- (BOOL)canHandleSwipes;
- (int)swipe:(int)fp8 withEvent:(struct __GSEvent *)fp12;
- (void)_updateVisibleCellsNow;
- (void)_updateVisibleCellsImmediatelyIfNecessary;
- (void)_suspendReloads;
- (void)_resumeReloads;
- (int)_rowForTableCell:(id)fp8;
- (void)_deleteRowAlertDidEndWithResult:(BOOL)fp8 contextInfo:(id)fp12;
- (void)_deleteRowAlertDidEndContinuation:(id)fp8;
- (BOOL)_shouldDeleteRowForTableCell:(id)fp8;
- (void)_animateRowsForDeletionOfRow:(int)fp8 withSep:(id)fp12;
- (void)_saveTableStateBeforeAnimationForRow:(int)fp8;
- (void)_restoreTableStateAfterAnimation;
- (int)_removeFromVisibleRows:(id)fp8;
- (void)_getRowCount:(int *)fp8 andHeight:(float *)fp12 beforeVisibleCellsForRows:(id)fp16;
- (void)_animateRemovalOfCell:(id)fp8 atRow:(int)fp12 col:(int)fp16 viaEdge:(int)fp20 withAmountToSlideUp:(float *)fp24;
- (void)_animateRemovalOfVisibleRows:(id)fp8 viaEdge:(int)fp12 withAmountToSlideUp:(float *)fp16;
- (void)_animateNewCells:(id)fp8 bySlidingUpAmount:(float)fp12;
- (void)_animateNewCells:(id)fp8 bySlidingDownAmount:(float)fp12;
- (float)_partOfRow:(int)fp8 thatIsHidden:(BOOL)fp12;
- (void)deleteRows:(id)fp8 viaEdge:(int)fp12 animated:(BOOL)fp16;
- (void)deleteRows:(id)fp8 viaEdge:(int)fp12;
- (void)_fadeCellOutAnimationDidStop:(id)fp8 finished:(id)fp12 context:(id)fp16;
- (void)insertRows:(id)fp8 deleteRows:(id)fp12 reloadRows:(id)fp16;
- (void)_willDeleteRow:(int)fp8 forTableCell:(id)fp12 viaEdge:(int)fp16 animateOthers:(BOOL)fp20;
- (void)_enableAndRestoreTableStateAfterAnimation;
- (void)_finishedRemovingRemovalButtonForTableCell:(id)fp8;
- (void)_didDeleteRowForTableCell:(id)fp8;
- (void)_didInsertRowForTableCell:(id)fp8;
- (void)touchesBegan:(id)fp8 withEvent:(id)fp12;
- (void)touchesMoved:(id)fp8 withEvent:(id)fp12;
- (void)touchesEnded:(id)fp8 withEvent:(id)fp12;
- (void)mouseDown:(struct __GSEvent *)fp8;
- (void)mouseDragged:(struct __GSEvent *)fp8;
- (BOOL)_shouldTryPromoteDescendantToFirstResponder;
- (void)_scheduleAdjustExtraSeparators;
- (void)_adjustExtraSeparators;
- (unsigned int)_countStringRowCount;
- (void)_setRowCount:(int)fp8;
- (void)_adjustCountLabel;
- (void)_adjustReusableTableCells;

@end
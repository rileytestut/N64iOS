#ifndef GRAPHICSSERVICES_H
#define GRAPHICSSERVICES_H

#define kGSEventTypeOneFingerDown   1
#define kGSEventTypeAllFingersUp    2
#define kGSEventTypeOneFingerUp     5
/* A "gesture" is either one finger dragging or two fingers down. */
#define kGSEventTypeGesture         6

#ifdef __cplusplus
extern "C" {
#endif
    
    struct __GSEvent;
    typedef struct __GSEvent GSEvent;
    typedef GSEvent *GSEventRef;
    
#import "CoreGraphics/CGGeometry.h"
    //struct CGPoint;
    //typedef struct CGPoint CGPoint;
    
    int GSEventIsChordingHandEvent(GSEvent *ev);
    int GSEventGetClickCount(GSEvent *ev);
    CGPoint GSEventGetLocationInWindow(GSEvent *ev);
    float GSEventGetDeltaX(GSEvent *ev); 
    float GSEventGetDeltaY(GSEvent *ev); 
    CGPoint GSEventGetInnerMostPathPosition(GSEvent *ev);
    CGPoint GSEventGetOuterMostPathPosition(GSEvent *ev);
    unsigned int GSEventGetSubType(GSEvent *ev);
    unsigned int GSEventGetType(GSEvent *ev);
    unsigned int GSEventDeviceOrientation(GSEvent *ev);
    
#ifdef __cplusplus
}
#endif

#endif
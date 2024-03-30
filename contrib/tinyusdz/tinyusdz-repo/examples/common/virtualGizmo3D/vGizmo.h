//------------------------------------------------------------------------------
//  Copyright (c) 2018-2020 Michele Morrone
//  All rights reserved.
//
//  https://michelemorrone.eu - https://BrutPitt.com
//
//  twitter: https://twitter.com/BrutPitt - github: https://github.com/BrutPitt
//
//  mailto:brutpitt@gmail.com - mailto:me@michelemorrone.eu
//  
//  This software is distributed under the terms of the BSD 2-Clause license
//------------------------------------------------------------------------------
#pragma once

#define VGIZMO_H_FILE
#include "vgMath.h"

#ifdef VGM_USES_TEMPLATE
    #define VGIZMO_BASE_CLASS virtualGizmoBaseClass<T>
    #define imGuIZMO_BASE_CLASS virtualGizmoBaseClass<float>
#else
    #define VGIZMO_BASE_CLASS virtualGizmoBaseClass
    #define imGuIZMO_BASE_CLASS VGIZMO_BASE_CLASS
    #define T VG_T_TYPE
#endif

typedef int vgButtons;
typedef int vgModifiers;

namespace vg {
//  Default values for button and modifiers.
//      This values are aligned with GLFW defines (for my comfort),
//      but they are loose from any platform library: simply initialize
//      the virtualGizmo with your values: 
//          look at "onInit" in glWindow.cpp example.
//////////////////////////////////////////////////////////////////////
    enum {
        evLeftButton  ,
        evRightButton ,
        evMiddleButton
    };

    enum {
        evNoModifier      =  0,
        evShiftModifier   =  1   ,
        evControlModifier =  1<<1,
        evAltModifier     =  1<<2,
        evSuperModifier   =  1<<3  
    };

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
//  Base manipulator class
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
TEMPLATE_TYPENAME_T class virtualGizmoBaseClass {

public:
    virtualGizmoBaseClass() : tbActive(false), pos(0), delta(0),
                           tbControlButton(evLeftButton), tbControlModifiers(evNoModifier),
                           tbRotationButton(evRightButton), xRotationModifier(evShiftModifier),
                           yRotationModifier(evControlModifier),  zRotationModifier(evAltModifier|evSuperModifier)
    { 
        viewportSize(T(256), T(256));  //initial dummy value
    }
    virtual ~virtualGizmoBaseClass() {}

    //    Call to initialize and on reshape
    ////////////////////////////////////////////////////////////////////////////
    void viewportSize(int w, int h) { viewportSize(T(w), T(h)); }
    void viewportSize(T w, T h) { 
        width = w; height = h; 
        minVal = T(width < height ? width*T(0.5) : height*T(0.5));
        offset = tVec3(T(.5 * width), T(.5 * height), T(0));
    }

    void inline activateMouse(T x, T y) {
        pos.x = x;
        pos.y = y;
        delta.x = delta.y = 0;
    }
    void inline deactivateMouse() {
        if(delta.x == 0 && delta.y == 0) update();
        delta.x = delta.y = 0;
    }
    void inline testRotModifier(int x, int y, vgModifiers mod) { }
    
    //    Call on mouse button event
    //      button:  your mouse button
    //      mod:     your modifier key -> CTRL, SHIFT, ALT, SUPER
    //      pressed: if Button is pressed (TRUE) or released (FALSE)
    //      x, y:    mouse coordinates
    ////////////////////////////////////////////////////////////////////////////
    virtual void mouse( vgButtons button, vgModifiers mod, bool pressed, T x, T y)
    {
        if ( (button == tbControlButton) && pressed && (tbControlModifiers ? tbControlModifiers & mod : tbControlModifiers == mod) ) {
            tbActive = true;
            activateMouse(x,y);
        }
        else if ( button == tbControlButton && !pressed) {
            deactivateMouse();
            tbActive = false;
        }

        if((button == tbRotationButton) && pressed) {
            if      (xRotationModifier & mod) { tbActive = true; rotationVector = tVec3(T(1), T(0), T(0)); activateMouse(x,y); }
            else if (yRotationModifier & mod) { tbActive = true; rotationVector = tVec3(T(0), T(1), T(0)); activateMouse(x,y); }
            else if (zRotationModifier & mod) { tbActive = true; rotationVector = tVec3(T(0), T(0), T(1)); activateMouse(x,y); }
        } else if((button == tbRotationButton) && !pressed) { 
            deactivateMouse(); rotationVector = tVec3(T(1)); tbActive = false; 
        }
    
    }

    //    Call on Mouse motion
    ////////////////////////////////////////////////////////////////////////////
    virtual void motion( T x, T y) {
        delta.x = x - pos.x;   delta.y = pos.y - y;
        pos.x = x;   pos.y = y;
        update();
    }
    //    Call on Pinching
    ////////////////////////////////////////////////////////////////////////////
    void pinching(T d, T z = T(0)) {
        delta.y = d * z;
        update();
    }

    //    Call every rendering to implement continue spin rotation 
    ////////////////////////////////////////////////////////////////////////////
    void idle() { qtV = qtIdle*qtV;  }

    //    Call after changed settings
    ////////////////////////////////////////////////////////////////////////////
    virtual void update() = 0;
    void updateGizmo() 
    {

        if(!delta.x && !delta.y) {
            qtIdle = qtStep = tQuat(T(1), T(0), T(0), T(0)); //no rotation
            return;
        }

        tVec3 a(T(pos.x-delta.x), T(height - (pos.y+delta.y)), T(0));
        tVec3 b(T(pos.x    ),     T(height -  pos.y         ), T(0));

        auto vecFromPos = [&] (tVec3 &v) {
            v -= offset;
            v /= minVal;
            const T len = length(v);
            v.z = len>T(0) ? pow(T(2), -T(.5) * len) : T(1);
            v = normalize(v);
        };

        vecFromPos(a);
        vecFromPos(b);

        tVec3 axis = cross(a, b);
        axis = normalize(axis);

        T AdotB = dot(a, b); 
        T angle = acos( AdotB>T(1) ? T(1) : (AdotB<-T(1) ? -T(1) : AdotB)); // clamp need!!! corss float is approximate to FLT_EPSILON

        qtStep = normalize(angleAxis(angle * tbScale * fpsRatio                  , axis * rotationVector));
        qtIdle = normalize(angleAxis(angle * tbScale * fpsRatio * qIdleSpeedRatio, axis * rotationVector));
        qtV = qtStep*qtV;

    }

    //  Set the sensitivity for the virtualGizmo.
    //////////////////////////////////////////////////////////////////
    void setGizmoFeeling( T scale) { tbScale = scale; }
    //  Call with current fps (every rendering) to adjust "auto" sensitivity
    //////////////////////////////////////////////////////////////////
    void setGizmoFPS(T fps) { fpsRatio = T(60.0)/fps;}

    //  Apply rotation
    //////////////////////////////////////////////////////////////////
    inline void applyRotation(tMat4 &m) { m = m * mat4_cast(qtV); }                                     

    //  Set the point around which the virtualGizmo will rotate.
    //////////////////////////////////////////////////////////////////
    void setRotationCenter( const tVec3& c) { rotationCenter = c; }
    tVec3& getRotationCenter() { return rotationCenter; }

    //  Set the mouse button and modifier for rotation 
    //////////////////////////////////////////////////////////////////
    void setGizmoRotControl( vgButtons b, vgModifiers m = evNoModifier) {
        tbControlButton = b;
        tbControlModifiers = m;
    }
    //  Set the mouse button and modifier for rotation around X axis
    //////////////////////////////////////////////////////////////////
    void setGizmoRotXControl( vgButtons b, vgModifiers m = evNoModifier) {
        tbRotationButton = b;
        xRotationModifier = m;
    }
    //  Set the mouse button and modifier for rotation around Y axis
    //////////////////////////////////////////////////////////////////
    void setGizmoRotYControl( vgButtons b, vgModifiers m = evNoModifier) {
        tbRotationButton = b;
        yRotationModifier = m;
    }
    //  Set the mouse button and modifier for rotation around Z axis
    //////////////////////////////////////////////////////////////////
    void setGizmoRotZControl( vgButtons b, vgModifiers m = evNoModifier) {
        tbRotationButton = b;
        zRotationModifier = m;
    }

    //  get the rotation quaternion
    //////////////////////////////////////////////////////////////////
    virtual tQuat &getRotation() { return qtV; }

    //  get the rotation increment
    //////////////////////////////////////////////////////////////////
    tQuat &getStepRotation() { return qtStep; }

    //  get the rotation quaternion
    //////////////////////////////////////////////////////////////////
    void setRotation(const tQuat &q) { qtV = q; }

    //  get the rotation increment
    //////////////////////////////////////////////////////////////////
    void setStepRotation(const tQuat &q) { qtStep = q; }

    // attenuation<1.0 / increment>1.0 of rotation speed in idle
    ////////////////////////////////////////////////////////////////////////////
    void setIdleRotSpeed(T f) { qIdleSpeedRatio = f;    }
    T    getIdleRotSpeed()    { return qIdleSpeedRatio; }

    //  return current transformations as 4x4 matrix.
    ////////////////////////////////////////////////////////////////////////////
    virtual tMat4 getTransform() = 0;
    ////////////////////////////////////////////////////////////////////////////
    virtual void applyTransform(tMat4 &model) = 0;

// Immediate mode helpers
//////////////////////////////////////////////////////////////////////

    // for imGuIZMO or immediate mode control
    //////////////////////////////////////////////////////////////////
    void motionImmediateLeftButton( T x, T y, T dx, T dy) {
        tbActive = true;
        delta = tVec2(dx,-dy);
        pos   = tVec2( x,  y);
        update();
    }
    //  for imGuIZMO or immediate mode control
    //////////////////////////////////////////////////////////////////
    virtual void motionImmediateMode( T x, T y, T dx, T dy,  vgModifiers mod) {
        tbActive = true;
        delta = tVec2(dx,-dy);
        pos   = tVec2( x,  y);
        if      (xRotationModifier & mod) { rotationVector = tVec3(T(1), T(0), T(0)); }
        else if (yRotationModifier & mod) { rotationVector = tVec3(T(0), T(1), T(0)); }
        else if (zRotationModifier & mod) { rotationVector = tVec3(T(0), T(0), T(1)); }
        update();
    }

protected:

    tVec2 pos, delta;

    // UI commands that this virtualGizmo responds to (defaults to left mouse button with no modifier key)
    vgButtons   tbControlButton, tbRotationButton;   
    vgModifiers tbControlModifiers, xRotationModifier, yRotationModifier, zRotationModifier;

    tVec3 rotationVector = tVec3(T(1));

    tQuat qtV    = tQuat(T(1), T(0), T(0), T(0));
    tQuat qtStep = tQuat(T(1), T(0), T(0), T(0));
    tQuat qtIdle = tQuat(T(1), T(0), T(0), T(0));

    tVec3 rotationCenter = tVec3(T(0));

    //  settings for the sensitivity
    //////////////////////////////////////////////////////////////////
    T tbScale = T(1);    //base scale sensibility
    T fpsRatio = T(1);   //auto adjust by FPS (call idle with current FPS)
    T qIdleSpeedRatio = T(.33); //autoRotation factor to speedup/slowdown
    
    T minVal;
    tVec3 offset;

    bool tbActive;  // trackbal activated via mouse

    T width, height;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// virtualGizmoClass
//  trackball: simple mouse rotation control 
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
TEMPLATE_TYPENAME_T class virtualGizmoClass : public VGIZMO_BASE_CLASS {

public:

    virtualGizmoClass()  { }

    //////////////////////////////////////////////////////////////////
    void motion( T x, T y) { if(this->tbActive) VGIZMO_BASE_CLASS::motion(x,y); }

    //////////////////////////////////////////////////////////////////
    void update() { this->updateGizmo(); }

    //////////////////////////////////////////////////////////////////
    void applyTransform(tMat4 &model) {
        model = translate(model, -this->rotationCenter);
        VGIZMO_BASE_CLASS::applyRotation(model);
        model = translate(model, this->rotationCenter);
    }

    //////////////////////////////////////////////////////////////////
    tMat4 getTransform() {
        tMat4 trans, invTrans, rotation;
        rotation = mat4_cast(this->qtV);

        trans = translate(tMat4(T(1)),this->rotationCenter);
        invTrans = translate(tMat4(T(1)),-this->rotationCenter);
        
        return invTrans * rotation * trans;
    }

    //  Set the speed for the virtualGizmo.
    //////////////////////////////////////////////////////////////////
    //void setGizmoScale( T scale) { scale = scale; }

    // get the rotation quaternion
    tQuat &getRotation() { return this->qtV; }
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// virtualGizmo3DClass
//  3D trackball: rotation interface with pan and dolly operations
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
TEMPLATE_TYPENAME_T class virtualGizmo3DClass : public VGIZMO_BASE_CLASS {

using VGIZMO_BASE_CLASS::delta;
using VGIZMO_BASE_CLASS::qtV; 

public:
    //////////////////////////////////////////////////////////////////
    virtualGizmo3DClass() : dollyControlButton(evRightButton),   panControlButton(evMiddleButton),  dollyActive(false),
                         dollyControlModifiers(evNoModifier), panControlModifiers(evNoModifier), panActive(false) { }

    //////////////////////////////////////////////////////////////////
    void mouse( vgButtons button, vgModifiers mod, bool pressed, int x, int y) { mouse(button, mod, pressed, T(x), T(y)); }
    void mouse( vgButtons button, vgModifiers mod, bool pressed, T x, T y)
    {
        VGIZMO_BASE_CLASS::mouse(button, mod, pressed,  x,  y);
        if ( button == dollyControlButton && pressed && (dollyControlModifiers ? dollyControlModifiers & mod : dollyControlModifiers == mod) ) {
            dollyActive = true;
            this->activateMouse(x,y);
        }
        else if ( button == dollyControlButton && !pressed) {
            this->deactivateMouse();
            dollyActive = false;
        }
        
        if ( button == panControlButton && pressed && (panControlModifiers ? panControlModifiers & mod : panControlModifiers == mod) ) {
            panActive = true;
            this->activateMouse(x,y);
        }
        else if ( button == panControlButton && !pressed) {
            this->deactivateMouse();
            panActive = false;
        }
    }

    //    Call on wheel (only for Dolly/Zoom)
    ////////////////////////////////////////////////////////////////////////////
    void wheel( T x, T y, T z=T(0)) {
        povPanDollyFactor = z;
        dolly.z += y * dollyScale * wheelScale * (povPanDollyFactor>T(0) ? povPanDollyFactor : T(1));
    }

    //////////////////////////////////////////////////////////////////
    void motion( int x, int y, T z=T(0)) { motion( T(x), T(y), z); }
    void motion( T x, T y, T z=T(0)) {
        povPanDollyFactor = z;
        if( this->tbActive || dollyActive || panActive) VGIZMO_BASE_CLASS::motion(x,y);
    }

    //////////////////////////////////////////////////////////////////
    void updatePan() {
        tVec3 v(delta.x, delta.y, T(0));
        pan += v * panScale * (povPanDollyFactor>T(0) ? povPanDollyFactor : T(1));
    }

    //////////////////////////////////////////////////////////////////
    void updateDolly() {
        tVec3 v(T(0), T(0), delta.y);
        dolly -= v * dollyScale * (povPanDollyFactor>T(0) ? povPanDollyFactor : T(1));
    }

    //////////////////////////////////////////////////////////////////
    void update() {
        if (this->tbActive) VGIZMO_BASE_CLASS::updateGizmo();
        if (dollyActive) updateDolly();
        if (panActive)   updatePan();
    }

    //////////////////////////////////////////////////////////////////
    void applyTransform(tMat4 &m) {
        m = translate(m, pan);
        m = translate(m, dolly);
        m = translate(m, -this->rotationCenter);
        VGIZMO_BASE_CLASS::applyRotation(m);
        m = translate(m, this->rotationCenter);
    }

    //////////////////////////////////////////////////////////////////
    tMat4 getTransform() {
        tMat4 trans, invTrans, rotation;
        tMat4 panMat, dollyMat;

        //create pan and dolly translations
        panMat   = translate(tMat4(T(1)),pan  );
        dollyMat = translate(tMat4(T(1)),dolly);

        //create the virtualGizmo rotation
        rotation = mat4_cast(qtV);

        //create the translations to move the center of rotation to the origin and back
        trans    = translate(tMat4(T(1)), this->rotationCenter);
        invTrans = translate(tMat4(T(1)),-this->rotationCenter);

        //concatenate all the tranforms
        return panMat * dollyMat * invTrans * rotation * trans;
    }

    //  Set the mouse button and mods for dolly operation.
    //////////////////////////////////////////////////////////////////
    void setDollyControl( vgButtons b, vgModifiers m = evNoModifier) {
        dollyControlButton = b;
        dollyControlModifiers = m;
    }
    //  Set the mouse button and optional mods for pan
    //////////////////////////////////////////////////////////////////
    void setPanControl( vgButtons b, vgModifiers m = evNoModifier) {
        panControlButton = b;
        panControlModifiers = m;
    }
    int getPanControlButton() { return panControlButton; }
    int getPanControlModifier() { return panControlModifiers; }

    // Sensitivity for Wheel movements -> Normalized: less < 1 < more
    //////////////////////////////////////////////////////////////////
    void setNormalizedWheelScale( T scale) { wheelScale = scale*constWheelScale;  }
    void setWheelScale( T scale)           { wheelScale = scale;  }
    T getNormalizedWheelScale() { return wheelScale/constWheelScale;  }
    T getWheelScale()           { return wheelScale;  }
    // Sensitivity for Dolly movements -> Normalized: less < 1 < more
    //////////////////////////////////////////////////////////////////
    void setNormalizedDollyScale( T scale) { dollyScale = scale*constDollyScale;  }
    void setDollyScale( T scale)           { dollyScale = scale;  }
    T getNormalizedDollyScale() { return dollyScale/constDollyScale;  }
    T getDollyScale()           { return dollyScale;  }
    // Sensitivity for Pan movements -> Normalized: less < 1 < more
    //////////////////////////////////////////////////////////////////
    void setNormalizedPanScale( T scale) { panScale = scale*constPanScale; }
    void setPanScale( T scale)           { panScale = scale; }
    T getNormalizedPanScale() { return panScale/constPanScale; }
    T getPanScale()           { return panScale; }
    // Sensitivity for pan/dolly by distance -> Normalized: less < 1 < more
    //////////////////////////////////////////////////////////////////
    void setNormalizedDistScale( T scale) { distScale = scale*constDistScale; }
    void setDistScale( T scale)           { distScale = scale; }
    T getNormalizedDistScale() { return distScale/constDistScale; }
    T getDistScale()           { return distScale; }

    //  Set the Dolly to a specified distance.
    //////////////////////////////////////////////////////////////////
    void setDollyPosition(T pos)             { dolly.z = pos; }
    void setDollyPosition(const tVec3 &pos) { dolly.z = pos.z; }

    //  Set the Dolly to a specified distance.
    //////////////////////////////////////////////////////////////////
    void setPanPosition(const tVec3 &pos) { pan.x = pos.x; pan.y = pos.y;}

    //  Get dolly pos... use as Zoom factor
    //////////////////////////////////////////////////////////////////
    tVec3 &getDollyPosition() { return dolly; }

    //  Get Pan pos... use as Zoom factor
    //////////////////////////////////////////////////////////////////
    tVec3 &getPanPosition() { return pan; }

    //  Get Pan (xy) & Dolly (z) position
    //////////////////////////////////////////////////////////////////
    tVec3 getPosition() const { return tVec3(pan.x, pan.y, dolly.z); }
    void  setPosition(const tVec3 &p) { pan.x = p.x; pan.y = p.y; dolly.z = p.z; }

    bool isDollyActive() { return dollyActive; }
    bool isPanActive() { return panActive; }

    void motionImmediateMode( T x, T y, T dx, T dy,  vgModifiers mod) {
        this->tbActive = true;
        this->delta = tVec2(dx,-dy);
        this->pos   = tVec2( x,  y);
        if (dollyControlModifiers & mod) dollyActive = true;
        else if (panControlModifiers & mod) panActive = true;
        update();
    }

private:
    // UI commands that this virtualGizmo responds to (defaults to left mouse button with no modifier key)
    vgButtons   dollyControlButton,    panControlButton;
    vgModifiers dollyControlModifiers, panControlModifiers;

    // Variable used to determine if the manipulator is presently tracking the mouse
    bool dollyActive;
    bool panActive;      

    tVec3 pan   = tVec3(T(0));
    tVec3 dolly = tVec3(T(0));

    const T constDollyScale = T(.01);
    const T constPanScale   = T(.01);  //pan scale
    const T constWheelScale = T(7.5);  //dolly multiply for wheel step
    const T constDistScale  = T( .1);  //speed by distance sensibility

    T dollyScale = constDollyScale;  //dolly scale
    T panScale   = constPanScale  ;  //pan scale
    T wheelScale = constWheelScale;  //dolly multiply for wheel step
    T distScale  = constDistScale ;  //speed by distance sensibility

    T povPanDollyFactor = T(0); // internal use, maintain memory of current distance (pan/zoom speed by distance)
};

#ifdef VGM_USES_TEMPLATE
    #ifdef VGM_USES_DOUBLE_PRECISION
        using vGizmo   = virtualGizmoClass<double>;
        using vGizmo3D = virtualGizmo3DClass<double>;
    #else
        using vGizmo   = virtualGizmoClass<float>;
        using vGizmo3D = virtualGizmo3DClass<float>;
    #endif
    #ifndef IMGUIZMO_USE_ONLY_ROT
        using vImGuIZMO = virtualGizmo3DClass<float>;
    #else
        using vImGuIZMO = virtualGizmoClass<float>;
    #endif
#else
    #ifndef IMGUIZMO_USE_ONLY_ROT
        using vImGuIZMO = virtualGizmo3DClass;
    #else
        using vImGuIZMO = virtualGizmoClass;
    #endif
    using vGizmo    = virtualGizmoClass;
    using vGizmo3D  = virtualGizmo3DClass;
#endif
} // end namespace vg::

#undef T  // if used T as #define, undef it
#undef VGIZMO_H_FILE
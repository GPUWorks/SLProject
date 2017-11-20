//#############################################################################
//  File:      SLScene.h
//  Author:    Marcus Hudritsch
//  Date:      July 2014
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

#ifndef SLSCENE_H
#define SLSCENE_H

#include <stdafx.h>
#include <SLMaterial.h>
#include <SLEventHandler.h>
#include <SLLight.h>
#include <SLNode.h>
#include <SLSkeleton.h>
#include <SLGLOculus.h>
#include <SLAnimManager.h>
#include <SLAverage.h>
#include <SLCVCalibration.h>

class SLSceneView;
class SLCVTracked;

//-----------------------------------------------------------------------------
typedef vector<SLSceneView*> SLVSceneView; //!< Vector of SceneView pointers
typedef vector<SLCVTracked*> SLVCVTracker; //!< Vector of CV tracker pointers
//-----------------------------------------------------------------------------
//! The SLScene class represents the top level instance holding the scene structure
/*!      
The SLScene class holds everything that is common for all scene views such as 
the root pointer (_root3D) to the scene, an array of lights as well as the
global resources (_meshes (SLMesh), _materials (SLMaterial), _textures
(SLGLTexture) and _shaderProgs (SLGLProgram)).
All these resources and the scene with all nodes to which _root3D pointer points
get deleted in the method unInit. \n
A scene could have multiple scene views. A pointer of each is stored in the
vector _sceneViews. \n
The onLoad method can build a of several built in test and demo scenes.
You can access the current scene from everywhere with the static pointer _current.
\n
The SLScene instance has two video camera calibrations, one for a main camera
(SLScene::_calibMainCam) and one for the selfie camera on mobile devices
(SLScene::_calibScndCam). The member SLScene::_activeCalib references the active
one and is set by the SLScene::videoType (VT_NONE, VT_MAIN, VT_SCND) during the
scene assembly in SLScene::onLoad.
*/
class SLScene: public SLObject    
{  
    friend class SLNode;
   
    public:                 SLScene             (SLstring name="");
                           ~SLScene             ();
            // Setters
            void            root3D              (SLNode* root3D){_root3D = root3D;}
            void            root2D              (SLNode* root2D){_root2D = root2D;}
            void            globalAmbiLight     (SLCol4f gloAmbi){_globalAmbiLight=gloAmbi;}
            void            stopAnimations      (SLbool stop) {_stopAnimations = stop;}
            void            videoType           (SLVideoType vt);
            void            showDetection       (SLbool st) {_showDetection = st;}
            void            usesRotation        (SLbool use);
            void            deviceRotStarted    (SLbool started) {_deviceRotStarted = started;}
            void            zeroYawAtStart      (SLbool set) {_zeroYawAtStart = set;}
            void            usesLocation        (SLbool use);
                           
            // Getters
            SLAnimManager&  animManager         () {return _animManager;}
            SLSceneView*    sv                  (SLuint index) {return _sceneViews[index];}
            SLVSceneView&   sceneViews          () {return _sceneViews;}
            SLNode*         root3D              () {return _root3D;}
            SLNode*         root2D              () {return _root2D;}
            SLstring&       info                () {return _info;}
            void            timerStart          () {_timer.start();}
            SLfloat         timeSec             () {return (SLfloat)_timer.getElapsedTimeInSec();}
            SLfloat         timeMilliSec        () {return (SLfloat)_timer.getElapsedTimeInMilliSec();}
            SLfloat         elapsedTimeMS       () {return _elapsedTimeMS;}
            SLfloat         elapsedTimeSec      () {return _elapsedTimeMS * 0.001f;}
            SLVEventHandler& eventHandlers      () {return _eventHandlers;}

            SLCol4f         globalAmbiLight     () const {return _globalAmbiLight;}
            SLVLight&       lights              () {return _lights;}
            SLfloat         fps                 () {return _fps;}
            SLAvgFloat&     frameTimesMS        () {return _frameTimesMS;}
            SLAvgFloat&     updateTimesMS       () {return _updateTimesMS;}
            SLAvgFloat&     trackingTimesMS     () {return _trackingTimesMS;}
            SLAvgFloat&     detectTimesMS       () {return _detectTimesMS;}
            SLAvgFloat&     matchTimesMS        () {return _matchTimesMS;}
            SLAvgFloat&     optFlowTimesMS      () {return _optFlowTimesMS;}
            SLAvgFloat&     poseTimesMS         () {return _poseTimesMS;}
            SLAvgFloat&     cullTimesMS         () {return _cullTimesMS;}
            SLAvgFloat&     draw2DTimesMS       () {return _draw2DTimesMS;}
            SLAvgFloat&     draw3DTimesMS       () {return _draw3DTimesMS;}
            SLAvgFloat&     captureTimesMS      () {return _captureTimesMS;}
            SLVMaterial&    materials           () {return _materials;}
            SLVMesh&        meshes              () {return _meshes;}
            SLVGLTexture&   textures            () {return _textures;}
            SLVGLProgram&   programs            () {return _programs;}
            SLGLProgram*    programs            (SLShaderProg i) {return _programs[i];}
            SLNode*         selectedNode        () {return _selectedNode;}
            SLMesh*         selectedMesh        () {return _selectedMesh;}
            SLbool          stopAnimations      () const {return _stopAnimations;}
            SLGLOculus*     oculus              () {return &_oculus;}
            SLint           numSceneCameras     ();
            SLCamera*       nextCameraInScene   (SLSceneView* activeSV);

            // Video and OpenCV stuff
            SLVideoType         videoType       () {return _videoType;}
            SLGLTexture*        videoTexture    () {return &_videoTexture;}
            SLCVCalibration*    activeCalib     () {return _activeCalib;}
            SLCVCalibration*    calibMainCam    () {return &_calibMainCam;}
            SLCVCalibration*    calibScndCam    () {return &_calibScndCam;}
            SLVCVTracker&       trackers        () {return _trackers;}
            SLbool              showDetection   () {return _showDetection;}

            // Device rotation stuff
            SLbool              usesRotation    () const {return _usesRotation;}
            SLMat3f             deviceRotation  () const {return _deviceRotation;}

            SLfloat             devicePitchRAD  () const {return _devicePitchRAD;}
            SLfloat             deviceYawRAD    () const {return _deviceYawRAD;}
            SLfloat             deviceRollRAD   () const {return _deviceRollRAD;}
            SLbool              zeroYawAtStart  () const {return _zeroYawAtStart;}
            SLfloat             startYawRAD     () const {return _startYawRAD;}

            // Device GPS location stuff
            SLbool              usesLocation    () const {return _usesLocation;}
            SLVec3d             lla             () const {return _lla;}
            float               accuracyM       () const {return _accuracyM;}
            SLVec3d             enu             () const {return _enu;}
            SLVec3d             enuOrigin       () const {return _enuOrigin;}
            SLbool              hasGlobalRefPos () const {return _hasGlobalRefPos;}
            const SLVec3d&      globalRefPosEcef() const {return _globalRefPosEcef;}
            const SLMat3d&      wRecef          () const {return _wRecef;}

            // Misc.
   virtual  void            onLoad              (SLSceneView* sv, 
                                                 SLCommand _currentID);
   virtual  void            onLoadAsset         (SLstring assetFile, 
                                                 SLuint processFlags);
   virtual  void            onAfterLoad         ();
            bool            onUpdate            ();
            void            onRotationPYR       (SLfloat pitchRAD,
                                                 SLfloat yawRAD,
                                                 SLfloat rollRAD);
            void            onRotationQUAT      (SLfloat quatX,
                                                 SLfloat quatY,
                                                 SLfloat quatZ,
                                                 SLfloat quatW);
            void            init                ();
            void            unInit              ();
            SLbool          onCommandAllSV      (const SLCommand cmd);
            void            selectNode          (SLNode* nodeToSelect);
            void            selectNodeMesh      (SLNode* nodeToSelect, SLMesh* meshToSelect);
            void            copyVideoImage      (SLint camWidth, 
                                                 SLint camHeight,
                                                 SLPixelFormat srcPixelFormat,
                                                 SLuchar* data,
                                                 SLbool isContinuous,
                                                 SLbool isTopLeft);

            void            onLocationLLA       (double latitudeDEG,
                                                 double longitudeDEG,
                                                 double altitudeM,
                                                 float accuracyM);
            void            initGlobalRefPos    (double latDeg, 
                                                 double lonDeg, 
                                                 double altM);

     static SLScene*        current;            //!< global static scene pointer
   protected:
            SLVSceneView    _sceneViews;        //!< Vector of all sceneview pointers
            SLVMesh         _meshes;            //!< Vector of all meshes
            SLVMaterial     _materials;         //!< Vector of all materials pointers
            SLVGLTexture    _textures;          //!< Vector of all texture pointers
            SLVGLProgram    _programs;          //!< Vector of all shader program pointers
            SLVLight        _lights;            //!< Vector of all lights
            SLVEventHandler _eventHandlers;     //!< Vector of all event handler
            SLAnimManager   _animManager;       //!< Animation manager instance
            
            SLNode*         _root3D;            //!< Root node for 3D scene
            SLNode*         _root2D;            //!< Root node for 2D scene displayed in ortho projection
            SLstring        _info;              //!< scene info string
            SLNode*         _selectedNode;      //!< Pointer to the selected node
            SLMesh*         _selectedMesh;      //!< Pointer to the selected mesh

            SLTimer         _timer;             //!< high precision timer
            SLCol4f         _globalAmbiLight;   //!< global ambient light intensity
            SLbool          _rootInitialized;   //!< Flag if scene is initialized
            SLint           _numProgsPreload;   //!< No. of preloaded shaderProgs
            
            SLfloat         _elapsedTimeMS;     //!< Last frame time in ms
            SLfloat         _lastUpdateTimeMS;  //!< Last time after update in ms
            SLfloat         _fps;               //!< Averaged no. of frames per second
            SLAvgFloat      _updateTimesMS;     //!< Averaged time for update in ms
            SLAvgFloat      _trackingTimesMS;   //!< Averaged time for video tracking in ms
            SLAvgFloat      _detectTimesMS;     //!< Averaged time for video feature detection & description in ms
            SLAvgFloat      _matchTimesMS;      //!< Averaged time for video feature matching in ms
            SLAvgFloat      _optFlowTimesMS;    //!< Averaged time for video feature optical flow tracking in ms
            SLAvgFloat      _poseTimesMS;       //!< Averaged time for video feature pose estimation in ms
            SLAvgFloat      _frameTimesMS;      //!< Averaged time per frame in ms
            SLAvgFloat      _cullTimesMS;       //!< Averaged time for culling in ms
            SLAvgFloat      _draw3DTimesMS;     //!< Averaged time for 3D drawing in ms
            SLAvgFloat      _draw2DTimesMS;     //!< Averaged time for 2D drawing in ms
            SLAvgFloat      _captureTimesMS;    //!< Averaged time for video capturing in ms
            
            SLbool          _stopAnimations;    //!< Global flag for stopping all animations
            
            SLGLOculus      _oculus;            //!< Oculus Rift interface
            
            // Video stuff
            SLVideoType         _videoType;         //!< Flag for using the live video image
            SLGLTexture         _videoTexture;      //!< Texture for live video image
            SLCVCalibration*    _activeCalib;       //!< Pointer to the active calibration
            SLCVCalibration     _calibMainCam;      //!< OpenCV calibration for main video camera
            SLCVCalibration     _calibScndCam;      //!< OpenCV calibration for secondary video camera
            SLVCVTracker        _trackers;          //!< Vector of all AR trackers
            SLbool              _showDetection;     //!< Flag if detection should be visualized

            // IMU Sensor stuff
            SLbool              _usesRotation;      //!< Flag if device rotation is used
            SLfloat             _devicePitchRAD;    //!< Device pitch angle in radians
            SLfloat             _deviceYawRAD;      //!< Device yaw angle in radians
            SLfloat             _deviceRollRAD;     //!< Device roll angle in radians
            SLMat3f             _deviceRotation;    //!< Mobile device rotation as quaternion
            SLbool              _deviceRotStarted;  //!< Flag for the first sensor values
            SLbool              _zeroYawAtStart;    //!< Flag if yaw angle should be zeroed at sensor start
            SLfloat             _startYawRAD;       //!< Initial yaw angle after _zeroYawAfterSec in radians

            // GPS Sensor stuff
            SLbool              _usesLocation;      //!< Flag if GPS Sensor is used
            SLbool              _deviceLocStarted;  //!< Flag for the first sensor values
            SLVec3d             _lla;               //!< GPS location in latitudeDEG, longitudeDEG & AltitudeM
            SLfloat             _accuracyM;         //!< Horizontal accuracy radius in m with 68% probability
            SLVec3d             _enu;               //!< gps in enu
            SLVec3d             _enuOrigin;         //!< enu origin location
            SLbool              _hasGlobalRefPos;   //!< Flag if this scene has a global reference position
            SLVec3d             _globalRefPosEcef;  //!< Global ecef reference position of scene origin (world)
            SLMat3d             _wRecef;            //!< ecef frame to world frame rotation: rotates a point defined in ecef
};
//-----------------------------------------------------------------------------
#endif

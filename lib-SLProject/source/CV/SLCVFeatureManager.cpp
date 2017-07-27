//#############################################################################
//  File:      SLCVFeatureManager.cpp
//  Purpose:   OpenCV Detector Describer Wrapper
//  Author:    Marcus Hudritsch
//  Date:      Spring 2017
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

#include <stdafx.h>         // precompiled headers

/*
The OpenCV library version 3.1 with extra module must be present.
If the application captures the live video stream with OpenCV you have
to define in addition the constant SL_USES_CVCAPTURE.
All classes that use OpenCV begin with SLCV.
See also the class docs for SLCVCapture, SLCVCalibration and SLCVTracked
for a good top down information.
*/

#include <SLCVFeatureManager.h>
#include <SLScene.h>
#include <SLSceneView.h>
#include <SLCVRaulMurOrb.h>
using namespace cv;

//-----------------------------------------------------------------------------
SLCVFeatureManager::SLCVFeatureManager(SLCVDetectDescribeType detectDescribeType)
{
    _type = detectDescribeType;

    switch(detectDescribeType)
    {
        case DDT_FAST_BRIEF:
            _detector = FastFeatureDetector::create(30, true, FastFeatureDetector::TYPE_9_16);
            _descriptor = xfeatures2d::BriefDescriptorExtractor::create(32, true); return; 
            return;
        case DDT_ORB_ORB:
            _detector = ORB::create(200, 1.44f, 3, 31, 0, 2, ORB::HARRIS_SCORE, 31, 30);
            _descriptor = _detector; 
            return;
        case DDT_RAUL_RAUL:
            _detector = new SLCVRaulMurOrb(1500, 1.44f, 4, 30, 20);
            _descriptor = _detector; 
            return;
        case DDT_SURF_SURF:
            _detector = xfeatures2d::SURF::create(100, 2, 2, false, false);
            _descriptor = _detector; 
            return;
        case DDT_SIFT_SIFT:
            _detector = xfeatures2d::SIFT::create(300, 2, 0.04, 10, 1.6);;
            _descriptor = _detector; 
            return;
        default: 
            SL_EXIT_MSG("Unknown detector-descriptor type.");
    }
}
//-----------------------------------------------------------------------------
SLCVFeatureManager::~SLCVFeatureManager()
{
    deleteAll();
}
//-----------------------------------------------------------------------------
void SLCVFeatureManager::deleteAll()
{
    if (!_detector && !_descriptor)
        return;

    if (_detector == _descriptor)
    {   delete _detector;
        _detector = nullptr;
        _descriptor = nullptr;
        return;
    }

    if (_detector)
    {   delete _detector;
        _detector = nullptr;
        return;
    }

    if (_descriptor)
    {   delete _descriptor;
        _descriptor = nullptr;
        return;
    }
}
//-----------------------------------------------------------------------------
void SLCVFeatureManager::detectorDescriptor(SLCVDetectDescribeType detectDescribeType,
                                            SLCVFeatureDetector* detector,
                                            SLCVDescriptorExtractor* descriptor)
{
    deleteAll();
    _type = detectDescribeType;
    _detector = detector;
    _descriptor = descriptor;
}
//-----------------------------------------------------------------------------
void SLCVFeatureManager::detectAndDescribe(SLCVInputArray image,
                                           SLCVVKeyPoint& keypoints,
                                           SLCVOutputArray descriptors,
                                           SLCVInputArray mask)
{
    assert(_detector && "SLCVFeatureManager::detectAndDescribe: No detector!");
    assert(_descriptor && "SLCVFeatureManager::detectAndDescribe: No descriptor!");

    if (_detector == _descriptor)
        _detector->detectAndCompute(image, mask, keypoints, descriptors);
    else
    {
        _detector->detect(image, keypoints, mask);
        _descriptor->compute(image, keypoints, descriptors);
    }
}
//-----------------------------------------------------------------------------

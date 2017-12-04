//#############################################################################
//  File:      SLDemoGui.cpp
//  Purpose:   UI with the ImGUI framework fully rendered in OpenGL 3+
//  Author:    Marcus Hudritsch
//  Date:      Summer 2017
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

#include <stdafx.h>           // precompiled headers
#ifdef SL_MEMLEAKDETECT       // set in SL.h for debug config only
#include <debug_new.h>        // memory leak detector
#endif

#include <SLDemoGui.h>
#include <SLScene.h>
#include <SLSceneView.h>
#include <SLInterface.h>
#include <SLNode.h>
#include <SLMesh.h>
#include <SLMaterial.h>
#include <SLGLTexture.h>
#include <SLGLProgram.h>
#include <SLGLShader.h>
#include <SLLightSpot.h>
#include <SLLightRect.h>
#include <SLLightDirect.h>
#include <SLAnimPlayback.h>
#include <SLImporter.h>
#include <SLCVCapture.h>
#include <SLCVImage.h>
#include <SLGLTexture.h>
#include <SLTransferFunction.h>
#include <SLCVTrackedFeatures.h>

#include <imgui.h>

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

//-----------------------------------------------------------------------------
//! Vector getter callback for combo and listbox with std::vector<std::string>
static auto vectorGetter = [](void* vec, int idx, const char** out_text)
{
    auto& vector = *(SLVstring*)vec;
    if (idx < 0 || idx >= (int)vector.size())
        return false;

    *out_text = vector.at(idx).c_str();
    return true;
};
//-----------------------------------------------------------------------------
//! Combobox that allows to pass the items as a string vector
bool myComboBox(const char* label, int* currIndex, SLVstring& values)
{
    if (values.empty())
        return false;

    return ImGui::Combo(label,
                        currIndex,
                        vectorGetter,
                        (void*)&values,
                        (int)values.size());
}
//-----------------------------------------------------------------------------
//! Listbox that allows to pass the items as a string vector
bool myListBox(const char* label, int* currIndex, SLVstring& values)
{
    if (values.empty())
        return false;

    return ImGui::ListBox(label,
                          currIndex,
                          vectorGetter,
                          (void*)&values,
                          (int)values.size());
}
//-----------------------------------------------------------------------------
//! Centers the next ImGui window in the parent
void centerNextWindow(SLSceneView* sv, SLfloat widthPC=0.9f, SLfloat heightPC=0.7f)
{
    SLfloat width  = (SLfloat)sv->scrW()*widthPC;
    SLfloat height = (SLfloat)sv->scrH()*heightPC;
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_Always);
    ImGui::SetNextWindowPosCenter(ImGuiSetCond_Always);
}
//-----------------------------------------------------------------------------
// Init global static variables
SLGLTexture*    SLDemoGui::cpvrLogo            = nullptr;
SLstring        SLDemoGui::configTime          = "-";
SLbool          SLDemoGui::showAbout           = false;
SLbool          SLDemoGui::showHelp            = false;
SLbool          SLDemoGui::showHelpCalibration = false;
SLbool          SLDemoGui::showCredits         = false;
SLbool          SLDemoGui::showStatsTiming     = false;
SLbool          SLDemoGui::showStatsScene      = false;
SLbool          SLDemoGui::showStatsVideo      = false;
SLbool          SLDemoGui::showInfosFrameworks = false;
SLbool          SLDemoGui::showInfosScene      = false;
SLbool          SLDemoGui::showInfosSensors    = false;
SLbool          SLDemoGui::showSceneGraph      = false;
SLbool          SLDemoGui::showProperties      = false;

SLstring SLDemoGui::infoAbout =
"Welcome to the SLProject demo app. It is developed at the \
Computer Science Department of the Bern University of Applied Sciences. \
The app shows what you can learn in two semesters about 3D computer graphics \
in real time rendering and ray tracing. The framework is developed \
in C++ with OpenGL ES so that it can run also on mobile devices. \
Ray tracing provides in addition high quality transparencies, reflections and soft shadows. \
Click to close and use the menu to choose different scenes and view settings. \
For more information please visit: https://github.com/cpvrlab/SLProject";

SLstring SLDemoGui::infoCredits =
"Contributors since 2005 in alphabetic order: Martin Christen, Manuel Frischknecht, Michael \
Goettlicher, Timo Tschanz, Marc Wacker, Pascal Zingg \n\n\
Credits for external libraries:\n\
- assimp: assimp.sourceforge.net\n\
- imgui: github.com/ocornut/imgui\n\
- glew: glew.sourceforge.net\n\
- glfw: glfw.org\n\
- OpenCV: opencv.org\n\
- OpenGL: opengl.org";

SLstring SLDemoGui::infoHelp =
"Help for mouse or finger control:\n\
- Use mouse or your finger to rotate the scene\n\
- Use mouse-wheel or pinch 2 fingers to go forward/backward\n\
- Use CTRL-mouse or 2 fingers to move sidewards/up-down\n\
- Double click or double tap to select object\n\
- If no menu is visible press ESC";

SLstring SLDemoGui::infoCalibrate =
"The calibration process requires a chessboard image to be printed \
and glued on a flat board. You can find the PDF with the chessboard image on: \n\
https://github.com/cpvrlab/SLProject/tree/master/_data/calibrations/ \n\n\
For a calibration you have to take 20 images with detected inner \
chessboard corners. To take an image you have to click with the mouse \
or tap with finger into the screen. You can mirror the video image under \
Preferences > Video. \n\
After calibration the yellow wireframe cube should stick on the chessboard.\n\n\
Please close first this info dialog.";

//-----------------------------------------------------------------------------
//! This is the main building function for the GUI of the Demo apps
/*! Is is passed to the SLGLImGui::build function in main of the app-Demo-GLFW
 app. This function will be called once per frame roughly at the end of
 SLSceneView::onPaint in SLSceneView::draw2DGL by calling ImGui::Render.\n
 See also the comments on SLGLImGui.
 */
void SLDemoGui::buildDemoGui(SLScene* s, SLSceneView* sv)
{
    if (!showAbout && !showCredits)
        buildMenuBar(s, sv);

    if (showAbout)
    {
        if (cpvrLogo == nullptr)
        {
            // The texture resources get deleted by the SLScene destructor
            cpvrLogo = new SLGLTexture("LogoCPVR_256L.png");
            if (cpvrLogo != nullptr)
                cpvrLogo->bindActive();
        } else  cpvrLogo->bindActive();

        centerNextWindow(sv);
        ImGui::Begin("About SLProject", &showAbout);
        ImGui::Image((ImTextureID)cpvrLogo->texName(), ImVec2(100,100), ImVec2(0,1), ImVec2(1,0));
        ImGui::SameLine();
        ImGui::Text("Version %s", SL::version.c_str());
        ImGui::Separator();
        ImGui::TextWrapped(infoAbout.c_str());
        ImGui::End();
    }

    if (showHelp)
    {
        centerNextWindow(sv);
        ImGui::Begin("Help on Interaction", &showHelp);
        ImGui::Separator();

        ImGui::TextWrapped(infoHelp.c_str());
        ImGui::End();
    }

    if (showHelpCalibration)
    {
        centerNextWindow(sv);
        ImGui::Begin("Help on Camera Calibration", &showHelpCalibration, ImVec2(400,0));
        ImGui::TextWrapped(infoCalibrate.c_str());
        ImGui::End();
    }

    if (showCredits)
    {
        centerNextWindow(sv);
        ImGui::Begin("Credits for all Contributors and external Libraries", &showCredits);
        ImGui::TextWrapped(infoCredits.c_str());
        ImGui::End();
    }

    if (showStatsTiming)
    {
        SLRenderType rType = sv->renderType();
        SLCamera* cam = sv->camera();
        SLfloat ft = s->frameTimesMS().average();
        SLchar m[2550];   // message character array
        m[0]=0;           // set zero length

        if (rType == RT_gl)
        {
            // Get averages from average variables (see SLAverage)
            SLfloat captureTime     = s->captureTimesMS().average();
            SLfloat updateTime      = s->updateTimesMS().average();
            SLfloat trackingTime    = s->trackingTimesMS().average();
            SLfloat detectTime      = s->detectTimesMS().average();
            SLfloat matchTime       = s->matchTimesMS().average();
            SLfloat optFlowTime     = s->optFlowTimesMS().average();
            SLfloat poseTime        = s->poseTimesMS().average();
            SLfloat draw3DTime      = s->draw3DTimesMS().average();
            SLfloat draw2DTime      = s->draw2DTimesMS().average();
            SLfloat cullTime        = s->cullTimesMS().average();

            // Calculate percentage from frame time
            SLfloat captureTimePC   = SL_clamp(captureTime  / ft * 100.0f, 0.0f,100.0f);
            SLfloat updateTimePC    = SL_clamp(updateTime   / ft * 100.0f, 0.0f,100.0f);
            SLfloat trackingTimePC  = SL_clamp(trackingTime / ft * 100.0f, 0.0f,100.0f);
            SLfloat detectTimePC    = SL_clamp(detectTime   / ft * 100.0f, 0.0f,100.0f);
            SLfloat matchTimePC     = SL_clamp(matchTime    / ft * 100.0f, 0.0f,100.0f);
            SLfloat optFlowTimePC   = SL_clamp(optFlowTime  / ft * 100.0f, 0.0f,100.0f);
            SLfloat poseTimePC      = SL_clamp(poseTime     / ft * 100.0f, 0.0f,100.0f);
            SLfloat draw3DTimePC    = SL_clamp(draw3DTime   / ft * 100.0f, 0.0f,100.0f);
            SLfloat draw2DTimePC    = SL_clamp(draw2DTime   / ft * 100.0f, 0.0f,100.0f);
            SLfloat cullTimePC      = SL_clamp(cullTime     / ft * 100.0f, 0.0f,100.0f);

            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::Begin("Timing", &showStatsTiming, ImVec2(300,0));
            ImGui::Text("Renderer      : OpenGL");
            ImGui::Text("Frame size    : %d x %d", sv->scrW(), sv->scrH());
            ImGui::Text("NO. drawcalls : %d\n", SLGLVertexArray::totalDrawCalls);
            ImGui::Text("Frames per s. : %4.1f", s->fps());
            ImGui::Text("Frame time    : %4.1f ms (100%%)", ft);
            ImGui::Text("  Capture     : %4.1f ms (%3d%%)", captureTime,  (SLint)captureTimePC);
            ImGui::Text("  Update      : %4.1f ms (%3d%%)", updateTime,   (SLint)updateTimePC);
            ImGui::Text("    Tracking  : %4.1f ms (%3d%%)", trackingTime, (SLint)trackingTimePC);
            ImGui::Text("      Detect  : %4.1f ms (%3d%%)", detectTime,   (SLint)detectTimePC);
            ImGui::Text("      Match   : %4.1f ms (%3d%%)", matchTime,    (SLint)matchTimePC);
            ImGui::Text("      Opt.Flow: %4.1f ms (%3d%%)", optFlowTime,  (SLint)optFlowTimePC);
            ImGui::Text("      Pose    : %4.1f ms (%3d%%)", poseTime,     (SLint)poseTimePC);
            ImGui::Text("  Culling     : %4.1f ms (%3d%%)", cullTime,     (SLint)cullTimePC);
            ImGui::Text("  Drawing 3D  : %4.1f ms (%3d%%)", draw3DTime,   (SLint)draw3DTimePC);
            ImGui::Text("  Drawing 2D  : %4.1f ms (%3d%%)", draw2DTime,   (SLint)draw2DTimePC);

            ImGui::End();
            ImGui::PopFont();

        } else
        if (rType == RT_rt)
        {
            SLRaytracer* rt = sv->raytracer();
            SLint primaries = sv->scrW() * sv->scrH();
            SLuint total = primaries + SLRay::reflectedRays + SLRay::subsampledRays + SLRay::refractedRays + SLRay::shadowRays;
            SLfloat rpms = rt->renderSec() ? total/rt->renderSec()/1000.0f : 0.0f;
            sprintf(m+strlen(m), "Renderer      : Ray Tracer\n");
            sprintf(m+strlen(m), "Frame size    : %d x %d\n", sv->scrW(), sv->scrH());
            sprintf(m+strlen(m), "Frames per s. : %4.1f\n", s->fps());
            sprintf(m+strlen(m), "Frame Time    : %4.2f sec.\n", rt->renderSec());
            sprintf(m+strlen(m), "Rays per ms   : %6.0f\n", rpms);
            sprintf(m+strlen(m), "Threads       : %d\n", rt->numThreads());

            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::Begin("Timing", &showStatsTiming, ImVec2(300,0));
            ImGui::TextWrapped(m);
            ImGui::End();
            ImGui::PopFont();
        }
    }

    if (showStatsScene)
    {
        SLchar m[2550];   // message character array
        m[0]=0;           // set zero length

        SLNodeStats& stats2D  = sv->stats2D();
        SLNodeStats& stats3D  = sv->stats3D();
        SLfloat vox           = (SLfloat)stats3D.numVoxels;
        SLfloat voxEmpty      = (SLfloat)stats3D.numVoxEmpty;
        SLfloat voxelsEmpty   = vox ? voxEmpty / vox*100.0f : 0.0f;
        SLfloat numRTTria     = (SLfloat)stats3D.numTriangles;
        SLfloat avgTriPerVox  = vox ? numRTTria / (vox-voxEmpty) : 0.0f;
        SLint numOpaqueNodes  = (int)sv->visibleNodes()->size();
        SLint numBlendedNodes = (int)sv->blendNodes()->size();
        SLint numVisibleNodes =  numOpaqueNodes + numBlendedNodes;
        SLint numGroupPC      = (SLint)((SLfloat)stats3D.numGroupNodes/(SLfloat)stats3D.numNodes * 100.0f);
        SLint numLeafPC       = (SLint)((SLfloat)stats3D.numLeafNodes/(SLfloat)stats3D.numNodes * 100.0f);
        SLint numLightsPC     = (SLint)((SLfloat)stats3D.numLights/(SLfloat)stats3D.numNodes * 100.0f);
        SLint numOpaquePC     = (SLint)((SLfloat)numOpaqueNodes/(SLfloat)stats3D.numNodes * 100.0f);
        SLint numBlendedPC    = (SLint)((SLfloat)numBlendedNodes/(SLfloat)stats3D.numNodes * 100.0f);
        SLint numVisiblePC    = (SLint)((SLfloat)numVisibleNodes/(SLfloat)stats3D.numNodes * 100.0f);

        // Calculate total size of texture bytes on CPU
        SLfloat cpuMBTexture = 0;
        for (auto t : s->textures())
            for (auto i : t->images())
                cpuMBTexture += i->bytesPerImage();
        cpuMBTexture  = cpuMBTexture / 1E6f;

        SLfloat cpuMBMeshes    = (SLfloat)stats3D.numBytes / 1E6f;
        SLfloat cpuMBVoxels    = (SLfloat)stats3D.numBytesAccel / 1E6f;
        SLfloat cpuMBTotal     = cpuMBTexture + cpuMBMeshes + cpuMBVoxels;
        SLint   cpuMBTexturePC = (SLint)(cpuMBTexture / cpuMBTotal * 100.0f);
        SLint   cpuMBMeshesPC  = (SLint)(cpuMBMeshes  / cpuMBTotal * 100.0f);
        SLint   cpuMBVoxelsPC  = (SLint)(cpuMBVoxels  / cpuMBTotal * 100.0f);
        SLfloat gpuMBTexture   = (SLfloat)SLGLTexture::numBytesInTextures / 1E6f;
        SLfloat gpuMBVbo       = (SLfloat)SLGLVertexBuffer::totalBufferSize / 1E6f;
        SLfloat gpuMBTotal     = gpuMBTexture + gpuMBVbo;
        SLint   gpuMBTexturePC = (SLint)(gpuMBTexture / gpuMBTotal * 100.0f);
        SLint   gpuMBVboPC     = (SLint)(gpuMBVbo / gpuMBTotal * 100.0f);

        sprintf(m+strlen(m), "Scene Name      : %s\n", s->name().c_str());
        sprintf(m+strlen(m), "No. of Nodes    : %5d (100%%)\n", stats3D.numNodes);
        sprintf(m+strlen(m), "- Group Nodes   : %5d (%3d%%)\n", stats3D.numGroupNodes, numGroupPC);
        sprintf(m+strlen(m), "- Leaf  Nodes   : %5d (%3d%%)\n", stats3D.numLeafNodes, numLeafPC);
        sprintf(m+strlen(m), "- Light Nodes   : %5d (%3d%%)\n", stats3D.numLights, numLightsPC);
        sprintf(m+strlen(m), "- Opaque Nodes  : %5d (%3d%%)\n", numOpaqueNodes, numOpaquePC);
        sprintf(m+strlen(m), "- Blended Nodes : %5d (%3d%%)\n", numBlendedNodes, numBlendedPC);
        sprintf(m+strlen(m), "- Visible Nodes : %5d (%3d%%)\n", numVisibleNodes, numVisiblePC);
        sprintf(m+strlen(m), "No. of Meshes   : %u\n", stats3D.numMeshes);
        sprintf(m+strlen(m), "No. of Triangles: %u\n", stats3D.numTriangles);
        sprintf(m+strlen(m), "CPU MB in Total : %6.2f (100%%)\n", cpuMBTotal);
        sprintf(m+strlen(m), "-   MB in Tex.  : %6.2f (%3d%%)\n", cpuMBTexture, cpuMBTexturePC);
        sprintf(m+strlen(m), "-   MB in Meshes: %6.2f (%3d%%)\n", cpuMBMeshes, cpuMBMeshesPC);
        sprintf(m+strlen(m), "-   MB in Voxels: %6.2f (%3d%%)\n", cpuMBVoxels, cpuMBVoxelsPC);
        sprintf(m+strlen(m), "GPU MB in Total : %6.2f (100%%)\n", gpuMBTotal);
        sprintf(m+strlen(m), "-   MB in Tex.  : %6.2f (%3d%%)\n", gpuMBTexture, gpuMBTexturePC);
        sprintf(m+strlen(m), "-   MB in VBO   : %6.2f (%3d%%)\n", gpuMBVbo, gpuMBVboPC);

        sprintf(m+strlen(m), "No. of Voxels   : %d\n", stats3D.numVoxels);
        sprintf(m+strlen(m), "- empty Voxels  : %4.1f%%\n", voxelsEmpty);
        sprintf(m+strlen(m), "Avg. Tria/Voxel : %4.1F\n", avgTriPerVox);
        sprintf(m+strlen(m), "Max. Tria/Voxel : %d\n", stats3D.numVoxMaxTria);

        // Switch to fixed font
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::Begin("Scene Statistics", &showStatsScene, ImVec2(300,0));
        ImGui::TextUnformatted(m);
        ImGui::End();
        ImGui::PopFont();
    }

    if (showStatsVideo)
    {
        SLchar m[2550];   // message character array
        m[0]=0;           // set zero length

        SLCVCalibration* c = s->activeCalib();
        SLCVSize capSize = SLCVCapture::captureSize;
        SLVideoType vt = s->videoType();
        SLstring mirrored = "None";
        if (c->isMirroredH() && c->isMirroredV()) mirrored = "horizontally & vertically"; else
        if (c->isMirroredH()) mirrored = "horizontally"; else
        if (c->isMirroredV()) mirrored = "vertically";

        sprintf(m+strlen(m), "Video Type    : %s\n", vt==0 ? "None" : vt==1 ? "Main Camera" : "Secondary Camera");
        sprintf(m+strlen(m), "Display size  : %d x %d\n", SLCVCapture::lastFrame.cols, SLCVCapture::lastFrame.rows);
        sprintf(m+strlen(m), "Capture size  : %d x %d\n", capSize.width, capSize.height);
        sprintf(m+strlen(m), "Requested size: %d\n", SLCVCapture::requestedSizeIndex);
        sprintf(m+strlen(m), "Mirrored      : %s\n", mirrored.c_str());
        sprintf(m+strlen(m), "Undistorted   : %s\n", c->showUndistorted()&&c->state()==CS_calibrated?"Yes":"No");
        sprintf(m+strlen(m), "FOV (deg.)    : %4.1f\n", c->cameraFovDeg());
        sprintf(m+strlen(m), "fx,fy,cx,cy   : %4.1f,%4.1f,%4.1f,%4.1f\n", c->fx(),c->fy(),c->cx(),c->cy());
        sprintf(m+strlen(m), "k1,k2,p1,p2   : %4.2f,%4.2f,%4.2f,%4.2f\n", c->k1(),c->k2(),c->p1(),c->p2());
        sprintf(m+strlen(m), "Calib. time   : %s\n", c->calibrationTime().c_str());
        sprintf(m+strlen(m), "Calib. file   : %s\n", c->calibFileName().c_str());
        sprintf(m+strlen(m), "Calib. state  : %s\n", c->stateStr().c_str());

        // Switch to fixed font
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::Begin("Video", &showStatsVideo, ImVec2(300,0));
        ImGui::TextUnformatted(m);
        ImGui::End();
        ImGui::PopFont();
    }

    if (showInfosScene && !showAbout && !showCredits)
    {
        // Calculate window position for dynamic status bar at the bottom of the main window
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoResize;
        SLfloat w = (SLfloat)sv->scrW();
        ImVec2 size = ImGui::CalcTextSize(s->info().c_str(), 0, true, w);
        SLfloat h = size.y + SLGLImGui::fontPropDots * 1.2f;
        SLstring info = "Scene Info: " + s->info();

        ImGui::SetNextWindowPos(ImVec2(0,sv->scrH()-h));
        ImGui::SetNextWindowSize(ImVec2(w,h));
        ImGui::Begin("Scene Information", &showInfosScene, window_flags);
        ImGui::TextWrapped(info.c_str());
        ImGui::End();
    }

    if (showInfosFrameworks)
    {
        SLGLState* stateGL = SLGLState::getInstance();
        SLchar m[2550];   // message character array
        m[0]=0;           // set zero length
        sprintf(m+strlen(m), "OpenGL Verion  : %s\n", stateGL->glVersionNO().c_str());
        sprintf(m+strlen(m), "OpenGL Vendor  : %s\n", stateGL->glVendor().c_str());
        sprintf(m+strlen(m), "OpenGL Renderer: %s\n", stateGL->glRenderer().c_str());
        sprintf(m+strlen(m), "OpenGL GLSL    : %s\n", stateGL->glSLVersionNO().c_str());
        sprintf(m+strlen(m), "OpenCV Version : %d.%d.%d\n", CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_VERSION_REVISION);
        sprintf(m+strlen(m), "CV has OpenCL  : %s\n", cv::ocl::haveOpenCL() ? "yes":"no");
        sprintf(m+strlen(m), "ImGui Version  : %s\n", ImGui::GetVersion());

        // Switch to fixed font
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::Begin("Framework Informations", &showInfosFrameworks, ImVec2(300,0));
        ImGui::TextUnformatted(m);
        ImGui::End();
        ImGui::PopFont();
    }

    if (showInfosSensors)
    {
        SLGLState* stateGL = SLGLState::getInstance();
        SLchar m[2550];   // message character array
        m[0]=0;           // set zero length
        sprintf(m+strlen(m), "Uses Rotation       : %s\n",    s->usesRotation() ? "yes" : "no");
        sprintf(m+strlen(m), "Orientation Pitch   : %1.0f\n", s->devicePitchRAD()*SL_RAD2DEG);
        sprintf(m+strlen(m), "Orientation Yaw     : %1.0f\n", s->deviceYawRAD()*SL_RAD2DEG);
        sprintf(m+strlen(m), "Orientation Roll    : %1.0f\n", s->deviceRollRAD()*SL_RAD2DEG);
        sprintf(m+strlen(m), "Zero Yaw at Start   : %s\n",    s->zeroYawAtStart() ? "yes" : "no");
        sprintf(m+strlen(m), "Start Yaw           : %1.0f\n", s->startYawRAD() * SL_RAD2DEG);
        sprintf(m+strlen(m), "--------------------:\n");
        sprintf(m+strlen(m), "Uses Location       : %s\n",    s->usesLocation() ? "yes" : "no");
        sprintf(m+strlen(m), "Latitude (deg)      : %f\n",    s->lla().x);
        sprintf(m+strlen(m), "Longitude (deg)     : %f\n",    s->lla().y);
        sprintf(m+strlen(m), "Altitude (m)        : %f\n",    s->lla().z);
        sprintf(m+strlen(m), "Accuracy Radius (m) : %f\n",    s->accuracyM());
        // Switch to fixed font
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::Begin("Sensor Informations", &showInfosSensors, ImVec2(300,0));
        ImGui::TextUnformatted(m);
        ImGui::End();
        ImGui::PopFont();
    }

    if (showSceneGraph)
    {
        buildSceneGraph(s);
    }

    if (showProperties)
    {
        buildProperties(s);
    }
}
//-----------------------------------------------------------------------------
void SLDemoGui::buildMenuBar(SLScene* s, SLSceneView* sv)
{
    SLCommand curS = SL::currentSceneID;
    SLRenderType rType = sv->renderType();
    SLbool hasAnimations = (s->animManager().allAnimNames().size() > 0);
    static SLint curAnimIx = -1;
    if (!hasAnimations) curAnimIx = -1;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Load Test Scene"))
            {
                if (ImGui::BeginMenu("General Scenes"))
                {
                    SLstring large1 = SLImporter::defaultPath + "PLY/xyzrgb_dragon.ply";
                    SLstring large2 = SLImporter::defaultPath + "PLY/mesh_zermatt.ply";
                    SLstring large3 = SLImporter::defaultPath + "PLY/switzerland.ply";
                    SLbool largeFileExists = SLFileSystem::fileExists(large1) ||
                                             SLFileSystem::fileExists(large2) ||
                                             SLFileSystem::fileExists(large3);

                    if (ImGui::MenuItem("Minimal Scene", 0, curS==C_sceneMinimal))
                        sv->onCommand(C_sceneMinimal);
                    if (ImGui::MenuItem("Figure Scene", 0, curS==C_sceneFigure))
                        sv->onCommand(C_sceneFigure);
                    if (ImGui::MenuItem("Large Model", 0, curS==C_sceneLargeModel, largeFileExists))
                        sv->onCommand(C_sceneLargeModel);
                    if (ImGui::MenuItem("Mesh Loader", 0, curS==C_sceneMeshLoad))
                        sv->onCommand(C_sceneMeshLoad);
                    if (ImGui::MenuItem("Texture Blending", 0, curS==C_sceneTextureBlend))
                        sv->onCommand(C_sceneTextureBlend);
                    if (ImGui::MenuItem("Texture Filters", 0, curS==C_sceneTextureFilter))
                        sv->onCommand(C_sceneTextureFilter);
                    if (ImGui::MenuItem("Frustum Culling", 0, curS==C_sceneFrustumCull))
                        sv->onCommand(C_sceneFrustumCull);
                    if (ImGui::MenuItem("Massive Data Scene", 0, curS==C_sceneMassiveData))
                        sv->onCommand(C_sceneMassiveData);
                    if (ImGui::MenuItem("2D and 3D Text", 0, curS==C_scene2Dand3DText))
                        sv->onCommand(C_scene2Dand3DText);
                    if (ImGui::MenuItem("Point Clouds", 0, curS==C_scenePointClouds))
                        sv->onCommand(C_scenePointClouds);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Shader"))
                {
                    if (ImGui::MenuItem("Per Vertex Blinn-Phong Lighting", 0, curS==C_sceneShaderPerVertexBlinn))
                        sv->onCommand(C_sceneShaderPerVertexBlinn);
                    if (ImGui::MenuItem("Per Pixel Blinn-Phing Lighting", 0, curS==C_sceneShaderPerPixelBlinn))
                        sv->onCommand(C_sceneShaderPerPixelBlinn);
                    if (ImGui::MenuItem("Per Pixel Cook-Torrance Lighting", 0, curS==C_sceneShaderPerPixelCookTorrance))
                        sv->onCommand(C_sceneShaderPerPixelCookTorrance);
                    if (ImGui::MenuItem("Per Vertex Wave", 0, curS==C_sceneShaderPerVertexWave))
                        sv->onCommand(C_sceneShaderPerVertexWave);
                    if (ImGui::MenuItem("Water", 0, curS==C_sceneShaderWater))
                        sv->onCommand(C_sceneShaderWater);
                    if (ImGui::MenuItem("Bump Mapping", 0, curS==C_sceneShaderBumpNormal))
                        sv->onCommand(C_sceneShaderBumpNormal);
                    if (ImGui::MenuItem("Parallax Mapping", 0, curS==C_sceneShaderBumpParallax))
                        sv->onCommand(C_sceneShaderBumpParallax);
                    if (ImGui::MenuItem("Glass Shader", 0, curS==C_sceneRevolver))
                        sv->onCommand(C_sceneRevolver);
                    if (ImGui::MenuItem("Skybox Shader", 0, curS==C_sceneShaderSkyBox))
                        sv->onCommand(C_sceneShaderSkyBox);
                    if (ImGui::MenuItem("Earth Shader", 0, curS==C_sceneShaderEarth))
                        sv->onCommand(C_sceneShaderEarth);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Animation"))
                {
                    if (ImGui::MenuItem("Mass Animation", 0, curS==C_sceneAnimationMass))
                        sv->onCommand(C_sceneAnimationMass);
                    if (ImGui::MenuItem("Astroboy Army", 0, curS==C_sceneAnimationArmy))
                        sv->onCommand(C_sceneAnimationArmy);
                    if (ImGui::MenuItem("Skeletal Animation", 0, curS==C_sceneAnimationSkeletal))
                        sv->onCommand(C_sceneAnimationSkeletal);
                    if (ImGui::MenuItem("Node Animation", 0, curS==C_sceneAnimationNode))
                        sv->onCommand(C_sceneAnimationNode);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Using Video"))
                {
                    if (ImGui::MenuItem("Track ArUco Marker (Main)", 0, curS==C_sceneVideoTrackArucoMain))
                        sv->onCommand(C_sceneVideoTrackArucoMain);
                    if (ImGui::MenuItem("Track ArUco Marker (Scnd)", 0, curS==C_sceneVideoTrackArucoScnd, SLCVCapture::hasSecondaryCamera))
                        sv->onCommand(C_sceneVideoTrackArucoScnd);
                    if (ImGui::MenuItem("Track Chessboard (Main)", 0, curS==C_sceneVideoTrackChessMain))
                        sv->onCommand(C_sceneVideoTrackChessMain);
                    if (ImGui::MenuItem("Track Chessboard (Scnd)", 0, curS==C_sceneVideoTrackChessScnd, SLCVCapture::hasSecondaryCamera))
                        sv->onCommand(C_sceneVideoTrackChessScnd);
                    if (ImGui::MenuItem("Track Features (Main)", 0, curS==C_sceneVideoTrackFeature2DMain))
                        sv->onCommand(C_sceneVideoTrackFeature2DMain);
                    if (ImGui::MenuItem("Texture from live video", 0, curS==C_sceneVideoTexture))
                        sv->onCommand(C_sceneVideoTexture);
                    if (ImGui::MenuItem("GPS Display Scene", 0, curS==C_sceneSensorTest))
                        sv->onCommand(C_sceneSensorTest);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Volume Rendering"))
                {
                    if (ImGui::MenuItem("Head MRI Ray Cast", 0, curS==C_sceneVolumeRayCastHeadMRI))
                        sv->onCommand(C_sceneVolumeRayCastHeadMRI);

                    #ifndef SL_GLES
                    if (ImGui::MenuItem("Head MRI Ray Cast Lighted", 0, curS==C_sceneVolumeRayCastLightedMRIHead))
                        sv->onCommand(C_sceneVolumeRayCastLightedMRIHead);
                    #endif

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Ray tracing"))
                {
                    if (ImGui::MenuItem("Spheres", 0, curS==C_sceneRTSpheres))
                        sv->onCommand(C_sceneRTSpheres);
                    if (ImGui::MenuItem("Muttenzer Box", 0, curS==C_sceneRTMuttenzerBox))
                        sv->onCommand(C_sceneRTMuttenzerBox);
                    if (ImGui::MenuItem("Soft Shadows", 0, curS==C_sceneRTSoftShadows))
                        sv->onCommand(C_sceneRTSoftShadows);
                    if (ImGui::MenuItem("Depth of Field", 0, curS==C_sceneRTDoF))
                        sv->onCommand(C_sceneRTDoF);
                    if (ImGui::MenuItem("Lens Test", 0, curS==C_sceneRTLens))
                        sv->onCommand(C_sceneRTLens);
                    if (ImGui::MenuItem("RT Test", 0, curS==C_sceneRTTest))
                        sv->onCommand(C_sceneRTTest);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Path tracing"))
                {
                    if (ImGui::MenuItem("Muttenzer Box", 0, curS==C_sceneRTMuttenzerBox))
                        sv->onCommand(C_sceneRTMuttenzerBox);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit & Save"))
                sv->onCommand(C_quit);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Preferences"))
        {
            if (ImGui::MenuItem("Slow down on Idle", 0, sv->waitEvents()))
                sv->onCommand(C_waitEventsToggle);

            if (ImGui::MenuItem("Do Multi Sampling", 0, sv->doMultiSampling()))
                sv->onCommand(C_multiSampleToggle);

            if (ImGui::MenuItem("Do Frustum Culling", 0, sv->doFrustumCulling()))
                sv->onCommand(C_frustCullToggle);

            if (ImGui::MenuItem("Do Depth Test", 0, sv->doDepthTest()))
                sv->onCommand(C_depthTestToggle);

            if (ImGui::MenuItem("Animation off", 0, s->stopAnimations()))
                s->stopAnimations(!s->stopAnimations());

            ImGui::Separator();

            if (ImGui::BeginMenu("Rotation Sensor"))
            {
                if (ImGui::MenuItem("Use Device Rotation", 0, s->usesRotation()))
                    s->usesRotation(!s->usesRotation());

                if (ImGui::MenuItem("Zero Yaw at Start", 0, s->zeroYawAtStart()))
                    s->zeroYawAtStart(!s->zeroYawAtStart());

                if (ImGui::MenuItem("Reset Zero Yaw"))
                    s->deviceRotStarted(true);

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Use GPS Sensor", 0, s->usesLocation()))
                s->usesLocation(!s->usesLocation());
            ImGui::Separator();

            if (ImGui::BeginMenu("Video"))
            {
                SLCVCalibration* ac = s->activeCalib();
                SLCVCalibration* mc = s->calibMainCam();
                SLCVCalibration* sc = s->calibScndCam();

                SLCVTrackedFeatures* featureTracker = nullptr;
                for (auto tracker : s->trackers())
                {   if (typeid(*tracker)==typeid(SLCVTrackedFeatures))
                    {   featureTracker = (SLCVTrackedFeatures*)tracker;
                        break;
                    }
                }

                if (ImGui::BeginMenu("Mirror Main Camera"))
                {
                    if (ImGui::MenuItem("Horizontally", 0, mc->isMirroredH()))
                        sv->onCommand(C_mirrorHMainVideoToggle);

                    if (ImGui::MenuItem("Vertically", 0, mc->isMirroredV()))
                        sv->onCommand(C_mirrorVMainVideoToggle);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Mirror Scnd. Camera", SLCVCapture::hasSecondaryCamera))
                {
                    if (ImGui::MenuItem("Horizontally", 0, sc->isMirroredH()))
                        sv->onCommand(C_mirrorHScndVideoToggle);

                    if (ImGui::MenuItem("Vertically", 0, sc->isMirroredV()))
                        sv->onCommand(C_mirrorVScndVideoToggle);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Calibration"))
                {
                    if (ImGui::MenuItem("Start Calibration on Main Camera"))
                    {
                        sv->onCommand(C_sceneVideoCalibrateMain);
                        showHelpCalibration = true;
                        showInfosScene = true;
                    }

                    if (ImGui::MenuItem("Start Calibration on Scnd. Camera", 0, false, SLCVCapture::hasSecondaryCamera))
                    {
                        sv->onCommand(C_sceneVideoCalibrateScnd);
                        showHelpCalibration = true;
                        showInfosScene = true;
                    }

                    if (ImGui::MenuItem("Undistort Image", 0, ac->showUndistorted(), ac->state()==CS_calibrated))
                        sv->onCommand(C_undistortVideoToggle);

                    if (ImGui::MenuItem("Zero Tangent Distortion", 0, ac->calibZeroTangentDist()))
                        sv->onCommand(C_calibZeroTangentDistToggle);

                    if (ImGui::MenuItem("Fix Aspect Ratio", 0, ac->calibFixAspectRatio()))
                        sv->onCommand(C_calibFixAspectRatioToggle);

                    if (ImGui::MenuItem("Fix Prinicpal Point", 0, ac->calibFixPrincipalPoint()))
                        sv->onCommand(C_calibFixPrincipPointalToggle);

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Show Tracking Detection", 0, s->showDetection()))
                    s->showDetection(!s->showDetection());

                if (ImGui::BeginMenu("Feature Tracking", featureTracker!=nullptr))
                {
                    if (ImGui::MenuItem("Force Relocation", 0, featureTracker->forceRelocation()))
                        featureTracker->forceRelocation(!featureTracker->forceRelocation());

                    if (ImGui::BeginMenu("Detector/Descriptor", featureTracker!=nullptr))
                    {
                        SLCVDetectDescribeType type = featureTracker->type();

                        if (ImGui::MenuItem("RAUL/RAUL", 0, type == DDT_RAUL_RAUL))
                            featureTracker->type(DDT_RAUL_RAUL);
                        if (ImGui::MenuItem("ORB/ORB", 0, type == DDT_ORB_ORB))
                            featureTracker->type(DDT_ORB_ORB);
                        if (ImGui::MenuItem("FAST/BRIEF", 0, type == DDT_FAST_BRIEF))
                            featureTracker->type(DDT_FAST_BRIEF);
                        if (ImGui::MenuItem("SURF/SURF", 0, type == DDT_SURF_SURF))
                            featureTracker->type(DDT_SURF_SURF);
                        if (ImGui::MenuItem("SIFT/SIFT", 0, type == DDT_SIFT_SIFT))
                            featureTracker->type(DDT_SIFT_SIFT);

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("User Interface"))
            {
                ImGui::SliderFloat("Prop. Font Size", &SLGLImGui::fontPropDots, 16.f, 60.f,"%0.0f");

                ImGui::SliderFloat("Fixed Font Size", &SLGLImGui::fontFixedDots, 13.f, 60.f,"%0.0f");

                ImGuiStyle& style = ImGui::GetStyle();
                if (ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f"))
                    style.WindowPadding.x = style.FramePadding.x;
                if (ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f"))
                    style.ItemInnerSpacing.x = style.ItemSpacing.y;

                ImGui::Separator();

                if (ImGui::MenuItem("Reset User Interface"))
                {
                    SLstring fullPathFilename = SL::configPath + "DemoGui.yml";
                    SLFileSystem::deleteFile(fullPathFilename);
                    loadConfig(SL::dpi);
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer"))
        {
            if (ImGui::MenuItem("OpenGL", 0, rType==RT_gl))
                sv->onCommand(C_renderOpenGL);

            if (ImGui::MenuItem("Ray Tracing", 0, rType==RT_rt))
                sv->onCommand(C_rt5);

            if (ImGui::MenuItem("Path Tracing", 0, rType==RT_pt))
                sv->onCommand(C_pt10);

            ImGui::EndMenu();
        }

        if (rType == RT_gl)
        {
            if (ImGui::BeginMenu("Setting"))
            {
                if (ImGui::MenuItem("Wired Mesh", 0, sv->drawBits()->get(SL_DB_WIREMESH)))
                    sv->onCommand(C_wireMeshToggle);

                if (ImGui::MenuItem("Normals", 0, sv->drawBits()->get(SL_DB_NORMALS)))
                    sv->onCommand(C_normalsToggle);

                if (ImGui::MenuItem("Voxels", 0, sv->drawBits()->get(SL_DB_VOXELS)))
                    sv->onCommand(C_voxelsToggle);

                if (ImGui::MenuItem("Axis", 0, sv->drawBits()->get(SL_DB_AXIS)))
                    sv->onCommand(C_axisToggle);

                if (ImGui::MenuItem("Bounding Boxes", 0, sv->drawBits()->get(SL_DB_BBOX)))
                    sv->onCommand(C_bBoxToggle);

                if (ImGui::MenuItem("Skeleton", 0, sv->drawBits()->get(SL_DB_SKELETON)))
                    sv->onCommand(C_skeletonToggle);

                if (ImGui::MenuItem("Back Faces", 0, sv->drawBits()->get(SL_DB_CULLOFF)))
                    sv->onCommand(C_faceCullToggle);

                if (ImGui::MenuItem("Textures off", 0, sv->drawBits()->get(SL_DB_TEXOFF)))
                    sv->onCommand(C_textureToggle);

                if (ImGui::MenuItem("All off"))
                    sv->drawBits()->allOff();

                if (ImGui::MenuItem("All on"))
                {
                    sv->drawBits()->on(SL_DB_WIREMESH);
                    sv->drawBits()->on(SL_DB_NORMALS);
                    sv->drawBits()->on(SL_DB_VOXELS);
                    sv->drawBits()->on(SL_DB_AXIS);
                    sv->drawBits()->on(SL_DB_BBOX);
                    sv->drawBits()->on(SL_DB_SKELETON);
                    sv->drawBits()->on(SL_DB_CULLOFF);
                    sv->drawBits()->on(SL_DB_TEXOFF);
                }

                ImGui::EndMenu();
            }
        }
        else if (rType == RT_rt)
        {
            if (ImGui::BeginMenu("Settings"))
            {
                SLRaytracer* rt = sv->raytracer();

                if (ImGui::MenuItem("Parallel distributed", 0, rt->distributed()))
                    sv->onCommand(C_rtDistributed);

                if (ImGui::MenuItem("Continuously", 0, rt->continuous()))
                    sv->onCommand(C_rtContinuously);

                if (ImGui::BeginMenu("Max. Depth"))
                {
                    if (ImGui::MenuItem("1", 0, rt->maxDepth()==1))
                        sv->onCommand(C_rt1);
                    if (ImGui::MenuItem("2", 0, rt->maxDepth()==2))
                        sv->onCommand(C_rt2);
                    if (ImGui::MenuItem("3", 0, rt->maxDepth()==3))
                        sv->onCommand(C_rt3);
                    if (ImGui::MenuItem("5", 0, rt->maxDepth()==5))
                        sv->onCommand(C_rt5);
                    if (ImGui::MenuItem("Max. Contribution", 0, rt->maxDepth()==0))
                        sv->onCommand(C_rt0);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Anti-Aliasing Sub Samples"))
                {
                    if (ImGui::MenuItem("Off", 0, rt->aaSamples()==1))
                        rt->aaSamples(1);
                    if (ImGui::MenuItem("3x3", 0, rt->aaSamples()==3))
                        rt->aaSamples(3);
                    if (ImGui::MenuItem("5x5", 0, rt->aaSamples()==5))
                        rt->aaSamples(5);
                    if (ImGui::MenuItem("7x7", 0, rt->aaSamples()==7))
                        rt->aaSamples(7);
                    if (ImGui::MenuItem("9x9", 0, rt->aaSamples()==9))
                        rt->aaSamples(9);

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Save Rendered Image"))
                    sv->onCommand(C_rtSaveImage);

                ImGui::EndMenu();
            }
        }
        else if (rType == RT_pt)
        {
            if (ImGui::BeginMenu("Settings"))
            {
                SLPathtracer* pt = sv->pathtracer();

                if (ImGui::BeginMenu("NO. of Samples"))
                {
                    if (ImGui::MenuItem("1", 0, pt->aaSamples()==1))
                        sv->onCommand(C_pt1);
                    if (ImGui::MenuItem("10", 0, pt->aaSamples()==10))
                        sv->onCommand(C_pt10);
                    if (ImGui::MenuItem("100", 0, pt->aaSamples()==100))
                        sv->onCommand(C_pt100);
                    if (ImGui::MenuItem("1000", 0, pt->aaSamples()==1000))
                        sv->onCommand(C_pt1000);
                    if (ImGui::MenuItem("10000", 0, pt->aaSamples()==10000))
                        sv->onCommand(C_pt10000);

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Save Rendered Image"))
                    sv->onCommand(C_ptSaveImage);

                ImGui::EndMenu();
            }
        }

        if (ImGui::BeginMenu("Camera"))
        {
            SLCamera* cam = sv->camera();
            SLProjection proj = cam->projection();

            if (ImGui::MenuItem("Reset"))
                sv->onCommand(C_camReset);

            if (s->numSceneCameras())
            {
                if (ImGui::MenuItem("Set next camera in Scene"))
                    sv->onCommand(C_camSetNextInScene);

                if (ImGui::MenuItem("Set SceneView Camera"))
                    sv->onCommand(C_camSetSceneViewCamera);
            }

            if (ImGui::BeginMenu("Projection"))
            {
                static SLfloat clipN = cam->clipNear();
                static SLfloat clipF = cam->clipFar();
                static SLfloat focalDist = cam->focalDist();
                static SLfloat fov = cam->fov();

                if (ImGui::MenuItem("Perspective", 0, proj==P_monoPerspective))
                    sv->onCommand(C_projPersp);

                if (ImGui::MenuItem("Orthographic", 0, proj==P_monoOrthographic))
                    sv->onCommand(C_projOrtho);

                if (ImGui::BeginMenu("Stereo"))
                {
                    for (SLint p=P_stereoSideBySide; p<=P_stereoColorYB; ++p)
                    {
                        SLstring pStr = SLCamera::projectionToStr((SLProjection)p);
                        if (ImGui::MenuItem(pStr.c_str(), 0, proj==(SLProjection)p))
                            sv->onCommand((SLCommand)(C_projPersp+p));
                    }

                    if (proj >=P_stereoSideBySide)
                    {
                        ImGui::Separator();
                        static SLfloat eyeSepar = cam->eyeSeparation();
                        if (ImGui::SliderFloat("Eye Sep.", &eyeSepar, 0.0f, focalDist/10.f))
                            cam->eyeSeparation(eyeSepar);
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::SliderFloat("FOV", &fov, 1.f, 179.f))
                    cam->fov(fov);

                if (ImGui::SliderFloat("Near Clip", &clipN, 0.001f, 10.f))
                    cam->clipNear(clipN);

                if (ImGui::SliderFloat("Far Clip",  &clipF, clipN, SL_min(clipF*1.1f,1000000.f)))
                    cam->clipFar(clipF);

                if (ImGui::SliderFloat("Focal Dist.", &focalDist, clipN, clipF))
                    cam->focalDist(focalDist);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Animation"))
            {
                SLCamAnim ca = cam->camAnim();

                if (ImGui::MenuItem("Turntable Y up", 0, ca==CA_turntableYUp))
                    sv->onCommand(C_camAnimTurnYUp);

                if (ImGui::MenuItem("Turntable Z up", 0, ca==CA_turntableZUp))
                    sv->onCommand(C_camAnimTurnZUp);

                if (ImGui::MenuItem("Walk Y up", 0, ca==CA_walkingYUp))
                    sv->onCommand(C_camAnimWalkYUp);

                if (ImGui::MenuItem("Walk Z up", 0, ca==CA_walkingZUp))
                    sv->onCommand(C_camAnimWalkZUp);

                if (ImGui::MenuItem("Device Rotated Y up", 0, ca==CA_deviceRotYUp))
                    sv->onCommand(C_camAnimDeviceRotYUp);

                if (ImGui::MenuItem("Device Rotated Y up and GPS positioned", 0, ca == CA_deviceRotYUpPosGPS))
                    sv->onCommand(C_camAnimDeviceRotYUpPosGPS);
                if (ca==CA_walkingZUp || ca==CA_walkingYUp || ca==CA_deviceRotYUp)
                {
                    static SLfloat ms = cam->maxSpeed();
                    if (ImGui::SliderFloat("Walk Speed",  &ms, 0.01f, SL_min(ms*1.1f,10000.f)))
                        cam->maxSpeed(ms);
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Animation", hasAnimations))
        {
            SLVstring animations = s->animManager().allAnimNames();
            if (curAnimIx == -1) curAnimIx = 0;
            SLAnimPlayback* anim = s->animManager().allAnimPlayback(curAnimIx);

            if (myComboBox("", &curAnimIx, animations))
                anim = s->animManager().allAnimPlayback(curAnimIx);

            if (ImGui::MenuItem("Play forward", 0, anim->isPlayingForward()))
                anim->playForward();

            if (ImGui::MenuItem("Play backward", 0, anim->isPlayingBackward()))
                anim->playBackward();

            if (ImGui::MenuItem("Pause", 0, anim->isPaused()))
                anim->pause();

            if (ImGui::MenuItem("Stop", 0, anim->isStopped()))
                anim->enabled(false);

            if (ImGui::MenuItem("Skip to next keyframe", 0, false))
                anim->skipToNextKeyframe();

            if (ImGui::MenuItem("Skip to previous keyframe", 0, false))
                anim->skipToPrevKeyframe();

            if (ImGui::MenuItem("Skip to start", 0, false))
                anim->skipToStart();

            if (ImGui::MenuItem("Skip to end", 0, false))
                anim->skipToEnd();

            SLfloat speed = anim->playbackRate();
            if (ImGui::SliderFloat("Speed", &speed, 0.f, 4.f))
                anim->playbackRate(speed);

            SLfloat lenSec = anim->parentAnimation()->lengthSec();
            SLfloat localTimeSec = anim->localTime();
            if (ImGui::SliderFloat("Time", &localTimeSec, 0.f, lenSec))
                anim->localTime(localTimeSec);

            SLint curEasing = (SLint)anim->easing();
            const char* easings[] = { "linear",
                                      "in quad",  "out quad",  "in out quad",  "out in quad",
                                      "in cubic", "out cubic", "in out cubic", "out in cubic",
                                      "in quart", "out quart", "in out quart", "out in quart",
                                      "in quint", "out quint", "in out quint", "out in quint",
                                      "in sine",  "out sine",  "in out sine",  "out in sine"};
            if (ImGui::Combo("Easing", &curEasing, easings,  IM_ARRAYSIZE(easings)))
                anim->easing((SLEasingCurve)curEasing);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Infos"))
        {
            ImGui::MenuItem("Infos on Scene",      0, &showInfosScene);
            ImGui::MenuItem("Stats on Timing"    , 0, &showStatsTiming);
            ImGui::MenuItem("Stats on Scene"     , 0, &showStatsScene);
            ImGui::MenuItem("Stats on Video"     , 0, &showStatsVideo);
            ImGui::Separator();
            ImGui::MenuItem("Show Scenegraph",     0, &showSceneGraph);
            ImGui::MenuItem("Show Properties",     0, &showProperties);
            ImGui::Separator();
            ImGui::MenuItem("Infos on Sensors",    0, &showInfosSensors);
            ImGui::MenuItem("Infos on Frameworks", 0, &showInfosFrameworks);
            ImGui::Separator();
            ImGui::MenuItem("Help on Interaction", 0, &showHelp);
            ImGui::MenuItem("Help on Calibration", 0, &showHelpCalibration);
            ImGui::Separator();
            ImGui::MenuItem("Credits"            , 0, &showCredits);
            ImGui::MenuItem("About SLProject"    , 0, &showAbout);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
//-----------------------------------------------------------------------------
void SLDemoGui::buildSceneGraph(SLScene* s)
{
    ImGui::Begin("Scenegraph", &showSceneGraph);

    if (s->root3D())
        addSceneGraphNode(s, s->root3D());

    ImGui::End();
}
//-----------------------------------------------------------------------------
void SLDemoGui::addSceneGraphNode(SLScene* s, SLNode* node)
{
    SLbool isSelectedNode = s->selectedNode()==node;
    SLbool isLeafNode = node->children().size()==0 && node->meshes().size()==0;

    ImGuiTreeNodeFlags nodeFlags = 0;
    if (isLeafNode)
         nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    else nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;

    if (isSelectedNode)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    bool nodeIsOpen = ImGui::TreeNodeEx(node->name().c_str(), nodeFlags);

    if (ImGui::IsItemClicked())
        s->selectNodeMesh(node, 0);

    if (nodeIsOpen)
    {
        for(auto mesh : node->meshes())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImColor(1.0f,1.0f,0.0f));

            ImGuiTreeNodeFlags meshFlags = ImGuiTreeNodeFlags_Leaf;
            if (s->selectedMesh()==mesh)
                meshFlags |= ImGuiTreeNodeFlags_Selected;
          //ImGui::TreeNodeEx(mesh->name().c_str(), meshFlags);
            ImGui::TreeNodeEx(mesh, meshFlags, mesh->name().c_str());

            if (ImGui::IsItemClicked())
                s->selectNodeMesh(node, mesh);

            ImGui::TreePop();
            ImGui::PopStyleColor();
        }

        for (auto child : node->children())
            addSceneGraphNode(s, child);

        ImGui::TreePop();
    }
}
//-----------------------------------------------------------------------------
void SLDemoGui::buildProperties(SLScene* s)
{
    SLNode* node = s->selectedNode();
    SLMesh* mesh = s->selectedMesh();
    ImGuiColorEditFlags colFlags = ImGuiColorEditFlags_NoSliders;

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::Begin("Properties", &showProperties);

    if (ImGui::TreeNode("Node Properties"))
    {
        if (node)
        {   SLuint c = (SLuint)node->children().size();
            SLuint m = (SLuint)node->meshes().size();

            ImGui::Text("Node Name       : %s", node->name().c_str());
            ImGui::Text("No. of children : %u", c);
            ImGui::Text("No. of meshes   : %u", m);
            if (ImGui::TreeNode("Drawing Flags"))
            {
                SLbool db;
                db = node->drawBit(SL_DB_HIDDEN);
                if (ImGui::Checkbox("Hide", &db))
                    node->drawBits()->set(SL_DB_HIDDEN, db);

                db = node->drawBit(SL_DB_WIREMESH);
                if (ImGui::Checkbox("Show wireframe", &db))
                    node->drawBits()->set(SL_DB_WIREMESH, db);

                db = node->drawBit(SL_DB_NORMALS);
                if (ImGui::Checkbox("Show normals", &db))
                    node->drawBits()->set(SL_DB_NORMALS, db);

                db = node->drawBit(SL_DB_VOXELS);
                if (ImGui::Checkbox("Show voxels", &db))
                    node->drawBits()->set(SL_DB_VOXELS, db);

                db = node->drawBit(SL_DB_BBOX);
                if (ImGui::Checkbox("Show bounding boxes", &db))
                    node->drawBits()->set(SL_DB_BBOX, db);

                db = node->drawBit(SL_DB_AXIS);
                if (ImGui::Checkbox("Show axis", &db))
                    node->drawBits()->set(SL_DB_AXIS, db);

                db = node->drawBit(SL_DB_CULLOFF);
                if (ImGui::Checkbox("Show back faces", &db))
                    node->drawBits()->set(SL_DB_CULLOFF, db);

                db = node->drawBit(SL_DB_TEXOFF);
                if (ImGui::Checkbox("No textures", &db))
                    node->drawBits()->set(SL_DB_TEXOFF, db);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Local Transform"))
            {
                SLMat4f om(node->om());
                SLVec3f t, r, s;
                om.decompose(t, r, s);
                r *= SL_RAD2DEG;

                ImGui::Text("Translation  : %s", t.toString().c_str());
                ImGui::Text("Rotation     : %s", r.toString().c_str());
                ImGui::Text("Scaling      : %s", s.toString().c_str());
                ImGui::TreePop();
            }

            // Show special camera properties
            if (typeid(*node)==typeid(SLCamera))
            {
                SLCamera* cam = (SLCamera*)node;

                if (ImGui::TreeNode("Camera"))
                {
                    SLfloat clipN = cam->clipNear();
                    SLfloat clipF = cam->clipFar();
                    SLfloat focalDist = cam->focalDist();
                    SLfloat fov = cam->fov();

                    const char* projections[] = {"Mono Perspective",
                                                 "Mono Orthographic",
                                                 "Stereo Side By Side",
                                                 "Stereo Side By Side Prop.",
                                                 "Stereo Side By Side Dist.",
                                                 "Stereo Line By Line",
                                                 "Stereo Column By Column",
                                                 "Stereo Pixel By Pixel",
                                                 "Stereo Color Red Cyan",
                                                 "Stereo Color Red Green",
                                                 "Stereo Color Red Blue",
                                                 "Stereo Color Yelle Blue" };

                    int proj = cam->projection();
                    if (ImGui::Combo("Projection", &proj, projections, IM_ARRAYSIZE(projections)))
                        cam->projection((SLProjection)proj);

                    if (cam->projection() > P_monoOrthographic)
                    {
                        SLfloat eyeSepar = cam->eyeSeparation();
                        if (ImGui::SliderFloat("Eye Sep.", &eyeSepar, 0.0f, focalDist/10.f))
                            cam->eyeSeparation(eyeSepar);
                    }

                    if (ImGui::SliderFloat("FOV", &fov, 1.f, 179.f))
                        cam->fov(fov);

                    if (ImGui::SliderFloat("Near Clip", &clipN, 0.001f, 10.f))
                        cam->clipNear(clipN);

                    if (ImGui::SliderFloat("Far Clip",  &clipF, clipN, SL_min(clipF*1.1f,1000000.f)))
                        cam->clipFar(clipF);

                    if (ImGui::SliderFloat("Focal Dist.", &focalDist, clipN, clipF))
                        cam->focalDist(focalDist);

                    ImGui::TreePop();
                }
            }

            // Show special light properties
            if (typeid(*node)==typeid(SLLightSpot) ||
                typeid(*node)==typeid(SLLightRect) ||
                typeid(*node)==typeid(SLLightDirect))
            {
                SLLight* light;
                SLstring typeName;
                if (typeid(*node)==typeid(SLLightSpot))
                {   light = (SLLight*)(SLLightSpot*)node;
                    typeName = "Light (spot):";
                }
                if (typeid(*node)==typeid(SLLightRect))
                {   light = (SLLight*)(SLLightRect*)node;
                    typeName = "Light (rectangular):";
                }
                if (typeid(*node)==typeid(SLLightDirect))
                {   light = (SLLight*)(SLLightDirect*)node;
                    typeName = "Light (directional):";
                }

                if (ImGui::TreeNode(typeName.c_str()))
                {
                    SLbool on = light->isOn();
                    if (ImGui::Checkbox("Is on", &on))
                        light->isOn(on);

                    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
                    SLCol4f a = light->ambient();
                    if (ImGui::InputFloat3("Ambient", (float*)&a, 1, flags))
                        light->ambient(a);

                    SLCol4f d = light->diffuse();
                    if (ImGui::InputFloat3("Diffuse", (float*)&d, 1, flags))
                        light->diffuse(d);

                    SLCol4f s = light->specular();
                    if (ImGui::InputFloat3("Specular", (float*)&s, 1, flags))
                        light->specular(s);

                    float cutoff = light->spotCutOffDEG();
                    if (ImGui::SliderFloat("Spot cut off angle", &cutoff, 0.0f, 180.0f))
                        light->spotCutOffDEG(cutoff);

                    float kc = light->kc();
                    if (ImGui::SliderFloat("Constant attenutation", &kc, 0.0f, 1.0f))
                        light->kc(kc);

                    float kl = light->kl();
                    if (ImGui::SliderFloat("Linear attenutation", &kl, 0.0f, 1.0f))
                        light->kl(kl);

                    float kq = light->kq();
                    if (ImGui::SliderFloat("Quadradic attenutation", &kq, 0.0f, 1.0f))
                        light->kq(kq);

                    ImGui::TreePop();
                }
            }
        } else
        {
            ImGui::Text("No node selected.");
        }
        ImGui::TreePop();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(1.0f,1.0f,0.0f));
    ImGui::Separator();
    if (ImGui::TreeNode("Mesh Properties"))
    {
        if (mesh)
        {   SLuint v = (SLuint)mesh->P.size();
            SLuint t = (SLuint)(mesh->I16.size() ? mesh->I16.size() : mesh->I32.size());
            SLMaterial* m = mesh->mat;
            ImGui::Text("Mesh Name       : %s", mesh->name().c_str());
            ImGui::Text("No. of Vertices : %u", v);
            ImGui::Text("No. of Triangles: %u", t);

            if (m && ImGui::TreeNode("Material"))
            {
                ImGui::Text("Material Name: %s", m->name().c_str());

                if (ImGui::TreeNode("Reflection colors"))
                {
                    SLCol4f ac = m->ambient();
                    if (ImGui::ColorEdit3("Ambient color",  (float*)&ac, colFlags))
                        m->ambient(ac);

                    SLCol4f dc = m->diffuse();
                    if (ImGui::ColorEdit3("Diffuse color",  (float*)&dc, colFlags))
                        m->diffuse(dc);

                    SLCol4f sc = m->specular();
                    if (ImGui::ColorEdit3("Specular color",  (float*)&sc, colFlags))
                        m->specular(sc);

                    SLCol4f ec = m->emissive();
                    if (ImGui::ColorEdit3("Emissive color",  (float*)&ec, colFlags))
                        m->emissive(ec);

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Other variables"))
                {
                    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);

                    SLfloat shine = m->shininess();
                    if (ImGui::SliderFloat("Shininess", &shine, 0.0f, 1000.0f))
                        m->shininess(shine);

                    SLfloat rough = m->roughness();
                    if (ImGui::SliderFloat("Roughness", &rough, 0.0f, 1.0f))
                        m->roughness(rough);

                    SLfloat metal = m->metalness();
                    if (ImGui::SliderFloat("Metalness", &metal, 0.0f, 1.0f))
                        m->metalness(metal);

                    SLfloat kr = m->kr();
                    if (ImGui::SliderFloat("kr", &kr, 0.0f, 1.0f))
                        m->kr(kr);

                    SLfloat kt = m->kt();
                    if (ImGui::SliderFloat("kt", &kt, 0.0f, 1.0f))
                        m->kt(kt);

                    SLfloat kn = m->kn();
                    if (ImGui::SliderFloat("kn", &kn, 1.0f, 2.5f))
                        m->kn(kn);

                    ImGui::PopItemWidth();
                    ImGui::TreePop();
                }

                if (m->textures().size() && ImGui::TreeNode("Textures"))
                {
                    ImGui::Text("No. of textures: %u", m->textures().size());

                    SLfloat lineH = ImGui::GetTextLineHeightWithSpacing();
                    SLfloat texW  = ImGui::GetWindowWidth() - 4*ImGui::GetTreeNodeToLabelSpacing() - 10;


                    for (SLint i=0; i<m->textures().size(); ++i)
                    {
                        SLGLTexture* t = m->textures()[i];
                        void* tid = (ImTextureID)t->texName();
                        SLfloat w = (SLfloat)t->width();
                        SLfloat h = (SLfloat)t->height();
                        SLfloat h_to_w = h / w;

                        if (ImGui::TreeNode(t->name().c_str()))
                        {
                            ImGui::Text("Size    : %d x %d x %d", t->width(), t->height(), t->depth());
                            ImGui::Text("Type    : %s", t->typeName().c_str());

                            if (t->depth() > 1)
                                ImGui::Text("3D textures can not be displayed.");
                            else
                            {   if (typeid(*t)==typeid(SLTransferFunction))
                                {
                                    SLTransferFunction* tf = (SLTransferFunction*)m->textures()[i];
                                    if (ImGui::TreeNode("Color Points in Transfer Function"))
                                    {
                                        for (SLint c = 0; c < tf->colors().size(); ++c)
                                        {
                                            SLCol3f color = tf->colors()[c].color;
                                            SLchar label[20]; sprintf(label, "Color %u", c);
                                            if (ImGui::ColorEdit3(label, (float*)&color, colFlags))
                                            {   tf->colors()[c].color = color;
                                                tf->generateTexture();
                                            }
                                            ImGui::SameLine();
                                            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
                                            sprintf(label, "Pos. %u", c);
                                            SLfloat pos = tf->colors()[c].pos;
                                            if (c > 0 && c < tf->colors().size()-1)
                                            {   SLfloat min = tf->colors()[c-1].pos + 2.0f/(SLfloat)tf->length();
                                                SLfloat max = tf->colors()[c+1].pos - 2.0f/(SLfloat)tf->length();
                                                if (ImGui::SliderFloat(label, &pos, min, max, "%3.2f"))
                                                {   tf->colors()[c].pos = pos;
                                                    tf->generateTexture();
                                                }
                                            } else ImGui::Text("%3.2f Pos. %u", pos, c);
                                            ImGui::PopItemWidth();
                                        }

                                        ImGui::TreePop();
                                    }

                                    if (ImGui::TreeNode("Alpha Points in Transfer Function"))
                                    {
                                        for (SLint a = 0; a < tf->alphas().size(); ++a)
                                        {
                                            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
                                            SLfloat alpha = tf->alphas()[a].alpha;
                                            SLchar label[20]; sprintf(label, "Alpha %u", a);
                                            if (ImGui::SliderFloat(label, &alpha, 0.0f, 1.0f, "%3.2f"))
                                            {   tf->alphas()[a].alpha = alpha;
                                                tf->generateTexture();
                                            }
                                            ImGui::SameLine();
                                            sprintf(label, "Pos. %u", a);
                                            SLfloat pos = tf->alphas()[a].pos;
                                            if (a > 0 && a < tf->alphas().size()-1)
                                            {   SLfloat min = tf->alphas()[a-1].pos + 2.0f/(SLfloat)tf->length();
                                                SLfloat max = tf->alphas()[a+1].pos - 2.0f/(SLfloat)tf->length();
                                                if (ImGui::SliderFloat(label, &pos, min, max, "%3.2f"))
                                                {   tf->alphas()[a].pos = pos;
                                                    tf->generateTexture();
                                                }
                                            } else ImGui::Text("%3.2f Pos. %u", pos, a);

                                            ImGui::PopItemWidth();
                                        }

                                        ImGui::TreePop();
                                    }

                                    ImGui::Image(tid, ImVec2(texW, texW * 0.25f), ImVec2(0,1), ImVec2(1,0), ImVec4(1,1,1,1), ImVec4(1,1,1,1));

                                    SLVfloat allAlpha = tf->allAlphas();
                                    ImGui::PlotLines("", allAlpha.data(), (SLint)allAlpha.size(), 0, 0, 0.0f, 1.0f, ImVec2(texW, texW * 0.25f));

                                } else
                                {
                                    ImGui::Image(tid, ImVec2(texW, texW * h_to_w), ImVec2(0,1), ImVec2(1,0), ImVec4(1,1,1,1), ImVec4(1,1,1,1));
                                }
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("GLSL Program"))
                {
                    for (SLint i=0; i<m->program()->shaders().size(); ++i)
                    {
                        SLGLShader* s = m->program()->shaders()[i];
                        SLfloat lineH = ImGui::GetTextLineHeight();

                        if (ImGui::TreeNode(s->name().c_str()))
                        {
                            SLchar text[1024*16];
                            strcpy(text, s->code().c_str());
                            ImGui::InputTextMultiline(s->name().c_str(), text, IM_ARRAYSIZE(text), ImVec2(-1.0f, lineH * 16));
                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

        } else
        {
            ImGui::Text("No mesh selected.");
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
    ImGui::End();
    ImGui::PopFont();
}
//-----------------------------------------------------------------------------
void SLDemoGui::loadConfig(SLint dotsPerInch)
{
    ImGuiStyle& style = ImGui::GetStyle();
    SLstring fullPathAndFilename = SL::configPath + "DemoGui.yml";

    if (!SLFileSystem::fileExists(fullPathAndFilename))
    {
        SLfloat dpiScale = dotsPerInch / 142.0f;

        // Default settings for the first time
        SLGLImGui::fontPropDots  = SL_max(16.0f * dpiScale, 16.0f);
        SLGLImGui::fontFixedDots = SL_max(13.0f * dpiScale, 13.0f);
        SLDemoGui::showAbout = true;
        SLDemoGui::showInfosScene = true;
        SLDemoGui::showStatsTiming = false;
        SLDemoGui::showStatsScene = false;
        SLDemoGui::showStatsVideo = false;
        SLDemoGui::showInfosFrameworks = false;
        SLDemoGui::showInfosSensors = false;
        SLDemoGui::showSceneGraph = false;
        SLDemoGui::showProperties = false;
        style.FramePadding.x = SL_max(8.0f * dpiScale, 8.0f);
        style.WindowPadding.x = style.FramePadding.x;
        style.FramePadding.y = SL_max(3.0f * dpiScale, 3.0f);
        style.ItemSpacing.x = SL_max(8.0f * dpiScale, 8.0f);
        style.ItemSpacing.y = SL_max(3.0f * dpiScale, 3.0f);
        style.ItemInnerSpacing.x = style.ItemSpacing.y;
        return;
    }

    SLCVFileStorage fs;
    try
    {   fs.open(fullPathAndFilename, SLCVFileStorage::READ);
        if (!fs.isOpened())
        {   SL_LOG("Failed to open file for reading: %s", fullPathAndFilename.c_str());
            return;
        }
    }
    catch(...)
    {   SL_LOG("Parsing of file failed: %s", fullPathAndFilename.c_str());
        return;
    }

    SLint i; SLbool b;
    fs["configTime"]            >> SLDemoGui::configTime;
    fs["fontPropDots"]          >> i; SLGLImGui::fontPropDots = (SLfloat)i;
    fs["fontFixedDots"]         >> i; SLGLImGui::fontFixedDots = (SLfloat)i;
    fs["FramePaddingX"]         >> i; style.FramePadding.x = (SLfloat)i;
                                      style.WindowPadding.x = style.FramePadding.x;
    fs["FramePaddingY"]         >> i; style.FramePadding.y = (SLfloat)i;
    fs["ItemSpacingX"]          >> i; style.ItemSpacing.x = (SLfloat)i;
    fs["ItemSpacingY"]          >> i; style.ItemSpacing.y = (SLfloat)i;
                                      style.ItemInnerSpacing.x = style.ItemSpacing.y;
    fs["currentSceneID"]        >> i; SL::currentSceneID = (SLCommand)i;
    fs["showInfosScene"]        >> b; SLDemoGui::showInfosScene = b;
    fs["showStatsTiming"]       >> b; SLDemoGui::showStatsTiming = b;
    fs["showStatsMemory"]       >> b; SLDemoGui::showStatsScene = b;
    fs["showStatsVideo"]        >> b; SLDemoGui::showStatsVideo = b;
    fs["showInfosFrameworks"]   >> b; SLDemoGui::showInfosFrameworks = b;
    fs["showInfosSensors"]      >> b; SLDemoGui::showInfosSensors = b;
    fs["showSceneGraph"]        >> b; SLDemoGui::showSceneGraph = b;
    fs["showProperties"]        >> b; SLDemoGui::showProperties = b;
    fs["showDetection"]         >> b; SLScene::current->showDetection(b);

    fs.release();
    SL_LOG("Config. loaded  : %s\n", fullPathAndFilename.c_str());
}
//-----------------------------------------------------------------------------
void SLDemoGui::saveConfig()
{
    ImGuiStyle& style = ImGui::GetStyle();
    SLstring fullPathAndFilename = SL::configPath + "DemoGui.yml";
    SLCVFileStorage fs(fullPathAndFilename, SLCVFileStorage::WRITE);

    if (!fs.isOpened())
    {   SL_LOG("Failed to open file for writing: %s", fullPathAndFilename.c_str());
        SL_EXIT_MSG("Exit in SLDemoGui::saveConfig");
        return;
    }

    fs << "configTime"              << SLUtils::getLocalTimeString();
    fs << "fontPropDots"            << (SLint)SLGLImGui::fontPropDots;
    fs << "fontFixedDots"           << (SLint)SLGLImGui::fontFixedDots;
    fs << "currentSceneID"          << (SLint)SL::currentSceneID;
    fs << "FramePaddingX"           << (SLint)style.FramePadding.x;
    fs << "FramePaddingY"           << (SLint)style.FramePadding.y;
    fs << "ItemSpacingX"            << (SLint)style.ItemSpacing.x;
    fs << "ItemSpacingY"            << (SLint)style.ItemSpacing.y;
    fs << "showStatsTiming"         << SLDemoGui::showStatsTiming;
    fs << "showStatsMemory"         << SLDemoGui::showStatsScene;
    fs << "showStatsVideo"          << SLDemoGui::showStatsVideo;
    fs << "showInfosFrameworks"     << SLDemoGui::showInfosFrameworks;
    fs << "showInfosScene"          << SLDemoGui::showInfosScene;
    fs << "showInfosSensors"        << SLDemoGui::showInfosSensors;
    fs << "showSceneGraph"          << SLDemoGui::showSceneGraph;
    fs << "showProperties"          << SLDemoGui::showProperties;
    fs << "showDetection"           << SLScene::current->showDetection();

    fs.release();
    SL_LOG("Config. saved   : %s\n", fullPathAndFilename.c_str());
}
//-----------------------------------------------------------------------------

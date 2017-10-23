//#############################################################################
//  File:      GLES3Activity.java
//  Author:    Marcus Hudritsch, Zingg Pascal
//  Date:      Spring 2017
//  Purpose:   Android Java toplevel activity class
//  Codestyle: https://github.com/cpvrlab/SLProject/wiki/Coding-Style-Guidelines
//  Copyright: Marcus Hudritsch, Zingg Pascal
//             This software is provide under the GNU General Public License
//             Please visit: http://opensource.org/licenses/GPL-3.0
//#############################################################################

// Please do not change the name space. The SLProject app is identified in the app-store with it.
package ch.fhnw.comgr;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.camera2.CameraCharacteristics;
import android.location.GnssStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;


public class GLES3Activity extends Activity implements View.OnTouchListener, SensorEventListener {
    GLES3View myView;             // OpenGL view
    static int pointersDown = 0;   // NO. of fingers down
    static long lastTouchMS = 0;    // Time of last touch in ms

    private static final String TAG = "SLProject";
    private int _currentVideoType;
    private boolean _cameraPermissionGranted;
    private boolean _gpsPermissionGranted;
    private boolean _permissionRequestIsOpen;
    private boolean _rotationSensorIsRunning = false;
    private boolean _gpsSensorIsRunning = false;
    protected LocationManager gpsLocationManager;
    private GeneralLocationListener gpsLocationListener;
    private static int MY_PERMISSION_ACCESS_COURSE_LOCATION = 0;

    @Override
    protected void onCreate(Bundle icicle) {
        Log.i(TAG, "GLES3Activity.onCreate");
        super.onCreate(icicle);

        // Extract (unzip) files in APK
        try {
            Log.i(TAG, "extractAPK");
            GLES3Lib.App = getApplication();
            GLES3Lib.extractAPK();
        } catch (IOException e) {
            Log.e(TAG, "Error extracting files from the APK archive: " + e.getMessage());
        }

        // Create view
        myView = new GLES3View(GLES3Lib.App);
        GLES3Lib.view = myView;
        GLES3Lib.activity = this;
        myView.setOnTouchListener(this);
        Log.i(TAG, "setContentView");
        setContentView(myView);

        // Get display resolution. This is used to scale the menu buttons accordingly
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        int dpi = (int) (((float) metrics.xdpi + (float) metrics.ydpi) * 0.5);
        GLES3Lib.dpi = dpi;
        Log.i(TAG, "DisplayMetrics: " + dpi);

        // Init Camera (the camera is started by cameraStart from within the view renderer)
        Log.i(TAG, "Request camera permission ...");
        //If we are on android 5.1 or lower the permission was granted during installation.
        //On Android 6 or higher it requests a dangerous permission during runtime.
        //On Android 7 there could be problems that permissions where not granted
        //(Huawei Honor 8 must enable soecial log setting by dialing *#*#2846579#*#*)
        if (ActivityCompat.checkSelfPermission(GLES3Activity.this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED)
            _cameraPermissionGranted = true;
        else {
            _permissionRequestIsOpen = true;
            ActivityCompat.requestPermissions(GLES3Activity.this, new String[]{Manifest.permission.CAMERA}, 1);
        }

        // Init GPS (the GPS is started by gpsSensorStart from within the view renderer)
        Log.i(TAG, "Request GPS permission ...");
        if (ActivityCompat.checkSelfPermission(GLES3Activity.this, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(GLES3Activity.this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
            _gpsPermissionGranted = true;
        else {
            _permissionRequestIsOpen = true;
            ActivityCompat.requestPermissions(GLES3Activity.this, new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, 1);
            ActivityCompat.requestPermissions(GLES3Activity.this, new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, 1);
        }
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "GLES3Activity.onPause");

        // The ActivityCompat.requestPermissions calls also onPause
        if (!_permissionRequestIsOpen) {
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onClose();
                }
            });
            cameraStop();
            finishAndRemoveTask();
        }

        super.onPause();
    }

    @Override
    protected void onStop() {
        Log.i(TAG, "GLES3Activity.onStop");
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        Log.i(TAG, "GLES3Activity.onDestroy");
        super.onDestroy();
    }

    @Override
    public boolean onTouch(View v, final MotionEvent event) {
        if (event == null) {
            Log.i(TAG, "onTouch: null event");
            return false;
        }

        int action = event.getAction();
        int actionCode = action & MotionEvent.ACTION_MASK;

        try {
            if (actionCode == MotionEvent.ACTION_DOWN ||
                    actionCode == MotionEvent.ACTION_POINTER_DOWN)
                return handleTouchDown(event);
            else if (actionCode == MotionEvent.ACTION_UP ||
                    actionCode == MotionEvent.ACTION_POINTER_UP)
                return handleTouchUp(event);
            else if (actionCode == MotionEvent.ACTION_MOVE)
                return handleTouchMove(event);
            else Log.i(TAG, "Unhandeled Event: " + actionCode);
        } catch (Exception ex) {
            Log.i(TAG, "onTouch (Exception: " + actionCode);
        }

        return false;
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.i(TAG, String.format("onAccuracyChanged"));
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR && _rotationSensorIsRunning) {
            // The ROTATION_VECTOR sensor is a virtual fusion sensor
            // The quality strongly depends on the underlying algorithm and on
            // the sensor manufacturer. (See also chapter 7 in the book:
            // "Professional Sensor Programming (WROX Publishing)"

            // Get 3x3 rotation matrix from XYZ-rotation vector (see docs)
            float R[] = new float[9];
            SensorManager.getRotationMatrixFromVector(R, event.values);

            // Get yaw, pitch & roll rotation angles in radians from rotation matrix
            float[] YPR = new float[3];
            SensorManager.getOrientation(R, YPR);

            // Check display orientation (a preset orientation is set in the AndroidManifext.xml)
            Display display = getWindowManager().getDefaultDisplay();
            DisplayMetrics displaymetrics = new DisplayMetrics();
            display.getMetrics(displaymetrics);
            int screenWidth = displaymetrics.widthPixels;
            int screenHeight = displaymetrics.heightPixels;

            if (screenWidth < screenHeight) {    // Map pitch, yaw and roll to portrait display orientation
                final float p = YPR[1] * -1.0f - (float) Math.PI * 0.5f;
                final float y = YPR[0] * -1.0f;
                final float r = YPR[2] * -1.0f;
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onRotationPYR(p, y, r);
                    }
                });
                //Log.i(TAG, String.format("onSensorChanged: Pitch(%3.0f), Yaw(%3.0f), Roll(%3.0f)", p, y, r));
            }
            else {    // Map pitch, yaw and roll to landscape display orientation for Oculus Rift conformance
                final float p = YPR[2] * -1.0f - (float) Math.PI * 0.5f;
                final float y = YPR[0] * -1.0f - (float) Math.PI * 0.5f;
                final float r = YPR[1];
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onRotationPYR(p, y, r);
                    }
                });
                //Log.i(TAG, String.format("onSensorChanged: Pitch(%3.0f), Yaw(%3.0f), Roll(%3.0f)", p, y, r));
            }

            /*
            // Get the rotation quaternion from the XYZ-rotation vector (see docs)
            final float Q[] = new float[4];
            SensorManager.getQuaternionFromVector(Q, event.values);
            myView.queueEvent(new Runnable() {public void run() {GLES3Lib.onRotationQUAT(Q[1],Q[2],Q[3],Q[0]);}});
            */
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case 1: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, String.format("onRequestPermissionsResult: CAMERA permission granted."));
                    _cameraPermissionGranted = true;
                } else {
                    Log.i(TAG, String.format("onRequestPermissionsResult: CAMERA permission refused."));
                    _cameraPermissionGranted = false;
                }
                _permissionRequestIsOpen = false;
                return;
            }
        }
    }

    /**
     * Events:
     * <p>
     * Finger Down
     * -----------
     * Just tap on screen -> onMouseDown, onMouseUp
     * Tap and hold       -> onMouseDown
     * release   -> onMouseUp
     * 2 Fingers same time -> onTouch2Down
     * 2 Fingers not same time -> onMouseDown, onMouseUp, onTouch2Down
     * <p>
     * Finger Up
     * ---------
     * 2 Down, release one -> onTouch2Up
     * release other one -> onMouseUp
     * 2 Down, release one, put another one down -> onTouch2Up, onTouch2Down
     * 2 Down, release both same time -> onTouch2Up
     * 2 Down, release both not same time -> onTouch2Up
     */

    public boolean handleTouchDown(final MotionEvent event) {
        int touchCount = event.getPointerCount();
        final int x0 = (int) event.getX(0);
        final int y0 = (int) event.getY(0);
        //Log.i(TAG, "Dn:" + touchCount);

        // just got a new single touch
        if (touchCount == 1) {
            // get time to detect double taps
            long touchNowMS = System.currentTimeMillis();
            long touchDeltaMS = touchNowMS - lastTouchMS;
            lastTouchMS = touchNowMS;

            if (touchDeltaMS < 250)
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onDoubleClick(1, x0, y0);
                    }
                });
            else
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onMouseDown(1, x0, y0);
                    }
                });
        }

        // it's two fingers but one delayed (already executed mouse down
        else if (touchCount == 2 && pointersDown == 1) {
            final int x1 = (int) event.getX(1);
            final int y1 = (int) event.getY(1);
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onMouseUp(1, x0, y0);
                }
            });
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onTouch2Down(x0, y0, x1, y1);
                }
            });
            // it's two fingers at the same time
        } else if (touchCount == 2) {
            // get time to detect double taps
            long touchNowMS = System.currentTimeMillis();
            long touchDeltaMS = touchNowMS - lastTouchMS;
            lastTouchMS = touchNowMS;

            final int x1 = (int) event.getX(1);
            final int y1 = (int) event.getY(1);

            if (touchDeltaMS < 250)
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onMenuButton();
                    }
                });
            else
                myView.queueEvent(new Runnable() {
                    public void run() {
                        GLES3Lib.onTouch2Down(x0, y0, x1, y1);
                    }
                });
        }
        pointersDown = touchCount;
        myView.requestRender();
        return true;
    }

    public boolean handleTouchUp(final MotionEvent event) {
        int touchCount = event.getPointerCount();
        //Log.i(TAG, "Up:" + touchCount + " x: " + (int)event.getX(0) + " y: " + (int)event.getY(0));
        final int x0 = (int) event.getX(0);
        final int y0 = (int) event.getY(0);
        if (touchCount == 1) {
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onMouseUp(1, x0, y0);
                }
            });
        } else if (touchCount == 2) {
            final int x1 = (int) event.getX(1);
            final int y1 = (int) event.getY(1);
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onTouch2Up(x0, y0, x1, y1);
                }
            });
        }

        pointersDown = touchCount;
        myView.requestRender();
        return true;
    }

    public boolean handleTouchMove(final MotionEvent event) {
        final int x0 = (int) event.getX(0);
        final int y0 = (int) event.getY(0);
        int touchCount = event.getPointerCount();
        //Log.i(TAG, "Mv:" + touchCount);

        if (touchCount == 1) {
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onMouseMove(x0, y0);
                }
            });
        } else if (touchCount == 2) {
            final int x1 = (int) event.getX(1);
            final int y1 = (int) event.getY(1);
            myView.queueEvent(new Runnable() {
                public void run() {
                    GLES3Lib.onTouch2Move(x0, y0, x1, y1);
                }
            });
        }
        myView.requestRender();
        return true;
    }

    /**
     * Starts the camera service if not running.
     * It is called from the GL view renderer thread.
     * While the service is starting no other calls to startService are allowed.
     *
     * @param requestedVideoType      (0 = GLES3Lib.VIDEO_TYPE_NONE, 1 = *_MAIN, 2 = *_SCND)
     * @param requestedVideoSizeIndex (0 = 640x480, -1 = the next smaller, +1 = the next bigger)
     */
    public void cameraStart(int requestedVideoType, int requestedVideoSizeIndex) {
        if (!_cameraPermissionGranted) return;

        if (!GLES3Camera2Service.isTransitioning) {
            if (!GLES3Camera2Service.isRunning) {
                GLES3Camera2Service.isTransitioning = true;
                GLES3Camera2Service.requestedVideoSizeIndex = requestedVideoSizeIndex;

                if (requestedVideoType == GLES3Lib.VIDEO_TYPE_MAIN) {
                    GLES3Camera2Service.videoType = CameraCharacteristics.LENS_FACING_BACK;
                    Log.i(TAG, "Going to start main back camera service ...");
                } else {
                    GLES3Camera2Service.videoType = CameraCharacteristics.LENS_FACING_FRONT;
                    Log.i(TAG, "Going to start front camera service ...");
                }

                //////////////////////////////////////////////////////////////////////
                startService(new Intent(getBaseContext(), GLES3Camera2Service.class));
                //////////////////////////////////////////////////////////////////////

                _currentVideoType = requestedVideoType;
            } else {
                // if the camera is running the type or size is different we first stop the camera
                if (requestedVideoType != _currentVideoType ||
                        requestedVideoSizeIndex != GLES3Camera2Service.requestedVideoSizeIndex) {
                    GLES3Camera2Service.isTransitioning = true;
                    Log.i(TAG, "Going to stop camera service to change type ...");
                    stopService(new Intent(getBaseContext(), GLES3Camera2Service.class));
                }
            }
        }
    }

    /**
     * Stops the camera service if running.
     * It is called from the GL view renderer thread.
     * While the service is stopping no other calls to stopService are allowed.
     */
    public void cameraStop() {
        if (!_cameraPermissionGranted) return;

        if (!GLES3Camera2Service.isTransitioning) {
            if (GLES3Camera2Service.isRunning) {
                GLES3Camera2Service.isTransitioning = true;
                Log.i(TAG, "Going to stop camera service ...");

                /////////////////////////////////////////////////////////////////////
                stopService(new Intent(getBaseContext(), GLES3Camera2Service.class));
                /////////////////////////////////////////////////////////////////////
            }
        }
    }

    /**
     * Registers the the rotation sensor listener
     * It is called from the GL view renderer thread.
     */
    public void rotationSensorStart() {
        if (_rotationSensorIsRunning)
            return;

        // Init Sensor
        try {
            SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
            sm.registerListener(this,
                    sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR),
                    sm.SENSOR_DELAY_GAME);
            _rotationSensorIsRunning = true;
        } catch (Exception e) {
            Log.i(TAG, "Exception: " + e.getMessage());
            _rotationSensorIsRunning = false;
        }
    }


    /**
     * Unregisters the the rotation sensor listener
     * It is called from the GL view renderer thread.
     */
    public void rotationSensorStop() {
        if (!_rotationSensorIsRunning)
            return;

        // Init Sensor
        try {
            SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
            sm.unregisterListener(this, sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR));
            _rotationSensorIsRunning = false;
        } catch (Exception e) {
            Log.i(TAG, "Exception: " + e.getMessage());
            _rotationSensorIsRunning = false;
        }
    }

    /**
     * Starts the location manager.
     */
    @SuppressWarnings("ResourceType")
    public void gpsSensorStart() {
        // Create GPS manager and listener
        if (_gpsSensorIsRunning)
            return;

        if (gpsLocationListener == null) {
            gpsLocationListener = new GeneralLocationListener(this, "GPS");
        }

        gpsLocationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

        if (gpsLocationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            Log.i(TAG, "Requesting GPS location updates");
            gpsLocationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 1000, 0, gpsLocationListener);

            _gpsSensorIsRunning = true;
            Log.d(TAG, "State of GPS Sensor: "+ _gpsSensorIsRunning);
        }

        if (!_gpsSensorIsRunning) {
            Log.i(TAG, "No provider available!");
            _gpsSensorIsRunning = false;
            return;
        }
    }

    /**
     * Stops the location managers
     */
    @SuppressWarnings("ResourceType")
    public void gpsSensorStop() {
        if (gpsLocationListener != null) {
            Log.d(TAG, "Removing gpsLocationManager updates");
            gpsLocationManager.removeUpdates(gpsLocationListener);
            gpsLocationListener = null;
        }
    }

    /**
     * Stops location manager, then starts it.
     */
    void restartGpsManagers() {
        Log.d(TAG, "Restarting location managers");
        gpsSensorStop();
        gpsSensorStart();
    }

    /**
     * This event is raised when the GeneralLocationListener has a new location.
     * This method in turn updates notification, writes to file, reobtains
     * preferences, notifies main service client and resets location managers.
     *
     * @param loc Location object
     */
    void onLocationChanged(Location loc) {
        long currentTimeStamp = System.currentTimeMillis();

        if (!loc.hasAccuracy() || loc.getAccuracy() == 0) {
            return;
        }


        Log.i(TAG, String.valueOf(loc.getLatitude()) + "," + String.valueOf(loc.getLongitude()));
        myView.queueEvent(new Runnable() {
            public void run() {
                GLES3Lib.onLocationGPS(loc.getLatitude(), loc.getLongitude(), loc.getAltitude());
            }
        });
    }
}

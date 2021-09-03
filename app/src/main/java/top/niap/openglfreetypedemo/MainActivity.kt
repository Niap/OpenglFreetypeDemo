package top.niap.openglfreetypedemo

import android.content.res.AssetManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import android.view.*

class MainActivity : AppCompatActivity() {

    lateinit var glSurfaceView: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initAsserts(getApplicationContext().resources.assets)

        glSurfaceView =  findViewById<SurfaceView>(R.id.surfaceView)

        glSurfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                nativeSetView(glSurfaceView.holder.surface)
            }

            override fun surfaceChanged(surfaceHolder: SurfaceHolder, i: Int, width: Int, height: Int) {

            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {

            }
        })
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun nativeSetView(surface: Surface)

    external fun initAsserts(assertManager: AssetManager?)

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}